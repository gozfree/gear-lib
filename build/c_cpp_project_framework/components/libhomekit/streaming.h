#pragma once

int streaming_init();

int streaming_get_video_port();
int streaming_get_audio_port();

int streaming_sessions_add(camera_session_t *session);
void streaming_sessions_remove(camera_session_t *session);
