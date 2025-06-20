
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

#ifndef LV_ATTRIBUTE_PROBE_M
#define LV_ATTRIBUTE_PROBE_M
#endif

static const
LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_PROBE_M
uint8_t probe_m_map[] = {

    0xff,0xf7,0xff,0x99,0x4c,0x70,0x47,0x00,0xff,0xf7,0xff,0xa4,0xff,0xf7,0xff,0x3d,
    0xff,0xf7,0xff,0xf5,0xff,0xf7,0xff,0xcb,0xff,0xf7,0xff,0x7b,0xff,0xf7,0xff,0x5c,
    0xff,0xf7,0xff,0x9f,0xff,0xf7,0xff,0x2f,0xff,0xf7,0xff,0x43,0xff,0xf7,0xff,0x8f,
    0xff,0xf7,0xff,0xf7,0xff,0xf7,0xff,0x87,0xff,0xf7,0xff,0xbf,0xff,0xf7,0xff,0xff,

    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
    0x11,0x11,0x78,0x91,0x11,0x11,0x11,0x11,0x19,0x87,0x11,0x11,
    0x11,0x11,0xff,0x01,0x11,0x11,0x11,0x11,0x10,0xff,0x11,0x11,
    0x11,0x11,0xff,0x01,0x11,0x11,0x11,0x11,0x10,0xff,0x11,0x11,
    0x11,0x11,0xff,0x01,0x11,0x11,0x11,0x11,0x10,0xff,0x11,0x11,
    0x11,0x11,0xff,0x01,0x11,0x11,0x11,0x11,0x10,0xff,0x11,0x11,
    0x11,0x11,0xff,0x01,0x11,0x11,0x11,0x11,0x10,0xff,0x11,0x11,
    0x11,0x11,0xff,0x01,0x11,0x11,0x11,0x11,0x10,0xff,0x11,0x11,
    0x11,0x11,0xff,0x01,0x11,0x11,0x11,0x11,0x10,0xff,0x11,0x11,
    0x65,0x31,0xff,0x01,0xd5,0x31,0x13,0x5d,0x10,0xff,0x13,0x56,
    0xff,0x4a,0xff,0x00,0xff,0x01,0x18,0xff,0x00,0xff,0xa4,0xff,
    0xbf,0xfc,0xff,0x4f,0xf4,0x31,0x13,0x4f,0xf4,0xff,0xcf,0xfb,
    0x10,0xff,0xff,0xff,0x43,0x11,0x11,0x34,0xff,0xff,0xff,0x01,
    0x11,0x0f,0xff,0xf4,0x31,0x11,0x11,0x13,0x4f,0xff,0xf0,0x11,
    0x11,0x10,0xff,0x43,0x11,0x11,0x11,0x11,0x34,0xff,0x01,0x11,
    0x11,0x11,0x6e,0x31,0x11,0x11,0x11,0x11,0x13,0xe6,0x11,0x11,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
    0x0f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf0,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0x72,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x27,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,

};

const lv_image_dsc_t probe_m = {
  .header.magic = LV_IMAGE_HEADER_MAGIC,
  .header.cf = LV_COLOR_FORMAT_I4,
  .header.flags = 0,
  .header.w = 24,
  .header.h = 21,
  .header.stride = 12,
  .data_size = sizeof(probe_m_map),
  .data = probe_m_map,
};

