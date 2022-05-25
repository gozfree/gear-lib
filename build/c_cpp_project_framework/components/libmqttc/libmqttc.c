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
#include "libmqttc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#define SOCK_API    0

#define MAX_PACKET_ID 65535 /* according to the MQTT specification - do not change! */
#define MAX_NO_OF_REMAINING_LENGTH_BYTES 4

#define NOSTACKTRACE 0
#define TRACE_MINIMUM 0
#define TRACE_MEDIUM 1
#define TRACE_MAXIMUM  2

#if defined(NOSTACKTRACE)
#define FUNC_ENTRY
#define FUNC_ENTRY_NOLOG
#define FUNC_ENTRY_MED
#define FUNC_ENTRY_MAX
#define FUNC_EXIT
#define FUNC_EXIT_NOLOG
#define FUNC_EXIT_MED
#define FUNC_EXIT_MAX
#define FUNC_EXIT_RC(x)
#define FUNC_EXIT_MED_RC(x)
#define FUNC_EXIT_MAX_RC(x)

#else

#define FUNC_ENTRY StackTrace_entry(__func__, __LINE__, TRACE_MINIMUM)
#define FUNC_ENTRY_NOLOG StackTrace_entry(__func__, __LINE__, -1)
#define FUNC_ENTRY_MED StackTrace_entry(__func__, __LINE__, TRACE_MEDIUM)
#define FUNC_ENTRY_MAX StackTrace_entry(__func__, __LINE__, TRACE_MAXIMUM)
#define FUNC_EXIT StackTrace_exit(__func__, __LINE__, NULL, TRACE_MINIMUM)
#define FUNC_EXIT_NOLOG StackTrace_exit(__func__, __LINE__, NULL, -1)
#define FUNC_EXIT_MED StackTrace_exit(__func__, __LINE__, NULL, TRACE_MEDIUM)
#define FUNC_EXIT_MAX StackTrace_exit(__func__, __LINE__, NULL, TRACE_MAXIMUM)
#define FUNC_EXIT_RC(x) StackTrace_exit(__func__, __LINE__, &x, TRACE_MINIMUM)
#define FUNC_EXIT_MED_RC(x) StackTrace_exit(__func__, __LINE__, &x, TRACE_MEDIUM)
#define FUNC_EXIT_MAX_RC(x) StackTrace_exit(__func__, __LINE__, &x, TRACE_MAXIMUM)

#endif

extern const struct mqttc_ops mqttc_socket_ops;

static void StackTrace_entry(const char* name, int line, int trace)
{
    printf("%s:%d %d\n", name, line, trace);
}

static void StackTrace_exit(const char* name, int line, void* return_value, int trace)
{
    printf("%s:%d ret=%d %d\n", name, line, *(int *)return_value, trace);
}

#define min(a, b) ((a < b) ? 1 : 0)

enum mqtt_msg_types {
    RESERVED = 0,   CONNECT,     CONNACK, PUBLISH, PUBACK,
    PUBREC,         PUBREL,      PUBCOMP, SUBSCRIBE, SUBACK,
    UNSUBSCRIBE,    UNSUBACK,    PINGREQ, PINGRESP, DISCONNECT
};

static const char* mqtt_pkt_names[] =
{
    "RESERVED",    "CONNECT",  "CONNACK", "PUBLISH",   "PUBACK",
    "PUBREC",      "PUBREL",   "PUBCOMP", "SUBSCRIBE", "SUBACK",
    "UNSUBSCRIBE", "UNSUBACK", "PINGREQ", "PINGRESP",  "DISCONNECT"
};

static void TimerInit(Timer* timer)
{
    timer->end_time = (struct timeval){0, 0};
}

static char TimerIsExpired(Timer* timer)
{
    struct timeval now, res;
    gettimeofday(&now, NULL);
    timersub(&timer->end_time, &now, &res);
    return res.tv_sec < 0 || (res.tv_sec == 0 && res.tv_usec <= 0);
}

static void TimerCountdownMS(Timer* timer, unsigned int timeout)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval interval = {timeout / 1000, (timeout % 1000) * 1000};
    timeradd(&now, &interval, &timer->end_time);
}

static void TimerCountdown(Timer* timer, unsigned int timeout)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval interval = {timeout, 0};
    timeradd(&now, &interval, &timer->end_time);
}

static int TimerLeftMS(Timer* timer)
{
    struct timeval now, res;
    gettimeofday(&now, NULL);
    timersub(&timer->end_time, &now, &res);
    //printf("left %d ms\n", (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000);
    return (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000;
}

static int mqtt_stringFormat_connect(char* strbuf, int strbuflen, mqtt_pkt_conn_data* data)
{
    int strindex = 0;

    strindex = snprintf(strbuf, strbuflen,
                    "CONNECT MQTT version %d, client id %.*s, clean session %d, keep alive %d",
                    (int)data->mqtt_version, data->clientID.lenstring.len, data->clientID.lenstring.data,
                    (int)data->cleansession, data->keepAliveInterval);
    if (data->willFlag)
        strindex += snprintf(&strbuf[strindex], strbuflen - strindex,
                        ", will QoS %d, will retain %d, will topic %.*s, will message %.*s",
                        data->will.qos, data->will.retained,
                        data->will.topicName.lenstring.len, data->will.topicName.lenstring.data,
                        data->will.message.lenstring.len, data->will.message.lenstring.data);
    if (data->username.lenstring.data && data->username.lenstring.len > 0)
        strindex += snprintf(&strbuf[strindex], strbuflen - strindex,
                        ", user name %.*s", data->username.lenstring.len, data->username.lenstring.data);
    if (data->password.lenstring.data && data->password.lenstring.len > 0)
        strindex += snprintf(&strbuf[strindex], strbuflen - strindex,
                            ", password %.*s", data->password.lenstring.len, data->password.lenstring.data);
    return strindex;
}

static int mqtt_stringFormat_connack(char* strbuf, int strbuflen, unsigned char connack_rc, unsigned char sessionPresent)
{
    int strindex = snprintf(strbuf, strbuflen, "CONNACK session present %d, rc %d", sessionPresent, connack_rc);
    return strindex;
}

static int mqtt_stringFormat_publish(char* strbuf, int strbuflen, unsigned char dup, int qos, unsigned char retained,
                unsigned short packetid, mqtt_string topicName, unsigned char* payload, int payloadlen)
{
    int strindex = snprintf(strbuf, strbuflen,
                    "PUBLISH dup %d, QoS %d, retained %d, packet id %d, topic %.*s, payload length %d, payload %.*s",
                    dup, qos, retained, packetid,
                    (topicName.lenstring.len < 20) ? topicName.lenstring.len : 20, topicName.lenstring.data,
                    payloadlen, (payloadlen < 20) ? payloadlen : 20, payload);
    return strindex;
}

static int mqtt_stringFormat_ack(char* strbuf, int strbuflen, unsigned char packettype, unsigned char dup, unsigned short packetid)
{
    int strindex = snprintf(strbuf, strbuflen, "%s, packet id %d", mqtt_pkt_names[packettype], packetid);
    if (dup)
        strindex += snprintf(strbuf + strindex, strbuflen - strindex, ", dup %d", dup);
    return strindex;
}

static int mqtt_stringFormat_subscribe(char* strbuf, int strbuflen, unsigned char dup, unsigned short packetid, int count,
    mqtt_string topicFilters[], int requestedQoSs[])
{
    return snprintf(strbuf, strbuflen,
               "SUBSCRIBE dup %d, packet id %d count %d topic %.*s qos %d",
               dup, packetid, count, topicFilters[0].lenstring.len,
               topicFilters[0].lenstring.data, requestedQoSs[0]);
}

static int mqtt_stringFormat_suback(char* strbuf, int strbuflen, unsigned short packetid, int count, int* grantedQoSs)
{
    return snprintf(strbuf, strbuflen,
                    "SUBACK packet id %d count %d granted qos %d", packetid, count, grantedQoSs[0]);
}

static int mqtt_stringFormat_unsubscribe(char* strbuf, int strbuflen, unsigned char dup, unsigned short packetid,
                int count, mqtt_string topicFilters[])
{
    return snprintf(strbuf, strbuflen,
                    "UNSUBSCRIBE dup %d, packet id %d count %d topic %.*s",
                    dup, packetid, count,
                    topicFilters[0].lenstring.len, topicFilters[0].lenstring.data);
}

/**
 * Encodes the message length according to the MQTT algorithm
 * @param buf the buffer into which the encoded data is written
 * @param length the length to be encoded
 * @return the number of bytes written to buffer
 */
static int mqtt_pkt_encode(unsigned char* buf, int length)
{
    int rc = 0;

    FUNC_ENTRY;
    do {
        char d = length % 128;
        length /= 128;
        /* if there are more digits to encode, set the top bit of this digit */
        if (length > 0)
                d |= 0x80;
        buf[rc++] = d;
    } while (length > 0);
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
 * Decodes the message length according to the MQTT algorithm
 * @param getcharfn pointer to function to read the next character from the data source
 * @param value the decoded length returned
 * @return the number of bytes read from the socket
 */
static int mqtt_pkt_decode(int (*getcharfn)(unsigned char*, int), int* value)
{
    unsigned char c;
    int multiplier = 1;
    int len = 0;

    FUNC_ENTRY;
    *value = 0;
    do {
        int rc = MQTTPACKET_READ_ERROR;

        if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES) {
            rc = MQTTPACKET_READ_ERROR; /* bad data */
            goto exit;
        }
        rc = (*getcharfn)(&c, 1);
        if (rc != 1)
            goto exit;
        *value += (c & 127) * multiplier;
        multiplier *= 128;
    } while ((c & 128) != 0);
exit:
    FUNC_EXIT_RC(len);
    return len;
}

static int mqtt_pkt_len(int rem_len)
{
    rem_len += 1; /* header byte */

    /* now remaining_length field */
    if (rem_len < 128)
        rem_len += 1;
    else if (rem_len < 16384)
        rem_len += 2;
    else if (rem_len < 2097151)
        rem_len += 3;
    else
        rem_len += 4;
    return rem_len;
}

static unsigned char* bufptr;

static int bufchar(unsigned char* c, int count)
{
    int i;

    for (i = 0; i < count; ++i)
        *c = *bufptr++;
    return count;
}

static int mqtt_pkt_decodeBuf(unsigned char* buf, int* value)
{
    bufptr = buf;
    return mqtt_pkt_decode(bufchar, value);
}

/**
 * Calculates an integer from two bytes read from the input buffer
 * @param pptr pointer to the input buffer - incremented by the number of bytes used & returned
 * @return the integer value calculated
 */
static int readInt(unsigned char** pptr)
{
    unsigned char* ptr = *pptr;
    int len = 256*(*ptr) + (*(ptr+1));
    *pptr += 2;
    return len;
}

/**
 * Reads one character from the input buffer.
 * @param pptr pointer to the input buffer - incremented by the number of bytes used & returned
 * @return the character read
 */
static char readChar(unsigned char** pptr)
{
    char c = **pptr;
    (*pptr)++;
    return c;
}

/**
 * Writes one character to an output buffer.
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param c the character to write
 */
static void writeChar(unsigned char** pptr, char c)
{
    **pptr = c;
    (*pptr)++;
}

/**
 * Writes an integer as 2 bytes to an output buffer.
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param anInt the integer to write
 */
static void writeInt(unsigned char** pptr, int anInt)
{
    **pptr = (unsigned char)(anInt / 256);
    (*pptr)++;
    **pptr = (unsigned char)(anInt % 256);
    (*pptr)++;
}

/**
 * Writes a "UTF" string to an output buffer.  Converts C string to length-delimited.
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param string the C string to write
 */
static void writeCString(unsigned char** pptr, const char* string)
{
    int len = strlen(string);
    writeInt(pptr, len);
    memcpy(*pptr, string, len);
    *pptr += len;
}

static void writemqtt_string(unsigned char** pptr, mqtt_string mqttstring)
{
    if (mqttstring.lenstring.len > 0) {
        writeInt(pptr, mqttstring.lenstring.len);
        memcpy(*pptr, mqttstring.lenstring.data, mqttstring.lenstring.len);
        *pptr += mqttstring.lenstring.len;
    } else if (mqttstring.cstring)
        writeCString(pptr, mqttstring.cstring);
    else
        writeInt(pptr, 0);
}

/**
 * @param mqttstring the mqtt_string structure into which the data is to be read
 * @param pptr pointer to the output buffer - incremented by the number of bytes used & returned
 * @param enddata pointer to the end of the data: do not read beyond
 * @return 1 if successful, 0 if not
 */
static int readMQTTLenString(mqtt_string* mqttstring, unsigned char** pptr, unsigned char* enddata)
{
    int rc = 0;

    FUNC_ENTRY;
    /* the first two bytes are the length of the string */
    if (enddata - (*pptr) > 1) /* enough length to read the integer? */
    {
        mqttstring->lenstring.len = readInt(pptr); /* increments pptr to point past length */
        if (&(*pptr)[mqttstring->lenstring.len] <= enddata) {
            mqttstring->lenstring.data = (char*)*pptr;
            *pptr += mqttstring->lenstring.len;
            rc = 1;
        }
    }
    mqttstring->cstring = NULL;
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
 * Return the length of the MQTTstring - C string if there is one, otherwise the length delimited string
 * @param mqttstring the string to return the length of
 * @return the length of the string
 */
static int mqtt_strlen(mqtt_string mqttstring)
{
    int rc = 0;

    if (mqttstring.cstring)
        rc = strlen(mqttstring.cstring);
    else
        rc = mqttstring.lenstring.len;
    return rc;
}

/**
 * Compares an mqtt_string to a C string
 * @param a the mqtt_string to compare
 * @param bptr the C string to compare
 * @return boolean - equal or not
 */
static int mqtt_pkt_equals(mqtt_string* a, char* bptr)
{
    int alen = 0;
    int blen = 0;
    char *aptr;

    if (a->cstring) {
        aptr = a->cstring;
        alen = strlen(a->cstring);
    } else {
        aptr = a->lenstring.data;
        alen = a->lenstring.len;
    }
    blen = strlen(bptr);

    return (alen == blen) && (strncmp(aptr, bptr, alen) == 0);
}

/**
 * Helper function to read packet data from some source into a buffer
 * @param buf the buffer into which the packet will be serialized
 * @param buflen the length in bytes of the supplied buffer
 * @param getfn pointer to a function which will read any number of bytes from the needed source
 * @return integer MQTT packet type, or -1 on error
 * @note  the whole message must fit into the caller's buffer
 */
int mqtt_pkt_read(unsigned char* buf, int buflen, int (*getfn)(unsigned char*, int))
{
    int rc = -1;
    mqtt_header header = {0};
    int len = 0;
    int rem_len = 0;

    /* 1. read the header byte.  This has the packet type in it */
    if ((*getfn)(buf, 1) != 1)
        goto exit;

    len = 1;
    /* 2. read the remaining length.  This is variable in itself */
    mqtt_pkt_decode(getfn, &rem_len);
    len += mqtt_pkt_encode(buf + 1, rem_len); /* put the original remaining length back into the buffer */

    /* 3. read the rest of the buffer using a callback to supply the rest of the data */
    if((rem_len + len) > buflen)
        goto exit;
    if (rem_len && ((*getfn)(buf + len, rem_len) != rem_len))
        goto exit;

    header.byte = buf[0];
    rc = header.bits.type;
exit:
    return rc;
}

/**
 * Decodes the message length according to the MQTT algorithm, non-blocking
 * @param trp pointer to a transport structure holding what is needed to solve getting data from it
 * @param value the decoded length returned
 * @return integer the number of bytes read from the socket, 0 for call again, or -1 on error
 */
static int mqtt_pkt_decodenb(mqtt_transport *trp)
{
    unsigned char c;
    int rc = MQTTPACKET_READ_ERROR;

    FUNC_ENTRY;
    if (trp->len == 0) {/* initialize on first call */
        trp->multiplier = 1;
        trp->rem_len = 0;
    }
    do {
        int frc;
        if (trp->len >= MAX_NO_OF_REMAINING_LENGTH_BYTES)
            goto exit;
        if ((frc=(*trp->getfn)(trp->sck, &c, 1)) == -1)
            goto exit;
        if (frc == 0){
            rc = 0;
            goto exit;
        }
        ++(trp->len);
        trp->rem_len += (c & 127) * trp->multiplier;
        trp->multiplier *= 128;
    } while ((c & 128) != 0);
    rc = trp->len;
exit:
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
 * Helper function to read packet data from some source into a buffer, non-blocking
 * @param buf the buffer into which the packet will be serialized
 * @param buflen the length in bytes of the supplied buffer
 * @param trp pointer to a transport structure holding what is needed to solve getting data from it
 * @return integer MQTT packet type, 0 for call again, or -1 on error
 * @note  the whole message must fit into the caller's buffer
 */
int mqtt_pkt_readnb(unsigned char* buf, int buflen, mqtt_transport *trp)
{
    int rc = -1, frc;
    mqtt_header header = {0};

    switch(trp->state){
    default:
        trp->state = 0;
        /*FALLTHROUGH*/
    case 0:
        /* read the header byte.  This has the packet type in it */
        if ((frc=(*trp->getfn)(trp->sck, buf, 1)) == -1)
            goto exit;
        if (frc == 0)
            return 0;
        trp->len = 0;
        ++trp->state;
        /*FALLTHROUGH*/
        /* read the remaining length.  This is variable in itself */
    case 1:
        if((frc=mqtt_pkt_decodenb(trp)) == MQTTPACKET_READ_ERROR)
                goto exit;
        if(frc == 0)
                return 0;
        trp->len = 1 + mqtt_pkt_encode(buf + 1, trp->rem_len); /* put the original remaining length back into the buffer */
        if((trp->rem_len + trp->len) > buflen)
                goto exit;
        ++trp->state;
        /*FALLTHROUGH*/
    case 2:
        if (trp->rem_len) {
            /* read the rest of the buffer using a callback to supply the rest of the data */
            if ((frc=(*trp->getfn)(trp->sck, buf + trp->len, trp->rem_len)) == -1)
                goto exit;
            if (frc == 0)
                return 0;
            trp->rem_len -= frc;
            trp->len += frc;
            if (trp->rem_len)
                return 0;
        }
        header.byte = buf[0];
        rc = header.bits.type;
        break;
    }

exit:
    trp->state = 0;
    return rc;
}

/**
  * Deserializes the supplied (wire) buffer into unsubscribe data
  * @param dup integer returned - the MQTT dup flag
  * @param packetid integer returned - the MQTT packet identifier
  * @param maxcount - the maximum number of members allowed in the topicFilters and requestedQoSs arrays
  * @param count - number of members in the topicFilters and requestedQoSs arrays
  * @param topicFilters - array of topic filter names
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return the length of the serialized data.  <= 0 indicates error
  */
static int mqtt_deserialize_unsubscribe(unsigned char* dup, unsigned short* packetid, int maxcount, int* count, mqtt_string topicFilters[],
                unsigned char* buf, int len)
{
    mqtt_header header = {0};
    unsigned char* curdata = buf;
    unsigned char* enddata = NULL;
    int rc = 0;
    int mylen = 0;

    FUNC_ENTRY;
    header.byte = readChar(&curdata);
    if (header.bits.type != UNSUBSCRIBE)
        goto exit;
    *dup = header.bits.dup;

    curdata += (rc = mqtt_pkt_decodeBuf(curdata, &mylen)); /* read remaining length */
    enddata = curdata + mylen;

    *packetid = readInt(&curdata);

    *count = 0;
    while (curdata < enddata) {
        if (!readMQTTLenString(&topicFilters[*count], &curdata, enddata))
            goto exit;
        (*count)++;
    }

    rc = 1;
exit:
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
  * Serializes the supplied unsuback data into the supplied buffer, ready for sending
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param packetid integer - the MQTT packet identifier
  * @return the length of the serialized data.  <= 0 indicates error
  */
int mqtt_serialize_unsuback(unsigned char* buf, int buflen, unsigned short packetid)
{
    mqtt_header header = {0};
    int rc = 0;
    unsigned char *ptr = buf;

    FUNC_ENTRY;
    if (buflen < 2) {
        rc = MQTTPACKET_BUFFER_TOO_SHORT;
        goto exit;
    }
    header.byte = 0;
    header.bits.type = UNSUBACK;
    writeChar(&ptr, header.byte); /* write header */

    ptr += mqtt_pkt_encode(ptr, 2); /* write remaining length */

    writeInt(&ptr, packetid);

    rc = ptr - buf;
exit:
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
  * Determines the length of the MQTT unsubscribe packet that would be produced using the supplied parameters
  * @param count the number of topic filter strings in topicFilters
  * @param topicFilters the array of topic filter strings to be used in the publish
  * @return the length of buffer needed to contain the serialized version of the packet
  */
static int mqtt_serialize_unsubscribeLength(int count, mqtt_string topicFilters[])
{
    int i;
    int len = 2; /* packetid */

    for (i = 0; i < count; ++i)
        len += 2 + mqtt_strlen(topicFilters[i]); /* length + topic*/
    return len;
}

/**
  * Serializes the supplied unsubscribe data into the supplied buffer, ready for sending
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @param dup integer - the MQTT dup flag
  * @param packetid integer - the MQTT packet identifier
  * @param count - number of members in the topicFilters array
  * @param topicFilters - array of topic filter names
  * @return the length of the serialized data.  <= 0 indicates error
  */
static int mqtt_serialize_unsubscribe(unsigned char* buf, int buflen, unsigned char dup, unsigned short packetid,
                int count, mqtt_string topicFilters[])
{
    unsigned char *ptr = buf;
    mqtt_header header = {0};
    int rem_len = 0;
    int rc = -1;
    int i = 0;

    FUNC_ENTRY;
    if (mqtt_pkt_len(rem_len = mqtt_serialize_unsubscribeLength(count, topicFilters)) > buflen) {
        rc = MQTTPACKET_BUFFER_TOO_SHORT;
        goto exit;
    }

    header.byte = 0;
    header.bits.type = UNSUBSCRIBE;
    header.bits.dup = dup;
    header.bits.qos = 1;
    writeChar(&ptr, header.byte); /* write header */

    ptr += mqtt_pkt_encode(ptr, rem_len); /* write remaining length */;

    writeInt(&ptr, packetid);

    for (i = 0; i < count; ++i)
        writemqtt_string(&ptr, topicFilters[i]);

    rc = ptr - buf;
exit:
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
  * Determines the length of the MQTT subscribe packet that would be produced using the supplied parameters
  * @param count the number of topic filter strings in topicFilters
  * @param topicFilters the array of topic filter strings to be used in the publish
  * @return the length of buffer needed to contain the serialized version of the packet
  */
int mqtt_serialize_subscribeLength(int count, mqtt_string topicFilters[])
{
    int i;
    int len = 2; /* packetid */

    for (i = 0; i < count; ++i)
        len += 2 + mqtt_strlen(topicFilters[i]) + 1; /* length + topic + req_qos */
    return len;
}

/**
  * Serializes the supplied subscribe data into the supplied buffer, ready for sending
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied bufferr
  * @param dup integer - the MQTT dup flag
  * @param packetid integer - the MQTT packet identifier
  * @param count - number of members in the topicFilters and reqQos arrays
  * @param topicFilters - array of topic filter names
  * @param requestedQoSs - array of requested QoS
  * @return the length of the serialized data.  <= 0 indicates error
  */
static int mqtt_serialize_subscribe(unsigned char* buf, int buflen, unsigned char dup, unsigned short packetid, int count,
                mqtt_string topicFilters[], int requestedQoSs[])
{
    unsigned char *ptr = buf;
    mqtt_header header = {0};
    int rem_len = 0;
    int rc = 0;
    int i = 0;

    FUNC_ENTRY;
    if (mqtt_pkt_len(rem_len = mqtt_serialize_subscribeLength(count, topicFilters)) > buflen) {
        rc = MQTTPACKET_BUFFER_TOO_SHORT;
        goto exit;
    }

    header.byte = 0;
    header.bits.type = SUBSCRIBE;
    header.bits.dup = dup;
    header.bits.qos = 1;
    writeChar(&ptr, header.byte); /* write header */

    ptr += mqtt_pkt_encode(ptr, rem_len); /* write remaining length */;

    writeInt(&ptr, packetid);

    for (i = 0; i < count; ++i) {
        writemqtt_string(&ptr, topicFilters[i]);
        writeChar(&ptr, requestedQoSs[i]);
    }

    rc = ptr - buf;
exit:
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
  * Deserializes the supplied (wire) buffer into suback data
  * @param packetid returned integer - the MQTT packet identifier
  * @param maxcount - the maximum number of members allowed in the grantedQoSs array
  * @param count returned integer - number of members in the grantedQoSs array
  * @param grantedQoSs returned array of integers - the granted qualities of service
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
static int mqtt_deserialize_suback(unsigned short* packetid, int maxcount, int* count, int grantedQoSs[], unsigned char* buf, int buflen)
{
    mqtt_header header = {0};
    unsigned char* curdata = buf;
    unsigned char* enddata = NULL;
    int rc = 0;
    int mylen;

    FUNC_ENTRY;
    header.byte = readChar(&curdata);
    if (header.bits.type != SUBACK)
        goto exit;

    curdata += (rc = mqtt_pkt_decodeBuf(curdata, &mylen)); /* read remaining length */
    enddata = curdata + mylen;
    if (enddata - curdata < 2)
        goto exit;

    *packetid = readInt(&curdata);

    *count = 0;
    while (curdata < enddata) {
        if (*count > maxcount) {
            rc = -1;
            goto exit;
        }
        grantedQoSs[(*count)++] = readChar(&curdata);
    }

    rc = 1;
exit:
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
  * Deserializes the supplied (wire) buffer into subscribe data
  * @param dup integer returned - the MQTT dup flag
  * @param packetid integer returned - the MQTT packet identifier
  * @param maxcount - the maximum number of members allowed in the topicFilters and requestedQoSs arrays
  * @param count - number of members in the topicFilters and requestedQoSs arrays
  * @param topicFilters - array of topic filter names
  * @param requestedQoSs - array of requested QoS
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return the length of the serialized data.  <= 0 indicates error
  */
static int mqtt_deserialize_subscribe(unsigned char* dup, unsigned short* packetid, int maxcount, int* count, mqtt_string topicFilters[],
                int requestedQoSs[], unsigned char* buf, int buflen)
{
    mqtt_header header = {0};
    unsigned char* curdata = buf;
    unsigned char* enddata = NULL;
    int rc = -1;
    int mylen = 0;

    FUNC_ENTRY;
    header.byte = readChar(&curdata);
    if (header.bits.type != SUBSCRIBE)
            goto exit;
    *dup = header.bits.dup;

    curdata += (rc = mqtt_pkt_decodeBuf(curdata, &mylen)); /* read remaining length */
    enddata = curdata + mylen;

    *packetid = readInt(&curdata);

    *count = 0;
    while (curdata < enddata) {
        if (!readMQTTLenString(&topicFilters[*count], &curdata, enddata))
            goto exit;
        if (curdata >= enddata) /* do we have enough data to read the req_qos version byte? */
            goto exit;
        requestedQoSs[*count] = readChar(&curdata);
        (*count)++;
    }

    rc = 1;
exit:
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
  * Serializes the supplied suback data into the supplied buffer, ready for sending
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param packetid integer - the MQTT packet identifier
  * @param count - number of members in the grantedQoSs array
  * @param grantedQoSs - array of granted QoS
  * @return the length of the serialized data.  <= 0 indicates error
  */
int mqtt_serialize_suback(unsigned char* buf, int buflen, unsigned short packetid, int count, int* grantedQoSs)
{
    mqtt_header header = {0};
    int rc = -1;
    unsigned char *ptr = buf;
    int i;

    FUNC_ENTRY;
    if (buflen < 2 + count) {
        rc = MQTTPACKET_BUFFER_TOO_SHORT;
        goto exit;
    }
    header.byte = 0;
    header.bits.type = SUBACK;
    writeChar(&ptr, header.byte); /* write header */

    ptr += mqtt_pkt_encode(ptr, 2 + count); /* write remaining length */

    writeInt(&ptr, packetid);

    for (i = 0; i < count; ++i)
        writeChar(&ptr, grantedQoSs[i]);

    rc = ptr - buf;
exit:
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
  * Deserializes the supplied (wire) buffer into publish data
  * @param dup returned integer - the MQTT dup flag
  * @param qos returned integer - the MQTT QoS value
  * @param retained returned integer - the MQTT retained flag
  * @param packetid returned integer - the MQTT packet identifier
  * @param topicName returned mqtt_string - the MQTT topic in the publish
  * @param payload returned byte buffer - the MQTT publish payload
  * @param payloadlen returned integer - the length of the MQTT payload
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success
  */
static int mqtt_deserialize_publish(unsigned char* dup, int* qos, unsigned char* retained, unsigned short* packetid, mqtt_string* topicName,
                unsigned char** payload, int* payloadlen, unsigned char* buf, int buflen)
{
    mqtt_header header = {0};
    unsigned char* curdata = buf;
    unsigned char* enddata = NULL;
    int rc = 0;
    int mylen = 0;

    FUNC_ENTRY;
    header.byte = readChar(&curdata);
    if (header.bits.type != PUBLISH)
            goto exit;
    *dup = header.bits.dup;
    *qos = header.bits.qos;
    *retained = header.bits.retain;

    curdata += (rc = mqtt_pkt_decodeBuf(curdata, &mylen)); /* read remaining length */
    enddata = curdata + mylen;

    if (!readMQTTLenString(topicName, &curdata, enddata) ||
         enddata - curdata < 0) /* do we have enough data to read the protocol version byte? */
        goto exit;

    if (*qos > 0)
        *packetid = readInt(&curdata);

    *payloadlen = enddata - curdata;
    *payload = curdata;
    rc = 1;
exit:
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
  * Deserializes the supplied (wire) buffer into an ack
  * @param packettype returned integer - the MQTT packet type
  * @param dup returned integer - the MQTT dup flag
  * @param packetid returned integer - the MQTT packet identifier
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
static int mqtt_deserialize_ack(unsigned char* packettype, unsigned char* dup, unsigned short* packetid, unsigned char* buf, int buflen)
{
    mqtt_header header = {0};
    unsigned char* curdata = buf;
    unsigned char* enddata = NULL;
    int rc = 0;
    int mylen;

    FUNC_ENTRY;
    header.byte = readChar(&curdata);
    *dup = header.bits.dup;
    *packettype = header.bits.type;

    curdata += (rc = mqtt_pkt_decodeBuf(curdata, &mylen)); /* read remaining length */
    enddata = curdata + mylen;

    if (enddata - curdata < 2)
        goto exit;
    *packetid = readInt(&curdata);

    rc = 1;
exit:
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
  * Determines the length of the MQTT publish packet that would be produced using the supplied parameters
  * @param qos the MQTT QoS of the publish (packetid is omitted for QoS 0)
  * @param topicName the topic name to be used in the publish  
  * @param payloadlen the length of the payload to be sent
  * @return the length of buffer needed to contain the serialized version of the packet
  */
static int mqtt_serialize_publishLength(int qos, mqtt_string topicName, int payloadlen)
{
    int len = 0;

    len += 2 + mqtt_strlen(topicName) + payloadlen;
    if (qos > 0)
        len += 2; /* packetid */
    return len;
}

/**
  * Serializes the supplied publish data into the supplied buffer, ready for sending
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param dup integer - the MQTT dup flag
  * @param qos integer - the MQTT QoS value
  * @param retained integer - the MQTT retained flag
  * @param packetid integer - the MQTT packet identifier
  * @param topicName mqtt_string - the MQTT topic in the publish
  * @param payload byte buffer - the MQTT publish payload
  * @param payloadlen integer - the length of the MQTT payload
  * @return the length of the serialized data.  <= 0 indicates error
  */
static int mqtt_serialize_publish(unsigned char* buf, int buflen, unsigned char dup, int qos, unsigned char retained, unsigned short packetid,
                mqtt_string topicName, unsigned char* payload, int payloadlen)
{
    unsigned char *ptr = buf;
    mqtt_header header = {0};
    int rem_len = 0;
    int rc = 0;

    FUNC_ENTRY;
    if (mqtt_pkt_len(rem_len = mqtt_serialize_publishLength(qos, topicName, payloadlen)) > buflen) {
        rc = MQTTPACKET_BUFFER_TOO_SHORT;
        goto exit;
    }

    header.bits.type = PUBLISH;
    header.bits.dup = dup;
    header.bits.qos = qos;
    header.bits.retain = retained;
    writeChar(&ptr, header.byte); /* write header */

    ptr += mqtt_pkt_encode(ptr, rem_len); /* write remaining length */;

    writemqtt_string(&ptr, topicName);

    if (qos > 0)
        writeInt(&ptr, packetid);

    memcpy(ptr, payload, payloadlen);
    ptr += payloadlen;
    rc = ptr - buf;

exit:
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
  * Serializes the ack packet into the supplied buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param type the MQTT packet type
  * @param dup the MQTT dup flag
  * @param packetid the MQTT packet identifier
  * @return serialized length, or error if 0
  */
static int mqtt_serialize_ack(unsigned char* buf, int buflen, unsigned char packettype, unsigned char dup, unsigned short packetid)
{
    mqtt_header header = {0};
    int rc = 0;
    unsigned char *ptr = buf;

    FUNC_ENTRY;
    if (buflen < 4) {
        rc = MQTTPACKET_BUFFER_TOO_SHORT;
        goto exit;
    }
    header.bits.type = packettype;
    header.bits.dup = dup;
    header.bits.qos = (packettype == PUBREL) ? 1 : 0;
    writeChar(&ptr, header.byte); /* write header */

    ptr += mqtt_pkt_encode(ptr, 2); /* write remaining length */
    writeInt(&ptr, packetid);
    rc = ptr - buf;
exit:
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
  * Serializes a puback packet into the supplied buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param packetid integer - the MQTT packet identifier
  * @return serialized length, or error if 0
  */
int mqtt_serialize_puback(unsigned char* buf, int buflen, unsigned short packetid)
{
    return mqtt_serialize_ack(buf, buflen, PUBACK, 0, packetid);
}

/**
  * Serializes a pubrel packet into the supplied buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param dup integer - the MQTT dup flag
  * @param packetid integer - the MQTT packet identifier
  * @return serialized length, or error if 0
  */
int mqtt_serialize_pubrel(unsigned char* buf, int buflen, unsigned char dup, unsigned short packetid)
{
    return mqtt_serialize_ack(buf, buflen, PUBREL, dup, packetid);
}

/**
  * Serializes a pubrel packet into the supplied buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param packetid integer - the MQTT packet identifier
  * @return serialized length, or error if 0
  */
int mqtt_serialize_pubcomp(unsigned char* buf, int buflen, unsigned short packetid)
{
    return mqtt_serialize_ack(buf, buflen, PUBCOMP, 0, packetid);
}

static void NewMessageData(MessageData* md, mqtt_string* aTopicName, mqtt_msg* aMessage) {
    md->topicName = aTopicName;
    md->message = aMessage;
}

static int getNextPacketId(mqtt_client *c)
{
    return c->next_packetid = (c->next_packetid == MAX_PACKET_ID) ? 1 : c->next_packetid + 1;
}


static int linux_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    struct timeval interval = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 0)) {
        interval.tv_sec = 0;
        interval.tv_usec = 100;
    }

    setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval, sizeof(struct timeval));

    int bytes = 0;
    while (bytes < len) {
        int rc = recv(n->my_socket, &buffer[bytes], (size_t)(len - bytes), 0);
        if (rc == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
                bytes = -1;
            break;
        } else if (rc == 0) {
            bytes = 0;
            break;
        } else
            bytes += rc;
    }
    return bytes;
}

static int linux_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    struct timeval tv;

    tv.tv_sec = 0;  /* 30 Secs Timeout */
    tv.tv_usec = timeout_ms * 1000;  // Not init'ing this can cause strange errors

    setsockopt(n->my_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,sizeof(struct timeval));
    int rc = write(n->my_socket, buffer, len);
    return rc;
}

static int sendPacket(mqtt_client* c, int length, Timer* timer)
{
    int rc = MQTT_FAILURE;
    int sent = 0;

    while (sent < length && !TimerIsExpired(timer)) {
#if SOCK_API
        rc = c->ops->write(c, &c->buf[sent], length);
#else
        rc = c->ipstack.write(&c->ipstack, &c->buf[sent], length, TimerLeftMS(timer));
#endif
        if (rc < 0) { // there was an error writing the data
            printf("%s:%d rc = %d\n", __func__, __LINE__, rc);
            break;
        }
        sent += rc;
    }
    if (sent == length) {
        TimerCountdown(&c->last_sent, c->keepAliveInterval); // record the fact that we have successfully sent the packet
        rc = MQTT_SUCCESS;
    } else {
        printf("sent = %d, length = %d\n", sent, length);
        rc = MQTT_FAILURE;
    }
    return rc;
}
static int readPacket(mqtt_client* c, Timer* timer)
{
    mqtt_header header = {0};
    int len = 0;
    int rem_len = 0;
    unsigned char i;
    int multiplier = 1;
    int tmp_len = 0;

    /* 1. read the header byte.  This has the packet type in it */
#if SOCK_API
    int rc = c->ops->read(c, c->readbuf, 1);
#else
    int rc = c->ipstack.read(&c->ipstack, c->readbuf, 1, TimerLeftMS(timer));
#endif
    if (rc != 1) {
        goto exit;
    }

    /* 2. read the remaining length.  This is variable in itself */
    do {
        rc = MQTTPACKET_READ_ERROR;
        if (++tmp_len > MAX_NO_OF_REMAINING_LENGTH_BYTES) {
            rc = MQTTPACKET_READ_ERROR; /* bad data */
            break;
        }
#if SOCK_API
        rc = c->ops->read(c, &i, 1);
#else
        rc = c->ipstack.read(&c->ipstack, &i, 1, TimerLeftMS(timer));
#endif
        if (rc != 1) {
            printf("read failed rc = %d\n", rc);
            break;
        }
        rem_len += (i & 127) * multiplier;
        multiplier *= 128;
    } while ((i & 128) != 0);

    len = 1;
    len += mqtt_pkt_encode(c->readbuf + 1, rem_len); /* put the original remaining length back into the buffer */

    if (rem_len > (c->readbuf_size - len)) {
        rc = MQTT_BUFFER_OVERFLOW;
        goto exit;
    }

    /* 3. read the rest of the buffer using a callback to supply the rest of the data */
#if SOCK_API
    if (rem_len > 0 && (rc = c->ops->read(c, c->readbuf + len, rem_len) != rem_len)) {
#else
    if (rem_len > 0 && (rc = c->ipstack.read(&c->ipstack, c->readbuf + len, rem_len, TimerLeftMS(timer)) != rem_len)) {
#endif
        rc = 0;
        goto exit;
    }

    header.byte = c->readbuf[0];
    rc = header.bits.type;
    if (c->keepAliveInterval > 0)
        TimerCountdown(&c->last_received, c->keepAliveInterval); // record the fact that we have successfully received a packet
exit:
    return rc;
}

//=============================================================================

int mqtt_client_init(mqtt_client* c, const char *host, int port)
{
    int i;
    int command_timeout_ms = 1000;
    int type = SOCK_STREAM;
    sa_family_t family = AF_INET;
    size_t sendlen = 1024;
    size_t readlen = 1024;

    c->ops = &mqttc_socket_ops;
    c->priv = c->ops->init(host, port);

    c->host = strdup(host);
    c->port = port;

    c->fd = socket(family, type, 0);
    if (c->fd == -1) {
        printf("%s:%d socket failed!\n", __func__, __LINE__);
        return -1;
    }

    c->ipstack.my_socket = c->fd;
    c->ipstack.read = linux_read;
    c->ipstack.write = linux_write;

    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
        c->messageHandlers[i].topicFilter = 0;
    c->command_timeout_ms = command_timeout_ms;
    c->buf = calloc(1, sendlen);
    c->buf_size = sendlen;
    c->readbuf = calloc(1, readlen);
    c->readbuf_size = readlen;
    c->isconnected = 0;
    c->cleansession = 0;
    c->ping_outstanding = 0;
    c->defaultMessageHandler = NULL;
    c->next_packetid = 1;
    TimerInit(&c->last_sent);
    TimerInit(&c->last_received);
#if defined(MQTT_TASK)
    MutexInit(&c->mutex);
#endif
    return 0;
}

// assume topic filter and name is in correct format
// # can only be at end
// + and # can only be next to separator
static char isTopicMatched(char* topicFilter, mqtt_string* topicName)
{
    char* curf = topicFilter;
    char* curn = topicName->lenstring.data;
    char* curn_end = curn + topicName->lenstring.len;

    while (*curf && curn < curn_end)
    {
        if (*curn == '/' && *curf != '/')
            break;
        if (*curf != '+' && *curf != '#' && *curf != *curn)
            break;
        if (*curf == '+')
        {   // skip until we meet the next separator, or end of string
            char* nextpos = curn + 1;
            while (nextpos < curn_end && *nextpos != '/')
                nextpos = ++curn + 1;
        }
        else if (*curf == '#')
            curn = curn_end - 1;    // skip until end of string
        curf++;
        curn++;
    };

    return (curn == curn_end) && (*curf == '\0');
}

static int deliverMessage(mqtt_client* c, mqtt_string* topicName, mqtt_msg* message)
{
    int i;
    int rc = MQTT_FAILURE;

    // we have to find the right message handler - indexed by topic
    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
    {
        if (c->messageHandlers[i].topicFilter != 0 && (mqtt_pkt_equals(topicName, (char*)c->messageHandlers[i].topicFilter) ||
                isTopicMatched((char*)c->messageHandlers[i].topicFilter, topicName)))
        {
            if (c->messageHandlers[i].fp != NULL)
            {
                MessageData md;
                NewMessageData(&md, topicName, message);
                c->messageHandlers[i].fp(&md);
                rc = MQTT_SUCCESS;
            }
        }
    }

    if (rc == MQTT_FAILURE && c->defaultMessageHandler != NULL)
    {
        MessageData md;
        NewMessageData(&md, topicName, message);
        c->defaultMessageHandler(&md);
        rc = MQTT_SUCCESS;
    }

    return rc;
}

static void MQTTCleanSession(mqtt_client* c)
{
    int i = 0;

    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
        c->messageHandlers[i].topicFilter = NULL;
}

static void MQTTCloseSession(mqtt_client* c)
{
    c->ping_outstanding = 0;
    c->isconnected = 0;
    if (c->cleansession)
        MQTTCleanSession(c);
}

static int mqtt_set_msg_handler(mqtt_client* c, const char* topicFilter, messageHandler messageHandler)
{
    int rc = MQTT_FAILURE;
    int i = -1;

    /* first check for an existing matching slot */
    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
    {
        if (c->messageHandlers[i].topicFilter != NULL && strcmp(c->messageHandlers[i].topicFilter, topicFilter) == 0)
        {
            if (messageHandler == NULL) /* remove existing */
            {
                c->messageHandlers[i].topicFilter = NULL;
                c->messageHandlers[i].fp = NULL;
            }
            rc = MQTT_SUCCESS; /* return i when adding new subscription */
            break;
        }
    }
    /* if no existing, look for empty slot (unless we are removing) */
    if (messageHandler != NULL) {
        if (rc == MQTT_FAILURE)
        {
            for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i)
            {
                if (c->messageHandlers[i].topicFilter == NULL)
                {
                    rc = MQTT_SUCCESS;
                    break;
                }
            }
        }
        if (i < MAX_MESSAGE_HANDLERS)
        {
            c->messageHandlers[i].topicFilter = topicFilter;
            c->messageHandlers[i].fp = messageHandler;
        }
    }
    return rc;
}

/**
  * Deserializes the supplied (wire) buffer into unsuback data
  * @param packetid returned integer - the MQTT packet identifier
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param buflen the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
static int mqtt_deserialize_unsuback(unsigned short* packetid, unsigned char* buf, int buflen)
{
    unsigned char type = 0;
    unsigned char dup = 0;
    int rc = 0;

    FUNC_ENTRY;
    rc = mqtt_deserialize_ack(&type, &dup, packetid, buf, buflen);
    if (type == UNSUBACK)
        rc = 1;
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
  * Determines the length of the MQTT connect packet that would be produced using the supplied connect options.
  * @param options the options to be used to build the connect packet
  * @return the length of buffer needed to contain the serialized version of the packet
  */
static int mqtt_serialize_connectLength(mqtt_pkt_conn_data* options)
{
    int len = 0;

    FUNC_ENTRY;

    if (options->mqtt_version == 3)
        len = 12; /* variable depending on MQTT or MQIsdp */
    else if (options->mqtt_version == 4)
        len = 10;

    len += mqtt_strlen(options->clientID)+2;
    if (options->willFlag)
        len += mqtt_strlen(options->will.topicName)+2 + mqtt_strlen(options->will.message)+2;
    if (options->username.cstring || options->username.lenstring.data)
        len += mqtt_strlen(options->username)+2;
    if (options->password.cstring || options->password.lenstring.data)
        len += mqtt_strlen(options->password)+2;

    FUNC_EXIT_RC(len);
    return len;
}

/**
  * Serializes the connect options into the buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param len the length in bytes of the supplied buffer
  * @param options the options to be used to build the connect packet
  * @return serialized length, or error if 0
  */
static int mqtt_serialize_connect(unsigned char* buf, int buflen, mqtt_pkt_conn_data* options)
{
    unsigned char *ptr = buf;
    mqtt_header header = {0};
    mqtt_conn_flags flags = {0};
    int len = 0;
    int rc = -1;

    FUNC_ENTRY;
    if (mqtt_pkt_len(len = mqtt_serialize_connectLength(options)) > buflen) {
        rc = MQTTPACKET_BUFFER_TOO_SHORT;
        goto exit;
    }

    header.byte = 0;
    header.bits.type = CONNECT;
    writeChar(&ptr, header.byte); /* write header */

    ptr += mqtt_pkt_encode(ptr, len); /* write remaining length */

    if (options->mqtt_version == 4) {
        writeCString(&ptr, "MQTT");
        writeChar(&ptr, (char) 4);
    } else {
        writeCString(&ptr, "MQIsdp");
        writeChar(&ptr, (char) 3);
    }

    flags.all = 0;
    flags.bits.cleansession = options->cleansession;
    flags.bits.will = (options->willFlag) ? 1 : 0;
    if (flags.bits.will) {
        flags.bits.willQoS = options->will.qos;
        flags.bits.willRetain = options->will.retained;
    }

    if (options->username.cstring || options->username.lenstring.data)
            flags.bits.username = 1;
    if (options->password.cstring || options->password.lenstring.data)
            flags.bits.password = 1;

    writeChar(&ptr, flags.all);
    writeInt(&ptr, options->keepAliveInterval);
    writemqtt_string(&ptr, options->clientID);
    if (options->willFlag) {
        writemqtt_string(&ptr, options->will.topicName);
        writemqtt_string(&ptr, options->will.message);
    }
    if (flags.bits.username)
        writemqtt_string(&ptr, options->username);
    if (flags.bits.password)
        writemqtt_string(&ptr, options->password);

    rc = ptr - buf;

exit: FUNC_EXIT_RC(rc);
      return rc;
}

/**
  * Deserializes the supplied (wire) buffer into connack data - return code
  * @param sessionPresent the session present flag returned (only for MQTT 3.1.1)
  * @param connack_rc returned integer value of the connack return code
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param len the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
static int mqtt_deserialize_connack(unsigned char* sessionPresent, unsigned char* connack_rc, unsigned char* buf, int buflen)
{
    mqtt_header header = {0};
    unsigned char* curdata = buf;
    unsigned char* enddata = NULL;
    int rc = 0;
    int mylen;
    mqtt_connack_flags flags = {0};

    FUNC_ENTRY;
    header.byte = readChar(&curdata);
    if (header.bits.type != CONNACK)
        goto exit;

    curdata += (rc = mqtt_pkt_decodeBuf(curdata, &mylen)); /* read remaining length */
    enddata = curdata + mylen;
    if (enddata - curdata < 2)
        goto exit;

    flags.all = readChar(&curdata);
    *sessionPresent = flags.bits.sessionpresent;
    *connack_rc = readChar(&curdata);

    rc = 1;
exit:
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
  * Serializes a 0-length packet into the supplied buffer, ready for writing to a socket
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer, to avoid overruns
  * @param packettype the message type
  * @return serialized length, or error if 0
  */
static int mqtt_serialize_zero(unsigned char* buf, int buflen, unsigned char packettype)
{
    mqtt_header header = {0};
    int rc = -1;
    unsigned char *ptr = buf;

    FUNC_ENTRY;
    if (buflen < 2) {
        rc = MQTTPACKET_BUFFER_TOO_SHORT;
        goto exit;
    }
    header.byte = 0;
    header.bits.type = packettype;
    writeChar(&ptr, header.byte); /* write header */

    ptr += mqtt_pkt_encode(ptr, 0); /* write remaining length */
    rc = ptr - buf;
exit:
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
  * Serializes a disconnect packet into the supplied buffer, ready for writing to a socket
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer, to avoid overruns
  * @return serialized length, or error if 0
  */
static int mqtt_serialize_disconnect(unsigned char* buf, int buflen)
{
    return mqtt_serialize_zero(buf, buflen, DISCONNECT);
}

/**
  * Serializes a disconnect packet into the supplied buffer, ready for writing to a socket
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer, to avoid overruns
  * @return serialized length, or error if 0
  */
static int mqtt_serialize_pingreq(unsigned char* buf, int buflen)
{
    return mqtt_serialize_zero(buf, buflen, PINGREQ);
}

/**
  * Validates MQTT protocol name and version combinations
  * @param protocol the MQTT protocol name as an mqtt_string
  * @param version the MQTT protocol version number, as in the connect packet
  * @return correct MQTT combination?  1 is true, 0 is false
  */
int mqtt_pkt_checkVersion(mqtt_string* protocol, int version)
{
    int rc = 0;

    if (version == 3 && memcmp(protocol->lenstring.data, "MQIsdp",
                            min(6, protocol->lenstring.len)) == 0)
        rc = 1;
    else if (version == 4 && memcmp(protocol->lenstring.data, "MQTT",
                            min(4, protocol->lenstring.len)) == 0)
        rc = 1;
    return rc;
}

/**
  * Deserializes the supplied (wire) buffer into connect data structure
  * @param data the connect data structure to be filled out
  * @param buf the raw buffer data, of the correct length determined by the remaining length field
  * @param len the length in bytes of the data in the supplied buffer
  * @return error code.  1 is success, 0 is failure
  */
static int mqtt_deserialize_connect(mqtt_pkt_conn_data* data, unsigned char* buf, int len)
{
    mqtt_header header = {0};
    mqtt_conn_flags flags = {0};
    unsigned char* curdata = buf;
    unsigned char* enddata = &buf[len];
    int rc = 0;
    mqtt_string Protocol;
    int version;
    int mylen = 0;

    FUNC_ENTRY;
    header.byte = readChar(&curdata);
    if (header.bits.type != CONNECT)
        goto exit;

    curdata += mqtt_pkt_decodeBuf(curdata, &mylen); /* read remaining length */

    if (!readMQTTLenString(&Protocol, &curdata, enddata) ||
         enddata - curdata < 0) /* do we have enough data to read the protocol version byte? */
        goto exit;

    version = (int)readChar(&curdata); /* Protocol version */
    /* If we don't recognize the protocol version, we don't parse the connect packet on the
     * basis that we don't know what the format will be.
     */
    if (mqtt_pkt_checkVersion(&Protocol, version)) {
        flags.all = readChar(&curdata);
        data->cleansession = flags.bits.cleansession;
        data->keepAliveInterval = readInt(&curdata);
        if (!readMQTTLenString(&data->clientID, &curdata, enddata))
            goto exit;
        data->willFlag = flags.bits.will;
        if (flags.bits.will) {
            data->will.qos = flags.bits.willQoS;
            data->will.retained = flags.bits.willRetain;
            if (!readMQTTLenString(&data->will.topicName, &curdata, enddata) ||
                !readMQTTLenString(&data->will.message, &curdata, enddata))
                goto exit;
        }
        if (flags.bits.username) {
            if (enddata - curdata < 3 || !readMQTTLenString(&data->username, &curdata, enddata))
                goto exit; /* username flag set, but no username supplied - invalid */
            if (flags.bits.password &&
                (enddata - curdata < 3 || !readMQTTLenString(&data->password, &curdata, enddata)))
                goto exit; /* password flag set, but no password supplied - invalid */
        }
        else if (flags.bits.password)
            goto exit; /* password flag set without username - invalid */
        rc = 1;
    }
exit:
    FUNC_EXIT_RC(rc);
    return rc;
}

/**
  * Serializes the connack packet into the supplied buffer.
  * @param buf the buffer into which the packet will be serialized
  * @param buflen the length in bytes of the supplied buffer
  * @param connack_rc the integer connack return code to be used 
  * @param sessionPresent the MQTT 3.1.1 sessionPresent flag
  * @return serialized length, or error if 0
  */
int mqtt_serialize_connack(unsigned char* buf, int buflen, unsigned char connack_rc, unsigned char sessionPresent)
{
    mqtt_header header = {0};
    int rc = 0;
    unsigned char *ptr = buf;
    mqtt_connack_flags flags = {0};

    FUNC_ENTRY;
    if (buflen < 2) {
        rc = MQTTPACKET_BUFFER_TOO_SHORT;
        goto exit;
    }
    header.byte = 0;
    header.bits.type = CONNACK;
    writeChar(&ptr, header.byte); /* write header */

    ptr += mqtt_pkt_encode(ptr, 2); /* write remaining length */

    flags.all = 0;
    flags.bits.sessionpresent = sessionPresent;
    writeChar(&ptr, flags.all); 
    writeChar(&ptr, connack_rc);

    rc = ptr - buf;
exit:
    FUNC_EXIT_RC(rc);
    return rc;
}

#if defined(MQTT_CLIENT)
char* mqtt_fmt_toClientString(char* strbuf, int strbuflen, unsigned char* buf, int buflen)
{
    int index = 0;
    int rem_length = 0;
    mqtt_header header = {0};
    int strindex = 0;

    header.byte = buf[index++];
    index += mqtt_pkt_decodeBuf(&buf[index], &rem_length);

    switch (header.bits.type) {
    case CONNACK: {
        unsigned char sessionPresent, connack_rc;
        if (mqtt_deserialize_connack(&sessionPresent, &connack_rc, buf, buflen) == 1)
                strindex = mqtt_stringFormat_connack(strbuf, strbuflen, connack_rc, sessionPresent);
    } break;
    case PUBLISH: {
        unsigned char dup, retained, *payload;
        unsigned short packetid;
        int qos, payloadlen;
        mqtt_string topicName = mqtt_string_initializer;
        if (mqtt_deserialize_publish(&dup, &qos, &retained, &packetid, &topicName,
                                &payload, &payloadlen, buf, buflen) == 1)
                strindex = mqtt_stringFormat_publish(strbuf, strbuflen, dup, qos, retained, packetid,
                                topicName, payload, payloadlen);
    } break;
    case PUBACK:
    case PUBREC:
    case PUBREL:
    case PUBCOMP: {
        unsigned char packettype, dup;
        unsigned short packetid;
        if (mqtt_deserialize_ack(&packettype, &dup, &packetid, buf, buflen) == 1)
                strindex = mqtt_stringFormat_ack(strbuf, strbuflen, packettype, dup, packetid);
    } break;
    case SUBACK: {
        unsigned short packetid;
        int maxcount = 1, count = 0;
        int grantedQoSs[1];
        if (mqtt_deserialize_suback(&packetid, maxcount, &count, grantedQoSs, buf, buflen) == 1)
                strindex = mqtt_stringFormat_suback(strbuf, strbuflen, packetid, count, grantedQoSs);
    } break;
    case UNSUBACK: {
        unsigned short packetid;
        if (mqtt_deserialize_unsuback(&packetid, buf, buflen) == 1)
                strindex = mqtt_stringFormat_ack(strbuf, strbuflen, UNSUBACK, 0, packetid);
    } break;
    case PINGREQ:
    case PINGRESP:
    case DISCONNECT:
                   strindex = snprintf(strbuf, strbuflen, "%s", mqtt_pkt_names[header.bits.type]);
        break;
    }
    printf("%s:%d strindex = %d\n", __func__, __LINE__, strindex);
    return strbuf;
}
#endif

#if defined(MQTT_SERVER)
char* mqtt_fmt_toServerString(char* strbuf, int strbuflen, unsigned char* buf, int buflen)
{
    int index = 0;
    int rem_length = 0;
    mqtt_header header = {0};
    int strindex = 0;

    header.byte = buf[index++];
    index += mqtt_pkt_decodeBuf(&buf[index], &rem_length);

    switch (header.bits.type) {
    case CONNECT: {
        mqtt_pkt_conn_data data;
        int rc;
        if ((rc = mqtt_deserialize_connect(&data, buf, buflen)) == 1)
            strindex = mqtt_stringFormat_connect(strbuf, strbuflen, &data);
    } break;
    case PUBLISH: {
        unsigned char dup, retained, *payload;
        unsigned short packetid;
        int qos, payloadlen;
        mqtt_string topicName = mqtt_string_initializer;
        if (mqtt_deserialize_publish(&dup, &qos, &retained, &packetid, &topicName,
            &payload, &payloadlen, buf, buflen) == 1)
                strindex = mqtt_stringFormat_publish(strbuf, strbuflen, dup, qos, retained, packetid,
                                topicName, payload, payloadlen);
    } break;
    case PUBACK:
    case PUBREC:
    case PUBREL:
    case PUBCOMP: {
        unsigned char packettype, dup;
        unsigned short packetid;
        if (mqtt_deserialize_ack(&packettype, &dup, &packetid, buf, buflen) == 1)
                strindex = mqtt_stringFormat_ack(strbuf, strbuflen, packettype, dup, packetid);
        } break;
    case SUBSCRIBE: {
        unsigned char dup;
        unsigned short packetid;
        int maxcount = 1, count = 0;
        mqtt_string topicFilters[1];
        int requestedQoSs[1];
        if (mqtt_deserialize_subscribe(&dup, &packetid, maxcount, &count,
                topicFilters, requestedQoSs, buf, buflen) == 1)
            strindex = mqtt_stringFormat_subscribe(strbuf, strbuflen, dup, packetid, count, topicFilters, requestedQoSs);;
    } break;
    case UNSUBSCRIBE: {
        unsigned char dup;
        unsigned short packetid;
        int maxcount = 1, count = 0;
        mqtt_string topicFilters[1];
        if (mqtt_deserialize_unsubscribe(&dup, &packetid, maxcount, &count, topicFilters, buf, buflen) == 1)
            strindex =  mqtt_stringFormat_unsubscribe(strbuf, strbuflen, dup, packetid, count, topicFilters);
    } break;
    case PINGREQ:
    case PINGRESP:
    case DISCONNECT:
        strindex = snprintf(strbuf, strbuflen, "%s", mqtt_pkt_names[header.bits.type]);
        break;
    }
    strbuf[strbuflen] = '\0';
    printf("%s:%d strindex = %d\n", __func__, __LINE__, strindex);
    return strbuf;
}
#endif

static int keepalive(mqtt_client* c)
{
    int rc = MQTT_SUCCESS;

    if (c->keepAliveInterval == 0)
        goto exit;

    if (TimerIsExpired(&c->last_sent) || TimerIsExpired(&c->last_received)) {
        if (c->ping_outstanding) {
            rc = MQTT_FAILURE; /* PINGRESP not received in keepalive interval */
        } else {
            Timer timer;
            TimerInit(&timer);
            TimerCountdownMS(&timer, 1000);
            int len = mqtt_serialize_pingreq(c->buf, c->buf_size);
            if (len > 0 && (rc = sendPacket(c, len, &timer)) == MQTT_SUCCESS) // send the ping packet
                c->ping_outstanding = 1;
        }
    }
exit:
    return rc;
}

static int cycle(mqtt_client* c, Timer* timer)
{
    int len = 0,
        rc = MQTT_SUCCESS;

    int packet_type = readPacket(c, timer);     /* read the socket, see what work is due */

    //printf("packet_type = %d\n", packet_type);
    switch (packet_type) {
        default:
            /* no more data to read, unrecoverable. Or read packet fails due to unexpected network error */
            rc = packet_type;
            goto exit;
        case 0: /* timed out reading packet */
            break;
        case CONNACK:
        case PUBACK:
        case SUBACK:
        case UNSUBACK:
            break;
        case PUBLISH: {
            mqtt_string topicName;
            mqtt_msg msg;
            int intQoS;
            msg.payloadlen = 0; /* this is a size_t, but deserialize publish sets this as int */
            if (mqtt_deserialize_publish(&msg.dup, &intQoS, &msg.retained, &msg.id, &topicName,
               (unsigned char**)&msg.payload, (int*)&msg.payloadlen, c->readbuf, c->readbuf_size) != 1) {
                printf("mqtt_deserialize_publish failed!\n");
                goto exit;
            }
            msg.qos = (enum mqtt_qos)intQoS;
            deliverMessage(c, &topicName, &msg);
            if (msg.qos != MQTT_QOS0) {
                if (msg.qos == MQTT_QOS1) {
                    len = mqtt_serialize_ack(c->buf, c->buf_size, PUBACK, 0, msg.id);
                } else if (msg.qos == MQTT_QOS2) {
                    len = mqtt_serialize_ack(c->buf, c->buf_size, PUBREC, 0, msg.id);
                }
                if (len <= 0) {
                    rc = MQTT_FAILURE;
                } else {
                    rc = sendPacket(c, len, timer);
                }
                if (rc == MQTT_FAILURE) {
                    printf("rc is MQTT_FAILURE\n");
                    goto exit; // there was a problem
                }
            }
            break;
        }
        case PUBREC:
        case PUBREL: {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (mqtt_deserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
                rc = MQTT_FAILURE;
            else if ((len = mqtt_serialize_ack(c->buf, c->buf_size,
                (packet_type == PUBREC) ? PUBREL : PUBCOMP, 0, mypacketid)) <= 0)
                rc = MQTT_FAILURE;
            else if ((rc = sendPacket(c, len, timer)) != MQTT_SUCCESS) // send the PUBREL packet
                rc = MQTT_FAILURE; // there was a problem
            if (rc == MQTT_FAILURE) {
                    printf("rc is MQTT_FAILURE\n");
                goto exit; // there was a problem
            }
            break;
        }
        case PUBCOMP:
            break;
        case PINGRESP:
            c->ping_outstanding = 0;
            break;
    }

    if (keepalive(c) != MQTT_SUCCESS) {
        //check only keepalive MQTT_FAILURE status so that previous MQTT_FAILURE status can be considered as FAULT
        printf("rc is MQTT_FAILURE\n");
        rc = MQTT_FAILURE;
    }

exit:
    if (rc == MQTT_SUCCESS)
        rc = packet_type;
    else if (c->isconnected)
        MQTTCloseSession(c);
    return rc;
}

static int waitfor(mqtt_client* c, enum mqtt_msg_types type, Timer* timer)
{
    int rc = MQTT_FAILURE;

    do {
        if (TimerIsExpired(timer))
            break; // we timed out
        rc = cycle(c, timer);
    } while (rc != type && rc >= 0);

    return rc;
}

int mqtt_connect(mqtt_client* c, mqtt_pkt_conn_data* options, mqtt_connack_data* data)
{
    Timer connect_timer;
    int rc = MQTT_FAILURE;
    mqtt_pkt_conn_data default_options = mqtt_pkt_conn_data_initializer;
    int len = 0;
    sa_family_t family = AF_INET;

#if defined(MQTT_TASK)
    MutexLock(&c->mutex);
#endif
    if (c->isconnected) /* don't send connect packet again if we are already connected */
        goto exit;

    struct sockaddr_in addr;
    struct addrinfo *result = NULL;
    struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};

    if ((rc = getaddrinfo(c->host, NULL, &hints, &result)) == 0) {
        struct addrinfo* res = result;

        /* prefer ip4 addresses */
        while (res) {
            if (res->ai_family == AF_INET) {
                result = res;
                break;
            }
            res = res->ai_next;
        }

        if (result->ai_family == AF_INET) {
            addr.sin_port = htons(c->port);
            addr.sin_family = family = AF_INET;
            addr.sin_addr = ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
        } else
            rc = -1;
        freeaddrinfo(result);
    }
    if (rc != 0) {
        printf("%s:%d get host addr info failed\n", __func__, __LINE__);
        return -1;
    }

    rc = connect(c->fd, (struct sockaddr*)&addr, sizeof(addr));
    if (rc == -1) {
        printf("%s:%d connect failed: %d\n", __func__, __LINE__, errno);
        return -1;
    }

    c->ops->connect(c);
    if (rc == -1) {
        printf("%s:%d connect failed: %d\n", __func__, __LINE__, errno);
        return -1;
    }

    TimerInit(&connect_timer);
    TimerCountdownMS(&connect_timer, c->command_timeout_ms);

    if (options == 0)
        options = &default_options; /* set default options if none were supplied */

    c->keepAliveInterval = options->keepAliveInterval;
    c->cleansession = options->cleansession;
    TimerCountdown(&c->last_received, c->keepAliveInterval);
    if ((len = mqtt_serialize_connect(c->buf, c->buf_size, options)) <= 0)
        goto exit;
    if ((rc = sendPacket(c, len, &connect_timer)) != MQTT_SUCCESS)  // send the connect packet
        goto exit; // there was a problem

    // this will be a blocking call, wait for the connack
    if (waitfor(c, CONNACK, &connect_timer) == CONNACK) {
        data->rc = 0;
        data->sessionPresent = 0;
        if (mqtt_deserialize_connack(&data->sessionPresent, &data->rc, c->readbuf, c->readbuf_size) == 1) {
            rc = data->rc;
        } else {
            rc = MQTT_FAILURE;
        }
    } else {
        rc = MQTT_FAILURE;
    }

exit:
    if (rc == MQTT_SUCCESS) {
        c->isconnected = 1;
        c->ping_outstanding = 0;
    }

#if defined(MQTT_TASK)
    MutexUnlock(&c->mutex);
#endif

    return rc;
}

int mqtt_publish(mqtt_client* c, const char* topicName, mqtt_msg* message)
{
    int rc = MQTT_FAILURE;
    Timer timer;
    mqtt_string topic = mqtt_string_initializer;
    topic.cstring = (char *)topicName;
    int len = 0;

#if defined(MQTT_TASK)
    MutexLock(&c->mutex);
#endif
    if (!c->isconnected) {
        printf("is not connected!\n");
        goto exit;
    }

    TimerInit(&timer);
    TimerCountdownMS(&timer, c->command_timeout_ms);

    if (message->qos == MQTT_QOS1 || message->qos == MQTT_QOS2)
        message->id = getNextPacketId(c);

    len = mqtt_serialize_publish(c->buf, c->buf_size, 0, message->qos, message->retained, message->id,
              topic, (unsigned char*)message->payload, message->payloadlen);
    if (len <= 0) {
        printf("mqtt_serialize_publish failed!\n");
        goto exit;
    }
    if ((rc = sendPacket(c, len, &timer)) != MQTT_SUCCESS) {// send the subscribe packet
        printf("sendPacket failed!\n");
        goto exit; // there was a problem
    }

    if (message->qos == MQTT_QOS1)
    {
        if (waitfor(c, PUBACK, &timer) == PUBACK)
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (mqtt_deserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
                rc = MQTT_FAILURE;
        }
        else
            rc = MQTT_FAILURE;
    }
    else if (message->qos == MQTT_QOS2)
    {
        if (waitfor(c, PUBCOMP, &timer) == PUBCOMP)
        {
            unsigned short mypacketid;
            unsigned char dup, type;
            if (mqtt_deserialize_ack(&type, &dup, &mypacketid, c->readbuf, c->readbuf_size) != 1)
                rc = MQTT_FAILURE;
        }
        else
            rc = MQTT_FAILURE;
    }

exit:
    if (rc == MQTT_FAILURE)
        MQTTCloseSession(c);
#if defined(MQTT_TASK)
    MutexUnlock(&c->mutex);
#endif
    return rc;
}

int mqtt_unsubscribe(mqtt_client* c, const char* topicFilter)
{
    int rc = MQTT_FAILURE;
    Timer timer;
    mqtt_string topic = mqtt_string_initializer;
    topic.cstring = (char *)topicFilter;
    int len = 0;

#if defined(MQTT_TASK)
    MutexLock(&c->mutex);
#endif
    if (!c->isconnected)
        goto exit;

    TimerInit(&timer);
    TimerCountdownMS(&timer, c->command_timeout_ms);

    if ((len = mqtt_serialize_unsubscribe(c->buf, c->buf_size, 0, getNextPacketId(c), 1, &topic)) <= 0)
        goto exit;
    if ((rc = sendPacket(c, len, &timer)) != MQTT_SUCCESS) // send the subscribe packet
        goto exit; // there was a problem

    if (waitfor(c, UNSUBACK, &timer) == UNSUBACK) {
        unsigned short mypacketid;  // should be the same as the packetid above
        if (mqtt_deserialize_unsuback(&mypacketid, c->readbuf, c->readbuf_size) == 1) {
            /* remove the subscription message handler associated with this topic, if there is one */
            mqtt_set_msg_handler(c, topicFilter, NULL);
        }
    } else {
        rc = MQTT_FAILURE;
    }

exit:
    if (rc == MQTT_FAILURE)
        MQTTCloseSession(c);
#if defined(MQTT_TASK)
    MutexUnlock(&c->mutex);
#endif
    return rc;
}

int mqtt_subscribe(mqtt_client* c, const char* topicFilter, enum mqtt_qos qos,
       messageHandler messageHandler, mqtt_suback_data* data)
{
    int rc = MQTT_FAILURE;
    Timer timer;
    int len = 0;
    mqtt_string topic = mqtt_string_initializer;
    topic.cstring = (char *)topicFilter;

#if defined(MQTT_TASK)
    MutexLock(&c->mutex);
#endif
    if (!c->isconnected)
        goto exit;

    TimerInit(&timer);
    TimerCountdownMS(&timer, c->command_timeout_ms);

    len = mqtt_serialize_subscribe(c->buf, c->buf_size, 0, getNextPacketId(c), 1, &topic, (int*)&qos);
    if (len <= 0)
        goto exit;
    if ((rc = sendPacket(c, len, &timer)) != MQTT_SUCCESS) // send the subscribe packet
        goto exit;             // there was a problem

    if (waitfor(c, SUBACK, &timer) == SUBACK)      // wait for suback
    {
        int count = 0;
        unsigned short mypacketid;
        data->grantedQoS = MQTT_QOS0;
        if (mqtt_deserialize_suback(&mypacketid, 1, &count, (int*)&data->grantedQoS, c->readbuf, c->readbuf_size) == 1)
        {
            if (data->grantedQoS != 0x80)
                rc = mqtt_set_msg_handler(c, topicFilter, messageHandler);
        }
    }
    else
        rc = MQTT_FAILURE;

exit:
    if (rc == MQTT_FAILURE)
        MQTTCloseSession(c);
#if defined(MQTT_TASK)
    MutexUnlock(&c->mutex);
#endif
    return rc;
}

int mqtt_yield(mqtt_client* c, int timeout_ms)
{
    int rc = MQTT_SUCCESS;
    Timer timer;

    TimerInit(&timer);
    TimerCountdownMS(&timer, timeout_ms);

    do {
        if (cycle(c, &timer) < 0) {
            rc = MQTT_FAILURE;
            break;
        }
    } while (!TimerIsExpired(&timer));

    return rc;
}

void MQTTRun(void* parm)
{
    Timer timer;
    mqtt_client* c = (mqtt_client*)parm;

    TimerInit(&timer);

    while (1) {
#if defined(MQTT_TASK)
        MutexLock(&c->mutex);
#endif
        TimerCountdownMS(&timer, 500); /* Don't wait too long if no traffic is incoming */
        cycle(c, &timer);
#if defined(MQTT_TASK)
        MutexUnlock(&c->mutex);
#endif
    }
}

#if defined(MQTT_TASK)
int mqtt_start_task(mqtt_client* client)
{
    return ThreadStart(&client->thread, &MQTTRun, client);
}
#endif


int mqtt_is_connected(mqtt_client* client)
{
    return client->isconnected;
}

int mqtt_disconnect(mqtt_client* c)
{
    int rc = MQTT_FAILURE;
    Timer timer;     // we might wait for incomplete incoming publishes to complete
    int len = 0;

#if defined(MQTT_TASK)
    MutexLock(&c->mutex);
#endif
    TimerInit(&timer);
    TimerCountdownMS(&timer, c->command_timeout_ms);

    len = mqtt_serialize_disconnect(c->buf, c->buf_size);
    if (len > 0)
        rc = sendPacket(c, len, &timer);            // send the disconnect packet
    MQTTCloseSession(c);

#if defined(MQTT_TASK)
    MutexUnlock(&c->mutex);
#endif
    return rc;
}

void mqtt_client_deinit(mqtt_client* c)
{
    if (c) {
        close(c->fd);
        free(c->buf);
        free(c->readbuf);
        free(c->host);
    }
}
