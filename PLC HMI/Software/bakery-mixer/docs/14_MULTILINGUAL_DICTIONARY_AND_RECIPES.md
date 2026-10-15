# 14. Multilingual Dictionary & Industrial Recipe Subsystem

## Executive Summary
Haffler industrial machinery operates in diverse international manufacturing environments across Iran, Turkey, and Central Asia. To guarantee seamless operation across multi-national workforces, the controller implements a zero-overhead **Trilingual Localization Engine (Persian, English, Turkish)** and an **Industrial Recipe Preset Subsystem** supporting up to 50 profiles with USB and WebApp backup.

---

## 1. Trilingual String Registry (Zero-Allocation)

To eliminate dynamic string allocations (`String` or `malloc`) in the UI loop, all localized terms are declared as a static trilingual string matrix indexed by an enum:

```cpp
typedef enum {
    LANG_FA = 0, // Persian (RTL)
    LANG_EN = 1, // English (LTR)
    LANG_TR = 2  // Turkish (LTR)
} app_lang_t;

typedef enum {
    STR_READY = 0,
    STR_RUNNING,
    STR_STOPPED,
    STR_TARGET_LEN,
    STR_ACTUAL_LEN,
    STR_PIECE_COUNT,
    STR_SETTINGS,
    STR_DIAGNOSTICS,
    STR_DAY_LOCK_WARN,
    STR_CLAMP_DOWN,
    STR_SAW_UP,
    STR_MAX_KEYS
} str_id_t;

static const char* const s_lang_matrix[STR_MAX_KEYS][3] = {
    /* STR_READY         */ {"آماده به کار",   "READY",             "HAZIR"},
    /* STR_RUNNING       */ {"در حال برش",     "RUNNING",           "CALISIYOR"},
    /* STR_STOPPED       */ {"متوقف شد",       "STOPPED",           "DURDURULDU"},
    /* STR_TARGET_LEN    */ {"طول مورد نظر",   "Target Length",     "Hedef Boyut"},
    /* STR_ACTUAL_LEN    */ {"طول فعلی",       "Actual Length",     "Mevcut Boyut"},
    /* STR_PIECE_COUNT   */ {"تعداد قطعات",    "Piece Counter",     "Parca Sayisi"},
    /* STR_SETTINGS      */ {"تنظیمات دستگاه", "Machine Settings",  "Makine Ayarlari"},
    /* STR_DIAGNOSTICS   */ {"دیاگ سخت‌افزار", "Hardware Diag",     "Donanim Teshis"},
    /* STR_DAY_LOCK_WARN */ {"مهلت قسط",       "Lease Expiring",    "Taksit Uyari"},
    /* STR_CLAMP_DOWN    */ {"جک گیره",        "Profile Clamp",     "Profil Sikma"},
    /* STR_SAW_UP        */ {"حرکت تیغ اره",   "Saw Blade Feed",    "Testere Ilerleme"}
};

// Global language retrieval
const char* app_str(str_id_t id) {
    app_lang_t cur_lang = settings_get_language();
    if (id >= STR_MAX_KEYS) return "";
    return s_lang_matrix[id][cur_lang];
}
```

### Zero-Leak Language Switching
When the operator toggles the language selector in F5 settings, the event callback iterates through a linked list of registered label pointers (`ui_register_label(lbl, STR_TARGET_LEN)`) and immediately updates the text via `lv_label_set_text(lbl, app_str(id))`. This accomplishes instantaneous, flicker-free language updates without rebooting the controller.

---

## 2. Zero-Tofu Font Engine

A common defect in embedded Persian and Turkish interfaces is missing character boxes (`[]` Tofu) caused by incomplete font glyph sets.

### Glyph Range Requirements
- **Persian Glyphs:** Basic Arabic (`0x0600..0x06FF`), Arabic Supplement (`0x0750..0x077F`), Arabic Presentation Forms-A and B (`0xFB50..0xFDFF`, `0xFE70..0xFEFF`). Specifically contains *Pe* (پ), *Che* (چ), *Zhe* (ژ), and *Gaf* (گ).
- **Turkish Glyphs:** Latin Extended-A (`0x0100..0x017F`). Contains *Ç/ç*, *Ğ/ğ*, *İ/ı*, *Ö/ö*, *Ş/ş*, and *Ü/ü*.
- **Numeric Isolation:** Western Arabic digits (`0-9`) must be isolated inside LTR containers to prevent BiDi text engines from inverting millimeter measurements (e.g. turning `1450.5` into `.5 1450`).

---

## 3. Industrial Recipe & Profile Preset Subsystem

Machines must store cutting, welding, or gas-filling recipes for different aluminum, UPVC, or profile geometries.

### Recipe Data Structure
```cpp
#define RECIPE_NAME_MAX_LEN 32
#define MAX_RECIPES         50

typedef struct {
    uint8_t  id;
    char     name[RECIPE_NAME_MAX_LEN];
    float    target_dimension;   // mm, deg, or bar
    uint16_t primary_timer_ms;   // Clamp dwell, melt time, etc.
    uint16_t secondary_timer_ms; // Cool time, purge time, etc.
    uint16_t speed_percentage;   // 10% to 100% feed rate
    uint16_t target_quantity;   // Batch piece count
    uint32_t checksum_crc32;
} machine_recipe_t;
```

### Persistent Storage in LittleFS
Recipes are stored in the internal SPI flash filesystem (`/spiffs/recipes.json`). If flash memory corruption is detected via CRC mismatch, the system auto-recovers by reloading factory default profile presets:
```json
[
  {
    "id": 1,
    "name": "UPVC Sash 60mm",
    "target_dimension": 1200.0,
    "primary_timer_ms": 2500,
    "secondary_timer_ms": 3000,
    "speed_percentage": 85,
    "target_quantity": 100
  },
  {
    "id": 2,
    "name": "UPVC Frame 70mm",
    "target_dimension": 1450.5,
    "primary_timer_ms": 3000,
    "secondary_timer_ms": 3500,
    "speed_percentage": 75,
    "target_quantity": 50
  }
]
```

### Backup & Fleet Replication
1. **WebApp Export/Import:** Technicians connect via smartphone Wi-Fi to download `recipes.json` directly through the browser.
2. **One-Touch Import:** Uploading the JSON file to another machine parses, validates CRC32, and writes the new recipe set to flash in under $500\text{ ms}$, synchronizing entire factory production lines in seconds.
