/*
 * Basic multicast DNS responder
 *
 * Advertises the IP address, port, and characteristics of a service to other
 * devices using multicast DNS on the same LAN, so they can find devices with
 * addresses dynamically allocated by DHCP. See avahi, Bonjour, etc See RFC6762,
 * RFC6763
 *
 * This sample code is in the public domain.
 *
 * by M J A Hamel 2016
 */

#ifndef __MDNSRESPONDER_H__
#define __MDNSRESPONDER_H__

#include <lwip/ip_addr.h>

/* The default maximum reply size, increase as necessary. */
#ifndef MDNS_RESPONDER_REPLY_SIZE
#define MDNS_RESPONDER_REPLY_SIZE      1460
#endif

// Starts the mDNS responder task, call first
void mdns_init();

// Build and advertise an appropriate linked set of PTR/TXT/SRV/A records for the parameters provided
// This is a simple canned way to build a set of records for a single service that will
// be advertised whenever the device is given an IP address by WiFi

typedef enum {
    mdns_TCP,
    mdns_UDP,
    mdns_Browsable        // see RFC6763:11 - adds a standard record that lets browsers find the service without needing to know its name
} mdns_flags;

// Clear all records
void mdns_clear();

void mdns_add_facility( const char* instanceName,   // Short user-friendly instance name, should NOT include serial number/MAC/etc
                        const char* serviceName,    // Must be registered, _name, (see RFC6335 5.1 & 5.2)
                        const char* addText,        // Should be <key>=<value>, or "" if unused (see RFC6763 6.3)
                        mdns_flags  flags,          // TCP or UDP plus browsable
                        u16_t       onPort,         // port number
                        u32_t       ttl             // time-to-live, seconds
                      );


// Low-level RR builders for rolling your own
void mdns_add_PTR(const char* rKey, u32_t ttl, const char* nameStr);
void mdns_add_SRV(const char* rKey, u32_t ttl, u16_t rPort, const char* targname);
void mdns_add_TXT(const char* rKey, u32_t ttl, const char* txtStr);
void mdns_add_A  (const char* rKey, u32_t ttl, const ip4_addr_t *addr);
#if LWIP_IPV6
void mdns_add_AAAA(const char* rKey, u32_t ttl, const ip6_addr_t *addr);
#endif

void mdns_TXT_append(char* txt, size_t txt_size, const char* record, size_t record_size);
/* Sample usage, advertising a secure web service

    mdns_init();
    mdns_add_facility("Fluffy", "_https", "Zoom=1", mdns_TCP+mdns_Browsable, 443, 600);

*/

#endif
