/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#include "librpc.h"
#include "librpc.h"
#include "librpc_stub.h"
#include <libthread.h>
#include <libhal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static struct thread *g_rpc_thread;

#define MAX_UUID_LEN                (21)

static int on_get_connect_cnt(struct rpc_session *r, void *ibuf, size_t ilen, void **obuf, size_t *olen)
{
    struct rpcs *s = rpc_server_get_handle(r);
    int cnt = hash_get_all_cnt(s->hash_session);
    *olen = sizeof(int);
    int *tmp = calloc(1, sizeof(int));
    *tmp = cnt;
    *obuf = tmp;
    return 0;
}

static int on_get_connect_list(struct rpc_session *r, void *ibuf, size_t ilen, void **obuf, size_t *olen)
{
    int expect = *(int*)ibuf;
    struct rpcs *s = rpc_server_get_handle(r);
    struct rpc_session *ss, *session;
    int i = 0;
    int num = 0;
    int cnt = hash_get_all_cnt(s->hash_session);
    char **key = calloc(cnt, sizeof(char **));
    void **ptr = calloc(cnt, sizeof(void **));
    printf("connect cnt = %d, ibuf=%d\n", cnt, *(int*)ibuf);
    hash_dump_all(s->hash_session, &num, key, ptr);
    if (num != cnt) {
        printf("hash cnt %d does not match dump %d\n", num, cnt);
    }
    if (num != expect) {
        printf("hash cnt %d does not match expected  %d\n", num, expect);
    }
    ss = calloc(cnt, sizeof(struct rpc_session));

    printf("dump connect list:\n");
    for (i = 0; i < cnt; i++) {
        session = ptr[i];
        print_session(session);
        memcpy(&ss[i], session, sizeof(struct rpc_session));
        //printf("session[%d]dump key:val = %s:%x\n", key[i], session->uuid_src);
    }
    *obuf = ss;
    *olen = cnt*sizeof(struct rpc_session);
#if 0
    void *ptr;
    int num = 0;
    struct iovec *buf = CALLOC(1, struct iovec);
    key_list *tmp, *uuids;
    dict_get_key_list(r->dict_uuid2fd, &uuids);
    for (num = 0, tmp = uuids; tmp; tmp = tmp->next, ++num) {
    }
    uuids = NULL;
    buf->iov_len = num * MAX_UUID_LEN;
    buf->iov_base = calloc(1, buf->iov_len);
    for (ptr = buf->iov_base, tmp = uuids; tmp; tmp = tmp->next, ++num) {
        logi("uuid list: %s\n", (tmp->key));
        len = MAX_UUID_LEN;
        memcpy(ptr, tmp->key, len);
        ptr += len;
    }
    r->send_pkt.header.msg_id = RPC_GET_CONNECT_LIST;
    r->send_pkt.header.payload_len = buf->iov_len;
    logi("rpc_send len = %d, buf = %s\n", buf->iov_len, buf->iov_base);
    rpc_send(r, buf->iov_base, buf->iov_len);
#endif
    printf("%s:%d xxxx\n", __func__, __LINE__);
    return 0;
}

static int on_test(struct rpc_session *r, void *ibuf, size_t ilen, void **obuf, size_t *olen)
{
    printf("%s:%d xxxx\n", __func__, __LINE__);
    return 0;
}

static int on_test_resp(struct rpc_session *r, void *ibuf, size_t ilen, void **obuf, size_t *olen)
{
    printf("%s:%d xxxx\n", __func__, __LINE__);
    return 0;
}

static int on_peer_post_msg(struct rpc_session *r, void *ibuf, size_t ilen, void **obuf, size_t *olen)
{
    struct rpcs *s = rpc_server_get_handle(r);

    printf("post msg from %x to %x\n", r->uuid_src, r->uuid_dst);
    struct rpc_session *dst_session = (struct rpc_session *)hash_get32(s->hash_session, r->uuid_dst);
    if (!dst_session) {
        printf("hash_get failed: key=%08x\n", r->uuid_dst);
        return -1;
    }
    struct rpc_packet send_pkt;
    size_t pkt_len = pack_msg(&send_pkt, r->uuid_dst, r->uuid_src, r->msg_id, ibuf, ilen);
    if (pkt_len == 0) {
        printf("pack_msg failed!\n");
        return -1;
    }

    return rpc_send(&dst_session->base, &send_pkt);
}

static int on_peer_post_msg_resp(struct rpc_session *r, void *ibuf, size_t ilen, void **obuf, size_t *olen)
{
    printf("msg from %x :%s\n", r->uuid_dst, (char *)ibuf);
    return 0;
}

static int on_shell_help(struct rpc_session *r, void *ibuf, size_t ilen, void **obuf, size_t *olen)
{
    int ret;
    char *cmd = (char *)ibuf;
    *obuf = calloc(1, 1024);
    *olen = 1024;
    printf("on_shell_help cmd = %s\n", cmd);
    memset(*obuf, 0, 1024);
    ret = system_with_result(cmd, *obuf, 1024);
    if (ret > 0) {
        *olen = ret;
    }
    return 0;
}

static int on_shell_help_resp(struct rpc_session *r, void *ibuf, size_t ilen, void **obuf, size_t *olen)
{
 //   printf("msg from %x:\n%s\n", r->send_pkt.header.uuid_src, (char *)arg);
    return 0;
}


BEGIN_RPC_MAP(RPC_CLIENT_API)
RPC_MAP(RPC_TEST, on_test_resp)
RPC_MAP(RPC_PEER_POST_MSG, on_peer_post_msg_resp)
RPC_MAP(RPC_SHELL_HELP, on_shell_help_resp)
END_RPC_MAP()

BEGIN_RPC_MAP(RPC_SERVER_API)
RPC_MAP(RPC_TEST, on_test)
RPC_MAP(RPC_GET_CONNECT_CNT, on_get_connect_cnt)
RPC_MAP(RPC_GET_CONNECT_LIST, on_get_connect_list)
RPC_MAP(RPC_PEER_POST_MSG, on_peer_post_msg)
RPC_MAP(RPC_SHELL_HELP, on_shell_help)
END_RPC_MAP()

static int rpc_get_connect_list(struct rpc *r, int cnt)
{
    int i;
    struct rpc_session *ss;
    ss = calloc(cnt, sizeof(struct rpc_session));
    rpc_call(r, RPC_GET_CONNECT_LIST, &cnt, sizeof(int), ss, sizeof(ss));
    printf("get connect list:\n");
    for (i = 0; i < cnt; i++) {
        print_session(&ss[i]);
    }
    return 0;
}

static int rpc_get_connect_cnt(struct rpc *r, int *num)
{
    rpc_call(r, RPC_GET_CONNECT_CNT, NULL, 0, num, sizeof(int));
    return 0;
}

static int rpc_shell_help(struct rpc *r, void *buf, size_t len)
{
    char res[1024] = {0};
    rpc_call(r, RPC_SHELL_HELP, buf, len, res, sizeof(res));
    printf("return buffer\n%s", res);
    //dump_packet(&r->packet);
    return 0;
}

static int rpc_peer_post_msg(struct rpc *r, uint32_t uuid, void *buf, size_t len)
{
    rpc_client_set_dest(r, uuid);
    rpc_call(r, RPC_PEER_POST_MSG, buf, len, NULL, 0);
    //printf("func_id = %x\n", RPC_PEER_POST_MSG);
    //dump_packet(&r->packet);
    return 0;
}

static void usage(void)
{
    fprintf(stderr, "./test_libskt -s <port>\n");
    fprintf(stderr, "./test_libskt -c <ip> <port>\n");
    fprintf(stderr, "e.g. ./test_libskt -s 127.0.0.1 12345\n");
}

static void cmd_usage(void)
{
    printf("====rpc cmd====\n"
            "a: get all connect list\n"
            "p: post message to peer\n"
            "s: remote shell help\n"
            "q: quit\n"
            "\n");
}

static void *rpc_client_thread(struct thread *t, void *arg)
{
    struct rpc *r = (struct rpc *)arg;
    uint32_t uuid_dst;
    char cmd[512];
    int loop = 1;
    int i;
    int len = 1024;
    int connect_cnt = 0;
    char ch;
    char *buf = (char *)calloc(1, len);
    for (i = 0; i < len; i++) {
        buf[i] = i;
    }

    while (loop) {
        memset(buf, 0, len);
        printf("input cmd> ");
        ch = getchar();
        switch (ch) {
        case 'n':
            rpc_get_connect_cnt(r, &connect_cnt);
            printf("get connect cnt=%d\n", connect_cnt);
            break;
        case 'a':
            rpc_get_connect_cnt(r, &connect_cnt);
            printf("get connect cnt=%d\n", connect_cnt);
            rpc_get_connect_list(r, connect_cnt);
            break;
        case 'i':
            rpc_client_dump_info(r);
            break;
        case 'p':
            printf("input uuid_dst> ");
            scanf("%x", &uuid_dst);
            printf("uuid_dst = %x\n", uuid_dst);
            //r->send_pkt.header.uuid_dst = uuid_dst;
            sprintf(buf, "%s", "hello world");
            rpc_peer_post_msg(r, uuid_dst, buf, 12);
            break;
        case 'q':
            loop = 0;
            break;
        case 's':
            printf("input shell cmd> ");
            scanf("%c", &ch);
            scanf("%[^\n]", cmd);
            rpc_shell_help(r, cmd, sizeof(cmd));
            break;
        case 'h':
            cmd_usage();
            break;
        default:
            break;
        }
        //dump_buffer(buf, len);
    }

    rpc_client_destroy(r);
    exit(0);
    return NULL;
}

static void *rpc_server_thread(struct thread *t, void *arg)
{
    struct rpcs *rpcs = (struct rpcs *)arg;
    int loop = 1;
    char ch;
    while (loop) {
        printf("input cmd> ");
        ch = getchar();
        switch (ch) {
        case 'q':
            loop = 0;
            break;
        case 'h':
            cmd_usage();
            break;
        default:
            break;
        }
    }

    rpc_server_destroy(rpcs);
    exit(0);
    return NULL;
}

static int rpc_client_test(char *ip, uint16_t port)
{
    struct rpc *rpc = rpc_client_create(ip, port);
    if (!rpc) {
        printf("rpc_client_create failed\n");
        return -1;
    }
    RPC_REGISTER_MSG_MAP(RPC_CLIENT_API);
    printf("RPC_TEST:%d\n", RPC_TEST);
    printf("RPC_GET_CONNECT_LIST:%d\n", RPC_GET_CONNECT_LIST);
    printf("RPC_PEER_POST_MSG:%d\n", RPC_PEER_POST_MSG);
    printf("RPC_SHELL_HELP:%d\n", RPC_SHELL_HELP);
    printf("RPC_GET_CONNECT_CNT:%d\n", RPC_GET_CONNECT_CNT);
    g_rpc_thread = thread_create(rpc_client_thread, rpc);
    rpc_client_dispatch(rpc);
    return 0;
}

static int rpc_server_test(uint16_t port)
{
    struct rpcs *rpcs = rpc_server_create(NULL, port);
    if (rpcs == NULL) {
        printf("rpc_server_create failed!\n");
        return -1;
    }
    RPC_REGISTER_MSG_MAP(RPC_SERVER_API);
    printf("RPC_TEST:%d\n", RPC_TEST);
    printf("RPC_GET_CONNECT_LIST:%d\n", RPC_GET_CONNECT_LIST);
    printf("RPC_PEER_POST_MSG:%d\n", RPC_PEER_POST_MSG);
    printf("RPC_SHELL_HELP:%d\n", RPC_SHELL_HELP);
    printf("RPC_GET_CONNECT_CNT:%d\n", RPC_GET_CONNECT_CNT);
    g_rpc_thread = thread_create(rpc_server_thread, rpcs);
    rpc_server_dispatch(rpcs);
    return 0;
}

int main(int argc, char **argv)
{
    uint16_t port;
    char *ip;
    if (argc < 2) {
        usage();
        exit(0);
    }
    if (!strcmp(argv[1], "-s") && argc > 2) {
        port = atoi(argv[2]);
        rpc_server_test(port);
    } else if (!strcmp(argv[1], "-c") && argc > 3) {
        ip = argv[2];
        port = atoi(argv[3]);
        rpc_client_test(ip, port);
    } else {
        usage();
        exit(0);
    }
    return 0;
}
