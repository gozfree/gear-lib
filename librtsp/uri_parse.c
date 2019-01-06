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
#include "uri_parse.h"
#include <liblog.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

struct uri_component_t
{
    int has_scheme;
    int has_userinfo;
    int has_host;
    int has_port;
};

static uint32_t s_characters[8] = { 0x00, 0xAFFFFFFA, 0xAFFFFFFF, 0x47FFFFFE, 0x00, 0x00, 0x00, 0x00 };

static int uri_characters_check(uint8_t c)
{
    int n = c / 32;
    int m = c % 32;
    return s_characters[n] & (1 << m);
}

enum
{
    URI_PARSE_START,
    URI_PARSE_SCHEME,
    URI_PARSE_AUTHORITY,
    URI_PARSE_USERINFO,
    URI_PARSE_HOST,
    URI_PARSE_PORT,

    URI_PARSE_HOST_IPV6,
} state;


static int uri_check(const char* str, int len, struct uri_component_t* items)
{
    char c;
    const char* pend;
    state = URI_PARSE_START;
    items->has_scheme = 0;
    items->has_userinfo = 0;
    items->has_host = 0;
    items->has_port = 0;

    for (pend = str + len; str < pend; ++str) {
        c = *str;
        if (0 == uri_characters_check(c))
            return -1; // invalid character

        switch (state) {
        case URI_PARSE_START:
            switch (c){
            case '/':
                // path only, all done
                return 0; // ok
            case '[':
                state = URI_PARSE_HOST_IPV6;
                items->has_host = 1;
                break;

            default:
                state = URI_PARSE_SCHEME;
                items->has_host = 1;
                --str; // go back
                break;
            }
            break;
		case URI_PARSE_SCHEME:
            switch (c) {
            case ':':
                state = URI_PARSE_AUTHORITY;
                break;
            case '@':
                state = URI_PARSE_HOST;
                items->has_userinfo = 1;
                break;

            case '/':
            case '?':
            case '#':
                // all done, host only
                return 0;

            default:
                break;
            }
            break;
        case URI_PARSE_AUTHORITY:
            if ('/' == c) {
                if (str + 1 < pend && '/' == str[1]) {
                    state = URI_PARSE_HOST;
                    items->has_scheme = 1;
                    str += 1; // skip "//"
                } else {
                    items->has_port = 1;
                    return 0;
                }
            } else {
                items->has_port = 1;
                state = URI_PARSE_PORT;
            }
            break;
        case URI_PARSE_HOST:
            if (']' == c) {
                return -1;
            }
            switch (c) {
            case '@':
                items->has_userinfo = 1;
                //state = URI_PARSE_HOST;
                break;
            case '[':
                state = URI_PARSE_HOST_IPV6;
                break;
            case ':':
                items->has_port = 1;
                state = URI_PARSE_PORT;
                break;
            case '/':
            case '?':
            case '#':
                return 0;
            default:
                break;
            }
            break;
        case URI_PARSE_PORT:
            switch (c) {
            case '@':
                items->has_port = 0;
                items->has_userinfo = 1;
                state = URI_PARSE_HOST;
                break;
            case '[':
            case ']':
            case ':':
                return -1;
            case '/':
            case '?':
            case '#':
                items->has_port = 1;
                return 0;
            default:
                break;
            }
            break;
        case URI_PARSE_HOST_IPV6:
            switch (c) {
            case ']':
                state = URI_PARSE_HOST;
                break;
            case '@':
            case '[':
            case '/':
            case '?':
            case '#':
                return -1;
            default:
                break;
            }
            break;
        default:
            return -1;
        }
    }
    return 0;
}

int uri_parse(struct uri_t *uri, char* str, size_t len)
{
    if (NULL == str || 0 == *str || len < 1) {
        return -1;
    }
    char *p;
    const char* pend;
    struct uri_component_t items;

	if (0 != uri_check(str, len, &items))
		return -1; // invalid uri

	pend = str + len;
	p = (char*)(uri + 1);

	// scheme
	if (items.has_scheme)
	{
		uri->scheme = p;
		while (str < pend && ':' != *str)
			*p++ = *str++;
		*p++ = 0;
		str += 3; // skip "://"
	}
	else
	{
		uri->scheme = NULL;
	}

	// user info
	if (items.has_userinfo)
	{
		uri->userinfo = p;
		while (str < pend && '@' != *str)
			*p++ = *str++;
		*p++ = 0;
		str += 1; // skip "@"
	}
	else
	{
		uri->userinfo = NULL;
	}

	// host
	if (items.has_host)
	{
		uri->host = p;
		if (str >= pend) {
            return -1;
        }
		if ('[' == *str)
		{
			// IPv6
			++str;
			while (str < pend && ']' != *str)
				*p++ = *str++;
			*p++ = 0;
			str += 1; // skip "]"

			if (str < pend && *str && NULL == strchr(":/?#", *str)) {
				return -1;
            }
		}
		else
		{
			while (str < pend && *str && NULL == strchr(":/?#", *str))
				*p++ = *str++;
			*p++ = 0;
		}
	}
	else
	{
		uri->host = NULL;
	}

	// port
	if (items.has_port)
	{
		++str; // skip ':'
		for (uri->port = 0; str < pend && *str >= '0' && *str <= '9'; str++)
			uri->port = uri->port * 10 + (*str - '0');

		if (str < pend && *str && NULL == strchr(":/?#", *str))
			return -1;
	}
	else
	{
		uri->port = 0;
	}

	// 3.3. Path (p22)
	// The path is terminated by the first question mark ("?") 
	// or number sign ("#") character, 
	// or by the end of the URI.
	uri->path = p; // awayls have path(default '/')
	if (str < pend && '/' == *str)
	{
		while (str < pend && *str && '?' != *str && '#' != *str)
			*p++ = *str++;
		*p++ = 0;
	}
	else
	{
		// default path
		*p++ = '/';
		*p++ = 0;
	}

	// 3.4. Query (p23)
	// The query component is indicated by the first question mark ("?") character 
	// and terminated by a number sign ("#") character
    // or by the end of the URI.
	if (str < pend && '?' == *str)
	{
		uri->query = p;
		for (++str; str < pend && *str && '#' != *str; ++str)
			*p++ = *str;
		*p++ = 0;
	}
	else
	{
		uri->query = NULL;
	}

	// 3.5. Fragment
	if (str < pend && '#' == *str)
	{
		uri->fragment = p;
		while (str < pend && *++str)
			*p++ = *str;
		*p++ = 0;
	}
	else
	{
		uri->fragment = NULL;
	}

	return 0;
}

int uri_query(const char* query, const char* end, struct uri_query_t** items)
{
#define N 64
	int count;
	int capacity;
	const char *p;
	struct uri_query_t items0[N], *pp;

	if (!items) {
        return -1;
    }
	*items = NULL;
	capacity = count = 0;

	for (p = query; p && p < end; query = p + 1)
	{
		p = strpbrk(query, "&=");
		if (!(!p || *p)) {
            return -1;
        }
		if (!p || p > end)
			break;

		if (p == query)
		{
			if ('&' == *p)
			{
				continue; // skip k1=v1&&k2=v2
			}
			else
			{
				uri_query_free(items);
				return -1;  // no-name k1=v1&=v2
			}
		}

		if (count < N)
		{
			pp = &items0[count++];
		}
		else
		{
			if (count >= capacity)
			{
				capacity = count + 64;
				pp = (struct uri_query_t*)realloc(*items, capacity * sizeof(struct uri_query_t));
				if (!pp) return -ENOMEM;
				*items = pp;
			}

			pp = &(*items)[count++];
		}

		pp->name = query;
		pp->n_name = p - query;

		if ('=' == *p)
		{
			pp->value = p + 1;
			p = strchr(pp->value, '&');
			if (NULL == p) p = end;
			pp->n_value = p - pp->value; // empty-value
		}
		else
		{
			if ('&' != *p) {
                return -1;
            }
			pp->value = NULL;
			pp->n_value = 0; // no-value
		}
	}

	if (count <= N && count > 0)
	{
		*items = (struct uri_query_t*)malloc(count * sizeof(struct uri_query_t));
		if (!*items) return -ENOMEM;
		memcpy(*items, items0, count * sizeof(struct uri_query_t));
	}
	else if(count > N)
	{
		memcpy(*items, items0, N * sizeof(struct uri_query_t));
	}

	return count;
}

void uri_query_free(struct uri_query_t** items)
{
	if (items && *items)
	{
		free(*items);
		*items = NULL;
	}
}

#define char_isnum(c)	('0'<=(c) && '9'>=(c))
#define char_isalpha(c) (('a'<=(c) && 'z'>=(c)) || ('A'<=(c) && 'Z'>=(c)))
#define char_isalnum(c) (char_isnum(c) || char_isalpha(c))
#define char_ishex(c)	(char_isnum(c) || ('a'<=(c)&&'f'>=(c)) || ('A'<=(c) && 'F'>=(c)))

static char s_encode[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

char ToHex(char c)
{
	return char_isnum(c) ? c-'0' : (char)tolower(c)-'a'+10;
}

int url_encode(const char* source, int srcBytes, char* target, int tgtBytes)
{
	int i, r;
	const char* p = source;

	r = 0;
	for(i=0; *p && (-1==srcBytes || p<source+srcBytes) && i<tgtBytes; ++p,++i)
	{
		if(char_isalnum(*p) || '-'==*p || '_'==*p || '.'==*p || '~'==*p)
		{
			target[i] = *p;
		}
		else if(' ' == *p)
		{
			target[i] = '+';
		}
		else
		{
			if(i+2 >= tgtBytes)
			{
				r = -1;
				break;
			}

			target[i] = '%';
			target[++i] = s_encode[(*p>>4) & 0xF];
			target[++i] = s_encode[*p & 0xF];
		}
	}

	if(i < tgtBytes)
		target[i] = '\0';
	return r;
}

int url_decode(const char* source, int srcBytes, char* target, int tgtBytes)
{
	int i, r;
	const char* p = source;

	r = 0;
	for(i=0; *p && (-1==srcBytes || p<source+srcBytes) && i<tgtBytes; ++p,++i)
	{
		if('+' == *p)
		{
			target[i] = ' ';
		}
		else if('%' == *p)
		{
			if(!char_ishex(p[1]) || !char_ishex(p[2]) || (-1!=srcBytes && p+2>=source+srcBytes))
			{
				r = -1;
				break;
			}
			
			target[i] = (ToHex(p[1])<<4) | ToHex(p[2]);
			p += 2;
		}
		else
		{
			target[i] = *p;
		}
	}

	if(i < tgtBytes)
		target[i] = 0;
	return r;
}
