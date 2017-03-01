/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Jesper Pedersen <jesper.pedersen@redhat.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "libdaio.h"

#include <unistd.h>
#include <stdlib.h>
#include <libaio.h>
#include <pthread.h>

/*
 * io_engine
 */
struct io_engine
{
   // Lock
   pthread_mutex_t lock;

   // Block size
   size_t block_size;

   // Number of channels
   long channels;

   // Sync type 0: No sync, 1: fsync, 2: fdatasync
   int sync;
   
   // I/O context
   io_context_t context;

   // The number of slots in use
   int in_use;

   // List of slot registrations
   int* slots;

   // List of iocb instances
   struct iocb** iocbs;

   // List of io event instances
   struct io_event* events;

   // Ready for read
   int ready;
   
   // List of data instances
   void** data;
};

static struct io_engine* engine;

/**
 * Is empty. The lock must be owned
 * @return 1 if empty, otherwise 0
 */
static int
is_empty()
{
   int i;

   for (i = 0; i < engine->channels; i++)
   {
      if (engine->slots[i] == 1)
         return 0;
   }

   return 1;
}

/**
 * Acquire a slot. The lock must be owned
 * @param extra Can an additional slot be requested
 * @return The slot number, or -1 if none available
 */
static int
acquire_slot(int extra)
{
   int i;
   int to;
   int slot = -1;

   to = engine->channels + extra;

   for (i = 0; i < to && slot == -1; i++)
   {
      if (engine->slots[i] == 0)
      {
         engine->slots[i] = 1;
         slot = i;
      }
   }

   if (slot != -1 && !extra)
      engine->in_use++;
   
   return slot;
}

/**
 * Release a slot. The lock must be owned
 * @param slot The slot number
 * @param extra Can an additional slot be requested
 * @return The number of used slots
 */
static int
release_slot(int slot, int extra)
{
   engine->slots[slot] = 0;
   if (!extra)
      engine->in_use--;

   return engine->in_use;
}

/**
 * Process the submitted requests
 * @return 0 upon success, otherwise error number
 */
static int
process()
{
   int res;

   res = io_submit(engine->context, engine->in_use, engine->iocbs);
   if (res < 0)
   {
      return res;
   }
   else
   {
      // We use 'res' from io_submit as the minimum value
      res = io_getevents(engine->context, res, engine->channels, engine->events, NULL);
      if (res < 0)
         return res;
   }

   return 0;
}

int
daio_initialize(size_t bs, long chns, int sc)
{
   int i;
   int c;
   int res;

   // Verify block_size >= 512 and block_size % 512 == 0
   if (bs < 512 || bs % 512 != 0)
      return -1;
   
   // Verify channels >= 1
   if (chns < 1)
      return -1;

   // Verify sync >= 0 and sync < 3
   if (sc < 0 || sc > 2)
      return -1;

   // Allocate an extra slot in case of sync usage
   c = sc == 0 ? chns : chns + 1;

   pthread_mutex_t l;
   res = pthread_mutex_init(&l, NULL);
   if (res < 0)
      return res;

   io_context_t ctx;
   memset(&ctx, 0, sizeof(ctx));
   res = io_setup(c, &ctx);
   if (res < 0)
      return res;

   int* ss = (int*)malloc((sizeof(int) * (size_t)c));
   if (ss == NULL)
      return -1;
   memset(ss, 0, sizeof(int) * (size_t)c);

   for (i = 0; i < c; i++)
   {
      ss[i] = 0;
   }
   
   struct iocb** cbs = (struct iocb**)malloc((sizeof(struct iocb*) * (size_t)c));
   if (cbs == NULL)
      return -1;
   memset(cbs, 0, sizeof(struct iocb*) * (size_t)c);

   for (i = 0; i < c; i++)
   {
      cbs[i] = (struct iocb *)malloc(sizeof(struct iocb));
      if (cbs[i] == NULL)
      {
         return -1;
      }
      memset(cbs[i], 0, sizeof(struct iocb));
   }

   struct io_event* evts = (struct io_event *)malloc(sizeof(struct io_event) * (size_t)c);
   if (evts == NULL)
      return -1;
   memset(evts, 0, sizeof(struct io_event) * (size_t)c);

   void** ds = (void**)malloc((sizeof(void*) * (size_t)c));
   if (ds == NULL)
      return -1;
   memset(ds, 0, sizeof(void *) * (size_t)c);

   for (i = 0; i < c; i++)
   {
      res = posix_memalign(&ds[i], (size_t)sysconf(_SC_PAGESIZE), bs);
      if (res < 0 || ds[i] == 0)
         return res;
      memset(ds[i], 0, bs);
   }

   // Build engine
   engine = (struct io_engine *)malloc(sizeof(struct io_engine));
   engine->lock = l;
   engine->block_size = bs;
   engine->channels = chns;
   engine->sync = sc;
   engine->context = ctx;
   engine->in_use = 0;
   engine->slots = ss;
   engine->iocbs = cbs;
   engine->events = evts;
   engine->ready = 0;
   engine->data = ds;

   return 0;
}

int
daio_destroy()
{
   int i;
   int c;
   int res1 = -1, res2 = -1;

   if (engine)
   {
      c = engine->sync == 0 ? engine->channels : engine->channels + 1;

      res1 = pthread_mutex_destroy(&engine->lock);

      res2 = io_destroy(engine->context);

      if (engine->iocbs)
      {
         for (i = 0; i < c; i++)
         {
            free(engine->iocbs[i]);
         }
      }

      if (engine->data)
      {
         for (i = 0; i < c; i++)
         {
            free(engine->data[i]);
         }
      }

      free(engine->slots);
      free(engine->iocbs);
      free(engine->events);
      free(engine->data);
      free(engine);
   }

   if (res1 < 0)
      return res1;
   
   if (res2 < 0)
      return res2;
   
   return 0;
}

int
daio_register_file(const char* filepath, mode_t mode)
{
   off_t res;
   int fd;

   fd = open(filepath, O_RDWR | O_CREAT | O_DIRECT, mode);

   res = lseek(fd, (off_t)0, SEEK_SET);
   if (res < 0)
      return -1;

   return fd;
}

int
daio_deregister_file(int fd)
{
   int res;

   res = close(fd);

   return res;
}

int
daio_get_available_slots()
{
   int num = 0;

   pthread_mutex_lock(&engine->lock);
   num = engine->channels - engine->in_use;
   pthread_mutex_unlock(&engine->lock);

   return num;
}

int
daio_submit_read(int fd, long page, int* slot)
{
   int sl = -1;

   pthread_mutex_lock(&engine->lock);
   if (!engine->ready)
      sl = acquire_slot(0);
   pthread_mutex_unlock(&engine->lock);

   if (sl != -1)
   {
      io_prep_pread(engine->iocbs[sl], fd, engine->data[sl],
                    engine->block_size, (long long)page * engine->block_size);

      *slot = sl;
      return 0;
   }

   return -1;
}

int
daio_read_slot(int slot, void** data, size_t* count)
{
   int i, res, rdy;
   void* d;
   size_t c;

   pthread_mutex_lock(&engine->lock);
   if (!engine->ready)
   {
      if (!is_empty())
      {
         res = process();
         if (res < 0)
         {
            pthread_mutex_unlock(&engine->lock);
            return -1;
         }
         engine->ready = 1;
      }
   }
   rdy = engine->ready;
   pthread_mutex_unlock(&engine->lock);

   if (rdy != 1)
      return -1;

   if (engine->slots[slot] == 1)
   {
      d = engine->data[slot];
      c = (size_t)((&(engine->events[slot]))->res);
   
      // Detach memory segment for next invocation
      i = posix_memalign(&engine->data[slot], (size_t)sysconf(_SC_PAGESIZE), engine->block_size);
      if (i < 0 || engine->data[slot] == 0)
      {
         pthread_mutex_lock(&engine->lock);
         if (release_slot(slot, 0) == 0)
            engine->ready = 0;
         pthread_mutex_unlock(&engine->lock);

         return -1;
      }
      memset(engine->data[slot], 0, engine->block_size);
   
      pthread_mutex_lock(&engine->lock);
      if (release_slot(slot, 0) == 0)
         engine->ready = 0;
      pthread_mutex_unlock(&engine->lock);

      if (d)
      {
         *data = d;
         *count = c;
         return 0;
      }
   }

   return -1;
}

int
daio_write_buffer(void** data, size_t* count)
{
   int res;
   void* d = NULL;
   size_t c;

   c = engine->block_size;
   res = posix_memalign(&d, (size_t)sysconf(_SC_PAGESIZE), c);

   if (res >= 0)
   {
      memset(d, 0, c);

      *data = d;
      *count = c;
      return 0;
   }

   return res;
}

int
daio_submit_write(int fd, void* buffer, size_t count, long page)
{
   int res, rdy;
   int write_slot = -1;
   int sync_slot = -1;

   pthread_mutex_lock(&engine->lock);
   rdy = engine->ready;
   pthread_mutex_unlock(&engine->lock);

   if (rdy == 1)
      return -1;
   
   pthread_mutex_lock(&engine->lock);
   write_slot = acquire_slot(0);

   if (write_slot != -1)
   {
      io_prep_pwrite(engine->iocbs[write_slot], fd, buffer, count, (long long)page * engine->block_size);

      if (engine->sync != 0)
      {
         sync_slot = acquire_slot(1);

         if (engine->sync == 1)
         {
            io_prep_fsync(engine->iocbs[sync_slot], fd);
         }
         else if (engine->sync == 2)
         {
            io_prep_fdsync(engine->iocbs[sync_slot], fd);
         }
      }

      res = process();
      if (res >= 0)
      {
         if (sync_slot != -1)
            release_slot(sync_slot, 1);

         if (release_slot(write_slot, 0) != 0)
            engine->ready = 1;
      
         pthread_mutex_unlock(&engine->lock);
         return 0;
      }
   }

   pthread_mutex_unlock(&engine->lock);
   return -1;
}
