
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

#ifndef LV_ATTRIBUTE_OK_B
#define LV_ATTRIBUTE_OK_B
#endif

static const
LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_OK_B
uint8_t ok_b_map[] = {

    0xff,0xff,0xff,0xf5,0x4c,0x70,0x47,0x00,0xff,0xff,0xff,0x3d,0xff,0xff,0xff,0x31,
    0xff,0xff,0xff,0x76,0xff,0xff,0xff,0xe9,0xff,0xff,0xff,0xea,0xff,0xff,0xff,0x80,
    0xff,0xff,0xff,0x70,0xff,0xff,0xff,0x2c,0xff,0xff,0xff,0x30,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,

    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x94,0x30,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x12,0x0b,0x50,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x20,0xbb,0x50,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x12,0x0b,0xb0,0x20,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x20,0xbb,0x02,0x10,
    0x11,0x11,0x11,0x11,0x11,0x11,0x12,0x0b,0xb0,0x21,0x10,
    0x34,0xa1,0x11,0x11,0x11,0x11,0x20,0xbb,0x02,0x11,0x10,
    0x6b,0x02,0x11,0x11,0x11,0x12,0x0b,0xb0,0x21,0x11,0x10,
    0x6b,0xb0,0x21,0x11,0x11,0x20,0xbb,0x02,0x11,0x11,0x10,
    0x20,0xbb,0x02,0x11,0x12,0x0b,0xb0,0x21,0x11,0x11,0x10,
    0x12,0x0b,0xb0,0x21,0x20,0xbb,0x02,0x11,0x11,0x11,0x10,
    0x11,0x20,0xbb,0x08,0x0b,0xb0,0x21,0x11,0x11,0x11,0x10,
    0x11,0x12,0x0b,0xbb,0xbb,0x02,0x11,0x11,0x11,0x11,0x10,
    0x11,0x11,0x20,0xbb,0xb0,0x21,0x11,0x11,0x11,0x11,0x10,
    0x11,0x11,0x12,0x0b,0x02,0x11,0x11,0x11,0x11,0x11,0x10,
    0x11,0x11,0x11,0x37,0x31,0x11,0x11,0x11,0x11,0x11,0x10,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x10,

};

const lv_image_dsc_t ok_b = {
  .header.magic = LV_IMAGE_HEADER_MAGIC,
  .header.cf = LV_COLOR_FORMAT_I4,
  .header.flags = 0,
  .header.w = 21,
  .header.h = 24,
  .header.stride = 11,
  .data_size = sizeof(ok_b_map),
  .data = ok_b_map,
};

