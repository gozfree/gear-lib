/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libptcp.c
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-10 00:15
 * updated: 2015-08-10 00:15
 *****************************************************************************/
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <inttypes.h>
#include <pthread.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <libmacro.h>
#include <liblog.h>
#include "queue.h"
#include "libptcp.h"

#ifndef TRUE
#define TRUE (1 == 1)
#endif

#ifndef FALSE
#define FALSE (0 == 1)
#endif

#define MAX_EPOLL_EVENT 16

typedef enum {
    TCP_LISTEN,
    TCP_SYN_SENT,
    TCP_SYN_RECEIVED,
    TCP_ESTABLISHED,
    TCP_CLOSED,
    TCP_FIN_WAIT_1,
    TCP_FIN_WAIT_2,
    TCP_CLOSING,
    TCP_TIME_WAIT,
    TCP_CLOSE_WAIT,
    TCP_LAST_ACK,
} ptcp_state_t;


//////////////////////////////////////////////////////////////////////
// Network Constants
//////////////////////////////////////////////////////////////////////

// Standard MTUs
static const uint16_t PACKET_MAXIMUMS[] = {
  65535,    // Theoretical maximum, Hyperchannel
  32000,    // Nothing
  17914,    // 16Mb IBM Token Ring
  8166,   // IEEE 802.4
  //4464,   // IEEE 802.5 (4Mb max)
  4352,   // FDDI
  //2048,   // Wideband Network
  2002,   // IEEE 802.5 (4Mb recommended)
  //1536,   // Expermental Ethernet Networks
  //1500,   // Ethernet, Point-to-Point (default)
  1492,   // IEEE 802.3
  1006,   // SLIP, ARPANET
  //576,    // X.25 Networks
  //544,    // DEC IP Portal
  //512,    // NETBIOS
  508,    // IEEE 802/Source-Rt Bridge, ARCNET
  296,    // Point-to-Point (low delay)
  //68,     // Official minimum
  0,      // End of list marker
};

#define MAX_PACKET 65535
// Note: we removed lowest level because packet overhead was larger!
#define MIN_PACKET 296

// (+ up to 40 bytes of options?)
#define IP_HEADER_SIZE 20
#define ICMP_HEADER_SIZE 8
#define UDP_HEADER_SIZE 8
// TODO: Make JINGLE_HEADER_SIZE transparent to this code?
// when relay framing is in use
#define JINGLE_HEADER_SIZE 64

//////////////////////////////////////////////////////////////////////
// Global Constants and Functions
//////////////////////////////////////////////////////////////////////
//
//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  0 |                      Conversation Number                      |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  4 |                        Sequence Number                        |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//  8 |                     Acknowledgment Number                     |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    |               |   |U|A|P|R|S|F|                               |
// 12 |    Control    |   |R|C|S|S|Y|I|            Window             |
//    |               |   |G|K|H|T|N|N|                               |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 16 |                       Timestamp sending                       |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 20 |                      Timestamp receiving                      |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// 24 |                             data                              |
//    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//////////////////////////////////////////////////////////////////////

#define MAX_SEQ 0xFFFFFFFF
#define HEADER_SIZE 24

#define PACKET_OVERHEAD (HEADER_SIZE + UDP_HEADER_SIZE + \
      IP_HEADER_SIZE + JINGLE_HEADER_SIZE)

// MIN_RTO = 250 ms (RFC1122, Sec 4.2.3.1 "fractions of a second")
#define MIN_RTO      250
#define DEF_RTO     1000 /* 1 seconds (RFC 6298 sect 2.1) */
#define MAX_RTO    60000 /* 60 seconds */
#define DEFAULT_ACK_DELAY    100 /* 100 milliseconds */
#define DEFAULT_NO_DELAY     FALSE

#define DEFAULT_RCV_BUF_SIZE (600 * 1024)
#define DEFAULT_SND_BUF_SIZE (900 * 1024)

/* NOTE: This must fit in 8 bits. This is used on the wire. */
typedef enum {
  /* Google-provided options: */
  TCP_OPT_EOL = 0,  /* end of list */
  TCP_OPT_NOOP = 1,  /* no-op */
  TCP_OPT_MSS = 2,  /* maximum segment size */
  TCP_OPT_WND_SCALE = 3,  /* window scale factor */
  /* libnice extensions: */
  TCP_OPT_FIN_ACK = 254,  /* FIN-ACK support */
} TcpOption;


/*
#define FLAG_SYN 0x02
#define FLAG_ACK 0x10
*/

/* NOTE: This must fit in 5 bits. This is used on the wire. */
typedef enum {
  FLAG_NONE = 0,
  FLAG_FIN = 1 << 0,
  FLAG_CTL = 1 << 1,
  FLAG_RST = 1 << 2,
} TcpFlags;

#define CTL_CONNECT  0
//#define CTL_REDIRECT  1
#define CTL_EXTRA 255


#define CTRL_BOUND 0x80000000

/* Maximum segment lifetime (1 minute).
 * RFC 793, §3.3 specifies 2 minutes; but Linux uses 1 minute, so let’s go with
 * that. */
#define TCP_MSL (60 * 1000)

// If there are no pending clocks, wake up every 4 seconds
#define DEFAULT_TIMEOUT 4000
// If the connection is closed, once per minute
#define CLOSED_TIMEOUT (60 * 1000)
/* Timeout after reaching the TIME_WAIT state, in milliseconds.
 * See: RFC 1122, §4.2.2.13.
 *
 * XXX: Since we can control the underlying layer’s channel ID, we can guarantee
 * delayed segments won’t affect subsequent connections, so can radically
 * shorten the TIME-WAIT timeout (to the extent that it basically doesn’t
 * exist). It would normally be (2 * TCP_MSL). */
#define TIME_WAIT_TIMEOUT 1

//////////////////////////////////////////////////////////////////////
// Helper Functions
//////////////////////////////////////////////////////////////////////
#ifndef G_OS_WIN32
#undef min
#undef max
#  define min(first, second) ((first) < (second) ? (first) : (second))
#  define max(first, second) ((first) > (second) ? (first) : (second))
#endif

static uint32_t bound(uint32_t lower, uint32_t middle, uint32_t upper)
{
   return min(max(lower, middle), upper);
}

static int time_is_between(uint32_t later, uint32_t middle, uint32_t earlier)
{
  if (earlier <= later) {
    return ((earlier <= middle) && (middle <= later));
  } else {
    return !((later < middle) && (middle < earlier));
  }
}

static int32_t time_diff(uint32_t later, uint32_t earlier)
{
  uint32_t LAST = 0xFFFFFFFF;
  uint32_t HALF = 0x80000000;
  if (time_is_between(earlier + HALF, later, earlier)) {
    if (earlier <= later) {
      return (long)(later - earlier);
    } else {
      return (long)(later + (LAST - earlier) + 1);
    }
  } else {
    if (later <= earlier) {
      return -(long) (earlier - later);
    } else {
      return -(long)(earlier + (LAST - later) + 1);
    }
  }
}

////////////////////////////////////////////////////////
// PseudoTcpFifo works exactly like FifoBuffer in libjingle
////////////////////////////////////////////////////////


typedef struct {
  uint8_t *buffer;
  size_t buffer_length;
  size_t data_length;
  size_t read_position;
} PseudoTcpFifo;


static void ptcp_fifo_init(PseudoTcpFifo *b, size_t size)
{
  b->buffer = (uint8_t *)calloc(1, size);
  b->buffer_length = size;
}

static void ptcp_fifo_clear(PseudoTcpFifo *b)
{
  if (b->buffer) {
    free(b->buffer);
  }
  b->buffer = NULL;
  b->buffer_length = 0;
}

static size_t ptcp_fifo_get_buffered(PseudoTcpFifo *b)
{
  return b->data_length;
}

static int ptcp_fifo_set_capacity(PseudoTcpFifo *b, size_t size)
{
  if (b->data_length > size)
    return FALSE;

  if (size != b->data_length) {
    uint8_t *buffer = calloc(1, size);
    size_t copy = b->data_length;
    size_t tail_copy = min(copy, b->buffer_length - b->read_position);

    memcpy(buffer, &b->buffer[b->read_position], tail_copy);
    memcpy(buffer + tail_copy, &b->buffer[0], copy - tail_copy);
    free(b->buffer);
    b->buffer = buffer;
    b->buffer_length = size;
    b->read_position = 0;
  }

  return TRUE;
}

static void ptcp_fifo_consume_read_data(PseudoTcpFifo *b, size_t size)
{
  assert(size <= b->data_length);

  b->read_position = (b->read_position + size) % b->buffer_length;
  b->data_length -= size;
}

static void ptcp_fifo_consume_write_buffer(PseudoTcpFifo *b, size_t size)
{
  assert(size <= b->buffer_length - b->data_length);

  b->data_length += size;
}

static size_t ptcp_fifo_get_write_remaining(PseudoTcpFifo *b)
{
  return b->buffer_length - b->data_length;
}

static size_t ptcp_fifo_read_offset(PseudoTcpFifo *b, uint8_t *buffer, size_t bytes,
    size_t offset)
{
  size_t available = b->data_length - offset;
  size_t read_position = (b->read_position + offset) % b->buffer_length;
  size_t copy = min (bytes, available);
  size_t tail_copy = min(copy, b->buffer_length - read_position);

  /* EOS */
  if (offset >= b->data_length)
    return 0;

  memcpy(buffer, &b->buffer[read_position], tail_copy);
  memcpy(buffer + tail_copy, &b->buffer[0], copy - tail_copy);

  return copy;
}

static size_t ptcp_fifo_write_offset(PseudoTcpFifo *b, const uint8_t *buffer,
    size_t bytes, size_t offset)
{
  size_t available = b->buffer_length - b->data_length - offset;
  size_t write_position = (b->read_position + b->data_length + offset)
      % b->buffer_length;
  size_t copy = min (bytes, available);
  size_t tail_copy = min(copy, b->buffer_length - write_position);

  if (b->data_length + offset >= b->buffer_length) {
    return 0;
  }

  memcpy(&b->buffer[write_position], buffer, tail_copy);
  memcpy(&b->buffer[0], buffer + tail_copy, copy - tail_copy);

  return copy;
}

static size_t ptcp_fifo_read(PseudoTcpFifo *b, uint8_t *buffer, size_t bytes)
{
  size_t copy;

  copy = ptcp_fifo_read_offset (b, buffer, bytes, 0);

  b->read_position = (b->read_position + copy) % b->buffer_length;
  b->data_length -= copy;

  return copy;
}

static size_t ptcp_fifo_write(PseudoTcpFifo *b, const uint8_t *buffer, size_t bytes)
{
  size_t copy;

  copy = ptcp_fifo_write_offset (b, buffer, bytes, 0);
  b->data_length += copy;

  return copy;
}


//////////////////////////////////////////////////////////////////////
// PseudoTcp
//////////////////////////////////////////////////////////////////////

/* Only used if FIN-ACK support is disabled. */
typedef enum {
  SD_NONE,
  SD_GRACEFUL,
  SD_FORCEFUL
} Shutdown;

typedef enum {
  sfNone,
  sfDelayedAck,
  sfImmediateAck,
  sfFin,
  sfRst,
} SendFlags;

typedef struct {
  uint32_t conv, seq, ack;
  TcpFlags flags;
  uint16_t wnd;
  const char * data;
  uint32_t len;
  uint32_t tsval, tsecr;
} Segment;

typedef struct {
  uint32_t seq, len;
  uint8_t xmit;
  TcpFlags flags;
} SSegment;

typedef struct {
  uint32_t seq, len;
} RSegment;

/**
 * ClosedownSource:
 * @CLOSEDOWN_LOCAL: Error detected locally, or connection forcefully closed
 * locally.
 * @CLOSEDOWN_REMOTE: RST segment received from the peer.
 *
 * Reasons for calling closedown().
 *
 * Since: UNRELEASED
 */
typedef enum {
  CLOSEDOWN_LOCAL,
  CLOSEDOWN_REMOTE,
} ClosedownSource;


struct _ptcp_socket {
/*************************/
/*extended paraments*/
  int on_read_fd;
  int on_write_fd;
  int fd;
  struct sockaddr_in si;
  timer_t timer_id;
  sem_t sem;
/**************************/
  ptcp_callbacks_t callbacks;

  Shutdown shutdown;  /* only used if !support_fin_ack */
  int shutdown_reads;
  int error;

  // TCB data
  ptcp_state_t state;
  uint32_t conv;
  int bReadEnable, bWriteEnable, bOutgoing;
  uint32_t last_traffic;

  // Incoming data
  list_t *rlist;
  uint32_t rbuf_len, rcv_nxt, rcv_wnd, lastrecv;
  uint8_t rwnd_scale; // Window scale factor
  PseudoTcpFifo rbuf;

  // Outgoing data
  queue_t slist;
  queue_t unsent_slist;
  uint32_t sbuf_len, snd_nxt, snd_wnd, lastsend;
  uint32_t snd_una;  /* oldest unacknowledged sequence number */
  uint8_t swnd_scale; // Window scale factor
  PseudoTcpFifo sbuf;

  // Maximum segment size, estimated protocol level, largest segment sent
  uint32_t mss, msslevel, largest, mtu_advise;
  // Retransmit timer
  uint32_t rto_base;

  // Timestamp tracking
  uint32_t ts_recent, ts_lastack;

  // Round-trip calculation
  uint32_t rx_rttvar, rx_srtt, rx_rto;

  // Congestion avoidance, Fast retransmit/recovery, Delayed ACKs
  uint32_t ssthresh, cwnd;
  uint8_t dup_acks;
  uint32_t recover;
  uint32_t t_ack;  /* time a delayed ack was scheduled; 0 if no acks scheduled */

  int use_nagling;
  uint32_t ack_delay;

  // This is used by unit tests to test backward compatibility of
  // PseudoTcp implementations that don't support window scaling.
  int support_wnd_scale;

  /* Current time. Typically only used for testing, when non-zero. When zero,
   * the system monotonic clock is used. Units: monotonic milliseconds. */
  uint32_t current_time;

  /* This is used by compatible implementations (with the TCP_OPT_FIN_ACK
   * option) to enable correct FIN-ACK connection termination. Defaults to
   * TRUE unless no compatible option is received. */
  int support_fin_ack;
};

#define G_MAXUINT32	((uint32_t) 0xffffffff)
#define LARGER(a,b) (((a) - (b) - 1) < (G_MAXUINT32 >> 1))
#define LARGER_OR_EQUAL(a,b) (((a) - (b)) < (G_MAXUINT32 >> 1))
#define SMALLER(a,b) LARGER ((b),(a))
#define SMALLER_OR_EQUAL(a,b) LARGER_OR_EQUAL ((b),(a))

/* properties */
enum
{
  PROP_CONVERSATION = 1,
  PROP_CALLBACKS,
  PROP_STATE,
  PROP_ACK_DELAY,
  PROP_NO_DELAY,
  PROP_RCV_BUF,
  PROP_SND_BUF,
  PROP_SUPPORT_FIN_ACK,
  LAST_PROPERTY
};


#if 0
static void ptcp_get_property (GObject *object, uint32_t property_id,
    GValue *value,  GParamSpec *pspec);
static void ptcp_set_property (GObject *object, uint32_t property_id,
    const GValue *value, GParamSpec *pspec);
static void ptcp_finalize (GObject *object);
#endif


static void queue_connect_message (ptcp_socket_t *self);
static uint32_t queue (ptcp_socket_t *self, const char *data,
    uint32_t len, TcpFlags flags);
static ptcp_write_result_t packet(ptcp_socket_t *self, uint32_t seq,
    TcpFlags flags, uint32_t offset, uint32_t len, uint32_t now);
static int parse (ptcp_socket_t *self,
    const uint8_t *_header_buf, size_t header_buf_len,
    const uint8_t *data_buf, size_t data_buf_len);
static int process(ptcp_socket_t *self, Segment *seg);
static int transmit(ptcp_socket_t *self, SSegment *sseg, uint32_t now);
static int attempt_send(ptcp_socket_t *self, SendFlags sflags);
static void closedown (ptcp_socket_t *self, uint32_t err,
    ClosedownSource source);
static void adjustMTU(ptcp_socket_t *self);
static void parse_options (ptcp_socket_t *self, const uint8_t *data,
    uint32_t len);
//static void resize_send_buffer (ptcp_socket_t *self, uint32_t new_size);
static void resize_receive_buffer (ptcp_socket_t *self, uint32_t new_size);
static void set_state (ptcp_socket_t *self, ptcp_state_t new_state);
static void set_state_established (ptcp_socket_t *self);
static void set_state_closed (ptcp_socket_t *self, uint32_t err);

static const char *ptcp_state_get_name (ptcp_state_t state);
static int ptcp_state_has_sent_fin (ptcp_state_t state);
static int ptcp_state_has_received_fin (ptcp_state_t state);

// The following logging is for detailed (packet-level) pseudotcp analysis only.
static ptcp_debug_level_t debug_level = PSEUDO_TCP_DEBUG_NONE;

#define DEBUG(...) (void *)0

void ptcp_set_debug_level(ptcp_debug_level_t level)
{
  debug_level = level;
}

static int64_t get_monotonic_time(void)
{
  struct timespec ts;
  int result;

  result = clock_gettime(CLOCK_MONOTONIC, &ts);

  if(UNLIKELY(result != 0))
      ;
//    DEBUG("GLib requires working CLOCK_MONOTONIC");

  return (((int64_t) ts.tv_sec) * 1000000) + (ts.tv_nsec / 1000);
}

static uint32_t get_current_time(ptcp_socket_t *ps)
{
  if (UNLIKELY(ps->current_time != 0))
    return ps->current_time;

  return get_monotonic_time () / 1000;
}

void ptcp_set_time(ptcp_socket_t *ps, uint32_t current_time)
{
  ps->current_time = current_time;
}

static void ptcp_destroy(ptcp_socket_t *ps)
{
  struct list_head *i, *next;
  SSegment *sseg;

  if (ps == NULL)
    return;

  while ((sseg = queue_pop_head(&ps->slist)))
    free(sseg);
  queue_clear(&ps->unsent_slist);
  if (ps->rlist) {
  list_for_each_safe(i, next, &ps->rlist->entry) {
    RSegment *rseg = (RSegment *)container_of(i, list_t, entry);
    free(rseg);
  }
  }
  free(ps->rlist);
  ps->rlist = NULL;

  ptcp_fifo_clear(&ps->rbuf);
  ptcp_fifo_clear(&ps->sbuf);

  free(ps);
  ps = NULL;
}

static void notify_clock(union sigval sv);
timer_t timeout_add(uint64_t msec, void (*func)(union sigval sv), void *data);

static ptcp_socket_t *ptcp_create(ptcp_callbacks_t *cbs)
{
  ptcp_socket_t *ps = (ptcp_socket_t *)calloc(1, sizeof(ptcp_socket_t));
  if (ps == NULL) {
    loge("malloc failed!\n");
    return NULL;
  }
  ps->timer_id = timeout_add(0, notify_clock, ps);
  sem_init(&ps->sem, 0, 0);
  ps->callbacks = *cbs;
  ps->shutdown = SD_NONE;
  ps->error = 0;

  ps->rbuf_len = DEFAULT_RCV_BUF_SIZE;
  ptcp_fifo_init(&ps->rbuf, ps->rbuf_len);
  ps->sbuf_len = DEFAULT_SND_BUF_SIZE;
  ptcp_fifo_init(&ps->sbuf, ps->sbuf_len);

  ps->state = TCP_LISTEN;
  ps->conv = 0;
  queue_init(&ps->slist);
  queue_init(&ps->unsent_slist);
  ps->rcv_wnd = ps->rbuf_len;
  ps->rwnd_scale = ps->swnd_scale = 0;
  ps->snd_nxt = 0;
  ps->snd_wnd = 1;
  ps->snd_una = ps->rcv_nxt = 0;
  ps->bReadEnable = TRUE;
  ps->bWriteEnable = FALSE;
  ps->t_ack = 0;

  ps->msslevel = 0;
  ps->largest = 0;
  ps->mss = MIN_PACKET - PACKET_OVERHEAD;
  ps->mtu_advise = MAX_PACKET;

  ps->rto_base = 0;

  ps->cwnd = 2 * ps->mss;
  ps->ssthresh = ps->rbuf_len;
  ps->lastrecv = ps->lastsend = ps->last_traffic = 0;
  ps->bOutgoing = FALSE;

  ps->dup_acks = 0;
  ps->recover = 0;

  ps->ts_recent = ps->ts_lastack = 0;

  ps->rx_rto = DEF_RTO;
  ps->rx_srtt = ps->rx_rttvar = 0;

  ps->ack_delay = DEFAULT_ACK_DELAY;
  ps->use_nagling = !DEFAULT_NO_DELAY;

  ps->support_wnd_scale = TRUE;
  ps->support_fin_ack = TRUE;

  return ps;
}

static void queue_connect_message(ptcp_socket_t *ps)
{
  uint8_t buf[8];
  size_t size = 0;

  buf[size++] = CTL_CONNECT;

  if (ps->support_wnd_scale) {
    buf[size++] = TCP_OPT_WND_SCALE;
    buf[size++] = 1;
    buf[size++] = ps->rwnd_scale;
  }

  if (ps->support_fin_ack) {
    buf[size++] = TCP_OPT_FIN_ACK;
    buf[size++] = 1;  /* option length; zero is invalid (RFC 1122, §4.2.2.5) */
    buf[size++] = 0;  /* currently unused */
  }

  ps->snd_wnd = size;

  queue(ps, (char *)buf, size, FLAG_CTL);
}

static void queue_fin_message(ptcp_socket_t *ps)
{
  assert(ps->support_fin_ack);

  /* FIN segments are always zero-length. */
  queue(ps, "", 0, FLAG_FIN);
}

static void queue_rst_message(ptcp_socket_t *ps)
{
  assert(ps->support_fin_ack);

  /* RST segments are always zero-length. */
  queue (ps, "", 0, FLAG_RST);
}

static int _ptcp_connect(ptcp_socket_t *p)
{
  if (p->state != TCP_LISTEN) {
    p->error = EINVAL;
    return FALSE;
  }

  set_state(p, TCP_SYN_SENT);

  queue_connect_message(p);
  return attempt_send(p, sfNone);
}

void ptcp_notify_mtu(ptcp_socket_t *ps, uint16_t mtu)
{
  ps->mtu_advise = mtu;
  if (ps->state == TCP_ESTABLISHED) {
    adjustMTU(ps);
  }
}

void ptcp_notify_clock(ptcp_socket_t *ps)
{
  uint32_t now = get_current_time(ps);

  if (ps->state == TCP_CLOSED)
    return;

  /* If in the TIME-WAIT state, any delayed segments have passed and the
   * connection can be considered closed from both ends.
   * FIXME: This should probably actually compare a timestamp before
   * operating. */
  if (ps->support_fin_ack && ps->state == TCP_TIME_WAIT) {
//    DEBUG (PSEUDO_TCP_DEBUG_NORMAL,
//        "Notified clock in TIME-WAIT state; closing connection.");
    set_state_closed (ps, 0);
  }

  /* If in the LAST-ACK state, resend the FIN because it hasn’t been ACKed yet.
   * FIXME: This should probably actually compare a timestamp before
   * operating. */
  if (ps->support_fin_ack && ps->state == TCP_LAST_ACK) {
//    DEBUG (PSEUDO_TCP_DEBUG_NORMAL,
//        "Notified clock in LAST-ACK state; resending FIN segment.");
    queue_fin_message (ps);
    attempt_send (ps, sfFin);
  }

  // Check if it's time to retransmit a segment
  if (ps->rto_base &&
      (time_diff(ps->rto_base + ps->rx_rto, now) <= 0)) {
    if (queue_get_length (&ps->slist) == 0) {
//      DEBUG(PSEUDO_TCP_DEBUG_NORMAL, "not reached");
    } else {
      // Note: (ps->slist.front().xmit == 0)) {
      // retransmit segments
      uint32_t nInFlight;
      uint32_t rto_limit;

//      DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "timeout retransmit (rto: %d) "
//          "(rto_base: %d) (now: %d) (dup_acks: %d)",
//          ps->rx_rto, ps->rto_base, now, (uint32_t) ps->dup_acks);

      if (!transmit(ps, queue_peek_head (&ps->slist), now)) {
        closedown (ps, ECONNABORTED, CLOSEDOWN_LOCAL);
        return;
      }

      nInFlight = ps->snd_nxt - ps->snd_una;
      ps->ssthresh = max(nInFlight / 2, 2 * ps->mss);
      //LOG(LS_INFO) << "ps->ssthresh: " << ps->ssthresh << "  nInFlight: " << nInFlight << "  ps->mss: " << ps->mss;
      ps->cwnd = ps->mss;

      // Back off retransmit timer.  Note: the limit is lower when connecting.
      rto_limit = (ps->state < TCP_ESTABLISHED) ? DEF_RTO : MAX_RTO;
      ps->rx_rto = min(rto_limit, ps->rx_rto * 2);
      ps->rto_base = now;
    }
  }

  // Check if it's time to probe closed windows
  if ((ps->snd_wnd == 0)
        && (time_diff(ps->lastsend + ps->rx_rto, now) <= 0)) {
    if (time_diff(now, ps->lastrecv) >= 15000) {
      closedown (ps, ECONNABORTED, CLOSEDOWN_LOCAL);
      return;
    }

    // probe the window
    packet(ps, ps->snd_nxt - 1, 0, 0, 0, now);
    ps->lastsend = now;

    // back off retransmit timer
    ps->rx_rto = min(MAX_RTO, ps->rx_rto * 2);
  }

  // Check if it's time to send delayed acks
  if (ps->t_ack && (time_diff(ps->t_ack + ps->ack_delay, now) <= 0)) {
    packet(ps, ps->snd_nxt, 0, 0, 0, now);
  }

}

int ptcp_notify_packet(ptcp_socket_t *ps, const char *buffer, uint32_t len)
{
  int retval;

  if (len > MAX_PACKET) {
    //LOG_F(WARNING) << "packet too large";
    return FALSE;
  } else if (len < HEADER_SIZE) {
    //LOG_F(WARNING) << "packet too small";
    return FALSE;
  }

  /* Hold a reference to the ptcp_socket_t during parsing, since it may be
   * closed from within a callback. */
//  g_object_ref (ps);
  retval = parse (ps, (uint8_t *) buffer, HEADER_SIZE,
      (uint8_t *) buffer + HEADER_SIZE, len - HEADER_SIZE);
//  g_object_unref (ps);

  return retval;
}

/* Assume there are two buffers in the given #NiceInputMessage: a 24-byte one
 * containing the header, and a bigger one for the data. */
#if 0
int
ptcp_notify_message (ptcp_socket_t *ps,
    NiceInputMessage *message)
{
  int retval;

  assert_cmpuint (message->n_buffers, >, 0);

  if (message->n_buffers == 1)
    return ptcp_notify_packet (ps, message->buffers[0].buffer,
        message->buffers[0].size);

  assert_cmpuint (message->n_buffers, ==, 2);
  assert_cmpuint (message->buffers[0].size, ==, HEADER_SIZE);

  if (message->length > MAX_PACKET) {
    //LOG_F(WARNING) << "packet too large";
    return FALSE;
  } else if (message->length < HEADER_SIZE) {
    //LOG_F(WARNING) << "packet too small";
    return FALSE;
  }

  /* Hold a reference to the ptcp_socket_t during parsing, since it may be
   * closed from within a callback. */
//  g_object_ref (self);
  retval = parse (ps, message->buffers[0].buffer, message->buffers[0].size,
      message->buffers[1].buffer, message->length - message->buffers[0].size);
//  g_object_unref (self);

  return retval;
}
#endif

int
ptcp_get_next_clock(ptcp_socket_t *ps, uint64_t *timeout)
{
  uint32_t now = get_current_time (ps);
  size_t snd_buffered;
  uint32_t closed_timeout;

  if (ps->shutdown == SD_FORCEFUL) {
    if (ps->support_fin_ack) {
//      DEBUG (PSEUDO_TCP_DEBUG_NORMAL,
//          "‘Forceful’ shutdown used when FIN-ACK support is enabled");
    }

    /* Transition to the CLOSED state. */
    closedown (ps, 0, CLOSEDOWN_REMOTE);

    return FALSE;
  }

  snd_buffered = ptcp_fifo_get_buffered (&ps->sbuf);
  if ((ps->shutdown == SD_GRACEFUL)
      && ((ps->state != TCP_ESTABLISHED)
          || ((snd_buffered == 0) && (ps->t_ack == 0)))) {
    if (ps->support_fin_ack) {
//      DEBUG (PSEUDO_TCP_DEBUG_NORMAL,
//          "‘Graceful’ shutdown used when FIN-ACK support is enabled");
    }

    /* Transition to the CLOSED state. */
    closedown (ps, 0, CLOSEDOWN_REMOTE);

    return FALSE;
  }

  /* FIN-ACK support. The timeout for closing the socket if nothing is received
   * varies depending on whether the socket is waiting in the TIME-WAIT state
   * for delayed segments to pass.
   *
   * See: http://vincent.bernat.im/en/blog/2014-tcp-time-wait-state-linux.html
   */
  closed_timeout = CLOSED_TIMEOUT;
  if (ps->support_fin_ack && ps->state == TCP_TIME_WAIT)
    closed_timeout = TIME_WAIT_TIMEOUT;

  if (ps->support_fin_ack && ps->state == TCP_CLOSED) {
    return FALSE;
  }

  if (*timeout == 0 || *timeout < now)
    *timeout = now + closed_timeout;

  if (ps->support_fin_ack && ps->state == TCP_TIME_WAIT) {
    *timeout = min (*timeout, now + TIME_WAIT_TIMEOUT);
    return TRUE;
  }

  if (ps->state == TCP_CLOSED && !ps->support_fin_ack) {
    *timeout = min (*timeout, now + CLOSED_TIMEOUT);
    return TRUE;
  }

  *timeout = min (*timeout, now + DEFAULT_TIMEOUT);

  if (ps->t_ack) {
    *timeout = min(*timeout, ps->t_ack + ps->ack_delay);
  }
  if (ps->rto_base) {
    *timeout = min(*timeout, ps->rto_base + ps->rx_rto);
  }
  if (ps->snd_wnd == 0) {
    *timeout = min(*timeout, ps->lastsend + ps->rx_rto);
  }

  return TRUE;
}

int
ptcp_recv(ptcp_socket_t *ps, void *buffer, size_t len)
{
  size_t bytesread;
  size_t available_space;

  /* Received a FIN from the peer, so return 0. RFC 793, §3.5, Case 2. */
  if (ps->support_fin_ack &&
      (ps->shutdown_reads ||
       ptcp_state_has_received_fin (ps->state))) {
    return 0;
  }

  /* Return 0 if FIN-ACK is not supported but the socket has been closed. */
  if (!ps->support_fin_ack && ptcp_is_closed (ps)) {
    return 0;
  }

  /* Return ENOTCONN if FIN-ACK is not supported and the connection is not
   * ESTABLISHED. */
  if (!ps->support_fin_ack && ps->state != TCP_ESTABLISHED) {
    ps->error = ENOTCONN;
    return -1;
  }

  if (len == 0)
    return 0;

  bytesread = ptcp_fifo_read (&ps->rbuf, (uint8_t *) buffer, len);

 // If there's no data in |m_rbuf|.
  if (bytesread == 0) {
    ps->bReadEnable = TRUE;
    ps->error = EWOULDBLOCK;
    return -1;
  }

  available_space = ptcp_fifo_get_write_remaining (&ps->rbuf);

  if (available_space - ps->rcv_wnd >=
      min (ps->rbuf_len / 2, ps->mss)) {
    // !?! Not sure about this was closed business
    int bWasClosed = (ps->rcv_wnd == 0);

    ps->rcv_wnd = available_space;

    if (bWasClosed) {
      attempt_send(ps, sfImmediateAck);
    }
  }

  return bytesread;
}

int
ptcp_send(ptcp_socket_t *ps, const void * buffer, size_t len)
{
  int written;
  size_t available_space;

  if (ps->state != TCP_ESTABLISHED) {
    ps->error = ptcp_state_has_sent_fin (ps->state) ? EPIPE : ENOTCONN;
    return -1;
  }

  available_space = ptcp_fifo_get_write_remaining (&ps->sbuf);

  if (!available_space) {
    ps->bWriteEnable = TRUE;
    ps->error = EWOULDBLOCK;
    loge("%s:%d xxxx\n", __func__, __LINE__);
    return -1;
  }

  written = queue (ps, buffer, len, FLAG_NONE);
  attempt_send(ps, sfNone);

  if (written > 0 && (uint32_t)written < len) {
    ps->bWriteEnable = TRUE;
  }

  return written;
}

static void
_ptcp_close(ptcp_socket_t *ps, int force)
{
//  DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Closing socket %p %s", ps,
//      force ? "forcefully" : "gracefully");

  /* Forced closure by sending an RST segment. RFC 1122, §4.2.2.13. */
  if (force && ps->state != TCP_CLOSED) {
    closedown (ps, ECONNABORTED, CLOSEDOWN_LOCAL);
    return;
  }

  /* Fall back to shutdown(). */
  ptcp_shutdown (ps, PSEUDO_TCP_SHUTDOWN_RDWR);
}

void ptcp_shutdown (ptcp_socket_t *ps, ptcp_shutdown_t how)
{
//  DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Shutting down socket %p: %u", ps, how);

  /* FIN-ACK--only stuff below here. */
  if (!ps->support_fin_ack) {
    if (ps->shutdown == SD_NONE)
      ps->shutdown = SD_GRACEFUL;
    return;
  }

  /* What needs shutting down? */
  switch (how) {
  case PSEUDO_TCP_SHUTDOWN_RD:
  case PSEUDO_TCP_SHUTDOWN_RDWR:
    ps->shutdown_reads = TRUE;
    break;
  case PSEUDO_TCP_SHUTDOWN_WR:
    /* Handled below. */
    break;
  default:
//    DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Invalid shutdown method: %u.", how);
    break;
  }

  if (how == PSEUDO_TCP_SHUTDOWN_RD) {
    return;
  }

  /* Unforced write closure. */
  switch (ps->state) {
  case TCP_LISTEN:
  case TCP_SYN_SENT:
    /* Just abort the connection without completing the handshake. */
    set_state_closed (ps, 0);
    break;
  case TCP_SYN_RECEIVED:
  case TCP_ESTABLISHED:
    /* Local user initiating the close: RFC 793, §3.5, Cases 1 and 3.
     * If there is pending receive data, send RST instead of FIN;
     * see RFC 1122, §4.2.2.13. */
    if (ptcp_get_available_bytes (ps) > 0) {
      closedown (ps, ECONNABORTED, CLOSEDOWN_LOCAL);
    } else {
      queue_fin_message (ps);
      attempt_send (ps, sfFin);
      set_state (ps, TCP_FIN_WAIT_1);
    }
    break;
  case TCP_CLOSE_WAIT:
    /* Remote user initiating the close: RFC 793, §3.5, Case 2.
     * We’ve previously received a FIN from the peer; now the user is closing
     * the local end of the connection. */
    queue_fin_message (ps);
    attempt_send (ps, sfFin);
    set_state (ps, TCP_LAST_ACK);
    break;
  case TCP_CLOSING:
  case TCP_CLOSED:
    /* Already closed on both sides. */
    break;
  case TCP_FIN_WAIT_1:
  case TCP_FIN_WAIT_2:
  case TCP_TIME_WAIT:
  case TCP_LAST_ACK:
    /* Already closed locally. */
    break;
  default:
    /* Do nothing. */
    break;
  }
}

int
ptcp_get_error(ptcp_socket_t *ps)
{
  return ps->error;
}

//
// Internal Implementation
//

static uint32_t
queue (ptcp_socket_t *ps, const char * data, uint32_t len, TcpFlags flags)
{
  size_t available_space;

  available_space = ptcp_fifo_get_write_remaining (&ps->sbuf);
  if (len > available_space) {
    assert (flags == FLAG_NONE);
    len = available_space;
  }

  // We can concatenate data if the last segment is the same type
  // (control v. regular data), and has not been transmitted yet
  if (queue_get_length (&ps->slist) &&
      (((SSegment *)queue_peek_tail (&ps->slist))->flags == flags) &&
      (((SSegment *)queue_peek_tail (&ps->slist))->xmit == 0)) {
    ((SSegment *)queue_peek_tail (&ps->slist))->len += len;
  } else {
    SSegment *sseg = (SSegment *)calloc(1, sizeof(SSegment));
    size_t snd_buffered = ptcp_fifo_get_buffered (&ps->sbuf);

    sseg->seq = ps->snd_una + snd_buffered;
    sseg->len = len;
    sseg->flags = flags;
    queue_push_tail (&ps->slist, sseg);
    queue_push_tail (&ps->unsent_slist, sseg);
  }

  //LOG(LS_INFO) << "PseudoTcp::queue - ps->slen = " << ps->slen;
  return ptcp_fifo_write (&ps->sbuf, (uint8_t*) data, len);;
}

// Creates a packet and submits it to the network. This method can either
// send payload or just an ACK packet.
//
// |seq| is the sequence number of this packet.
// |flags| is the flags for sending this packet.
// |offset| is the offset to read from |m_sbuf|.
// |len| is the number of bytes to read from |m_sbuf| as payload. If this
// value is 0 then this is an ACK packet, otherwise this packet has payload.

static ptcp_write_result_t
packet(ptcp_socket_t *ps, uint32_t seq, TcpFlags flags,
    uint32_t offset, uint32_t len, uint32_t now)
{
  union {
    uint8_t u8[MAX_PACKET];
    uint16_t u16[MAX_PACKET / 2];
    uint32_t u32[MAX_PACKET / 4];
  } buffer;
  ptcp_write_result_t wres = WR_SUCCESS;

  assert(HEADER_SIZE + len <= MAX_PACKET);

  *buffer.u32 = htonl(ps->conv);
  *(buffer.u32 + 1) = htonl(seq);
  *(buffer.u32 + 2) = htonl(ps->rcv_nxt);
  buffer.u8[12] = 0;
  buffer.u8[13] = flags;
  *(buffer.u16 + 7) = htons((uint16_t)(ps->rcv_wnd >> ps->rwnd_scale));

  // Timestamp computations
  *(buffer.u32 + 4) = htonl(now);
  *(buffer.u32 + 5) = htonl(ps->ts_recent);
  ps->ts_lastack = ps->rcv_nxt;

  if (len) {
    size_t bytes_read;

    bytes_read = ptcp_fifo_read_offset (&ps->sbuf, buffer.u8 + HEADER_SIZE,
        len, offset);
    //assert (bytes_read == len);
    if (bytes_read != len) {
      logd("bytes_read=%zd, len=%u\n", bytes_read, len);
    }
  }

//  DEBUG (PSEUDO_TCP_DEBUG_VERBOSE, "<-- <CONV=%d><FLG=%d><SEQ=%d:%d><ACK=%d>"
//      "<WND=%d><TS=%d><TSR=%d><LEN=%d>",
//      ps->conv, (unsigned)flags, seq, seq + len, ps->rcv_nxt, ps->rcv_wnd,
//      now % 10000, ps->ts_recent % 10000, len);

  wres = ps->callbacks.write(ps, (char *) buffer.u8, len + HEADER_SIZE,
                                     ps->callbacks.data);
  /* Note: When len is 0, this is an ACK packet.  We don't read the
     return value for those, and thus we won't retry.  So go ahead and treat
     the packet as a success (basically simulate as if it were dropped),
     which will prevent our timers from being messed up. */
  if ((wres != WR_SUCCESS) && (0 != len))
    return wres;

  ps->t_ack = 0;
  if (len > 0) {
    ps->lastsend = now;
  }
  ps->last_traffic = now;
  ps->bOutgoing = TRUE;

  return WR_SUCCESS;
}

static int
parse (ptcp_socket_t *ps, const uint8_t *_header_buf, size_t header_buf_len,
    const uint8_t *data_buf, size_t data_buf_len)
{
  Segment seg;

  union {
    const uint8_t *u8;
    const uint16_t *u16;
    const uint32_t *u32;
  } header_buf;

  header_buf.u8 = _header_buf;

  if (header_buf_len != 24)
    return FALSE;

  seg.conv = ntohl(*header_buf.u32);
  seg.seq = ntohl(*(header_buf.u32 + 1));
  seg.ack = ntohl(*(header_buf.u32 + 2));
  seg.flags = header_buf.u8[13];
  seg.wnd = ntohs(*(header_buf.u16 + 7));

  seg.tsval = ntohl(*(header_buf.u32 + 4));
  seg.tsecr = ntohl(*(header_buf.u32 + 5));

  seg.data = (const char *) data_buf;
  seg.len = data_buf_len;

//  DEBUG (PSEUDO_TCP_DEBUG_VERBOSE, "--> <CONV=%d><FLG=%d><SEQ=%d:%d><ACK=%d>"
//      "<WND=%d><TS=%d><TSR=%d><LEN=%d>",
//      seg.conv, (unsigned)seg.flags, seg.seq, seg.seq + seg.len, seg.ack,
//      seg.wnd, seg.tsval % 10000, seg.tsecr % 10000, seg.len);

  return process(ps, &seg);
}

/* True iff the @state requires that a FIN has already been sent by this
 * host. */
static int
ptcp_state_has_sent_fin (ptcp_state_t state)
{
  switch (state) {
  case TCP_LISTEN:
  case TCP_SYN_SENT:
  case TCP_SYN_RECEIVED:
  case TCP_ESTABLISHED:
  case TCP_CLOSE_WAIT:
    return FALSE;
  case TCP_CLOSED:
  case TCP_FIN_WAIT_1:
  case TCP_FIN_WAIT_2:
  case TCP_CLOSING:
  case TCP_TIME_WAIT:
  case TCP_LAST_ACK:
    return TRUE;
  default:
    return FALSE;
  }
}

/* True iff the @state requires that a FIN has already been received from the
 * peer. */
static int
ptcp_state_has_received_fin (ptcp_state_t state)
{
  switch (state) {
  case TCP_LISTEN:
  case TCP_SYN_SENT:
  case TCP_SYN_RECEIVED:
  case TCP_ESTABLISHED:
  case TCP_FIN_WAIT_1:
  case TCP_FIN_WAIT_2:
    return FALSE;
  case TCP_CLOSED:
  case TCP_CLOSING:
  case TCP_TIME_WAIT:
  case TCP_CLOSE_WAIT:
  case TCP_LAST_ACK:
    return TRUE;
  default:
    return FALSE;
  }
}

static int
process(ptcp_socket_t *ps, Segment *seg)
{
  uint32_t now;
  SendFlags sflags = sfNone;
  int bIgnoreData;
  int bNewData;
  int bConnect = FALSE;
  size_t snd_buffered;
  size_t available_space;
  uint32_t kIdealRefillSize;
  int is_valuable_ack, is_duplicate_ack, is_fin_ack = FALSE;

  /* If this is the wrong conversation, send a reset!?!
     (with the correct conversation?) */
  if (seg->conv != ps->conv) {
    //if ((seg->flags & FLAG_RST) == 0) {
    //  packet(sock, tcb, seg->ack, 0, FLAG_RST, 0, 0);
    //}
//    DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "wrong conversation");
    return FALSE;
  }

  now = get_current_time (ps);
  ps->last_traffic = ps->lastrecv = now;
  ps->bOutgoing = FALSE;

  if (ps->state == TCP_CLOSED ||
      (ptcp_state_has_sent_fin (ps->state) && seg->len > 0)) {
    /* Send an RST segment. See: RFC 1122, §4.2.2.13. */
    if ((seg->flags & FLAG_RST) == 0) {
      closedown (ps, 0, CLOSEDOWN_LOCAL);
    }
//    DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Segment received while closed; sent RST.");
    return FALSE;
  }

  // Check if this is a reset segment
  if (seg->flags & FLAG_RST) {
    closedown (ps, ECONNRESET, CLOSEDOWN_REMOTE);
    return FALSE;
  }

  // Check for control data
  bConnect = FALSE;
  if (seg->flags & FLAG_CTL) {
    if (seg->len == 0) {
//      DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Missing control code");
      return FALSE;
    } else if (seg->data[0] == CTL_CONNECT) {
      bConnect = TRUE;

      parse_options (ps, (uint8_t *) &seg->data[1], seg->len - 1);

      if (ps->state == TCP_LISTEN) {
        set_state (ps, TCP_SYN_RECEIVED);
        queue_connect_message (ps);
      } else if (ps->state == TCP_SYN_SENT) {
        set_state_established (ps);
      }
    } else {
//      DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Unknown control code: %d", seg->data[0]);
      return FALSE;
    }
  }

  // Update timestamp
  if (SMALLER_OR_EQUAL (seg->seq, ps->ts_lastack) &&
      SMALLER (ps->ts_lastack, seg->seq + seg->len)) {
    ps->ts_recent = seg->tsval;
  }

  // Check if this is a valuable ack
  is_valuable_ack = (LARGER(seg->ack, ps->snd_una) &&
      SMALLER_OR_EQUAL(seg->ack, ps->snd_nxt));
  is_duplicate_ack = (seg->ack == ps->snd_una);

  if (is_valuable_ack) {
    uint32_t nAcked;
    uint32_t nFree;

    // Calculate round-trip time
    if (seg->tsecr) {
      long rtt = time_diff(now, seg->tsecr);
      if (rtt >= 0) {
        if (ps->rx_srtt == 0) {
          ps->rx_srtt = rtt;
          ps->rx_rttvar = rtt / 2;
        } else {
          ps->rx_rttvar = (3 * ps->rx_rttvar +
              abs((long)(rtt - ps->rx_srtt))) / 4;
          ps->rx_srtt = (7 * ps->rx_srtt + rtt) / 8;
        }
        ps->rx_rto = bound(MIN_RTO,
            ps->rx_srtt + max(1LU, 4 * ps->rx_rttvar), MAX_RTO);

//        DEBUG (PSEUDO_TCP_DEBUG_VERBOSE, "rtt: %ld   srtt: %d  rto: %d",
//                rtt, ps->rx_srtt, ps->rx_rto);
      } else {
//        DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Invalid RTT: %ld", rtt);
        return FALSE;
      }
    }

    ps->snd_wnd = seg->wnd << ps->swnd_scale;

    nAcked = seg->ack - ps->snd_una;
    ps->snd_una = seg->ack;

    ps->rto_base = (ps->snd_una == ps->snd_nxt) ? 0 : now;

    /* ACKs for FIN segments give an increment on nAcked, but there is no
     * corresponding byte to read because the FIN segment is empty (it just has
     * a sequence number). */
    if (nAcked == ps->sbuf.data_length + 1 &&
        ptcp_state_has_sent_fin (ps->state)) {
      is_fin_ack = TRUE;
      nAcked--;
    }

    ptcp_fifo_consume_read_data (&ps->sbuf, nAcked);

    for (nFree = nAcked; nFree > 0; ) {
      SSegment *data;

      assert(queue_get_length (&ps->slist) != 0);
      data = (SSegment *) queue_peek_head (&ps->slist);

      if (nFree < data->len) {
        data->len -= nFree;
        data->seq += nFree;
        nFree = 0;
      } else {
        if (data->len > ps->largest) {
          ps->largest = data->len;
        }
        nFree -= data->len;
        free(data);
        queue_pop_head (&ps->slist);
      }
    }

    if (ps->dup_acks >= 3) {
      if (LARGER_OR_EQUAL (ps->snd_una, ps->recover)) { // NewReno
        uint32_t nInFlight = ps->snd_nxt - ps->snd_una;
        // (Fast Retransmit)
        ps->cwnd = min(ps->ssthresh, nInFlight + ps->mss);
//        DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "exit recovery");
        ps->dup_acks = 0;
      } else {
//        DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "recovery retransmit");
        if (!transmit(ps, queue_peek_head (&ps->slist), now)) {
          closedown (ps, ECONNABORTED, CLOSEDOWN_LOCAL);
          return FALSE;
        }
        ps->cwnd += ps->mss - min(nAcked, ps->cwnd);
      }
    } else {
      ps->dup_acks = 0;
      // Slow start, congestion avoidance
      if (ps->cwnd < ps->ssthresh) {
        ps->cwnd += ps->mss;
      } else {
        ps->cwnd += max(1LU, ps->mss * ps->mss / ps->cwnd);
      }
    }
  } else if (is_duplicate_ack) {
    /* !?! Note, tcp says don't do this... but otherwise how does a
       closed window become open? */
    ps->snd_wnd = seg->wnd << ps->swnd_scale;

    // Check duplicate acks
    if (seg->len > 0) {
      // it's a dup ack, but with a data payload, so don't modify ps->dup_acks
    } else if (ps->snd_una != ps->snd_nxt) {
      uint32_t nInFlight;

      ps->dup_acks += 1;
      if (ps->dup_acks == 3) { // (Fast Retransmit)
//        DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "enter recovery");
//        DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "recovery retransmit");
        if (!transmit(ps, queue_peek_head (&ps->slist), now)) {
          closedown (ps, ECONNABORTED, CLOSEDOWN_LOCAL);
          return FALSE;
        }
        ps->recover = ps->snd_nxt;
        nInFlight = ps->snd_nxt - ps->snd_una;
        ps->ssthresh = max(nInFlight / 2, 2 * ps->mss);
        //LOG(LS_INFO) << "ps->ssthresh: " << ps->ssthresh << "  nInFlight: " << nInFlight << "  ps->mss: " << ps->mss;
        ps->cwnd = ps->ssthresh + 3 * ps->mss;
      } else if (ps->dup_acks > 3) {
        ps->cwnd += ps->mss;
      }
    } else {
      ps->dup_acks = 0;
    }
  }

  // !?! A bit hacky
  if ((ps->state == TCP_SYN_RECEIVED) && !bConnect) {
    set_state_established (ps);
  }

  /* Check for connection closure. */
  if (ps->support_fin_ack) {
    /* For the moment, FIN segments must not contain data. */
    if (seg->flags & FLAG_FIN && seg->len != 0) {
//      DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "FIN segment contained data; ignored");
      return FALSE;
    }

    /* Update the state machine, implementing all transitions on ‘rcv FIN’ or
     * ‘rcv ACK of FIN’ from RFC 793, Figure 6; and RFC 1122, §4.2.2.8. */
    switch (ps->state) {
    case TCP_ESTABLISHED:
      if (seg->flags & FLAG_FIN) {
        /* Received a FIN from the network, RFC 793, §3.5, Case 2.
         * The code below will send an ACK for the FIN. */
         set_state (ps, TCP_CLOSE_WAIT);
      }
      break;
    case TCP_CLOSING:
      if (is_fin_ack) {
        /* Handle the ACK of a locally-sent FIN flag. RFC 793, §3.5, Case 3. */
        set_state (ps, TCP_TIME_WAIT);
      }
      break;
    case TCP_LAST_ACK:
      if (is_fin_ack) {
        /* Handle the ACK of a locally-sent FIN flag. RFC 793, §3.5, Case 2. */
        set_state_closed (ps, 0);
      }
      break;
    case TCP_FIN_WAIT_1:
      if (is_fin_ack && seg->flags & FLAG_FIN) {
        /* Simultaneous close with an ACK for a FIN previously sent,
         * RFC 793, §3.5, Case 3. */
        set_state (ps, TCP_TIME_WAIT);
      } else if (is_fin_ack) {
        /* Handle the ACK of a locally-sent FIN flag. RFC 793, §3.5, Case 1. */
        set_state (ps, TCP_FIN_WAIT_2);
      } else if (seg->flags & FLAG_FIN) {
        /* Simultaneous close, RFC 793, §3.5, Case 3. */
        set_state (ps, TCP_CLOSING);
      }
      break;
    case TCP_FIN_WAIT_2:
      if (seg->flags & FLAG_FIN) {
        /* Local user closed the connection, RFC 793, §3.5, Case 1. */
        set_state (ps, TCP_TIME_WAIT);
      }
      break;
    case TCP_LISTEN:
    case TCP_SYN_SENT:
    case TCP_SYN_RECEIVED:
    case TCP_TIME_WAIT:
    case TCP_CLOSED:
    case TCP_CLOSE_WAIT:
      /* Shouldn’t ever hit these cases. */
      if (seg->flags & FLAG_FIN) {
//        DEBUG (PSEUDO_TCP_DEBUG_NORMAL,
//           "Unexpected state %u when FIN received", ps->state);
      } else if (is_fin_ack) {
//        DEBUG (PSEUDO_TCP_DEBUG_NORMAL,
//           "Unexpected state %u when FIN-ACK received", ps->state);
      }
      break;
    default:
//      DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Invalid state %u when FIN received",
//          ps->state);
      return FALSE;
    }
  } else if (seg->flags & FLAG_FIN) {
//    DEBUG (PSEUDO_TCP_DEBUG_NORMAL,
//        "Invalid FIN received when FIN-ACK support is disabled");
  } else if (is_fin_ack) {
//    DEBUG (PSEUDO_TCP_DEBUG_NORMAL,
//        "Invalid FIN-ACK received when FIN-ACK support is disabled");
  }

  // If we make room in the send queue, notify the user
  // The goal it to make sure we always have at least enough data to fill the
  // window.  We'd like to notify the app when we are halfway to that point.
  kIdealRefillSize = (ps->sbuf_len + ps->rbuf_len) / 2;

  snd_buffered = ptcp_fifo_get_buffered (&ps->sbuf);
  if (ps->bWriteEnable && snd_buffered < kIdealRefillSize) {
    ps->bWriteEnable = FALSE;
    if (ps->callbacks.on_writable)
      ps->callbacks.on_writable(ps, ps->callbacks.data);
  }

  /* Conditions where acks must be sent:
   * 1) Segment is too old (they missed an ACK) (immediately)
   * 2) Segment is too new (we missed a segment) (immediately)
   * 3) Segment has data (so we need to ACK!) (delayed)
   * ... so the only time we don't need to ACK, is an empty segment
   * that points to rcv_nxt!
   * 4) Segment has the FIN flag set (immediately) — note that the FIN flag
   *    itself has to be included in the ACK as a numbered byte;
   *    see RFC 793, §3.3. Also see: RFC 793, §3.5.
   */
  if (seg->seq != ps->rcv_nxt) {
    sflags = sfImmediateAck; // (Fast Recovery)
  } else if (seg->len != 0) {
    if (ps->ack_delay == 0) {
      sflags = sfImmediateAck;
    } else {
      sflags = sfDelayedAck;
    }
  } else if (seg->flags & FLAG_FIN) {
    sflags = sfImmediateAck;
    ps->rcv_nxt += 1;
  }

  if (sflags == sfImmediateAck) {
    if (seg->seq > ps->rcv_nxt) {
//      DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "too new");
    } else if (SMALLER_OR_EQUAL(seg->seq + seg->len, ps->rcv_nxt)) {
//      DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "too old");
    }
  }

  // Adjust the incoming segment to fit our receive buffer
  if (SMALLER(seg->seq, ps->rcv_nxt)) {
    uint32_t nAdjust = ps->rcv_nxt - seg->seq;
    if (nAdjust < seg->len) {
      seg->seq += nAdjust;
      seg->data += nAdjust;
      seg->len -= nAdjust;
    } else {
      seg->len = 0;
    }
  }

  available_space = ptcp_fifo_get_write_remaining (&ps->rbuf);

  if ((seg->seq + seg->len - ps->rcv_nxt) > available_space) {
    uint32_t nAdjust = seg->seq + seg->len - ps->rcv_nxt - available_space;
    if (nAdjust < seg->len) {
      seg->len -= nAdjust;
    } else {
      seg->len = 0;
    }
  }

  bIgnoreData = (seg->flags & FLAG_CTL);
  if (!ps->support_fin_ack)
    bIgnoreData |= (ps->shutdown != SD_NONE);

  bNewData = FALSE;

  if (seg->flags & FLAG_FIN) {
    /* FIN flags have a sequence number. */
    if (seg->seq == ps->rcv_nxt) {
      ps->rcv_nxt++;
    }
  } else if (seg->len > 0) {
    if (bIgnoreData) {
      if (seg->seq == ps->rcv_nxt) {
        ps->rcv_nxt += seg->len;
      }
    } else {
      uint32_t nOffset = seg->seq - ps->rcv_nxt;
      size_t res;

      res = ptcp_fifo_write_offset (&ps->rbuf, (uint8_t *) seg->data,
          seg->len, nOffset);
      assert (res == seg->len);

      if (seg->seq == ps->rcv_nxt) {
        list_t *iter = NULL;
        list_t *next = NULL;

        ptcp_fifo_consume_write_buffer (&ps->rbuf, seg->len);
        ps->rcv_nxt += seg->len;
        ps->rcv_wnd -= seg->len;
        bNewData = TRUE;

        iter = ps->rlist;
#if 0
        while (iter &&
            SMALLER_OR_EQUAL(((RSegment *)iter->data)->seq, ps->rcv_nxt)) {
          RSegment *data = (RSegment *)(iter->data);
          if (LARGER (data->seq + data->len, ps->rcv_nxt)) {
            uint32_t nAdjust = (data->seq + data->len) - ps->rcv_nxt;
            sflags = sfImmediateAck; // (Fast Recovery)
            DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Recovered %d bytes (%d -> %d)",
                nAdjust, ps->rcv_nxt, ps->rcv_nxt + nAdjust);
            ptcp_fifo_consume_write_buffer (&ps->rbuf, nAdjust);
            ps->rcv_nxt += nAdjust;
            ps->rcv_wnd -= nAdjust;
          }
          g_slice_free (RSegment, ps->rlist->data);
          ps->rlist = g_list_delete_link (ps->rlist, ps->rlist);
          iter = ps->rlist;
        }
#else
        if (ps->rlist) {
        list_for_each_entry_safe(iter, next, &ps->rlist->entry, entry) {
          if (!SMALLER_OR_EQUAL(((RSegment *)iter->data)->seq, ps->rcv_nxt)) {
                break;
          }
          RSegment *data = (RSegment *)(iter->data);
          if (LARGER (data->seq + data->len, ps->rcv_nxt)) {
            uint32_t nAdjust = (data->seq + data->len) - ps->rcv_nxt;
            sflags = sfImmediateAck; // (Fast Recovery)
//            DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Recovered %d bytes (%d -> %d)",
//                nAdjust, ps->rcv_nxt, ps->rcv_nxt + nAdjust);
            ptcp_fifo_consume_write_buffer (&ps->rbuf, nAdjust);
            ps->rcv_nxt += nAdjust;
            ps->rcv_wnd -= nAdjust;
          }
          free(iter->data);
          list_del(&iter->entry);
        }
      }
#endif
      } else {
#if 0
        GList *iter = NULL;
        RSegment *rseg = (RSegment *)calloc(1, sizeof(RSegment));

        DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Saving %d bytes (%d -> %d)",
            seg->len, seg->seq, seg->seq + seg->len);
        rseg->seq = seg->seq;
        rseg->len = seg->len;
        iter = ps->rlist;
        while (iter && SMALLER (((RSegment*)iter->data)->seq, rseg->seq)) {
          iter = g_list_next (iter);
        }
        ps->rlist = g_list_insert_before(ps->rlist, iter, rseg);
#else
        list_t *iter = NULL;
        list_t *next = NULL;
        RSegment *rseg = (RSegment *)calloc(1, sizeof(RSegment));

//        DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Saving %d bytes (%d -> %d)",
//            seg->len, seg->seq, seg->seq + seg->len);
        rseg->seq = seg->seq;
        rseg->len = seg->len;
        if (ps->rlist) {
        list_for_each_entry_safe(iter, next, &ps->rlist->entry, entry) {
          if (!SMALLER (((RSegment*)iter->data)->seq, rseg->seq)) {
            break;
          }
        }
        iter->data = rseg;
        list_add_tail(&iter->entry, &ps->rlist->entry);
        }
#endif
      }
    }
  }

  attempt_send(ps, sflags);

  // If we have new data, notify the user
  if (bNewData && ps->bReadEnable) {
    /* ps->bReadEnable = FALSE; — removed so that we’re always notified of
     * incoming pseudo-TCP data, rather than having to read the entire buffer
     * on each readable() callback before the next callback is enabled.
     * (When client-provided buffers are small, this is not possible.) */
    if (ps->callbacks.on_readable)
      ps->callbacks.on_readable(ps, ps->callbacks.data);
  }

  return TRUE;
}

static int
transmit(ptcp_socket_t *ps, SSegment *segment, uint32_t now)
{
  uint32_t nTransmit = min(segment->len, ps->mss);

  if (segment->xmit >= ((ps->state == TCP_ESTABLISHED) ? 15 : 30)) {
//    DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "too many retransmits");
    return FALSE;
  }

  while (TRUE) {
    uint32_t seq = segment->seq;
    uint8_t flags = segment->flags;
    ptcp_write_result_t wres;

    /* The packet must not have already been acknowledged. */
    if (segment->seq >= ps->snd_una) {
        ;
    } else {
//        DEBUG("error!\n");
    }

    /* Write out the packet. */
    wres = packet(ps, seq, flags,
        segment->seq - ps->snd_una, nTransmit, now);

    if (wres == WR_SUCCESS)
      break;

    if (wres == WR_FAIL) {
//      DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "packet failed");
      return FALSE;
    }

    assert(wres == WR_TOO_LARGE);

    while (TRUE) {
      if (PACKET_MAXIMUMS[ps->msslevel + 1] == 0) {
//        DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "MTU too small");
        return FALSE;
      }
      /* !?! We need to break up all outstanding and pending packets
         and then retransmit!?! */

      ps->mss = PACKET_MAXIMUMS[++ps->msslevel] - PACKET_OVERHEAD;
      // I added this... haven't researched actual formula
      ps->cwnd = 2 * ps->mss;

      if (ps->mss < nTransmit) {
        nTransmit = ps->mss;
        break;
      }
    }
//    DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Adjusting mss to %d bytes ", ps->mss);
  }

  if (nTransmit < segment->len) {
    SSegment *subseg = (SSegment *)calloc(1, sizeof(SSegment));
    subseg->seq = segment->seq + nTransmit;
    subseg->len = segment->len - nTransmit;
    subseg->flags = segment->flags;
    subseg->xmit = segment->xmit;

//    DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "mss reduced to %d", ps->mss);

    segment->len = nTransmit;
    queue_insert_after (&ps->slist,
        queue_find (&ps->slist, segment), subseg);
    if (subseg->xmit == 0)
      queue_insert_after (&ps->unsent_slist,
          queue_find (&ps->unsent_slist, segment), subseg);
  }

  if (segment->xmit == 0) {
    //assert (queue_peek_head (&ps->unsent_slist) == segment);
    if (queue_peek_head (&ps->unsent_slist) == segment) {
      queue_pop_head (&ps->unsent_slist);
    }
    ps->snd_nxt += segment->len;

    /* FIN flags require acknowledgement. */
    if (segment->len == 0 && segment->flags & FLAG_FIN)
      ps->snd_nxt++;
  }
  segment->xmit += 1;

  if (ps->rto_base == 0) {
    ps->rto_base = now;
  }

  return TRUE;
}

static int
attempt_send(ptcp_socket_t *ps, SendFlags sflags)
{
  uint32_t now = get_current_time (ps);
  int bFirst = TRUE;

  if (time_diff(now, ps->lastsend) > (long) ps->rx_rto) {
    ps->cwnd = ps->mss;
  }


  while (TRUE) {
    uint32_t cwnd;
    uint32_t nWindow;
    uint32_t nInFlight;
    uint32_t nUseable;
    uint32_t nAvailable;
    size_t snd_buffered;
    list_t *iter;
    SSegment *sseg;

    cwnd = ps->cwnd;
    if ((ps->dup_acks == 1) || (ps->dup_acks == 2)) { // Limited Transmit
      cwnd += ps->dup_acks * ps->mss;
    }
    nWindow = min(ps->snd_wnd, cwnd);
    nInFlight = ps->snd_nxt - ps->snd_una;
    nUseable = (nInFlight < nWindow) ? (nWindow - nInFlight) : 0;
    snd_buffered = ptcp_fifo_get_buffered (&ps->sbuf);
    if (snd_buffered < nInFlight)  /* iff a FIN has been sent */
      nAvailable = 0;
    else
      nAvailable = min(snd_buffered - nInFlight, ps->mss);

    if (nAvailable > nUseable) {
      if (nUseable * 4 < nWindow) {
        // RFC 813 - avoid SWS
        nAvailable = 0;
      } else {
        nAvailable = nUseable;
      }
    }

    if (bFirst) {
//      size_t available_space = ptcp_fifo_get_write_remaining (&ps->sbuf);
      bFirst = FALSE;
//      DEBUG (PSEUDO_TCP_DEBUG_VERBOSE, "[cwnd: %d  nWindow: %d  nInFlight: %d "
//          "nAvailable: %d nQueued: %" G_GSIZE_FORMAT " nEmpty: %" G_GSIZE_FORMAT
//          "  ssthresh: %d]",
//          ps->cwnd, nWindow, nInFlight, nAvailable, snd_buffered,
//          available_space, ps->ssthresh);
    }

    if (nAvailable == 0 && sflags != sfFin && sflags != sfRst) {
      if (sflags == sfNone)
        return -1;

      // If this is an immediate ack, or the second delayed ack
      if ((sflags == sfImmediateAck) || ps->t_ack) {
        packet(ps, ps->snd_nxt, 0, 0, 0, now);
      } else {
        ps->t_ack = now;
      }
      return -1;
    }

    // Nagle algorithm
    // If there is data already in-flight, and we haven't a full segment of
    // data ready to send then hold off until we get more to send, or the
    // in-flight data is acknowledged.
    if (ps->use_nagling && sflags != sfFin && sflags != sfRst &&
        (ps->snd_nxt > ps->snd_una) &&
        (nAvailable < ps->mss))  {
      return -1;
    }

    // Find the next segment to transmit
    iter = queue_peek_head_link (&ps->unsent_slist);
    if (iter == NULL)
      return -1;
    sseg = iter->data;

    // If the segment is too large, break it into two
    if (sseg->len > nAvailable && sflags != sfFin && sflags != sfRst) {
      SSegment *subseg = (SSegment *)calloc(1, sizeof(SSegment));
      subseg->seq = sseg->seq + nAvailable;
      subseg->len = sseg->len - nAvailable;
      subseg->flags = sseg->flags;

      sseg->len = nAvailable;
      queue_insert_after (&ps->unsent_slist, iter, subseg);
      queue_insert_after (&ps->slist, queue_find (&ps->slist, sseg),
          subseg);
    }

    if (!transmit(ps, sseg, now)) {
//      DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "transmit failed");
      // TODO: consider closing socket
      return -1;
    }

    if (sflags == sfImmediateAck || sflags == sfDelayedAck)
      sflags = sfNone;
  }
  return 0;
}

/* If @source is %CLOSEDOWN_REMOTE, don’t send an RST packet, since closedown()
 * has been called as a result of an RST segment being received.
 * See: RFC 1122, §4.2.2.13. */
static void
closedown (ptcp_socket_t *ps, uint32_t err, ClosedownSource source)
{
  if (source == CLOSEDOWN_LOCAL && ps->support_fin_ack) {
    queue_rst_message (ps);
    attempt_send (ps, sfRst);
  } else if (source == CLOSEDOWN_LOCAL) {
    ps->shutdown = SD_FORCEFUL;
  }

  /* ‘Cute’ little navigation through the state machine to avoid breaking the
   * invariant that CLOSED can only be reached from TIME-WAIT or LAST-ACK. */
  switch (ps->state) {
  case TCP_LISTEN:
  case TCP_SYN_SENT:
    break;
  case TCP_SYN_RECEIVED:
  case TCP_ESTABLISHED:
    set_state (ps, TCP_FIN_WAIT_1);
    /* Fall through. */
  case TCP_FIN_WAIT_1:
    set_state (ps, TCP_FIN_WAIT_2);
    /* Fall through. */
  case TCP_FIN_WAIT_2:
  case TCP_CLOSING:
    set_state (ps, TCP_TIME_WAIT);
    break;
  case TCP_CLOSE_WAIT:
    set_state (ps, TCP_LAST_ACK);
    break;
  case TCP_LAST_ACK:
  case TCP_TIME_WAIT:
  case TCP_CLOSED:
  default:
    break;
  }

  set_state_closed (ps, err);
}

static void
adjustMTU(ptcp_socket_t *ps)
{
  // Determine our current mss level, so that we can adjust appropriately later
  for (ps->msslevel = 0;
       PACKET_MAXIMUMS[ps->msslevel + 1] > 0;
       ++ps->msslevel) {
    if (((uint16_t)PACKET_MAXIMUMS[ps->msslevel]) <= ps->mtu_advise) {
      break;
    }
  }
  ps->mss = ps->mtu_advise - PACKET_OVERHEAD;
  // !?! Should we reset ps->largest here?
//  DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Adjusting mss to %d bytes", ps->mss);
  // Enforce minimums on ssthresh and cwnd
  ps->ssthresh = max(ps->ssthresh, 2 * ps->mss);
  ps->cwnd = max(ps->cwnd, ps->mss);
}

static void
apply_window_scale_option (ptcp_socket_t *ps, uint8_t scale_factor)
{
   ps->swnd_scale = scale_factor;
}

static void
apply_fin_ack_option (ptcp_socket_t *ps)
{
  ps->support_fin_ack = TRUE;
}

static void
apply_option (ptcp_socket_t *ps, uint8_t kind, const uint8_t *data,
    uint32_t len)
{
  switch (kind) {
  case TCP_OPT_MSS:
//    DEBUG (PSEUDO_TCP_DEBUG_NORMAL,
//        "Peer specified MSS option which is not supported.");
    // TODO: Implement.
    break;
  case TCP_OPT_WND_SCALE:
    // Window scale factor.
    // http://www.ietf.org/rfc/rfc1323.txt
    if (len != 1) {
//      DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Invalid window scale option received.");
      return;
    }
    apply_window_scale_option(ps, data[0]);
    break;
  case TCP_OPT_FIN_ACK:
    // FIN-ACK support.
//    DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "FIN-ACK support enabled.");
    apply_fin_ack_option (ps);
    break;
  case TCP_OPT_EOL:
  case TCP_OPT_NOOP:
    /* Nothing to do. */
    break;
  default:
//    DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Invalid TCP option %u", kind);
    break;
  }
}

static void
parse_options (ptcp_socket_t *ps, const uint8_t *data, uint32_t len)
{
  int has_window_scaling_option = FALSE;
  int has_fin_ack_option = FALSE;
  uint32_t pos = 0;

  // See http://www.freesoft.org/CIE/Course/Section4/8.htm for
  // parsing the options list.
  while (pos < len) {
    uint8_t kind = TCP_OPT_EOL;
    uint8_t opt_len;

    if (len < pos + 1)
      return;

    kind = data[pos];
    pos++;

    if (kind == TCP_OPT_EOL) {
      // End of option list.
      break;
    } else if (kind == TCP_OPT_NOOP) {
      // No op.
      continue;
    }

    if (len < pos + 1)
      return;

    // Length of this option.
    opt_len = data[pos];
    pos++;

    if (len < pos + opt_len)
      return;

    // Content of this option.
    if (opt_len <= len - pos) {
      apply_option (ps, kind, data + pos, opt_len);
      pos += opt_len;
    } else {
//      DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Invalid option length received.");
      return;
    }

    if (kind == TCP_OPT_WND_SCALE)
      has_window_scaling_option = TRUE;
    else if (kind == TCP_OPT_FIN_ACK)
      has_fin_ack_option = TRUE;
  }

  if (!has_window_scaling_option) {
//    DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Peer doesn't support window scaling");
    if (ps->rwnd_scale > 0) {
      // Peer doesn't support TCP options and window scaling.
      // Revert receive buffer size to default value.
      resize_receive_buffer (ps, DEFAULT_RCV_BUF_SIZE);
      ps->swnd_scale = 0;
    }
  }

  if (!has_fin_ack_option) {
//    DEBUG (PSEUDO_TCP_DEBUG_NORMAL, "Peer doesn't support FIN-ACK");
    ps->support_fin_ack = FALSE;
  }
}

#if 0
static void
resize_send_buffer (ptcp_socket_t *ps, uint32_t new_size)
{
  ps->sbuf_len = new_size;
  ptcp_fifo_set_capacity (&ps->sbuf, new_size);
}
#endif


static void
resize_receive_buffer (ptcp_socket_t *ps, uint32_t new_size)
{
  uint8_t scale_factor = 0;
  int result;
  size_t available_space;

  if (ps->rbuf_len == new_size)
    return;

  // Determine the scale factor such that the scaled window size can fit
  // in a 16-bit unsigned integer.
  while (new_size > 0xFFFF) {
    ++scale_factor;
    new_size >>= 1;
  }

  // Determine the proper size of the buffer.
  new_size <<= scale_factor;
  result = ptcp_fifo_set_capacity (&ps->rbuf, new_size);

  // Make sure the new buffer is large enough to contain data in the old
  // buffer. This should always be true because this method is called either
  // before connection is established or when peers are exchanging connect
  // messages.
  assert(result);
  ps->rbuf_len = new_size;
  ps->rwnd_scale = scale_factor;
  ps->ssthresh = new_size;

  available_space = ptcp_fifo_get_write_remaining (&ps->rbuf);
  ps->rcv_wnd = available_space;
}

int
ptcp_get_available_bytes (ptcp_socket_t *ps)
{
  if (ps->state != TCP_ESTABLISHED) {
    return -1;
  }

  return ptcp_fifo_get_buffered (&ps->rbuf);
}

int
ptcp_can_send (ptcp_socket_t *ps)
{
  return (ptcp_get_available_send_space (ps) > 0);
}

size_t
ptcp_get_available_send_space (ptcp_socket_t *ps)
{
  size_t ret;


  if (ps->state == TCP_ESTABLISHED)
    ret = ptcp_fifo_get_write_remaining (&ps->sbuf);
  else
    ret = 0;

  if (ret == 0)
    ps->bWriteEnable = TRUE;

  return ret;
}

/* State names are capitalised and formatted as in RFC 793. */
static const char *
ptcp_state_get_name (ptcp_state_t state)
{
  switch (state) {
  case TCP_LISTEN: return "LISTEN";
  case TCP_SYN_SENT: return "SYN-SENT";
  case TCP_SYN_RECEIVED: return "SYN-RECEIVED";
  case TCP_ESTABLISHED: return "ESTABLISHED";
  case TCP_CLOSED: return "CLOSED";
  case TCP_FIN_WAIT_1: return "FIN-WAIT-1";
  case TCP_FIN_WAIT_2: return "FIN-WAIT-2";
  case TCP_CLOSING: return "CLOSING";
  case TCP_TIME_WAIT: return "TIME-WAIT";
  case TCP_CLOSE_WAIT: return "CLOSE-WAIT";
  case TCP_LAST_ACK: return "LAST-ACK";
  default: return "UNKNOWN";
  }
}

static void
set_state (ptcp_socket_t *ps, ptcp_state_t new_state)
{
  ptcp_state_t old_state = ps->state;

  if (new_state == old_state)
    return;

  logi("State %s → %s.\n",
      ptcp_state_get_name (old_state),
      ptcp_state_get_name (new_state));

  /* Check whether it’s a valid state transition. */
#define TRANSITION(OLD, NEW) \
    (old_state == TCP_##OLD && \
     new_state == TCP_##NEW)

  /* Valid transitions. See: RFC 793, p23; RFC 1122, §4.2.2.8. */
  assert (/* RFC 793, p23. */
            TRANSITION (CLOSED, SYN_SENT) ||
            TRANSITION (SYN_SENT, CLOSED) ||
            TRANSITION (CLOSED, LISTEN) ||
            TRANSITION (LISTEN, CLOSED) ||
            TRANSITION (LISTEN, SYN_SENT) ||
            TRANSITION (LISTEN, SYN_RECEIVED) ||
            TRANSITION (SYN_SENT, SYN_RECEIVED) ||
            TRANSITION (SYN_RECEIVED, ESTABLISHED) ||
            TRANSITION (SYN_SENT, ESTABLISHED) ||
            TRANSITION (SYN_RECEIVED, FIN_WAIT_1) ||
            TRANSITION (ESTABLISHED, FIN_WAIT_1) ||
            TRANSITION (ESTABLISHED, CLOSE_WAIT) ||
            TRANSITION (FIN_WAIT_1, FIN_WAIT_2) ||
            TRANSITION (FIN_WAIT_1, CLOSING) ||
            TRANSITION (CLOSE_WAIT, LAST_ACK) ||
            TRANSITION (FIN_WAIT_2, TIME_WAIT) ||
            TRANSITION (CLOSING, TIME_WAIT) ||
            TRANSITION (LAST_ACK, CLOSED) ||
            TRANSITION (TIME_WAIT, CLOSED) ||
            /* RFC 1122, §4.2.2.8. */
            TRANSITION (SYN_RECEIVED, LISTEN) ||
            TRANSITION (FIN_WAIT_1, TIME_WAIT));

#undef TRANSITION

  ps->state = new_state;
}

static void
set_state_established (ptcp_socket_t *ps)
{
  set_state (ps, TCP_ESTABLISHED);

  adjustMTU (ps);
  if (ps->callbacks.on_opened)
    ps->callbacks.on_opened(ps, ps->callbacks.data);
}

/* (err == 0) means no error. */
static void
set_state_closed (ptcp_socket_t *ps, uint32_t err)
{
  set_state (ps, TCP_CLOSED);

  /* Only call the callback if there was an error. */
  if (ps->callbacks.on_closed && err != 0)
    ps->callbacks.on_closed(ps, err, ps->callbacks.data);
}

int
ptcp_is_closed (ptcp_socket_t *ps)
{
  return (ps->state == TCP_CLOSED);
}

int
ptcp_is_closed_remotely (ptcp_socket_t *ps)
{
  return ptcp_state_has_received_fin (ps->state);
}

//===========================
timer_t timeout_add(uint64_t msec, void (*func)(union sigval sv), void *data)
{
    timer_t timerid = (timer_t)-1;
    struct sigevent sev;

    memset(&sev, 0, sizeof(struct sigevent));

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = func;
    sev.sigev_value.sival_ptr = data;

    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
        perror("timer_create");
    }

    return timerid;
}

#if 0
static int del_timer(timer_t id)
{
    return timer_delete(id);
}
#endif

static void clock_reset(ptcp_socket_t *p, uint64_t timeout)
{
    timer_t *timer_id = &p->timer_id; 
    struct itimerspec its;

    its.it_value.tv_sec = timeout / 1000; 
    its.it_value.tv_nsec = timeout % 1000 * 1000000;
    its.it_interval.tv_sec = 0; 
    its.it_interval.tv_nsec = 0; 
    if (timer_settime(*timer_id, 0, &its, NULL) < 0) {
        perror("timer_settime");
        loge("time = %ld s, %ld ns\n", 
                    its.it_value.tv_sec, its.it_value.tv_nsec );
    }
}

static void adjust_clock(ptcp_socket_t *p);
static void notify_clock(union sigval sv)
{
    void *data = (void *)sv.sival_ptr;
    ptcp_socket_t *p = (ptcp_socket_t *)data;
//    printf("Socket %p: Notifying clock\n", p);
    ptcp_notify_clock(p);
    adjust_clock(p);
}

static void adjust_clock(ptcp_socket_t *p)
{
    uint64_t timeout = 0;
    if (ptcp_get_next_clock(p, &timeout)) {
        timeout -= get_monotonic_time () / 1000;
//        printf("Socket %p: Adjusting clock to %"PRIu64" ms\n", p, timeout);
        clock_reset(p, timeout);
//        if (p->timer_id != 0)
//            del_timer(p->timer_id);
//        p->timer_id = timeout_add(timeout, notify_clock, p);
    } else {
        loge("Socket %p should be destroyed, it's done\n", p);
    }
}


static ptcp_write_result_t ptcp_write(ptcp_socket_t *p, const char *buf, uint32_t len, void *data)
{
    ssize_t res;
    res = sendto(p->fd, buf, len, 0, (struct sockaddr *)&p->si, sizeof(struct sockaddr));
    if (-1 == res) {
        loge("sendto error: %s\n", strerror(errno));
    }
    adjust_clock(p);
    if (res < len) {
        return WR_FAIL;
    }
    return WR_SUCCESS;
}

static ptcp_write_result_t ptcp_read(ptcp_socket_t *p, void *buf, size_t len)
{
    ssize_t res;
#define RECV_BUF_LEN (64*1024)
    char rbuf[RECV_BUF_LEN] = {0};
    socklen_t addrlen = sizeof(struct sockaddr);
    res = recvfrom(p->fd, rbuf, sizeof(rbuf), MSG_DONTWAIT,
                (struct sockaddr *)&p->si, &addrlen);
    if (-1 == res) {
        loge("recvfrom error: %s\n", strerror(errno));
        return WR_FAIL;
    }
    if (TRUE == ptcp_notify_packet(p, rbuf, res)) {
        adjust_clock(p);
        return WR_SUCCESS;
    }
    return WR_FAIL;
}

static void on_opened(ptcp_socket_t *p, void *data)
{
//    printf("%s:%d xxxx\n", __func__, __LINE__);
    sem_post(&p->sem);
}

static void on_readable(ptcp_socket_t *p, void *data)
{
//    printf("%s:%d xxxx\n", __func__, __LINE__);
}

static void on_writable(ptcp_socket_t *p, void *data)
{
//    printf("%s:%d xxxx\n", __func__, __LINE__);
}

static void on_closed(ptcp_socket_t *p, uint32_t error, void *data)
{
//    printf("%s:%d xxxx\n", __func__, __LINE__);
}


ptcp_socket_t *ptcp_socket(void)
{
    int fds[2];
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (-1 == fd) {
        return NULL;
    }
    ptcp_callbacks_t cbs = {NULL,
                            on_opened,
                            on_readable,
                            on_writable,
                            on_closed,
                            ptcp_write};
    ptcp_socket_t *p = ptcp_create(&cbs);
    p->fd = fd;
    ptcp_notify_mtu(p, 1460);

    if (pipe(fds)) {
        loge("create pipe failed: %s\n", strerror(errno));
        return NULL;
    }
    p->on_read_fd = fds[0];
    p->on_write_fd = fds[1];
    return p;
}

ptcp_socket_t *ptcp_socket_by_fd(int fd)
{
    int fds[2];
#if 0
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (-1 == fd) {
        return NULL;
    }
#endif
    ptcp_callbacks_t cbs = {NULL,
                            on_opened,
                            on_readable,
                            on_writable,
                            on_closed,
                            ptcp_write};
    ptcp_socket_t *p = ptcp_create(&cbs);
    p->fd = fd;
    ptcp_notify_mtu(p, 1460);

    if (pipe(fds)) {
        loge("create pipe failed: %s\n", strerror(errno));
        return NULL;
    }
    p->on_read_fd = fds[0];
    p->on_write_fd = fds[1];
    return p;
}
int ptcp_socket_fd(ptcp_socket_t *p)
{
    return p->on_read_fd;
}

static void event_read(void *arg)
{
    ptcp_socket_t *ps = (ptcp_socket_t *)arg;
    char notify = '1';
    char buf[10240] = {0};
    if (WR_SUCCESS == ptcp_read(ps, buf, sizeof(buf))) {
        if (write(ps->on_write_fd, &notify, sizeof(notify)) != 1) {
            loge("write pipe failed: %s\n", strerror(errno));
        }
    }
}

static void *event_loop(void *arg)
{
    struct _ptcp_socket *ps = (struct _ptcp_socket *)arg;
    int epfd;
    int ret, i;
    struct epoll_event evbuf[MAX_EPOLL_EVENT];
    struct epoll_event event;

    int fd = ps->fd;
    epfd = epoll_create(1);
    if (epfd == -1) {
        perror("epoll_create");
        return NULL;
    }

    memset(&event, 0, sizeof(event));
    event.data.ptr = ps;
    event.events = EPOLLIN | EPOLLET;

    if (-1 == epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event)) {
        perror("epoll_ctl");
        close(epfd);
        return NULL;
    }

    while (1) {
        ret = epoll_wait(epfd, evbuf, MAX_EPOLL_EVENT, -1);
        if (ret == -1) {
            perror("epoll_wait");
            continue;
        }
        for (i = 0; i < ret; i++) {
            if (evbuf[i].data.fd == -1)
                continue;
            if (evbuf[i].events & (EPOLLERR | EPOLLHUP)) {
                perror("epoll error");
            }
            if (evbuf[i].events & EPOLLOUT) {
                perror("epoll out");
            }
            if (evbuf[i].events & EPOLLIN) {
                event_read(evbuf[i].data.ptr);
            }
        }
    }
    return NULL;
}

int ptcp_connect(ptcp_socket_t *p, const struct sockaddr *addr,
                   socklen_t addrlen)
{
    pthread_t tid;
    if (-1 == connect(p->fd, addr, addrlen)) {
        loge("connect error: %s\n", strerror(errno));
        return -1;
    }
    memcpy(&p->si, (struct sockaddr_in *)addr, addrlen);

    pthread_create(&tid, NULL, event_loop, p);
    if (-1 == _ptcp_connect(p)) {
        loge("ptcp_connect failed\n");
    }
    sem_wait(&p->sem);
    return 0;
}

int ptcp_bind(ptcp_socket_t *p, const struct sockaddr *addr,
                socklen_t addrlen)
{
    if (-1 == bind(p->fd, addr, addrlen)) {
        loge("bind error %d: %s\n", errno, strerror(errno));
        struct sockaddr_in *si = (struct sockaddr_in *)addr;
        loge("bind fd = %d, ip = %d: port = %d\n", p->fd, si->sin_addr.s_addr, ntohs(si->sin_port));
        //return -1;
    }
    memcpy(&p->si, (struct sockaddr_in *)&addr, addrlen);
    return 0;
}

int ptcp_listen(ptcp_socket_t *p, int backlog)
{
    pthread_t tid;

    pthread_create(&tid, NULL, event_loop, p);
    sem_wait(&p->sem);
    return 0;
}

void ptcp_close(ptcp_socket_t *p)
{
    close(p->on_read_fd);
    close(p->on_write_fd);
    _ptcp_close(p, 0);
    ptcp_destroy(p);
}

#if 0
static void printf_buf(const char *buf, uint32_t len)
{
    int i;
    for (i = 0; i < len; i++) {
        if (!(i%16))
           printf("\n0x%04x: ", buf[i]);
        printf("%02x ", (buf[i] & 0xFF));
    }
    printf("\n");
}
#endif
