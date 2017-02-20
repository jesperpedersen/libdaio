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

#ifndef LIBDAIO_H
#define LIBDAIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <fcntl.h>

// libdaio uses O_DIRECT and libaio for disk access, and support multiple read and
// single write requests per cycle triggered by daio_read_slot and daio_submit_write.

/**
 * Initialize the libdaio structures
 * @param block_size The block size used, must be a multiple of 512
 * @param channels The maximum number of I/O requests processed per cycle
 * @param sync The disk synchronization level; 0 = nothing, 1 = fsync, 2 = fdatasync
 * @return The status code for the initialization
 */
int
daio_initialize(size_t block_size, long channels, int sync);

/**
 * Destroy the libdaio structures
 * @return The status code for the destruction
 */
int
daio_destroy();

/**
 * Register a file path with libdaio
 * @param file_path The path of the file
 * @param mode The mode that the file should be created / open in (symbolic constants)
 * @return The resulting file descriptor, or -1 if error
 */
int
daio_register_file(const char* file_path, mode_t mode);

/**
 * Deregister a file descriptor with libdaio
 * @param fd The file descriptor
 * @return The status code for the deregistration
 */
int
daio_deregister_file(int fd);

/**
 * Get the number of available slots
 * @return The value
 */
int
daio_get_available_slots();

/**
 * Submit a read request
 * @param fd The file descriptor
 * @param page The page of the file
 * @param slot The resulting slot allocation
 * @return The status code for the read request
 */
int
daio_submit_read(int fd, long page, int* slot);

/**
 * Read the data for a slot
 * @param slot The slot
 * @param data The data read
 * @param count The number of bytes read
 * @return The status code for the read request
 */
int
daio_read_slot(int slot, void** data, size_t* count);

/**
 * Allocate a write buffer
 * @param buffer The write buffer allocated
 * @param count The size of the write buffer
 * @return The status code for the write buffer allocation request
 */
int
daio_write_buffer(void** buffer, size_t* count);

/**
 * Submit a write request
 * @param fd The file descriptor
 * @param buffer The buffer to be written
 * @param count The size of the buffer
 * @param page The page of the file
 * @return The status code for the write request
 */
int
daio_submit_write(int fd, void* buffer, size_t count, long page);

#ifdef __cplusplus
}
#endif

#endif
