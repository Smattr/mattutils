/* Scan for embedded file data inside another file. You can think of this as
 * `strings` for locating binary objects or a more unopinionated and exhaustive
 * version of `binwalk`.
 */

#include <fcntl.h>
#include <magic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char **argv) {

    int ret = EXIT_SUCCESS;
    void *base = NULL;
    size_t size;
    int fd = -1;

    if (argc != 2) {
        fprintf(stderr, "usage: %s path\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Init libmagic. */

    magic_t db = magic_open(MAGIC_NONE);
    if (db == NULL) {
        fprintf(stderr, "failed to open Magic database\n");
        return EXIT_FAILURE;
    }

    if (magic_load(db, NULL) < 0) {
        fprintf(stderr, "failed to load Magic database\n");
        ret = EXIT_FAILURE;
        goto end;
    }

    /* First, try to identify the file as a whole. */
    const char *filetype = magic_file(db, argv[1]);
    if (filetype != NULL)
        printf("file identified as %s\n", filetype);

    /* Now open and mmap the file to scan its contents. */

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        ret = EXIT_FAILURE;
        goto end;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        ret = EXIT_FAILURE;
        goto end;
    }
    size = st.st_size;

    base = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (base == MAP_FAILED) {
        perror("mmap");
        ret = EXIT_FAILURE;
        goto end;
    }

    /* Scan through the file, stopping one byte before the end because libmagic
     * can't conclude anything from a single byte.
     */
    for (size_t i = 0; i < size - 1; i++) {

        /* Try to identify the byte stream at the current offset. Note that we
         * filter out the unhelpful "data" result.
         */
        const char *type = magic_buffer(db, base + i, size - i);
        if (type != NULL && !!strcmp(type, "data")) {
            printf("at offset %zu: %s\n", i, type);
        }

        /* Progress output. */
        if (i % 100 == 0) {
            printf("\0337 %.2f%%\0338", (float)i / (float)size * 100);
            fflush(stdout);
        }

    }

end:
    if (base != NULL)
        munmap(base, size);
    if (fd >= 0)
        close(fd);
    magic_close(db);

    return ret;
}
