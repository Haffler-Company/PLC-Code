#pragma once

#define VERSION "V1.9.47"

typedef enum {
    SUITE_UPVC = 0,
    SUITE_OTHER = 1
} FirmwareSuite;

#ifdef __cplusplus
extern "C" {
#endif
extern FirmwareSuite g_firmware_suite;
extern int g_splash_variant;

typedef enum {
    SOUND_TAP = 0,
    SOUND_ERROR = 1,
    SOUND_SUCCESS = 2,
    SOUND_SCROLL = 3,
    SOUND_INSTALL = 4
} SoundType;

void app_sound_play(SoundType snd);
void app_sound_stop(void);
bool app_sound_is_enabled(void);
void app_sound_set_enabled(bool en);

#ifdef __cplusplus
}
#endif

//? ============= SMART CRASH RESTORE =============
#define APP_RESTORE_MAGIC 0x48414652 // 'HAFR'

typedef struct {
    uint32_t magic;
    uint8_t  is_hwtest;       // 0 = Config launcher screen, 1 = hwtest screen
    uint8_t  screen_id;       // Screen ID (ScreensEnum or T_ScreensEnum)
    uint8_t  suite;           // FirmwareSuite (SUITE_UPVC=0, SUITE_OTHER=1)
    int16_t  scroll_x;        // موقعیت افقی اسکرول کارت‌ها (ذخیره با تاخیر ۱ ثانیه)
    uint8_t  checksum;        // Checksum verification
} AppRestoreContext;

static inline uint8_t app_restore_calc_checksum(const AppRestoreContext *ctx)
{
    uint8_t c = 0x5A;
    c ^= (uint8_t)(ctx->magic & 0xFF);
    c ^= (uint8_t)((ctx->magic >> 8) & 0xFF);
    c ^= (uint8_t)((ctx->magic >> 16) & 0xFF);
    c ^= (uint8_t)((ctx->magic >> 24) & 0xFF);
    c ^= ctx->is_hwtest;
    c ^= ctx->screen_id;
    c ^= ctx->suite;
    c ^= (uint8_t)(ctx->scroll_x & 0xFF);
    c ^= (uint8_t)((ctx->scroll_x >> 8) & 0xFF);
    return c;
}

static inline bool app_restore_is_valid(const AppRestoreContext *ctx)
{
    if (ctx->magic != APP_RESTORE_MAGIC) return false;
    if (ctx->checksum != app_restore_calc_checksum(ctx)) return false;
    if (ctx->is_hwtest == 0) {
        if (ctx->screen_id < 2 || ctx->screen_id > 4) return false;
    } else {
        if (ctx->screen_id < 1 || ctx->screen_id > 15) return false;
    }
    return true;
}

//? ============= SCREEN =============

#define TFT_BL -1  // بک‌لایت همیشه روشن است (سخت‌افزاری — VCC)

#define TFT_CS 13
#define TFT_DC 14
#define TFT_RST -1

#define TFT_MISO 11
#define TFT_MOSI 10
#define TFT_SCLK 12

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320

#define SCREEN_ROTATION 3

//? ============= TOUCH =============

#define TOUCH_ORIENTATION 270

#define TOUCH_SDA 8
#define TOUCH_SCL 9
#define TOUCH_INT -1
#define TOUCH_RST -1

//? ============= OUTPUT =============

#define BUZZER 45

//? ============= EEPROM =============

#define MAGIC_NUMBER 0xBBFF0FF0

//? ============= RTOS =============

#define LVGL_TICK_PERIOD 1
