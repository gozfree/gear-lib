/* mdns.h  -  mDNS/DNS-SD library  -  Public Domain  -  2017 Mattias Jansson
 *
 * This library provides a cross-platform mDNS and DNS-SD library in C.
 * The implementation is based on RFC 6762 and RFC 6763.
 *
 * The latest source code is always available at
 *
 * https://github.com/mjansson/mdns
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#ifdef _WIN32
#include <Winsock2.h>
#include <Ws2tcpip.h>
#define strncasecmp _strnicmp
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#define MDNS_INVALID_POS ((size_t)-1)

#define MDNS_STRING_CONST(s) (s), (sizeof((s))-1)
#define MDNS_STRING_FORMAT(s) (int)((s).length), s.str

enum mdns_record_type {
	MDNS_RECORDTYPE_IGNORE = 0,
	//Address
	MDNS_RECORDTYPE_A = 1,
	//Domain Name pointer
	MDNS_RECORDTYPE_PTR = 12,
	//Arbitrary text string
	MDNS_RECORDTYPE_TXT = 16,
	//IP6 Address [Thomson]
	MDNS_RECORDTYPE_AAAA = 28,
	//Server Selection [RFC2782]
	MDNS_RECORDTYPE_SRV = 33
};

enum mdns_entry_type {
	MDNS_ENTRYTYPE_ANSWER = 1,
	MDNS_ENTRYTYPE_AUTHORITY = 2,
	MDNS_ENTRYTYPE_ADDITIONAL = 3
};

enum mdns_class {
	MDNS_CLASS_IN = 1
};

typedef enum mdns_record_type  mdns_record_type_t;
typedef enum mdns_entry_type   mdns_entry_type_t;
typedef enum mdns_class        mdns_class_t;

typedef int (* mdns_record_callback_fn)(const struct sockaddr* from,
                                        mdns_entry_type_t entry, uint16_t type,
                                        uint16_t rclass, uint32_t ttl,
                                        const void* data, size_t size, size_t offset, size_t length,
                                        void* user_data);

typedef struct mdns_string_t       mdns_string_t;
typedef struct mdns_string_pair_t  mdns_string_pair_t;
typedef struct mdns_record_srv_t   mdns_record_srv_t;
typedef struct mdns_record_txt_t   mdns_record_txt_t;

struct mdns_string_t {
	const char* str;
	size_t length;
};

struct mdns_string_pair_t {
	size_t  offset;
	size_t  length;
	int     ref;
};

struct mdns_record_srv_t {
	uint16_t priority;
	uint16_t weight;
	uint16_t port;
	mdns_string_t name;
};

struct mdns_record_txt_t {
	mdns_string_t key;
	mdns_string_t value;
};

static int
mdns_socket_open_ipv4(void);

static int
mdns_socket_setup_ipv4(int sock);

static int
mdns_socket_open_ipv6(void);

static int
mdns_socket_setup_ipv6(int sock);

static void
mdns_socket_close(int sock);

static int
mdns_discovery_send(int sock);

static size_t
mdns_discovery_recv(int sock, void* buffer, size_t capacity,
                    mdns_record_callback_fn callback, void* user_data);

static int
mdns_query_send(int sock, mdns_record_type_t type, const char* name, size_t length,
                void* buffer, size_t capacity);

static size_t
mdns_query_recv(int sock, void* buffer, size_t capacity,
                mdns_record_callback_fn callback, void* user_data,
                uint8_t one_shot);

static mdns_string_t
mdns_string_extract(const void* buffer, size_t size, size_t* offset,
                    char* str, size_t capacity);

static int
mdns_string_skip(const void* buffer, size_t size, size_t* offset);

static int
mdns_string_equal(const void* buffer_lhs, size_t size_lhs, size_t* ofs_lhs,
                  const void* buffer_rhs, size_t size_rhs, size_t* ofs_rhs);

static void*
mdns_string_make(void* data, size_t capacity, const char* name, size_t length);

static mdns_string_t
mdns_record_parse_ptr(const void* buffer, size_t size, size_t offset, size_t length,
                      char* strbuffer, size_t capacity);

static mdns_record_srv_t
mdns_record_parse_srv(const void* buffer, size_t size, size_t offset, size_t length,
                      char* strbuffer, size_t capacity);

static struct sockaddr_in*
mdns_record_parse_a(const void* buffer, size_t size, size_t offset, size_t length,
                    struct sockaddr_in* addr);

static struct sockaddr_in6*
mdns_record_parse_aaaa(const void* buffer, size_t size, size_t offset, size_t length,
                       struct sockaddr_in6* addr);

static size_t
mdns_record_parse_txt(const void* buffer, size_t size, size_t offset, size_t length,
                      mdns_record_txt_t* records, size_t capacity);

// Implementations

static int
mdns_socket_open_ipv4(void) {
	int sock = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
		return -1;
	if (mdns_socket_setup_ipv4(sock)) {
		mdns_socket_close(sock);
		return -1;
	}
	return sock;
}

static int
mdns_socket_setup_ipv4(int sock) {
	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
#ifdef __APPLE__
	saddr.sin_len = sizeof(saddr);
#endif

	if (bind(sock, (struct sockaddr*)&saddr, sizeof(saddr)))
		return -1;

#ifdef _WIN32
	unsigned long param = 1;
	ioctlsocket(sock, FIONBIO, &param);
#else
	const int flags = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif

	unsigned char ttl = 1;
	unsigned char loopback = 1;
	struct ip_mreq req;

	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (const char*)&ttl, sizeof(ttl));
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (const char*)&loopback, sizeof(loopback));

	memset(&req, 0, sizeof(req));
	req.imr_multiaddr.s_addr = htonl((((uint32_t)224U) << 24U) | ((uint32_t)251U));
	req.imr_interface.s_addr = INADDR_ANY;
	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&req, sizeof(req)))
		return -1;

	return 0;
}

static int
mdns_socket_open_ipv6(void) {
	int sock = (int)socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0)
		return -1;
	if (mdns_socket_setup_ipv6(sock)) {
		mdns_socket_close(sock);
		return -1;
	}
	return sock;
}

static int
mdns_socket_setup_ipv6(int sock) {
	struct sockaddr_in6 saddr;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin6_family = AF_INET6;
	saddr.sin6_addr = in6addr_any;
#ifdef __APPLE__
	saddr.sin6_len = sizeof(saddr);
#endif

	if (bind(sock, (struct sockaddr*)&saddr, sizeof(saddr)))
		return -1;

#ifdef _WIN32
	unsigned long param = 1;
	ioctlsocket(sock, FIONBIO, &param);
#else
	const int flags = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif

	int hops = 1;
	unsigned int loopback = 1;
	struct ipv6_mreq req;

	setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (const char*)&hops, sizeof(hops));
	setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (const char*)&loopback, sizeof(loopback));

	memset(&req, 0, sizeof(req));
	req.ipv6mr_multiaddr.s6_addr[0] = 0xFF;
	req.ipv6mr_multiaddr.s6_addr[1] = 0x02;
	req.ipv6mr_multiaddr.s6_addr[15] = 0xFB;
	if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, (char*)&req, sizeof(req)))
		return -1;

	return 0;
}

static void
mdns_socket_close(int sock) {
#ifdef _WIN32
	closesocket(sock);
#else
	close(sock);
#endif
}

static int
mdns_is_string_ref(uint8_t val) {
	return (0xC0 == (val & 0xC0));
}

static mdns_string_pair_t
mdns_get_next_substring(const void* rawdata, size_t size, size_t offset) {
	const uint8_t* buffer = rawdata;
	mdns_string_pair_t pair = {MDNS_INVALID_POS, 0, 0};
	if (!buffer[offset]) {
		pair.offset = offset;
		return pair;
	}
	if (mdns_is_string_ref(buffer[offset])) {
		if (size < offset + 2)
			return pair;

		offset = (((size_t)(0x3f & buffer[offset]) << 8) | (size_t)buffer[offset + 1]);
		if (offset >= size)
			return pair;

		pair.ref = 1;
	}

	size_t length = (size_t)buffer[offset++];
	if (size < offset + length)
		return pair;

	pair.offset = offset;
	pair.length = length;

	return pair;
}

static int
mdns_string_skip(const void* buffer, size_t size, size_t* offset) {
	size_t cur = *offset;
	mdns_string_pair_t substr;
	do {
		substr = mdns_get_next_substring(buffer, size, cur);
		if (substr.offset == MDNS_INVALID_POS)
			return 0;
		if (substr.ref) {
			*offset = cur + 2;
			return 1;
		}
		cur = substr.offset + substr.length;
	}
	while (substr.length);

	*offset = cur + 1;
	return 1;
}

static int
mdns_string_equal(const void* buffer_lhs, size_t size_lhs, size_t* ofs_lhs,
                  const void* buffer_rhs, size_t size_rhs, size_t* ofs_rhs) {
	size_t lhs_cur = *ofs_lhs;
	size_t rhs_cur = *ofs_rhs;
	size_t lhs_end = MDNS_INVALID_POS;
	size_t rhs_end = MDNS_INVALID_POS;
	mdns_string_pair_t lhs_substr;
	mdns_string_pair_t rhs_substr;
	do {
		lhs_substr = mdns_get_next_substring(buffer_lhs, size_lhs, lhs_cur);
		rhs_substr = mdns_get_next_substring(buffer_rhs, size_rhs, rhs_cur);
		if ((lhs_substr.offset == MDNS_INVALID_POS) || (rhs_substr.offset == MDNS_INVALID_POS))
			return 0;
		if (lhs_substr.length != rhs_substr.length)
			return 0;
		if (strncasecmp((const char*)buffer_rhs + rhs_substr.offset,
		                (const char*)buffer_lhs + lhs_substr.offset, rhs_substr.length))
			return 0;
		if (lhs_substr.ref && (lhs_end == MDNS_INVALID_POS))
			lhs_end = lhs_cur + 2;
		if (rhs_substr.ref && (rhs_end == MDNS_INVALID_POS))
			rhs_end = rhs_cur + 2;
		lhs_cur = lhs_substr.offset + lhs_substr.length;
		rhs_cur = rhs_substr.offset + rhs_substr.length;
	}
	while (lhs_substr.length);

	if (lhs_end == MDNS_INVALID_POS)
		lhs_end = lhs_cur + 1;
	*ofs_lhs = lhs_end;

	if (rhs_end == MDNS_INVALID_POS)
		rhs_end = rhs_cur + 1;
	*ofs_rhs = rhs_end;

	return 1;
}

static mdns_string_t
mdns_string_extract(const void* buffer, size_t size, size_t* offset,
                    char* str, size_t capacity) {
	size_t cur = *offset;
	size_t end = MDNS_INVALID_POS;
	mdns_string_pair_t substr;
	mdns_string_t result = {str, 0};
	char* dst = str;
	size_t remain = capacity;
	do {
		substr = mdns_get_next_substring(buffer, size, cur);
		if (substr.offset == MDNS_INVALID_POS)
			return result;
		if (substr.ref && (end == MDNS_INVALID_POS))
			end = cur + 2;
		if (substr.length) {
			size_t to_copy = (substr.length < remain) ? substr.length : remain;
			memcpy(dst, (const char*)buffer + substr.offset, to_copy);
			dst += to_copy;
			remain -= to_copy;
			if (remain) {
				*dst++ = '.';
				--remain;
			}
		}
		cur = substr.offset + substr.length;
	}
	while (substr.length);

	if (end == MDNS_INVALID_POS)
		end = cur + 1;
	*offset = end;

	result.length = capacity - remain;
	return result;
}

static size_t
mdns_string_find(const char* str, size_t length, char c, size_t offset) {
	const void* found;
	if (offset >= length)
		return MDNS_INVALID_POS;
	found = memchr(str + offset, c, length - offset);
	if (found)
		return (size_t)((const char*)found - str);
	return MDNS_INVALID_POS;
}

static void*
mdns_string_make(void* data, size_t capacity, const char* name, size_t length) {
	size_t pos = 0;
	size_t last_pos = 0;
	size_t remain = capacity;
	unsigned char* dest = data;	
	while ((last_pos < length) && ((pos = mdns_string_find(name, length, '.', last_pos)) != MDNS_INVALID_POS)) {
		size_t sublength = pos - last_pos;
		if (sublength < remain) {
			*dest = (unsigned char)sublength;
			memcpy(dest + 1, name + last_pos, sublength);
			dest += sublength + 1;
			remain -= sublength + 1;
		}
		else {
			return 0;
		}
		last_pos = pos + 1;
	}
	if (last_pos < length) {
		size_t sublength = length - last_pos;
		if (sublength < capacity) {
			*dest = (unsigned char)sublength;
			memcpy(dest + 1, name + last_pos, sublength);
			dest += sublength + 1;
			remain -= sublength + 1;
		}
		else {
			return 0;
		}
	}
	if (!remain)
		return 0;
	*dest++ = 0;
	return dest;
}

static size_t
mdns_records_parse(const struct sockaddr* from, const void* buffer, size_t size, size_t* offset,
                   mdns_entry_type_t type, size_t records, mdns_record_callback_fn callback,
                   void* user_data) {
	size_t parsed = 0;
	int do_callback = 1;
	for (size_t i = 0; i < records; ++i) {
		mdns_string_skip(buffer, size, offset);
		const uint16_t* data = (const uint16_t*)((const char*)buffer + (*offset));

		uint16_t rtype = ntohs(*data++);
		uint16_t rclass = ntohs(*data++);
		uint32_t ttl = ntohs(*(const uint32_t*)(const void*)data); data += 2;
		uint16_t length = ntohs(*data++);

		*offset += 10;

		if (do_callback) {
			++parsed;
			if (callback(from, type, rtype, rclass, ttl, buffer, size, *offset, length,
			             user_data))
				do_callback = 0;
		}

		*offset += length;
	}
	return parsed;
}

static const uint8_t mdns_services_query[] = {
	// Transaction ID
	0x00, 0x00,
	// Flags
	0x00, 0x00,
	// 1 question
	0x00, 0x01,
	// No answer, authority or additional RRs
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	// _services._dns-sd._udp.local.
	0x09, '_', 's', 'e', 'r', 'v', 'i', 'c', 'e', 's',
	0x07, '_', 'd', 'n', 's', '-', 's', 'd',
	0x04, '_', 'u', 'd', 'p',
	0x05, 'l', 'o', 'c', 'a', 'l',
	0x00,
	// PTR record
	0x00, MDNS_RECORDTYPE_PTR,
	// QU (unicast response) and class IN
	0x80, MDNS_CLASS_IN
};

static int
mdns_discovery_send(int sock) {
	struct sockaddr_storage addr_storage;
	struct sockaddr_in addr;
	struct sockaddr_in6 addr6;
	struct sockaddr* saddr = (struct sockaddr*)&addr_storage;
	socklen_t saddrlen = sizeof(struct sockaddr_storage);
	if (getsockname(sock, saddr, &saddrlen))
		return -1;
	if (saddr->sa_family == AF_INET6) {
		memset(&addr6, 0, sizeof(struct sockaddr_in6));
		addr6.sin6_family = AF_INET6;
#ifdef __APPLE__
		addr6.sin6_len = sizeof(struct sockaddr_in6);
#endif
		addr6.sin6_addr.s6_addr[0] = 0xFF;
		addr6.sin6_addr.s6_addr[1] = 0x02;
		addr6.sin6_addr.s6_addr[15] = 0xFB;
		addr6.sin6_port = htons((unsigned short)5353);
		saddr = (struct sockaddr*)&addr6;
		saddrlen = sizeof(struct sockaddr_in6);
	}
	else {
		memset(&addr, 0, sizeof(struct sockaddr_in));
		addr.sin_family = AF_INET;
#ifdef __APPLE__
		addr.sin_len = sizeof(struct sockaddr_in);
#endif
		addr.sin_addr.s_addr = htonl((((uint32_t)224U) << 24U) | ((uint32_t)251U));
		addr.sin_port = htons((unsigned short)5353);
		saddr = (struct sockaddr*)&addr;
		saddrlen = sizeof(struct sockaddr_in);
	}

	if (sendto(sock, mdns_services_query, sizeof(mdns_services_query), 0,
	           saddr, saddrlen) < 0)
		return -1;
	return 0;
}

static size_t
mdns_discovery_recv(int sock, void* buffer, size_t capacity,
                    mdns_record_callback_fn callback, void* user_data) {
	struct sockaddr_in6 addr;
	struct sockaddr* saddr = (struct sockaddr*)&addr;
	memset(&addr, 0, sizeof(addr));
	saddr->sa_family = AF_INET;
#ifdef __APPLE__
	saddr->sa_len = sizeof(addr);
#endif
	socklen_t addrlen = sizeof(addr);
	int ret = recvfrom(sock, buffer, capacity, 0,
	                   saddr, &addrlen);
	if (ret <= 0)
		return 0;

	size_t data_size = (size_t)ret;
	size_t records = 0;
	uint16_t* data = (uint16_t*)buffer;

	uint16_t transaction_id = ntohs(*data++);
	uint16_t flags          = ntohs(*data++);
	uint16_t questions      = ntohs(*data++);
	uint16_t answer_rrs     = ntohs(*data++);
	uint16_t authority_rrs  = ntohs(*data++);
	uint16_t additional_rrs = ntohs(*data++);

	if (transaction_id || (flags != 0x8400))
		return 0; //Not a reply to our question

	if (questions > 1)
		return 0;

	int i;
	for (i = 0; i < questions; ++i) {
		size_t ofs = (size_t)((char*)data - (char*)buffer);
		size_t verify_ofs = 12;
		//Verify it's our question, _services._dns-sd._udp.local.
		if (!mdns_string_equal(buffer, data_size, &ofs,
		                       mdns_services_query, sizeof(mdns_services_query), &verify_ofs))
			return 0;
		data = (uint16_t*)((char*)buffer + ofs);

		uint16_t type = ntohs(*data++);
		uint16_t rclass = ntohs(*data++);

		//Make sure we get a reply based on our PTR question for class IN
		if ((type != MDNS_RECORDTYPE_PTR) || ((rclass & 0x7FFF) != MDNS_CLASS_IN))
			return 0;
	}

	int do_callback = 1;
	for (i = 0; i < answer_rrs; ++i) {
		size_t ofs = (size_t)((char*)data - (char*)buffer);
		size_t verify_ofs = 12;
		//Verify it's an answer to our question, _services._dns-sd._udp.local.
		int is_answer = mdns_string_equal(buffer, data_size, &ofs,
		                                  mdns_services_query, sizeof(mdns_services_query), &verify_ofs);
		data = (uint16_t*)((char*)buffer + ofs);

		uint16_t type = ntohs(*data++);
		uint16_t rclass = ntohs(*data++);
		uint32_t ttl = ntohl(*(uint32_t*)(void*)data); data += 2;
		uint16_t length = ntohs(*data++);

		if (is_answer && do_callback) {
			++records;
			if (callback(saddr, MDNS_ENTRYTYPE_ANSWER, type, rclass, ttl, buffer,
			             data_size, (size_t)((char*)data - (char*)buffer), length,
			             user_data))
				do_callback = 0;
		}
		data = (void*)((char*)data + length);
	}

	size_t offset = (size_t)((char*)data - (char*)buffer);
	records += mdns_records_parse(saddr, buffer, data_size, &offset,
	                              MDNS_ENTRYTYPE_AUTHORITY, authority_rrs, 
	                              callback, user_data);
	records += mdns_records_parse(saddr, buffer, data_size, &offset,
	                              MDNS_ENTRYTYPE_ADDITIONAL, additional_rrs, 
	                              callback, user_data);

	return records;
}

static uint16_t mdns_transaction_id = 0;

static int
mdns_query_send(int sock, mdns_record_type_t type, const char* name, size_t length,
                void* buffer, size_t capacity) {
	if (capacity < (17 + length))
		return -1;

	uint16_t* data = buffer;
	//Transaction ID
	*data++ = htons(++mdns_transaction_id);
	//Flags
	*data++ = 0;
	//Questions
	*data++ = htons(1);
	//No answer, authority or additional RRs
	*data++ = 0;
	*data++ = 0;
	*data++ = 0;
	//Name string
	data = mdns_string_make(data, capacity - 17, name, length);
	if (!data)
		return -1;
	//Record type
	*data++ = htons(type);
	//! Unicast response, class IN
	*data++ = htons(0x8000U | MDNS_CLASS_IN);

	struct sockaddr_storage addr_storage;
	struct sockaddr_in addr;
	struct sockaddr_in6 addr6;
	struct sockaddr* saddr = (struct sockaddr*)&addr_storage;
	socklen_t saddrlen = sizeof(struct sockaddr_storage);
	if (getsockname(sock, saddr, &saddrlen))
		return -1;
	if (saddr->sa_family == AF_INET6) {
		memset(&addr6, 0, sizeof(struct sockaddr_in6));
		addr6.sin6_family = AF_INET6;
#ifdef __APPLE__
		addr6.sin6_len = sizeof(struct sockaddr_in6);
#endif
		addr6.sin6_addr.s6_addr[0] = 0xFF;
		addr6.sin6_addr.s6_addr[1] = 0x02;
		addr6.sin6_addr.s6_addr[15] = 0xFB;
		addr6.sin6_port = htons((unsigned short)5353);
		saddr = (struct sockaddr*)&addr6;
		saddrlen = sizeof(struct sockaddr_in6);
	}
	else {
		memset(&addr, 0, sizeof(struct sockaddr_in));
		addr.sin_family = AF_INET;
#ifdef __APPLE__
		addr.sin_len = sizeof(struct sockaddr_in);
#endif
		addr.sin_addr.s_addr = htonl((((uint32_t)224U) << 24U) | ((uint32_t)251U));
		addr.sin_port = htons((unsigned short)5353);
		saddr = (struct sockaddr*)&addr;
		saddrlen = sizeof(struct sockaddr_in);
	}

	if (sendto(sock, buffer, (char*)data - (char*)buffer, 0,
	           saddr, saddrlen) < 0)
		return -1;
	return 0;
}

static size_t
mdns_query_recv(int sock, void* buffer, size_t capacity,
                mdns_record_callback_fn callback, void* user_data,
                uint8_t one_shot) {
	struct sockaddr_in6 addr;
	struct sockaddr* saddr = (struct sockaddr*)&addr;
	memset(&addr, 0, sizeof(addr));
	saddr->sa_family = AF_INET;
#ifdef __APPLE__
	saddr->sa_len = sizeof(addr);
#endif
	socklen_t addrlen = sizeof(addr);
	int ret = recvfrom(sock, buffer, capacity, 0,
	                   saddr, &addrlen);
	if (ret <= 0)
		return 0;

	size_t data_size = (size_t)ret;
	uint16_t* data = (uint16_t*)buffer;

	uint16_t transaction_id = ntohs(*data++);
	++data;// uint16_t flags = ntohs(*data++);
	uint16_t questions      = ntohs(*data++);
	uint16_t answer_rrs     = ntohs(*data++);
	uint16_t authority_rrs  = ntohs(*data++);
	uint16_t additional_rrs = ntohs(*data++);

	if (one_shot && transaction_id != mdns_transaction_id)// || (flags != 0x8400))
		return 0; //Not a reply to our last question

	if (questions > 1)
		return 0;

	//Skip questions part
	int i;
	for (i = 0; i < questions; ++i) {
		size_t ofs = (size_t)((char*)data - (char*)buffer);
		if (!mdns_string_skip(buffer, data_size, &ofs))
			return 0;
		data = (void*)((char*)buffer + ofs);
		++data;
		++data;
	}

	size_t records = 0;
	size_t offset = (size_t)((char*)data - (char*)buffer);
	records += mdns_records_parse(saddr, buffer, data_size, &offset,
	                              MDNS_ENTRYTYPE_ANSWER, answer_rrs, 
	                              callback, user_data);
	records += mdns_records_parse(saddr, buffer, data_size, &offset,
	                              MDNS_ENTRYTYPE_AUTHORITY, authority_rrs, 
	                              callback, user_data);
	records += mdns_records_parse(saddr, buffer, data_size, &offset,
	                              MDNS_ENTRYTYPE_ADDITIONAL, additional_rrs, 
	                              callback, user_data);
	return records;
}

static mdns_string_t
mdns_record_parse_ptr(const void* buffer, size_t size, size_t offset, size_t length,
                      char* strbuffer, size_t capacity) {
	//PTR record is just a string
	if ((size >= offset + length) && (length >= 2))
		return mdns_string_extract(buffer, size, &offset, strbuffer, capacity);
	mdns_string_t empty = {0, 0};
	return empty;
}

static mdns_record_srv_t
mdns_record_parse_srv(const void* buffer, size_t size, size_t offset, size_t length,
                      char* strbuffer, size_t capacity) {
	mdns_record_srv_t srv;
	memset(&srv, 0, sizeof(mdns_record_srv_t));
	// Read the priority, weight, port number and the discovery name
	// SRV record format (http://www.ietf.org/rfc/rfc2782.txt):
	// 2 bytes network-order unsigned priority
	// 2 bytes network-order unsigned weight
	// 2 bytes network-order unsigned port
	// string: discovery (domain) name, minimum 2 bytes when compressed
	if ((size >= offset + length) && (length >= 8)) {
		const uint16_t* recorddata = (const uint16_t*)((const char*)buffer + offset);
		srv.priority = ntohs(*recorddata++);
		srv.weight = ntohs(*recorddata++);
		srv.port = ntohs(*recorddata++);
		offset += 6;
		srv.name = mdns_string_extract(buffer, size, &offset, strbuffer, capacity);
	}
	return srv;
}

static struct sockaddr_in*
mdns_record_parse_a(const void* buffer, size_t size, size_t offset, size_t length,
                    struct sockaddr_in* addr) {
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
#ifdef __APPLE__
	addr->sin_len = sizeof(struct sockaddr_in);
#endif
	if ((size >= offset + length) && (length == 4))
		addr->sin_addr.s_addr = *(const uint32_t*)((const char*)buffer + offset);
	return addr;
}

static struct sockaddr_in6*
mdns_record_parse_aaaa(const void* buffer, size_t size, size_t offset, size_t length,
                       struct sockaddr_in6* addr) {
	memset(addr, 0, sizeof(struct sockaddr_in6));
	addr->sin6_family = AF_INET6;
#ifdef __APPLE__
	addr->sin6_len = sizeof(struct sockaddr_in6);
#endif
	if ((size >= offset + length) && (length == 16))
		addr->sin6_addr = *(const struct in6_addr*)((const char*)buffer + offset);
	return addr;
}

static size_t
mdns_record_parse_txt(const void* buffer, size_t size, size_t offset, size_t length,
                      mdns_record_txt_t* records, size_t capacity) {
	size_t parsed = 0;
	const char* strdata;
	size_t separator, sublength;
	size_t end = offset + length;

	if (size < end)
		end = size;

	while ((offset < end) && (parsed < capacity)) {
		strdata = (const char*)buffer + offset;
		sublength = *(const unsigned char*)strdata;

		++strdata;
		offset += sublength + 1;

		separator = 0;
		for (size_t c = 0; c < sublength; ++c) {
			//DNS-SD TXT record keys MUST be printable US-ASCII, [0x20, 0x7E]
			if ((strdata[c] < 0x20) || (strdata[c] > 0x7E))
				break;
			if (strdata[c] == '=') {
				separator = c;
				break;
			}
		}

		if (!separator)
			continue;

		if (separator < sublength) {
			records[parsed].key.str = strdata;
			records[parsed].key.length = separator;
			records[parsed].value.str = strdata + separator + 1;
			records[parsed].value.length = sublength - (separator + 1);
		}
		else {
			records[parsed].key.str = strdata;
			records[parsed].key.length = sublength;
		}

		++parsed;
	}

	return parsed;	
}

#ifdef _WIN32
#undef strncasecmp
#endif
