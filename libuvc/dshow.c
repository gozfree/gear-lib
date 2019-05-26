
#include "libuvc.h"
#include <stdio.h>
#include <stdlib.h>
#include "libposix4win.h"
#include <windows.h>

#define COBJMACROS
#include <dshow.h>
#include <dvdmedia.h>
#include <strmif.h>
#include "objidl.h"
#include "shlwapi.h"

#pragma comment(lib,"psapi.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"strmiids.lib")
#pragma comment(lib,"uuid.lib")
#pragma comment(lib,"oleaut32.lib")
#pragma comment(lib,"shlwapi.lib")

struct dshow_ctx {
    int fd;
    IGraphBuilder *graph;
    IMediaControl *mctrl;
    IMediaEvent *mevent;

};

static int dshow_init(struct dshow_ctx *ctx)
{
    CoInitialize(0);

    GetInterfaces(ctx);
    return 0;
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
    const wchar_t *filter_name[1] = { "Video capture filter" };

    r = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC,
                         &IID_IGraphBuilder, (void **)&graph);
    if (FAILED(r)) {
        printf("CoCreateInstance IID_IGraphBuilder failed!\n");
        goto exit;
    }
    ctx->graph = graph;

    r = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
                         &IID_ICreateDevEnum, (void **)&devenum);
    if (FAILED(r)) {
        printf("CoCreateInstance IID_ICreateDevEnum failed!\n");
        goto exit;
    }

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

#if 0
    r = IStream_Commit(ofile_stream, STGC_DEFAULT);
    if (FAILED(r)) {
        printf("Could not commit capture filter data to file.\n");
        goto exit;
    }

    r = IGraphBuilder_AddFilter(graph, (IBaseFilter *)capture_filter,
                                filter_name[devtype]);
    if (FAILED(r)) {
        printf("Could not add capture filter to graph\n");
        goto exit;
    }
#endif


    r = CoCreateInstance(&CLSID_CaptureGraphBuilder2 , NULL, CLSCTX_INPROC,
                         &IID_ICaptureGraphBuilder2, (void **)&capgraph);
    if (FAILED(r)) {
        printf("CoCreateInstance IID_ICaptureGraphBuilder2 failed!\n");
        goto exit;
    }
    ICaptureGraphBuilder2_SetFiltergraph(capgraph, graph);
    if (r != S_OK) {
        printf("Could not set graph for CaptureGraphBuilder2\n");
        goto exit;
    }

#if 0
    r = ICaptureGraphBuilder2_RenderStream(capgraph, NULL, NULL, (IUnknown *) device_pin, NULL /* no intermediate filter */,
        (IBaseFilter *) capture_filter); /* connect pins, optionally insert intermediate filters like crossbar if necessary */
    if (r != S_OK) {
        printf("Could not RenderStream to connect pins\n");
        goto exit;
    }
#endif

    r = IGraphBuilder_QueryInterface(graph, &IID_IMediaControl, (void **)&mctrl);
    if (FAILED(r))
            return r;

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
