
#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#elif defined(LV_BUILD_TEST)
#include "../lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif


#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_FILES_M
#define LV_ATTRIBUTE_FILES_M
#endif

static const
LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_FILES_M
uint8_t files_m_map[] = {

    0x4c,0x70,0x47,0x00,0xfb,0xfb,0xfb,0x36,0xfd,0xfd,0xfd,0xd5,0xe7,0xe7,0xe7,0x0e,
    0xff,0xff,0xff,0xf5,0xf9,0xf9,0xf9,0x59,0xf8,0xf8,0xf8,0x7d,0xfe,0xfe,0xfe,0x66,
    0xfc,0xfc,0xfc,0x92,0xff,0xff,0xff,0x9a,0xfe,0xfe,0xfe,0xe8,0xf0,0xf0,0xf0,0x25,
    0xff,0xff,0xff,0xa8,0xfd,0xfd,0xfd,0xbf,0xf7,0xf7,0xf7,0x48,0xff,0xff,0xff,0xff,

    0x00,0x00,0xbd,0xff,0xff,0xff,0xff,0xff,0x21,
    0x00,0x0b,0xaf,0xff,0xff,0xff,0xff,0xff,0xf4,
    0x00,0x1a,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0x01,0x4f,0xfc,0x3c,0xf8,0x3d,0xf7,0x32,0xff,
    0x14,0xff,0xf7,0x05,0xf1,0x07,0xf3,0x09,0xff,
    0x2f,0xff,0xf7,0x05,0xf1,0x07,0xf3,0x09,0xff,
    0xff,0xff,0xf6,0x07,0xfe,0x06,0xfb,0x0c,0xff,
    0xff,0xff,0xf4,0x9a,0xfa,0x94,0xf2,0xcf,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xf4,0x61,0x16,0x4c,0x11,0xe6,0xaf,0xff,
    0xff,0xf6,0x06,0x65,0xf8,0x05,0x73,0x32,0xff,
    0xff,0xf7,0x3a,0xff,0xf8,0x02,0xf2,0x05,0xff,
    0xff,0xfd,0x33,0x64,0xf8,0x02,0xff,0xb1,0xff,
    0xff,0xff,0x25,0x01,0xf8,0x02,0xff,0xb1,0xff,
    0xff,0xff,0xff,0x80,0x28,0x02,0xfd,0x06,0xff,
    0xff,0xfe,0x56,0x13,0x48,0x05,0x53,0xba,0xff,
    0xff,0xf9,0xe1,0x52,0xfd,0x55,0x78,0x4f,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0x9f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf4,
    0x39,0x4f,0xff,0xff,0xff,0xff,0xff,0xff,0x21,

};

const lv_image_dsc_t files_m = {
  .header.magic = LV_IMAGE_HEADER_MAGIC,
  .header.cf = LV_COLOR_FORMAT_I4,
  .header.flags = 0,
  .header.w = 18,
  .header.h = 24,
  .header.stride = 9,
  .data_size = sizeof(files_m_map),
  .data = files_m_map,
};

