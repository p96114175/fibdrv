#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define NUM_THREADS 2
#define FIB_DEV "/dev/fibonacci"

long long sz;
void Run_fibonacci()
{
    char buf[128];
    // char write_buf[] = "testing writing";
    int offset = 92;

    int fd = open(FIB_DEV, O_RDWR);

    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }
    // write(fd, write_buf, strlen(write_buf));
    for (int i = 0; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, sizeof(buf));
        // sz = write(fd, buf, sizeof(buf));
        printf("reading from " FIB_DEV
               " at ofset %d, returned the sequence "
               "%lld.\n",
               i, sz);
    }
    close(fd);
}


int main()
{
    pthread_t threads[NUM_THREADS];
    int i;
    int result_code;

    // create all threads one by one
    for (i = 0; i < NUM_THREADS; i++) {
        printf("IN MAIN: Creating thread %d.\n", i);
        result_code = pthread_create(&threads[i], NULL,
                                     (void *(*) (void *) ) Run_fibonacci, NULL);
        assert(!result_code);
    }

    printf("IN MAIN: All threads are created.\n");

    // wait for each thread to complete
    for (i = 0; i < NUM_THREADS; i++) {
        result_code = pthread_join(threads[i], NULL);
        assert(!result_code);
        printf("IN MAIN: Thread %d has ended.\n", i);
    }

    printf("MAIN program has ended.\n");
    return 0;
}