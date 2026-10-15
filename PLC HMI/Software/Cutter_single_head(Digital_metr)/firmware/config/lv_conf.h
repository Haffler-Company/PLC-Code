/**
 * @file lv_conf.h
 * Configuration file for v9.4.0

/* clang-format off */
#if 1 /*Set it to "1" to enable content*/

#ifndef LV_CONF_H
#define LV_CONF_H
/*If you need to include anything here, do it inside the `__ASSEMBLY__` guard */
#if 0 && defined(__ASSEMBLY__)
#include "my_include.h"
#endif
#define LV_COLOR_DEPTH 16
/* LVGL 9 swaps RGB565 explicitly in the flush callback. */
#define LV_COLOR_16_SWAP 0
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB
#define LV_STDINT_INCLUDE <stdint.h>
#define LV_STDDEF_INCLUDE <stddef.h>
#define LV_STDBOOL_INCLUDE <stdbool.h>
#define LV_INTTYPES_INCLUDE <inttypes.h>
#define LV_LIMITS_INCLUDE <limits.h>
#define LV_STDARG_INCLUDE <stdarg.h>
#if LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN
#define LV_MEM_SIZE (384 * 1024U) /*[bytes] — استخر LVGL در PSRAM ۲ مگابایتی (رمِ داخلی آزاد و پایدار)*/
#define LV_MEM_POOL_EXPAND_SIZE 0
#define LV_MEM_ADR 0 /*0: unused*/
#if LV_MEM_ADR == 0
#define LV_MEM_POOL_INCLUDE <esp_heap_caps.h>
#define LV_MEM_POOL_ALLOC(size) heap_caps_malloc(size, MALLOC_CAP_SPIRAM)
#endif
#endif                        /*LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN*/
#define LV_DEF_REFR_PERIOD 25 /*[ms] target 40 FPS before flush/scene cost*/
#define LV_INV_BUF_SIZE 96    /*Keep many small dirty regions instead of falling back to full-screen refresh*/
#define LV_DPI_DEF 130        /*[px/inch]*/
#define LV_USE_OS LV_OS_NONE
#define LV_DRAW_BUF_STRIDE_ALIGN 1  //changed from 1
#define LV_DRAW_BUF_ALIGN 4         //changed from 4
#define LV_DRAW_TRANSFORM_USE_MATRIX 0
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE (24 * 1024)         /*[bytes]*/
#define LV_DRAW_THREAD_STACK_SIZE (8 * 1024) /*[bytes]*/  // changed from 8
#define LV_USE_DRAW_SW 1
#if LV_USE_DRAW_SW == 1
#define LV_DRAW_SW_SUPPORT_RGB565 1
#define LV_DRAW_SW_SUPPORT_RGB565A8 1
#define LV_DRAW_SW_SUPPORT_RGB888 0
#define LV_DRAW_SW_SUPPORT_XRGB8888 0
#define LV_DRAW_SW_SUPPORT_ARGB8888 0
#define LV_DRAW_SW_SUPPORT_L8 0
#define LV_DRAW_SW_SUPPORT_AL88 0
#define LV_DRAW_SW_SUPPORT_A8 0
#define LV_DRAW_SW_SUPPORT_I1 0
#define LV_DRAW_SW_DRAW_UNIT_CNT 1
#define LV_USE_DRAW_ARM2D_SYNC 0
#define LV_USE_NATIVE_HELIUM_ASM 0
#define LV_DRAW_SW_COMPLEX 1
#if LV_DRAW_SW_COMPLEX == 1
#define LV_DRAW_SW_SHADOW_CACHE_SIZE 0
#define LV_DRAW_SW_CIRCLE_CACHE_SIZE 4
#endif
#define LV_USE_DRAW_SW_ASM LV_DRAW_SW_ASM_NONE
#if LV_USE_DRAW_SW_ASM == LV_DRAW_SW_ASM_CUSTOM
#define LV_DRAW_SW_ASM_CUSTOM_INCLUDE ""
#endif
#define LV_USE_DRAW_SW_COMPLEX_GRADIENTS 0
#endif
#define LV_USE_DRAW_VGLITE 0
#if LV_USE_DRAW_VGLITE
#define LV_USE_VGLITE_BLIT_SPLIT 0
#if LV_USE_OS
#define LV_USE_VGLITE_DRAW_THREAD 1
#if LV_USE_VGLITE_DRAW_THREAD
#define LV_USE_VGLITE_DRAW_ASYNC 1
#endif
#endif
#define LV_USE_VGLITE_ASSERT 0
#endif
#define LV_USE_PXP 0
#if LV_USE_PXP
#define LV_USE_DRAW_PXP 1
#define LV_USE_ROTATE_PXP 0
#if LV_USE_DRAW_PXP && LV_USE_OS
#define LV_USE_PXP_DRAW_THREAD 1
#endif
#define LV_USE_PXP_ASSERT 0
#endif
#define LV_USE_DRAW_DAVE2D 0
#define LV_USE_DRAW_SDL 0
#define LV_USE_DRAW_VG_LITE 0
#define LV_USE_LOG 0
#define LV_LOG_PRINTF 0
#define LV_LOG_USE_TIMESTAMP 1
#define LV_LOG_USE_FILE_LINE 1
#define LV_LOG_TRACE_MEM 0
#define LV_LOG_TRACE_TIMER 0
#define LV_LOG_TRACE_INDEV 0
#define LV_LOG_TRACE_DISP_REFR 0
#define LV_LOG_TRACE_EVENT 0
#define LV_LOG_TRACE_OBJ_CREATE 0
#define LV_LOG_TRACE_LAYOUT 0
#define LV_LOG_TRACE_ANIM 0
#define LV_LOG_TRACE_CACHE 0
#endif                                /*LV_USE_LOG*/
#define LV_USE_ASSERT_NULL 0          /*Check if the parameter is NULL. (Disabled to prevent unrecoverable CPU freeze)*/
#define LV_USE_ASSERT_MALLOC 1        /*Checks is the memory is successfully allocated or no. (Very fast, recommended)*/
#define LV_USE_ASSERT_STYLE 0         /*Check if the styles are properly initialized. (Very fast, recommended)*/
#define LV_USE_ASSERT_MEM_INTEGRITY 0 /*Check the integrity of `lv_mem` after critical operations. (Slow)*/
#define LV_USE_ASSERT_OBJ 0           /*Check the object's type and existence (e.g. not deleted). (Slow)*/
#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#define LV_ASSERT_HANDLER LV_LOG_ERROR("LVGL Assert caught!");
#define LV_USE_REFR_DEBUG 0
#define LV_USE_LAYER_DEBUG 0
#define LV_USE_PARALLEL_DRAW_DEBUG 0
#define LV_ENABLE_GLOBAL_CUSTOM 0
#define LV_CACHE_DEF_SIZE 0
#define LV_IMAGE_HEADER_CACHE_DEF_CNT 0
#define LV_GRADIENT_MAX_STOPS 2
#define LV_COLOR_MIX_ROUND_OFS 0
#define LV_OBJ_STYLE_CACHE 0
#define LV_USE_OBJ_ID 0
#define LV_OBJ_ID_AUTO_ASSIGN LV_USE_OBJ_ID
#define LV_USE_OBJ_ID_BUILTIN 1
#define LV_USE_OBJ_PROPERTY 0
#define LV_USE_OBJ_PROPERTY_NAME 1
#define LV_USE_VG_LITE_THORVG 0
#define LV_BIG_ENDIAN_SYSTEM 0
#define LV_ATTRIBUTE_TICK_INC
#define LV_ATTRIBUTE_TIMER_HANDLER
#define LV_ATTRIBUTE_FLUSH_READY
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1
#define LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY
#define LV_ATTRIBUTE_FAST_MEM
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning /*The default value just prevents GCC warning*/
#define LV_ATTRIBUTE_EXTERN_DATA
#define LV_USE_FLOAT 0
#define LV_USE_MATRIX 0
#define LV_USE_PRIVATE_API 0
#define LV_FONT_MONTSERRAT_8 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 1
#define LV_FONT_MONTSERRAT_38 1

#define LV_FONT_MONTSERRAT_28_COMPRESSED 0 /*bpp = 3*/
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0 /*Hebrew, Arabic, Persian letters and all their forms*/
#define LV_FONT_SIMSUN_14_CJK 0            /*1000 most common CJK radicals*/
#define LV_FONT_SIMSUN_16_CJK 0            /*1000 most common CJK radicals*/
#define LV_FONT_UNSCII_8 0
#define LV_FONT_UNSCII_16 0
#define LV_FONT_CUSTOM_DECLARE
#define LV_FONT_DEFAULT &lv_font_montserrat_14
#define LV_FONT_FMT_TXT_LARGE 1
#define LV_USE_FONT_COMPRESSED 0
#define LV_USE_FONT_PLACEHOLDER 1
#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS " ,.;:-_)]}"
#define LV_TXT_LINE_BREAK_LONG_LEN 0
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3
#define LV_USE_BIDI 1
#if LV_USE_BIDI
#define LV_BIDI_BASE_DIR_DEF LV_BASE_DIR_RTL
#endif
#define LV_USE_ARABIC_PERSIAN_CHARS 1
#define LV_WIDGETS_HAS_DEFAULT_VALUE 1
#define LV_USE_ANIMIMG 1
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_BUTTON 1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_CALENDAR 0
#define LV_USE_CANVAS 1
#define LV_USE_CHART 0
#define LV_USE_CHECKBOX 0
#define LV_USE_DROPDOWN 1 /*Requires: lv_label*/
#define LV_USE_IMAGE 1    /*Requires: lv_label*/
#define LV_USE_IMAGEBUTTON 1
#define LV_USE_KEYBOARD 1
#define LV_USE_LABEL 1
#if LV_USE_LABEL
#define LV_LABEL_TEXT_SELECTION 1  /*Enable selecting text of the label*/
#define LV_LABEL_LONG_TXT_HINT 1   /*Store some extra info in labels to speed up drawing of very long texts*/
#define LV_LABEL_WAIT_CHAR_COUNT 3 /*The count of wait chart*/
#endif
#define LV_USE_LED 1
#define LV_USE_LINE 1
#define LV_USE_LIST 0
#define LV_USE_LOTTIE 0 /*Requires: lv_canvas, thorvg */
#define LV_USE_MENU 0
#define LV_USE_MSGBOX 1
#define LV_USE_ROLLER 1 /*Requires: lv_label*/
#define LV_USE_SCALE 0
#define LV_USE_SLIDER 1 /*Requires: lv_bar*/
#define LV_USE_SPAN 0
#define LV_USE_SPINBOX 0
#define LV_USE_SPINNER 1
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 1 /*Requires: lv_label*/
#if LV_USE_TEXTAREA != 0
#define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500 /*ms*/
#endif
#define LV_USE_TABLE 0
#define LV_USE_TABVIEW 1
#define LV_USE_TILEVIEW 0
#define LV_USE_WIN 0
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
#define LV_THEME_DEFAULT_DARK 0
#define LV_THEME_DEFAULT_GROW 1
#define LV_THEME_DEFAULT_TRANSITION_TIME 1  // changed from 80
#endif                                      /*LV_USE_THEME_DEFAULT*/
#define LV_USE_THEME_SIMPLE 0
#define LV_USE_THEME_MONO 0
#define LV_USE_FLEX 1
#define LV_USE_GRID 1
#define LV_FS_DEFAULT_DRIVER_LETTER '\0'
#define LV_USE_FS_STDIO 0
#define LV_USE_FS_POSIX 0
#define LV_USE_FS_WIN32 0
#define LV_USE_FS_FATFS 0
#define LV_USE_FS_MEMFS 0
#define LV_USE_FS_ARDUINO_ESP_LITTLEFS 0
#define LV_USE_FS_ARDUINO_SD 0
#define LV_USE_LODEPNG 0
#define LV_USE_LIBPNG 0
#define LV_USE_BMP 0
#define LV_USE_TJPGD 0
#define LV_USE_LIBJPEG_TURBO 0
#define LV_BIN_DECODER_RAM_LOAD 0
#define LV_USE_RLE 0
#define LV_USE_QRCODE 1
#define LV_USE_BARCODE 0
#define LV_USE_RLOTTIE 0
#define LV_USE_VECTOR_GRAPHIC 0
#define LV_USE_THORVG_INTERNAL 0
#define LV_USE_THORVG_EXTERNAL 0
#define LV_USE_LZ4_INTERNAL 0
#define LV_USE_LZ4_EXTERNAL 0
#define LV_USE_FFMPEG 0
#define LV_USE_SNAPSHOT 0
#define LV_USE_SYSMON 0
#if LV_USE_SYSMON
#define LV_SYSMON_GET_IDLE lv_timer_get_idle
#define LV_USE_PERF_MONITOR 0
#if LV_USE_PERF_MONITOR
#define LV_USE_PERF_MONITOR_POS LV_ALIGN_BOTTOM_RIGHT
#define LV_USE_PERF_MONITOR_LOG_MODE 0
#endif
#define LV_USE_MEM_MONITOR 0
#endif /*LV_USE_SYSMON*/
#define LV_USE_PROFILER 0
#define LV_USE_MONKEY 0
#define LV_USE_GRIDNAV 0
#define LV_USE_FRAGMENT 0
#define LV_USE_IMGFONT 0
#define LV_USE_OBSERVER 0
#define LV_USE_IME_PINYIN 0
#define LV_USE_FILE_EXPLORER 0
#define LV_USE_SDL 0
#define LV_USE_X11 0
#define LV_USE_NUTTX 0
#define LV_USE_LINUX_DRM 0
#define LV_USE_TFT_ESPI 0  // display via custom my_disp_flush() in GUI_Func.h
#define LV_USE_EVDEV 0
#define LV_USE_LIBINPUT 0
#define LV_USE_GENERIC_MIPI (LV_USE_ST7735 | LV_USE_ST7789 | LV_USE_ST7796 | LV_USE_ILI9341)
#define LV_USE_RENESAS_GLCDC 0
#define LV_USE_WINDOWS 0
#define LV_USE_QNX 0
#define LV_BUILD_EXAMPLES 0
#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0
#define LV_USE_DEMO_BENCHMARK 0
#define LV_USE_DEMO_RENDER 0
#define LV_USE_DEMO_STRESS 0
#define LV_USE_DEMO_MUSIC 0
#define LV_USE_DEMO_FLEX_LAYOUT 0
#define LV_USE_DEMO_MULTILANG 0
#define LV_USE_DEMO_TRANSFORM 0
#define LV_USE_DEMO_SCROLL 0
#define LV_USE_DEMO_VECTOR_GRAPHIC 0
#endif
