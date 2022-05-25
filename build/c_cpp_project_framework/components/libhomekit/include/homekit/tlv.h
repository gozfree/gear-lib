#ifndef __TLV_H__
#define __TLV_H__

typedef unsigned char byte;

typedef struct _tlv {
    struct _tlv *next;
    byte type;
    byte *value;
    size_t size;
} tlv_t;


typedef struct {
    tlv_t *head;
} tlv_values_t;


tlv_values_t *tlv_new();

void tlv_free(tlv_values_t *values);

int tlv_add_value(tlv_values_t *values, byte type, const byte *value, size_t size);
int tlv_add_string_value(tlv_values_t *values, byte type, const char *value);
int tlv_add_integer_value(tlv_values_t *values, byte type, size_t size, int value);
int tlv_add_tlv_value(tlv_values_t *values, byte type, tlv_values_t *value);

tlv_t *tlv_get_value(const tlv_values_t *values, byte type);
int tlv_get_integer_value(const tlv_values_t *values, byte type, int def);
tlv_values_t *tlv_get_tlv_value(const tlv_values_t *values, byte type);

int tlv_format(const tlv_values_t *values, byte *buffer, size_t *size);

int tlv_parse(const byte *buffer, size_t length, tlv_values_t *values);

#endif // __TLV_H__
