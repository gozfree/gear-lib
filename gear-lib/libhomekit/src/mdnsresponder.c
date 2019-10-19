/*
 * Basic multicast DNS responder
 * 
 * Advertises the IP address, port, and characteristics of a service to other
 * devices using multicast DNS on the same LAN, so they can find devices with
 * addresses dynamically allocated by DHCP. See avahi, Bonjour, etc. See
 * RFC6762, RFC6763
 *
 * This sample code is in the public domain.
 *
 * by M J A Hamel 2016
 */


#include <espressif/esp_common.h>
#include <espressif/esp_wifi.h>

#include <string.h>
#include <stdio.h>
#include <etstimer.h>
#include <esplibs/libmain.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>
#include <lwip/prot/dns.h>
#include <lwip/prot/iana.h>
#include <lwip/udp.h>
#include <lwip/igmp.h>
#include <lwip/netif.h>

#include "mdnsresponder.h"

#if !LWIP_IGMP
#error "LWIP_IGMP needs to be defined in lwipopts.h"
#endif

// #define qDebugLog             // Log activity generally
// #define qLogIncoming          // Log all arriving multicast packets
// #define qLogAllTraffic        // Log and decode all mDNS packets

#ifndef HOMEKIT_MDNS_NETWORK_CHECK_PERIOD
#define HOMEKIT_MDNS_NETWORK_CHECK_PERIOD 2000
#endif

//-------------------------------------------------------------------

#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif

PACK_STRUCT_BEGIN
/** DNS message header */
struct mdns_hdr {
    PACK_STRUCT_FIELD(u16_t id);
    PACK_STRUCT_FIELD(u8_t flags1);
    PACK_STRUCT_FIELD(u8_t flags2);
    PACK_STRUCT_FIELD(u16_t numquestions);
    PACK_STRUCT_FIELD(u16_t numanswers);
    PACK_STRUCT_FIELD(u16_t numauthrr);
    PACK_STRUCT_FIELD(u16_t numextrarr);
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END

#define SIZEOF_DNS_HDR 12

PACK_STRUCT_BEGIN
/** MDNS query message structure */
struct mdns_query {
    /* MDNS query record starts with either a domain name or a pointer
     to a name already present somewhere in the packet. */
    PACK_STRUCT_FIELD(u16_t type);
    PACK_STRUCT_FIELD(u16_t class);
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END

#define SIZEOF_DNS_QUERY 4

PACK_STRUCT_BEGIN
/** MDNS answer message structure */
struct mdns_answer {
    /* MDNS answer record starts with either a domain name or a pointer
     to a name already present somewhere in the packet. */
    PACK_STRUCT_FIELD(u16_t type);
    PACK_STRUCT_FIELD(u16_t class);
    PACK_STRUCT_FIELD(u32_t ttl);
    PACK_STRUCT_FIELD(u16_t len);
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#define SIZEOF_DNS_ANSWER 10

PACK_STRUCT_BEGIN
struct mdns_rr_srv {
    /* RR SRV  */
    PACK_STRUCT_FIELD(u16_t prio);
    PACK_STRUCT_FIELD(u16_t weight);
    PACK_STRUCT_FIELD(u16_t port);
}PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#define SIZEOF_DNS_RR_SRV 6


#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

#define vTaskDelayMs(ms)    vTaskDelay((ms)/portTICK_PERIOD_MS)
#define UNUSED_ARG(x)       (void)x
#define kDummyDataSize      8           // arbitrary, dynamically resized
#define kMaxNameSize        64
#define kMaxQStr            128         // max incoming question key handled

typedef struct mdns_rsrc {
    struct mdns_rsrc*    rNext;
    u16_t     rType;
    u32_t    rTTL;
    u16_t    rKeySize;
    u16_t    rDataSize;
    char    rData[kDummyDataSize];      // Key, as C str with . seperators, followed by data in network-ready form
                                        // at rData[rKeySize]
} mdns_rsrc;

static struct udp_pcb* gMDNS_pcb = NULL;
static const ip_addr_t gMulticastV4Addr = DNS_MQUERY_IPV4_GROUP_INIT;
#if LWIP_IPV6
#include "lwip/mld6.h"
static const ip_addr_t gMulticastV6Addr = DNS_MQUERY_IPV6_GROUP_INIT;
#endif
static SemaphoreHandle_t gDictMutex = NULL;
static mdns_rsrc*      gDictP = NULL;       // RR database, linked list

//---------------------- Debug/logging utilities -------------------------

    // DNS field TYPE used for "Resource Records", some additions
    #define DNS_RRTYPE_AAAA           28    /* IPv6 host address */
    #define DNS_RRTYPE_SRV            33    /* Service record */
    #define DNS_RRTYPE_OPT            41    /* EDNS0 OPT record */
    #define DNS_RRTYPE_NSEC           47    /* NSEC record */
    #define DNS_RRTYPE_TSIG           250   /* Transaction Signature */
    #define DNS_RRTYPE_ANY            255   /* Not a DNS type, but a DNS query type, meaning "all types"*/

    // DNS field CLASS used for "Resource Records" 
    #define DNS_RRCLASS_ANY           255  /* Any class (q) */

    #define DNS_FLAG1_RESP            0x80
    #define DNS_FLAG1_OPMASK          0x78
    #define DNS_FLAG1_AUTH            0x04
    #define DNS_FLAG1_TRUNC           0x02
    #define DNS_FLAG1_RD              0x01
    #define DNS_FLAG2_RA              0x80
    #define DNS_FLAG2_RESMASK         0x0F

#ifdef qDebugLog
    static char qstr[12];

    static char* mdns_qrtype(uint16_t typ)
    {
        switch(typ) {
            case DNS_RRTYPE_A    : return ("A");
            case DNS_RRTYPE_NS   : return ("NS");
            case DNS_RRTYPE_PTR  : return ("PTR");
            case DNS_RRTYPE_TXT  : return ("TXT ");
            case DNS_RRTYPE_AAAA : return ("AAAA");
            case DNS_RRTYPE_SRV  : return ("SRV ");
            case DNS_RRTYPE_NSEC : return ("NSEC ");
            case DNS_RRTYPE_ANY  : return ("ANY");
        }
        sprintf(qstr, "type %d", typ);
        return qstr;
    }

#ifdef qLogAllTraffic

        static void mdns_printhex(u8_t* p, int n)
        {
            int i;
            for (i=0; i<n; i++) {
                printf("%02X ",*p++);
                if ((i % 32) == 31) printf("\n");
            }
            printf("\n");
        }

        static void mdns_print_pstr(u8_t* p)
        {
            int i, n;
            char* cp;

            n = *p++;
            cp = (char*)p;
            for (i = 0; i < n; i++) {
                putchar(*cp++);
            }
        }

        static char cstr[16];

        static char* mdns_qclass(uint16_t cls)
        {
            switch(cls) {
                case DNS_RRCLASS_IN  : return ("In");
                case DNS_RRCLASS_ANY : return ("ANY");
            }
            sprintf(cstr,"class %d",cls);
            return cstr;
        }

        // Sequence of Pascal strings, terminated by zero-length string
        // Handles compression, returns ptr to next item
        static u8_t* mdns_print_name(u8_t* p, struct mdns_hdr* hp)
        {
            char* cp = (char*)p;
            int i, n;

            do {
                n = *cp++;
                if ((n & 0xC0) == 0xC0) {
                    n = (n & 0x3F) << 8;
                    n |= (u8_t)*cp++;
                    mdns_print_name((u8_t*)hp + n, hp);
                    n = 0;
                } else if (n & 0xC0) {
                    printf("<label $%X?>",n);
                    n = 0;
                } else {
                    for (i = 0; i < n; i++)
                        putchar(*cp++);
                    if (n != 0) putchar('.');
                }
            } while (n > 0);
            return (u8_t*)cp;
        }


        static u8_t* mdns_print_header(struct mdns_hdr* hdr)
        {
            if (hdr->flags1 & DNS_FLAG1_RESP) {
                printf("Response, ID $%X %s ", htons(hdr->id), (hdr->flags1 & DNS_FLAG1_AUTH) ? "Auth " : "Non-auth ");
                if (hdr->flags2 & DNS_FLAG2_RA) printf("RA ");
                if ((hdr->flags2 & DNS_FLAG2_RESMASK) == 0) printf("noerr");
                                       else printf("err %d", hdr->flags2 & DNS_FLAG2_RESMASK);
            } else {
                printf("Query, ID $%X op %d", htons(hdr->id), (hdr->flags1 >> 4) & 0x7 );
            }
            if (hdr->flags1 & DNS_FLAG1_RD) printf("RD ");
            if (hdr->flags1 & DNS_FLAG1_TRUNC) printf("[TRUNC] ");

            printf(": %d questions", htons(hdr->numquestions) );
            if (hdr->numanswers != 0)
                printf(", %d answers",htons(hdr->numanswers));
            if (hdr->numauthrr != 0)
                printf(", %d auth RR",htons(hdr->numauthrr));
            if (hdr->numextrarr != 0)
                printf(", %d extra RR",htons(hdr->numextrarr));
            putchar('\n');
            return (u8_t*)hdr + SIZEOF_DNS_HDR;
        }

        // Copy needed because it may be misaligned
        static u8_t* mdns_print_query(u8_t* p)
        {
            struct mdns_query q;
            uint16_t c;

            memcpy(&q, p, SIZEOF_DNS_QUERY);
            c = htons(q.class);
            printf(" %s %s", mdns_qrtype(htons(q.type)), mdns_qclass(c & 0x7FFF) );
            if (c & 0x8000) printf(" unicast-req");
            printf("\n");
            return p + SIZEOF_DNS_QUERY;
        }

        // Copy needed because it may be misaligned
        static u8_t* mdns_print_answer(u8_t* p, struct mdns_hdr* hp)
        {
            struct mdns_answer ans;
            u16_t rrlen, atype, rrClass;;

            memcpy(&ans,p,SIZEOF_DNS_ANSWER);
            atype = htons(ans.type);
            rrlen = htons(ans.len);
            rrClass = htons(ans.class);
            printf(" %s %s TTL %d ", mdns_qrtype(atype), mdns_qclass(rrClass & 0x7FFF), htonl(ans.ttl));
            if (rrClass & 0x8000) 
                printf("cache-flush ");
            if (rrlen > 0) {
                u8_t* rp = p + SIZEOF_DNS_ANSWER;
                if (atype == DNS_RRTYPE_A && rrlen == 4) {
                    printf("%d.%d.%d.%d\n",rp[0],rp[1],rp[2],rp[3]);
                } else if (atype == DNS_RRTYPE_PTR) {
                    mdns_print_name(rp, hp);
                    printf("\n");
                } else if (atype == DNS_RRTYPE_TXT) {
                    mdns_print_pstr(rp);
                    printf("\n");
                } else if (atype == DNS_RRTYPE_SRV && rrlen > SIZEOF_DNS_RR_SRV) {
                    struct mdns_rr_srv srvRR;
                    memcpy(&srvRR, rp, SIZEOF_DNS_RR_SRV);
                    printf("prio %d, weight %d, port %d, target ", srvRR.prio, srvRR.weight, ntohs(srvRR.port));
                    mdns_print_name(rp + SIZEOF_DNS_RR_SRV, hp);
                    printf("\n");
                } else {
                    printf("%db:", rrlen);
                    mdns_printhex(rp, rrlen);
                }
            } else
                printf("\n");
            return p + SIZEOF_DNS_ANSWER + rrlen;
        }

        static int mdns_print_msg(u8_t* msgP, int msgLen)
        {
            int i;
            u8_t *tp;
            u8_t *limP = msgP + msgLen;
            struct mdns_hdr* hdr;

            hdr = (struct mdns_hdr*) msgP;
            tp = mdns_print_header(hdr);
            for (i = 0; i < htons(hdr->numquestions); i++) {
                printf(" Q%d: ", i + 1);
                tp = mdns_print_name(tp, hdr);
                tp = mdns_print_query(tp);
                if (tp > limP) return 0;
            }

            for (i = 0; i < htons(hdr->numanswers); i++) {
                printf(" A%d: ", i + 1);
                tp = mdns_print_name(tp, hdr);
                tp = mdns_print_answer(tp, hdr);
                if (tp > limP) return 0;
            }

            for (i = 0; i < htons(hdr->numauthrr); i++) {
                printf(" AuRR%d: ", i + 1);
                tp = mdns_print_name(tp, hdr);
                tp = mdns_print_answer(tp, hdr);
                if (tp > limP) return 0;
            }

            for (i = 0; i < htons(hdr->numextrarr); i++) {
                printf(" ExRR%d: ", i + 1);
                tp = mdns_print_name(tp, hdr);
                tp = mdns_print_answer(tp, hdr);
                if (tp > limP) return 0;
            }
            return 1;
        }
#endif // qLogAllTraffic
#endif // qDebugLog

//---------------------------------------------------------------------------

// Convert a DNS domain name label sequence into C string with . seperators
// Handles compression
static u8_t* mdns_labels2str(u8_t* hdrP, u8_t* p, char* qStr)
{
    int i, n;

    do {
        n = *p++;
        if ((n & 0xC0) == 0xC0) {
            n = (n & 0x3F) << 8;
            n |= (u8_t)*p++;
            mdns_labels2str( hdrP, hdrP + n, qStr);
            return p;
        } else if (n & 0xC0) {
            printf(">>> mdns_labels2str,label $%X?",n);
            return p;
        } else {
            for (i = 0; i < n; i++)
                *qStr++ = *p++;
            if (n == 0) *qStr++ = 0;
                 else *qStr++ = '.';
        }
    } while (n>0);
    return p;
}

// Encode a <string>.<string>.<string> as a sequence of labels, return length
static int mdns_str2labels(const char* name, u8_t* lseq, int max)
{
    int i, n, sdx, idx = 0;
    int lc = 0;

    do {
        sdx = idx;
        while (name[idx] != '.' && name[idx] != 0) idx++;
        n = idx - sdx;
        if (lc + 1 + n > max) {
            printf(">>> mdns_str2labels: oversize (%d)\n", lc + 1 + n);
            return 0;
        }
        *lseq++ = n;
        lc++;
        for (i = 0; i < n; i++)
            *lseq++ = name[sdx + i];
        lc += n;
        if (name[idx] == '.')
            idx++;
    } while (n > 0);
    return lc;
}

// Unpack a DNS question RR at qp, return pointer to next RR
static u8_t* mdns_get_question(u8_t* hdrP, u8_t* qp, char* qStr, uint16_t* qClass, uint16_t* qType, u8_t* qUnicast)
{
    struct mdns_query qr;
    uint16_t cls;

    qp = mdns_labels2str(hdrP, qp, qStr);
    memcpy(&qr, qp, SIZEOF_DNS_QUERY);
    *qType = htons(qr.type);
    cls = htons(qr.class);
    *qUnicast = cls >> 15;
    *qClass = cls & 0x7FFF;
    return qp + SIZEOF_DNS_QUERY;
}

//---------------------------------------------------------------------------
static void mdns_announce_netif(struct netif *netif, const ip_addr_t *addr);

static ETSTimer announce_timer;

static bool network_down = true;
static ETSTimer network_monitor_timer;


void mdns_clear() {
    sdk_os_timer_disarm(&announce_timer);
    sdk_os_timer_disarm(&network_monitor_timer);

    if (!xSemaphoreTake(gDictMutex, portMAX_DELAY))
        return;

    mdns_rsrc *rsrc = gDictP;
    gDictP = NULL;

    while (rsrc) {
        mdns_rsrc *next = rsrc->rNext;
        free(rsrc);
        rsrc = next;
    }

    xSemaphoreGive(gDictMutex);
}


void mdns_TXT_append(char* txt, size_t txt_size, const char* record, size_t record_size)
{
    size_t txt_len = strlen(txt);

    if (record_size > 255) {
        char *s = strndup(record, record_size);
        printf(">>> mdns_txt_add: record %s section is longer than 255\n", s);
        free(s);
        return;
    }

    if (txt_len + record_size + 2 > txt_size) {  // extra 2 is for length and terminator
        char *s = strndup(record, record_size);
        printf(">>> mdns_txt_add: not enough space to add TXT record %s\n", s);
        free(s);
        return;
    }

    txt[txt_len] = record_size;
    memcpy(txt + txt_len + 1, record, record_size);
    txt[txt_len + record_size + 1] = 0;
}


// Add a record to the RR database list
static void mdns_add_response(const char* vKey, u16_t vType, u32_t ttl, const void* dataP, u16_t vDataSize)
{
    mdns_rsrc* rsrcP;
    int keyLen, recSize;

    keyLen = strlen(vKey) + 1;
    recSize = sizeof(mdns_rsrc) - kDummyDataSize + keyLen + vDataSize;
    rsrcP = (mdns_rsrc*)malloc(recSize);
    if (rsrcP == NULL) {
        printf(">>> mdns_add_response: couldn't alloc %d\n",recSize);
    } else {
        rsrcP->rType = vType;
        rsrcP->rTTL = ttl;
        rsrcP->rKeySize = keyLen;
        rsrcP->rDataSize = vDataSize;
        memcpy(rsrcP->rData, vKey, keyLen);
        memcpy(&rsrcP->rData[keyLen], dataP, vDataSize);

        if (xSemaphoreTake(gDictMutex, portMAX_DELAY)) {
            rsrcP->rNext = gDictP;
            gDictP = rsrcP;
            xSemaphoreGive(gDictMutex);
        }

#ifdef qDebugLog
        printf("mDNS added RR '%s' %s, %d bytes\n", vKey, mdns_qrtype(vType), vDataSize);
#endif
    }
}

void mdns_add_PTR(const char* rKey, u32_t ttl, const char* nmStr)
{
    size_t nl;
    u8_t lBuff[kMaxNameSize];

    nl = mdns_str2labels(nmStr, lBuff, sizeof(lBuff));
    if (nl > 0) {
        mdns_add_response(rKey, DNS_RRTYPE_PTR, ttl, lBuff, nl);
    }
}

void mdns_add_SRV(const char* rKey, u32_t ttl, u16_t rPort, const char* targName)
{
    typedef struct SrvRec  {
        struct mdns_rr_srv srvRR;
        u8_t lBuff[kMaxNameSize];
    } __attribute__((packed)) SrvRec;

    int     nl;
    SrvRec     temp;

    temp.srvRR.prio = 0;
    temp.srvRR.weight = 0;
    temp.srvRR.port = htons(rPort);
    nl = mdns_str2labels(targName, temp.lBuff, sizeof(temp.lBuff));
    if (nl > 0)
        mdns_add_response(rKey, DNS_RRTYPE_SRV, ttl, &temp, SIZEOF_DNS_RR_SRV + nl);
}

// Single TXT str, can be concatenated
void mdns_add_TXT(const char* rKey, u32_t ttl, const char* txStr)
{
    mdns_add_response(rKey, DNS_RRTYPE_TXT, ttl, txStr, strlen(txStr));
}

void mdns_add_A(const char* rKey, u32_t ttl, const ip4_addr_t *addr)
{
    mdns_add_response(rKey, DNS_RRTYPE_A, ttl, addr, sizeof(ip4_addr_t));
}

#if LWIP_IPV6
void mdns_add_AAAA(const char* rKey, u32_t ttl, const ip6_addr_t *addr)
{
    mdns_add_response(rKey, DNS_RRTYPE_AAAA, ttl, addr, sizeof(addr->addr));
}
#endif

void mdns_announce() {
    struct netif *netif = sdk_system_get_netif(STATION_IF);
#if LWIP_IPV4
    mdns_announce_netif(netif, &gMulticastV4Addr);
#endif
#if LWIP_IPV6
    mdns_announce_netif(netif, &gMulticastV6Addr);
#endif
}

void mdns_add_facility( const char* instanceName,   // Friendly name, need not be unique
                        const char* serviceName,    // Must be "_name", e.g. "_hap" or "_http"
                        const char* addText,        // Must be <key>=<value>
                        mdns_flags flags,           // TCP or UDP
                        u16_t onPort,               // port number
                        u32_t ttl                   // seconds
                      )
{
    size_t key_len = strlen(serviceName) + 12;
    char *key = malloc(key_len + 1);
    size_t full_name_len = strlen(instanceName) + 1 + key_len;
    char *fullName = malloc(full_name_len + 1);
    size_t dev_name_len = strlen(instanceName) + 7;
    char *devName = malloc(dev_name_len + 1);

#ifdef qDebugLog
    printf("\nmDNS advertising instance %s protocol %s", instanceName, serviceName);
    if (addText) {
        printf(" text %s", addText);
    }
    printf(" on port %d %s TTL %d secs\n", onPort, (flags & mdns_UDP) ? "UDP" : "TCP", ttl);
#endif

    snprintf(key, key_len + 1, "%s.%s.local.", serviceName, (flags & mdns_UDP) ? "_udp" :"_tcp");
    snprintf(fullName, full_name_len + 1, "%s.%s", instanceName, key);
    snprintf(devName, dev_name_len + 1, "%s.local.", instanceName);

    // Order has significance for extraRR feature
    if (addText) {
        mdns_add_TXT(fullName, ttl, addText);
    }

#if LWIP_IPV6
    const ip6_addr_t addr6 = { {0ul, 0ul, 0ul, 0ul} };
    mdns_add_AAAA(devName, ttl, &addr6);
#endif

    const ip4_addr_t addr4 = { 0 };
    mdns_add_A(devName, ttl, &addr4);

    mdns_add_SRV(fullName, ttl, onPort, devName);
    mdns_add_PTR(key, ttl, fullName);

    // Optional, makes us browsable
    if (flags & mdns_Browsable) {
        mdns_add_PTR("_services._dns-sd._udp.local.", ttl, key);
    }

    free(key);
    free(fullName);
    free(devName);

    if (sdk_wifi_station_get_connect_status() == STATION_GOT_IP)
        mdns_announce();

    sdk_os_timer_arm(&announce_timer, ttl * 1000, 1);
    sdk_os_timer_arm(&network_monitor_timer, HOMEKIT_MDNS_NETWORK_CHECK_PERIOD, 1);
}

static mdns_rsrc* mdns_match(const char* qstr, u16_t qType)
{
    mdns_rsrc* rp = gDictP;
    while (rp != NULL) {
       if (rp->rType == qType || qType == DNS_RRTYPE_ANY) {
            if (strcasecmp(rp->rData, qstr) == 0) {
#ifdef qDebugLog
                printf(" - matched '%s' %s\n", qstr, mdns_qrtype(rp->rType));
#endif
                break;
            }
        }
        rp = rp->rNext;
    }
    return rp;
}

// Create answer RR and append to resp[respLen], return new length
static int mdns_add_to_answer(mdns_rsrc* rsrcP, u8_t* resp, int respLen)
{
    // Key is stored as C str, convert to labels
    size_t rem = MDNS_RESPONDER_REPLY_SIZE - respLen;
    size_t len = mdns_str2labels(rsrcP->rData, &resp[respLen], rem);
    if (len == 0) {
        // Overflow, skip this answer.
        return respLen;
    }
    if ((len + SIZEOF_DNS_ANSWER + rsrcP->rDataSize) > rem) {
        // Overflow, skip this answer.
        printf(">>> mdns_add_to_answer: oversize (%d)\n", len + SIZEOF_DNS_ANSWER + rsrcP->rDataSize);
        return respLen;
    }
    respLen += len;

    // Answer fields: may be misaligned, so build and memcpy
    struct mdns_answer ans;
    ans.type  = htons(rsrcP->rType);
    ans.class = htons(DNS_RRCLASS_IN);
    ans.ttl   = htonl(rsrcP->rTTL);
    ans.len   = htons(rsrcP->rDataSize);
    memcpy(&resp[respLen], &ans, SIZEOF_DNS_ANSWER);
    respLen += SIZEOF_DNS_ANSWER;

    // Data for this key
    memcpy(&resp[respLen], &rsrcP->rData[rsrcP->rKeySize], rsrcP->rDataSize);
    respLen += rsrcP->rDataSize;

    return respLen;
}

//---------------------------------------------------------------------------

// Send UDP to multicast address
static void mdns_send_mcast(const ip_addr_t *addr, u8_t* msgP, int nBytes)
{
    struct pbuf* p;
    err_t err;

#ifdef qLogAllTraffic
    mdns_print_msg(msgP, nBytes);
#endif

    p = pbuf_alloc(PBUF_TRANSPORT, nBytes, PBUF_RAM);
    if (p) {
        memcpy(p->payload, msgP, nBytes);
        const ip_addr_t *dest_addr;
        if (IP_IS_V6_VAL(*addr)) {
#if LWIP_IPV6
            dest_addr = &gMulticastV6Addr;
#endif
        } else {
            dest_addr = &gMulticastV4Addr;
        }
        LOCK_TCPIP_CORE();
        err = udp_sendto(gMDNS_pcb, p, dest_addr, LWIP_IANA_PORT_MDNS);
        UNLOCK_TCPIP_CORE();
        if (err == ERR_OK) {
#ifdef qDebugLog
            printf(" - responded with %d bytes err %d\n", nBytes, err);
#endif
        } else
            printf(">>> mdns_send failed %d\n", err);
        pbuf_free(p);
    } else {
        printf(">>> mdns_send: alloc failed[%d]\n", nBytes);
    }
}
    
// Message has passed tests, may want to send an answer
static void mdns_reply(const ip_addr_t *addr, struct mdns_hdr* hdrP)
{
    int i, nquestions, respLen;
    struct mdns_hdr* rHdr;
    mdns_rsrc* extra;
    u8_t* qBase = (u8_t*)hdrP;
    u8_t* qp;
    u8_t* mdns_response;

    mdns_response = malloc(MDNS_RESPONDER_REPLY_SIZE);
    if (mdns_response == NULL) {
        printf(">>> mdns_reply could not alloc %d\n", MDNS_RESPONDER_REPLY_SIZE);
        return;
    }

    // Build response header
    rHdr = (struct mdns_hdr*) mdns_response;
    rHdr->id = hdrP->id;
    rHdr->flags1 = DNS_FLAG1_RESP + DNS_FLAG1_AUTH;
    rHdr->flags2 = 0;
    rHdr->numquestions = 0;
    rHdr->numanswers = 0;
    rHdr->numauthrr = 0;
    rHdr->numextrarr = 0;
    respLen = SIZEOF_DNS_HDR;

    extra = NULL;
    qp = qBase + SIZEOF_DNS_HDR;
    nquestions = htons(hdrP->numquestions);

    if (xSemaphoreTake(gDictMutex, portMAX_DELAY)) {

        for (i = 0; i < nquestions; i++) {
            char  qStr[kMaxQStr];
            u16_t qClass, qType;
            u8_t  qUnicast;
            mdns_rsrc* rsrcP;

            qp = mdns_get_question(qBase, qp, qStr, &qClass, &qType, &qUnicast);
            if (qClass == DNS_RRCLASS_IN || qClass == DNS_RRCLASS_ANY) {
                rsrcP = mdns_match(qStr, qType);
                if (rsrcP) {
#if LWIP_IPV6
                    if (rsrcP->rType == DNS_RRTYPE_AAAA) {
                        // Emit an answer for each ipv6 address.
                        struct netif *netif = ip_current_input_netif();
                        for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
                            if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i))) {
                                const ip6_addr_t *addr6 = netif_ip6_addr(netif, i);
#ifdef qDebugLog
                                char addr6_str[IP6ADDR_STRLEN_MAX];
                                ip6addr_ntoa_r(addr6, addr6_str, IP6ADDR_STRLEN_MAX);
                                printf("Updating AAAA record for '%s' to %s\n", rsrcP->rData, addr6_str);
#endif
                                memcpy(&rsrcP->rData[rsrcP->rKeySize], addr6, sizeof(addr6->addr));
                                size_t new_len = mdns_add_to_answer(rsrcP, mdns_response, respLen);
                                if (new_len > respLen) {
                                    rHdr->numanswers = htons(htons(rHdr->numanswers) + 1);
                                    respLen = new_len;
                                }
                            }
                        }
                        continue;
                    }
#endif

                    if (rsrcP->rType == DNS_RRTYPE_A) {
                        struct netif *netif = ip_current_input_netif();
#ifdef qDebugLog
                        char addr4_str[IP4ADDR_STRLEN_MAX];
                        ip4addr_ntoa_r(netif_ip4_addr(netif), addr4_str, IP4ADDR_STRLEN_MAX);
                        printf("Updating A record for '%s' to %s\n", rsrcP->rData, addr4_str);
#endif
                        memcpy(&rsrcP->rData[rsrcP->rKeySize], netif_ip4_addr(netif), sizeof(ip4_addr_t));
                    }

                    size_t new_len = mdns_add_to_answer(rsrcP, mdns_response, respLen);
                    if (new_len > respLen) {
                        rHdr->numanswers = htons(htons(rHdr->numanswers) + 1);
                        respLen = new_len;
                    }

                    // Extra RR logic: if SRV follows PTR, or A follows SRV, volunteer it in extraRR
                    // Not required, but could do more here, see RFC6763 s12
                    if (qType == DNS_RRTYPE_PTR) {
                        if (rsrcP->rNext && rsrcP->rNext->rType == DNS_RRTYPE_SRV)
                            extra = rsrcP->rNext;
                    } else if (qType == DNS_RRTYPE_SRV) {
                        if (rsrcP->rNext && rsrcP->rNext->rType == DNS_RRTYPE_A)
                            extra = rsrcP->rNext;
                    }
                }
            }
        } // for nQuestions

        xSemaphoreGive(gDictMutex);
    }

    if (respLen > SIZEOF_DNS_HDR) {
        if (extra) {
            if (extra->rType == DNS_RRTYPE_A) {
                struct netif *netif = ip_current_input_netif();
#ifdef qDebugLog
                char addr4_str[IP4ADDR_STRLEN_MAX];
                ip4addr_ntoa_r(netif_ip4_addr(netif), addr4_str, IP4ADDR_STRLEN_MAX);
                printf("Updating A record for '%s' to %s\n", extra->rData, addr4_str);
#endif
                memcpy(&extra->rData[extra->rKeySize], netif_ip4_addr(netif), sizeof(ip4_addr_t));
            }
            size_t new_len = mdns_add_to_answer(extra, mdns_response, respLen);
            if (new_len > respLen) {
                rHdr->numextrarr = htons(htons(rHdr->numextrarr) + 1);
                respLen = new_len;
            }
        }
        mdns_send_mcast(addr, mdns_response, respLen);
    }

    free(mdns_response);
}

// Announce all configured services
static void mdns_announce_netif(struct netif *netif, const ip_addr_t *addr)
{
    u8_t *mdns_response = malloc(MDNS_RESPONDER_REPLY_SIZE);
    if (mdns_response == NULL) {
        printf(">>> mdns_reply could not alloc %d\n", MDNS_RESPONDER_REPLY_SIZE);
        return;
    }

    // Build response header
    struct mdns_hdr *rHdr = (struct mdns_hdr*) mdns_response;
    memset(rHdr, 0, sizeof(*rHdr));
    rHdr->flags1 = DNS_FLAG1_RESP + DNS_FLAG1_AUTH;

    int respLen = SIZEOF_DNS_HDR;

    if (xSemaphoreTake(gDictMutex, portMAX_DELAY)) {
        mdns_rsrc *rsrcP = gDictP;
        while (rsrcP) {
#if LWIP_IPV6
            if (rsrcP->rType == DNS_RRTYPE_AAAA) {
                // Emit an answer for each ipv6 address.
                for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
                    if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i))) {
                        const ip6_addr_t *addr6 = netif_ip6_addr(netif, i);
#ifdef qDebugLog
                        char addr6_str[IP6ADDR_STRLEN_MAX];
                        ip6addr_ntoa_r(addr6, addr6_str, IP6ADDR_STRLEN_MAX);
                        printf("Updating AAAA record for '%s' to %s\n", rsrcP->rData, addr6_str);
#endif
                        memcpy(&rsrcP->rData[rsrcP->rKeySize], addr6, sizeof(addr6->addr));
                        size_t new_len = mdns_add_to_answer(rsrcP, mdns_response, respLen);
                        if (new_len > respLen) {
                            rHdr->numanswers = htons(htons(rHdr->numanswers) + 1);
                            respLen = new_len;
                        }
                    }
                }
                continue;
            }
#endif

            if (rsrcP->rType == DNS_RRTYPE_A) {
#ifdef qDebugLog
                char addr4_str[IP4ADDR_STRLEN_MAX];
                ip4addr_ntoa_r(netif_ip4_addr(netif), addr4_str, IP4ADDR_STRLEN_MAX);
                printf("Updating A record for '%s' to %s\n", rsrcP->rData, addr4_str);
#endif
                memcpy(&rsrcP->rData[rsrcP->rKeySize], netif_ip4_addr(netif), sizeof(ip4_addr_t));
            }

            size_t new_len = mdns_add_to_answer(rsrcP, mdns_response, respLen);
            if (new_len > respLen) {
                rHdr->numanswers = htons(htons(rHdr->numanswers) + 1);
                respLen = new_len;
            }

            rsrcP = rsrcP->rNext;
        }

        xSemaphoreGive(gDictMutex);
    }

    if (respLen > SIZEOF_DNS_HDR) {
        mdns_send_mcast(addr, mdns_response, respLen);
    }

    free(mdns_response);
}

// Callback from udp_recv
static void mdns_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    UNUSED_ARG(pcb);
    UNUSED_ARG(port);

    u8_t* mdns_payload;
    int   plen;

    plen = p->tot_len;
#ifdef qLogIncoming
    char addr_str[IPADDR_STRLEN_MAX];
    ipaddr_ntoa_r(addr, addr_str, IPADDR_STRLEN_MAX);
    printf("\n\nmDNS IPv%d got %d bytes from %s\n", IP_IS_V6(addr) ? 6 : 4, plen, addr_str);
#endif

    // Sanity checks on size
    if (plen > MDNS_RESPONDER_REPLY_SIZE) {
        printf(">>> mdns_recv: pbuf too big\n");
    } else if (plen < (SIZEOF_DNS_HDR + SIZEOF_DNS_QUERY + 1 + SIZEOF_DNS_ANSWER + 1)) {
        printf(">>> mdns_recv: pbuf too small\n");
    } else {
        mdns_payload = malloc(plen);
        if (!mdns_payload) {
            printf(">>> mdns_recv, could not alloc %d\n",plen);
        } else {
            if (pbuf_copy_partial(p, mdns_payload, plen, 0) == plen) {
                struct mdns_hdr* hdrP = (struct mdns_hdr*) mdns_payload;
#ifdef qLogAllTraffic
                mdns_print_msg(mdns_payload, plen);
#endif

                if ( (hdrP->flags1 & (DNS_FLAG1_RESP + DNS_FLAG1_OPMASK + DNS_FLAG1_TRUNC) ) == 0
                     && hdrP->numquestions > 0 )
                    mdns_reply(addr, hdrP);
            }
            free(mdns_payload);
        }
    }
    pbuf_free(p);
}

static void mdns_check_network() {
    if (sdk_wifi_station_get_connect_status() == STATION_GOT_IP) {
        if (network_down) {
            network_down = false;
            mdns_announce();
        }
    } else {
        network_down = true;
    }
}

// If we are in station mode and have an IP address, start a multicast UDP receive
void mdns_init()
{
    sdk_os_timer_setfn(&announce_timer, mdns_announce, NULL);
    sdk_os_timer_setfn(&network_monitor_timer, mdns_check_network, NULL);

    err_t err;

    struct netif *netif = sdk_system_get_netif(STATION_IF);
    if (netif == NULL) {
        printf(">>> mDNS_init: wifi opmode not station\n");
        return;
    }

    LOCK_TCPIP_CORE();

    // Start IGMP on the netif for our interface: this isn't done for us
    if (!(netif->flags & NETIF_FLAG_IGMP)) {
        netif->flags |= NETIF_FLAG_IGMP;
        err = igmp_start(netif);
        if (err != ERR_OK) {
            printf(">>> mDNS_init: igmp_start on %c%c failed %d\n", netif->name[0], netif->name[1],err);
            UNLOCK_TCPIP_CORE();
            return;
        }
    }

    gDictMutex = xSemaphoreCreateBinary();
    if (!gDictMutex) {
        printf(">>> mDNS_init: failed to initialize mutex\n");
        UNLOCK_TCPIP_CORE();
        return;
    }
    xSemaphoreGive(gDictMutex);

    gMDNS_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!gMDNS_pcb) {
        printf(">>> mDNS_init: udp_new failed\n");
        UNLOCK_TCPIP_CORE();
        return;
    }

    if ((err = igmp_joingroup_netif(netif, ip_2_ip4(&gMulticastV4Addr))) != ERR_OK) {
        printf(">>> mDNS_init: igmp_join failed %d\n",err);
        UNLOCK_TCPIP_CORE();
        return;
    }

#if LWIP_IPV6
    if ((err = mld6_joingroup_netif(netif, ip_2_ip6(&gMulticastV6Addr))) != ERR_OK) {
        printf(">>> mDNS_init: igmp_join failed %d\n",err);
        UNLOCK_TCPIP_CORE();
        return;
    }
#endif

    if ((err = udp_bind(gMDNS_pcb, IP_ANY_TYPE, LWIP_IANA_PORT_MDNS)) != ERR_OK) {
        printf(">>> mDNS_init: udp_bind failed %d\n",err);
        UNLOCK_TCPIP_CORE();
        return;
    }

    udp_bind_netif(gMDNS_pcb, netif);

    udp_recv(gMDNS_pcb, mdns_recv, NULL);

    UNLOCK_TCPIP_CORE();
}
