# فرهنگ نمادهای حیاتی ماشین‌آلات هافلر (HAFFLER_CORE_SYMBOLS.md)
## نمایه رسمی توابع ایمنی، پین‌ها، خطاها و ماکروهای حیاتی جهت حذف کامل توهم نام توابع

> **دستورالعمل برای ایجنت‌ها:** قبل از فراخوانی یا ارجاع به هر تابع یا پین، نام دقیق و امضای آن را در این سند چک کنید.

### ICE CREEAM (15 نماد کلیدی)
`c
#define FAULT_LOG_MAX_ENTRIES 10              // main\fault_log.h:9
#define PIN_BL     -1                         // main\project_config.h:24
#define PIN_CS     13                         // main\project_config.h:21
#define PIN_DC     14                         // main\project_config.h:22
#define PIN_IS_ON(pin)  (gpio_get_level(pin) == 0) // main\ui\screens.c:6239
#define PIN_MISO   -1                         // main\project_config.h:19
#define PIN_MOSI   10                         // main\project_config.h:18
#define PIN_RST    -1                         // main\project_config.h:23
#define PIN_SCLK   12                         // main\project_config.h:20
bool cylinder_freeze_protection_trip          // main\extra_modes.c:414
bool cylinder_freeze_protection_trip          // main\extra_modes.h:57
bool lvgl_lock                                // main\lvgl_port.c:63
bool lvgl_lock                                // main\lvgl_port.h:13
void day_lock_init                            // main\day_lock_core.c:553
void day_lock_init                            // main\day_lock_core.h:12
`

### Welding_single_head (32 نماد کلیدی)
`c
#define PIN_BUZZER  02  // 18                 // include\setting.h:168
#define PIN_D0  16  // GPIO16       WAKE      // lib\TFT_eSPI\User_Setup_Select.h:286
#define PIN_D1   5  // GPIO5        User purpose // lib\TFT_eSPI\User_Setup_Select.h:287
#define PIN_D10  1  //              TXD0      // lib\TFT_eSPI\User_Setup_Select.h:296
#define PIN_D11  9  // SD2                    // lib\TFT_eSPI\User_Setup_Select.h:303
#define PIN_D12 10  // SD4                    // lib\TFT_eSPI\User_Setup_Select.h:304
#define PIN_D2   4  // GPIO4        User purpose // lib\TFT_eSPI\User_Setup_Select.h:288
#define PIN_D3   0  // GPIO0        Low on boot means enter FLASH mode // lib\TFT_eSPI\User_Setup_Select.h:289
#define PIN_D4   2  // GPIO2        TXD1 (must be high on boot to go to UART0 FLASH mode) // lib\TFT_eSPI\User_Setup_Select.h:290
#define PIN_D5  14  // GPIO14       HSCLK     // lib\TFT_eSPI\User_Setup_Select.h:291
#define PIN_D6  12  // GPIO12       HMISO     // lib\TFT_eSPI\User_Setup_Select.h:292
#define PIN_D7  13  // GPIO13       HMOSI  RXD2 // lib\TFT_eSPI\User_Setup_Select.h:293
#define PIN_D8  15  // GPIO15       HCS    TXD0 (must be low on boot to enter UART0 FLASH mode) // lib\TFT_eSPI\User_Setup_Select.h:294
#define PIN_D9   3  //              RXD0      // lib\TFT_eSPI\User_Setup_Select.h:295
#define PIN_HWCS 0  // D3                     // lib\TFT_eSPI\User_Setup_Select.h:301
#define PIN_MISO 7  // SD0                    // lib\TFT_eSPI\User_Setup_Select.h:299
#define PIN_MOSI 8  // SD1          FLASH and overlap mode // lib\TFT_eSPI\User_Setup_Select.h:298
#define PIN_NC (-1)                           // include\defines.h:52
#define PIN_RGB 00                            // include\setting.h:167
#define PIN_SCLK 6  // CLK                    // lib\TFT_eSPI\User_Setup_Select.h:300
#define SENSOR_BAND_BOTH_HI  2750             // include\defines.h:123
#define SENSOR_BAND_BOTH_LO  2350             // include\defines.h:122
#define SENSOR_BAND_CLOSED   3500 // سنسورِ «بسته»: > 3500 // include\defines.h:124
#define SENSOR_BAND_NONE_HI  2240             // include\defines.h:121
#define SENSOR_BAND_NONE_LO  1850             // include\defines.h:120
#define SENSOR_BAND_OPEN_HI  1800             // include\defines.h:119
#define SENSOR_BAND_OPEN_LO  1300 // سنسورِ «باز»: 1300..1800 // include\defines.h:118
#define SENSOR_TIMEOUT_MS    7000 // اگر سنسور تا ۷ ثانیه نرسید → خطا + استوپ // include\defines.h:125
bool get_var_emergency_stop                   // src\vars.c:88
void hal_io_all_outputs_off                   // components\hal_io\include\hal_io.h:92
void hal_io_all_outputs_off                   // components\hal_io\src\hal_io.cpp:218
void set_var_emergency_stop                   // src\vars.c:92
`

### Cutter_single_head(Digital_metr) (34 نماد کلیدی)
`c
#define PIN_D0  16  // GPIO16       WAKE      // firmware\.pio\libdeps\esp32-s3-devkitc-1\TFT_eSPI\User_Setup_Select.h:286
#define PIN_D0  16  // GPIO16       WAKE      // libraries\TFT_eSPI\User_Setup_Select.h:286
#define PIN_D1   5  // GPIO5        User purpose // firmware\.pio\libdeps\esp32-s3-devkitc-1\TFT_eSPI\User_Setup_Select.h:287
#define PIN_D1   5  // GPIO5        User purpose // libraries\TFT_eSPI\User_Setup_Select.h:287
#define PIN_D10  1  //              TXD0      // firmware\.pio\libdeps\esp32-s3-devkitc-1\TFT_eSPI\User_Setup_Select.h:296
#define PIN_D10  1  //              TXD0      // libraries\TFT_eSPI\User_Setup_Select.h:296
#define PIN_D11  9  // SD2                    // firmware\.pio\libdeps\esp32-s3-devkitc-1\TFT_eSPI\User_Setup_Select.h:303
#define PIN_D11  9  // SD2                    // libraries\TFT_eSPI\User_Setup_Select.h:303
#define PIN_D12 10  // SD4                    // firmware\.pio\libdeps\esp32-s3-devkitc-1\TFT_eSPI\User_Setup_Select.h:304
#define PIN_D12 10  // SD4                    // libraries\TFT_eSPI\User_Setup_Select.h:304
#define PIN_D2   4  // GPIO4        User purpose // firmware\.pio\libdeps\esp32-s3-devkitc-1\TFT_eSPI\User_Setup_Select.h:288
#define PIN_D2   4  // GPIO4        User purpose // libraries\TFT_eSPI\User_Setup_Select.h:288
#define PIN_D3   0  // GPIO0        Low on boot means enter FLASH mode // firmware\.pio\libdeps\esp32-s3-devkitc-1\TFT_eSPI\User_Setup_Select.h:289
#define PIN_D3   0  // GPIO0        Low on boot means enter FLASH mode // libraries\TFT_eSPI\User_Setup_Select.h:289
#define PIN_D4   2  // GPIO2        TXD1 (must be high on boot to go to UART0 FLASH mode) // firmware\.pio\libdeps\esp32-s3-devkitc-1\TFT_eSPI\User_Setup_Select.h:290
#define PIN_D4   2  // GPIO2        TXD1 (must be high on boot to go to UART0 FLASH mode) // libraries\TFT_eSPI\User_Setup_Select.h:290
#define PIN_D5  14  // GPIO14       HSCLK     // firmware\.pio\libdeps\esp32-s3-devkitc-1\TFT_eSPI\User_Setup_Select.h:291
#define PIN_D5  14  // GPIO14       HSCLK     // libraries\TFT_eSPI\User_Setup_Select.h:291
#define PIN_D6  12  // GPIO12       HMISO     // firmware\.pio\libdeps\esp32-s3-devkitc-1\TFT_eSPI\User_Setup_Select.h:292
#define PIN_D6  12  // GPIO12       HMISO     // libraries\TFT_eSPI\User_Setup_Select.h:292
#define PIN_D7  13  // GPIO13       HMOSI  RXD2 // firmware\.pio\libdeps\esp32-s3-devkitc-1\TFT_eSPI\User_Setup_Select.h:293
#define PIN_D7  13  // GPIO13       HMOSI  RXD2 // libraries\TFT_eSPI\User_Setup_Select.h:293
#define PIN_D8  15  // GPIO15       HCS    TXD0 (must be low on boot to enter UART0 FLASH mode) // firmware\.pio\libdeps\esp32-s3-devkitc-1\TFT_eSPI\User_Setup_Select.h:294
#define PIN_D8  15  // GPIO15       HCS    TXD0 (must be low on boot to enter UART0 FLASH mode) // libraries\TFT_eSPI\User_Setup_Select.h:294
#define PIN_D9   3  //              RXD0      // firmware\.pio\libdeps\esp32-s3-devkitc-1\TFT_eSPI\User_Setup_Select.h:295
#define PIN_D9   3  //              RXD0      // libraries\TFT_eSPI\User_Setup_Select.h:295
#define PIN_HWCS 0  // D3                     // firmware\.pio\libdeps\esp32-s3-devkitc-1\TFT_eSPI\User_Setup_Select.h:301
#define PIN_HWCS 0  // D3                     // libraries\TFT_eSPI\User_Setup_Select.h:301
#define PIN_MISO 7  // SD0                    // firmware\.pio\libdeps\esp32-s3-devkitc-1\TFT_eSPI\User_Setup_Select.h:299
#define PIN_MISO 7  // SD0                    // libraries\TFT_eSPI\User_Setup_Select.h:299
#define PIN_MOSI 8  // SD1          FLASH and overlap mode // firmware\.pio\libdeps\esp32-s3-devkitc-1\TFT_eSPI\User_Setup_Select.h:298
#define PIN_MOSI 8  // SD1          FLASH and overlap mode // libraries\TFT_eSPI\User_Setup_Select.h:298
#define PIN_SCLK 6  // CLK                    // firmware\.pio\libdeps\esp32-s3-devkitc-1\TFT_eSPI\User_Setup_Select.h:300
#define PIN_SCLK 6  // CLK                    // libraries\TFT_eSPI\User_Setup_Select.h:300
`

### Bakery (20 نماد کلیدی)
`c
#define PIN_D0  16  // GPIO16       WAKE      // V.1.1.0\ChoneProject\libraries\TFT_eSPI\User_Setup_Select.h:286
#define PIN_D1   5  // GPIO5        User purpose // V.1.1.0\ChoneProject\libraries\TFT_eSPI\User_Setup_Select.h:287
#define PIN_D10  1  //              TXD0      // V.1.1.0\ChoneProject\libraries\TFT_eSPI\User_Setup_Select.h:296
#define PIN_D11  9  // SD2                    // V.1.1.0\ChoneProject\libraries\TFT_eSPI\User_Setup_Select.h:303
#define PIN_D12 10  // SD4                    // V.1.1.0\ChoneProject\libraries\TFT_eSPI\User_Setup_Select.h:304
#define PIN_D2   4  // GPIO4        User purpose // V.1.1.0\ChoneProject\libraries\TFT_eSPI\User_Setup_Select.h:288
#define PIN_D3   0  // GPIO0        Low on boot means enter FLASH mode // V.1.1.0\ChoneProject\libraries\TFT_eSPI\User_Setup_Select.h:289
#define PIN_D4   2  // GPIO2        TXD1 (must be high on boot to go to UART0 FLASH mode) // V.1.1.0\ChoneProject\libraries\TFT_eSPI\User_Setup_Select.h:290
#define PIN_D5  14  // GPIO14       HSCLK     // V.1.1.0\ChoneProject\libraries\TFT_eSPI\User_Setup_Select.h:291
#define PIN_D6  12  // GPIO12       HMISO     // V.1.1.0\ChoneProject\libraries\TFT_eSPI\User_Setup_Select.h:292
#define PIN_D7  13  // GPIO13       HMOSI  RXD2 // V.1.1.0\ChoneProject\libraries\TFT_eSPI\User_Setup_Select.h:293
#define PIN_D8  15  // GPIO15       HCS    TXD0 (must be low on boot to enter UART0 FLASH mode) // V.1.1.0\ChoneProject\libraries\TFT_eSPI\User_Setup_Select.h:294
#define PIN_D9   3  //              RXD0      // V.1.1.0\ChoneProject\libraries\TFT_eSPI\User_Setup_Select.h:295
#define PIN_HWCS 0  // D3                     // V.1.1.0\ChoneProject\libraries\TFT_eSPI\User_Setup_Select.h:301
#define PIN_MISO 7  // SD0                    // V.1.1.0\ChoneProject\libraries\TFT_eSPI\User_Setup_Select.h:299
#define PIN_MOSI 8  // SD1          FLASH and overlap mode // V.1.1.0\ChoneProject\libraries\TFT_eSPI\User_Setup_Select.h:298
#define PIN_SCLK 6  // CLK                    // V.1.1.0\ChoneProject\libraries\TFT_eSPI\User_Setup_Select.h:300
#define SENSOR_MOTION_TIMEOUT_MS 5000   // Max duration waiting for inductive sensor before trip // V.1.1.0\ChoneProject\defines.h:66
void day_lock_init                            // V.1.1.0\ChoneProject\day_lock.cpp:143
void day_lock_init                            // V.1.1.0\ChoneProject\day_lock.h:40
`

### bakery-mixer (68 نماد کلیدی)
`c
#define PIN_BOWL_BRAKE            PIN_RELAY_Q2  // GPIO 39 - Rotating Bowl Friction Brake Solenoid (Active-LOW) // include\haffler_pins.h:127
#define PIN_BUZZER                45  // Piezo Buzzer (LEDC Channel) // include\haffler_pins.h:120
#define PIN_EMERGENCY             3   // I6 - Mushroom E-Stop Button (NC, Safety Critical) // include\haffler_pins.h:33
#define PIN_FLOUR_FEEDER          PIN_RELAY_Q1  // GPIO 42 - Motorized Flour Feeder / Auger Screw (Active-LOW) // include\haffler_pins.h:126
#define PIN_I2C_SCL               9   // Hardware I2C0 Clock // include\haffler_pins.h:66
#define PIN_I2C_SDA               8   // Hardware I2C0 Data // include\haffler_pins.h:65
#define PIN_INPUT_ESTOP           PIN_EMERGENCY // GPIO 3 - Mushroom E-Stop Button (NC) // include\haffler_pins.h:133
#define PIN_INPUT_FLOWMETER       PIN_INP_REF   // GPIO 6 - Water Flow Meter Pulse Input (Opto) // include\haffler_pins.h:137
#define PIN_INPUT_GUARD           PIN_INP_GUARD // GPIO 5 - Safety Bowl Guard Interlock (NC) // include\haffler_pins.h:135
#define PIN_INPUT_OVERLOAD        PIN_INP_FAULT // GPIO 4 - Motor Thermal Overload Relay (NC) // include\haffler_pins.h:136
#define PIN_INPUT_START           PIN_START_PEDAL // GPIO 7 - Physical Start Button (NO) // include\haffler_pins.h:134
#define PIN_INP_FAULT             PIN_SENSOR_PAIR_4_5 // include\haffler_pins.h:44
#define PIN_INP_GUARD             PIN_SENSOR_PAIR_2_3 // include\haffler_pins.h:45
#define PIN_INP_REF               PIN_SENSOR_PAIR_0_1 // include\haffler_pins.h:46
#define PIN_JOY_T1                21  // Left Auxiliary Joystick / Pedal 1 // include\haffler_pins.h:59
#define PIN_JOY_T2                37  // Right Auxiliary Joystick / Pedal 2 // include\haffler_pins.h:60
#define PIN_KEY_F                 1   // ADC1_CH0 - Shortcut Keys F1..F5 // include\haffler_pins.h:16
#define PIN_KEY_M                 2   // ADC1_CH1 - Process Keys (SAVE, CANCEL, MODE, START, STOP) // include\haffler_pins.h:17
#define PIN_LCD_BL                (-1)// Backlight tied to 3.3V plane // include\haffler_pins.h:79
#define PIN_LCD_CS                13  // Chip Select // include\haffler_pins.h:73
#define PIN_LCD_DC                14  // Data / Command Select // include\haffler_pins.h:74
#define PIN_LCD_MISO              11  // Master In Slave Out // include\haffler_pins.h:77
#define PIN_LCD_MOSI              10  // Master Out Slave In // include\haffler_pins.h:75
#define PIN_LCD_RST               (-1)// Hardware Reset tied to EN/RC // include\haffler_pins.h:78
#define PIN_LCD_SCK               12  // SPI Serial Clock (up to 80MHz) // include\haffler_pins.h:76
#define PIN_LIGHT_GREEN           PIN_RELAY_Q8  // GPIO 38 - Stack Light Green (Active Mixing) // include\haffler_pins.h:130
#define PIN_LIGHT_RED             PIN_RELAY_Q9  // GPIO 41 - Stack Light Red (Fault / Temperature Alert) // include\haffler_pins.h:131
#define PIN_MIXER_MOTOR           PIN_RELAY_Q0  // GPIO 16 - Single-Speed Mixer Motor Contactor (Active-LOW) // include\haffler_pins.h:125
#define PIN_RELAY_Q0              16  // Output Q0 (Table Cylinder / STEP Pulse) // include\haffler_pins.h:85
#define PIN_RELAY_Q1              42  // Output Q1 (Left Clamp Solenoid / STEP Dir) // include\haffler_pins.h:86
#define PIN_RELAY_Q2              39  // Output Q2 (Right Clamp Solenoid / STEP Enable) // include\haffler_pins.h:87
#define PIN_RELAY_Q3              48  // Output Q3 (Heating Element Advance Solenoid) // include\haffler_pins.h:88
#define PIN_RELAY_Q4              40  // Output Q4 (Shablon / Zero Gauge Solenoid) // include\haffler_pins.h:89
#define PIN_RELAY_Q5              15  // Output Q5 (Solid State Relay - SSR Heater PWM) // include\haffler_pins.h:90
#define PIN_RELAY_Q6              35  // Output Q6 (Knife / Auxiliary Solenoid 1) // include\haffler_pins.h:91
#define PIN_RELAY_Q7              36  // Output Q7 (Knife / Auxiliary Solenoid 2) // include\haffler_pins.h:92
#define PIN_RELAY_Q8              38  // Output Q8 (Spare Terminal Q8 for Field Remap) // include\haffler_pins.h:93
#define PIN_RELAY_Q9              41  // Output Q9 (Spare Terminal Q9 for Field Remap) // include\haffler_pins.h:94
#define PIN_RS485_DE              46  // RS485 Direction Control (DE/RE) // include\haffler_pins.h:114
#define PIN_RS485_DIR             PIN_RS485_DE // Direction Control Alias // include\haffler_pins.h:115
`

### Gas-filling (26 نماد کلیدی)
`c
#define PIN_D0  16  // GPIO16       WAKE      // lib\TFT_eSPI\User_Setup_Select.h:286
#define PIN_D1   5  // GPIO5        User purpose // lib\TFT_eSPI\User_Setup_Select.h:287
#define PIN_D10  1  //              TXD0      // lib\TFT_eSPI\User_Setup_Select.h:296
#define PIN_D11  9  // SD2                    // lib\TFT_eSPI\User_Setup_Select.h:303
#define PIN_D12 10  // SD4                    // lib\TFT_eSPI\User_Setup_Select.h:304
#define PIN_D2   4  // GPIO4        User purpose // lib\TFT_eSPI\User_Setup_Select.h:288
#define PIN_D3   0  // GPIO0        Low on boot means enter FLASH mode // lib\TFT_eSPI\User_Setup_Select.h:289
#define PIN_D4   2  // GPIO2        TXD1 (must be high on boot to go to UART0 FLASH mode) // lib\TFT_eSPI\User_Setup_Select.h:290
#define PIN_D5  14  // GPIO14       HSCLK     // lib\TFT_eSPI\User_Setup_Select.h:291
#define PIN_D6  12  // GPIO12       HMISO     // lib\TFT_eSPI\User_Setup_Select.h:292
#define PIN_D7  13  // GPIO13       HMOSI  RXD2 // lib\TFT_eSPI\User_Setup_Select.h:293
#define PIN_D8  15  // GPIO15       HCS    TXD0 (must be low on boot to enter UART0 FLASH mode) // lib\TFT_eSPI\User_Setup_Select.h:294
#define PIN_D9   3  //              RXD0      // lib\TFT_eSPI\User_Setup_Select.h:295
#define PIN_HWCS 0  // D3                     // lib\TFT_eSPI\User_Setup_Select.h:301
#define PIN_KEY_F      1                      // GasFilling_HMI\panel_keys.cpp:14
#define PIN_KEY_M      2                      // GasFilling_HMI\panel_keys.cpp:15
#define PIN_MISO 7  // SD0                    // lib\TFT_eSPI\User_Setup_Select.h:299
#define PIN_MOSI 8  // SD1          FLASH and overlap mode // lib\TFT_eSPI\User_Setup_Select.h:298
#define PIN_OPTO_START 7                      // GasFilling_HMI\panel_keys.cpp:16
#define PIN_OPTO_STOP  3                      // GasFilling_HMI\panel_keys.cpp:17
#define PIN_OPTO_STOP 3                       // GasFilling_HMI\warranty_mgr.cpp:5
#define PIN_SCLK 6  // CLK                    // lib\TFT_eSPI\User_Setup_Select.h:300
void day_lock_init                            // GasFilling_HMI\day_lock.cpp:198
void day_lock_init                            // GasFilling_HMI\day_lock.h:49
void panel_keys_emergency_stop_all            // GasFilling_HMI\panel_keys.cpp:52
void panel_keys_emergency_stop_all            // GasFilling_HMI\panel_keys.h:35
`

