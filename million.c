#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"

int main()
{
    char buf[10] = {0};
    // char write_buf[] = "testing writing";
    int offset = 10000;

    int fd = open(FIB_DEV, O_RDWR);

    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }
    for (int i = 0; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);
        read(fd, buf, sizeof(buf));
        printf("reading from " FIB_DEV
               " at ofset %d, returned the sequence "
               "%s.\n",
               i, buf);
    }
    close(fd);
    return 0;
}