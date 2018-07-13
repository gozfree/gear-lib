#include <libtime.h>
#include <liblog.h>
#include "sdp.h"
#include "media_source.h"
#include <stdio.h>
#include <string.h>

static int is_auth()
{
    return 0;
}

static int sdp_generate(struct media_source *ms)
{
    char sdp[SDP_LEN_MAX];

    uint64_t now_usec = time_get_usec();
    const char* prefix_fmt =
        "v=0\n"
        "o=%s %llu %llu IN IP4 %s\n"
        "s=%s\n"
        "i=%s\n";

    int n = 0;
    n += snprintf(sdp+n, sizeof(sdp)-n, prefix_fmt, 
                is_auth()?"username":"-", now_usec/1000000, now_usec, "0.0.0.0",
                ms->name,
                ms->info);
    n += snprintf(sdp+n, sizeof(sdp)-n, "c=IN IP4 0.0.0.0\n");
    n += snprintf(sdp+n, sizeof(sdp)-n, "t=0 0\n");
    n += snprintf(sdp+n, sizeof(sdp)-n, "a=range:npt=0-\n");
    n += snprintf(sdp+n, sizeof(sdp)-n, "a=recvonly\n");
    n += snprintf(sdp+n, sizeof(sdp)-n, "a=control:*\n");
    n += snprintf(sdp+n, sizeof(sdp)-n, "a=source-filter: incl IN IP4 * %s\r\n", "0.0.0.0");
    n += snprintf(sdp+n, sizeof(sdp)-n, "a=rtcp-unicast: reflection\r\n");
    n += snprintf(sdp+n, sizeof(sdp)-n, "a=x-qt-text-nam:%s\r\n", ms->name);
    n += snprintf(sdp+n, sizeof(sdp)-n, "a=x-qt-text-inf:%s\r\n", ms->info);

    n += snprintf(sdp+n, sizeof(sdp)-n, "m=video 0 RTP/AVP 33\r\n");
    n += snprintf(sdp+n, sizeof(sdp)-n, "b=AS:5000\r\n");
    n += snprintf(sdp+n, sizeof(sdp)-n, "a=control:track1");

    strcpy(ms->sdp, sdp);
    return 0;
}

struct media_source live_media_source = {
    .sdp_generate = sdp_generate,
};
