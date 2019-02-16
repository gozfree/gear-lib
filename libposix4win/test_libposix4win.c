
#include "libposix4win.h"
#include <stdio.h>
#include <stdlib.h>

void foo()
{
    char proc[128] = {0};
    const char *file = "version.sh";
    struct stat st;
    memset(&st, 0, sizeof(st));
    if (0 > stat(file, &st)) {
        printf("stat %s failed\n", file);
    }
    printf("%s size=%d\n", file, st.st_size);
    get_proc_name(proc, sizeof(proc));
    printf("proc name = %s\n", proc);

}

int main(int argc, char **argv)
{
    foo();
    return 0;
}
