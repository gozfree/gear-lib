#include "base64.h"

static unsigned char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned char base64_encode_char(unsigned char c) {
  return base64_chars[c];
}

unsigned char base64_decode_char(unsigned char c) {
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 26;
  if (c >= '0' && c <= '9')
    return c - '0' + 52;
  if (c == '+')
    return 62;
  if (c == '/')
    return 63;
  if (c == '=')
    return 64;

  return 0;
}

size_t base64_encoded_size(const unsigned char *data, size_t size) {
  return (size + 2)/3*4;
}

size_t base64_decoded_size(const unsigned char *encoded_data, size_t encoded_size) {
  size_t size = (encoded_size + 3)/4*3;
  if (encoded_data[encoded_size-1] == '=')
      size--;
  if (encoded_data[encoded_size-2] == '=')
      size--;
  return size;
}

int base64_encode(const unsigned char* data, size_t size, unsigned char *encoded_data) {
  size_t i=0, j=0, size1=size - size%3;
  for (; i<size1; i+=3, j+=4) {
    encoded_data[j+0] = base64_encode_char(data[i+0]>>2);
    encoded_data[j+1] = base64_encode_char(((data[i+0]&0x3)<<4) + (data[i+1]>>4));
    encoded_data[j+2] = base64_encode_char(((data[i+1]&0xF)<<2) + (data[i+2]>>6));
    encoded_data[j+3] = base64_encode_char(data[i+2]&0x3F);
  }

  if (size % 3 == 1) {
    encoded_data[j+0] = base64_encode_char(data[i+0]>>2);
    encoded_data[j+1] = base64_encode_char((data[i+0]&0x3)<<4);
    encoded_data[j+2] = encoded_data[j+3] = '=';
    j += 4;
  } else if (size % 3 == 2) {
    encoded_data[j+0] = base64_encode_char(data[i+0]>>2);
    encoded_data[j+1] = base64_encode_char(((data[i+0]&0x3)<<4) + (data[i+1]>>4));
    encoded_data[j+2] = base64_encode_char(((data[i+1]&0xF)<<2));
    encoded_data[j+3] = '=';
    j += 4;
  }

  return j;
}

int base64_decode(const unsigned char* encoded_data, size_t encoded_size, unsigned char *data) {
  if (encoded_size % 4 != 0)
    return -1;

  size_t i=0, j=0;
  for (; i<encoded_size; i+=4) {
    unsigned char block[4];
    for (size_t k=0; k<4; k++)
      block[k] = base64_decode_char(encoded_data[i+k]);

    data[j++] = (block[0]<<2) + (block[1]>>4);
    if (block[2] == 64)
      break;

    data[j++] = ((block[1]&0xF)<<4) + (block[2]>>2);
    if (block[3] == 64)
      break;

    data[j++] = ((block[2]&0x3)<<6) + block[3];
  }

  return j;
}

