
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

#ifndef LV_ATTRIBUTE_CHANGE_B
#define LV_ATTRIBUTE_CHANGE_B
#endif

static const
LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_CHANGE_B
uint8_t change_b_map[] = {

    0x4c,0x70,0x47,0x00,0xff,0xff,0xff,0xcd,0xff,0xff,0xff,0xec,0xff,0xff,0xff,0x7f,
    0xff,0xff,0xff,0x99,0xff,0xff,0xff,0x89,0xff,0xff,0xff,0x36,0xff,0xff,0xff,0x66,
    0xff,0xff,0xff,0xaf,0xff,0xff,0xff,0x9a,0xff,0xff,0xff,0xa3,0xff,0xff,0xff,0x4a,
    0xff,0xff,0xff,0x0e,0xff,0xff,0xff,0x07,0xff,0xff,0xff,0x4e,0xff,0xff,0xff,0xff,

    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x63,0x37,0x60,0x00,0x00,0x00,0x00,
    0x00,0x31,0x11,0x11,0x80,0x3f,0xff,0xf2,0x5c,0x00,0x53,0x00,
    0x00,0x5f,0xff,0xff,0xf0,0x3f,0xff,0xff,0xf2,0xb4,0xf1,0x00,
    0x00,0x04,0xff,0xff,0xf0,0xe9,0x98,0x2f,0xff,0xff,0xf1,0x00,
    0x00,0x0b,0xff,0xff,0xf0,0x00,0x00,0xd7,0x2f,0xff,0xf1,0x00,
    0x00,0xc2,0xff,0xff,0xf0,0x00,0x00,0x00,0xaf,0xff,0xf1,0x00,
    0x00,0x5f,0xf2,0xaf,0xf0,0x00,0x00,0x04,0xff,0xff,0xf1,0x00,
    0x00,0x2f,0xf7,0x04,0x20,0x00,0x00,0x02,0xff,0xff,0xf8,0x00,
    0x06,0xff,0x2d,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x07,0xff,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x0e,0x33,0x60,
    0x03,0xff,0x90,0x00,0x00,0x00,0x00,0x00,0x00,0x09,0xff,0x30,
    0x03,0xff,0x90,0x00,0x00,0x00,0x00,0x00,0x00,0x09,0xff,0x30,
    0x06,0x33,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0xff,0x70,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xd2,0xff,0x60,
    0x00,0x8f,0xff,0xff,0x20,0x00,0x00,0x02,0x40,0x7f,0xf2,0x00,
    0x00,0x1f,0xff,0xff,0x40,0x00,0x00,0x0f,0xfa,0x2f,0xf5,0x00,
    0x00,0x1f,0xff,0xfa,0x00,0x00,0x00,0x0f,0xff,0xff,0x2c,0x00,
    0x00,0x1f,0xff,0xf2,0x7d,0x00,0x00,0x0f,0xff,0xff,0xb0,0x00,
    0x00,0x1f,0xff,0xff,0xf2,0x89,0x9e,0x0f,0xff,0xff,0x40,0x00,
    0x00,0x1f,0x4b,0x2f,0xff,0xff,0xf3,0x0f,0xff,0xff,0xf5,0x00,
    0x00,0x35,0x00,0xc5,0x2f,0xff,0xf3,0x08,0x11,0x11,0x13,0x00,
    0x00,0x00,0x00,0x00,0x06,0x73,0x36,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

};

const lv_image_dsc_t change_b = {
  .header.magic = LV_IMAGE_HEADER_MAGIC,
  .header.cf = LV_COLOR_FORMAT_I4,
  .header.flags = 0,
  .header.w = 24,
  .header.h = 24,
  .header.stride = 12,
  .data_size = sizeof(change_b_map),
  .data = change_b_map,
};

