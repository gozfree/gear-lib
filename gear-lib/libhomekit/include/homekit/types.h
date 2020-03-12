#ifndef __HOMEKIT_TYPES_H__
#define __HOMEKIT_TYPES_H__

#include <stdbool.h>
#include <stdint.h>

#include <homekit/tlv.h>


typedef enum {
    homekit_format_bool,
    homekit_format_uint8,
    homekit_format_uint16,
    homekit_format_uint32,
    homekit_format_uint64,
    homekit_format_int,
    homekit_format_float,
    homekit_format_string,
    homekit_format_tlv,
    homekit_format_data
} homekit_format_t;

typedef enum {
    homekit_unit_none,
    homekit_unit_celsius,
    homekit_unit_percentage,
    homekit_unit_arcdegrees,
    homekit_unit_lux,
    homekit_unit_seconds
} homekit_unit_t;

typedef enum {
    homekit_permissions_paired_read = 1,
    homekit_permissions_paired_write = 2,
    homekit_permissions_notify = 4,
    homekit_permissions_additional_authorization = 8,
    homekit_permissions_timed_write = 16,
    homekit_permissions_hidden = 32
} homekit_permissions_t;

typedef enum {
    homekit_accessory_category_other = 1,
    homekit_accessory_category_bridge = 2,
    homekit_accessory_category_fan = 3,
    homekit_accessory_category_garage = 4,
    homekit_accessory_category_lightbulb = 5,
    homekit_accessory_category_door_lock = 6,
    homekit_accessory_category_outlet = 7,
    homekit_accessory_category_switch = 8,
    homekit_accessory_category_thermostat = 9,
    homekit_accessory_category_sensor = 10,
    homekit_accessory_category_security_system = 11,
    homekit_accessory_category_door = 12,
    homekit_accessory_category_window = 13,
    homekit_accessory_category_window_covering = 14,
    homekit_accessory_category_programmable_switch = 15,
    homekit_accessory_category_range_extender = 16,
    homekit_accessory_category_ip_camera = 17,
    homekit_accessory_category_video_door_bell = 18,
    homekit_accessory_category_air_purifier = 19,
    homekit_accessory_category_heater = 20,
    homekit_accessory_category_air_conditioner = 21,
    homekit_accessory_category_humidifier = 22,
    homekit_accessory_category_dehumidifier = 23,
    homekit_accessory_category_apple_tv = 24,
    homekit_accessory_category_speaker = 26,
    homekit_accessory_category_airport = 27,
    homekit_accessory_category_sprinkler = 28,
    homekit_accessory_category_faucet = 29,
    homekit_accessory_category_shower_head = 30,
    homekit_accessory_category_television = 31,
    homekit_accessory_category_target_controller = 32,
} homekit_accessory_category_t;

struct _homekit_accessory;
struct _homekit_service;
struct _homekit_characteristic;
typedef struct _homekit_accessory homekit_accessory_t;
typedef struct _homekit_service homekit_service_t;
typedef struct _homekit_characteristic homekit_characteristic_t;


typedef struct {
    bool is_null : 1;
    bool is_static : 1;
    homekit_format_t format : 6;
    union {
        bool bool_value;
        int int_value;
        uint8_t uint8_value;
        uint16_t uint16_value;
        uint32_t uint32_value;
        uint64_t uint64_value;
        float float_value;
        char *string_value;
        tlv_values_t *tlv_values;
        // data
    };
} homekit_value_t;

bool homekit_value_equal(homekit_value_t *a, homekit_value_t *b);
void homekit_value_copy(homekit_value_t *dst, homekit_value_t *src);
homekit_value_t *homekit_value_clone(homekit_value_t *value);
void homekit_value_destruct(homekit_value_t *value);
void homekit_value_free(homekit_value_t *value);


#define HOMEKIT_NULL_(...) \
    {.format=homekit_format_bool, .is_null=true, ##__VA_ARGS__}
#define HOMEKIT_NULL(...) (homekit_value_t) HOMEKIT_NULL_( __VA_ARGS__ )

#define HOMEKIT_BOOL_(value, ...) \
    {.format=homekit_format_bool, .bool_value=(value), ##__VA_ARGS__}
#define HOMEKIT_BOOL(value, ...) (homekit_value_t) HOMEKIT_BOOL_(value, __VA_ARGS__)

#define HOMEKIT_INT_(value, ...) \
    {.format=homekit_format_int, .int_value=(value), ##__VA_ARGS__}
#define HOMEKIT_INT(value, ...) (homekit_value_t) HOMEKIT_INT_(value, ##__VA_ARGS__)

#define HOMEKIT_UINT8_(value, ...) \
    {.format=homekit_format_uint8, .uint8_value=(value), ##__VA_ARGS__}
#define HOMEKIT_UINT8(value, ...) (homekit_value_t) HOMEKIT_UINT8_(value, ##__VA_ARGS__)

#define HOMEKIT_UINT16_(value, ...) \
    {.format=homekit_format_uint16, .uint16_value=(value), ##__VA_ARGS__}
#define HOMEKIT_UINT16(value, ...) (homekit_value_t) HOMEKIT_UINT16_(value, ##__VA_ARGS__)

#define HOMEKIT_UINT32_(value, ...) \
    {.format=homekit_format_uint32, .uint32_value=(value), ##__VA_ARGS__}
#define HOMEKIT_UINT32(value, ...) (homekit_value_t) HOMEKIT_UINT32_(value, ##__VA_ARGS__)

#define HOMEKIT_UINT64_(value, ...) \
    {.format=homekit_format_uint64, .uint64_value=(value), ##__VA_ARGS__}
#define HOMEKIT_UINT64(value, ...) (homekit_value_t) HOMEKIT_UINT64_(value, ##__VA_ARGS__)

#define HOMEKIT_FLOAT_(value, ...) \
    {.format=homekit_format_float, .float_value=(value), ##__VA_ARGS__}
#define HOMEKIT_FLOAT(value, ...) (homekit_value_t) HOMEKIT_FLOAT_(value, ##__VA_ARGS__)

#define HOMEKIT_STRING_(value, ...) \
    {.format=homekit_format_string, .string_value=(value), ##__VA_ARGS__}
#define HOMEKIT_STRING(value, ...) (homekit_value_t) HOMEKIT_STRING_(value, ##__VA_ARGS__)

#define HOMEKIT_TLV_(value, ...) \
    {.format=homekit_format_tlv, .tlv_values=(value), ##__VA_ARGS__}
#define HOMEKIT_TLV(value, ...) (homekit_value_t) HOMEKIT_TLV_(value, ##__VA_ARGS__)
/*
#define HOMEKIT_DATA_(value, ...) \
    {.format=homekit_format_data, .data_value=(value), ##__VA_ARGS__}
#define HOMEKIT_DATA(value, ...) (homekit_value_t) HOMEKIT_DATA_(value, ##__VA_ARGS__)
*/


typedef struct {
    int count;
    uint8_t *values;
} homekit_valid_values_t;


typedef struct {
    uint8_t start;
    uint8_t end;
} homekit_valid_values_range_t;


typedef struct {
    int count;
    homekit_valid_values_range_t *ranges;
} homekit_valid_values_ranges_t;


typedef void (*homekit_characteristic_change_callback_fn)(homekit_characteristic_t *ch, homekit_value_t value, void *context);

typedef struct _homekit_characteristic_change_callback {
    homekit_characteristic_change_callback_fn function;
    void *context;
    struct _homekit_characteristic_change_callback *next;
} homekit_characteristic_change_callback_t;


#define HOMEKIT_CHARACTERISTIC_CALLBACK(f, ...) &(homekit_characteristic_change_callback_t) { .function = f, ##__VA_ARGS__ }


struct _homekit_characteristic {
    homekit_service_t *service;

    unsigned int id;
    const char *type;
    const char *description;
    homekit_format_t format;
    homekit_unit_t unit;
    homekit_permissions_t permissions;
    homekit_value_t value;

    float *min_value;
    float *max_value;
    float *min_step;
    int *max_len;
    int *max_data_len;

    homekit_valid_values_t valid_values;
    homekit_valid_values_ranges_t valid_values_ranges;

    homekit_value_t (*getter)();
    void (*setter)(const homekit_value_t);
    homekit_characteristic_change_callback_t *callback;

    homekit_value_t (*getter_ex)(const homekit_characteristic_t *ch);
    void (*setter_ex)(homekit_characteristic_t *ch, const homekit_value_t value);

    void *context;
};

struct _homekit_service {
    homekit_accessory_t *accessory;

    unsigned int id;
    const char *type;
    bool hidden;
    bool primary;

    homekit_service_t **linked;
    homekit_characteristic_t **characteristics;
};

struct _homekit_accessory {
    unsigned int id;

    homekit_accessory_category_t category;
    int config_number;

    homekit_service_t **services;
};

// Macro to define accessory
#define HOMEKIT_ACCESSORY(...) \
    &(homekit_accessory_t) { \
        .config_number=1, \
        .category=homekit_accessory_category_other, \
        ##__VA_ARGS__ \
    }

// Macro to define service inside accessory definition.
// Requires HOMEKIT_SERVICE_<name> define to expand to service type UUID string
#define HOMEKIT_SERVICE(_type, ...) \
    &(homekit_service_t) { .type=HOMEKIT_SERVICE_ ## _type, ##__VA_ARGS__ }

// Macro to define standalone service (outside of accessory definition)
// Requires HOMEKIT_SERVICE_<name> define to expand to service type UUID string
#define HOMEKIT_SERVICE_(_type, ...) \
    { .type=HOMEKIT_SERVICE_ ## _type, ##__VA_ARGS__ }

// Macro to define characteristic inside service definition
#define HOMEKIT_CHARACTERISTIC(name, ...) \
    &(homekit_characteristic_t) { \
        HOMEKIT_DECLARE_CHARACTERISTIC_ ## name( __VA_ARGS__ ) \
    }

// Macro to define standalone characteristic (outside of service definition)
// Requires HOMEKIT_DECLARE_CHARACTERISTIC_<name>() macro
#define HOMEKIT_CHARACTERISTIC_(name, ...) \
    { \
        HOMEKIT_DECLARE_CHARACTERISTIC_ ## name( __VA_ARGS__ ) \
    }

// Declaration macro to create a custom characteristic inplace without
// having to define HOMKIT_DECLARE_CHARACTERISTIC_<name>() macro.
//
// Useage:
//     homekit_characteristic_t custom_ch = HOMEKIT_CHARACTERISTIC_(
//         CUSTOM,
//         .type = "00000023-0000-1000-8000-0026BB765291",
//         .description = "My custom characteristic",
//         .format = homekit_format_string,
//         .permissions = homekit_permissions_paired_read
//                      | homekit_permissions_paired_write,
//         .value = HOMEKIT_STRING_("my value"),
//     );
#define HOMEKIT_DECLARE_CHARACTERISTIC_CUSTOM(...) \
    .format = homekit_format_uint8, \
    .unit = homekit_unit_none, \
    .permissions = homekit_permissions_paired_read, \
    ##__VA_ARGS__


// Allocate memory and copy given accessory
// Does not make copies of all accessory services, so make sure thay are
// either allocated on heap or in static memory (but not on stack).
homekit_accessory_t *homekit_accessory_clone(homekit_accessory_t *accessory);
// Allocate memory and copy given service.
// Does not make copies of all service characteristics, so make sure that are
// either allocated on heap or in static memory (but not on stack).
homekit_service_t *homekit_service_clone(homekit_service_t *service);
// Allocate memory and copy given characteristic.
homekit_characteristic_t *homekit_characteristic_clone(homekit_characteristic_t *characteristic);


// Macro to define an accessory in dynamic memory.
// Used to aid creating accessories definitions in runtime.
// Makes copy of all internal services/characteristics.
#define NEW_HOMEKIT_ACCESSORY(...) \
    homekit_accessory_clone(HOMEKIT_ACCESSORY(__VA_ARGS__))

// Macro to define an service in dynamic memory.
// Used to aid creating services definitions in runtime.
// Makes copy of all internal characteristics.
#define NEW_HOMEKIT_SERVICE(name, ...) \
    homekit_service_clone(HOMEKIT_SERVICE(name, ## __VA_ARGS__))

// Macro to define an characteristic in dynamic memory.
// Used to aid creating characteristics definitions in runtime.
#define NEW_HOMEKIT_CHARACTERISTIC(name, ...) \
    homekit_characteristic_clone(HOMEKIT_CHARACTERISTIC(name, ## __VA_ARGS__))


// Init accessories by automatically assigning IDs to all
// accessories/services/characteristics, normalizing internal data.
void homekit_accessories_init(homekit_accessory_t **accessories);

// Find accessory by ID. Returns NULL if not found
homekit_accessory_t *homekit_accessory_by_id(homekit_accessory_t **accessories, int aid);
// Find service inside accessory by service type. Returns NULL if not found
homekit_service_t *homekit_service_by_type(homekit_accessory_t *accessory, const char *type);
// Find characteristic inside service by type. Returns NULL if not found
homekit_characteristic_t *homekit_service_characteristic_by_type(homekit_service_t *service, const char *type);
// Find characteristic by accessory ID and characteristic ID. Returns NULL if not found
homekit_characteristic_t *homekit_characteristic_by_aid_and_iid(homekit_accessory_t **accessories, int aid, int iid);

void homekit_characteristic_notify(homekit_characteristic_t *ch, const homekit_value_t value);
void homekit_characteristic_add_notify_callback(
    homekit_characteristic_t *ch,
    homekit_characteristic_change_callback_fn callback,
    void *context
);
void homekit_characteristic_remove_notify_callback(
    homekit_characteristic_t *ch,
    homekit_characteristic_change_callback_fn callback,
    void *context
);
void homekit_accessories_clear_notify_callbacks(
    homekit_accessory_t **accessories,
    homekit_characteristic_change_callback_fn callback,
    void *context
);
bool homekit_characteristic_has_notify_callback(
    const homekit_characteristic_t *ch,
    homekit_characteristic_change_callback_fn callback,
    void *context
);


#endif // __HOMEKIT_TYPES_H__
