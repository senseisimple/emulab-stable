#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <unistd.h>

size_t wcrtomb(char *s, wchar_t wc, mbstate_t *ps) {
    fprintf(stderr,"wcrtomb not implemented\n");
    exit(1);
    return -1;
}

int mbsinit(const mbstate_t *ps) {
    fprintf(stderr,"mbsinit not implemented\n");
    fprintf(stderr,"ps==%p\n",ps);
    exit(1);
    return 0;
}

int fdatasync(int fd) {
    return fsync(fd);
}

size_t mbsrtowcs(wchar_t *dest, const char **src, size_t len, mbstate_t *ps) {
    fprintf(stderr,"mbsrtowcs not implemented\n");
    exit(1);
    return 0;
}

size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps) {
    fprintf(stderr,"mbrtowc not implemented\n");
    exit(1);
    return 0;
}

