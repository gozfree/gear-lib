#ifndef __HOMEKIT_H__
#define __HOMEKIT_H__

#include <homekit/types.h>

typedef void *homekit_client_id_t;


typedef enum {
    HOMEKIT_EVENT_SERVER_INITIALIZED,
    // Just accepted client connection
    HOMEKIT_EVENT_CLIENT_CONNECTED,
    // Pairing verification completed and secure session is established
    HOMEKIT_EVENT_CLIENT_VERIFIED,
    HOMEKIT_EVENT_CLIENT_DISCONNECTED,
    HOMEKIT_EVENT_PAIRING_ADDED,
    HOMEKIT_EVENT_PAIRING_REMOVED,
} homekit_event_t;


typedef struct {
    // Pointer to an array of homekit_accessory_t pointers.
    // Array should be terminated by a NULL pointer.
    homekit_accessory_t **accessories;

    homekit_accessory_category_t category;

    int config_number;

    // Password in format "111-23-456".
    // If password is not specified, a random password
    // will be used. In that case, a password_callback
    // field must contain pointer to a function that should
    // somehow communicate password to the user (e.g. display
    // it on a screen if accessory has one).
    char *password;
    void (*password_callback)(const char *password);

    // Setup ID in format "XXXX" (where X is digit or latin capital letter)
    // Used for pairing using QR code
    char *setupId;

    // Callback for "POST /resource" to get snapshot image from camera
    void (*on_resource)(const char *body, size_t body_size);

    void (*on_event)(homekit_event_t event);
} homekit_server_config_t;

// Get pairing URI
int homekit_get_setup_uri(const homekit_server_config_t *config,
                          char *buffer, size_t buffer_size);

// Initialize HomeKit accessory server
void homekit_server_init(homekit_server_config_t *config);

// Reset HomeKit accessory server, removing all pairings
void homekit_server_reset();

int  homekit_get_accessory_id(char *buffer, size_t size);
bool homekit_is_paired();

// Client related stuff
homekit_client_id_t homekit_get_client_id();

bool homekit_client_is_admin();
int  homekit_client_send(unsigned char *data, size_t size);

#endif // __HOMEKIT_H__
