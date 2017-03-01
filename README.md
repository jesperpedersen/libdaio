# libdaio

libdaio is a library that simplifies disk access when using ```O_DIRECT``` and
[libaio](https://git.fedorahosted.org/cgit/libaio.git "libaio Homepage").

## Installation

libdaio is compiled using ```cmake```:

```bash
cmake .
make
```

libdaio requires the [libaio](https://git.fedorahosted.org/cgit/libaio.git "libaio Homepage")
development package to be installed:

```bash
dnf install -y libaio libaio-devel
```

## API

The API for libdaio is described in ```libdaio.h```

The documentation for the library can be generated using ```doxygen``` with

```bash
make doc
```

## Usage

libdaio provides a simple way for multiple readers and a single writer to perform
disk operations using ```O_DIRECT``` and libaio.

```c
int fd;
void* data;
size_t count;
int slot;

// Initialize the libdaio library with 8k block size, 4 channels and fdatasync
daio_initialize((size_t)8192, (long)4, 2);

// Create and open a file using O_DIRECT with 644 permissions
fd = daio_register_file("./libdaio.bin", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

// Create a write buffer
daio_write_buffer(&data, &count);

// Set write buffer data to 0's
memset(data, 0, count);

// Write the buffer to page 0 of the file
daio_submit_write(fd, data, count, (long)0);

// Free write buffer
free(data);

// Submit a read request for page 0
daio_submit_read(fd, (long)0, &slot);

// Read page 0 using the slot
daio_read_slot(slot, &data, &count);

// Free the read buffer
free(data);

// Deregister the file descriptor
daio_deregister_file(fd);

// Destroy the libdaio data structures
daio_destroy();
```

See ```libdaio_example.c``` for a complete example.

## License

libdaio is released under the MIT license.

## Author

Jesper Pedersen

jesper (dot) pedersen (at) redhat (dot) com
