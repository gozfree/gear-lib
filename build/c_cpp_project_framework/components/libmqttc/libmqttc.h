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
#ifndef LIBMQTTC_H
#define LIBMQTTC_H

#define LIBMQTTC_VERSION "0.0.1"

#include <libposix.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mqtt_client;
struct mqttc_ops {
    void *(*init)(const char *host, uint16_t port);
    void (*deinit)(struct mqtt_client *c);
    int (*connect)(struct mqtt_client *c);
    int (*disconnect)(struct mqtt_client *c);
    int (*read)(struct mqtt_client *c, void *buf, size_t len);
    int (*write)(struct mqtt_client *c, const void *buf, size_t len);
};

typedef struct Timer {
    struct timeval end_time;
} Timer;

typedef struct Network {
    int my_socket;
    int (*read) (struct Network*, unsigned char*, int, int);
    int (*write) (struct Network*, unsigned char*, int, int);
} Network;

enum mqtt_errors {
    MQTTPACKET_BUFFER_TOO_SHORT = -2,
    MQTTPACKET_READ_ERROR = -1,
    MQTTPACKET_READ_COMPLETE
};

enum mqtt_connack_return_codes {
    MQTT_CONNECTION_ACCEPTED = 0,
    MQTT_UNNACCEPTABLE_PROTOCOL = 1,
    MQTT_CLIENTID_REJECTED = 2,
    MQTT_SERVER_UNAVAILABLE = 3,
    MQTT_BAD_USERNAME_OR_PASSWORD = 4,
    MQTT_NOT_AUTHORIZED = 5,
};

/**
 * Bitfields for the MQTT header byte.
 */
typedef union
{
    unsigned char byte;          /**< the whole byte */
#if defined(REVERSED)
    struct {
        unsigned int type   : 4; /**< message type nibble */
        unsigned int dup    : 1; /**< DUP flag bit */
        unsigned int qos    : 2; /**< QoS value, 0, 1 or 2 */
        unsigned int retain : 1; /**< retained flag bit */
    } bits;
#else
    struct {
        unsigned int retain : 1; /**< retained flag bit */
        unsigned int qos    : 2; /**< QoS value, 0, 1 or 2 */
        unsigned int dup    : 1; /**< DUP flag bit */
        unsigned int type   : 4; /**< message type nibble */
    } bits;
#endif
} mqtt_header;

typedef struct {
    int len;
    char* data;
} mqtt_len_string;

typedef struct {
    char* cstring;
    mqtt_len_string lenstring;
} mqtt_string;

#define mqtt_string_initializer {NULL, {0, NULL}}

typedef union
{
    unsigned char all;  /**< all connect flags */
#if defined(REVERSED)
    struct {
        unsigned int username     : 1; /**< 3.1 user name */
        unsigned int password     : 1; /**< 3.1 password */
        unsigned int willRetain   : 1; /**< will retain setting */
        unsigned int willQoS      : 2; /**< will QoS value */
        unsigned int will         : 1; /**< will flag */
        unsigned int cleansession : 1; /**< clean session flag */
        unsigned int              : 1; /**< unused */
    } bits;
#else
    struct {
        unsigned int              : 1; /**< unused */
        unsigned int cleansession : 1; /**< cleansession flag */
        unsigned int will         : 1; /**< will flag */
        unsigned int willQoS      : 2; /**< will QoS value */
        unsigned int willRetain   : 1; /**< will retain setting */
        unsigned int password     : 1; /**< 3.1 password */
        unsigned int username     : 1; /**< 3.1 user name */
    } bits;
#endif
} mqtt_conn_flags;/**< connect flags byte */

/**
 * Defines the MQTT "Last Will and Testament" (LWT) settings for
 * the connect packet.
 */
typedef struct
{
    /** The eyecatcher for this structure.  must be MQTW. */
    char struct_id[4];
    /** The version number of this structure.  Must be 0 */
    int struct_version;
    /** The LWT topic to which the LWT message will be published. */
    mqtt_string topicName;
    /** The LWT payload. */
    mqtt_string message;
    /**
     * The retained flag for the LWT message (see MQTTAsync_message.retained).
     */
    unsigned char retained;
    /**
     * The quality of service setting for the LWT message (see
     * MQTTAsync_message.qos and @ref qos).
     */
    char qos;
} mqtt_pkt_will_options;

#define mqtt_pkt_will_options_initializer \
    { {'M', 'Q', 'T', 'W'}, 0, {NULL, {0, NULL}}, {NULL, {0, NULL}}, 0, 0 }

typedef struct
{
    /** The eyecatcher for this structure.  must be MQTC. */
    char struct_id[4];
    /** The version number of this structure.  Must be 0 */
    int struct_version;
    /** Version of MQTT to be used.  3 = 3.1 4 = 3.1.1
     */
    unsigned char mqtt_version;
    mqtt_string clientID;
    unsigned short keepAliveInterval;
    unsigned char cleansession;
    unsigned char willFlag;
    mqtt_pkt_will_options will;
    mqtt_string username;
    mqtt_string password;
} mqtt_pkt_conn_data;

#define mqtt_pkt_conn_data_initializer \
    { {'M', 'Q', 'T', 'C'}, 0, 4, {NULL, {0, NULL}}, 60, 1, 0, \
        mqtt_pkt_will_options_initializer, {NULL, {0, NULL}}, {NULL, {0, NULL}} }

typedef union
{
    unsigned char all;  /**< all connack flags */
#if defined(REVERSED)
    struct {
        unsigned int reserved       : 7; /**< unused */
        unsigned int sessionpresent : 1; /**< session present flag */
    } bits;
#else
    struct {
        unsigned int sessionpresent : 1; /**< session present flag */
        unsigned int reserved       : 7; /**< unused */
    } bits;
#endif
} mqtt_connack_flags;   /**< connack flags byte */

typedef struct {
    int (*getfn)(void *, unsigned char*, int); /* must return -1 for error, 0 for call again, or the number of bytes read */
    void *sck;  /* pointer to whatever the system may use to identify the transport */
    int multiplier;
    int rem_len;
    int len;
    char state;
} mqtt_transport;

#if !defined(MAX_MESSAGE_HANDLERS)
#define MAX_MESSAGE_HANDLERS 5 /* redefinable - how many subscriptions do you want? */
#endif

enum mqtt_qos { MQTT_QOS0, MQTT_QOS1, MQTT_QOS2, MQTT_SUBFAIL=0x80 };

/* all failure return codes must be negative */
enum returnCode { MQTT_BUFFER_OVERFLOW = -2, MQTT_FAILURE = -1, MQTT_SUCCESS = 0 };

typedef struct mqtt_msg
{
    enum mqtt_qos qos;
    unsigned char retained;
    unsigned char dup;
    unsigned short id;
    void *payload;
    size_t payloadlen;
} mqtt_msg;

typedef struct MessageData
{
    mqtt_msg* message;
    mqtt_string* topicName;
} MessageData;

typedef struct mqtt_connack_data
{
    unsigned char rc;
    unsigned char sessionPresent;
} mqtt_connack_data;

typedef struct mqtt_suback_data
{
    enum mqtt_qos grantedQoS;
} mqtt_suback_data;

typedef void (*messageHandler)(MessageData*);

typedef struct mqtt_client
{
    const struct mqttc_ops *ops;
    char *host;
    int port;
    int fd;
    unsigned int next_packetid;
    unsigned int command_timeout_ms;
    size_t buf_size;
    size_t readbuf_size;
    unsigned char *buf;
    unsigned char *readbuf;
    unsigned int keepAliveInterval;
    char ping_outstanding;
    int isconnected;
    int cleansession;

    struct MessageHandlers {
        const char* topicFilter;
        void (*fp) (MessageData*);
    } messageHandlers[MAX_MESSAGE_HANDLERS];      /* Message handlers are indexed by subscription topic */

    void (*defaultMessageHandler) (MessageData*);

    Network ipstack;
    Timer last_sent, last_received;
#if defined(MQTT_TASK)
    Mutex mutex;
    Thread thread;
#endif
    void *priv;
} mqtt_client;

/**
 * Create an MQTT client object
 * @param client
 * @param
 */
int mqtt_client_init(mqtt_client* c, const char *host, int port);
void mqtt_client_deinit(mqtt_client* c);

/** MQTT Connect - send an MQTT connect packet down the network and wait for a Connack
 *  The nework object must be connected to the network endpoint before calling this
 *  @param options - connect options
 *  @return success code
 */
int mqtt_connect(mqtt_client *c, mqtt_pkt_conn_data *options, mqtt_connack_data *data);

/** MQTT Disconnect - send an MQTT disconnect packet and close the connection
 *  @param client - the client object to use
 *  @return success code
 */
int mqtt_disconnect(mqtt_client* client);

/** MQTT Publish - send an MQTT publish packet and wait for all acks to complete for all QoSs
 *  @param client - the client object to use
 *  @param topic - the topic to publish to
 *  @param message - the message to send
 *  @return success code
 */
int mqtt_publish(mqtt_client* client, const char*, mqtt_msg*);

/** MQTT Subscribe - send an MQTT subscribe packet and wait for suback before returning.
 *  @param client - the client object to use
 *  @param topicFilter - the topic filter to subscribe to
 *  @param message - the message to send
 *  @param data - suback granted QoS returned
 *  @return success code
 */
int mqtt_subscribe(mqtt_client* client, const char* topicFilter, enum mqtt_qos, messageHandler, mqtt_suback_data* data);

/** MQTT Subscribe - send an MQTT unsubscribe packet and wait for unsuback before returning.
 *  @param client - the client object to use
 *  @param topicFilter - the topic filter to unsubscribe from
 *  @return success code
 */
int mqtt_unsubscribe(mqtt_client* client, const char* topicFilter);

/** MQTT Yield - MQTT background
 *  @param client - the client object to use
 *  @param time - the time, in milliseconds, to yield for
 *  @return success code
 */
int mqtt_yield(mqtt_client* client, int time);

/** MQTT isConnected
 *  @param client - the client object to use
 *  @return truth value indicating whether the client is connected to the server
 */
int mqtt_is_connected(mqtt_client* client);

#if defined(MQTT_TASK)
/** MQTT start background thread for a client.  After this, mqtt_yield should not be called.
*  @param client - the client object to use
*  @return success code
*/
int mqtt_start_task(mqtt_client* client);
#endif

#ifdef __cplusplus
}
#endif
#endif
