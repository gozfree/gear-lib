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
#define COBJMACROS

#include "libuvc.h"
#include <stdio.h>
#include <stdlib.h>
#include <dshow.h>
#include <dvdmedia.h>
#include <strmif.h>
#include <ObjIdl.h>
#include <shlwapi.h>

#include "libposix4win.h"

#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"strmiids.lib")
#pragma comment(lib,"uuid.lib")
#pragma comment(lib,"oleaut32.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"libposix4win.lib")

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
};

static int create_dshow_graph(struct dshow_ctx *ctx)
{
    int ret;
    ret = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC,
                    &IID_IGraphBuilder, (void **)&ctx->graph);
    if (FAILED(ret)) {
        printf("CoCreateInstance IID_IGraphBuilder failed!\n");
        goto exit;
    }

    ret = CoCreateInstance(&CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC,
                    &IID_ICaptureGraphBuilder2, (void **)&ctx->capgraph);
    if (FAILED(ret)) {
        printf("CoCreateInstance IID_ICaptureGraphBuilder2 failed!\n");
        goto exit;
    }

    ret = ICaptureGraphBuilder2_SetFiltergraph(ctx->capgraph, ctx->graph);
    if (FAILED(ret)) {
        printf("Could not set graph for CaptureGraphBuilder2\n");
        goto exit;
    }

    ret = IGraphBuilder_QueryInterface(ctx->graph, &IID_IMediaControl,
                    (void **)&ctx->mctrl);
    if (FAILED(ret)) {
        printf("QueryInterface IID_IMediaControl failed\n");
        goto exit;
    }

exit:
    return ret;
}

static int find_device(struct dshow_ctx *ctx)
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
        int i;

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
            printf("Unable to BindToObject for %s\n", ctx->name);
            goto fail1;
        }
        ctx->unique_name = strdup(unique_name);

        printf("friendly_name=\"%s\"\n", friendly_name);
        printf("unique_name=\"%s\"\n", unique_name);
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
        ctx->dev_filter = device_filter;
    }

exit:
    return ret;
}

static int enum_device(struct dshow_ctx *ctx)
{
    int ret;
    IEnumPins *pins = NULL;
    IPin *device_pin = NULL;
    IPin *pin;

    ret = IBaseFilter_EnumPins(ctx->dev_filter, &pins);
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
        char *name_buf = NULL;
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
        name_buf = dup_wchar_to_utf8(info.achName);

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
                printf("Selecting pin %s\n", name_buf);
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
        free(name_buf);
        free(pin_buf);
        if (pin_id)
            CoTaskMemFree(pin_id);
    }
    IEnumPins_Release(pins);

exit:
    return ret;
}

static int open_device(struct dshow_ctx *ctx)
{
    int ret;

    ret = IGraphBuilder_AddFilter(ctx->graph, ctx->dev_filter, NULL);
    if (ret != S_OK) {
        printf("Could not add device filter to graph.\n");
        goto exit;
    }

exit:
    return ret;
}

static int GetInterfaces(struct dshow_ctx *ctx)
{
    int r;
    IGraphBuilder *graph = NULL;
    ICreateDevEnum *devenum = NULL;
    IMediaControl *mctrl = NULL;
    IMediaEvent *mevent = NULL;
    ICaptureGraphBuilder2 *capgraph = NULL;
    IStream *ifile_stream = NULL;
    IStream *ofile_stream = NULL;
    IPersistStream *pers_stream = NULL;
    IBaseFilter *device_filter = NULL;
    HANDLE mevent_handle;

    char *ifilename = "video input stream";
    char *ofilename = "video output stream";
    //const wchar_t *filter_name[1] = { "Video capture filter" };


    r = SHCreateStreamOnFile((LPCSTR)ifilename, STGM_READ, &ifile_stream);
    if (FAILED(r)) {
        printf("Could not open capture filter description file.\n");
        goto exit;
    }

    r = OleLoadFromStream(ifile_stream, &IID_IBaseFilter, (void **)&device_filter);
    if (FAILED(r)) {
        printf("Could not load capture filter from file.\n");
        goto exit;
    }

    r = IGraphBuilder_AddFilter(graph, device_filter, NULL);
    if (FAILED(r)) {
        printf("Could not add device filter to graph.\n");
        goto exit;
    }

    r = SHCreateStreamOnFile((LPCSTR)ofilename, STGM_CREATE|STGM_READWRITE, &ofile_stream);
    if (FAILED(r)) {
        printf("Could not create capture filter description file.\n");
        goto exit;
    }

    r = IBaseFilter_QueryInterface(device_filter, &IID_IPersistStream, (void **)&pers_stream);
    if (FAILED(r)) {
        printf("Query for IPersistStream failed.\n");
        goto exit;
    }

    r = OleSaveToStream(pers_stream, ofile_stream);
    if (FAILED(r)) {
        printf("Could not save capture filter \n");
        goto exit;
    }

    IStream_Commit(ofile_stream, STGC_DEFAULT);
    if (FAILED(r)) {
        printf("Could not commit capture filter data to file.\n");
        goto exit;
    }

#if 0
    r = IGraphBuilder_AddFilter(graph, (IBaseFilter *)capture_filter,
                                filter_name[devtype]);
    if (FAILED(r)) {
        printf("Could not add capture filter to graph\n");
        goto exit;
    }
#endif


#if 0
    r = ICaptureGraphBuilder2_RenderStream(capgraph, NULL, NULL, (IUnknown *) device_pin, NULL /* no intermediate filter */,
        (IBaseFilter *) capture_filter); /* connect pins, optionally insert intermediate filters like crossbar if necessary */
    if (r != S_OK) {
        printf("Could not RenderStream to connect pins\n");
        goto exit;
    }
#endif

    r = IGraphBuilder_QueryInterface(graph, &IID_IMediaEvent, (void **)&mevent);
    if (FAILED(r))
            return r;

    r = IMediaEvent_GetEventHandle(mevent, (void *) &mevent_handle);
    if (r != S_OK) {
        printf("Could not get media event handle.\n");
        goto exit;
    }

    r = IMediaControl_Run(mctrl);
    if (r == S_FALSE) {
        OAFilterState pfs;
        r = IMediaControl_GetState(mctrl, 0, &pfs);
    }

    return 0;

exit:
    return r;
}

static int dshow_init(struct dshow_ctx *ctx)
{
    CoInitialize(0);

    if (create_dshow_graph(ctx)) {
        printf("create_dshow_graph failed!\n");
    }
    if (find_device(ctx)) {
        printf("find_device failed!\n");
    }
    enum_device(ctx);
    open_device(ctx);
    if (0) {
    GetInterfaces(ctx);
    }
    return 0;
}


static void dshow_deinit()
{
    CoUninitialize();
}

static struct dshow_ctx *dshow_open(struct uvc_ctx *uvc, const char *dev, int width, int height)
{
    struct dshow_ctx *dc = calloc(1, sizeof(struct dshow_ctx));
    if (!dc) {
        printf("malloc dshow_ctx failed!\n");
        return NULL;
    }
    dc->name = strdup(dev);
    dc->width = width;
    dc->height = height;
    dshow_init(dc);

    return dc;
}

static void dshow_close(struct uvc_ctx *uvc)
{
    dshow_deinit();
}

struct uvc_ops dshow_ops = {
    dshow_open,
    dshow_close,
    NULL,
    NULL,
    NULL,
    NULL,
};
