#ifndef DSHOW_H
#define DSHOW_H

#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dshow.h>
#include <strmif.h>
#include <dvdmedia.h>
#include <uuids.h>
#include <control.h>
#include <objidl.h>
#include <shlwapi.h>
#include <vfwmsgs.h>
#include <stdint.h>

enum dshowDeviceType {
    VideoDevice = 0,
    AudioDevice = 1,
};


//#define dshowdebug(...) printf(__VA_ARGS__)
#define dshowdebug(...) do{} while(0)

struct GUIDoffset {
    const GUID *iid;
    int offset;
};

#define DECLARE_QUERYINTERFACE(class, ...)                                   \
long WINAPI class##_QueryInterface(class *this, const GUID *riid, void **ppobj) \
{                                                                            \
    struct GUIDoffset ifaces[] = __VA_ARGS__;                                \
    int i;                                                                   \
    dshowdebug("%s(%p, %p, %p)\n", __func__, this, riid, ppobj);             \
    if (!ppobj)                                                              \
        return E_POINTER;                                                    \
    for (i = 0; i < sizeof(ifaces)/sizeof(ifaces[0]); i++) {                 \
        if (IsEqualGUID(riid, ifaces[i].iid)) {                              \
            void *obj = (void *) ((uint8_t *) this + ifaces[i].offset);      \
            class##_AddRef(this);                                            \
            *ppobj = (void *) obj;                                           \
            return S_OK;                                                     \
        }                                                                    \
    }                                                                        \
    dshowdebug("%s(%p) E_NOINTERFACE\n", __func__, this, riid, ppobj);       \
    *ppobj = NULL;                                                           \
    return E_NOINTERFACE;                                                    \
}

#define DECLARE_ADDREF(class)                                                \
unsigned long WINAPI                                                         \
class##_AddRef(class *this)                                                  \
{                                                                            \
    dshowdebug("%s(%p)\t%ld\n", __func__, this, this->ref+1);  \
    return InterlockedIncrement(&this->ref);                                 \
}
#define DECLARE_RELEASE(class)                                               \
unsigned long WINAPI                                                         \
class##_Release(class *this)                                                 \
{                                                                            \
    long ref = InterlockedDecrement(&this->ref);                             \
    dshowdebug("%s(%p)\t%ld\n", __func__, this, ref);         \
    if (!ref)                                                                \
        class##_Destroy(this);                                               \
    return ref;                                                              \
}


#define DECLARE_DESTROY(class, func)                                         \
void class##_Destroy(class *this)                                            \
{                                                                            \
    dshowdebug("%s(%p)\n", __func__, this);                   \
    func(this);                                                              \
    if (this) {                                                              \
        if (this->vtbl)                                                      \
            CoTaskMemFree(this->vtbl);                                       \
        CoTaskMemFree(this);                                                 \
    }                                                                        \
}
#define DECLARE_CREATE(class, setup, ...)                                    \
class *class##_Create(__VA_ARGS__)                                           \
{                                                                            \
    class *this = CoTaskMemAlloc(sizeof(class));                             \
    void  *vtbl = CoTaskMemAlloc(sizeof(*this->vtbl));                       \
    if (!this || !vtbl)                                                      \
        goto fail;                                                           \
    ZeroMemory(this, sizeof(class));                                         \
    ZeroMemory(vtbl, sizeof(*this->vtbl));                                   \
    this->ref  = 1;                                                          \
    this->vtbl = vtbl;                                                       \
    if (!setup)                                                              \
        goto fail;                                                           \
    dshowdebug("%s(%p)\n", __func__, this);                                  \
    return this;                                                             \
fail:                                                                        \
    class##_Destroy(this);                                                   \
    dshowdebug("could not create \n");                  \
    return NULL;                                                             \
}

#define SETVTBL(vtbl, class, fn) \
    do { (vtbl)->fn = (void *) class##_##fn; } while(0)

#define imemoffset offsetof(dshow_pin, imemvtbl)

/*****************************************************************************
 * dshow_pin
 ****************************************************************************/
typedef struct dshow_pin dshow_pin;
typedef struct dshow_filter dshow_filter;
typedef struct dshow_enum_pins dshow_enum_pins;
typedef struct dshow_enum_media_types dshow_enum_media_types;
typedef struct dshow_input_pin dshow_input_pin;

struct dshow_pin {
    IPinVtbl *vtbl;
    IMemInputPinVtbl *imemvtbl;
    IPin *connectedto;
    struct dshow_filter *filter;
    AM_MEDIA_TYPE type;
    long ref;
};

struct dshow_filter {
    IBaseFilterVtbl *vtbl;
    long ref;
    const wchar_t *name;
    struct dshow_pin *pin;
    FILTER_INFO info;
    FILTER_STATE state;
    IReferenceClock *clock;
    void *priv_data;
    int stream_index;
    int64_t start_time;
    void (*callback)(void *priv, int index, uint8_t *buf, int size, int64_t time, int devtype);
};


long          WINAPI dshow_pin_QueryInterface          (dshow_pin *, const GUID *, void **);
unsigned long WINAPI dshow_pin_AddRef                  (dshow_pin *);
unsigned long WINAPI dshow_pin_Release                 (dshow_pin *);
long          WINAPI dshow_pin_Connect                 (dshow_pin *, IPin *, const AM_MEDIA_TYPE *);
long          WINAPI dshow_pin_ReceiveConnection       (dshow_pin *, IPin *, const AM_MEDIA_TYPE *);
long          WINAPI dshow_pin_Disconnect              (dshow_pin *);
long          WINAPI dshow_pin_ConnectedTo             (dshow_pin *, IPin **);
long          WINAPI dshow_pin_ConnectionMediaType     (dshow_pin *, AM_MEDIA_TYPE *);
long          WINAPI dshow_pin_QueryPinInfo            (dshow_pin *, PIN_INFO *);
long          WINAPI dshow_pin_QueryDirection          (dshow_pin *, PIN_DIRECTION *);
long          WINAPI dshow_pin_QueryId                 (dshow_pin *, wchar_t **);
long          WINAPI dshow_pin_QueryAccept             (dshow_pin *, const AM_MEDIA_TYPE *);
long          WINAPI dshow_pin_EnumMediaTypes          (dshow_pin *, IEnumMediaTypes **);
long          WINAPI dshow_pin_QueryInternalConnections(dshow_pin *, IPin **, unsigned long *);
long          WINAPI dshow_pin_EndOfStream             (dshow_pin *);
long          WINAPI dshow_pin_BeginFlush              (dshow_pin *);
long          WINAPI dshow_pin_EndFlush                (dshow_pin *);
long          WINAPI dshow_pin_NewSegment              (dshow_pin *, REFERENCE_TIME, REFERENCE_TIME, double);
dshow_pin           *dshow_pin_Create (dshow_filter *filter);
void                 dshow_pin_Destroy(dshow_pin *);

/*****************************************************************************
 * dshow_input_pin
 ****************************************************************************/
long          WINAPI dshow_input_pin_QueryInterface          (dshow_input_pin *, const GUID *, void **);
unsigned long WINAPI dshow_input_pin_AddRef                  (dshow_input_pin *);
unsigned long WINAPI dshow_input_pin_Release                 (dshow_input_pin *);
long          WINAPI dshow_input_pin_GetAllocator            (dshow_input_pin *, IMemAllocator **);
long          WINAPI dshow_input_pin_NotifyAllocator         (dshow_input_pin *, IMemAllocator *, BOOL);
long          WINAPI dshow_input_pin_GetAllocatorRequirements(dshow_input_pin *, ALLOCATOR_PROPERTIES *);
long          WINAPI dshow_input_pin_Receive                 (dshow_input_pin *, IMediaSample *);
long          WINAPI dshow_input_pin_ReceiveMultiple         (dshow_input_pin *, IMediaSample **, long, long *);
long          WINAPI dshow_input_pin_ReceiveCanBlock         (dshow_input_pin *);
void                 dshow_input_pin_Destroy(dshow_input_pin *);

/*****************************************************************************
 * dshow_enum_media_types
 ****************************************************************************/
struct dshow_enum_media_types {
    IEnumMediaTypesVtbl *vtbl;
    long ref;
    int pos;
    AM_MEDIA_TYPE type;
};

long          WINAPI dshow_enum_media_types_QueryInterface(dshow_enum_media_types *, const GUID *, void **);
unsigned long WINAPI dshow_enum_media_types_AddRef        (dshow_enum_media_types *);
unsigned long WINAPI dshow_enum_media_types_Release       (dshow_enum_media_types *);
long          WINAPI dshow_enum_media_types_Next          (dshow_enum_media_types *, unsigned long, AM_MEDIA_TYPE **, unsigned long *);
long          WINAPI dshow_enum_media_types_Skip          (dshow_enum_media_types *, unsigned long);
long          WINAPI dshow_enum_media_types_Reset         (dshow_enum_media_types *);
long          WINAPI dshow_enum_media_types_Clone         (dshow_enum_media_types *, dshow_enum_media_types **);

void                 dshow_enum_media_types_Destroy(dshow_enum_media_types *);
dshow_enum_media_types *dshow_enum_media_types_Create(const AM_MEDIA_TYPE *type);

/*****************************************************************************
 * dshow_filter
 ****************************************************************************/
long          WINAPI dshow_filter_QueryInterface (dshow_filter *, const GUID *, void **);
unsigned long WINAPI dshow_filter_AddRef         (dshow_filter *);
unsigned long WINAPI dshow_filter_Release        (dshow_filter *);
long          WINAPI dshow_filter_GetClassID     (dshow_filter *, CLSID *);
long          WINAPI dshow_filter_Stop           (dshow_filter *);
long          WINAPI dshow_filter_Pause          (dshow_filter *);
long          WINAPI dshow_filter_Run            (dshow_filter *, REFERENCE_TIME);
long          WINAPI dshow_filter_GetState       (dshow_filter *, DWORD, FILTER_STATE *);
long          WINAPI dshow_filter_SetSyncSource  (dshow_filter *, IReferenceClock *);
long          WINAPI dshow_filter_GetSyncSource  (dshow_filter *, IReferenceClock **);
long          WINAPI dshow_filter_EnumPins       (dshow_filter *, IEnumPins **);
long          WINAPI dshow_filter_FindPin        (dshow_filter *, const wchar_t *, IPin **);
long          WINAPI dshow_filter_QueryFilterInfo(dshow_filter *, FILTER_INFO *);
long          WINAPI dshow_filter_JoinFilterGraph(dshow_filter *, IFilterGraph *, const wchar_t *);
long          WINAPI dshow_filter_QueryVendorInfo(dshow_filter *, wchar_t **);

void          dshow_filter_Destroy(dshow_filter *);
dshow_filter   *dshow_filter_Create (void *, void *, enum dshowDeviceType);


/*****************************************************************************
 * dshow_enum_pins
 ****************************************************************************/
struct dshow_enum_pins {
    IEnumPinsVtbl *vtbl;
    long ref;
    int pos;
    dshow_pin *pin;
    dshow_filter *filter;
};

long          WINAPI dshow_enum_pins_QueryInterface(dshow_enum_pins *, const GUID *, void **);
unsigned long WINAPI dshow_enum_pins_AddRef        (dshow_enum_pins *);
unsigned long WINAPI dshow_enum_pins_Release       (dshow_enum_pins *);
long          WINAPI dshow_enum_pins_Next          (dshow_enum_pins *, unsigned long, IPin **, unsigned long *);
long          WINAPI dshow_enum_pins_Skip          (dshow_enum_pins *, unsigned long);
long          WINAPI dshow_enum_pins_Reset         (dshow_enum_pins *);
long          WINAPI dshow_enum_pins_Clone         (dshow_enum_pins *, dshow_enum_pins **);

void                 dshow_enum_pins_Destroy(dshow_enum_pins *);
dshow_enum_pins       *dshow_enum_pins_Create (dshow_pin *pin, dshow_filter *filter);


#endif
