/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/
#include "rtsp_parser.h"
#include "rtp.h"
#include <liblog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>

static void url_decode(char* url)
{
    // Replace (in place) any %<hex><hex> sequences with the appropriate 8-bit character.
    char* cursor = url;
    while (*cursor) {
        if ((cursor[0] == '%') &&
             cursor[1] && isxdigit(cursor[1]) &&
             cursor[2] && isxdigit(cursor[2])) {
            // We saw a % followed by 2 hex digits, so we copy the literal hex value into the URL, then advance the cursor past it:
            char hex[3];
            hex[0] = cursor[1];
            hex[1] = cursor[2];
            hex[2] = '\0';
            *url++ = (char)strtol(hex, NULL, 16);
            cursor += 3;
        } else {
            // Common case: This is a normal character or a bogus % expression, so just copy it
            *url++ = *cursor++;
        }
    }
    *url = '\0';
}

int parse_rtsp_request(struct rtsp_request *req)
{
    char *str = (char *)req->raw->iov_base;
    int len = req->raw->iov_len;
    int cmd_len = sizeof(req->cmd);
    int url_prefix_len = sizeof(req->url_prefix);
    int url_suffix_len = sizeof(req->url_suffix);
    int cseq_len = sizeof(req->cseq);
    int session_id_len = sizeof(req->session.id);
    int cur;
    char c;
    const char *rtsp_prefix = "rtsp://";
    int rtsp_prefix_len = sizeof(rtsp_prefix);

    logd("rtsp request[%d]:\n>>>>>>>>\n%s\n", len, str);

    // "Be liberal in what you accept": Skip over any whitespace at the start of the request:
    for (cur = 0; cur < len; ++cur) {
        c = str[cur];
        if (!(c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\0')) {
            break;
        }
    }
    if (cur == len) {
        loge("request consisted of nothing but whitespace!\n");
        return -1;
    }

    // Then read everything up to the next space (or tab) as the command name:
    int parse_succeed = 0;
    int i;
    for (i = 0; i < cmd_len - 1 && i < len; ++cur, ++i) {
        c = str[cur];
        if (c == ' ' || c == '\t') {
            parse_succeed = 1;
            break;
        }
        req->cmd[i] = c;
    }
    req->cmd[i] = '\0';
    if (!parse_succeed) {
        loge("can't find request command name\n");
        return -1;
    }

    // Skip over the prefix of any "rtsp://" or "rtsp:/" URL that follows:
    int j = cur+1;
    while (j < len && (str[j] == ' ' || str[j] == '\t')) {
        ++j; // skip over any additional white space
    }
    for (; j < (len - rtsp_prefix_len); ++j) {
        if ((str[j] == 'r' || str[j] == 'R') &&
            (str[j+1] == 't' || str[j+1] == 'T') &&
            (str[j+2] == 's' || str[j+2] == 'S') &&
            (str[j+3] == 'p' || str[j+3] == 'P') &&
             str[j+4] == ':' && str[j+5] == '/') {
            j += 6;
            if (str[j] == '/') {
                // This is a "rtsp://" URL; skip over the host:port part that follows:
                ++j;
                while (j < len && str[j] != '/' && str[j] != ' ') ++j;
            } else {
                // This is a "rtsp:/" URL; back up to the "/":
                --j;
            }
            cur = j;
            break;
        }
    }

    // Look for the URL suffix (before the following "RTSP/"):
    parse_succeed = 0;
    int k;
    for (k = cur+1; k < (len-5); ++k) {
        if (str[k] == 'R' && str[k+1] == 'T' &&
            str[k+2] == 'S' && str[k+3] == 'P' && str[k+4] == '/') {
            while (--k >= cur && str[k] == ' ') {} // go back over all spaces before "RTSP/"
            int k1 = k;
            while (k1 > cur && str[k1] != '/') --k1;
            // ASSERT: At this point
            //   cur: first space or slash after "host" or "host:port"
            //   k: last non-space before "RTSP/"
            //   k1: last slash in the range [cur,k]
            // The URL suffix comes from [k1+1,k]
            // Copy "url_suffix":
            int n = 0, k2 = k1+1;
            if (k2 <= k) {
                if (k - k1 + 1 > url_suffix_len) return -1; // there's no room
                while (k2 <= k) req->url_suffix[n++] = str[k2++];
            }
            req->url_suffix[n] = '\0';
            // The URL 'pre-suffix' comes from [cur+1,k1-1]
            // Copy "url_prefix":
            n = 0; k2 = cur+1;
            if (k2+1 <= k1) {
                if (k1 - cur > url_prefix_len) return -1; // there's no room
                while (k2 <= k1-1) req->url_prefix[n++] = str[k2++];
            }
            req->url_prefix[n] = '\0';
            url_decode(req->url_prefix);
            cur = k + 7; // to go past " RTSP/"
            parse_succeed = 1;
            break;
        }
    }
    if (!parse_succeed) {
        return -1;
    }
    int org = 0;
    for (org = 0; org < k-7; org++) {
        req->url_origin[org] = str[i+org+1];
    }

    // Look for "CSeq:" (mandatory, case insensitive), skip whitespace,
    // then read everything up to the next \r or \n as 'CSeq':
    parse_succeed = 0;
    int n;
    for (j = cur; j < (len-5); ++j) {
        if (strncasecmp("CSeq:", &str[j], 5) == 0) {
            j += 5;
            while (j < len && (str[j] ==  ' ' || str[j] == '\t')) ++j;
            for (n = 0; n < cseq_len - 1 && j < len; ++n,++j) {
                c = str[j];
                if (c == '\r' || c == '\n') {
                  parse_succeed = 1;
                  break;
                }
                req->cseq[n] = c;
            }
            req->cseq[n] = '\0';
            break;
        }
    }
    if (!parse_succeed) {
        return -1;
    }

    // Look for "Session:" (optional, case insensitive), skip whitespace,
    // then read everything up to the next \r or \n as 'Session':
    req->session.id[0] = '\0'; // default value (empty string)
    for (j = cur; j < (len-8); ++j) {
        if (strncasecmp("Session:", &str[j], 8) == 0) {
            j += 8;
            while (j < len && (str[j] ==  ' ' || str[j] == '\t')) ++j;
            for (n = 0; n < session_id_len - 1 && j < len; ++n,++j) {
                c = str[j];
                if (c == '\r' || c == '\n') {
                    break;
                }
                req->session.id[n] = c;
            }
            req->session.id[n] = '\0';
            req->session.timeout = 60000;
            char *p = strchr(&str[j], ';');
            if (p && 0 == strncmp("timeout=", p+1, 8)) {
                req->session.timeout = (int)(atof(p+9) * 1000);
            }
            break;
        }
    }

    // Also: Look for "Content-Length:" (optional, case insensitive)
    req->content_len = 0; // default value
    for (j = cur; (int)j < (int)(len-15); ++j) {
        if (strncasecmp("Content-Length:", &(str[j]), 15) == 0) {
            j += 15;
            while (j < len && (str[j] ==  ' ' || str[j] == '\t')) ++j;
            int num;
            if (sscanf(&str[j], "%u", &num) == 1) {
                req->content_len = num;
            }
        }
    }
    return 0;
}

int parse_transport(struct transport_header *t, char *buf, int len)
{
    // First, find "Transport:"
    while (1) {
        if (*buf == '\0')
            return -1; // not found
        if (*buf == '\r' && *(buf+1) == '\n' && *(buf+2) == '\r')
            return -1; // end of the headers => not found
        if (strncasecmp(buf, "Transport:", 10) == 0)
            break;
        ++buf;
    }

    // Then, run through each of the fields, looking for ones we handle:
    char const* fields = buf + 10;
    while (*fields == ' ') ++fields;
    char* field = (char *)calloc(1, strlen(fields)+1);
    while (sscanf(fields, "%[^;\r\n]", field) == 1) {
        switch (*field) {
        case 'r':
        case 'R':
            if (strncasecmp(field, "RTP/AVP/TCP", 11) == 0) {
                t->mode = RTP_TCP;
            } else if (strncasecmp(field, "RTP/AVP/UDP", 11) == 0 ||
                       strncasecmp(field, "RTP/AVP", 7) == 0) {
                t->mode = RTP_UDP;
            } else if (strncasecmp(field, "RAW/RAW/UDP", 11) == 0) {
                t->mode = RAW_UDP;
            }
            break;
        case 'u':
        case 'U':
            if (strncasecmp(field, "unicast", 7) == 0) {
                t->multicast = 0;
            }
            break;
        case 'd':
        case 'D':
            if (strncasecmp(field, "destination=", 12) == 0) {
                strcpy(t->destination, field+12);
            }
            break;
        case 'm':
        case 'M':
            if (strncasecmp(field, "multicast", 9) == 0) {
                t->multicast = 1;
            } else if (strncasecmp(field, "mode=", 5) == 0) {
                if (strcasecmp(field+5, "\"PLAY\"") == 0 ||
                    strcasecmp(field+5, "PLAY") == 0) {
                    //XXX
                } else if (strcasecmp(field+5, "\"RECORD\"") == 0 ||
                          strcasecmp(field+5, "RECORD") == 0) {
                    //XXX
                }
            }
            break;
        case 's':
        case 'S':
            if (strncasecmp(field, "source=", 7) == 0) {
                strcpy(t->source, field+7);
            } else if (strncasecmp(field, "ssrc=", 5) == 0) {
                if (t->multicast) {
                    loge("must be unicast!\n");
                    return -1;
                }
                t->rtp.u.ssrc = (int)strtol(field+5, NULL, 16);
            } else if (2 == sscanf(field, "server_port=%hu-%hu", &t->rtp.u.server_port1, &t->rtp.u.server_port2)) {
                if (t->multicast) {
                    loge("must be unicast!\n");
                    return -1;
                }
            } else if (1 == sscanf(field, "server_port=%hu", &t->rtp.u.server_port1)) {
                if (t->multicast) {
                    loge("must be unicast!\n");
                    return -1;
                }
                t->rtp.u.server_port1 = t->rtp.u.server_port1 / 2 * 2;
                t->rtp.u.server_port2 = t->rtp.u.server_port1 + 1;
            }
            break;
        case 'a':
            if (strcasecmp(field, "append") == 0) {
                t->append = 1;
            }
            break;
        case 'p':
            if (2 == sscanf(field, "port=%hu-%hu", &t->rtp.m.port1, &t->rtp.m.port2)) {
                if (!t->multicast) {
                    loge("must be multicast!\n");
                    return -1;
                }
            } else if (1 == sscanf(field, "port=%hu", &t->rtp.m.port1)) {
                if (!t->multicast) {
                    loge("must be multicast!\n");
                    return -1;
                }
                t->rtp.m.port1 = t->rtp.m.port1 / 2 * 2;
                t->rtp.m.port2 = t->rtp.m.port1 + 1;
            }
            break;
        case 'c':
            if (2 == sscanf(field, "client_port=%hu-%hu", &t->rtp.u.client_port1, &t->rtp.u.client_port2)) {
                if (t->multicast) {
                    loge("must be unicast!\n");
                    return -1;
                }
            } else if(1 == sscanf(field, "client_port=%hu", &t->rtp.u.client_port1)) {
                if (t->multicast) {
                    loge("must be unicast!\n");
                    return -1;
                }
                t->rtp.u.client_port1 = t->rtp.u.client_port1 / 2 * 2;
                t->rtp.u.client_port2 = t->rtp.u.client_port1 + 1;
            }
            break;
        case 'i':
            if (2 == sscanf(field, "interleaved=%hu-%hu", &t->interleaved1, &t->interleaved2)) {
            } else if(1 == sscanf(field, "interleaved=%hu", &t->interleaved1)) {
                t->interleaved2 = t->interleaved1 + 1;
            }
            break;
        case 't':
            if (1 == sscanf(field, "ttl=%d", &t->rtp.m.ttl)) {
                if (t->multicast) {
                    loge("must be unicast!\n");
                    return -1;
                }
            }
            break;
        case 'l':
            if(1 == sscanf(field, "layers=%d", &t->layer)) {
            }
            break;
        default:
            break;
        }
        fields += strlen(field);
        while (*fields == ';' || *fields == ' ' || *fields == '\t') ++fields; // skip over separating ';' chars or whitespace
        if (*fields == '\0' || *fields == '\r' || *fields == '\n') break;
    }
    free(field);
    logd("client_port1 = %d, client_port2 = %d\n", t->rtp.u.client_port1, t->rtp.u.client_port2);
    return 0;
}

int parse_session(struct session_header *s, char *buf, int len)
{
    int found = 0;
    while (1) {
        if (*buf == '\0') {
            break;
        }
        if (*buf == '\r' && *(buf+1) == '\n' && *(buf+2) == '\r') {
            break;
        }
        if (strncasecmp(buf, "Session:", 8) == 0) {
            found = 1;
            break;
        }
        ++buf;
    }
    if (!found) {
        return 0;
    }
    s->timeout = 60000;
    const char* p;
    p = strchr(buf, ';');
    if (p) {
        size_t n = (size_t)(p - buf);
        if (n >= sizeof(s->id))
            return -1;

        memcpy(s->id, buf, n);
        s->id[n] = '\0';

        if (0 == strncmp("timeout=", p+1, 8))
            s->timeout = (int)(atof(p+9) * 1000);
    } else {
        size_t n = strlen(buf);
        if (n >= sizeof(s->id))
            return -1;
        memcpy(s->id, buf, n);
        s->id[n] = '\0';
    }
    return 0;
}

const char* http_reason_phrase(int code)
{
    static const char *reason1xx[] = 
    {
        "Continue", // 100
        "Switching Protocols" // 101
    };

    static const char *reason2xx[] = 
    {
        "OK", // 200
        "Created", // 201
        "Accepted", // 202
        "Non-Authoritative Information", // 203
        "No Content", // 204
        "Reset Content", // 205
        "Partial Content" // 206
    };

    static const char *reason3xx[] = 
    {
        "Multiple Choices", // 300
        "Move Permanently", // 301
        "Found", // 302
        "See Other", // 303
        "Not Modified", // 304
        "Use Proxy", // 305
        "Unused", // 306
        "Temporary Redirect" // 307
    };

    static const char *reason4xx[] = 
    {
        "Bad Request", // 400
        "Unauthorized", // 401
        "Payment Required", // 402
        "Forbidden", // 403
        "Not Found", // 404
        "Method Not Allowed", // 405
        "Not Acceptable", // 406
        "Proxy Authentication Required", // 407
        "Request Timeout", // 408
        "Conflict", // 409
        "Gone", // 410
        "Length Required", //411
        "Precondition Failed", // 412
        "Request Entity Too Large", // 413
        "Request-URI Too Long", // 414
        "Unsupported Media Type", // 415
        "Request Range Not Satisfiable", // 416
        "Expectation Failed" // 417
    };

    static const char *reason5xx[] = 
    {
        "Internal Server Error", // 500
        "Not Implemented", // 501
        "Bad Gateway", // 502
        "Service Unavailable", // 503
        "Gateway Timeout", // 504
        "HTTP Version Not Supported" // 505
    };

    if (code >= 100 && code < 100+sizeof(reason1xx)/sizeof(reason1xx[0]))
        return reason1xx[code-100];
    else if (code >= 200 && code < 200+sizeof(reason2xx)/sizeof(reason2xx[0]))
        return reason2xx[code-200];
    else if (code >= 300 && code < 300+sizeof(reason3xx)/sizeof(reason3xx[0]))
        return reason3xx[code-300];
    else if (code >= 400 && code < 400+sizeof(reason4xx)/sizeof(reason4xx[0]))
        return reason4xx[code-400];
    else if (code >= 500 && code < 500+sizeof(reason5xx)/sizeof(reason5xx[0]))
        return reason5xx[code-500];
    else
        return "unknown";
}

const char* rtsp_reason_phrase(int code)
{
    static const char *reason45x[] = 
    {
        "Parameter Not Understood", // 451
        "Conference Not Found", // 452
        "Not Enough Bandwidth", // 453
        "Session Not Found", // 454
        "Method Not Valid in This State", // 455
        "Header Field Not Valid for Resource", // 456
        "Invalid Range", // 457
        "Parameter Is Read-Only", // 458
        "Aggregate Operation Not Allowed", // 459
        "Only Aggregate Operation Allowed", // 460
        "Unsupported Transport", // 461
        "Destination Unreachable", // 462
    };

    if (451 <= code && code < 451+sizeof(reason45x)/sizeof(reason45x[0]))
        return reason45x[code-451];

    switch(code) {
    case 505:
        return "RTSP Version Not Supported";
    case 551:
        return "Option not supported";
    default:
        return http_reason_phrase(code);
    }
}

struct time_smpte_t
{
    int second;		// [0,24*60*60)
    int frame;		// [0,99]
    int subframe;	// [0,99]
};

struct time_npt_t
{
	int64_t second;	// [0,---)
	int fraction;	// [0,999]
};

struct time_clock_t
{
	int second;		// [0,24*60*60)
	int fraction;	// [0,999]
	int day;		// [0,30]
	int month;		// [0,11]
	int year;		// [xxxx]
};

#define RANGE_SPECIAL ",;\r\n"

static uint64_t utc_mktime(const struct tm *t)
{
    int mon = t->tm_mon+1, year = t->tm_year+1900;

    /* 1..12 -> 11,12,1..10 */
    if (0 >= (int) (mon -= 2)) {
        mon += 12;  /* Puts Feb last since it has leap day */
        year -= 1;
    }
    
    return ((((uint64_t)
              (year/4 - year/100 + year/400 + 367*mon/12 + t->tm_mday) +
              year*365 - 719499
              )*24 + t->tm_hour /* now have hours */
             )*60 + t->tm_min /* now have minutes */
            )*60 + t->tm_sec; /* finally seconds */
}

static inline const char* string_token_int(const char* str, int *value)
{
	*value = 0;
	while ('0' <= *str && *str <= '9')
	{
		*value = (*value * 10) + (*str - '0');
		++str;
	}
	return str;
}

static inline const char* string_token_int64(const char* str, int64_t *value)
{
	*value = 0;
	while ('0' <= *str && *str <= '9')
	{
		*value = (*value * 10) + (*str - '0');
		++str;
	}
	return str;
}

// smpte-time = 1*2DIGIT ":" 1*2DIGIT ":" 1*2DIGIT [ ":" 1*2DIGIT ][ "." 1*2DIGIT ]
// hours:minutes:seconds:frames.subframes
static const char* rtsp_header_range_smpte_time(const char* str, int *hours, int *minutes, int *seconds, int *frames, int *subframes)
{
	const char* p;

	assert(str);
	p = string_token_int(str, hours);
	if(*p != ':')
		return NULL;

	p = string_token_int(p+1, minutes);
	if(*p != ':')
		return NULL;

	p = string_token_int(p+1, seconds);

	*frames = 0;
	*subframes = 0;
	if(*p == ':')
	{
		p = string_token_int(p+1, frames);
	}

	if(*p == '.')
	{
		p = string_token_int(p+1, subframes);
	}

	return p;
}

// 3.5 SMPTE Relative Timestamps (p16)
// smpte-range = smpte-type "=" smpte-time "-" [ smpte-time ]
// smpte-type = "smpte" | "smpte-30-drop" | "smpte-25"
// Examples:
//	smpte=10:12:33:20-
//	smpte=10:07:33-
//	smpte=10:07:00-10:07:33:05.01
//	smpte-25=10:07:00-10:07:33:05.01
static int rtsp_header_range_smpte(const char* fields, struct range_header* range)
{
	const char *p;
	int hours, minutes, seconds, frames, subframes;

	assert(fields);
	p = rtsp_header_range_smpte_time(fields, &hours, &minutes, &seconds, &frames, &subframes);
	if(!p || '-' != *p)
		return -1;

	range->from_value = RTSP_RANGE_TIME_NORMAL;
	range->from = (hours%24)*3600 + (minutes%60)*60 + seconds;
	range->from = range->from * 1000 + frames % 1000;

	assert('-' == *p);
	if('\0' == p[1] || strchr(RANGE_SPECIAL, p[1]))
	{
		range->to_value = RTSP_RANGE_TIME_NOVALUE;
		range->to = 0;
	}
	else
	{
		p = rtsp_header_range_smpte_time(p+1, &hours, &minutes, &seconds, &frames, &subframes);
		if(!p ) return -1;
		assert('\0' == p[0] || strchr(RANGE_SPECIAL, p[0]));

		range->to_value = RTSP_RANGE_TIME_NORMAL;
		range->to = (hours%24)*3600 + (minutes%60)*60 + seconds;
		range->to = range->to * 1000 + frames % 1000;
	}

	return 0;
}

// npt-time = "now" | npt-sec | npt-hhmmss
// npt-sec = 1*DIGIT [ "." *DIGIT ]
// npt-hhmmss = npt-hh ":" npt-mm ":" npt-ss [ "." *DIGIT ]
// npt-hh = 1*DIGIT ; any positive number
// npt-mm = 1*2DIGIT ; 0-59
// npt-ss = 1*2DIGIT ; 0-59
static const char* rtsp_header_range_npt_time(const char* str, uint64_t *seconds, int *fraction)
{
	const char* p;
	int v1, v2;

	assert(str);
	p = strpbrk(str, "-\r\n");
	if(!str || (p-str==3 && 0==strncasecmp(str, "now", 3)))
	{
		*seconds = 0; // now
		*fraction = -1;
	}
	else
	{
		p = string_token_int64(str, (int64_t*)seconds);
		if(*p == ':')
		{
			// npt-hhmmss
			p = string_token_int(p+1, &v1);
			if(*p != ':')
				return NULL;

			p = string_token_int(p+1, &v2);

			assert(0 <= v1 && v1 < 60);
			assert(0 <= v2 && v2 < 60);
			*seconds = *seconds * 3600 + (v1%60)*60 + v2%60;
		}
		else
		{
			// npt-sec
			//*seconds = hours;
		}

		if(*p == '.')
			p = string_token_int(p+1, fraction);
		else
			*fraction = 0;
	}

	return p;
}

// 3.6 Normal Play Time (p17)
// npt-range = ( npt-time "-" [ npt-time ] ) | ( "-" npt-time )
// Examples:
//	npt=123.45-125
//	npt=12:05:35.3-
//	npt=now-
static int rtsp_header_range_npt(const char* fields, struct range_header* range)
{
	const char* p;
	uint64_t seconds;
	int fraction;

	p = fields;
	if('-' != *p)
	{
		p = rtsp_header_range_npt_time(p, &seconds, &fraction);
		if(!p || '-' != *p)
			return -1;

		if(0 == seconds && -1 == fraction)
		{
			range->from_value = RTSP_RANGE_TIME_NOW;
			range->from = 0L;
		}
		else
		{
			range->from_value = RTSP_RANGE_TIME_NORMAL;
			range->from = seconds;
			range->from = range->from * 1000 + fraction % 1000;
		}
	}
	else
	{
		range->from_value = RTSP_RANGE_TIME_NOVALUE;
		range->from = 0;
	}

	assert('-' == *p);
	if('\0' == p[1] || strchr(RANGE_SPECIAL, p[1]))
	{
		assert('-' != *fields);
		range->to_value = RTSP_RANGE_TIME_NOVALUE;
		range->to = 0;
	}
	else
	{
		p = rtsp_header_range_npt_time(p+1, &seconds, &fraction);
		if( !p ) return -1;
		assert('\0' == p[0] || strchr(RANGE_SPECIAL, p[0]));

		range->to_value = RTSP_RANGE_TIME_NORMAL;
		range->to = seconds;
		range->to = range->to * 1000 + fraction % 1000;
	}

	return 0;
}

// utc-time = utc-date "T" utc-time "Z"
// utc-date = 8DIGIT ; < YYYYMMDD >
// utc-time = 6DIGIT [ "." fraction ] ; < HHMMSS.fraction >
static const char* rtsp_header_range_clock_time(const char* str, uint64_t *second, int *fraction)
{
	struct tm t;
	const char* p;
	int year, month, day;
	int hour, minute;

	assert(str);
	if(!str || 5 != sscanf(str, "%4d%2d%2dT%2d%2d", &year, &month, &day, &hour, &minute))
		return NULL;

	*second = 0;
	*fraction = 0;
	p = string_token_int64(str + 13, (int64_t*)second);
    assert(p);
	if(*p == '.')
	{
		p = string_token_int(p+1, fraction);
	}

	memset(&t, 0, sizeof(t));
	t.tm_year = year - 1900;
	t.tm_mon = month - 1;
	t.tm_mday = day;
	t.tm_hour = hour;
	t.tm_min = minute;
//	*second += mktime(&t);
    *second += utc_mktime(&t);

//	assert('Z' == *p);
//	assert('\0' == p[1] || strchr(RANGE_SPECIAL"-", p[1]));
	return 'Z'==*p ? p+1 : p;
}

// 3.7 Absolute Time (p18)
// utc-range = "clock" "=" utc-time "-" [ utc-time ]
// Range: clock=19961108T143720.25Z-
// Range: clock=19961110T1925-19961110T2015 (p72)
static int rtsp_header_range_clock(const char* fields, struct range_header* range)
{
	const char* p;
	uint64_t second;
	int fraction;

	p = rtsp_header_range_clock_time(fields, &second, &fraction);
	if(!p || '-' != *p)
		return -1;

	range->from_value = RTSP_RANGE_TIME_NORMAL;
	range->from = second * 1000;
	range->from += fraction % 1000;

	assert('-' == *p);
	if('\0'==p[1] || strchr(RANGE_SPECIAL, p[1]))
	{
		range->to_value = RTSP_RANGE_TIME_NOVALUE;
		range->to = 0;
	}
	else
	{
		p = rtsp_header_range_clock_time(p+1, &second, &fraction);
		if( !p ) return -1;

		range->to_value = RTSP_RANGE_TIME_NORMAL;
		range->to = second * 1000;
		range->to += (unsigned int)fraction % 1000;
	}

	return 0;
}

int parse_range(struct range_header *range, char* buf, int len)
{
    int r = 0;
    int found = 0;
    while (1) {
        if (*buf == '\0') {
            break;
        }
        if (*buf == '\r' && *(buf+1) == '\n' && *(buf+2) == '\r') {
            break;
        }
        if (strncasecmp(buf, "range", 5) == 0) {
            found = 1;
            break;
        }
        ++buf;
    }
    if (!found) {
        return 0;
    }
    char *field = buf;
	range->time = 0L;
	while(field && 0 == r)
	{
		if(0 == strncasecmp("clock=", field, 6))
		{
			range->type = RTSP_RANGE_CLOCK;
			r = rtsp_header_range_clock(field+6, range);
		}
		else if(0 == strncasecmp("npt=", field, 4))
		{
			range->type = RTSP_RANGE_NPT;
			r = rtsp_header_range_npt(field+4, range);
		}
		else if(0 == strncasecmp("smpte=", field, 6))
		{
			range->type = RTSP_RANGE_SMPTE;
			r = rtsp_header_range_smpte(field+6, range);
			if(RTSP_RANGE_TIME_NORMAL == range->from_value)
				range->from = (range->from/1000 * 1000) + (1000/30 * (range->from%1000)); // frame to ms
			if(RTSP_RANGE_TIME_NORMAL == range->to_value)
				range->to = (range->to/1000 * 1000) + (1000/30 * (range->to%1000)); // frame to ms
		}
		else if(0 == strncasecmp("smpte-30-drop=", field, 15))
		{
			range->type = RTSP_RANGE_SMPTE_30;
			r = rtsp_header_range_smpte(field+15, range);
			if(RTSP_RANGE_TIME_NORMAL == range->from_value)
				range->from = (range->from/1000 * 1000) + (1000/30 * (range->from%1000)); // frame to ms
			if(RTSP_RANGE_TIME_NORMAL == range->to_value)
				range->to = (range->to/1000 * 1000) + (1000/30 * (range->to%1000)); // frame to ms
		}
		else if(0 == strncasecmp("smpte-25=", field, 9))
		{
			range->type = RTSP_RANGE_SMPTE_25;
			r = rtsp_header_range_smpte(field+9, range);
			if(RTSP_RANGE_TIME_NORMAL == range->from_value)
				range->from = (range->from/1000 * 1000) + (1000/25 * (range->from%1000)); // frame to ms
			if(RTSP_RANGE_TIME_NORMAL == range->to_value)
				range->to = (range->to/1000 * 1000) + (1000/25 * (range->to%1000)); // frame to ms
		}
		else if(0 == strncasecmp("time=", field, 5))
		{
			if (rtsp_header_range_clock_time(field + 5, &range->time, &r))
				range->time = range->time * 1000 + r % 1000;
		}
		
		field = strchr(field, ';');
		if(field)
			++field;
	}

	return r;
}
