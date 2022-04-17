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
#ifndef USBCAM_H
#define USBCAM_H
/*
 * refer to https://github.com/libuvc/libuvc
 */

/** Converts an unaligned four-byte little-endian integer into an int32 */
#define DW_TO_INT(p) ((p)[0] | ((p)[1] << 8) | ((p)[2] << 16) | ((p)[3] << 24))
/** Converts an unaligned two-byte little-endian integer into an int16 */
#define SW_TO_SHORT(p) ((p)[0] | ((p)[1] << 8))
/** Converts an int16 into an unaligned two-byte little-endian integer */
#define SHORT_TO_SW(s, p) \
  (p)[0] = (s); \
  (p)[1] = (s) >> 8;
/** Converts an int32 into an unaligned four-byte little-endian integer */
#define INT_TO_DW(i, p) \
  (p)[0] = (i); \
  (p)[1] = (i) >> 8; \
  (p)[2] = (i) >> 16; \
  (p)[3] = (i) >> 24;

/** VideoControl interface descriptor subtype (A.5) */
enum uvc_vc_desc_subtype {
  UVC_VC_DESCRIPTOR_UNDEFINED = 0x00,
  UVC_VC_HEADER = 0x01,
  UVC_VC_INPUT_TERMINAL = 0x02,
  UVC_VC_OUTPUT_TERMINAL = 0x03,
  UVC_VC_SELECTOR_UNIT = 0x04,
  UVC_VC_PROCESSING_UNIT = 0x05,
  UVC_VC_EXTENSION_UNIT = 0x06
};

/** VideoStreaming interface descriptor subtype (A.6) */
enum uvc_vs_desc_subtype {
  UVC_VS_UNDEFINED = 0x00,
  UVC_VS_INPUT_HEADER = 0x01,
  UVC_VS_OUTPUT_HEADER = 0x02,
  UVC_VS_STILL_IMAGE_FRAME = 0x03,
  UVC_VS_FORMAT_UNCOMPRESSED = 0x04,
  UVC_VS_FRAME_UNCOMPRESSED = 0x05,
  UVC_VS_FORMAT_MJPEG = 0x06,
  UVC_VS_FRAME_MJPEG = 0x07,
  UVC_VS_FORMAT_MPEG2TS = 0x0a,
  UVC_VS_FORMAT_DV = 0x0c,
  UVC_VS_COLORFORMAT = 0x0d,
  UVC_VS_FORMAT_FRAME_BASED = 0x10,
  UVC_VS_FRAME_FRAME_BASED = 0x11,
  UVC_VS_FORMAT_STREAM_BASED = 0x12
};

/** UVC endpoint descriptor subtype (A.7) */
enum uvc_ep_desc_subtype {
  UVC_EP_UNDEFINED = 0x00,
  UVC_EP_GENERAL = 0x01,
  UVC_EP_ENDPOINT = 0x02,
  UVC_EP_INTERRUPT = 0x03
};

/** VideoControl interface control selector (A.9.1) */
enum uvc_vc_ctrl_selector {
  UVC_VC_CONTROL_UNDEFINED = 0x00,
  UVC_VC_VIDEO_POWER_MODE_CONTROL = 0x01,
  UVC_VC_REQUEST_ERROR_CODE_CONTROL = 0x02
};

/** Terminal control selector (A.9.2) */
enum uvc_term_ctrl_selector {
  UVC_TE_CONTROL_UNDEFINED = 0x00
};

/** Selector unit control selector (A.9.3) */
enum uvc_su_ctrl_selector {
  UVC_SU_CONTROL_UNDEFINED = 0x00,
  UVC_SU_INPUT_SELECT_CONTROL = 0x01
};

/** Extension unit control selector (A.9.6) */
enum uvc_xu_ctrl_selector {
  UVC_XU_CONTROL_UNDEFINED = 0x00
};

/** VideoStreaming interface control selector (A.9.7) */
enum uvc_vs_ctrl_selector {
  UVC_VS_CONTROL_UNDEFINED = 0x00,
  UVC_VS_PROBE_CONTROL = 0x01,
  UVC_VS_COMMIT_CONTROL = 0x02,
  UVC_VS_STILL_PROBE_CONTROL = 0x03,
  UVC_VS_STILL_COMMIT_CONTROL = 0x04,
  UVC_VS_STILL_IMAGE_TRIGGER_CONTROL = 0x05,
  UVC_VS_STREAM_ERROR_CODE_CONTROL = 0x06,
  UVC_VS_GENERATE_KEY_FRAME_CONTROL = 0x07,
  UVC_VS_UPDATE_FRAME_SEGMENT_CONTROL = 0x08,
  UVC_VS_SYNC_DELAY_CONTROL = 0x09
};

/** USB terminal type (B.1) */
enum uvc_term_type {
  UVC_TT_VENDOR_SPECIFIC = 0x0100,
  UVC_TT_STREAMING = 0x0101
};

/** Input terminal type (B.2) */
enum uvc_it_type {
  UVC_ITT_VENDOR_SPECIFIC = 0x0200,
  UVC_ITT_CAMERA = 0x0201,
  UVC_ITT_MEDIA_TRANSPORT_INPUT = 0x0202
};

/** Output terminal type (B.3) */
enum uvc_ot_type {
  UVC_OTT_VENDOR_SPECIFIC = 0x0300,
  UVC_OTT_DISPLAY = 0x0301,
  UVC_OTT_MEDIA_TRANSPORT_OUTPUT = 0x0302
};

/** External terminal type (B.4) */
enum uvc_et_type {
  UVC_EXTERNAL_VENDOR_SPECIFIC = 0x0400,
  UVC_COMPOSITE_CONNECTOR = 0x0401,
  UVC_SVIDEO_CONNECTOR = 0x0402,
  UVC_COMPONENT_CONNECTOR = 0x0403
};

/** Status packet type (2.4.2.2) */
enum uvc_status_type {
  UVC_STATUS_TYPE_CONTROL = 1,
  UVC_STATUS_TYPE_STREAMING = 2
};

enum uvc_status_class {
  UVC_STATUS_CLASS_CONTROL = 0x10,
  UVC_STATUS_CLASS_CONTROL_CAMERA = 0x11,
  UVC_STATUS_CLASS_CONTROL_PROCESSING = 0x12,
};

enum uvc_status_attribute {
  UVC_STATUS_ATTRIBUTE_VALUE_CHANGE = 0x00,
  UVC_STATUS_ATTRIBUTE_INFO_CHANGE = 0x01,
  UVC_STATUS_ATTRIBUTE_FAILURE_CHANGE = 0x02,
  UVC_STATUS_ATTRIBUTE_UNKNOWN = 0xff
};

enum uvc_frame_format {
  UVC_FRAME_FORMAT_UNKNOWN = 0,
  /** Any supported format */
  UVC_FRAME_FORMAT_ANY = 0,
  UVC_FRAME_FORMAT_UNCOMPRESSED,
  UVC_FRAME_FORMAT_COMPRESSED,
  /** YUYV/YUV2/YUV422: YUV encoding with one luminance value per pixel and
   * one UV (chrominance) pair for every two pixels.
   */
  UVC_FRAME_FORMAT_YUYV,
  UVC_FRAME_FORMAT_UYVY,
  /** 24-bit RGB */
  UVC_FRAME_FORMAT_RGB,
  UVC_FRAME_FORMAT_BGR,
  /** Motion-JPEG (or JPEG) encoded images */
  UVC_FRAME_FORMAT_MJPEG,
  UVC_FRAME_FORMAT_GRAY8,
  UVC_FRAME_FORMAT_BY8,
  /** Number of formats understood */
  UVC_FRAME_FORMAT_COUNT,
};

/** Payload header flags (2.4.3.3) */
#define UVC_STREAM_EOH (1 << 7)
#define UVC_STREAM_ERR (1 << 6)
#define UVC_STREAM_STI (1 << 5)
#define UVC_STREAM_RES (1 << 4)
#define UVC_STREAM_SCR (1 << 3)
#define UVC_STREAM_PTS (1 << 2)
#define UVC_STREAM_EOF (1 << 1)
#define UVC_STREAM_FID (1 << 0)

/** Control capabilities (4.1.2) */
#define UVC_CONTROL_CAP_GET (1 << 0)
#define UVC_CONTROL_CAP_SET (1 << 1)
#define UVC_CONTROL_CAP_DISABLED (1 << 2)
#define UVC_CONTROL_CAP_AUTOUPDATE (1 << 3)
#define UVC_CONTROL_CAP_ASYNCHRONOUS (1 << 4)

struct ucam_frame_desc {
    /** Type of frame, such as JPEG frame or uncompressed frme */
    enum uvc_vs_desc_subtype bDescriptorSubtype;
    /** Index of the frame within the list of specs available for this format */
    uint8_t bFrameIndex;
    uint8_t bmCapabilities;
    /** Image width */
    uint16_t wWidth;
    /** Image height */
    uint16_t wHeight;
    /** Bitrate of corresponding stream at minimal frame rate */
    uint32_t dwMinBitRate;
    /** Bitrate of corresponding stream at maximal frame rate */
    uint32_t dwMaxBitRate;
    /** Maximum number of bytes for a video frame */
    uint32_t dwMaxVideoFrameBufferSize;
    /** Default frame interval (in 100ns units) */
    uint32_t dwDefaultFrameInterval;
    /** Minimum frame interval for continuous mode (100ns units) */
    uint32_t dwMinFrameInterval;
    /** Maximum frame interval for continuous mode (100ns units) */
    uint32_t dwMaxFrameInterval;
    /** Granularity of frame interval range for continuous mode (100ns) */
    uint32_t dwFrameIntervalStep;
    /** Frame intervals */
    uint8_t bFrameIntervalType;
    /** number of bytes per line */
    uint32_t dwBytesPerLine;
    /** Available frame rates, zero-terminated (in 100ns units) */
    uint32_t *intervals;
};

struct ucam_format_desc {
    /** Type of image stream, such as JPEG or uncompressed. */
    enum uvc_vs_desc_subtype bDescriptorSubtype;
    /** Identifier of this format within the VS interface's format list */
    uint8_t bFormatIndex;
    uint8_t bNumFrameDescriptors;
    /** Format specifier */
    union {
        uint8_t guidFormat[16];
        uint8_t fourccFormat[4];
    };
    /** Format-specific data */
    union {
        /** BPP for uncompressed stream */
        uint8_t bBitsPerPixel;
        /** Flags for JPEG stream */
        uint8_t bmFlags;
    };
    /** Default {uvc_frame_desc} to choose given this format */
    uint8_t bDefaultFrameIndex;
    uint8_t bAspectRatioX;
    uint8_t bAspectRatioY;
    uint8_t bmInterlaceFlags;
    uint8_t bCopyProtect;
    uint8_t bVariableSize;
    /** Available frame specifications for this format */
    struct ucam_frame_desc *frame_descs;
};

struct ucam_input_terminal {
    /** Index of the terminal within the device */
    uint8_t bTerminalID;
    /** Type of terminal (e.g., camera) */
    enum uvc_it_type wTerminalType;
    uint16_t wObjectiveFocalLengthMin;
    uint16_t wObjectiveFocalLengthMax;
    uint16_t wOcularFocalLength;
    /** Camera controls (meaning of bits given in {uvc_ct_ctrl_selector}) */
    uint64_t bmControls;
};

struct ucam_stream_ctrl {
    uint16_t bmHint;
    uint8_t bFormatIndex;
    uint8_t bFrameIndex;
    uint32_t dwFrameInterval;
    uint16_t wKeyFrameRate;
    uint16_t wPFrameRate;
    uint16_t wCompQuality;
    uint16_t wCompWindowSize;
    uint16_t wDelay;
    uint32_t dwMaxVideoFrameSize;
    uint32_t dwMaxPayloadTransferSize;
    uint32_t dwClockFrequency;
    uint8_t bmFramingInfo;
    uint8_t bPreferredVersion;
    uint8_t bMinVersion;
    uint8_t bMaxVersion;
    uint8_t bInterfaceNumber;
};

struct ucam_frame {
  /** Image data for this frame */
  void *data;
  /** Size of image data buffer */
  size_t data_bytes;
  /** Width of image in pixels */
  uint32_t width;
  /** Height of image in pixels */
  uint32_t height;
  /** Pixel data format */
  enum uvc_frame_format frame_format;
  /** Number of bytes per horizontal line (undefined for compressed format) */
  size_t step;
  /** Frame number (may skip, but is strictly monotonically increasing) */
  uint32_t sequence;
  /** Estimate of system time when the device started capturing the image */
  struct timeval capture_time;
  /** Handle on the device that produced the image.
   * @warning You must not call any uvc_* functions during a callback. */
  //uvc_device_handle_t *source;
  /** Is the data buffer owned by the library?
   * If 1, the data buffer can be arbitrarily reallocated by frame conversion
   * functions.
   * If 0, the data buffer will not be reallocated or freed by the library.
   * Set this field to zero if you are supplying the buffer.
   */
  uint8_t library_owns_data;
} uvc_frame_t;

struct ucam_ctrl {
#if 0
    struct uvc_device_info *parent;
    struct uvc_input_terminal *input_term_descs;
    // struct uvc_output_terminal *output_term_descs;
    struct uvc_selector_unit *selector_unit_descs;
    struct uvc_processing_unit *processing_unit_descs;
    struct uvc_extension_unit *extension_unit_descs;
#endif
    struct ucam_input_terminal term;
    struct ucam_selector_unit {
        /** Index of the selector unit within the device */
        uint8_t bUnitID;
    } selector;

    struct ucam_processing_unit {
        struct uvc_processing_unit *prev, *next;
        /** Index of the processing unit within the device */
        uint8_t bUnitID;
        /** Index of the terminal from which the device accepts images */
        uint8_t bSourceID;
        /** Processing controls (meaning of bits given in {uvc_pu_ctrl_selector}) */
        uint64_t bmControls;
    } processing;

    struct ucam_extension_unit {
        /** Index of the extension unit within the device */
        uint8_t bUnitID;
        /** GUID identifying the extension unit */
        uint8_t guidExtensionCode[16];
        /** Bitmap of available controls (manufacturer-dependent) */
        uint64_t bmControls;
    } extension;

    uint16_t bcdUVC;
    uint32_t dwClockFrequency;
    uint8_t bEndpointAddress;
    /** Interface number */
    uint8_t bInterfaceNumber;
};

struct ucam_stream {
    /** Interface number */
    uint8_t bInterfaceNumber;
    /** Video formats that this interface provides */
    struct uvc_format_desc *format_descs;
    /** USB endpoint to use when communicating with this interface */
    uint8_t bEndpointAddress;
    uint8_t bTerminalLink;
};

struct ucam_descriptor {
    /** Vendor ID */
    uint16_t idVendor;
    /** Product ID */
    uint16_t idProduct;
    /** UVC compliance level, e.g. 0x0100 (1.0), 0x0110 */
    uint16_t bcdUVC;
    /** Serial number (null if unavailable) */
    const char *serialNumber;
    /** Device-reported manufacturer name (or null) */
    const char *manufacturer;
    /** Device-reporter product name (or null) */
    const char *product;
};

struct ucam_dev_info {
    struct libusb_config_descriptor *usb_conf;
    struct ucam_ctrl control;
    /** VideoStream interface */
    struct ucam_stream stream;
    struct ucam_stream_ctrl stream_ctrl;
    struct ucam_format_desc format;
    struct ucam_frame_desc frame;
    struct ucam_descriptor descriptor;
};

typedef void(ucam_frame_callback_t)(struct ucam_frame *frame, void *user_ptr);
typedef void(uvc_status_callback_t)(enum uvc_status_class status_class,
                                    int event,
                                    int selector,
                                    enum uvc_status_attribute status_attribute,
                                    void *data, size_t data_len,
                                    void *user_ptr);

typedef void(uvc_button_callback_t)(int button,
                                    int state,
                                    void *user_ptr);



/*
  set a high number of transfer buffers. This uses a lot of ram, but
  avoids problems with scheduling delays on slow boards causing missed
  transfers. A better approach may be to make the transfer thread FIFO
  scheduled (if we have root).
  We could/should change this to allow reduce it to, say, 5 by default
  and then allow the user to change the number of buffers as required.
 */
#define LIBUVC_NUM_TRANSFER_BUFS 100

#define LIBUVC_XFER_BUF_SIZE	( 16 * 1024 * 1024 )

struct ucam_stream_handle {
  struct uvc_device_handle *devh;
  struct ucam_stream_handle *prev, *next;
  struct uvc_streaming_interface *stream_if;

  /** if true, stream is running (streaming video to host) */
  uint8_t running;
  /** Current control block */
  struct ucam_stream_ctrl cur_ctrl;

  /* listeners may only access hold*, and only when holding a
   * lock on cb_mutex (probably signaled with cb_cond) */
  uint8_t fid;
  uint32_t seq, hold_seq;
  uint32_t pts, hold_pts;
  uint32_t last_scr, hold_last_scr;
  size_t got_bytes, hold_bytes;
  uint8_t *outbuf, *holdbuf;
  pthread_mutex_t cb_mutex;
  pthread_cond_t cb_cond;
  pthread_t cb_thread;
  uint32_t last_polled_seq;
  ucam_frame_callback_t *user_cb;
  void *user_ptr;
  struct libusb_transfer *transfers[LIBUVC_NUM_TRANSFER_BUFS];
  uint8_t *transfer_bufs[LIBUVC_NUM_TRANSFER_BUFS];
  struct ucam_frame frame;
  enum uvc_frame_format frame_format;
};

#endif
