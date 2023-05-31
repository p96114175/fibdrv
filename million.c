#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// #include "BigNumber.h"

#define FIB_DEV "/dev/fibonacci"

char *bn_to_string(void *str, size_t size)
{
    // log10(x) = log2(x) / log2(10) ~= log2(x) / 3.322
    unsigned int *number = (unsigned int *) str;
    size_t len = (8 * sizeof(int) * size) / 3 + 2;
    char *s = malloc(len);
    char *p = s;

    memset(s, '0', len - 1);
    s[len - 1] = '\0';

    for (int i = size - 1; i >= 0; i--) {
        for (unsigned int d = 1U << 31; d; d >>= 1) {
            /* binary -> decimal string */
            int carry = !!(d & number[i]);
            for (int j = len - 2; j >= 0; j--) {
                s[j] += s[j] - '0' + carry;  // double it
                carry = (s[j] > '9');
                if (carry)
                    s[j] -= 10;
            }
        }
    }
    // skip leading zero
    while (p[0] == '0' && p[1] != '\0') {
        p++;
    }
    // if (src.sign)
    //     *(--p) = '-';
    memmove(s, p, strlen(p) + 1);
    return s;
}

int main()
{
    char buf[100000];
    // char write_buf[] = "testing writing";
    int offset = 100000;

    int fd = open(FIB_DEV, O_RDWR);

    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }
    for (int i = offset; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);
        int len = read(fd, buf, sizeof(buf));
        char *p = bn_to_string(buf, len);
        printf("reading from " FIB_DEV
               " at ofset %d, returned the sequence "
               "%s.\n",
               i, p);
    }
    close(fd);
    return 0;
}