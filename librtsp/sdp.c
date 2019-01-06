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

#include "sdp.h"
#include <liblog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SDP_TOOL_NAME		((const char *)"ipcam rtsp")
#define SDP_TOOL_VERSION	((const char *)"version 2016.05.22")
#define SDP_FILTER_FMT      ((const char *)     \
        "a=source-filter: incl IN IP4 * %s\r\n" \
        "a=rtcp-unicast: reflection\r\n")

#define RTSP_SERVER_IP      ((const char *)"127.0.0.1")

enum { SDP_M_MEDIA_UNKOWN=0, SDP_M_MEDIA_AUDIO, SDP_M_MEDIA_VIDEO, SDP_M_MEDIA_TEXT, SDP_M_MEDIA_APPLICATION, SDP_M_MEDIA_MESSAGE };
enum { SDP_M_PROTO_UKNOWN=0, SDP_M_PROTO_UDP, SDP_M_PROTO_RTP_AVP, SDP_M_PROTO_RTP_SAVP };

#define N_EMAIL 1
#define N_PHONE 1
#define N_CONNECTION 1
#define N_BANDWIDTH 1
#define N_TIMING 1
#define N_REPEAT 1
#define N_TIMEZONE 1
#define N_REPEAT_OFFSET 1
#define N_ATTRIBUTE 5
#define N_MEDIA 3 // audio/video/whiteboard
#define N_MEDIA_FORMAT 5

struct sdp_connection
{
	char* network;
	char* addrtype;
	char* address;
};

struct sdp_origin
{
	char* username;
	char* session;
	char* session_version;
	struct sdp_connection c;
};

struct sdp_email
{
	char* email;
};

struct sdp_phone
{
	char* phone;
};

struct sdp_bandwidth
{
	char* bwtype;
	char* bandwidth;
};

struct bandwidths
{
	size_t count;
	struct sdp_bandwidth bandwidths[N_BANDWIDTH];
	struct sdp_bandwidth *ptr;
	size_t capacity;
};

struct sdp_repeat
{
	char* interval;
	char* duration;

	struct offset
	{
		size_t count;
		char *offsets[N_REPEAT_OFFSET];
		char **ptr;
		size_t capacity;
	} offsets;
};

struct sdp_timezone
{
	char* time;
	char* offset;
};

struct sdp_timing
{
	char* start;
	char* stop;

	struct repeat
	{
		size_t count;
		struct sdp_repeat repeats[N_REPEAT];
		struct sdp_repeat *ptr;
		size_t capacity;
	} r;

	struct timezone_t
	{
		size_t count;
		struct sdp_timezone timezones[N_TIMEZONE];
		struct sdp_timezone *ptr;
		size_t capacity;
	} z;
};

struct sdp_encryption
{
	char* method;
	char* key;
};

struct sdp_attribute
{
	char* name;
	char* value;
};

struct attributes
{
	size_t count;
	struct sdp_attribute attrs[N_ATTRIBUTE];
	struct sdp_attribute *ptr;
	size_t capacity;
};

struct sdp_media
{
	char* media; //audio, video, text, application, message
	char* port;
	char* proto; // udp, RTP/AVP, RTP/SAVP
	struct format
	{
		size_t count;
		char *formats[N_MEDIA_FORMAT];
		char **ptr;
		size_t capacity;
	} fmt;

	char* i;
	struct connection
	{
		size_t count;
		struct sdp_connection connections[N_EMAIL];
		struct sdp_connection *ptr;
		size_t capacity;
	} c;

	struct attributes a;
	struct bandwidths b;
	struct sdp_encryption k;
};

struct sdp_t
{
	char *raw; // raw source string
	size_t offset; // parse offset

	int v;

	struct sdp_origin o;
	char* s;
	char* i;
	char* u;

	struct email
	{
		size_t count;
		struct sdp_email emails[N_EMAIL];
		struct sdp_email *ptr;
		size_t capacity;
	} e;

	struct phone
	{
		size_t count;
		struct sdp_phone phones[N_PHONE];
		struct sdp_phone *ptr;
		size_t capacity;
	} p;

	struct sdp_connection c;

	struct bandwidths b;

	struct timing
	{
		size_t count;
		struct sdp_timing times[N_TIMING];
		struct sdp_timing *ptr;
		size_t capacity;
	} t;

	struct sdp_encryption k;

	struct attributes a;

	struct media
	{
		size_t count;
		struct sdp_media medias[N_MEDIA];
		struct sdp_media *ptr;
		size_t capacity;
	} m;
};


static float duration()
{
    return 0.0;
}

//v=0
//o=<username> <sess-id> <sess-version> <nettype> <addrtype> <unicast-address>
//s=<session name>
//c=<net type> <addr type> <ip addr>
//t=<session start> <<session end>
//r=
//i=<No author> <No copyright>
//a=
//m=

const char* SDP_VOD_FMT =
        "v=0\n"
        "o=%s %llu %llu IN IP4 %s\n"
        "s=%s\n"
        "c=IN IP4 0.0.0.0\n"
        "t=0 0\n"
        "a=range:npt=0-%.1f\n"
        "a=recvonly\n"
        "a=control:*\n";

const char* SDP_LIVE_FMT =
        "v=0\n"
        "o=- %llu %llu IN IP4 %s\n"
        "s=%s\n"
        "c=IN IP4 0.0.0.0\n"
        "t=0 0\n"
        "a=range:npt=now-\n"
        "a=recvonly\n"
        "a=control:*\n";

int get_sdp(struct media_source *ms, char *sdp, size_t len)
{
    int sdp_len = 0;
    char sdp_filter[128];
    char sdp_range[128];
    const char *sdp_media = "m=video 0 RTP/AVP 33\r\nc=IN IP4 0.0.0.0\r\nb=AS:5000\r\na=control:track1";

    float dur = duration();
    if (dur == 0.0) {
      snprintf(sdp_range, sizeof(sdp_range), "%s", "a=range:npt=0-\r\n");
    } else if (dur > 0.0) {
      snprintf(sdp_range, sizeof(sdp_range), "%s", "a=range:npt=0-%.3f\r\n");
    } else { // subsessions have differing durations, so "a=range:" lines go there
      snprintf(sdp_range, sizeof(sdp_range), "%s", "");
    }
    snprintf(sdp_filter, sizeof(sdp_filter), SDP_FILTER_FMT, RTSP_SERVER_IP);

    char const* const sdp_prefix_fmt =
      "v=0\r\n"
      "o=- %ld%06ld %d IN IP4 %s\r\n"
      "s=%s\r\n"
      "i=%s\r\n"
      "t=0 0\r\n"
      "a=tool:%s %s\r\n"
      "a=type:broadcast\r\n"
      "a=control:*\r\n"
      "%s"
      "%s"
      "a=x-qt-text-nam:%s\r\n"
      "a=x-qt-text-inf:%s\r\n"
      "%s";

    sdp_len = strlen(sdp_prefix_fmt)
      + 20 + 6 + 20 + strlen("127.0.0.1")
      + strlen(ms->name)
      + strlen(ms->info)
      + strlen(SDP_TOOL_NAME) + strlen(SDP_TOOL_VERSION)
      + strlen(sdp_filter)
      + strlen(sdp_range)
      + strlen(ms->name)
      + strlen(ms->info)
      + strlen(sdp_media);
    if (sdp_len > len) {
        loge("sdp len %d is larger than buffer len %d!\n", sdp_len, len);
        return -1;
    }

    // Generate the SDP prefix (session-level lines):
    snprintf(sdp, sdp_len, sdp_prefix_fmt,
        ms->tm_create.tv_sec, ms->tm_create.tv_usec,// o= <session id>
	     1, // o= <version>
	     RTSP_SERVER_IP, // o= <address>
	     ms->name, // s= <description>
	     ms->info, // i= <info>
	     SDP_TOOL_NAME, SDP_TOOL_VERSION, // a=tool:
	     sdp_filter, // a=source-filter: incl (if a SSM session)
	     sdp_range, // a=range: line
	     ms->name, // a=x-qt-text-nam: line
	     ms->info, // a=x-qt-text-inf: line
	     sdp_media); // miscellaneous session SDP lines (if any)
    logd("strlen sdp = %d\n", strlen(sdp));

  return 0;
}
