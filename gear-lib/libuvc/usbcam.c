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
#include "libuvc.h"
#include <libusb-1.0/libusb.h>
#include <unistd.h>
#include <pthread.h>
#include "usbcam.h"

struct ucam_ctx {
    struct ucam_dev_info    dev_info;
    struct libusb_context  *usb_ctx;
    struct libusb_device   *usb_dev;
    struct libusb_device_handle *usb_devh;
    struct libusb_transfer *usb_xfer;
    struct ucam_stream_handle *strmh;
    uvc_status_callback_t  *status_cb;
    void                   *status_user_ptr;
    uvc_button_callback_t  *button_cb;
    void                   *button_user_ptr;
    uint8_t                 status_buf[32];
    uint32_t                claimed;
    pthread_t               tid;
    int                     thread_run;
};


static int ucam_parse_vs(struct ucam_ctx *c, struct ucam_dev_info *info,
                struct ucam_stream *stream,
                const unsigned char *block, size_t block_size)
{
    int ret;
    int descriptor_subtype;
    const unsigned char *p;
    int i;

    ret = 0;
    descriptor_subtype = block[2];

    switch (descriptor_subtype) {
    case UVC_VS_INPUT_HEADER:
        stream->bEndpointAddress = block[6] & 0x8f;
        stream->bTerminalLink = block[8];
        break;
    case UVC_VS_OUTPUT_HEADER:
        printf("unsupported descriptor subtype VS_OUTPUT_HEADER\n");
        break;
    case UVC_VS_STILL_IMAGE_FRAME:
        printf("unsupported descriptor subtype VS_STILL_IMAGE_FRAME\n");
        break;
    case UVC_VS_FORMAT_UNCOMPRESSED:
        info->format.bDescriptorSubtype = block[2];
        info->format.bFormatIndex       = block[3];
        //info->format.bmCapabilities   = block[4];
        //info->format.bmFlags          = block[5];
        memcpy(info->format.guidFormat, &block[5], 16);
        info->format.bBitsPerPixel      = block[21];
        info->format.bDefaultFrameIndex = block[22];
        info->format.bAspectRatioX      = block[23];
        info->format.bAspectRatioY      = block[24];
        info->format.bmInterlaceFlags   = block[25];
        info->format.bCopyProtect       = block[26];
        break;
    case UVC_VS_FORMAT_MJPEG:
        info->format.bDescriptorSubtype = block[2];
        info->format.bFormatIndex       = block[3];
        memcpy(info->format.fourccFormat, "MJPG", 4);
        info->format.bmFlags            = block[5];
        info->format.bBitsPerPixel      = 0;
        info->format.bDefaultFrameIndex = block[6];
        info->format.bAspectRatioX      = block[7];
        info->format.bAspectRatioY      = block[8];
        info->format.bmInterlaceFlags   = block[9];
        info->format.bCopyProtect       = block[10];
        break;
    case UVC_VS_FRAME_UNCOMPRESSED:
    case UVC_VS_FRAME_MJPEG:
        info->frame.bDescriptorSubtype = block[2];
        info->frame.bFrameIndex        = block[3];
        info->frame.bmCapabilities     = block[4];
        info->frame.wWidth             = block[5] + (block[6] << 8);
        info->frame.wHeight            = block[7] + (block[8] << 8);
        info->frame.dwMinBitRate       = DW_TO_INT(&block[9]);
        info->frame.dwMaxBitRate       = DW_TO_INT(&block[13]);
        info->frame.dwMaxVideoFrameBufferSize = DW_TO_INT(&block[17]);
        info->frame.dwDefaultFrameInterval = DW_TO_INT(&block[21]);
        info->frame.bFrameIntervalType = block[25];

        if (block[25] == 0) {
            info->frame.dwMinFrameInterval  = DW_TO_INT(&block[26]);
            info->frame.dwMaxFrameInterval  = DW_TO_INT(&block[30]);
            info->frame.dwFrameIntervalStep = DW_TO_INT(&block[34]);
        } else {
            info->frame.intervals = calloc(block[25] + 1, sizeof(info->frame.intervals[0]));
            p = &block[26];
            for (i = 0; i < block[25]; ++i) {
                info->frame.intervals[i] = DW_TO_INT(p);
                p += 4;
            }
            info->frame.intervals[block[25]] = 0;
        }
        break;
    case UVC_VS_FORMAT_MPEG2TS:
        printf("unsupported descriptor subtype VS_FORMAT_MPEG2TS\n");
        break;
    case UVC_VS_FORMAT_DV:
        printf("unsupported descriptor subtype VS_FORMAT_DV\n");
        break;
    case UVC_VS_COLORFORMAT:
        printf("unsupported descriptor subtype VS_COLORFORMAT\n");
        break;
    case UVC_VS_FORMAT_FRAME_BASED:
        info->format.bDescriptorSubtype   = block[2];
        info->format.bFormatIndex         = block[3];
        info->format.bNumFrameDescriptors = block[4];
        memcpy(info->format.guidFormat, &block[5], 16);
        info->format.bBitsPerPixel        = block[21];
        info->format.bDefaultFrameIndex   = block[22];
        info->format.bAspectRatioX        = block[23];
        info->format.bAspectRatioY        = block[24];
        info->format.bmInterlaceFlags     = block[25];
        info->format.bCopyProtect         = block[26];
        info->format.bVariableSize        = block[27];
        break;
    case UVC_VS_FRAME_FRAME_BASED:
        info->frame.bDescriptorSubtype = block[2];
        info->frame.bFrameIndex        = block[3];
        info->frame.bmCapabilities     = block[4];
        info->frame.wWidth             = block[5] + (block[6] << 8);
        info->frame.wHeight            = block[7] + (block[8] << 8);
        info->frame.dwMinBitRate       = DW_TO_INT(&block[9]);
        info->frame.dwMaxBitRate       = DW_TO_INT(&block[13]);
        info->frame.dwDefaultFrameInterval = DW_TO_INT(&block[17]);
        info->frame.bFrameIntervalType = block[21];
        info->frame.dwBytesPerLine     = DW_TO_INT(&block[22]);
        if (block[21] == 0) {
            info->frame.dwMinFrameInterval  = DW_TO_INT(&block[26]);
            info->frame.dwMaxFrameInterval  = DW_TO_INT(&block[30]);
            info->frame.dwFrameIntervalStep = DW_TO_INT(&block[34]);
        } else {
            info->frame.intervals = calloc(block[21] + 1, sizeof(info->frame.intervals[0]));
            p = &block[26];
            for (i = 0; i < block[21]; ++i) {
                info->frame.intervals[i] = DW_TO_INT(p);
                p += 4;
            }
            info->frame.intervals[block[21]] = 0;
        }
        break;
    case UVC_VS_FORMAT_STREAM_BASED:
        printf("unsupported descriptor subtype VS_FORMAT_STREAM_BASED\n");
        break;
    default:
        /** @todo handle JPEG and maybe still frames or even DV... */
        printf("unsupported descriptor subtype: %d\n", descriptor_subtype);
        break;
    }

    return ret;
}

static int ucam_scan_streaming(struct ucam_ctx *c, struct ucam_dev_info *info,
                int if_idx)
{
    const struct libusb_interface_descriptor *if_desc;
    const unsigned char *buffer;
    size_t buffer_left, block_size;
    int ret, parse_ret;

    ret = 0;

    if_desc = &(info->usb_conf->interface[if_idx].altsetting[0]);
    buffer = if_desc->extra;
    buffer_left = if_desc->extra_length;

    info->stream.bInterfaceNumber = if_desc->bInterfaceNumber;

    while (buffer_left >= 3) {
        block_size = buffer[0];
        parse_ret = ucam_parse_vs(c, info, &info->stream, buffer, block_size);
        if (parse_ret != 0) {
            ret = parse_ret;
            break;
        }

        buffer_left -= block_size;
        buffer += block_size;
    }

    return ret;
}

static int ucam_parse_vc_header(struct ucam_ctx *c, struct ucam_dev_info *info,
                const unsigned char *block, size_t block_size)
{
    size_t i;
    int scan_ret, ret = 0;

    /*
       int uvc_version;
       uvc_version = (block[4] >> 4) * 1000 + (block[4] & 0x0f) * 100
       + (block[3] >> 4) * 10 + (block[3] & 0x0f);
     */
    info->control.bcdUVC = SW_TO_SHORT(&block[3]);

    switch (info->control.bcdUVC) {
    case 0x0100:
        info->control.dwClockFrequency = DW_TO_INT(block + 7);
        break;
    case 0x010a:
        info->control.dwClockFrequency = DW_TO_INT(block + 7);
        break;
    case 0x0110:
        break;
    default:
        break;
    }

    for (i = 12; i < block_size; ++i) {
        scan_ret = ucam_scan_streaming(c, info, block[i]);
        if (scan_ret != 0) {
            ret = scan_ret;
            break;
        }
    }

    return ret;
}

static int ucam_parse_vc_input_terminal(struct ucam_ctx *c,
                struct ucam_dev_info *info,
                const unsigned char *block, size_t block_size)
{
    size_t i;

    /* only supporting camera-type input terminals */
    if (SW_TO_SHORT(&block[4]) != UVC_ITT_CAMERA) {
        printf("only support camera-type input terminals\n");
        return -1;
    }

    info->control.term.bTerminalID = block[3];
    info->control.term.wTerminalType = SW_TO_SHORT(&block[4]);
    info->control.term.wObjectiveFocalLengthMin = SW_TO_SHORT(&block[8]);
    info->control.term.wObjectiveFocalLengthMax = SW_TO_SHORT(&block[10]);
    info->control.term.wOcularFocalLength = SW_TO_SHORT(&block[12]);

    for (i = 14 + block[14]; i >= 15; --i)
        info->control.term.bmControls = block[i] + (info->control.term.bmControls << 8);

    return 0;
}

static int ucam_get_dev_list(struct ucam_ctx *c)
{
    struct libusb_device **dev_list;
    struct libusb_device *pdev;
    struct libusb_config_descriptor *conf;
    struct libusb_device_descriptor desc;
    const struct libusb_interface *intf;
    const struct libusb_interface_descriptor *if_desc;
    uint8_t got_dev;
    int alt_idx;
    int num_uvc_devices;
    int dev_idx;
    int if_idx;

    int num_usb_devices = libusb_get_device_list(c->usb_ctx, &dev_list);
    if (num_usb_devices < 0) {
        printf("libusb_get_device_list failed!\n");
        return -1;
    }

    num_uvc_devices = 0;
    dev_idx = -1;

    while ((pdev = dev_list[++dev_idx]) != NULL) {
        got_dev = 0;
        if (libusb_get_config_descriptor(pdev, 0, &conf) != 0)
            continue;

        if (libusb_get_device_descriptor(pdev, &desc) != LIBUSB_SUCCESS)
            continue;

        for (if_idx = 0; !got_dev && if_idx < conf->bNumInterfaces;
                ++if_idx) {
            intf = &conf->interface[if_idx];

            for (alt_idx = 0; !got_dev && alt_idx < intf->num_altsetting;
                    ++alt_idx) {
                if_desc = &intf->altsetting[alt_idx];

                // Skip TIS cameras that definitely aren't UVC even though they might
                // look that way
                if (0x199e == desc.idVendor &&
                    desc.idProduct >= 0x8201 && desc.idProduct <= 0x8208) {
                    continue;
                }

                // Special case for Imaging Source cameras
                /* Video, Streaming */
                if (0x199e == desc.idVendor &&
                    (0x8101 == desc.idProduct || 0x8102 == desc.idProduct) &&
                    if_desc->bInterfaceClass == 255 &&
                    if_desc->bInterfaceSubClass == 2 ) {
                    got_dev = 1;
                    printf("got special usb device 0x%x, 0x%x\n",
                                    desc.idVendor, desc.idProduct);
                }

                /* Video, Streaming */
                if (if_desc->bInterfaceClass == 14 &&
                    if_desc->bInterfaceSubClass == 2) {
                    got_dev = 1;
                    printf("got usb device 0x%x, 0x%x\n",
                                    desc.idVendor, desc.idProduct);
                }
            }
        }

        libusb_free_config_descriptor(conf);

        if (got_dev) {
            num_uvc_devices++;
            c->usb_dev = pdev;
            break;
        }
    }

    libusb_free_device_list(dev_list, 1);
    printf("find %d uvc devices\n", num_uvc_devices);
    if (!got_dev)
        return -1;
    return 0;
}

static int ucam_get_dev_desc(struct ucam_ctx *c, struct ucam_dev_info *info)
{
    unsigned char buf[64];
    struct libusb_device_descriptor usb_desc;
    if (libusb_get_device_descriptor(c->usb_dev, &usb_desc) != 0) {
        printf("libusb_get_device_descriptor failed!\n");
        return -1;
    }
    info->descriptor.idVendor = usb_desc.idVendor;
    info->descriptor.idProduct = usb_desc.idProduct;

    if (libusb_open(c->usb_dev, &c->usb_devh) != 0) {
        printf("libusb_open device %04x:%04x failed!\n",
                        usb_desc.idVendor, usb_desc.idProduct);
        return -1;
    }

    if (libusb_get_string_descriptor_ascii(c->usb_devh, usb_desc.iSerialNumber,
                            buf, sizeof(buf)) > 0) {
        info->descriptor.serialNumber = strdup((const char*) buf);
        printf("serialNumber %s\n", info->descriptor.serialNumber);
    }

    if (libusb_get_string_descriptor_ascii(c->usb_devh, usb_desc.iManufacturer,
                            buf, sizeof(buf)) > 0) {
        info->descriptor.manufacturer = strdup((const char*) buf);
        printf("manufacturer %s\n", info->descriptor.manufacturer);
    }

    if (libusb_get_string_descriptor_ascii(c->usb_devh, usb_desc.iProduct,
                            buf, sizeof(buf)) > 0) {
        info->descriptor.product = strdup((const char*) buf);
        printf("product %s\n", info->descriptor.product);
    }

    libusb_close(c->usb_devh);
    return 0;
}

static int ucam_claim_if(struct ucam_ctx *c, int idx)
{
    int ret = 0;

    /* Tell libusb to detach any active kernel drivers. libusb will keep track of whether
     * it found a kernel driver for this interface. */
    ret = libusb_detach_kernel_driver(c->usb_devh, idx);
    if (ret == 0 || ret == LIBUSB_ERROR_NOT_FOUND ||
        ret == LIBUSB_ERROR_NOT_SUPPORTED) {
        if (libusb_claim_interface(c->usb_devh, idx) != 0) {
            printf("libusb_claim_interface failed!");
        }
    } else {
        printf("not claiming interface %d: unable to detach kernel driver (%s)\n",
                            idx, libusb_strerror(ret));
    }
    printf("ucam_claim_if %s\n", libusb_strerror(ret));

    return 0;
}

static int ucam_parse_vc(struct ucam_ctx *c, struct ucam_dev_info *info,
                const unsigned char *block, size_t block_size)
{
    int descriptor_subtype;
    int ret = 0;
    int i;

    if (block[1] != 36) { // not a CS_INTERFACE descriptor??
        printf("ucam_parse_vc invalid!\n");
        return -1;
    }

    descriptor_subtype = block[2];
    switch (descriptor_subtype) {
    case UVC_VC_HEADER:
        ret = ucam_parse_vc_header(c, info, block, block_size);
        break;
    case UVC_VC_INPUT_TERMINAL:
        ret = ucam_parse_vc_input_terminal(c, info, block, block_size);
        break;
    case UVC_VC_OUTPUT_TERMINAL:
        break;
    case UVC_VC_SELECTOR_UNIT:
        info->control.selector.bUnitID = block[3];
        break;
    case UVC_VC_PROCESSING_UNIT:
        info->control.processing.bUnitID   = block[3];
        info->control.processing.bSourceID = block[4];
        for (i = 7 + block[7]; i >= 8; --i)
            info->control.processing.bmControls = block[i] + (info->control.processing.bmControls << 8);
        break;
    case UVC_VC_EXTENSION_UNIT: {
        const uint8_t *start_of_controls;
        int size_of_controls, num_in_pins;
        info->control.extension.bUnitID = block[3];
        memcpy(info->control.extension.guidExtensionCode, &block[4], 16);
        num_in_pins = block[21];
        size_of_controls = block[22 + num_in_pins];
        start_of_controls = &block[23 + num_in_pins];
        for (i = size_of_controls - 1; i >= 0; --i)
            info->control.extension.bmControls = start_of_controls[i] + (info->control.extension.bmControls << 8);
        } break;
    default:
        ret = -1;
        break;
    }

    return ret;
}

static int ucam_get_devinfo(struct ucam_ctx *c, struct ucam_dev_info *info)
{
    int ret = 0;
    int is_TISCamera = 0;
    int if_idx;
    size_t buffer_left, block_size;
    const unsigned char *buffer;
    const struct libusb_interface_descriptor *if_desc;

    if (libusb_get_config_descriptor(c->usb_dev, 0, &info->usb_conf) != 0) {
        printf("libusb_get_config_descriptor failed!\n");
        return -1;
    }

    if (ucam_get_dev_desc(c, info) != 0) {
        printf("ucam_get_dev_desc failed!\n");
        return -1;
    }
    printf("got usb device 0x%x, 0x%x, %s\n",
            info->descriptor.idVendor, info->descriptor.idProduct, info->descriptor.serialNumber);

    if (0x199e == info->descriptor.idVendor &&
         (0x8101 == info->descriptor.idProduct ||
          0x8102 == info->descriptor.idProduct )) {
        is_TISCamera = 1;
    }

    for (if_idx = 0; if_idx < info->usb_conf->bNumInterfaces; ++if_idx) {
        if_desc = &info->usb_conf->interface[if_idx].altsetting[0];
        if (is_TISCamera &&
            if_desc->bInterfaceClass == 255 &&
            if_desc->bInterfaceSubClass == 1) // Video, Control
            break;
        if (if_desc->bInterfaceClass == 14 &&
            if_desc->bInterfaceSubClass == 1) // Video, Control
            break;
        if_desc = NULL;
    }

    if (if_desc == NULL) {
        printf("not found interface!\n");
        return -1;
    }

    info->control.bInterfaceNumber = if_idx;
    if (if_desc->bNumEndpoints != 0) {
        info->control.bEndpointAddress = if_desc->endpoint[0].bEndpointAddress;
    }

    buffer = if_desc->extra;
    buffer_left = if_desc->extra_length;
    block_size = 0;
    block_size = block_size;

    while (buffer_left >= 3) { // parseX needs to see buf[0,2] = length,type
        block_size = buffer[0];
        if (ucam_parse_vc(c, info, buffer, block_size) != 0) {
            ret = -1;
            break;
        }

        buffer_left -= block_size;
        buffer += block_size;
    }

    return ret;
}

static int ucam_find_device(struct ucam_ctx *c)
{
    if (ucam_get_dev_list(c) != 0) {
        printf("ucam_get_dev_list failed!\n");
        return -1;
    }

    if (ucam_get_devinfo(c, &c->dev_info) != 0) {
        printf("ucam_get_devinfo failed!\n");
        return -1;
    }
    return 0;
}

static void ucam_process_control_status(struct ucam_ctx *c, unsigned char *data, int len)
{
    enum uvc_status_class status_class;
    uint8_t originator = 0, selector = 0, event = 0;
    enum uvc_status_attribute attribute = UVC_STATUS_ATTRIBUTE_UNKNOWN;
    void *content = NULL;
    size_t content_len = 0;
    int found_entity = 0;

    if (len < 5) {
        printf("Short read of VideoControl status update (%d bytes)", len);
        return;
    }

    originator = data[1];
    event = data[2];
    selector = data[3];

    if (originator == 0) {
        printf("Unhandled update from VC interface");
        return;  /* @todo VideoControl virtual entity interface updates */
    }

    if (event != 0) {
        printf("Unhandled VC event %d", (int) event);
        return;
    }

    /* printf("bSelector: %d\n", selector); */
    if (c->dev_info.control.term.bTerminalID == originator) {
        status_class = UVC_STATUS_CLASS_CONTROL_CAMERA;
        found_entity = 1;
    }

    if (!found_entity) {
        if (c->dev_info.control.processing.bUnitID == originator) {
            status_class = UVC_STATUS_CLASS_CONTROL_PROCESSING;
            found_entity = 1;
        }
    }

    if (!found_entity) {
        printf("Got status update for unknown VideoControl entity %d",
                            (int) originator);
        return;
    }

    attribute = data[4];
    content = data + 5;
    content_len = len - 5;

    printf("Event: class=%d, event=%d, selector=%d, attribute=%d, content_len=%zd",
                    status_class, event, selector, attribute, content_len);

    if (c->status_cb) {
        printf("Running user-supplied status callback");
        c->status_cb(status_class,
                        event,
                        selector,
                        attribute,
                        content, content_len,
                        c->status_user_ptr);
    }
}

static void ucam_process_streaming_status(struct ucam_ctx *c, unsigned char *data, int len)
{
    if (len < 3) {
        printf("Invalid streaming status event received.\n");
        return;
    }

    if (data[2] == 0) {
        if (len < 4) {
            printf("Short read of status update (%d bytes)", len);
            return;
        }
        printf("Button (intf %u) %s len %d\n", data[1], data[3] ? "pressed" : "released", len);
        if (c->button_cb) {
            printf("Running user-supplied button callback");
            c->button_cb(data[1],
                                data[3],
                                c->button_user_ptr);
        }
    } else {
        printf("Stream %u error event %02x %02x len %d.\n", data[1], data[2], data[3], len);
    }
}

void ucam_process_status_xfer(struct ucam_ctx *c,
                struct libusb_transfer *transfer)
{
    /* printf("Got transfer of aLen = %d\n", transfer->actual_length); */
    if (transfer->actual_length > 0) {
        switch (transfer->buffer[0] & 0x0f) {
        case 1: /* VideoControl interface */
            ucam_process_control_status(c, transfer->buffer, transfer->actual_length);
            break;
        case 2:  /* VideoStreaming interface */
            ucam_process_streaming_status(c, transfer->buffer, transfer->actual_length);
            break;
        }
    }
}

static void _ucam_status_callback(struct libusb_transfer *transfer)
{
    struct ucam_ctx *c = (struct ucam_ctx *)transfer->user_data;
    switch (transfer->status) {
    case LIBUSB_TRANSFER_ERROR:
    case LIBUSB_TRANSFER_CANCELLED:
    case LIBUSB_TRANSFER_NO_DEVICE:
        printf("not processing/resubmitting, status = %d", transfer->status);
        return;
    case LIBUSB_TRANSFER_COMPLETED:
        ucam_process_status_xfer(c, transfer);
        break;
    case LIBUSB_TRANSFER_TIMED_OUT:
    case LIBUSB_TRANSFER_STALL:
    case LIBUSB_TRANSFER_OVERFLOW:
        printf("retrying transfer, status = %d", transfer->status);
        break;
    }
    libusb_submit_transfer(transfer);
}

static const char *_uvc_name_for_format_subtype(uint8_t subtype)
{
    switch (subtype) {
    case UVC_VS_FORMAT_UNCOMPRESSED:
        return "UncompressedFormat";
    case UVC_VS_FORMAT_MJPEG:
        return "MJPEGFormat";
    case UVC_VS_FORMAT_FRAME_BASED:
        return "FrameFormat";
    default:
        return "Unknown";
    }
}

static int ucam_print_info(struct ucam_ctx *c)
{
    struct ucam_ctrl *ctrl = &c->dev_info.control;
    struct ucam_descriptor *desc = &c->dev_info.descriptor;
    struct ucam_stream *stream = &c->dev_info.stream;
    struct ucam_frame_desc *frame = &c->dev_info.frame;
    struct ucam_format_desc *format = &c->dev_info.format;
    int stream_idx = 0;

    if (!ctrl->bcdUVC) {
        printf("usb bcdUVC is invalid!\n");
        return -1;
    }

    printf("DEVICE CONFIGURATION (%04x:%04x/%s) ---\n",
        desc->idVendor, desc->idProduct,
        desc->serialNumber ? desc->serialNumber : "[none]");

    printf("Status: %s\n", stream ? "streaming" : "idle");
    printf("VideoControl:\n"
                    "\tbcdUVC: 0x%04x\n",
                    ctrl->bcdUVC);

    ++stream_idx;

    printf("VideoStreaming(%d):\n"
                    "\tbEndpointAddress: %d\n\tFormats:\n",
                    stream_idx, stream->bEndpointAddress);

    int i;

    switch (format->bDescriptorSubtype) {
    case UVC_VS_FORMAT_UNCOMPRESSED:
    case UVC_VS_FORMAT_MJPEG:
    case UVC_VS_FORMAT_FRAME_BASED:
        printf("\t\%s(%d)\n"
                        "\t\t  bits per pixel: %d\n"
                        "\t\t  GUID: ",
                        _uvc_name_for_format_subtype(format->bDescriptorSubtype),
                        format->bFormatIndex,
                        format->bBitsPerPixel);

        for (i = 0; i < 16; ++i)
            printf("%02x", format->guidFormat[i]);

        printf(" (%4s)\n", format->fourccFormat);

        printf("\t\t  default frame: %d\n"
               "\t\t  aspect ratio: %dx%d\n"
               "\t\t  interlace flags: %02x\n"
               "\t\t  copy protect: %02x\n",
               format->bDefaultFrameIndex,
               format->bAspectRatioX,
               format->bAspectRatioY,
               format->bmInterlaceFlags,
               format->bCopyProtect);

        printf("\t\t\tFrameDescriptor(%d)\n"
               "\t\t\t  capabilities: %02x\n"
               "\t\t\t  size: %dx%d\n"
               "\t\t\t  bit rate: %d-%d\n"
               "\t\t\t  max frame size: %d\n"
               "\t\t\t  default interval: 1/%d\n",
               frame->bFrameIndex,
               frame->bmCapabilities,
               frame->wWidth,
               frame->wHeight,
               frame->dwMinBitRate,
               frame->dwMaxBitRate,
               frame->dwMaxVideoFrameBufferSize,
               10000000 / frame->dwDefaultFrameInterval);
        if (frame->intervals) {
            uint32_t *ptr;
            for (ptr = frame->intervals; *ptr; ++ptr) {
                printf("\t\t\t  interval[%d]: 1/%d\n",
                                (int) (ptr - frame->intervals),
                                10000000 / *ptr);
            }
        } else {
            printf("\t\t\t  min interval[%d] = 1/%d\n"
                   "\t\t\t  max interval[%d] = 1/%d\n",
                   frame->dwMinFrameInterval,
                   10000000 / frame->dwMinFrameInterval,
                   frame->dwMaxFrameInterval,
                   10000000 / frame->dwMaxFrameInterval);
            if (frame->dwFrameIntervalStep)
                printf("\t\t\t  interval step[%d] = 1/%d\n",
                        frame->dwFrameIntervalStep,
                        10000000 / frame->dwFrameIntervalStep);
        }
        break;
    default:
        printf("\t-UnknownFormat (%d)\n",
                            format->bDescriptorSubtype);
    }
    return 0;
}

struct format_table_entry {
    enum uvc_frame_format format;
    uint8_t abstract_fmt;
    uint8_t guid[16];
    int children_count;
    enum uvc_frame_format *children;
};

struct format_table_entry *_get_format_entry(enum uvc_frame_format format)
{
    #define ABS_FMT(_fmt, _num, ...) \
        case _fmt: { \
            static enum uvc_frame_format _fmt##_children[] = __VA_ARGS__; \
            static struct format_table_entry _fmt##_entry = { \
                _fmt, 0, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, _num, _fmt##_children }; \
            return &_fmt##_entry; }

    #define FMT(_fmt, ...) \
        case _fmt: { \
            static struct format_table_entry _fmt##_entry = { \
                    _fmt, 0, __VA_ARGS__, 0, NULL }; \
        return &_fmt##_entry; }

    switch(format) {
    /* Define new formats here */
    ABS_FMT(UVC_FRAME_FORMAT_ANY, 2,
        {UVC_FRAME_FORMAT_UNCOMPRESSED, UVC_FRAME_FORMAT_COMPRESSED})

    ABS_FMT(UVC_FRAME_FORMAT_UNCOMPRESSED, 3,
        {UVC_FRAME_FORMAT_YUYV, UVC_FRAME_FORMAT_UYVY, UVC_FRAME_FORMAT_GRAY8})
    FMT(UVC_FRAME_FORMAT_YUYV,
        {'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})
    FMT(UVC_FRAME_FORMAT_UYVY,
        {'U',  'Y',  'V',  'Y', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})
    FMT(UVC_FRAME_FORMAT_GRAY8,
        {'Y',  '8',  '0',  '0', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})
    FMT(UVC_FRAME_FORMAT_BY8,
        {'B',  'Y',  '8',  ' ', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71})

    ABS_FMT(UVC_FRAME_FORMAT_COMPRESSED, 1,
        {UVC_FRAME_FORMAT_MJPEG})
    FMT(UVC_FRAME_FORMAT_MJPEG,
        {'M',  'J',  'P',  'G'})
    default:
        return NULL;
    }

    #undef ABS_FMT
    #undef FMT
}

uint8_t _uvc_frame_format_matches_guid(enum uvc_frame_format fmt, uint8_t guid[16])
{
    struct format_table_entry *format;
    int child_idx;

    format = _get_format_entry(fmt);
    if (!format)
        return 0;

    if (!format->abstract_fmt && !memcmp(guid, format->guid, 16))
        return 1;

    for (child_idx = 0; child_idx < format->children_count; child_idx++) {
        if (_uvc_frame_format_matches_guid(format->children[child_idx], guid))
                return 1;
    }

  return 0;
}

static int uvc_get_stream_ctrl_format_size(
    struct ucam_ctx *c,
    struct ucam_stream_ctrl *ctrl,
    enum uvc_frame_format cf,
    int width, int height,
    int fps)
{
#if 0
  uvc_streaming_interface_t *stream_if;

  /* find a matching frame descriptor and interval */
  DL_FOREACH(devh->info->stream_ifs, stream_if) {
    uvc_format_desc_t *format;

    DL_FOREACH(stream_if->format_descs, format) {
      uvc_frame_desc_t *frame;

      if (!_uvc_frame_format_matches_guid(cf, format->guidFormat))
        continue;

      DL_FOREACH(format->frame_descs, frame) {
        if (frame->wWidth != width || frame->wHeight != height)
          continue;

        uint32_t *interval;

        if (frame->intervals) {
          for (interval = frame->intervals; *interval; ++interval) {
            // allow a fps rate of zero to mean "accept first rate available"
            if (10000000 / *interval == (unsigned int) fps || fps == 0) {

              /* get the max values -- we need the interface number to be able
                 to do this */
              ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
              uvc_query_stream_ctrl( devh, ctrl, 1, UVC_GET_MAX);

              ctrl->bmHint = (1 << 0); /* don't negotiate interval */
              ctrl->bFormatIndex = format->bFormatIndex;
              ctrl->bFrameIndex = frame->bFrameIndex;
              ctrl->dwFrameInterval = *interval;

              goto found;
            }
          }
        } else {
          uint32_t interval_100ns = 10000000 / fps;
          uint32_t interval_offset = interval_100ns - frame->dwMinFrameInterval;

          if (interval_100ns >= frame->dwMinFrameInterval
              && interval_100ns <= frame->dwMaxFrameInterval
              && !(interval_offset
                   && (interval_offset % frame->dwFrameIntervalStep))) {

            /* get the max values -- we need the interface number to be able
               to do this */
            ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
            uvc_query_stream_ctrl( devh, ctrl, 1, UVC_GET_MAX);

            ctrl->bmHint = (1 << 0);
            ctrl->bFormatIndex = format->bFormatIndex;
            ctrl->bFrameIndex = frame->bFrameIndex;
            ctrl->dwFrameInterval = interval_100ns;

            goto found;
          }
        }
      }
    }
  }

  return UVC_ERROR_INVALID_MODE;

found:
  return uvc_probe_stream_ctrl(devh, ctrl);
#endif
    return 0;
}
static void *ucam_handle_events(void *arg)
{
    struct ucam_ctx *c = (struct ucam_ctx *)arg;

    while (c->thread_run)
        libusb_handle_events_completed(c->usb_ctx, &c->thread_run);
    return NULL;
}

static void *ucam_open(struct uvc_ctx *uvc, const char *dev, struct uvc_config *conf)
{
    int ret;
    struct ucam_ctx *c = calloc(1, sizeof(struct ucam_ctx));
    if (!c) {
        printf("malloc ucam_ctx failed!\n");
        return NULL;
    }
    if (libusb_init(&c->usb_ctx) < 0) {
        printf("libusb_init failed!\n");
        return NULL;
    }
    libusb_set_debug(c->usb_ctx, LIBUSB_LOG_LEVEL_INFO);

    if (ucam_find_device(c) < 0) {
        printf("ucam_find_device failed!\n");
        return NULL;
    }
    ucam_print_info(c);

    ret = libusb_open(c->usb_dev, &c->usb_devh);
    if (ret < 0) {
        printf("libusb_open failed %s!\n", libusb_strerror(ret));
        return NULL;
    }
    //libusb_ref_device(c->usb_dev);


    if (ucam_claim_if(c, c->dev_info.control.bInterfaceNumber) != 0) {
        printf("ucam_claim_if failed!\n");
        return NULL;
    }

    if (c->dev_info.control.bEndpointAddress) {
        c->usb_xfer = libusb_alloc_transfer(0);
        if (!c->usb_xfer) {
            ret = -1;
            printf("libusb_alloc_transfer failed!\n");
            return NULL;
        }

        libusb_fill_interrupt_transfer(c->usb_xfer,
                        c->usb_devh,
                        c->dev_info.control.bEndpointAddress,
                        c->status_buf,
                        sizeof(c->status_buf),
                        _ucam_status_callback,
                        c,
                        0);
        ret = libusb_submit_transfer(c->usb_xfer);
        if (ret) {
            printf("libusb_submit_transfer failed: %s\n", libusb_strerror(ret));
            return NULL;
        }
        printf("libusb_submit_transfer: %s\n", libusb_strerror(ret));
    }
    c->thread_run = 1;
    pthread_create(&c->tid, NULL, ucam_handle_events, c);

    uvc->opaque = c;
    printf("ucam_open ok!\n");
    return c;
}

static void ucam_close(struct uvc_ctx *uvc)
{
    struct ucam_ctx *c = (struct ucam_ctx *)uvc->opaque;
    libusb_exit(c->usb_ctx);
    free(c);
}

static int ucam_stream_open_ctrl(struct ucam_ctx *c,
                struct ucam_stream_handle **strmhp,
                struct ucam_stream_ctrl *ctrl)
{
#if 0
    /* Chosen frame and format descriptors */
    uvc_stream_handle_t *strmh = NULL;
    uvc_streaming_interface_t *stream_if;
    uvc_error_t ret;

    UVC_ENTER();

    if (_uvc_get_stream_by_interface(devh, ctrl->bInterfaceNumber) != NULL) {
        ret = UVC_ERROR_BUSY; /* Stream is already opened */
        goto fail;
    }

    stream_if = _uvc_get_stream_if(devh, ctrl->bInterfaceNumber);
    if (!stream_if) {
            ret = UVC_ERROR_INVALID_PARAM;
            goto fail;
    }

    strmh = calloc(1, sizeof(*strmh));
    if (!strmh) {
            ret = UVC_ERROR_NO_MEM;
            goto fail;
    }
    strmh->devh = devh;
    strmh->stream_if = stream_if;
    strmh->frame.library_owns_data = 1;

    ret = uvc_claim_if(strmh->devh, strmh->stream_if->bInterfaceNumber);
    if (ret != UVC_SUCCESS)
            goto fail;

    ret = uvc_stream_ctrl(strmh, ctrl);
    if (ret != UVC_SUCCESS)
            goto fail;

    // Set up the streaming status and data space
    strmh->running = 0;
    /** @todo take only what we need */
    strmh->outbuf = malloc( LIBUVC_XFER_BUF_SIZE );
    strmh->holdbuf = malloc( LIBUVC_XFER_BUF_SIZE );

    pthread_mutex_init(&strmh->cb_mutex, NULL);
    pthread_cond_init(&strmh->cb_cond, NULL);

    DL_APPEND(devh->streams, strmh);

    *strmhp = strmh;

    UVC_EXIT(0);
    return UVC_SUCCESS;

fail:
    if(strmh)
            free(strmh);
    UVC_EXIT(ret);
    return ret;
#endif
    return 0;
}

static int ucam_start_stream(struct uvc_ctx *uvc)
{
    int ret;
    struct ucam_ctx *c = (struct ucam_ctx *)uvc->opaque;

    uvc_get_stream_ctrl_format_size(c, &c->dev_info.stream_ctrl, 
                    UVC_FRAME_FORMAT_YUYV, 640, 480, 30);

    ret = ucam_stream_open_ctrl(c, &c->strmh, &c->dev_info.stream_ctrl);
    if (ret != 0)
        return ret;

#if 0
    ret = ucam_stream_start(strmh, cb, user_ptr, flags);
    if (ret != 0) {
        ucam_stream_close(strmh);
        return ret;
    }
#endif

    return 0;
}

static int ucam_stop_stream(struct uvc_ctx *uvc)
{
    return 0;
}

static int ucam_query_frame(struct uvc_ctx *uvc, struct video_frame *frame)
{
    return 0;
}

static int ucam_ioctl(struct uvc_ctx *uvc, unsigned long int cmd, ...)
{
    return 0;
}

struct uvc_ops ucam_ops = {
    ._open        = ucam_open,
    ._close       = ucam_close,
    .ioctl        = ucam_ioctl,
    .start_stream = ucam_start_stream,
    .stop_stream  = ucam_stop_stream,
    .query_frame  = ucam_query_frame,
};
