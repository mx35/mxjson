/*
 * ----------------------------------------------------------------------
 * |\ /| mxjson.c
 * | X | Parse a JSON file
 * |/ \|
 * ----------------------------------------------------------------------
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mxjson.h"


/**
 * Map a file into memory as read-only.
 *
 * @param[in] filename
 *   The name of the file to map in.
 *
 * @param[out] size
 *   The size of the file.
 *
 * @return
 *   Pointer to the memory location where the file is mapped, or NULL
 *   if the file could not be mapped.
 */
static void *
map_file (const char *filename, size_t *size)
{
    int          fd;
    struct stat  sb;
    int          res;
    size_t       filesize = 0;
    void        *buffer = NULL;

    fd = open(filename, O_RDONLY);

    if (fd != -1) {
        res = fstat (fd, &sb);

        if (res == 0 && S_ISREG(sb.st_mode) && sb.st_size != 0) {
            filesize = sb.st_size;
            buffer = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);

            if (buffer == MAP_FAILED) {
                buffer = NULL;
            }
        }

        res = close(fd);
        assert(res == 0);
    }

    *size = filesize;

    return (buffer);
}


/**
 * Unmap a previously mapped file.
 *
 * @param[in] buffer
 *   Pointer to the memory location where the file is mapped.
 *
 * @param[in] size
 *   The size of the mapped memory.
 */
static void
unmap_file(void *buffer, size_t size)
{
    int res;

    res = munmap(buffer, size);
    assert(res == 0);
}


/**
 * The process is expected to be invoked with a single parameter containing
 * the name of the file to parse.
 *
 * An exit code of 0 indicates the file contains valid JSON. A non-zero
 * exit code indicates either that an error occurred, or that the JSON is
 * not valid.
 */
int main (int argc, char **argv)
{
    void            *buf;
    size_t           size;
    mxjson_parser_t  p;
    mxstr_t          json;
    bool             ok = false;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    } else {
        buf = map_file(argv[1], &size);

        if (buf != NULL) {
            json = mxstr(buf, size);

            mxjson_init(&p, 1024, NULL, mxjson_resize);
            ok = mxjson_parse(&p, json);
            mxjson_free(&p);

            unmap_file(buf, size);
        } else {
            fprintf(stderr, "%s: %s: %s\n", argv[0], argv[1], strerror(errno));
        }
    }

    return (!ok);
}
