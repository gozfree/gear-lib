#pragma once

#include "streaming_session.h"


void session_init_crypto(streaming_session_t *session);
int session_encrypt(streaming_session_t *session, uint8_t *in, int len, int outlen);
