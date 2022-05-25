
#include "libmqttc.h"
#include <libsock_ext.h>
#include <libringbuffer.h>
#include <unistd.h>


struct sock_ctx {
    struct sock_client *sc;
    struct ringbuffer *rbuf;
};

static void on_connect_client(struct sock_client *c, struct sock_connection *conn)
{
    printf("on_connect_client: fd=%d local=%s:%d, remote=%s:%d\n", conn->fd,
            conn->local.ip_str, conn->local.port,
            conn->remote.ip_str, conn->remote.port);
}

static void on_recv_buf(struct sock_client *c, void *buf, size_t len)
{
    struct sock_ctx *ctx = (struct sock_ctx *)c->priv;
    //printf("%s:%d fd = %d, recv buf len = %ld\n", __func__, __LINE__, c->fd, len);
    rb_write(ctx->rbuf, buf, len);
}

static void *sock_init(const char *host, uint16_t port)
{
    struct sock_ctx *ctx = calloc(1, sizeof(struct sock_ctx));
    if (!ctx) {
        printf("malloc sock_ctx failed!\n");
        return NULL;
    }
    ctx->sc = sock_client_create(host, port, SOCK_TYPE_TCP);
    ctx->rbuf = rb_create(1024);

    //sock_set_noblk(ctx->sc->fd, 1);
    sock_client_set_callback(ctx->sc, on_connect_client, on_recv_buf, NULL);
    ctx->sc->priv = ctx;
    return ctx;
}

static int sock_connect(struct mqtt_client *c)
{
    struct sock_ctx *ctx = (struct sock_ctx *)c->priv;
    sock_client_connect(ctx->sc);
    return 0;
}

static int sock_write(struct mqtt_client *c, const void *buf, size_t len)
{
    struct sock_ctx *ctx = (struct sock_ctx *)c->priv;
    int ret = 0;
    ret = sock_send(ctx->sc->fd, buf, len);

    //printf("%s:%d write len=%d, ret = %d\n", __func__, __LINE__, len, ret);
    return ret;
}

static int sock_read(struct mqtt_client *c, void *buf, size_t len)
{
    struct sock_ctx *ctx = (struct sock_ctx *)c->priv;
    usleep(5000);
    int ret = rb_read(ctx->rbuf, buf, len);
    //printf("%s:%d sock read len = %ld, ret=%d\n", __func__, __LINE__, len, ret);
    return ret;
}

struct mqttc_ops mqttc_socket_ops = {
    .init = sock_init,
    .connect = sock_connect,
    .write   = sock_write,
    .read    = sock_read,
};
