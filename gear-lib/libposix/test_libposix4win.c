
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

}

int main(int argc, char **argv)
{
    foo();
    return 0;
}
