/******************************************************************************
 * Copyright (C) 2014-2020 Zhifeng Gong <gozfree@163.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/
#include <stddef.h>
#include "dshow.h"
#include "libuvc.h"
#include "libposix4win.h"
#include <libfile.h>

struct file *fp;

struct dshow_ctx {
    int                    fd;
    char                  *name;
    char                  *unique_name;
    int                    width;
    int                    height;
    IGraphBuilder         *graph;
    ICaptureGraphBuilder2 *capgraph;
    IMediaControl         *mctrl;
    IMediaEvent           *mevent;
    IBaseFilter           *dev_filter;
    IPin                  *device_pin;
    dshow_filter          *cap_filter;
    ICreateDevEnum        *devenum;
    HANDLE mutex;
    HANDLE event[2]; /* event[0] is set by DirectShow
                      * event[1] is set by callback() */
};


/*****************************************************************************
 * dshow_enum_pins
 ****************************************************************************/
DECLARE_QUERYINTERFACE(dshow_enum_pins, {{&IID_IUnknown,0}, {&IID_IEnumPins,0}})
DECLARE_ADDREF(dshow_enum_pins)
DECLARE_RELEASE(dshow_enum_pins)

long WINAPI dshow_enum_pins_Next(dshow_enum_pins *this, unsigned long n,
                                IPin **pins, unsigned long *fetched)
{
    int count = 0;
    dshowdebug("dshow_enum_pins_Next(%p)\n", this);
    if (!pins)
        return E_POINTER;
    if (!this->pos && n == 1) {
        dshow_pin_AddRef(this->pin);
        *pins = (IPin *) this->pin;
        count = 1;
        this->pos = 1;
    }
    if (fetched)
        *fetched = count;
    if (!count)
        return S_FALSE;
    return S_OK;
}

long WINAPI dshow_enum_pins_Skip(dshow_enum_pins *this, unsigned long n)
{
    dshowdebug("dshow_enum_pins_Skip(%p)\n", this);
    if (n) /* Any skip will always fall outside of the only valid pin. */
        return S_FALSE;
    return S_OK;
}

long WINAPI dshow_enum_pins_Reset(dshow_enum_pins *this)
{
    dshowdebug("dshow_enum_pins_Reset(%p)\n", this);
    this->pos = 0;
    return S_OK;
}

long WINAPI dshow_enum_pins_Clone(dshow_enum_pins *this, dshow_enum_pins **pins)
{
    dshow_enum_pins *new;
    dshowdebug("dshow_enum_pins_Clone(%p)\n", this);
    if (!pins)
        return E_POINTER;
    new = dshow_enum_pins_Create(this->pin, this->filter);
    if (!new)
        return E_OUTOFMEMORY;
    new->pos = this->pos;
    *pins = new;
    return S_OK;
}

static int dshow_enum_pins_Setup(dshow_enum_pins *this, dshow_pin *pin,
                                 dshow_filter *filter)
{
    IEnumPinsVtbl *vtbl = this->vtbl;
    SETVTBL(vtbl, dshow_enum_pins, QueryInterface);
    SETVTBL(vtbl, dshow_enum_pins, AddRef);
    SETVTBL(vtbl, dshow_enum_pins, Release);
    SETVTBL(vtbl, dshow_enum_pins, Next);
    SETVTBL(vtbl, dshow_enum_pins, Skip);
    SETVTBL(vtbl, dshow_enum_pins, Reset);
    SETVTBL(vtbl, dshow_enum_pins, Clone);

    this->pin = pin;
    this->filter = filter;
    dshow_filter_AddRef(this->filter);

    return 1;
}

static int dshow_enum_pins_Cleanup(dshow_enum_pins *this)
{
    dshow_filter_Release(this->filter);
    return 1;
}

DECLARE_CREATE(dshow_enum_pins, dshow_enum_pins_Setup(this, pin, filter),
               dshow_pin *pin, dshow_filter *filter)
DECLARE_DESTROY(dshow_enum_pins, dshow_enum_pins_Cleanup)

/*****************************************************************************
 * dshow_filter
 ****************************************************************************/
DECLARE_QUERYINTERFACE(dshow_filter, {{&IID_IUnknown,0}, {&IID_IBaseFilter,0}})
DECLARE_ADDREF(dshow_filter)
DECLARE_RELEASE(dshow_filter)

long WINAPI dshow_filter_GetClassID(dshow_filter *this, CLSID *id)
{
    dshowdebug("dshow_filter_GetClassID(%p)\n", this);
    /* I'm not creating a ClassID just for this. */
    return E_FAIL;
}

long WINAPI dshow_filter_Stop(dshow_filter *this)
{
    dshowdebug("dshow_filter_Stop(%p)\n", this);
    this->state = State_Stopped;
    return S_OK;
}

long WINAPI dshow_filter_Pause(dshow_filter *this)
{
    dshowdebug("dshow_filter_Pause(%p)\n", this);
    this->state = State_Paused;
    return S_OK;
}

long WINAPI dshow_filter_Run(dshow_filter *this, REFERENCE_TIME start)
{
    dshowdebug("dshow_filter_Run(%p) %"PRId64"\n", this, start);
    this->state = State_Running;
    this->start_time = start;
    return S_OK;
}

long WINAPI dshow_filter_GetState(dshow_filter *this, DWORD ms,
                                  FILTER_STATE *state)
{
    dshowdebug("dshow_filter_GetState(%p)\n", this);
    if (!state)
        return E_POINTER;
    *state = this->state;
    return S_OK;
}

long WINAPI dshow_filter_SetSyncSource(dshow_filter *this, IReferenceClock *clock)
{
    dshowdebug("dshow_filter_SetSyncSource(%p)\n", this);

    if (this->clock != clock) {
        if (this->clock)
            IReferenceClock_Release(this->clock);
        this->clock = clock;
        if (clock)
            IReferenceClock_AddRef(clock);
    }

    return S_OK;
}

long WINAPI dshow_filter_GetSyncSource(dshow_filter *this, IReferenceClock **clock)
{
    dshowdebug("dshow_filter_GetSyncSource(%p)\n", this);

    if (!clock)
        return E_POINTER;
    if (this->clock)
        IReferenceClock_AddRef(this->clock);
    *clock = this->clock;

    return S_OK;
}

long WINAPI dshow_filter_EnumPins(dshow_filter *this, IEnumPins **enumpin)
{
    dshow_enum_pins *new;
    dshowdebug("dshow_filter_EnumPins(%p)\n", this);

    if (!enumpin)
        return E_POINTER;
    new = dshow_enum_pins_Create(this->pin, this);
    if (!new)
        return E_OUTOFMEMORY;

    *enumpin = (IEnumPins *) new;
    return S_OK;
}

long WINAPI dshow_filter_FindPin(dshow_filter *this, const wchar_t *id, IPin **pin)
{
    dshow_pin *found = NULL;
    dshowdebug("dshow_filter_FindPin(%p)\n", this);

    if (!id || !pin)
        return E_POINTER;
    if (!wcscmp(id, L"In")) {
        found = this->pin;
        dshow_pin_AddRef(found);
    }
    *pin = (IPin *) found;
    if (!found)
        return VFW_E_NOT_FOUND;

    return S_OK;
}

long WINAPI dshow_filter_QueryFilterInfo(dshow_filter *this, FILTER_INFO *info)
{
    dshowdebug("dshow_filter_QueryFilterInfo(%p)\n", this);

    if (!info)
        return E_POINTER;
    if (this->info.pGraph)
        IFilterGraph_AddRef(this->info.pGraph);
    *info = this->info;

    return S_OK;
}

long WINAPI dshow_filter_JoinFilterGraph(dshow_filter *this, IFilterGraph *graph,
                            const wchar_t *name)
{
    dshowdebug("dshow_filter_JoinFilterGraph(%p)\n", this);

    this->info.pGraph = graph;
    if (name)
        wcscpy(this->info.achName, name);

    return S_OK;
}

long WINAPI dshow_filter_QueryVendorInfo(dshow_filter *this, wchar_t **info)
{
    dshowdebug("dshow_filter_QueryVendorInfo(%p)\n", this);

    if (!info)
        return E_POINTER;
    return E_NOTIMPL; /* don't have to do anything here */
}

static int dshow_filter_setup(dshow_filter *this, void *priv_data,
                              void *callback, int type)
{
    IBaseFilterVtbl *vtbl = this->vtbl;
    SETVTBL(vtbl, dshow_filter, QueryInterface);
    SETVTBL(vtbl, dshow_filter, AddRef);
    SETVTBL(vtbl, dshow_filter, Release);
    SETVTBL(vtbl, dshow_filter, GetClassID);
    SETVTBL(vtbl, dshow_filter, Stop);
    SETVTBL(vtbl, dshow_filter, Pause);
    SETVTBL(vtbl, dshow_filter, Run);
    SETVTBL(vtbl, dshow_filter, GetState);
    SETVTBL(vtbl, dshow_filter, SetSyncSource);
    SETVTBL(vtbl, dshow_filter, GetSyncSource);
    SETVTBL(vtbl, dshow_filter, EnumPins);
    SETVTBL(vtbl, dshow_filter, FindPin);
    SETVTBL(vtbl, dshow_filter, QueryFilterInfo);
    SETVTBL(vtbl, dshow_filter, JoinFilterGraph);
    SETVTBL(vtbl, dshow_filter, QueryVendorInfo);

    this->pin = dshow_pin_Create(this);

    this->priv_data = priv_data;
    this->callback  = callback;

    return 1;
}

static int dshow_filter_Cleanup(dshow_filter *this)
{
    dshow_pin_Release(this->pin);
    return 1;
}

DECLARE_CREATE(dshow_filter, dshow_filter_setup(this, priv_data, callback, type),
               void *priv_data, void *callback, int type)
DECLARE_DESTROY(dshow_filter, dshow_filter_Cleanup)


/*****************************************************************************
 * dshow_pin
 ****************************************************************************/
DECLARE_QUERYINTERFACE(dshow_pin, {{&IID_IUnknown,0}, {&IID_IPin,0},
                      {&IID_IMemInputPin,imemoffset}})
DECLARE_ADDREF(dshow_pin)
DECLARE_RELEASE(dshow_pin)

long WINAPI dshow_pin_Connect(dshow_pin *this, IPin *pin, const AM_MEDIA_TYPE *type)
{
    dshowdebug("dshow_pin_Connect(%p, %p, %p)\n", this, pin, type);
    /* Input pins receive connections. */
    return S_FALSE;
}

long WINAPI dshow_pin_ReceiveConnection(dshow_pin *this, IPin *pin,
                           const AM_MEDIA_TYPE *type)
{
    dshowdebug("dshow_pin_ReceiveConnection(%p)\n", this);

    if (!pin)
        return E_POINTER;
    if (this->connectedto)
        return VFW_E_ALREADY_CONNECTED;

    if (!IsEqualGUID(&type->majortype, &MEDIATYPE_Video))
            return VFW_E_TYPE_NOT_ACCEPTED;

    IPin_AddRef(pin);
    this->connectedto = pin;

    return S_OK;
}

long WINAPI dshow_pin_Disconnect(dshow_pin *this)
{
    dshowdebug("dshow_pin_Disconnect(%p)\n", this);

    if (this->filter->state != State_Stopped)
        return VFW_E_NOT_STOPPED;
    if (!this->connectedto)
        return S_FALSE;
    IPin_Release(this->connectedto);
    this->connectedto = NULL;

    return S_OK;
}

long WINAPI dshow_pin_ConnectedTo(dshow_pin *this, IPin **pin)
{
    dshowdebug("dshow_pin_ConnectedTo(%p)\n", this);

    if (!pin)
        return E_POINTER;
    if (!this->connectedto)
        return VFW_E_NOT_CONNECTED;
    IPin_AddRef(this->connectedto);
    *pin = this->connectedto;

    return S_OK;
}

long copy_dshow_media_type(AM_MEDIA_TYPE *dst, const AM_MEDIA_TYPE *src)
{
    uint8_t *pbFormat = NULL;

    if (src->cbFormat) {
        pbFormat = CoTaskMemAlloc(src->cbFormat);
        if (!pbFormat)
            return E_OUTOFMEMORY;
        memcpy(pbFormat, src->pbFormat, src->cbFormat);
    }

    *dst = *src;
    dst->pUnk = NULL;
    dst->pbFormat = pbFormat;

    return S_OK;
}

long WINAPI dshow_pin_ConnectionMediaType(dshow_pin *this, AM_MEDIA_TYPE *type)
{
    dshowdebug("dshow_pin_ConnectionMediaType(%p)\n", this);

    if (!type)
        return E_POINTER;
    if (!this->connectedto)
        return VFW_E_NOT_CONNECTED;

    return copy_dshow_media_type(type, &this->type);
}

long WINAPI dshow_pin_QueryPinInfo(dshow_pin *this, PIN_INFO *info)
{
    dshowdebug("dshow_pin_QueryPinInfo(%p)\n", this);

    if (!info)
        return E_POINTER;

    if (this->filter)
        dshow_filter_AddRef(this->filter);

    info->pFilter = (IBaseFilter *) this->filter;
    info->dir     = PINDIR_INPUT;
    wcscpy(info->achName, L"Capture");

    return S_OK;
}

long WINAPI dshow_pin_QueryDirection(dshow_pin *this, PIN_DIRECTION *dir)
{
    dshowdebug("dshow_pin_QueryDirection(%p)\n", this);
    if (!dir)
        return E_POINTER;
    *dir = PINDIR_INPUT;
    return S_OK;
}

long WINAPI dshow_pin_QueryId(dshow_pin *this, wchar_t **id)
{
    dshowdebug("dshow_pin_QueryId(%p)\n", this);

    if (!id)
        return E_POINTER;

    *id = wcsdup(L"libAV Pin");

    return S_OK;
}

long WINAPI dshow_pin_QueryAccept(dshow_pin *this, const AM_MEDIA_TYPE *type)
{
    dshowdebug("dshow_pin_QueryAccept(%p)\n", this);
    return S_FALSE;
}

long WINAPI dshow_pin_EnumMediaTypes(dshow_pin *this, IEnumMediaTypes **enumtypes)
{
    const AM_MEDIA_TYPE *type = NULL;
    dshow_enum_media_types *new;
    dshowdebug("dshow_pin_EnumMediaTypes(%p)\n", this);

    if (!enumtypes)
        return E_POINTER;
    new = dshow_enum_media_types_Create(type);
    if (!new)
        return E_OUTOFMEMORY;

    *enumtypes = (IEnumMediaTypes *) new;
    return S_OK;
}

long WINAPI dshow_pin_QueryInternalConnections(dshow_pin *this, IPin **pin,
                                  unsigned long *npin)
{
    dshowdebug("dshow_pin_QueryInternalConnections(%p)\n", this);
    return E_NOTIMPL;
}

long WINAPI dshow_pin_EndOfStream(dshow_pin *this)
{
    dshowdebug("dshow_pin_EndOfStream(%p)\n", this);
    /* I don't care. */
    return S_OK;
}

long WINAPI dshow_pin_BeginFlush(dshow_pin *this)
{
    dshowdebug("dshow_pin_BeginFlush(%p)\n", this);
    /* I don't care. */
    return S_OK;
}

long WINAPI dshow_pin_EndFlush(dshow_pin *this)
{
    dshowdebug("dshow_pin_EndFlush(%p)\n", this);
    /* I don't care. */
    return S_OK;
}

long WINAPI dshow_pin_NewSegment(dshow_pin *this, REFERENCE_TIME start,
                                 REFERENCE_TIME stop, double rate)
{
    dshowdebug("dshow_pin_NewSegment(%p)\n", this);
    /* I don't care. */
    return S_OK;
}

static int dshow_pin_setup(dshow_pin *this, dshow_filter *filter)
{
    IPinVtbl *vtbl = this->vtbl;
    IMemInputPinVtbl *imemvtbl;

    if (!filter)
        return 0;

    imemvtbl = malloc(sizeof(IMemInputPinVtbl));
    if (!imemvtbl)
        return 0;

    SETVTBL(imemvtbl, dshow_input_pin, QueryInterface);
    SETVTBL(imemvtbl, dshow_input_pin, AddRef);
    SETVTBL(imemvtbl, dshow_input_pin, Release);
    SETVTBL(imemvtbl, dshow_input_pin, GetAllocator);
    SETVTBL(imemvtbl, dshow_input_pin, NotifyAllocator);
    SETVTBL(imemvtbl, dshow_input_pin, GetAllocatorRequirements);
    SETVTBL(imemvtbl, dshow_input_pin, Receive);
    SETVTBL(imemvtbl, dshow_input_pin, ReceiveMultiple);
    SETVTBL(imemvtbl, dshow_input_pin, ReceiveCanBlock);

    this->imemvtbl = imemvtbl;

    SETVTBL(vtbl, dshow_pin, QueryInterface);
    SETVTBL(vtbl, dshow_pin, AddRef);
    SETVTBL(vtbl, dshow_pin, Release);
    SETVTBL(vtbl, dshow_pin, Connect);
    SETVTBL(vtbl, dshow_pin, ReceiveConnection);
    SETVTBL(vtbl, dshow_pin, Disconnect);
    SETVTBL(vtbl, dshow_pin, ConnectedTo);
    SETVTBL(vtbl, dshow_pin, ConnectionMediaType);
    SETVTBL(vtbl, dshow_pin, QueryPinInfo);
    SETVTBL(vtbl, dshow_pin, QueryDirection);
    SETVTBL(vtbl, dshow_pin, QueryId);
    SETVTBL(vtbl, dshow_pin, QueryAccept);
    SETVTBL(vtbl, dshow_pin, EnumMediaTypes);
    SETVTBL(vtbl, dshow_pin, QueryInternalConnections);
    SETVTBL(vtbl, dshow_pin, EndOfStream);
    SETVTBL(vtbl, dshow_pin, BeginFlush);
    SETVTBL(vtbl, dshow_pin, EndFlush);
    SETVTBL(vtbl, dshow_pin, NewSegment);

    this->filter = filter;

    return 1;
}

static inline void nothing(void *foo)
{
}

DECLARE_CREATE(dshow_pin, dshow_pin_setup(this, filter), dshow_filter *filter)
DECLARE_DESTROY(dshow_pin, nothing)


/*****************************************************************************
 * dshow_input_pin
 ****************************************************************************/
long WINAPI dshow_input_pin_QueryInterface(dshow_input_pin *this,
                                           const GUID *riid, void **ppvObject)
{
    struct dshow_pin *pin = (struct dshow_pin *) ((uint8_t *) this - imemoffset);
    dshowdebug("dshow_input_pin_QueryInterface(%p)\n", this);
    return dshow_pin_QueryInterface(pin, riid, ppvObject);
}

unsigned long WINAPI dshow_input_pin_AddRef(dshow_input_pin *this)
{
    struct dshow_pin *pin = (struct dshow_pin *) ((uint8_t *) this - imemoffset);
    dshowdebug("dshow_input_pin_AddRef(%p)\n", this);
    return dshow_pin_AddRef(pin);
}

unsigned long WINAPI dshow_input_pin_Release(dshow_input_pin *this)
{
    struct dshow_pin *pin = (struct dshow_pin *) ((uint8_t *) this - imemoffset);
    dshowdebug("dshow_input_pin_Release(%p)\n", this);
    return dshow_pin_Release(pin);
}

long WINAPI dshow_input_pin_GetAllocator(dshow_input_pin *this, IMemAllocator **alloc)
{
    dshowdebug("dshow_input_pin_GetAllocator(%p)\n", this);
    return VFW_E_NO_ALLOCATOR;
}

long WINAPI dshow_input_pin_NotifyAllocator(dshow_input_pin *this, IMemAllocator *alloc,
                                 BOOL rdwr)
{
    dshowdebug("dshow_input_pin_NotifyAllocator(%p)\n", this);
    return S_OK;
}

long WINAPI dshow_input_pin_GetAllocatorRequirements(dshow_input_pin *this,
                                          ALLOCATOR_PROPERTIES *props)
{
    dshowdebug("dshow_input_pin_GetAllocatorRequirements(%p)\n", this);
    return E_NOTIMPL;
}

long WINAPI dshow_input_pin_Receive(dshow_input_pin *this, IMediaSample *sample)
{
    struct dshow_pin *pin = (struct dshow_pin *) ((uint8_t *) this - imemoffset);
    void *priv_data;
    uint8_t *buf;
    int buf_size; /* todo should be a long? */
    int index;
    int64_t curtime;
    int64_t orig_curtime;
    int64_t graphtime;
    IReferenceClock *clock = pin->filter->clock;
    int64_t dummy;
    struct dshow_ctx *c;


    dshowdebug("dshow_input_pin_Receive(%p)\n", this);

    if (!sample)
        return E_POINTER;

    IMediaSample_GetTime(sample, &orig_curtime, &dummy);
    orig_curtime += pin->filter->start_time;
    IReferenceClock_GetTime(clock, &graphtime);
    IReferenceClock_GetTime(clock, &curtime);

    buf_size = IMediaSample_GetActualDataLength(sample);
    IMediaSample_GetPointer(sample, &buf);
    priv_data = pin->filter->priv_data;
    c = priv_data;
    index = pin->filter->stream_index;

    pin->filter->callback(priv_data, index, buf, buf_size, curtime, 0);

    return S_OK;
}

long WINAPI dshow_input_pin_ReceiveMultiple(dshow_input_pin *this,
                                 IMediaSample **samples, long n, long *nproc)
{
    int i;
    dshowdebug("dshow_input_pin_ReceiveMultiple(%p)\n", this);

    for (i = 0; i < n; i++)
        dshow_input_pin_Receive(this, samples[i]);

    *nproc = n;
    return S_OK;
}

long WINAPI dshow_input_pin_ReceiveCanBlock(dshow_input_pin *this)
{
    dshowdebug("dshow_input_pin_ReceiveCanBlock(%p)\n", this);
    /* I swear I will not block. */
    return S_FALSE;
}

void dshow_input_pin_Destroy(dshow_input_pin *this)
{
    struct dshow_pin *pin = (struct dshow_pin *) ((uint8_t *) this - imemoffset);
    dshowdebug("dshow_input_pin_Destroy(%p)\n", this);
    dshow_pin_Destroy(pin);
}

/*****************************************************************************
 * dshow_enum_media
 ****************************************************************************/

DECLARE_QUERYINTERFACE(dshow_enum_media_types,
    { {&IID_IUnknown,0}, {&IID_IEnumMediaTypes,0} })
DECLARE_ADDREF(dshow_enum_media_types)
DECLARE_RELEASE(dshow_enum_media_types)

long WINAPI dshow_enum_media_types_Next(dshow_enum_media_types *this, unsigned long n,
                         AM_MEDIA_TYPE **types, unsigned long *fetched)
{
    int count = 0;
    dshowdebug("dshow_enum_media_types_Next(%p)\n", this);
    if (!types)
        return E_POINTER;
    if (!this->pos && n == 1) {
        if (!IsEqualGUID(&this->type.majortype, &GUID_NULL)) {
            AM_MEDIA_TYPE *type = malloc(sizeof(AM_MEDIA_TYPE));
            if (!type)
                return E_OUTOFMEMORY;
            copy_dshow_media_type(type, &this->type);
            *types = type;
            count = 1;
        }
        this->pos = 1;
    }
    if (fetched)
        *fetched = count;
    if (!count)
        return S_FALSE;
    return S_OK;
}

long WINAPI dshow_enum_media_types_Skip(dshow_enum_media_types *this, unsigned long n)
{
    dshowdebug("dshow_enum_media_types_Skip(%p)\n", this);
    if (n) /* Any skip will always fall outside of the only valid type. */
        return S_FALSE;
    return S_OK;
}

long WINAPI dshow_enum_media_types_Reset(dshow_enum_media_types *this)
{
    dshowdebug("dshow_enum_media_types_Reset(%p)\n", this);
    this->pos = 0;
    return S_OK;
}


long WINAPI dshow_enum_media_types_Clone(dshow_enum_media_types *this, dshow_enum_media_types **enums)
{
    dshow_enum_media_types *new;
    dshowdebug("dshow_enum_media_types_Clone(%p)\n", this);
    if (!enums)
        return E_POINTER;
    new = dshow_enum_media_types_Create(&this->type);
    if (!new)
        return E_OUTOFMEMORY;
    new->pos = this->pos;
    *enums = new;
    return S_OK;
}


static int dshow_enum_media_types_Setup(dshow_enum_media_types *this, const AM_MEDIA_TYPE *type)
{
    IEnumMediaTypesVtbl *vtbl = this->vtbl;
    SETVTBL(vtbl, dshow_enum_media_types, QueryInterface);
    SETVTBL(vtbl, dshow_enum_media_types, AddRef);
    SETVTBL(vtbl, dshow_enum_media_types, Release);
    SETVTBL(vtbl, dshow_enum_media_types, Next);
    SETVTBL(vtbl, dshow_enum_media_types, Skip);
    SETVTBL(vtbl, dshow_enum_media_types, Reset);
    SETVTBL(vtbl, dshow_enum_media_types, Clone);

    if (!type) {
        this->type.majortype = GUID_NULL;
    } else {
        copy_dshow_media_type(&this->type, type);
    }

    return 1;
}
DECLARE_CREATE(dshow_enum_media_types, dshow_enum_media_types_Setup(this, type), const AM_MEDIA_TYPE *type)
DECLARE_DESTROY(dshow_enum_media_types, nothing)
/*****************************************************************************
 * xxxx
 ****************************************************************************/

static int create_dshow_graph(struct dshow_ctx *c)
{
    int ret;
    ret = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC,
                    &IID_IGraphBuilder, (void **)&c->graph);
    if (FAILED(ret)) {
        printf("CoCreateInstance IID_IGraphBuilder failed!\n");
        goto exit;
    }

    ret = CoCreateInstance(&CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC,
                    &IID_ICaptureGraphBuilder2, (void **)&c->capgraph);
    if (FAILED(ret)) {
        printf("CoCreateInstance IID_ICaptureGraphBuilder2 failed!\n");
        goto exit;
    }

    ret = ICaptureGraphBuilder2_SetFiltergraph(c->capgraph, c->graph);
    if (FAILED(ret)) {
        printf("Could not set graph for CaptureGraphBuilder2\n");
        goto exit;
    }

    ret = IGraphBuilder_QueryInterface(c->graph, &IID_IMediaControl,
                    (void **)&c->mctrl);
    if (FAILED(ret)) {
        printf("QueryInterface IID_IMediaControl failed\n");
        goto exit;
    }

exit:
    return ret;
}

static int find_device(struct dshow_ctx *c)
{
    int ret;
    ICreateDevEnum *devenum = NULL;
    IEnumMoniker *classenum = NULL;
    IBaseFilter *device_filter = NULL;
    IMoniker *m = NULL;

    ret = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
                    &IID_ICreateDevEnum, (void **)&devenum);
    if (FAILED(ret)) {
        printf("CoCreateInstance IID_ICreateDevEnum failed!\n");
        goto exit;
    }

    ret = ICreateDevEnum_CreateClassEnumerator(devenum, &CLSID_VideoInputDeviceCategory,
                    (IEnumMoniker **)&classenum, 0);
    if (FAILED(ret)) {
        printf("failed to create class enumerator\n");
        goto exit;
    }

    while (!device_filter && IEnumMoniker_Next(classenum, 1, &m, NULL) == S_OK) {
        IPropertyBag *bag = NULL;
        char *friendly_name = NULL;
        char *unique_name = NULL;
        VARIANT var;
        IBindCtx *bind_ctx = NULL;
        LPOLESTR olestr = NULL;
        LPMALLOC co_malloc = NULL;

        ret = CoGetMalloc(1, &co_malloc);
        if (ret != S_OK)
            goto fail1;
        ret = CreateBindCtx(0, &bind_ctx);
        if (ret != S_OK)
            goto fail1;

        ret = IMoniker_GetDisplayName(m, bind_ctx, NULL, &olestr);
        if (ret != S_OK)
            goto fail1;
        unique_name = dup_wchar_to_utf8(olestr);
        ret = IMoniker_BindToStorage(m, 0, 0, &IID_IPropertyBag, (void *)&bag);
        if (ret != S_OK)
            goto fail1;

        var.vt = VT_BSTR;
        ret = IPropertyBag_Read(bag, L"FriendlyName", &var, NULL);
        if (ret != S_OK)
            goto fail1;
        friendly_name = dup_wchar_to_utf8(var.bstrVal);

        ret = IMoniker_BindToObject(m, 0, 0, &IID_IBaseFilter, (void *)&device_filter);
        if (ret != S_OK) {
            printf("Unable to BindToObject for %s\n", c->name);
            goto fail1;
        }
        c->unique_name = strdup(unique_name);

        printf("friendly_name=\"%s\"\n", friendly_name);
        //printf("unique_name=\"%s\"\n", unique_name);
        free(unique_name);

fail1:
        if (olestr && co_malloc)
            IMalloc_Free(co_malloc, olestr);
        if (bind_ctx)
            IBindCtx_Release(bind_ctx);
        free(friendly_name);
        if (bag)
            IPropertyBag_Release(bag);
        IMoniker_Release(m);
    }
    IEnumMoniker_Release(classenum);
    if (device_filter) {
        c->dev_filter = device_filter;
    }

exit:
    return ret;
}

static int enum_device(struct dshow_ctx *c)
{
    int ret;
    IEnumPins *pins = NULL;
    IPin *device_pin = NULL;
    IPin *pin;

    ret = IBaseFilter_EnumPins(c->dev_filter, &pins);
    if (ret != S_OK) {
        printf("Could not enumerate pins.\n");
        goto exit;
    }

    while (!device_pin && IEnumPins_Next(pins, 1, &pin, NULL) == S_OK) {
        IKsPropertySet *p = NULL;
        IEnumMediaTypes *types = NULL;
        PIN_INFO info = {0};
        AM_MEDIA_TYPE *type;
        GUID category;
        DWORD r2;
        wchar_t *pin_id = NULL;
        char *pin_buf = NULL;

        IPin_QueryPinInfo(pin, &info);
        IBaseFilter_Release(info.pFilter);

        if (info.dir != PINDIR_OUTPUT)
            goto next;
        if (IPin_QueryInterface(pin, &IID_IKsPropertySet, (void **) &p) != S_OK)
            goto next;
        if (IKsPropertySet_Get(p, &AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY,
                               NULL, 0, &category, sizeof(GUID), &r2) != S_OK)
            goto next;
        if (!IsEqualGUID(&category, &PIN_CATEGORY_CAPTURE))
            goto next;

        ret = IPin_QueryId(pin, &pin_id);
        if (ret != S_OK) {
            printf("Could not query pin id\n");
            goto exit;
        }
        pin_buf = dup_wchar_to_utf8(pin_id);

        if (IPin_EnumMediaTypes(pin, &types) != S_OK)
            goto next;
        IEnumMediaTypes_Reset(types);

        while (!device_pin && IEnumMediaTypes_Next(types, 1, &type, NULL) == S_OK) {
            if (IsEqualGUID(&type->majortype, &MEDIATYPE_Video)) {
                device_pin = pin;
                goto next;
            }
            CoTaskMemFree(type);
        }

next:
        if (types)
            IEnumMediaTypes_Release(types);
        if (p)
            IKsPropertySet_Release(p);
        if (device_pin != pin)
            IPin_Release(pin);
        free(pin_buf);
        if (pin_id)
            CoTaskMemFree(pin_id);
    }
    IEnumPins_Release(pins);
    if (!device_pin) {
        printf("could not find capture device\n");
        ret = -1;
    } else {
        c->device_pin = device_pin;
    }

exit:
    return ret;
}

static void dshow_callback(void *priv_data, int index, uint8_t *buf, int buf_size,
                           int64_t time, int devtype)
{
    struct dshow_ctx *c = priv_data;

    //printf("%s index=%d, buf=%p, buf_size=%d, time=%d, devtype=%d\n", __func__,
    //       index, buf, buf_size, time, devtype);

    file_write(fp, buf, buf_size);
#if 0
    SetEvent(c->event[1]);
#endif

    return;
}

static int open_device(struct dshow_ctx *c)
{
    int ret;
    HANDLE media_event_handle;
    HANDLE proc;

    c->cap_filter = dshow_filter_Create(c, dshow_callback, VideoDevice);
    if (!c->cap_filter) {
        printf("dshow_filter_Create failed!\n");
        goto exit;
    }

    ret = IGraphBuilder_AddFilter(c->graph, c->dev_filter, NULL);
    if (ret != S_OK) {
        printf("Could not add device filter to graph.\n");
        goto exit;
    }

    ret = IGraphBuilder_AddFilter(c->graph, (IBaseFilter *)c->cap_filter, NULL);
    if (ret != S_OK) {
        printf("Could not add device filter to graph.\n");
        goto exit;
    }

    dshow_pin_AddRef(c->cap_filter->pin);

    ret = ICaptureGraphBuilder2_RenderStream(c->capgraph, NULL, NULL,
                  (IUnknown *)c->device_pin, NULL, (IBaseFilter *)c->cap_filter);
    /* connect pins, optionally insert intermediate filters like crossbar if necessary */

    if (ret != S_OK) {
        printf("Could not RenderStream to connect pins\n");
        goto exit;
    }

    c->mutex = CreateMutex(NULL, 0, NULL);
    if (!c->mutex) {
        printf("Could not create Mutex\n");
        goto exit;
    }
    c->event[1] = CreateEvent(NULL, 1, 0, NULL);
    if (!c->event[1]) {
        printf("Could not create Event\n");
        goto exit;
    }

    ret = IGraphBuilder_QueryInterface(c->graph, &IID_IMediaControl, (void **)&c->mctrl);
    if (ret != S_OK) {
        printf("Could not get media control.\n");
        goto exit;
    }

    ret = IGraphBuilder_QueryInterface(c->graph, &IID_IMediaEvent, (void **)&c->mevent);
    if (ret != S_OK) {
        printf("Could not get media event.\n");
        goto exit;
    }

    ret = IMediaEvent_GetEventHandle(c->mevent, (void *)&media_event_handle);
    if (ret != S_OK) {
        printf("Could not get media event handle.\n");
        goto exit;
    }
    proc = GetCurrentProcess();
    ret = DuplicateHandle(proc, media_event_handle, proc, &c->event[0],
                        0, 0, DUPLICATE_SAME_ACCESS);
    if (!ret) {
        printf("Could not duplicate media event handle.\n");
        goto exit;
    }

    return 0;

exit:
    return ret;
}

static void *dshow_open(struct uvc_ctx *uvc, const char *dev, uint32_t width, uint32_t height)
{
    struct dshow_ctx *c = calloc(1, sizeof(struct dshow_ctx));
    if (!c) {
        printf("malloc dshow_ctx failed!\n");
        return NULL;
    }
    c->name = strdup(dev);
    c->width = width;
    c->height = height;
    uvc->format = VIDEO_FORMAT_I420;

    CoInitialize(0);

    fp = file_open("uvc.yuv", F_CREATE);
    if (create_dshow_graph(c)) {
        printf("create_dshow_graph failed!\n");
        goto exit;
    }
    if (find_device(c)) {
        printf("find_device failed!\n");
        goto exit;
    }
    if (enum_device(c)) {
        printf("enum_device failed\n");
        goto exit;
    }
    if (open_device(c)) {
        printf("open_device failed!\n");
        goto exit;
    }

    return c;
exit:
    if (c)
        free(c);
    return NULL;
}

static void dshow_close(struct uvc_ctx *uvc)
{
    CoUninitialize();
}

static int dshow_dequeue(struct uvc_ctx *uvc, struct video_frame *frame)
{
    return 0;
}

static int dshow_enqueue(struct uvc_ctx *uvc, void *buf, size_t len)
{
    return 0;
}

static int dshow_ioctl(struct uvc_ctx *uvc, unsigned long int cmd, ...)
{
    return 0;
}

static int dshow_start_stream(struct uvc_ctx *uvc)
{
    int ret;
    struct dshow_ctx *c = (struct dshow_ctx *)uvc->opaque;

    ret = IMediaControl_Run(c->mctrl);
    if (ret == S_FALSE) {
        OAFilterState pfs;
        ret = IMediaControl_GetState(c->mctrl, 0, &pfs);
        return -1;
    }
    if (ret != S_OK) {
        printf("Could not run graph (sometimes caused by a device already"
                "in use by other application)\n");
        return -1;
    }
    return 0;
}

static int dshow_stop_stream(struct uvc_ctx *uvc)
{
    int ret;
    struct dshow_ctx *c = (struct dshow_ctx *)uvc->opaque;
    IMediaControl_Stop(c->mctrl);
    return 0;
}

struct uvc_ops dshow_ops = {
    /*.open        = */ dshow_open,
    /*.close       = */ dshow_close,
    /*.dequeue     = */ dshow_dequeue,
    /*.enqueue     = */ dshow_enqueue,
    /*.ioctl       = */ dshow_ioctl,
    /*.printf_info = */ NULL,
    /*.start_stream= */ dshow_start_stream,
    /*.stop_stream = */ dshow_stop_stream,
};
