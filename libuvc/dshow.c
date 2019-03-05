
#include "libuvc.h"
#include <stdio.h>
#include <stdlib.h>
#include "libposix4win.h"

struct dshow_ctx {
    int fd;

};

static struct dshow_ctx *dshow_open(struct uvc_ctx *uvc, const char *dev, int width, int height)
{
    struct dshow_ctx *dc = calloc(1, sizeof(struct dshow_ctx));
    if (!dc) {
        printf("malloc dshow_ctx failed!\n");
        return NULL;
    }

    return dc;
}

struct uvc_ops dshow_ops = {
    dshow_open,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};
