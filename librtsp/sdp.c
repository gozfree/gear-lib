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


static float duration()
{
    return 0.0;
}


char *get_sdp(struct media_session *ms)
{
    char *sdp = NULL;
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
      + strlen(ms->description)
      + strlen(ms->info)
      + strlen(SDP_TOOL_NAME) + strlen(SDP_TOOL_VERSION)
      + strlen(sdp_filter)
      + strlen(sdp_range)
      + strlen(ms->description)
      + strlen(ms->info)
      + strlen(sdp_media);
    logd("sdp_len = %d\n", sdp_len);
    sdp = (char *)calloc(1, sdp_len);

    // Generate the SDP prefix (session-level lines):
    snprintf(sdp, sdp_len, sdp_prefix_fmt,
        ms->tm_create.tv_sec, ms->tm_create.tv_usec,// o= <session id>
	     1, // o= <version>
	     RTSP_SERVER_IP, // o= <address>
	     ms->description, // s= <description>
	     ms->info, // i= <info>
	     SDP_TOOL_NAME, SDP_TOOL_VERSION, // a=tool:
	     sdp_filter, // a=source-filter: incl (if a SSM session)
	     sdp_range, // a=range: line
	     ms->description, // a=x-qt-text-nam: line
	     ms->info, // a=x-qt-text-inf: line
	     sdp_media); // miscellaneous session SDP lines (if any)
    logd("strlen sdp = %d\n", strlen(sdp));

  return sdp;
}
