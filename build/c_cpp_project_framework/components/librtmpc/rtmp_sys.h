#ifndef __RTMP_SYS_H__
#define __RTMP_SYS_H__
/*
 *      Copyright (C) 2010 Howard Chu
 *
 *  This file is part of librtmp.
 *
 *  librtmp is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as
 *  published by the Free Software Foundation; either version 2.1,
 *  or (at your option) any later version.
 *
 *  librtmp is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with librtmp see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *  http://www.gnu.org/copyleft/lgpl.html
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <errno.h>
#include <stdarg.h>
#include <limits.h>

#include <stdint.h>

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <Mstcpip.h>
#include <time.h>

#ifdef _MSC_VER	/* MSVC */
#define snprintf _snprintf
#define strcasecmp stricmp
#define strncasecmp strnicmp
#define vsnprintf _vsnprintf
#endif

#define GetSockError()	WSAGetLastError()
#define SetSockError(e)	WSASetLastError(e)
#define setsockopt(a,b,c,d,e)	(setsockopt)(a,b,c,(const char *)d,(int)e)
#ifdef EWOULDBLOCK
#undef EWOULDBLOCK
#endif
#define EWOULDBLOCK	WSAETIMEDOUT	/* we don't use nonblocking, but we do use timeouts */
#define sleep(n)	Sleep(n*1000)
#define msleep(n)	Sleep(n)
#define SET_RCVTIMEO(tv,s)	int tv = s*1000
#else /* !_WIN32 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#ifndef FREERTOS
#include <netinet/tcp.h>
#endif
#include <arpa/inet.h>
#define GetSockError()	errno
#define SetSockError(e)	errno = e
#undef closesocket
#define closesocket(s)	close(s)
#define msleep(n)	usleep(n*1000)
#define SET_RCVTIMEO(tv,s)	struct timeval tv = {s,0}
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif
#endif

#include "rtmp.h"

#if defined(USE_MBEDTLS)
#include <mbedtls/version.h>
#include <mbedtls/net.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

#define my_dhm_P \
    "E4004C1F94182000103D883A448B3F80" \
    "2CE4B44A83301270002C20D0321CFD00" \
    "11CCEF784C26A400F43DFB901BCA7538" \
    "F2C6B176001CF5A0FD16D2C48B1D0C1C" \
    "F6AC8E1DA6BCC3B4E1F96B0564965300" \
    "FFA1D0B601EB2800F489AA512C4B248C" \
    "01F76949A60BB7F00A40B1EAB64BDD48" \
    "E8A700D60B7F1200FA8E77B0A979DABF"

#define my_dhm_G "4"

#define	SSL_SET_SESSION(S,resume,timeout,ctx)	mbedtls_ssl_set_session(S,ctx)

typedef struct tls_ctx
{
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_config conf;
    mbedtls_ssl_session ssn;
    mbedtls_x509_crt *cacert;
    mbedtls_net_context net;
} tls_ctx;

typedef struct tls_server_ctx
{
  mbedtls_ssl_config *conf;
  mbedtls_ctr_drbg_context *ctr_drbg;
  mbedtls_pk_context key;
  mbedtls_x509_crt cert;
} tls_server_ctx;

typedef tls_ctx *TLS_CTX;

#define TLS_client(ctx,s)	\
  s = malloc(sizeof(mbedtls_ssl_context));\
  mbedtls_ssl_init(s);\
  mbedtls_ssl_setup(s, &ctx->conf);\
	mbedtls_ssl_config_defaults(&ctx->conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);\
  mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_REQUIRED);\
	mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random, &ctx->ctr_drbg)

#define TLS_server(ctx,s)\
  s = malloc(sizeof(mbedtls_ssl_context));\
  mbedtls_ssl_init(s);\
  mbedtls_ssl_setup(s, ctx->conf);\
	mbedtls_ssl_conf_endpoint(ctx->conf, MBEDTLS_SSL_IS_SERVER);\
  mbedtls_ssl_conf_authmode(ctx->conf, MBEDTLS_SSL_VERIFY_REQUIRED);\
	mbedtls_ssl_conf_rng(ctx->conf, mbedtls_ctr_drbg_random, ctx->ctr_drbg);\
	mbedtls_ssl_conf_own_cert(ctx->conf, &ctx->cert, &ctx->key);\
	mbedtls_ssl_conf_dh_param_bin(ctx->conf,\
    (const unsigned char *)my_dhm_P, strlen(my_dhm_P),\
    (const unsigned char *)my_dhm_G, strlen(my_dhm_G))

#define TLS_setfd(s,fd)	mbedtls_ssl_set_bio(s, fd, mbedtls_net_send, mbedtls_net_recv, NULL)
#define TLS_connect(s)	mbedtls_ssl_handshake(s)
#define TLS_accept(s)	mbedtls_ssl_handshake(s)
#define TLS_read(s,b,l)	mbedtls_ssl_read(s,(unsigned char *)b,l)
#define TLS_write(s,b,l)	mbedtls_ssl_write(s,(unsigned char *)b,l)
#define TLS_shutdown(s)	mbedtls_ssl_close_notify(s)
#define TLS_close(s)	mbedtls_ssl_free(s); free(s)

#elif defined(USE_POLARSSL)
#include <polarssl/version.h>
#include <polarssl/net.h>
#include <polarssl/ssl.h>
#include <polarssl/havege.h>
#if POLARSSL_VERSION_NUMBER < 0x01010000
#define havege_random	havege_rand
#endif
#if POLARSSL_VERSION_NUMBER >= 0x01020000
#define	SSL_SET_SESSION(S,resume,timeout,ctx)	ssl_set_session(S,ctx)
#else
#define	SSL_SET_SESSION(S,resume,timeout,ctx)	ssl_set_session(S,resume,timeout,ctx)
#endif
typedef struct tls_ctx
{
    havege_state hs;
    ssl_session ssn;
} tls_ctx;
typedef struct tls_server_ctx
{
    havege_state *hs;
    x509_cert cert;
    rsa_context key;
    ssl_session ssn;
    const char *dhm_P, *dhm_G;
} tls_server_ctx;

#define TLS_CTX tls_ctx *
#define TLS_client(ctx,s)	s = malloc(sizeof(ssl_context)); ssl_init(s);\
	ssl_set_endpoint(s, SSL_IS_CLIENT); ssl_set_authmode(s, SSL_VERIFY_NONE);\
	ssl_set_rng(s, havege_random, &ctx->hs);\
	ssl_set_ciphersuites(s, ssl_default_ciphersuites);\
	SSL_SET_SESSION(s, 1, 600, &ctx->ssn)
#define TLS_server(ctx,s)	s = malloc(sizeof(ssl_context)); ssl_init(s);\
	ssl_set_endpoint(s, SSL_IS_SERVER); ssl_set_authmode(s, SSL_VERIFY_NONE);\
	ssl_set_rng(s, havege_random, ((tls_server_ctx*)ctx)->hs);\
	ssl_set_ciphersuites(s, ssl_default_ciphersuites);\
	SSL_SET_SESSION(s, 1, 600, &((tls_server_ctx*)ctx)->ssn);\
	ssl_set_own_cert(s, &((tls_server_ctx*)ctx)->cert, &((tls_server_ctx*)ctx)->key);\
	ssl_set_dh_param(s, ((tls_server_ctx*)ctx)->dhm_P, ((tls_server_ctx*)ctx)->dhm_G)
#define TLS_setfd(s,fd)	ssl_set_bio(s, net_recv, &fd, net_send, &fd)
#define TLS_connect(s)	ssl_handshake(s)
#define TLS_accept(s)	ssl_handshake(s)
#define TLS_read(s,b,l)	ssl_read(s,(unsigned char *)b,l)
#define TLS_write(s,b,l)	ssl_write(s,(unsigned char *)b,l)
#define TLS_shutdown(s)	ssl_close_notify(s)
#define TLS_close(s)	ssl_free(s); free(s)


#elif defined(USE_GNUTLS)
#include <gnutls/gnutls.h>
typedef struct tls_ctx
{
    gnutls_certificate_credentials_t cred;
    gnutls_priority_t prios;
} tls_ctx;
#define TLS_CTX	tls_ctx *
#define TLS_client(ctx,s)	gnutls_init((gnutls_session_t *)(&s), GNUTLS_CLIENT); gnutls_priority_set(s, ctx->prios); gnutls_credentials_set(s, GNUTLS_CRD_CERTIFICATE, ctx->cred)
#define TLS_server(ctx,s)	gnutls_init((gnutls_session_t *)(&s), GNUTLS_SERVER); gnutls_priority_set_direct(s, "NORMAL", NULL); gnutls_credentials_set(s, GNUTLS_CRD_CERTIFICATE, ctx)
#define TLS_setfd(s,fd)	gnutls_transport_set_ptr(s, (gnutls_transport_ptr_t)(long)fd)
#define TLS_connect(s)	gnutls_handshake(s)
#define TLS_accept(s)	gnutls_handshake(s)
#define TLS_read(s,b,l)	gnutls_record_recv(s,b,l)
#define TLS_write(s,b,l)	gnutls_record_send(s,b,l)
#define TLS_shutdown(s)	gnutls_bye(s, GNUTLS_SHUT_RDWR)
#define TLS_close(s)	gnutls_deinit(s)

#elif defined(USE_ONLY_MD5)
#include "md5.h"
#include "cencode.h"
#define MD5_DIGEST_LENGTH 16

#else	/* USE_OPENSSL */
#define TLS_CTX	SSL_CTX *
#define TLS_client(ctx,s)	s = SSL_new(ctx)
#define TLS_server(ctx,s)	s = SSL_new(ctx)
#define TLS_setfd(s,fd)	SSL_set_fd(s,fd)
#define TLS_connect(s)	SSL_connect(s)
#define TLS_accept(s)	SSL_accept(s)
#define TLS_read(s,b,l)	SSL_read(s,b,l)
#define TLS_write(s,b,l)	SSL_write(s,b,l)
#define TLS_shutdown(s)	SSL_shutdown(s)
#define TLS_close(s)	SSL_free(s)

#endif
#endif
