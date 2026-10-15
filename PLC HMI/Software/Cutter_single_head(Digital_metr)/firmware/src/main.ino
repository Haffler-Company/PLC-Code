//25600 ppr -> minimum delay 8us  (leadshine)
//12800 ppr -> minimum delay 16us (leadshine)
//6400  ppr -> minimum delay 32us (leadshine) selected

#include <SPI.h>
#include <Arduino.h>
#include <esp_heap_caps.h>
#define LV_USE_SIMPLE_CONF 1
#include <lvgl.h>
#include "ui.h"
#include <string.h>
#include <Network.h>
#include <WiFi.h>
#include <WebServer.h>
//#include <DNSServer.h>

#define ENABLE_WIFI_UPLOAD 0
#define SERIAL_DEBUG_BAUD 115200  // هم‌تراز monitor/SHOT — نه 921600 (لاگ بوت ناخوانا می‌شد)

//DNSServer dnsServer;
//#define DNS_PORT 53

#define MAX_NUMBERS 200

int numbers[MAX_NUMBERS];
int numbersCount = 0;

WebServer server(80);

// SoftAP SSID max 32 chars. Protected like the welding screen server.
char ap_ssid[33] = "Haffler-Cutter";
const char* ap_pass = "haffler.com";

String uploadedData = "";

// ---------- RTOS ----------
TaskHandle_t webTaskHandle;
TaskHandle_t parserTaskHandle;
TaskHandle_t webPortalTaskHandle = NULL;
SemaphoreHandle_t uploadSemaphore;

TaskHandle_t PrinterTaskHandle = NULL;

// ---------- Function Prototypes ----------
void handleFileUpload();
void parseNumbers(String data);
void TaskWebServer(void *pvParameters);
void TaskParser(void *pvParameters);
bool startWebPortalTask();
void WebPortalTask(void *pvParameters);
void sendPortalPage();
void panel_request_stop_motion(void);


HardwareSerial printer(2); // UART2

volatile bool stopstate = 1;
volatile bool stopstate_save_request = false;
volatile uint32_t bl_pulse_until_ms = 0;
volatile uint32_t buzzer_hold_until_ms = 0;
volatile bool printertest = 0;
bool printer_enabled = false;              // F1 setting; persisted by Header Save
uint8_t printer_model = 0;                 // 0=ESC/POS UART, 1=reserved/unknown
volatile bool ref_failed_flag = false;     // set by Task3 on homing failure, consumed in Task2/GUI
volatile bool ref_wrong_dir_flag = false;  // set by Task3 when upper-limit hit during homing (wrong direction)
volatile int  ref_wrong_dir_sensor = 0;    // 1=سنسور اول خط نابجا  2=سنسور آخر خط نابجا
volatile int  ref_fail_reason = 0;         // 1=timeout موتور/اتصال  2=استاپ کاربر  0=سایر
volatile bool ref_error_ui_active = false; // کارت پیام خطا؛ لمس → برگشت به دکمه سبز
volatile bool blade_guard_timeout_flag = false;
volatile uint8_t cut_start_ui_notice_req = 0;  // Task8 -> Task2; 1=Start برش خارج از Main
// Task8 -> Task2 only: one coalesced, actionable cut-cycle notice.  LVGL remains in Task2.
enum CutUiNotice : uint8_t {
  CUT_UI_NOTICE_NONE = 0,
  CUT_UI_NOTICE_UI_LOCKED,
  CUT_UI_NOTICE_DEVICE_LOCKED,
  CUT_UI_NOTICE_PINMAP,
  CUT_UI_NOTICE_STOP,
  CUT_UI_NOTICE_GUARD_OPEN,
  CUT_UI_NOTICE_BLADE_NOT_HOME,
  CUT_UI_NOTICE_MOTOR_FAULT,
  CUT_UI_NOTICE_HOLDER_ENGAGED,
  CUT_UI_NOTICE_BLADE_LEAVE_TIMEOUT,
  CUT_UI_NOTICE_BLADE_RETURN_TIMEOUT,
  CUT_UI_NOTICE_BLADE_RETURNING,
  CUT_UI_NOTICE_BLADE_RETURNED,
  CUT_UI_NOTICE_FAULT_RECOVERED,
  CUT_UI_NOTICE_ANGLE_90_CONFIRM_NEEDED
};
volatile uint8_t cut_safety_ui_notice_req = CUT_UI_NOTICE_NONE;
volatile bool cut_start_inputs_released = true;
volatile bool cut_start_ui_modal_active = false;  // Task2 writes; Task4 reads without LVGL access
volatile bool cut_angle_90_prompt_active = false; // Task2 modal state
volatile bool cut_angle_90_prompt_req = false;    // Task8/GUI -> Task2 request
volatile bool cut_angle_90_confirmed = false;     // operator confirmed 90° manual setup
volatile bool limit_recal_flag = false;    // Task3: برخورد با سنسور حد و اصلاح خودکار موقعیت
volatile int  limit_recal_which = 0;       // 1: سنسور ابتدای خط (ref)   2: سنسور انتهای خط (up-limit)
volatile int  limit_recal_err_mm = 0;      // اختلاف mm قبل از snap (برای متن هشدار)
volatile bool sensor_alert_motion_lock = false;  // تا تأیید پیام: صفر پالس استپر
volatile bool critical_task_start_failed = false;  // خطای بوت تسک ایمنی؛ فقط با ریست کامل پاک می‌شود
volatile uint8_t sensor_alert_kind = 0;         // 0=none 1=wrong_dir 2=limit_hit
volatile int  limit_escape_sensor = 0;          // 1/2: اجازهٔ دور شدن از همان سنسور
volatile bool sensor_stuck_escape_flag = false; // دور شدیم ولی سنسور >5s آزاد نشد
volatile int  sensor_stuck_which = 0;           // 1 اول / 2 آخر
volatile bool force_left_backlash_settle = false; // فقط موتور چپ: لقی ۵mm؛ راست هرگز
volatile bool jog_seek_allow_backlash = true;     // seek بعد از جاگ: لقی فقط اگر جاگ چپ بود
volatile bool f3_stepper_test_cmd = false;       // F3: short tap Q0/Q1/Q2 -> Task3 pulse test
volatile bool f3_stepper_test_busy = false;      // F3: stepper test running (UI green)
volatile bool f3_stepper_test_abort = false;     // F3: second tap -> reverse net pulses + EN off
volatile bool f3_stepper_test_stop_fault = false;  // STOP/E-stop during test: stop now, await operator recovery
volatile bool f3_stepper_test_recovery_needed = false;
volatile bool f3_stepper_test_recover_cmd = false;
volatile int f3_stepper_test_recovery_net = 0;
volatile uint32_t f3_stepper_test_pulse_count = 0;
volatile uint32_t f3_stepper_test_deadline_ms = 0;
volatile int f3_stepper_test_pulses = 50;        // F3: pulses in each direction during the pin test
volatile int f3_stepper_test_period_ms = 25;     // F3: interval between test pulses
volatile int8_t sensor_trip_which = 0;          // ISR: 1=اول خط 2=آخر خط (latch تا Task3 مصرف کند)
volatile bool jog_numlock_from_joystick = false; // ورود جاگ از جویستیک → خروج خودکار با توقف
volatile bool jog_soft_limit_flag = false; // رسیدن به حد نرم تنظیمات (اول/آخر خط عددی)
volatile int  jog_soft_limit_which = 0;    // 1: حداقل   2: حداکثر
volatile bool web_portal_start_request = false;  // Task2 → WebPortalTask: روشن کردن SoftAP
volatile bool web_portal_stop_request = false;   // Task2 → WebPortalTask: خاموش کردن SoftAP
volatile bool web_portal_running = false;
volatile bool web_portal_transitioning = false;  // شروع/توقف AP در حال انجام است؛ مالکیت WiFi هنوز آزاد نیست
volatile bool web_portal_ota_active = false;     // قفل تعمیرات تا پایان/لغو OTA محلی
volatile int8_t web_portal_ap_result = 0;  // 0=idle 1=AP ok -1=AP fail (Task2 toast)
volatile bool g_flag_time_synced_toast = false;
volatile bool g_flag_update_available_toast = false;
uint32_t total_cuts = 0;      // شمارندهٔ کل برش‌ها عمری — در NVS (غیرقابل ریست کاربر)
uint32_t user_cuts = 0;       // شمارندهٔ قابل‌ریست کاربر — در NVS
uint32_t cuts_life_manual = 0;
uint32_t cuts_life_list = 0;
uint32_t cuts_life_opt = 0;
uint32_t cuts_day_manual = 0;
uint32_t cuts_day_list = 0;
uint32_t cuts_day_opt = 0;
uint32_t cuts_day_key = 0;    // YYYYMMDD
uint32_t blade_cuts = 0;
uint32_t blade_limit = 5000;
bool blade_reminder_enabled = true;
char blade_service_date[24] = "";
float cpu_temp_alarm_c = 75.0f;
bool warranty_active = false; // فعال‌سازی یک‌بارهٔ گارانتی در F5
char warranty_date[24] = "";  // تاریخ ثابت روز فعال‌سازی (نمایش)
String admin_pin = "123456";  // رمز مدیر F4 / قفل روزشمار / آپدیت / کلید سرویس UART
uint8_t web_save_policy = 0;  // سیاست ذخیرهٔ فرمان‌های اپ وب: 0=پرسیده نشده، 1=خودکار، 2=تأیید روی نمایشگر
uint8_t web_pending_kind = 0; // 0=هیچ، 1=تنظیمات، 2=لیست برش
uint32_t web_pending_ms = 0;
bool web_policy_ask_request = false;
uint32_t web_portal_started_ms = 0;
int day_lock_remain = 0;      // روز باقی‌مانده قفل (0=خاموش)
int day_lock_stage_days[5] = {0, 0, 0, 0, 0}; // T1..T5، روزهای هر مرحله
uint8_t day_lock_active_stage = 0;            // 1..5، 0=غیرفعال
bool day_lock_grace10_used = false;           // ۱۱۲۲۳۳ یک‌بارمصرف مصرف شده است
bool day_lock_t1_from_chain = false;          // زنجیره از T1 مودال تایمرها مسلح شده
volatile bool device_locked = false;
int64_t soft_epoch_base = 0;
uint32_t soft_millis_base = 0;
int64_t day_lock_last_midnight = 0;
bool soft_clock_valid = false;
volatile bool flag_start_update_flow = false;  // F5 Update → جریان OTA داخل برنامه
bool clear_buffer_sp_flag = true;  // پاک‌کردن بافر setpoint با اولین رقم (کیبورد + نامپد)
// جریانِ بروزرسانیِ F5-بوت: اگر هنگامِ روشن‌شدنِ دستگاه کلیدِ F5 نگه داشته شود true می‌شود.
// در این حالت تسک‌های حرکت/برش (Task3/Task8) غیرفعال و جریانِ رمز→وایفای→آپدیت اجرا می‌شود.
volatile bool g_update_mode = false;
enum WifiRuntimeOwner : uint8_t {
  WIFI_OWNER_NONE = 0,
  WIFI_OWNER_PORTAL,
  WIFI_OWNER_UPDATER_PENDING,
  WIFI_OWNER_UPDATER,
  WIFI_OWNER_LOCAL_OTA
};
static volatile uint8_t wifi_runtime_owner = WIFI_OWNER_NONE;
static portMUX_TYPE wifi_runtime_owner_mux = portMUX_INITIALIZER_UNLOCKED;

static bool wifi_owner_try_acquire(uint8_t owner) {
  bool ok = false;
  portENTER_CRITICAL(&wifi_runtime_owner_mux);
  if (wifi_runtime_owner == WIFI_OWNER_NONE || wifi_runtime_owner == owner) {
    wifi_runtime_owner = owner;
    ok = true;
  }
  portEXIT_CRITICAL(&wifi_runtime_owner_mux);
  return ok;
}

static bool wifi_owner_transition(uint8_t expected, uint8_t next) {
  bool ok = false;
  portENTER_CRITICAL(&wifi_runtime_owner_mux);
  if (wifi_runtime_owner == expected) {
    wifi_runtime_owner = next;
    ok = true;
  }
  portEXIT_CRITICAL(&wifi_runtime_owner_mux);
  return ok;
}

static void wifi_owner_release(uint8_t owner) {
  portENTER_CRITICAL(&wifi_runtime_owner_mux);
  if (wifi_runtime_owner == owner) wifi_runtime_owner = WIFI_OWNER_NONE;
  portEXIT_CRITICAL(&wifi_runtime_owner_mux);
}

static uint8_t wifi_owner_get() {
  uint8_t owner;
  portENTER_CRITICAL(&wifi_runtime_owner_mux);
  owner = wifi_runtime_owner;
  portEXIT_CRITICAL(&wifi_runtime_owner_mux);
  return owner;
}

// Reserve the single WiFi driver for the updater without a check/set race.
// If the portal is already running, it keeps the radio only long enough to
// drain and stop; portal_stop_ap() then completes PENDING -> UPDATER.
static bool wifi_owner_begin_updater() {
  bool ok = false;
  portENTER_CRITICAL(&wifi_runtime_owner_mux);
  if (wifi_runtime_owner == WIFI_OWNER_NONE) {
    wifi_runtime_owner = WIFI_OWNER_UPDATER;
    ok = true;
  } else if (wifi_runtime_owner == WIFI_OWNER_PORTAL && !web_portal_transitioning) {
    wifi_runtime_owner = WIFI_OWNER_UPDATER_PENDING;
    ok = true;
  }
  portEXIT_CRITICAL(&wifi_runtime_owner_mux);
  return ok;
}

static void wifi_owner_cancel_updater() {
  const bool portal_alive = web_portal_running || web_portal_transitioning;
  portENTER_CRITICAL(&wifi_runtime_owner_mux);
  if (wifi_runtime_owner == WIFI_OWNER_UPDATER) {
    wifi_runtime_owner = WIFI_OWNER_NONE;
  } else if (wifi_runtime_owner == WIFI_OWNER_UPDATER_PENDING) {
    wifi_runtime_owner = portal_alive ? WIFI_OWNER_PORTAL : WIFI_OWNER_NONE;
  }
  portEXIT_CRITICAL(&wifi_runtime_owner_mux);
}
volatile bool pending_reference_confirm = false;
volatile uint32_t reference_jog_inhibit_until_ms = 0;
volatile bool keyboard_pending_enter = false;
volatile bool main1_safe_mode = false;
volatile bool cut_done_notify_req = false;  // CL-MAIN-04
volatile float cut_done_notify_mm = 0;

//---------- لیست برش 100تایی + بهینه‌سازی ----------
#define LIST_MAX 100                  // ظرفیت لیست برش
#define LIST_ROWS_PER_PAGE 5
#define LIST_PAGE_COUNT ((LIST_MAX + LIST_ROWS_PER_PAGE - 1) / LIST_ROWS_PER_PAGE)
int stock_len_mm = 6000;      // طول شاخهٔ خام mm — مجاز: 10..100000 (۱۰mm تا ۱۰۰m)
float kerf_mm = 3.0f;         // عرض تیغه / پرت mm با دقت ۰.۱
bool blade_err_enable = false; // اصلاح خطای زاویه تیغه — پیش‌فرض خاموش
bool blade_err_armed = false;  // لمس دایرهٔ صفحه اصلی: ±mm فقط روی هدف واقعی موتور
float blade_err_mm = 2.0f;     // پیش‌فرض ۲mm؛ مجاز −۹٫۹…۹٫۹ (|v|≥۰٫۱)
int trim_mm = 0;              // پیش‌برش سر شاخه (rough-end trim)
int jog_speed_interval = 50;  // سرعت جوگ مستقل (از وب) — کروز
int jog_init_speed = 320;     // راه‌اندازی نرم جوگ/حرکت دستی (مثل global_init_speed: 40..400)
int auto_ramp_ms = 1000;      // auto/manual move soft ramp time, 0..5000 ms (web + future HMI)
int jog_ramp_ms = 1000;       // jog soft ramp time, 0..5000 ms (web + future HMI)
#define OPT_MAX_PIECES 5000
#define OPT_USER_MAX_PIECES 5000  // سقف مجموع تعداد برش در یک محاسبه
#define OPT_MAX_BARS 1000
// ۱۰ ردیف مجازی در هر صفحه؛ همه شاخه‌ها با اسکرول و صفحه‌بندی در دسترس می‌مانند.
#define OPT_RENDER_ROWS 10
#define OPT_CUT_LOG_MAX 500
EXT_RAM_BSS_ATTR struct st_OPT_PARAM {
  volatile bool active = false;      // پنل بهینه‌سازی باز و در حال اجراست
  int piece_count = 0;
  int bar_count = 0;
  volatile int cur = 0;              // ایندکس قطعهٔ جاری در صف اجرا
  float piece_len[OPT_MAX_PIECES];   // طول قطعات به ترتیب اجرا (شاخه‌به‌شاخه)
  uint16_t piece_bar[OPT_MAX_PIECES]; // شمارهٔ شاخهٔ هر قطعه
  uint8_t piece_entry[OPT_MAX_PIECES];  // ردیف لیست مبدأ (برای کم‌کردن تعداد)
  bool piece_done[OPT_MAX_PIECES];   // وضعیت برش قطعات
  float bar_waste[OPT_MAX_BARS];     // پرت هر شاخه
  uint8_t bar_angle[OPT_MAX_BARS];   // درجهٔ هر شاخه (90/45) — بعد از پایان موتور مرجع است
  float kerf_mm = 4.2f;              // پرت تیغه (میلی‌متر)
  volatile bool badge_wait_pick = true;  // تا انتخاب صریح شاخه از پنل، عدد شاخه در صفحهٔ اصلی «—» می‌ماند
  float cut_log_mm[OPT_CUT_LOG_MAX]; // ابعاد برش‌خورده یکی‌یکی (بدون تعداد)
  int16_t cut_log_bar[OPT_CUT_LOG_MAX]; // شاخهٔ مبدأ برای گروه‌بندی کادر (شماره در UI نشان داده نمی‌شود)
  int cut_log_n = 0;
  volatile bool waiting_move = false; // true: عدد آماده است؛ Enter بعدی حرکت محور را می‌زند
  volatile bool flag_refresh = false;   // Task8 → Task2: بروزرسانی پنل بعد از برش
} OPT;

struct st_IO_PARAM {
  //---------output pins--------------//
  int pul_pin = 16;       // Q0
  int dir_pin = 42;       // Q1
  int enb_pin = 39;       // Q2
  int u_clip_pin = 48;    // Q3
  int f_clip_pin = 40;    // Q4
  int spray_pin = 15;     // Q5
  int saw_pin = 35;       // Q6
  int lamp_pin = 36;      // Q7
  int sawl_ift_pin = 38;  // Q8
  int q9_pin = 41;        // Q9
  //----------input states----------------//
  volatile bool ref_sensor_state = false;
  volatile bool up_lim_sensor_state = false;
  volatile bool start_button_state = false;
  volatile bool start1_button_state = false;
  volatile bool start2_button_state = false;
  volatile bool both_start_button_state = false;
  volatile bool guard_switch_state = false;
  volatile bool cut_print_request = false;
  volatile bool opt_print_request = false;
  volatile bool opt_print_done_request = false;  // چاپ فقط اعداد برش‌خورده
  volatile bool stop_button_state = false;
  volatile bool joystick_state_R = false;
  volatile bool joystick_state_L = false;
  volatile bool blade_return_state = false;  // I4 — سنسور برگشت تیغه
  volatile bool motor_fault_state = false;   // I5 — خطای موتور
  volatile bool micro_switch_state = false;
  //---------------------------------
  int cut_prcs_timer = 5000;
  bool saw_is_on = false;
  volatile bool clamp_engaged = false;       // آینهٔ q3_latched از Task8 — برای گارد ورود به تنظیمات
  volatile bool cut_confirm_pending = false; // آینهٔ cut_ready_for_confirm از Task8
} IOS;
struct st_Step_PARAM {
  volatile int drv_ppr = 6400;
  volatile int currentPosition_p = 0;
  volatile int initial_Position_p = 0;
  String final_position_mm = "6000";
  volatile int system_base_dir = 1;
  volatile int system_set_dir = +1;
   int global_speed = 50;
   int global_init_speed = 320;
   int global_lowSpeedTime = 1000;
  int position_Setpoint_p = 0;
  float Man_Sp_mm = 0;
  volatile bool jogEnabled = false;
  bool begin_calibration = false;
  int calib_stage = 0;
  bool reachedToSp = false;
  volatile bool moveStepperCMD = false;
  int total_course = drv_ppr * 3;
  float Axis_Pitch = 20;
  float um_per_pulse = 3.125f;  // exact µm per pulse from endstop calibration
  bool go_sampling_CMD = false;
  volatile bool referenced = false;  // for exhibition
  volatile bool motorIs_moving = false;
  bool goref_cmd = false;
  volatile bool mvm_interupt = false;  // user can interupt movement in any function ref , movestepper , calibration sampling
  volatile int final_position_p = 6000;
  bool in_position_flag = true;
  bool flag_is_cutting = false;
} STP;

static inline float axisPulsesPerMm() {
  if (isfinite(STP.um_per_pulse) && STP.um_per_pulse > 0.0001f)
    return 1000.0f / STP.um_per_pulse;
  if (isfinite(STP.Axis_Pitch) && STP.Axis_Pitch > 0.0001f && STP.drv_ppr > 0)
    return (float)STP.drv_ppr / STP.Axis_Pitch;
  return 1.0f;
}

// مقصد UI همیشه عدد واردشدهٔ کاربر است. اصلاح زاویه فقط در هدف واقعی موتور
// اعمال می‌شود؛ بنابراین خاموش بودن کلید دقیقاً معادل آفست صفر است.
static inline float blade_err_active_offset_mm() {
  return (blade_err_enable && blade_err_armed) ? blade_err_mm : 0.0f;
}

static inline float blade_err_motor_target_mm(float ui_destination_mm) {
  return ui_destination_mm + blade_err_active_offset_mm();
}

static inline int blade_err_motor_target_p(float ui_destination_mm) {
  float ppmm = axisPulsesPerMm();
  if (ppmm <= 0.0f) return 0;
  return (int)lroundf(blade_err_motor_target_mm(ui_destination_mm) * ppmm);
}

EXT_RAM_BSS_ATTR struct st_GUI_PARAM {
  int system_language = 1;  // 1:farsi // 2:English // 0 not set
  String inputBuffer_sp = "0";
  String inputBuffer_initial_L1 = "0";
  String inputBuffer_initial_L2 = "0";
  String inputBuffer_L_3_1_x[LIST_MAX];  // پر می‌شود در recall / صفر پیش‌فرض
  String inputBuffer_L_3_2_x[LIST_MAX];
  String inputBuffer_max_L = "0";
  int L1_GUI = 100;
  int L2_GUI = 100;
  bool edit_mode_lable1_1 = false;
  bool edit_mode_lable2_2 = false;
  bool edit_mode_lable2_3 = false;
  bool enable_edit_lable2_3 = false;
  bool cast_message = false;
  volatile bool buzzer_cmd_short = false;
  volatile bool buzzer_cmd_long = false;
  bool onece_beep = true;
  volatile bool buzzer_enable = false;  // Task5 هستهٔ ۰؛ UI هستهٔ ۱
  bool virtual_kb_enable = true;  // effective soft-KB
  uint8_t vkb_mode = 0;  // 0=AUTO 1=FORCE_ON 2=FORCE_OFF
  bool joystick_enable = true;  // F4: خاموش = دستور جویستیک نخوان
  int scr_id = 0;
  bool NumLock = false;
  int visible_raws = -1;
  int tab_view_select = 0;
  bool update_lable1_1 = true;
  bool update_lable1_3 = false;
  bool flag_flash_count_dec = false;  // سایهٔ موقعیت: قرمز→سبز وقتی تعداد کم شد
  bool lock_screen = false;
  bool flag_cast_error1_1 = false;
  bool flag_remove_error1_1 = false;
  bool clear_buffer_aut_flag = true;
  bool flag_update_lable2_5 = true;
  bool enable_edit_lable2_5 = false;
  bool flag_update_panel3_2_x_clr = true;
  bool flag_save_key = false;
  bool flag_cancl_key = false;
  bool flag_mode_key = false;
  bool flag_start_key = false;
  volatile bool flag_stop_key = false;  // Task10 می‌نویسد، Task2 مصرف می‌کند
  bool flag_panel_nav_exit_key = false;
  bool flag_panel3_edit_labels_txt = false;  //clr flag
  bool flag_update_labels_3_1 = false;       // txt flag
  bool flag_update_labels_3_2 = false;       //txt flag
  volatile bool flag_update_lable1_2 = true;
  bool flag_update_lables_2 = false;
  bool flag_uncheck_saw_btns = false;
  bool flag_check_saw_btns = false;
  bool flag_cast_emg = false;
  bool flag_del_emg = false;
  bool flag_cast_error1_3 = false;
  bool flag_remove_error1_3 = true;
  bool flag_reinit_settings = false;
  bool flag_factory_reset_dialog = false;  // CANCEL پنل 5s یا نگه‌داشتن Save هدر F → Task2 دیالوگ تایمردار
  bool flag_web_portal_dialog = false;     // Task10 (نگه‌داشتن SAVE پنل 5 ثانیه) → toggle پورتال
  bool flag_web_portal_toggle = false;     // کلید WiFi در F1 → روشن/خاموش پورتال
  bool flag_web_portal_chip_refresh = false; // همگام‌سازی ظاهر چیپ WiFi با web_portal_running
  bool flag_serial_import_dialog = false;  // SerialImportTask → Task2: دیالوگ تأیید ورود سایزها
  // ── تأیید ذخیره‌سازی وب (SERIAL-CHALLENGE-WEB-CONFIRM-01) ──
  volatile bool flag_web_save_prompt = false;   // ۵ ثانیه بعد از روشن شدن وب → پرامپت انتخاب حالت ذخیره
  unsigned long web_save_prompt_at = 0;         // مهلت ۵ ثانیه‌ای (millis) برای نمایش پرامپت
  volatile uint8_t web_save_mode = 0;           // 0 = هر تغییر با تأیید روی دستگاه (پیش‌فرض)، 1 = ذخیره خودکار (فقط همین نشست وب)
  volatile bool flag_web_change_confirm = false; // WebPortalTask → Task2: msgbox تأیید تغییر معلق از وب
  int list_page = 0;                       // صفحهٔ جاری لیست (0..LIST_PAGE_COUNT-1)
  bool flag_update_page = true;
  bool flag_refresh_visible = false;       // اجبار به بازمحاسبهٔ ردیف‌های قابل‌نمایش
  bool flag_clear_list_counts = false;     // سطل: شروع پاک‌کردن تعدادها
  int clear_visual_idx = -1;                // ردیف در حال پاک‌شدن برای نمایش مرحله‌ای
  int clear_nvs_idx = -1;                  // سطل: نوشتن NVS تکه‌تکه (−1 = بیکار) — جلوگیری از هنگ
  bool flag_save_visible_raws = false;     // ذخیرهٔ تعداد ردیف‌ها در Task2
  bool flag_save_all = false;              // ذخیرهٔ تنظیمات — فقط در Task2
  bool flag_save_stock_kerf = false;       // stock/kerf — فقط در Task2
  bool flag_sync_stock_kerf = false;       // stock/kerf label sync — فقط در Task2
  bool flag_close_numpad = false;          // بستن نامپد — فقط در Task2
  volatile bool flag_abort_opt_calc = false; // استوپ پنل → لغو محاسبه بهینه + آزاد lock
  bool flag_save_vkb = false;              // vkb_en — فقط در Task2
  bool flag_save_buzzer = false;           // buz_stat — فقط در Task2
  bool flag_apply_language = false;        // تعویض FA/EN لیبل‌ها در Task2
  bool flag_exit_queue_prompt = false;     // خروج از لیست/بهینه: دیالوگ تأیید در Task2
  uint8_t exit_queue_prompt_why = 0;       // 1=رقم  2=جاگ
  int pending_lang_selected = 0;           // 0=FA 1=EN برای flag_apply_language
  bool pending_reinit_settings = false;    // بعد از settle ورود به تنظیمات
  uint8_t ui_screen_settle = 0;            // تیک‌های سبک بعد از loadScreen (ضد WDT میانی‌رند)
  bool pending_f1_numpad_open = false;     // باز کردن نامپد یک تیک بعد از reinit
  uint32_t soft_kb_auto_ready_ms = 0;       // AUTO: نمایش بعد از فرصت شناسایی USB در بوت
  uint32_t ui_nav_ready_ms = 0;            // قبل از این، ورود به تنظیمات را نگه دار (ضد WDT بوت)
  bool pending_boot_main_ready = false;    // بعد از اسپلش → main سبک + گیت ناوبری
  int pending_goto_scr = -1;               // تعویض صفحه فقط در Task2
  uint8_t pending_zero_bits[(LIST_MAX + 7) / 8] = { 0 };  // ردیف‌های تازه‌ساخته → NVS صفر تکه‌تکه
  uint8_t pending_sp_save_bits[(LIST_MAX + 7) / 8] = { 0 };     // ذخیرهٔ متراژ لیست در Task2
  uint8_t pending_count_save_bits[(LIST_MAX + 7) / 8] = { 0 };  // ذخیرهٔ تعداد لیست در Task2
} GUI;
void warranty_request_beep(void) {
  GUI.buzzer_cmd_short = true;
}
int warranty_get_system_language(void) {
  return GUI.system_language;
}
static volatile int qa_ui_action = 0;  // QA-only UI action executed from Task2/LVGL thread
EXT_RAM_BSS_ATTR struct st_auto_PARAM {
  bool auto_mode_active = false;
  float list_sp_Val_p[LIST_MAX] = { 0 };
  int list_count_Val[LIST_MAX] = { 0 };
  int list_count_base_Val[LIST_MAX] = { 0 };  // تعداد اولیه برای نمایش خط‌خورده کنار باقی‌مانده
  uint8_t list_angle_Val[LIST_MAX] = { 0 };   // 0 یا 45 = 45 درجه، 90 = 90 درجه
  int temp_list_count_Val = 0;
  int selected_count_val = -1;
  bool list_edit_mode_sp[LIST_MAX] = { false };
  bool list_edit_mode_count[LIST_MAX] = { false };
  bool in_position_flag = false;
} AUT;
//------------------runtime------------------------------
// char statsBuffer[1024];  // for cpu runtime stats
//-----------------Rtos_task hanldes----------------------------------
TaskHandle_t h_Task1;
TaskHandle_t h_Task2;
TaskHandle_t h_Task3;
TaskHandle_t h_Task4;
TaskHandle_t h_Task5;
TaskHandle_t h_Task6;
TaskHandle_t h_Task7;
TaskHandle_t h_Task8;
TaskHandle_t h_Task9;
TaskHandle_t h_Task10;
TaskHandle_t h_Task11;
//--------------semafores---------------------------------
SemaphoreHandle_t lable1_1_Semaphore;
SemaphoreHandle_t adcSemaphore;
SemaphoreHandle_t preferencesSemaphore;
SemaphoreHandle_t touchWireMutex = NULL;
static volatile bool ui_bootstrap_complete = false;
volatile bool keyboardJogActive = false;
static const int JOG_TIMER_MAX_INTERVAL_US = 600;  // سقف فنی: ۶ × کندترین global_speed
// حافظهٔ مقصد F1..F5 روی صفحهٔ اصلی (نگه‌داشتن F + Enter = ذخیره؛ رها کردن F = فراخوانی)
float main_f_mem_mm[5] = { 0 };
bool main_f_mem_valid[5] = { false };
volatile int panel_f_held = 0;            // 0=رها، 1..5 = Fx در حال فشردن
volatile int panel_f_press_edge = 0;      // لبهٔ رها کردن برای فراخوانی Task2 (۱..۵)
volatile bool panel_f_save_consumed = false;  // این نگه داشتن با Enter ذخیره شد → رها = فراخوانی نکن
volatile bool flag_save_main_f_mem = false;
volatile bool flag_panel_stop_motion_toast = false;  // استوپ پنل حین حرکت → توست

volatile int pending_main_f_mem_idx = -1;
volatile bool jog_release_seek_pending = false;
volatile int jog_live_interval_us = 320;  // پس از بارگذاری تنظیمات با jog_init_speed همگام می‌شود
volatile int jog_pred_dir = 0;            // آخرین جهت برای پیش‌بینی مقصد
volatile uint32_t jog_pred_dir_ms = 0;    // زمان عوض شدن جهت (جلوگیری از پرش مقصد)
volatile int jog_wish_dir = 0;            // جهت درخواستی UI؛ DIR فیزیکی بعد از توقف نرم
volatile bool jog_backup_active = false;  // عوض‌جهت: توقف تطبیقی تا مقصد زنده، سپس جهت جدید
volatile float jog_dest_backup_mm = 0;    // کپی مقصد قبل از عوض‌جهت؛ پایهٔ رشد مقصد جدید
volatile bool jog_keep_dest = false;      // بعد از رها: ادامهٔ همان جهت مقصد را عقب نکشد
volatile bool jog_forward_resume_dest_valid = false;
volatile float jog_forward_resume_dest_mm = 0;
volatile bool jog_grow_active = false;    // ادامهٔ همان جهت: مقصد از پایه با شیب رشد کند
volatile float jog_grow_base_mm = 0;      // پایهٔ رشد (عدد فریز لحظهٔ دوباره گرفتن کلید)
volatile uint32_t jog_grow_t0 = 0;        // شروع رشد مقصد (زمان‌محور، نه وابسته به استارت سرد موتور)
volatile int jog_grow_peak_whole = 0;     // مقصد در جلسهٔ رشد فقط جلو برود (بدون جهش عقب)
volatile bool jog_soft_restart = false;   // رها→دوباره: جاگ از خزیدن دوباره شیب بگیرد (نه ادامهٔ decel)
volatile bool jog_seek_soft = false;      // seek بعد از رها جاگ → شیب جاگ (نه برش)
volatile bool jog_seek_speed_handoff = false;  // ادامهٔ هم‌جهت وسط seek: استارت از سرعت جاری
volatile bool jog_fine_held = false;      // کلید ۲: سرعت جاگ ÷۶ (آرام)
volatile bool jog_reverse_stop_active = false;
volatile int32_t jog_reverse_stop_target_p = 0;
volatile int8_t jog_reverse_stop_dir = 0;
volatile bool jog_motion_test_request = false;  // فقط فرمان سریال JQT؛ سناریوی محدود و خودمتوقف
volatile uint8_t jog_remote_test_action = 0;
volatile uint32_t jog_remote_test_deadline_ms = 0;
volatile bool jog_remote_test_active = false;
extern volatile int movement_mode;
// نوشتن String مقصد فقط از Task2 — Task11/4 فقط عدد می‌گذارند (جلوگیری از heap corrupt / ریست)
volatile bool jog_dest_buf_pending = false;
volatile int jog_dest_buf_whole = 0;
volatile int ui_active_settings_tab = -1;  // همگام‌سازی تب فعال تنظیمات بین Task2 و Task4 بدون دسترسی متقاطع LVGL
// بعد از LONG_PRESSED راهنما: رها کردن همان لمس نباید اکشن دکمه را اجرا کند
volatile bool ui_help_suppress_release = false;
void prcs_jog_motion_test();
//------------include headers-----------------------------
#include "Pref_Func.h"
#include "pinmap_runtime.h"
#include "lcd_bench.h"
#include "perf_overlay.h"
#include "screenshot.h"
#include "haffler_fault_log.h"
#include "GUI_Func.h"
#include "oobe_wizard.h"
#include "haffler_warranty.h"
#include "ui_extras.h"
#include "main_position_fast.h"
#include "admin_ui.h"
#include "screensaver.h"
#include "serial_import.h"
#include "update_flow.h"   // جریانِ بروزرسانیِ F5-بوت — باید قبل از Task2.h باشد (update_flow_tick)
#include "Task1.h"
#include "Task2.h"
#include "Task3.h"
#include "Task4.h"
#include "Task5.h"
#include "Task6.h"
#include "Task7.h"
#include "Task8.h"
#include "Task9.h"
#include "Task10.h"
#include "Task11.h"
#include "printer.h"
#include "webportal.h"

static void initRuntimeServices() {
  lable1_1_Semaphore = xSemaphoreCreateMutex();
  adcSemaphore = xSemaphoreCreateMutex();
  preferencesSemaphore = xSemaphoreCreateRecursiveMutex();
  touchWireMutex = xSemaphoreCreateRecursiveMutex();
  if (!lable1_1_Semaphore || !adcSemaphore || !preferencesSemaphore || !touchWireMutex)
    critical_task_start_failed = true;
}

// Task2 = رندر LVGL. اگر ساخته نشود صفحه برای همیشه سیاه می‌ماند.
// باید قبل از splash/ui_extras/WebPortal ساخته شود (وقتی هنوز heap کافی است).
static bool startUiTasks() {
  // Keep rendering away from the high-rate stepper timer ISR on core 1.
  BaseType_t t1ok = xTaskCreatePinnedToCore(Task1, "Task1", 2048, NULL, 5, &h_Task1, 0);
  if (t1ok != pdPASS) return false;
  const uint32_t stacks[] = { 12288, 10240, 8192, 6144 };
  BaseType_t t2ok = pdFAIL;
  for (uint32_t s : stacks) {
    t2ok = xTaskCreatePinnedToCore(Task2, "Task2", s, NULL, 5, &h_Task2, 0);
    ets_printf("T2 create=%d stack=%u heap=%u largest=%u\n",
               (int)t2ok, (unsigned)s, (unsigned)ESP.getFreeHeap(),
               (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    if (t2ok == pdPASS) break;
  }
  return t2ok == pdPASS;
}

static void initDisplayAndTouch() {
  ets_printf("CHK1 h=%u\n", (unsigned)ESP.getFreeHeap());
  // TFT_eSPI owns and initializes the selected SPI host. Starting global FSPI
  // here while TFT_eSPI selected HSPI mapped two peripherals onto the same pins.
  tft.begin();
  ets_printf("CHK2 h=%u\n", (unsigned)ESP.getFreeHeap());
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  ets_printf("CHK3 h=%u\n", (unsigned)ESP.getFreeHeap());
  initTouchController();
  ets_printf("CHK4 h=%u\n", (unsigned)ESP.getFreeHeap());
  lv_init();
  Setup_Gui();
  ets_printf("CHK5 h=%u largest=%u\n", (unsigned)ESP.getFreeHeap(),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
  register_clb();
  ets_printf("CHK6 h=%u\n", (unsigned)ESP.getFreeHeap());
  init_widgets();
  ets_printf("CHK7 h=%u\n", (unsigned)ESP.getFreeHeap());
  // بحرانی: Task2 را همین‌جا بساز — قبل از splash/extras که heap را تکه‌تکه می‌کنند
  if (!startUiTasks()) {
    ets_printf("FATAL: Task2 failed — black screen would occur\n");
    tft.fillScreen(TFT_RED);
    while (1) delay(1000);
  }
  // حافظهٔ داخلی شبکه را پیش از ساخت آبجکت‌های سنگین UI رزرو کن.
  // اسکن بعدی دیگر به بلوک پیوسته‌ای وابسته نیست که در پایان بوت تکه‌تکه شده باشد.
  uf_start_net_task_once();
  if (uf_net_task_started) {
    const bool wifi_ready = uf_wifi_ensure_sta();
    // در بوت عادی حافظهٔ درایور رزرو می‌ماند، اما رادیوی بیکار نباید
    // با sleep=false بار Core0/RF بسازد. اسکن واقعی دوباره آن را بیدار می‌کند.
    if (wifi_ready && !g_update_mode) WiFi.setSleep(true);
    ets_printf("[UF] early reserve ready=%d heap=%u internal=%u largest=%u\n",
               (int)wifi_ready, (unsigned)ESP.getFreeHeap(),
               (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
               (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
  }
  ui_extras_init();
  ets_printf("CHK8 h=%u\n", (unsigned)ESP.getFreeHeap());
  splash_show();
  // Task2 runs on the other core; release it only after boot-time LVGL writes end.
  ui_bootstrap_complete = true;
  ets_printf("CHK9 h=%u\n", (unsigned)ESP.getFreeHeap());
}

static void startApplicationTasks() {
  // هستهٔ 1: استپر + ایمنی | هستهٔ 0: بازر/کلید پنل
  // Task1/Task2 قبلاً در initDisplayAndTouch ساخته شده‌اند
  BaseType_t ok;
  ok = xTaskCreatePinnedToCore(Task3, "Task3", 3072, NULL, 4, &h_Task3, 1);
  ets_printf("Task3 create=%d heap=%u\n", (int)ok, (unsigned)ESP.getFreeHeap());
  if (ok != pdPASS) critical_task_start_failed = true;
  ok = xTaskCreatePinnedToCore(Task4, "Task4", 3072, NULL, 4, &h_Task4, 1);
  ets_printf("Task4 create=%d heap=%u\n", (int)ok, (unsigned)ESP.getFreeHeap());
  ok = xTaskCreatePinnedToCore(Task5, "Task5", 1536, NULL, 5, &h_Task5, 0);
  ets_printf("Task5 create=%d heap=%u\n", (int)ok, (unsigned)ESP.getFreeHeap());
  ok = xTaskCreatePinnedToCore(Task6, "Task6", 2048, NULL, 5, &h_Task6, 1);
  ets_printf("Task6 create=%d heap=%u\n", (int)ok, (unsigned)ESP.getFreeHeap());
  if (ok != pdPASS) critical_task_start_failed = true;
  ok = xTaskCreatePinnedToCore(Task7, "Task7", 2048, NULL, 5, &h_Task7, 1);
  ets_printf("Task7 create=%d heap=%u\n", (int)ok, (unsigned)ESP.getFreeHeap());
  if (ok != pdPASS) critical_task_start_failed = true;
  ok = xTaskCreatePinnedToCore(Task8, "Task8", 3072, NULL, 5, &h_Task8, 1);
  ets_printf("Task8 create=%d heap=%u\n", (int)ok, (unsigned)ESP.getFreeHeap());
  if (ok != pdPASS) critical_task_start_failed = true;
  ok = xTaskCreatePinnedToCore(Task9, "Task9", 2048, NULL, 4, &h_Task9, 0);
  ets_printf("Task9 create=%d heap=%u\n", (int)ok, (unsigned)ESP.getFreeHeap());
  ok = xTaskCreatePinnedToCore(Task10, "Task10", 2048, NULL, 4, &h_Task10, 0);
  ets_printf("Task10 create=%d heap=%u\n", (int)ok, (unsigned)ESP.getFreeHeap());
  if (ok != pdPASS) critical_task_start_failed = true;
  ok = xTaskCreatePinnedToCore(Task11, "Task11", 1536, NULL, 5, &h_Task11, 1);
  ets_printf("Task11 create=%d heap=%u\n", (int)ok, (unsigned)ESP.getFreeHeap());
  if (ok != pdPASS) critical_task_start_failed = true;
  if (critical_task_start_failed) {
    sensor_alert_motion_lock = true;
    device_locked = true;
    STP.referenced = false;
    STP.jogEnabled = false;
    STP.moveStepperCMD = false;
    STP.mvm_interupt = true;
    ets_printf("FATAL: critical task missing; motion locked\n");
  }
}

static void build_ap_ssid() {
  snprintf(ap_ssid, sizeof(ap_ssid), "Haffler-Cutter-V%s", PROJECT_VERSION_STRING);
  ap_ssid[sizeof(ap_ssid) - 1] = '\0';
}

bool startWebPortalTask() {
  build_ap_ssid();
  if (webPortalTaskHandle != NULL) return true;
  BaseType_t ok = xTaskCreatePinnedToCore(WebPortalTask, "WebPortal", 4096, NULL, 2, &webPortalTaskHandle, 0);
  ets_printf("WebPortal create=%d heap=%u ssid=%s\n", (int)ok, (unsigned)ESP.getFreeHeap(), ap_ssid);
  return ok == pdPASS;
}

static void deferredHeavyTasks(void *pv) {
  // شروع اتصال پس‌زمینه به وای‌فای از ۵۰۰ میلی‌ثانیه بوت همزمان با نمایش اسپلش
  vTaskDelay(pdMS_TO_TICKS(500));
  BaseType_t ok = xTaskCreatePinnedToCore(SerialImportTask, "SerialImport", 4096, NULL, 2, NULL, 0);
  ets_printf("Deferred SerialImport create=%d heap=%u\n", (int)ok, (unsigned)ESP.getFreeHeap());

  // بررسی اتصال خودکار به وای‌فای ذخیره‌شده، همگام‌سازی ساعت با NTP و بررسی بروزرسانی
  String ss, pw;
  uf_get_persisted_wifi(ss, pw);
  if (ss.length() > 0 && wifi_owner_get() == WIFI_OWNER_NONE) {
    Serial.printf("[StartupNet] Connecting to persisted WiFi: %s\n", ss.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(ss.c_str(), pw.length() > 0 ? pw.c_str() : nullptr);
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 25) {
      if (wifi_owner_get() != WIFI_OWNER_NONE) {
        Serial.println("[StartupNet] WiFi ownership changed; aborting startup net task.");
        vTaskDelete(NULL);
        return;
      }
      vTaskDelay(pdMS_TO_TICKS(500));
      retry++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[StartupNet] WiFi Connected! IP: %s\n", WiFi.localIP().toString().c_str());
      warranty_on_wifi_connected();

      // ۱. همگام‌سازی ساعت با NTP در صورت اختلاف بیش از ۱ دقیقه
      configTime(3.5 * 3600, 0, "pool.ntp.org", "time.google.com", "ir.pool.ntp.org");
      time_t now = 0;
      int ntp_retry = 0;
      while (ntp_retry < 16) {
        now = time(nullptr);
        if (now > 1704067200LL) break; // زمان معتبر بعد از ۲۰۲۴
        vTaskDelay(pdMS_TO_TICKS(500));
        ntp_retry++;
      }

      if (now > 1704067200LL) {
        int ry = 0, rm = 0, rd = 0, rh = 0, rmi = 0, rs = 0;
        bool rtc_read_ok = ds3231_read_time(&ry, &rm, &rd, &rh, &rmi, &rs);
        int64_t rtc_epoch = rtc_read_ok ? civil_to_epoch(ry, rm, rd, rh, rmi, rs) : 0;
        int64_t diff = (int64_t)now - rtc_epoch;
        if (diff < 0) diff = -diff;
        Serial.printf("[StartupNet] NTP: %lld, RTC: %lld, Diff: %lld s\n", (long long)now, (long long)rtc_epoch, (long long)diff);
        if (!rtc_read_ok || diff > 60 || ry < 2024) {
          struct tm tm_info;
          localtime_r(&now, &tm_info);
          int ny = tm_info.tm_year + 1900;
          int nm = tm_info.tm_mon + 1;
          int nd = tm_info.tm_mday;
          int nh = tm_info.tm_hour;
          int nmi = tm_info.tm_min;
          int ns = tm_info.tm_sec;
          if (rtc_read_ok) ds3231_write_time(ny, nm, nd, nh, nmi, ns);
          soft_clock_set_civil(ny, nm, nd, nh, nmi, ns);
          g_flag_time_synced_toast = true;
          Serial.println("[StartupNet] Clock synchronized with NTP!");
        }
      }

      // ۲. استعلام نگارش جدید از سرور در پس‌زمینه استارتاپ
      if (wifi_owner_get() == WIFI_OWNER_NONE) {
        WiFiClientSecure secClient;
        WiFiClient plainClient;
        HTTPClient http;
        String vUrl = UF_OTA_CUTTER_VERSION_URL;
        if (vUrl.startsWith("https://")) {
          secClient.setInsecure();
          secClient.setTimeout(8);
          http.begin(secClient, vUrl);
        } else {
          plainClient.setTimeout(8);
          http.begin(plainClient, vUrl);
        }
        http.setTimeout(7000);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        int code = http.GET();
        if (code == 200) {
          String body = http.getString();
          char found_ver[32] = {0};
          int ls = 0;
          while (ls < body.length()) {
            int le = body.indexOf('\n', ls);
            if (le < 0) le = body.length();
            String l = body.substring(ls, le);
            l.trim();
            if (l.startsWith("display=") && found_ver[0] == 0) {
              strncpy(found_ver, l.substring(8).c_str(), sizeof(found_ver) - 1);
            } else if (l.startsWith("code=") && found_ver[0] == 0) {
              strncpy(found_ver, l.substring(5).c_str(), sizeof(found_ver) - 1);
            }
            ls = le + 1;
          }
          if (found_ver[0] == 0) {
            String v = body; v.trim();
            if (v.length() > 0 && v.length() <= 20) strncpy(found_ver, v.c_str(), sizeof(found_ver) - 1);
          }
          if (found_ver[0] != 0 && strcmp(found_ver, PROJECT_VERSION_STRING) != 0 && strcmp(found_ver, PROJECT_VERSION_DISPLAY) != 0) {
            Serial.printf("[StartupNet] New update available on server: %s (installed: %s)\n", found_ver, PROJECT_VERSION_STRING);
            g_flag_update_available_toast = true;
          }
        }
        http.end();
      }
    } else {
      Serial.println("[StartupNet] Saved WiFi network not reachable.");
    }
  }

  vTaskDelete(NULL);
}

void setup() {
  // حتماً اولین کار بوت: قبل از هر callback/LVGL که scr_id=0 را ثبت کند.
  ui_resume_boot_latch_early();
  Serial.begin(SERIAL_DEBUG_BAUD);
  haffler_fault_log_init();
  esp_reset_reason_t rr = esp_reset_reason();
  if (rr == ESP_RST_BROWNOUT) {
    haffler_fault_log_record(FAULT_BROWNOUT, (int16_t)rr);
  } else if (rr == ESP_RST_TASK_WDT || rr == ESP_RST_INT_WDT) {
    haffler_fault_log_record(FAULT_WATCHDOG_RESET, (int16_t)rr);
  } else if (rr == ESP_RST_PANIC) {
    haffler_fault_log_record(FAULT_CORE_PANIC, (int16_t)rr);
  }
  pinMode(45, OUTPUT);
  digitalWrite(45, LOW);
  ledcWrite(45, 0);
  initRuntimeServices();
  delay(100);
  recall_params();
  warranty_init();
  if (warranty_is_expired_and_locked()) {
    device_locked = true;
  }
  pinmap_in_load();
  pinmap_out_load();
  delay(200);
  update_flow_boot_check();   // F5 هنگامِ روشن‌شدن نگه داشته شده؟ → g_update_mode
  initDisplayAndTouch();
  fw_install_history_register_current();
  startApplicationTasks();
  update_flow_start();        // اگر F5-بوت بود: جریانِ رمز→وایفای→آپدیت را راه بینداز
  ets_printf("CHKa h=%u\n", (unsigned)ESP.getFreeHeap());
  delay(100);
  printer.begin(19200, SERIAL_8N1, 18, 17);
  delay(100);
  ets_printf("CHKb h=%u\n", (unsigned)ESP.getFreeHeap());


  // B7: بلوک کامنت‌شدهٔ قدیمی وب‌سرور حذف شد — نسخهٔ فعال پایین داخل ENABLE_WIFI_UPLOAD است
  if (ENABLE_WIFI_UPLOAD) {
    WiFi.persistent(false);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_pass);
    Serial.println("AP Started");
    Serial.println(WiFi.softAPIP());

  //dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

    uploadSemaphore = xSemaphoreCreateBinary();

  // ===== Root =====
    server.on("/", HTTP_GET, sendPortalPage);

  // ===== Upload =====
    server.on("/upload", HTTP_POST,
      []() {
        server.send(200, "text/plain", "OK");
      },
      handleFileUpload
    );


/*
  // ===== Windows Captive Portal FIX =====
  server.on("/connecttest.txt", [](){
    server.sendHeader("Location", "http://192.168.4.1/");
    server.send(302, "text/plain", "");
  });

  server.on("/redirect", [](){
    server.sendHeader("Location", "http://192.168.4.1/");
    server.send(302, "text/plain", "");
  });

  // ===== Force Redirect Everything =====
  server.onNotFound([](){
    server.sendHeader("Location", "http://192.168.4.1/");
    server.send(302, "text/plain", "");
  });
*/
    server.begin();

    xTaskCreatePinnedToCore(TaskWebServer,"WebTask",3000,NULL,5,&webTaskHandle,0);
  } else {
    Serial.println("WiFi upload disabled");
  }

  xTaskCreatePinnedToCore(PrinterTask, "Printer Task", 4096, NULL, 1, &PrinterTaskHandle, 0);
  // WebPortal/SerialImport با تأخیر — قبلاً ۸KB استک در بوت Task2 را می‌کشت و صفحه سیاه می‌شد
  // در حالتِ F5-بوت راه‌اندازی نمی‌شوند تا وب‌پورتالِ AP با وایفای STAِ آپدیت تداخل نکند.
  if (!g_update_mode)
    xTaskCreatePinnedToCore(deferredHeavyTasks, "DeferHeavy", 5120, NULL, 1, NULL, 0);

  Serial.print("Free Heap: ");
  Serial.println(ESP.getFreeHeap());
}


void loop() {
  // همهٔ کارها در تسک‌های FreeRTOS انجام می‌شود — loop لازم نیست.
  // (برای دیباگ stack/heap از uxTaskGetStackHighWaterMark / ESP.getFreeHeap استفاده کنید)
  vTaskDelete(NULL);
}


static const char PORTAL_PAGE_PROGMEM[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Upload</title>
<style>
body {
  margin:0;
  font-family:Arial;
  background:linear-gradient(135deg,#1e3c72,#2a5298);
  display:flex;
  justify-content:center;
  align-items:center;
  height:100vh;
}
.card {
  background:white;
  padding:30px;
  border-radius:15px;
  box-shadow:0 10px 25px rgba(0,0,0,0.2);
  text-align:center;
  width:350px;
}

/* بخش متن فارسی */
.persian-box{
  margin-top:20px;
  text-align:right;
  direction:rtl;
  background:#f5f5f5;
  padding:15px;
  border-radius:10px;
  font-size:14px;
  line-height:1.8;
}

button {
  background:#2a5298;
  color:white;
  border:none;
  padding:10px 20px;
  border-radius:8px;
  cursor:pointer;
}
.progress {
  width:100%;
  background:#eee;
  border-radius:10px;
  margin-top:15px;
  display:none;
}
.progress-bar {
  height:10px;
  width:0%;
  background:#2a5298;
  border-radius:10px;
}
.success {
  color:green;
  margin-top:15px;
  display:none;
}
</style>
</head>
<body>

<div class="card">
<h2>Upload TXT File</h2>
<input type="file" id="fileInput"><br><br>
<button onclick="uploadFile()">Upload</button>

<div class="progress" id="progressBox">
  <div class="progress-bar" id="progressBar"></div>
</div>

<div class="success" id="successMsg">
Upload Successful ✅
</div>

<!-- متن فارسی اضافه شده -->
<div class="persian-box">
<b>فرمت ورودی</b>:<br><br>
,تعدادXمتراژ<br>
,تعدادXمتراژ<br>
,تعدادXمتراژ<br>
,تعدادXمتراژ<br>
...
</div>

</div>

<script>
const uploadFile = () => {
  var file = document.getElementById("fileInput").files[0];
  if (!file) { alert("Select file"); return; }

  var formData = new FormData();
  formData.append("file", file);

  var xhr = new XMLHttpRequest();
  xhr.open("POST", "/upload", true);

  document.getElementById("progressBox").style.display="block";

  xhr.upload.onprogress = (e) => {
    if (e.lengthComputable) {
      var percent = (e.loaded/e.total)*100;
      document.getElementById("progressBar").style.width=percent+"%";
    }
  };

  xhr.onload = () => {
    document.getElementById("successMsg").style.display="block";
  };

  xhr.send(formData);
};
</script>

</body>
</html>
)rawliteral";

void sendPortalPage() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  server.send_P(200, "text/html; charset=UTF-8", PORTAL_PAGE_PROGMEM);
}


// ================== WEB TASK ==================
void TaskWebServer(void *pvParameters) {
  while(true) {
    //dnsServer.processNextRequest();
    server.handleClient();
    vTaskDelay(1);
  }
}

// ================== UPLOAD ==================
void handleFileUpload() {

  HTTPUpload& upload = server.upload();
  static String tempNumber = "";

  if(upload.status == UPLOAD_FILE_START) {
    numbersCount = 0;
    tempNumber = "";
  }

  else if(upload.status == UPLOAD_FILE_WRITE) {

    for(size_t i=0;i<upload.currentSize;i++) {

      char c = upload.buf[i];

      if(c == ',' || c == 'X' || c == '\n' || c == '\r' || c == ' ' || c == '\t') {
        if(numbersCount < MAX_NUMBERS) {
          if (tempNumber.length() > 0) {
            numbers[numbersCount++] = tempNumber.toInt();
          }
        }
        tempNumber = "";
      }
      else if(isDigit(c) || c=='-') {
        tempNumber += c;
      }
    }
  }

  else if(upload.status == UPLOAD_FILE_END) {

    if(tempNumber.length() > 0 && numbersCount < MAX_NUMBERS) {
      numbers[numbersCount++] = tempNumber.toInt();
    }

    Serial.println("Parsed Numbers:");
    for(int i=0;i<numbersCount;i++)
      Serial.println(numbers[i]);
  }


  if (upload.status != UPLOAD_FILE_END) return;
  if (GUI.scr_id != 3) return;
  if (numbersCount <= 0 || numbersCount > MAX_NUMBERS || (numbersCount & 1)) {
    GUI.buzzer_cmd_long = true;
    Serial.printf("Upload rejected: expected 1..%d LENGTH,COUNT pairs.\n", LIST_MAX);
    return;
  }

  GUI.buzzer_cmd_short = true;
  int last_import_row = -1;
  bool import_count_clamped = false;
  int import_rows = numbersCount / 2;
  if (import_rows > LIST_MAX) import_rows = LIST_MAX;
  for (int i = 0; i < import_rows; i++) {
    const int spIndex = i * 2;
    const int countIndex = spIndex + 1;
    GUI.clear_buffer_aut_flag = true;

    GUI.inputBuffer_L_3_1_x[i] = String(numbers[spIndex]);
    AUT.list_sp_Val_p[i] = GUI.inputBuffer_L_3_1_x[i].toFloat();
    save_permanently_preferences_sp_val(i, AUT.list_sp_Val_p[i]);
    GUI.flag_update_labels_3_1 = true;

    GUI.inputBuffer_L_3_2_x[i] = String(numbers[countIndex]);
    int raw_count = GUI.inputBuffer_L_3_2_x[i].toInt();
    AUT.list_count_Val[i] = constrain(raw_count, 0, 999);
    if (raw_count != AUT.list_count_Val[i]) import_count_clamped = true;
    GUI.inputBuffer_L_3_2_x[i] = String(AUT.list_count_Val[i]);
    AUT.list_count_base_Val[i] = AUT.list_count_Val[i];
    save_permanently_preferences_count_val(i, AUT.list_count_Val[i]);
    save_permanently_preferences_count_base(i, AUT.list_count_base_Val[i]);
    GUI.flag_update_labels_3_2 = true;
    if (AUT.list_sp_Val_p[i] > 0 && AUT.list_count_Val[i] > 0) last_import_row = i;

    GUI.flag_panel3_edit_labels_txt = true;
  }
  GUI.visible_raws = (last_import_row >= 0) ? (last_import_row + 1) : 0;
  PEND.visible_raws = GUI.visible_raws;
  GUI.flag_save_visible_raws = true;
  flush_pending_list_saves_now();
  GUI.flag_refresh_visible = true;
  GUI.flag_update_page = true;
  if (import_count_clamped) {
    ui_toast("ظرفیت تعداد هر ردیف 999 عدد است", "Row quantity capacity is 999", 3500);
  }
}
