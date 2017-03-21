-- This is the configuration of Jpeg Encode
-- md_enable (bool):        true|false, false means disable jpeg encode
-- md_source_type (string): "yuv" | "me1"
--   "yuv": capture yuv from DSP
--   "me1": get luma frame from motion detect callback
-- fps (float):             < 30.0 capture frame per second when motion detect
-- source_buffer:           0|1|2|3, effective only when md_source_type="yuv"
-- dump_yuv_enable (bool):  true|false
-- jpeg_path (string):      dir of jpeg files saved
-- yuv_path (string):       dir of yuv files saved, effective only when dump_yuv_enable
-- max_files (int):         max jpeg files saved

jpeg_encode = {

  type_1 = {
    type = "422",
    index = 1,
  },
  type_2 = {
    type = "420",
    index = 2,
    sub_type_1 = {
      my = "bbb",
    },
  },
  type_3 = {
    type = "420",
    index = 2,
    sub_type_1 = {
      my = "aaa",
    },
  },
  md_enable       = false,
  md_source_type  = "yuv",
  source_buffer   = 1,
  fps             = 10.0,
  dump_yuv_enable = false,
  jpeg_path       = "/tmp/jpeg",
  yuv_path        = "/tmp/yuv",
  max_files       = 50
}

return jpeg_encode
