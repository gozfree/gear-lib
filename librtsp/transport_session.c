
#include "transport_session.h"
#include "media_source.h"
#include "librtp.h"
#include <liblog.h>
#include <libdict.h>
#include <libmacro.h>
#include <libatomic.h>
#include <strings.h>
#include <sys/time.h>


void *transport_session_pool_create()
{
    return (void *)dict_new();
}

void transport_session_pool_destroy(void *pool)
{
    int rank = 0;
    char *key, *val;
    while (1) {
        rank = dict_enumerate((dict *)pool, rank, &key, &val);
        if (rank < 0) {
            break;
        }
        free(val);
    }
    dict_free((dict *)pool);
}

static uint32_t get_random_number()
{
    struct timeval now = {0};
    gettimeofday(&now, NULL);
    srand(now.tv_usec);
    return (rand() % ((uint32_t)-1));
}

struct transport_session *transport_session_create(void *pool, struct transport_header *t)
{
    char key[9];
    loge("mode = %d\n", t->mode);
    loge("multicast = %d\n", t->multicast);
    loge("destination = %s\n", t->destination);
    loge("source = %s\n", t->source);
    loge("client.port = %d, %d\n", t->rtp.u.client_port1, t->rtp.u.client_port2);
    struct transport_session *s = CALLOC(1, struct transport_session);
    s->session_id = get_random_number();
    snprintf(key, sizeof(key), "%08X", s->session_id);
    s->rtp = rtp_create(90000, 0);
    s->rtp->sock = rtp_socket_create(t->mode, 0, NULL);
    dict_add((dict *)pool, key, (char *)s);
    return s;
}

void transport_session_destroy(void *pool, char *key)
{
    struct transport_session *s = transport_session_lookup(pool, key);
    if (!s) {
        return;
    }
    rtp_socket_destroy(s->rtp->sock);
    dict_del((dict *)pool, key);
}

struct transport_session *transport_session_lookup(void *pool, char *key)
{
    return (struct transport_session *)dict_get((dict *)pool, key, NULL);
}

int transport_session_start(struct transport_session *ts, struct media_source *ms)
{
#if 0
    struct gevent *e = NULL;
    ts->evbase = gevent_base_create();
    if (!ts->evbase) {
        goto failed;
    }
    e = gevent_create(ts->rtp.sock.rtcp_fd, on_connect, NULL, on_error, NULL);
    if (-1 == gevent_add(ts->evbase, e)) {
        loge("event_add failed!\n");
        gevent_destroy(e);
    }
#endif
    return 0;
}
