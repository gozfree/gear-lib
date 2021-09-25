
#include "libposix.h"
#include <stdio.h>
#include <stdlib.h>

static void *thread_func(void *arg)
{
    printf("this is thread\n");
    return NULL;
}

void foo()
{
    int ret;
    int written = 0;
    char buf[10];
    int fds[2];
    pthread_t tid;
    char proc[128] = {0};
    const char *file = "Makefile";
    struct stat st;
    memset(&st, 0, sizeof(st));
    if (0 > stat(file, &st)) {
        printf("stat %s failed\n", file);
    }
    printf("%s size=%d\n", file, st.st_size);
    get_proc_name(proc, sizeof(proc));
    printf("proc name = %s\n", proc);

    pthread_create(&tid, NULL, thread_func, NULL);
    sleep(1);
    pthread_join(tid, NULL);

    memset(buf, 0, sizeof(buf));
    pipe(fds);
    printf("fds[0]=%d, fds[1]=%d\n", fds[0], fds[1]);
    ret = write(fds[1], "aaa", 3);
    printf("write ret = %d\n", ret);
    read(fds[0], buf, 5);
    printf("buf = %s\n", buf);
}

int main(int argc, char **argv)
{
    foo();
    sleep(10);
    return 0;
}
