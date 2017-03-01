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

#include "libdaio.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Main
 */
int
main(int argc, char* argv[])
{
   int res;
   void* data1;
   size_t count1;
   void* data2;
   size_t count2;
   int fd;
   int slot1, slot2;

   // Initialize the libdaio library with 8k block size, 4 channels and fdatasync
   res = daio_initialize((size_t)8192, (long)4, 2);
   if (res < 0)
   {
      printf("daio_initialize failed: %d\n", res);
      exit(1);
   }
   printf("libdaio_example: initialized\n");
   
   // Create and open a file using O_DIRECT with 644 permissions
   fd = daio_register_file("./libdaio.bin", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
   if (fd < 0)
   {
      printf("daio_register_file failed: %d\n", fd);
      exit(1);
   }
   printf("libdaio_example: registered libdaio.bin\n");
   
   // Create a write buffer
   res = daio_write_buffer(&data1, &count1);
   if (res < 0)
   {
      printf("daio_write_buffer failed: %d\n", res);
      exit(1);
   }

   // Set write buffer data to 0's
   memset(data1, 0, count1);
   
   // Write the buffer to page 0 of the file
   res = daio_submit_write(fd, data1, count1, (long)0);
   if (res < 0)
   {
      printf("daio_submit_write failed: %d\n", res);
      exit(1);
   }
   printf("libdaio_example: wrote page %d of libdaio.bin\n", 0);

   // Free write buffer
   free(data1);

   // Create a write buffer
   res = daio_write_buffer(&data1, &count1);
   if (res < 0)
   {
      printf("daio_write_buffer failed: %d\n", res);
      exit(1);
   }

   // Set write buffer data to 1's
   memset(data1, 1, count1);
   
   // Write the buffer to page 1 of the file
   res = daio_submit_write(fd, data1, count1, (long)1);
   if (res < 0)
   {
      printf("daio_submit_write failed: %d\n", res);
      exit(1);
   }
   printf("libdaio_example: wrote page %d of libdaio.bin\n", 1);

   // Free write buffer
   free(data1);

   // Submit a read request for page 0
   res = daio_submit_read(fd, (long)0, &slot1);
   if (res < 0)
   {
      printf("daio_submit_read failed: %d\n", res);
      exit(1);
   }

   // Submit a read request for page 1
   res = daio_submit_read(fd, (long)1, &slot2);
   if (res < 0)
   {
      printf("daio_submit_read failed: %d\n", res);
      exit(1);
   }

   // Read page 0 using slot1
   res = daio_read_slot(slot1, &data1, &count1);
   if (res < 0)
   {
      printf("daio_read_slot failed: %d\n", res);
      exit(1);
   }
   printf("libdaio_example: read page %d of libdaio.bin\n", 0);

   // Read page 1 using slot2
   res = daio_read_slot(slot2, &data2, &count2);
   if (res < 0)
   {
      printf("daio_read_slot failed: %d\n", res);
      exit(1);
   }
   printf("libdaio_example: read page %d of libdaio.bin\n", 1);

   // Free the read buffers
   free(data1);
   free(data2);
   
   // Deregister the file descriptor
   res = daio_deregister_file(fd);
   if (res < 0)
   {
      printf("daio_deregister_file failed: %d\n", res);
      exit(1);
   }
   printf("libdaio_example: deregistered libdaio.bin\n");

   // Destroy the libdaio data structures
   res = daio_destroy();
   if (res < 0)
   {
      printf("daio_destroy failed: %d\n", res);
      exit(1);
   }
   printf("libdaio_example: destroyed\n");

   return 0;
}
