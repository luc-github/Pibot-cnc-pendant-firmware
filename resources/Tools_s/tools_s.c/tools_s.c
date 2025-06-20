
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

#ifndef LV_ATTRIBUTE_TOOLS_S
#define LV_ATTRIBUTE_TOOLS_S
#endif

static const
LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_TOOLS_S
uint8_t tools_s_map[] = {

    0x75,0xf7,0xbb,0x01,0x3c,0xef,0xeb,0xff,0x4d,0xf1,0xe6,0x97,0xe7,0xf6,0xfb,0xfb,
    0xd3,0xf4,0xf8,0xfb,0x4e,0xf1,0xe8,0x56,0xff,0xf7,0xff,0x84,0x5b,0xf2,0xdf,0x2e,
    0xff,0xf7,0xff,0xcd,0xff,0xf7,0xff,0xcf,0xff,0xf7,0xff,0xff,0xf6,0xf7,0xfe,0xff,
    0xbc,0xf3,0xf5,0xff,0xa0,0xf2,0xf2,0xff,0x8a,0xf1,0xf0,0xff,0x53,0xf0,0xec,0xff,

    0x00,0x00,0x00,0x00,0x05,0x52,0x25,0x50,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x72,0x11,0xec,0xcc,0xe1,0x27,0x00,0x00,0x00,
    0x00,0x68,0x93,0x33,0x41,0xca,0xaa,0xab,0xcf,0x20,0x66,0x00,
    0x00,0x6a,0xaa,0xaa,0xa1,0xca,0xaa,0xaa,0xab,0xd4,0xa8,0x00,
    0x00,0x04,0xaa,0xaa,0xa1,0xd4,0x44,0xba,0xaa,0xaa,0xa9,0x00,
    0x00,0x2d,0xaa,0xaa,0xa1,0x11,0x11,0xfc,0xba,0xaa,0xa3,0x00,
    0x07,0xfb,0xaa,0xaa,0xa1,0xe1,0x1e,0x11,0x4a,0xaa,0xa3,0x70,
    0x02,0xca,0xab,0x4a,0xad,0xa1,0x13,0xc4,0xaa,0xaa,0xa3,0x20,
    0x0f,0xba,0xac,0x14,0xbb,0xa1,0x1b,0xbb,0xaa,0xaa,0xa4,0x10,
    0x5e,0xaa,0xbf,0x11,0x1a,0xad,0xea,0xaf,0x11,0x11,0x11,0x15,
    0x2c,0xaa,0x41,0x11,0x13,0xaa,0xaa,0xb1,0x11,0x1d,0xcc,0xe5,
    0x2c,0xaa,0x41,0x11,0x1e,0xba,0xaa,0xe1,0x11,0x14,0xaa,0xc2,
    0x2c,0xaa,0x41,0x11,0x11,0xda,0xad,0x11,0x11,0x14,0xaa,0xc2,
    0x2e,0xcc,0xd1,0x11,0x11,0xda,0xad,0x11,0x11,0x14,0xaa,0xc5,
    0x51,0x11,0x11,0x11,0x11,0xda,0xae,0x11,0x11,0xfb,0xaa,0xe5,
    0x01,0x4a,0xaa,0xaa,0xb1,0xda,0xae,0x1b,0x41,0xca,0xab,0x10,
    0x02,0x3a,0xaa,0xaa,0x41,0xd3,0x3e,0x1a,0xa4,0xba,0xac,0x20,
    0x07,0x3a,0xaa,0xa4,0x11,0xdb,0xbe,0x1a,0xaa,0xaa,0xbf,0x70,
    0x00,0x3a,0xaa,0xab,0xcf,0x1d,0xe1,0x1a,0xaa,0xaa,0xd2,0x00,
    0x00,0x8a,0xaa,0xaa,0xab,0x44,0x4d,0x1a,0xaa,0xaa,0x40,0x00,
    0x00,0x8a,0x4d,0xba,0xaa,0xaa,0xac,0x1a,0xaa,0xaa,0xa6,0x00,
    0x00,0x66,0x02,0xfc,0xba,0xaa,0xac,0x14,0x33,0x39,0x86,0x00,
    0x00,0x00,0x00,0x72,0x1e,0xcc,0xce,0x1f,0x27,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x05,0x22,0x22,0x50,0x00,0x00,0x00,0x00,

};

const lv_image_dsc_t tools_s = {
  .header.magic = LV_IMAGE_HEADER_MAGIC,
  .header.cf = LV_COLOR_FORMAT_I4,
  .header.flags = 0,
  .header.w = 24,
  .header.h = 24,
  .header.stride = 12,
  .data_size = sizeof(tools_s_map),
  .data = tools_s_map,
};

