#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"

long long sz, start, real_time, k_time;

static inline long long get_ns()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_nsec + t.tv_sec;
}
int main()
{
    char buf[128];
    char write_buf[] = "testing writing";
    int offset = 92;

    int fd = open(FIB_DEV, O_RDWR);

    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }
    write(fd, write_buf, strlen(write_buf));
    for (int i = 0; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);
        start = get_ns();
        sz = read(fd, buf, 1);
        real_time = get_ns() - start;
        k_time = write(fd, write_buf, strlen(write_buf));
        printf("%d %lld %lld %lld\n", i, real_time, k_time, real_time - k_time);
        // printf("%s\n", buf);
    }
    close(fd);
    return 0;
}