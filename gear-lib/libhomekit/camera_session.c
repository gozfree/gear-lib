#include <stdlib.h>
#include <sys/socket.h>

#if 0
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#endif

#include "camera_session.h"


camera_session_t *camera_session_new() {
    camera_session_t *session = calloc(1, sizeof(camera_session_t));
    session->video_ssrc = 1;
    session->audio_ssrc = 1;

    return session;
}


void camera_session_free(camera_session_t *session) {
    if (!session)
        return;

    if (session->controller_ip_address)
        free(session->controller_ip_address);

    if (session->video_socket) {
        close(session->video_socket);
    }

    free(session);
}
