/*****************************************************************************
 * Copyright (C) 2014-2015
 * file:    libstun.cc
 * author:  gozfree <gozfree@163.com>
 * created: 2015-08-05 00:14
 * updated: 2015-08-05 00:14
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#ifndef __ANDROID__
#include <ifaddrs.h>
#endif
#include <fcntl.h>
#include <netdb.h>
#include <resolv.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netinet/in.h>
#include <net/if.h>
#include "libstun.h"


#define STUN_PORT                   (3478)
#define STUN_MAX_STRING             (256)
#define STUN_MAX_UNKNOWN_ATTRIBUTES (8)
#define STUN_MAX_MESSAGE_SIZE       (2048)

static stun_addr g_stun_srv;
static const int INVALID_SOCKET = -1;
static const int SOCKET_ERROR = -1;

// define some basic types
/// define a structure to hold a stun address
static const uint8_t IPv4Family = 0x01;
static const uint8_t IPv6Family = 0x02;

// define  flags
static const uint32_t ChangeIpFlag = 0x04;
static const uint32_t ChangePortFlag = 0x02;

// define  stun attribute
#define MappedAddress       0x0001
#define ResponseAddress     0x0002
#define ChangeRequest       0x0003
#define SourceAddress       0x0004
#define ChangedAddress      0x0005
#define Username            0x0006
#define Password            0x0007
#define MessageIntegrity    0x0008
#define ErrorCode           0x0009
#define UnknownAttribute    0x000A
#define ReflectedFrom       0x000B
#define XorMappedAddress    0x8020
#define XorOnly             0x0021
#define ServerName          0x8022
#define SecondaryAddress    0x8050 // Non standard extention

// define types for a stun message
#define BindRequestMsg               0x0001
#define BindResponseMsg              0x0101
#define BindErrorResponseMsg         0x0111
#define SharedSecretRequestMsg       0x0002
#define SharedSecretResponseMsg      0x0102
#define SharedSecretErrorResponseMsg 0x0112

typedef struct {
    unsigned char octet[16];
} uint128_t;

typedef struct {
    uint16_t msgType;
    uint16_t msgLength;
    uint128_t id;
} StunMsgHdr;

typedef struct {
    uint16_t type;
    uint16_t length;
} StunAtrHdr;

typedef struct {
    uint8_t pad;
    uint8_t family;
    stun_addr ipv4;
} StunAtrAddress4;

typedef struct {
    uint32_t value;
} StunAtrChangeRequest;

typedef struct {
    uint16_t pad;                 // all 0
    uint8_t errorClass;
    uint8_t number;
    char reason[STUN_MAX_STRING];
    uint16_t sizeReason;
} StunAtrError;

typedef struct {
    uint16_t attrType[STUN_MAX_UNKNOWN_ATTRIBUTES];
    uint16_t numAttributes;
} StunAtrUnknown;

typedef struct {
    char value[STUN_MAX_STRING];
    uint16_t sizeValue;
} StunAtrString;

typedef struct {
    char hash[20];
} StunAtrIntegrity;

typedef struct {
    StunMsgHdr msgHdr;

    int hasMappedAddress;
    StunAtrAddress4 mappedAddress;

    int hasResponseAddress;
    StunAtrAddress4 responseAddress;

    int hasChangeRequest;
    StunAtrChangeRequest changeRequest;

    int hasSourceAddress;
    StunAtrAddress4 sourceAddress;

    int hasChangedAddress;
    StunAtrAddress4 changedAddress;

    int hasUsername;
    StunAtrString username;

    int hasPassword;
    StunAtrString password;

    int hasMessageIntegrity;
    StunAtrIntegrity messageIntegrity;

    int hasErrorCode;
    StunAtrError errorCode;

    int hasUnknownAttributes;
    StunAtrUnknown unknownAttributes;

    int hasReflectedFrom;
    StunAtrAddress4 reflectedFrom;

    int hasXorMappedAddress;
    StunAtrAddress4 xorMappedAddress;

    int xorOnly;

    int hasServerName;
    StunAtrString serverName;

    int hasSecondaryAddress;
    StunAtrAddress4 secondaryAddress;
} StunMessage;

// Define enum with different types of NAT
typedef enum {
    StunTypeUnknown = 0,
    StunTypeFailure,
    StunTypeOpen,
    StunTypeBlocked,
    StunTypeConeNat,
    StunTypeRestrictedNat,
    StunTypePortRestrictedNat,
    StunTypeSymNat,
    StunTypeSymFirewall,
} NatType;

static void computeHmac(char *hmac, const char *input,
                int length, const char *key, int sizeKey)
{
    strncpy(hmac, "hmac-not-implemented", 20);
}

static int openPort(unsigned short port, unsigned int interfaceIp)
{
    int fd;
    int reuse;

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == INVALID_SOCKET) {
        printf("Could not create a UDP socket: %d", errno);
        return INVALID_SOCKET;
    }

    reuse = 1;
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                            (void *)&reuse, sizeof(reuse))) {
        printf("setsockopt: errno = %d\n", errno);
    }

    struct sockaddr_in addr;
    memset((char *)&(addr), 0, sizeof((addr)));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if ((interfaceIp != 0) && (interfaceIp != 0x100007f)) {
        addr.sin_addr.s_addr = htonl(interfaceIp);
    }

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        int e = errno;
        switch (e) {
        case 0:
            printf("Could not bind socket\n");
            break;
        case EADDRINUSE:
            printf("Port %d for receiving UDP is in use\n", port);
            break;
        case EADDRNOTAVAIL:
            break;
        default:
            printf("Could not bind UDP receive port errno:%d\n", e);
            break;
        }
        return INVALID_SOCKET;
    }

    return fd;
}

static int getMessage(int fd, char *buf, int *len, unsigned int *srcIp,
                unsigned short *srcPort)
{
    int originalSize = *len;
    struct sockaddr_in from;
    int fromLen = sizeof(from);
//    int flag = MSG_DONTWAIT;
    int flag = 0;

    *len = recvfrom(fd, buf, originalSize, flag,
                    (struct sockaddr *)&from, (socklen_t *)&fromLen);
    if (*len == SOCKET_ERROR) {
        int err = errno;
        switch (err) {
        case ENOTSOCK:
            fprintf(stderr, "Error fd not a socket\n");
            break;
        case ECONNRESET:
            fprintf(stderr, "Error connection reset - host not reachable\n");
            break;
        default:
            fprintf(stderr, "socket Error=%d\n", err);
        }
        return 0;
    }

    if (*len < 0) {
        fprintf(stderr, "socket closed? negative len\n");
        return 0;
    }

    if (*len == 0) {
        fprintf(stderr, "socket closed? zero len\n");
        return 0;
    }

    *srcPort = ntohs(from.sin_port);
    *srcIp = ntohl(from.sin_addr.s_addr);

    if ((*len) + 1 >= originalSize) {
        return 0;
    }
    buf[*len] = 0;

    return 1;
}

static int sendMessage(int fd, char *buf, int l, unsigned int dstIp,
                unsigned short dstPort)
{
    int verbose = 0;
    int s;
    if (dstPort == 0) {
        // sending on a connected port
        s = send(fd, buf, l, 0);
    } else {
        struct sockaddr_in to;
        int toLen = sizeof(to);
        memset(&to, 0, toLen);

        to.sin_family = AF_INET;
        to.sin_port = htons(dstPort);
        to.sin_addr.s_addr = htonl(dstIp);
        s = sendto(fd, buf, l, 0, (struct sockaddr *) & to, toLen);
    }

    if (s == SOCKET_ERROR) {
        int e = errno;
        switch (e) {
        case ECONNREFUSED:
        case EHOSTDOWN:
        case EHOSTUNREACH:
            // quietly ignore this
            break;
        case EAFNOSUPPORT:
            fprintf(stderr, "err EAFNOSUPPORT in send\n");
            break;
        default:
            fprintf(stderr, "err %d %s in send\n", e, strerror(e));
            break;
        }
        return 0;
    }

    if (s == 0) {
        fprintf(stderr, "no data sent in send\n");
        return 0;
    }

    if (s != l) {
        if (verbose) {
            fprintf(stderr, "only %d out of %d bytes sent\n", s, l);
        }
        return 0;
    }

    return 1;
}

static int stunParseAtrAddr(char *body, unsigned int hdrLen,
                StunAtrAddress4 *result)
{
    if (hdrLen != 8) {
        fprintf(stderr, "hdrLen wrong for Address\n");
        return 0;
    }
    result->pad = *body++;
    result->family = *body++;
    if (result->family == IPv4Family) {
        uint16_t nport;
        memcpy(&nport, body, 2);
        body += 2;
        result->ipv4.port = ntohs(nport);

        uint32_t naddr;
        memcpy(&naddr, body, 4);
        body += 4;
        result->ipv4.addr = ntohl(naddr);
        return 1;
    } else if (result->family == IPv6Family) {
        fprintf(stderr, "ipv6 not supported\n");
    } else {
        fprintf(stderr, "bad address family: %d\n", result->family);
    }

    return 0;
}

static int stunParseAtrChangeRequest(char *body,
                unsigned int hdrLen, StunAtrChangeRequest *result)
{
    if (hdrLen != 4) {
        printf("hdr length = %u expecting %zu\n",  hdrLen, sizeof(*result));
        printf("Incorrect size for ChangeRequest\n");
        return 0;
    } else {
        memcpy(&result->value, body, 4);
        result->value = ntohl(result->value);
        return 1;
    }
}

static int stunParseAtrError(char *body, unsigned int hdrLen,
                StunAtrError *result)
{
    if (hdrLen >= sizeof(*result)) {
        fprintf(stderr, "head on Error too large\n");
        return 0;
    } else {
        memcpy(&result->pad, body, 2);
        body += 2;
        result->pad = ntohs(result->pad);
        result->errorClass = *body++;
        result->number = *body++;

        result->sizeReason = hdrLen - 4;
        memcpy(&result->reason, body, result->sizeReason);
        result->reason[result->sizeReason] = 0;
        return 1;
    }
}

static int stunParseAtrUnknown(char *body, unsigned int hdrLen,
                StunAtrUnknown *result)
{
    int i;
    if (hdrLen >= sizeof(*result)) {
        return 0;
    } else {
        if (hdrLen % 4 != 0)
            return 0;
        result->numAttributes = hdrLen / 4;
        for (i = 0; i < result->numAttributes; i++) {
            memcpy(&result->attrType[i], body, 2);
            body += 2;
            result->attrType[i] = ntohs(result->attrType[i]);
        }
        return 1;
    }
}

static int stunParseAtrString(char *body, unsigned int hdrLen,
                StunAtrString *result)
{
    if (hdrLen >= STUN_MAX_STRING) {
        fprintf(stderr, "String is too large\n");
        return 0;
    } else {
        if (hdrLen % 4 != 0) {
            fprintf(stderr, "Bad length string %d\n", hdrLen);
            return 0;
        }
        result->sizeValue = hdrLen;
        memcpy(&result->value, body, hdrLen);
        result->value[hdrLen] = 0;
        return 1;
    }
}

static int stunParseAtrIntegrity(char *body, unsigned int hdrLen,
                      StunAtrIntegrity *result)
{
    if (hdrLen != 20) {
        fprintf(stderr, "MessageIntegrity must be 20 bytes\n");
        return 0;
    } else {
        memcpy(&result->hash, body, hdrLen);
        return 1;
    }
}

static int stunParseMessage(char *buf, unsigned int bufLen, StunMessage *msg)
{
    memset(msg, 0, sizeof(*msg));

    if (sizeof(StunMsgHdr) > bufLen) {
        fprintf(stderr, "Bad message\n");
        return 0;
    }

    memcpy(&msg->msgHdr, buf, sizeof(StunMsgHdr));
    msg->msgHdr.msgType = ntohs(msg->msgHdr.msgType);
    msg->msgHdr.msgLength = ntohs(msg->msgHdr.msgLength);

    if (msg->msgHdr.msgLength + sizeof(StunMsgHdr) != bufLen) {
        printf("Message header length doesn't match message size: %d - %d\n",
                        msg->msgHdr.msgLength, bufLen);
        return 0;
    }

    char *body = buf + sizeof(StunMsgHdr);
    unsigned int size = msg->msgHdr.msgLength;

    while (size > 0) {
        // !jf! should check that there are enough bytes left in the buffer

        StunAtrHdr *attr = (StunAtrHdr *)body;

        unsigned int attrLen = ntohs(attr->length);
        int atrType = ntohs(attr->type);

        if (attrLen + 4 > size) {
            printf("claims attribute is larger than size of"
                            " message \"(attribute type= %d \")\n", atrType);
            return 0;
        }

        body += 4;              // skip the length and type in attribute header
        size -= 4;

        switch (atrType) {
        case MappedAddress:
            msg->hasMappedAddress = 1;
            if (stunParseAtrAddr(body, attrLen, &msg->mappedAddress) == 0) {
                fprintf(stderr, "problem parsing MappedAddress\n");
                return 0;
            }
            break;
        case ResponseAddress:
            msg->hasResponseAddress = 1;
            if (stunParseAtrAddr(body, attrLen, &msg->responseAddress) ==
                0) {
                fprintf(stderr, "problem parsing ResponseAddress\n");
                return 0;
            }
            break;
        case ChangeRequest:
            msg->hasChangeRequest = 1;
            if (stunParseAtrChangeRequest(body, attrLen,
                                    &msg->changeRequest) == 0) {
                fprintf(stderr, "problem parsing ChangeRequest\n");
                return 0;
            }
            break;
        case SourceAddress:
            msg->hasSourceAddress = 1;
            if (stunParseAtrAddr(body, attrLen, &msg->sourceAddress) == 0) {
                fprintf(stderr, "problem parsing SourceAddress\n");
                return 0;
            }
            break;
        case ChangedAddress:
            msg->hasChangedAddress = 1;
            if (stunParseAtrAddr(body, attrLen, &msg->changedAddress) == 0) {
                fprintf(stderr, "problem parsing ChangedAddress\n");
                return 0;
            }
            break;
        case Username:
            msg->hasUsername = 1;
            if (stunParseAtrString(body, attrLen, &msg->username) == 0) {
                fprintf(stderr, "problem parsing Username\n");
                return 0;
            }
            break;
        case Password:
            msg->hasPassword = 1;
            if (stunParseAtrString(body, attrLen, &msg->password) == 0) {
                fprintf(stderr, "problem parsing Password\n");
                return 0;
            }
            break;
        case MessageIntegrity:
            msg->hasMessageIntegrity = 1;
            if (stunParseAtrIntegrity(body, attrLen,
                                    &msg->messageIntegrity) == 0) {
                fprintf(stderr, "problem parsing MessageIntegrity\n");
                return 0;
            }
            // read the current HMAC
            // look up the password given the user of given the transaction id
            // compute the HMAC on the buffer
            // decide if they match or not
            break;
        case ErrorCode:
            msg->hasErrorCode = 1;
            if (stunParseAtrError(body, attrLen, &msg->errorCode) == 0) {
                fprintf(stderr, "problem parsing ErrorCode\n");
                return 0;
            }
            break;

        case UnknownAttribute:
            msg->hasUnknownAttributes = 1;
            if (stunParseAtrUnknown(body, attrLen,
                                    &msg->unknownAttributes) == 0) {
                fprintf(stderr, "problem parsing UnknownAttribute\n");
                return 0;
            }
            break;

        case ReflectedFrom:
            msg->hasReflectedFrom = 1;
            if (stunParseAtrAddr(body, attrLen, &msg->reflectedFrom) == 0) {
                fprintf(stderr, "problem parsing ReflectedFrom\n");
                return 0;
            }
            break;

        case XorMappedAddress:
            msg->hasXorMappedAddress = 1;
            if (stunParseAtrAddr(body, attrLen, &msg->xorMappedAddress) == 0) {
                fprintf(stderr, "problem parsing XorMappedAddress\n");
                return 0;
            }
            break;
        case XorOnly:
            msg->xorOnly = 1;
            break;
        case ServerName:
            msg->hasServerName = 1;
            if (stunParseAtrString(body, attrLen, &msg->serverName) == 0) {
                fprintf(stderr, "problem parsing ServerName\n");
                return 0;
            }
            break;
        case SecondaryAddress:
            msg->hasSecondaryAddress = 1;
            if (stunParseAtrAddr(body, attrLen, &msg->secondaryAddress) == 0) {
                fprintf(stderr, "problem parsing secondaryAddress\n");
                return 0;
            }
            break;
        default:
            fprintf(stderr, "Unknown attribute: %d\n", atrType);
            if (atrType <= 0x7FFF) {
                return 0;
            }
            break;
        }

        body += attrLen;
        size -= attrLen;
    }

    return 1;
}

static char *encode16(char *buf, uint16_t data)
{
    uint16_t ndata = htons(data);
    memcpy(buf, (void *)(&ndata), sizeof(uint16_t));
    return buf + sizeof(uint16_t);
}

static char *encode32(char *buf, uint32_t data)
{
    uint32_t ndata = htonl(data);
    memcpy(buf, (void *)(&ndata), sizeof(uint32_t));
    return buf + sizeof(uint32_t);
}

static char *encode(char *buf, const char *data, unsigned int length)
{
    memcpy(buf, data, length);
    return buf + length;
}

static char *encodeAtrAddress4(char *ptr, uint16_t type,
                               const StunAtrAddress4 *atr)
{
    ptr = encode16(ptr, type);
    ptr = encode16(ptr, 8);
    *ptr++ = atr->pad;
    *ptr++ = IPv4Family;
    ptr = encode16(ptr, atr->ipv4.port);
    ptr = encode32(ptr, atr->ipv4.addr);

    return ptr;
}

static char *encodeAtrChangeRequest(char *ptr, const StunAtrChangeRequest *atr)
{
    ptr = encode16(ptr, ChangeRequest);
    ptr = encode16(ptr, 4);
    ptr = encode32(ptr, atr->value);
    return ptr;
}

static char *encodeAtrError(char *ptr, const StunAtrError *atr)
{
    ptr = encode16(ptr, ErrorCode);
    ptr = encode16(ptr, 6 + atr->sizeReason);
    ptr = encode16(ptr, atr->pad);
    *ptr++ = atr->errorClass;
    *ptr++ = atr->number;
    ptr = encode(ptr, atr->reason, atr->sizeReason);
    return ptr;
}

static char *encodeAtrUnknown(char *ptr, const StunAtrUnknown *atr)
{
    int i;
    ptr = encode16(ptr, UnknownAttribute);
    ptr = encode16(ptr, 2 + 2 * atr->numAttributes);
    for (i = 0; i < atr->numAttributes; i++) {
        ptr = encode16(ptr, atr->attrType[i]);
    }
    return ptr;
}

static char *encodeXorOnly(char *ptr)
{
    ptr = encode16(ptr, XorOnly);
    return ptr;
}

static char *encodeAtrString(char *ptr, uint16_t type, const StunAtrString *atr)
{
    assert(atr->sizeValue % 4 == 0);

    ptr = encode16(ptr, type);
    ptr = encode16(ptr, atr->sizeValue);
    ptr = encode(ptr, atr->value, atr->sizeValue);
    return ptr;
}

static char *encodeAtrIntegrity(char *ptr, const StunAtrIntegrity *atr)
{
    ptr = encode16(ptr, MessageIntegrity);
    ptr = encode16(ptr, 20);
    ptr = encode(ptr, atr->hash, sizeof(atr->hash));
    return ptr;
}

static unsigned int stunEncodeMessage(const StunMessage *msg,
                  char *buf,
                  unsigned int bufLen,
                  const StunAtrString *password)
{
    assert(bufLen >= sizeof(StunMsgHdr));
    char *ptr = buf;

    ptr = encode16(ptr, msg->msgHdr.msgType);
    char *lengthp = ptr;
    ptr = encode16(ptr, 0);
    ptr =
        encode(ptr, (const char *)(msg->msgHdr.id.octet),
               sizeof(msg->msgHdr.id));


    if (msg->hasMappedAddress) {

        ptr = encodeAtrAddress4(ptr, MappedAddress, &msg->mappedAddress);
    }
    if (msg->hasResponseAddress) {
        ptr = encodeAtrAddress4(ptr, ResponseAddress, &msg->responseAddress);
    }
    if (msg->hasChangeRequest) {
        ptr = encodeAtrChangeRequest(ptr, &msg->changeRequest);
    }
    if (msg->hasSourceAddress) {
        ptr = encodeAtrAddress4(ptr, SourceAddress, &msg->sourceAddress);
    }
    if (msg->hasChangedAddress) {
        ptr = encodeAtrAddress4(ptr, ChangedAddress, &msg->changedAddress);
    }
    if (msg->hasUsername) {
        ptr = encodeAtrString(ptr, Username, &msg->username);
    }
    if (msg->hasPassword) {
        ptr = encodeAtrString(ptr, Password, &msg->password);
    }
    if (msg->hasErrorCode) {
        ptr = encodeAtrError(ptr, &msg->errorCode);
    }
    if (msg->hasUnknownAttributes) {
        ptr = encodeAtrUnknown(ptr, &msg->unknownAttributes);
    }
    if (msg->hasReflectedFrom) {
        ptr = encodeAtrAddress4(ptr, ReflectedFrom, &msg->reflectedFrom);
    }
    if (msg->hasXorMappedAddress) {
        ptr = encodeAtrAddress4(ptr, XorMappedAddress, &msg->xorMappedAddress);
    }
    if (msg->xorOnly) {
        ptr = encodeXorOnly(ptr);
    }
    if (msg->hasServerName) {
        ptr = encodeAtrString(ptr, ServerName, &msg->serverName);
    }
    if (msg->hasSecondaryAddress) {
        ptr = encodeAtrAddress4(ptr, SecondaryAddress, &msg->secondaryAddress);
    }

    if (password->sizeValue > 0) {
        StunAtrIntegrity integrity;
        computeHmac(integrity.hash, buf, (int)(ptr - buf), password->value,
                    password->sizeValue);
        ptr = encodeAtrIntegrity(ptr, &integrity);
    }

    encode16(lengthp, (uint16_t)(ptr - buf - sizeof(StunMsgHdr)));
    return (int)(ptr - buf);
}

static int stunRand(void)
{
    // return 32 bits of random stuff
    assert(sizeof(int) == 4);
    static int init = 0;
    if (!init) {
        init = 1;
        uint64_t tick;
#if defined(__GNUC__) || ( defined(__i686__) || defined(__i386__) )
        asm("rdtsc":"=A"(tick));
#elif defined(__MACH__)  || defined(__linux)
        int fd = open("/dev/random", O_RDONLY);
        read(fd, &tick, sizeof(tick));
        close(fd);
#else
#error Need some way to seed the random number generator
#endif
        int seed = (int)(tick);
        srandom(seed);
    }

    return random();
}

/// return a random number to use as a port
static int stunRandomPort(void)
{
    int min = 0x4000;
    int max = 0x7FFF;

    int ret = stunRand();
    ret = ret | min;
    ret = ret & max;

    return ret;
}

static int stunParseHostName(char *peerName,
                  uint32_t *ip, uint16_t *portVal, uint16_t defaultPort)
{
    struct in_addr sin_addr;

    char host[512];
    strncpy(host, peerName, 512);
    host[512 - 1] = '\0';
    char *port = NULL;

    int portNum = defaultPort;

    // pull out the port part if present.
    char *sep = strchr(host, ':');

    if (sep == NULL) {
        portNum = defaultPort;
    } else {
        *sep = '\0';
        port = sep + 1;
        // set port part

        char *endPtr = NULL;

        portNum = strtol(port, &endPtr, 10);

        if (endPtr != NULL) {
            if (*endPtr != '\0') {
                portNum = defaultPort;
            }
        }
    }

    if (portNum < 1024)
        return 0;
    if (portNum >= 0xFFFF)
        return 0;

    // figure out the host part
    struct hostent *h;

    h = gethostbyname(host);
    if (h == NULL) {
        fprintf(stderr, "error was %d\n", errno);
        *ip = ntohl(0x7F000001L);
        return 0;
    } else {
        sin_addr = *(struct in_addr *)h->h_addr;
        *ip = ntohl(sin_addr.s_addr);
    }

    *portVal = portNum;

    return 1;
}

static int stunParseServerName(char *name, stun_addr *addr)
{
    // TODO - put in DNS SRV stuff.
    int ret = stunParseHostName(name, &addr->addr, &addr->port, STUN_PORT);
    if (ret != 1) {
        addr->port = 0xFFFF;
    }
    return ret;
}

static void stunBuildReqSimple(StunMessage * msg,
                   const StunAtrString *username,
                   int changePort, int changeIp, unsigned int id)
{
    int i;
    memset(msg, 0, sizeof(*msg));

    msg->msgHdr.msgType = BindRequestMsg;

    for (i = 0; i < 16; i += 4) {
        int r = stunRand();
        msg->msgHdr.id.octet[i + 0] = r >> 0;
        msg->msgHdr.id.octet[i + 1] = r >> 8;
        msg->msgHdr.id.octet[i + 2] = r >> 16;
        msg->msgHdr.id.octet[i + 3] = r >> 24;
    }

    if (id != 0) {
        msg->msgHdr.id.octet[0] = id;
    }

    msg->hasChangeRequest = 1;
    msg->changeRequest.value = (changeIp ? ChangeIpFlag : 0) |
        (changePort ? ChangePortFlag : 0);

    if (username->sizeValue > 0) {
        msg->hasUsername = 1;
        msg->username = *username;
    }
}

static void stunSendTest(int myFd, stun_addr *dest,
             const StunAtrString *username, const StunAtrString *password,
             int testNum)
{
    int changePort = 0;
    int changeIP = 0;
    //int discard = 0;

    switch (testNum) {
    case 1:
    case 10:
    case 11:
        break;
    case 2:
        //changePort=1;
        changeIP = 1;
        break;
    case 3:
        changePort = 1;
        break;
    case 4:
        changeIP = 1;
        break;
    case 5:
        //discard = 1;
        break;
    default:
        fprintf(stderr, "Test %d is unkown\n", testNum);
        assert(0);
        break;
    }

    StunMessage req;
    memset(&req, 0, sizeof(StunMessage));

    stunBuildReqSimple(&req, username, changePort, changeIP, testNum);
    char buf[STUN_MAX_MESSAGE_SIZE];
    int len = STUN_MAX_MESSAGE_SIZE;
    len = stunEncodeMessage(&req, buf, len, password);
    sendMessage(myFd, buf, len, dest->addr, dest->port);

    // add some delay so the packets don't get sent too quickly
    //usleep(10 * 1000);
}

static NatType stunNatType(stun_addr *dest,
    int * preservePort, // if set, is return for if NAT preservers ports or not
    int * hairpin, // if set, is the return for if NAT will hairpin packets
    int port,   // port to use for the test, 0 to choose random port
    stun_addr * sAddr    // NIC to use
    )
{
    int i;
    if (hairpin) {
        *hairpin = 0;
    }

    if (port == 0) {
        port = stunRandomPort();
    }
    uint32_t interfaceIp = 0;
    if (sAddr) {
        interfaceIp = sAddr->addr;
    }
    int myFd1 = openPort(port, interfaceIp);
    int myFd2 = openPort(port + 1, interfaceIp);

    if ((myFd1 == INVALID_SOCKET) || (myFd2 == INVALID_SOCKET)) {
        fprintf(stderr, "Some problem opening port/interface to send on\n");
        return StunTypeFailure;
    }

    assert(myFd1 != INVALID_SOCKET);
    assert(myFd2 != INVALID_SOCKET);

    int respTestI = 0;
    int isNat = 1;
    //stun_addr testIchangedAddr;
    stun_addr testImappedAddr;
    int respTestI2 = 0;
    int mappedIpSame = 1;
    stun_addr testI2mappedAddr;
    stun_addr testI2dest = *dest;
    int respTestII = 0;
    int respTestIII = 0;

    int respTestHairpin = 0;
    int respTestPreservePort = 0;

    memset(&testImappedAddr, 0, sizeof(testImappedAddr));

    StunAtrString username;
    StunAtrString password;

    username.sizeValue = 0;
    password.sizeValue = 0;

    int count = 0;
    while (count < 7) {
        struct timeval tv;
        fd_set fdSet;

        int fdSetSize;
        FD_ZERO(&fdSet);
        fdSetSize = 0;
        FD_SET(myFd1, &fdSet);
        fdSetSize = (myFd1 + 1 > fdSetSize) ? myFd1 + 1 : fdSetSize;
        FD_SET(myFd2, &fdSet);
        fdSetSize = (myFd2 + 1 > fdSetSize) ? myFd2 + 1 : fdSetSize;
        tv.tv_sec = 0;
        tv.tv_usec = 150 * 1000;    // 150 ms
        if (count == 0)
            tv.tv_usec = 0;

        int err = select(fdSetSize, &fdSet, NULL, NULL, &tv);
        int e = errno;
        if (err == SOCKET_ERROR) {
            // error occured
            fprintf(stderr, "Error %d %s in select\n", e, strerror(e));
            return StunTypeFailure;
        } else if (err == 0) {
            // timeout occured
            count++;

            if (!respTestI) {
                stunSendTest(myFd1, dest, &username, &password, 1);
            }

            if ((!respTestI2) && respTestI) {
                // check the address to send to if valid
                if ((testI2dest.addr != 0) && (testI2dest.port != 0)) {
                    stunSendTest(myFd1, &testI2dest, &username, &password, 10);
                }
            }

            if (!respTestII) {
                stunSendTest(myFd2, dest, &username, &password, 2);
            }

            if (!respTestIII) {
                stunSendTest(myFd2, dest, &username, &password, 3);
            }

            if (respTestI && (!respTestHairpin)) {
                if ((testImappedAddr.addr != 0)
                                && (testImappedAddr.port != 0)) {
                    stunSendTest(myFd1, &testImappedAddr,
                                    &username, &password, 11);
                }
            }
        } else {
            assert(err > 0);
            // data is avialbe on some fd

            for (i = 0; i < 2; i++) {
                int myFd;
                if (i == 0) {
                    myFd = myFd1;
                } else {
                    myFd = myFd2;
                }

                if (myFd != INVALID_SOCKET) {
                    if (FD_ISSET(myFd, &fdSet)) {
                        char msg[STUN_MAX_MESSAGE_SIZE];
                        int msgLen = sizeof(msg);

                        stun_addr from;

                        getMessage(myFd,
                                   msg,
                                   &msgLen, &from.addr, &from.port);

                        StunMessage resp;
                        memset(&resp, 0, sizeof(StunMessage));

                        stunParseMessage(msg, msgLen, &resp);

                        switch (resp.msgHdr.id.octet[0]) {
                        case 1:
                            if (!respTestI) {

                                //testIchangedAddr.addr =
                                //    resp.changedAddress.ipv4.addr;
                                //testIchangedAddr.port =
                                //    resp.changedAddress.ipv4.port;
                                testImappedAddr.addr =
                                    resp.mappedAddress.ipv4.addr;
                                testImappedAddr.port =
                                    resp.mappedAddress.ipv4.port;

                                respTestPreservePort =
                                    (testImappedAddr.port == port);
                                if (preservePort) {
                                    *preservePort = respTestPreservePort;
                                }

                                testI2dest.addr =
                                    resp.changedAddress.ipv4.addr;

                                if (sAddr) {
                                    sAddr->port = testImappedAddr.port;
                                    sAddr->addr = testImappedAddr.addr;
                                }

                                count = 0;
                            }
                            respTestI = 1;
                            break;
                        case 2:
                            respTestII = 1;
                            break;
                        case 3:
                            respTestIII = 1;
                            break;
                        case 10:
                            if (!respTestI2) {
                                testI2mappedAddr.addr =
                                    resp.mappedAddress.ipv4.addr;
                                testI2mappedAddr.port =
                                    resp.mappedAddress.ipv4.port;

                                mappedIpSame = 0;
                                if ((testI2mappedAddr.addr ==
                                     testImappedAddr.addr)
                                    && (testI2mappedAddr.port ==
                                        testImappedAddr.port)) {
                                    mappedIpSame = 1;
                                }

                            }
                            respTestI2 = 1;
                            break;
                        case 11:
                            if (hairpin) {
                                *hairpin = 1;
                            }
                            respTestHairpin = 1;
                            break;
                        }
                    }
                }
            }
        }
    }

    // see if we can bind to this address
    int s = openPort(0 /*use ephemeral */ , testImappedAddr.addr);
    if (s != INVALID_SOCKET) {
        close(s);
        isNat = 0;
    } else {
        isNat = 1;
    }

    // implement logic flow chart from draft RFC
    if (respTestI) {
        if (isNat) {
            if (respTestII) {
                return StunTypeConeNat;
            } else {
                if (mappedIpSame) {
                    if (respTestIII) {
                        return StunTypeRestrictedNat;
                    } else {
                        return StunTypePortRestrictedNat;
                    }
                } else {
                    return StunTypeSymNat;
                }
            }
        } else {
            if (respTestII) {
                return StunTypeOpen;
            } else {
                return StunTypeSymFirewall;
            }
        }
    } else {
        return StunTypeBlocked;
    }
    return StunTypeUnknown;
}

static int stunOpenSocket(stun_addr *dest, stun_addr * mapAddr,
               int port, stun_addr * srcAddr)
{
    stun_addr from;
    int err;
    int fdSetSize;
    struct timeval tv;
    fd_set fdSet;

    if (port == 0) {
        port = stunRandomPort();
    }
    unsigned int interfaceIp = 0;
    if (srcAddr) {
        interfaceIp = srcAddr->addr;
    }

    int myFd = openPort(port, interfaceIp);
    if (myFd == INVALID_SOCKET) {
        return myFd;
    }

    char msg[STUN_MAX_MESSAGE_SIZE];
    int msgLen = sizeof(msg);

    StunAtrString username;
    StunAtrString password;

    username.sizeValue = 0;
    password.sizeValue = 0;

    stunSendTest(myFd, dest, &username, &password, 1);

    while (1) {
        FD_ZERO(&fdSet);
        FD_SET(myFd,&fdSet);
        tv.tv_sec=0;
        tv.tv_usec=2000*1000;
        fdSetSize = (myFd+1>fdSetSize) ? myFd+1 : fdSetSize;

        err = select(fdSetSize, &fdSet, NULL, NULL, &tv);
        if (-1 == err) {
            perror("select");
        } else if (0 == err) {
            fprintf(stderr, "no data to recv\n");
            return -1;
        } else {
            break;
        }
    }

    getMessage(myFd, msg, &msgLen, &from.addr, &from.port);

    StunMessage resp;
    memset(&resp, 0, sizeof(StunMessage));

    int ok = stunParseMessage(msg, msgLen, &resp);
    if (!ok) {
        return -1;
    }

    stun_addr mappedAddr = resp.mappedAddress.ipv4;
    //stun_addr changedAddr = resp.changedAddress.ipv4;

    *mapAddr = mappedAddr;

    return myFd;
}

int stun_init(const char *ip)
{
    return stunParseServerName((char *)ip, &g_stun_srv);
}

int stun_socket(const char *ip, uint16_t port, stun_addr *map)
{
    int fd;
    stun_addr src;
    if (ip == NULL) {
        fd = stunOpenSocket(&g_stun_srv, map, port, NULL);
    } else {
        stunParseServerName((char *)ip, &src);
        fd = stunOpenSocket(&g_stun_srv, map, port, &src);
    }

    return fd;
}

int stun_nat_type(void)
{
    int presPort = 0;
    int hairpin = 0;

    stun_addr sAddr;
    sAddr.port = 0;
    sAddr.addr = 0;

    NatType stype = stunNatType(&g_stun_srv, &presPort, &hairpin, 0, &sAddr);
    switch (stype) {
        case StunTypeOpen:
            printf("No NAT detected - P2P should work\n");
            break;
        case StunTypeConeNat:
            printf("NAT type: Full Cone Nat detect - P2P will work with STUN\n");
            break;
        case StunTypeRestrictedNat:
            printf("NAT type: Address restricted - P2P will work with STUN\n");
            break;
        case StunTypePortRestrictedNat:
            printf("NAT type: Port restricted - P2P will work with STUN\n");
            break;
        case StunTypeSymNat:
            printf("NAT type: Symetric - P2P will NOT work\n");
            break;
        case StunTypeBlocked:
            printf("Could not reach the stun server - check server name is correct\n");
            break;
        case StunTypeFailure:
            printf("Local network is not work!\n");
            break;
        default:
            printf("Unkown NAT type\n");
            break;
    }
    return stype;
}

void stun_keep_alive(int fd)
{
    char buf[32] = "keep alive";
    sendMessage(fd, buf, strlen(buf), g_stun_srv.addr, g_stun_srv.port);
}
