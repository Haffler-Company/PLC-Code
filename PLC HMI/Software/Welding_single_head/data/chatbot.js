/**
 * Haffler Industrial HMI AI Chatbot Engine - V2.5.0
 * Specialized Offline Industrial Assistant for ESP32-S3 Single-Head UPVC Welding Machine
 * Standards: Siemens Deterministic Automation & Apple HIG Craftsmanship
 */

(function () {
    'use strict';

    // ── 0. State & Configuration ──────────────────────────────────────────────
    const SUPPORT_INFO = {
        phone: '۰۹۱۳-۱۶۹-۴۷۴۶',
        company: 'شرکت مهندسی ماشین‌سازی هافلر (Haffler Machinery)',
        website: 'www.haffler.com',
        email: 'info@haffler.com',
        warranty_period: '۱۸ ماه گارانتی طلایی تعویض قطعات و ۱۰ سال پشتیبانی فنی و قطعات یدکی'
    };

    // ── Knowledge Base: UPVC Profiles & Technical Recipes ─────────────────────
    const KNOWLEDGE_BASE = {
        recipes: {
            'wintech': {
                id: 'wintech',
                name: 'وین‌تک (WinTech 60/70)',
                aliases: ['وین تک', 'وین‌تک', 'وینتک', 'wintech', 'win-tech'],
                temp: 248,
                melt: 25,
                cool: 30,
                pressure: '۶ الی ۷ بار',
                bead: '۲.۵ الی ۳.۰ میلی‌متر',
                desc: 'پروفیل محبوب ۴ و ۵ کاناله با گرید کیفی بالا؛ دمای استاندارد ۲۴۸°C و خنک‌کاری ۳۰ ثانیه'
            },
            'vistabest': {
                id: 'vistabest',
                name: 'ویستابست (Vistabest)',
                aliases: ['ویستابست', 'ویستا بست', 'ویستا', 'vistabest', 'vista-best'],
                temp: 252,
                melt: 28,
                cool: 35,
                pressure: '۶.۵ الی ۷.۵ بار',
                bead: '۲.۵ الی ۳.۰ میلی‌متر',
                desc: 'پروفیل سنگین و با چگالی بالا با استاندارد RAL آلمان؛ نیازمند ۲۸s ذوب و ۳۵s خنک‌کاری'
            },
            'haffler': {
                id: 'haffler',
                name: 'هافلر (Haffler 60)',
                aliases: ['هافلر', 'haffler', 'هافلر ۶۰', 'هافلر 60'],
                temp: 248,
                melt: 25,
                cool: 30,
                pressure: '۶.۵ بار',
                bead: '۲.۸ میلی‌متر',
                desc: 'پروفیل استاندارد صنعتی با پایداری حرارتی بهینه'
            },
            'plaspn': {
                id: 'plaspn',
                name: 'پلاسپن (Plaspn)',
                aliases: ['پلاسپن', 'پلاس پن', 'plaspn', 'plaspen'],
                temp: 246,
                melt: 24,
                cool: 28,
                pressure: '۶.۰ بار',
                bead: '۲.۵ میلی‌متر',
                desc: 'پروفیل سبک با شکل‌پذیری سریع حرارتی'
            },
            'syndezh': {
                id: 'syndezh',
                name: 'سیندژ (SynDezh)',
                aliases: ['سیندژ', 'سین دژ', 'syndezh', 'syn-dezh'],
                temp: 250,
                melt: 26,
                cool: 32,
                pressure: '۶.۵ بار',
                bead: '۲.۸ میلی‌متر',
                desc: 'پروفیل با مقاومت خمشی بالا و جوش‌پذیری یکنواخت'
            }
        }
    };
    KNOWLEDGE_BASE.PROFILE_RECIPES = [
        KNOWLEDGE_BASE.recipes.wintech,
        KNOWLEDGE_BASE.recipes.vistabest,
        KNOWLEDGE_BASE.recipes.haffler,
        KNOWLEDGE_BASE.recipes.plaspn,
        KNOWLEDGE_BASE.recipes.syndezh
    ];

    let lastContext = { topic: null, value: null };
    let pendingSlot = null;
    let pendingRecommendation = null;

    const diagState = {
        active: false,
        tree: null,
        step: 0,
        data: {}
    };

    function resetDiagnostic() {
        diagState.active = false;
        diagState.tree = null;
        diagState.step = 0;
        diagState.data = {};
        pendingRecommendation = null;
    }

    // Telemetry extraction helper
    function getMachineTelemetry() {
        var tm = document.getElementById('TM');
        var curT = document.getElementById('current_T');
        var curH = document.getElementById('current_H');
        var curL = document.getElementById('current_L');
        var wm = document.getElementById('WM');
        var ws = document.getElementById('Ws');
        var actDate = document.getElementById('activ_date');

        var heaterTemp = tm ? parseInt(tm.innerText, 10) : (window.current_temp || 248);
        var targetTemp = curT ? parseInt(curT.innerText, 10) : (window.selectedProgram ? window.selectedProgram.t : 250);
        var meltTime = curH ? parseInt(curH.innerText, 10) : (window.selectedProgram ? window.selectedProgram.h : 25);
        var coolTime = curL ? parseInt(curL.innerText, 10) : (window.selectedProgram ? window.selectedProgram.l : 30);
        var totalWelds = wm ? wm.innerText : '--';
        var sessionWelds = ws ? ws.innerText : '--';
        var activationDate = actDate ? actDate.innerText : '1403/06/10';

        return {
            heaterTemp: isNaN(heaterTemp) ? 248 : heaterTemp,
            targetTemp: isNaN(targetTemp) ? 250 : targetTemp,
            meltTime: isNaN(meltTime) ? 25 : meltTime,
            coolTime: isNaN(coolTime) ? 30 : coolTime,
            totalWelds: totalWelds,
            sessionWelds: sessionWelds,
            activationDate: activationDate,
            activeProgId: (window.selectedProgram && window.selectedProgram.id) ? window.selectedProgram.id : 1
        };
    }

    // ── 1. Digits Normalization ───────────────────────────────────────────────
    function toAsciiDigits(str) {
        if (!str) return '';
        return String(str)
            .replace(/[۰٠]/g, '0').replace(/[۱١]/g, '1').replace(/[۲٢]/g, '2').replace(/[۳٣]/g, '3').replace(/[۴٤]/g, '4')
            .replace(/[۵٥]/g, '5').replace(/[۶٦]/g, '6').replace(/[۷٧]/g, '7').replace(/[۸٨]/g, '8').replace(/[۹٩]/g, '9');
    }

    // ── 2. Guardrails & Profanity Filter ──────────────────────────────────────
    const PROFANITY_LIST = [
        'کص', 'کیر', 'کون', 'جنده', 'کسکش', 'دیوث', 'حرومزاده', 'پدرسگ', 'پدر سوخته', 'کسخول',
        'خارکسه', 'بیناموس', 'بیشرف', 'سیکتیر', 'عوضی', 'fuck', 'shit', 'bitch', 'asshole', 'siktir'
    ];

    function containsProfanity(text) {
        if (!text) return false;
        const normalized = text.toLowerCase().replace(/[\u200c\u200b\s_\-\.]+/g, '');
        for (let word of PROFANITY_LIST) {
            if (normalized.includes(word)) return true;
        }
        return false;
    }

    // ── 3. Execution Engine (Direct JSON Packets via WebSocket) ───────────────
    const Executor = {
        sendWs: function (payload) {
            try {
                var wsConn = window.connection || window.ws;
                if (wsConn && wsConn.readyState === 1) {
                    wsConn.send(JSON.stringify(payload));
                    return true;
                }
            } catch (e) {
                console.warn('[Chatbot] WS send error', e);
            }
            return false;
        },

        setHeaterTemp: function (val) {
            val = parseInt(val, 10);
            if (isNaN(val) || val < 180 || val > 280) {
                return { success: false, msg: '⚠️ دمای استاندارد اتو باید در محدوده ۱۸۰ الی ۲۸۰ درجه سانتی‌گراد باشد (پیشنهاد استاندارد: ۲۴۵ الی ۲۵۵°C).' };
            }
            var telem = getMachineTelemetry();
            var pId = telem.activeProgId;
            var payload = {
                RQ: 5, // UPD_PROG
                ID: pId,
                h: telem.meltTime,
                l: telem.coolTime,
                t: val
            };
            this.sendWs(payload);
            if (window.selectedProgram) window.selectedProgram.t = val;
            var curT = document.getElementById('current_T');
            if (curT) curT.innerText = val + '°';
            return {
                success: true,
                msg: `✅ دمای هدف اتو برای برنامه P${pId} با موفقیت روی ${val}°C تنظیم شد.\nسیستم کنترل هوشمند PID دما را روی این مقدار تثبیت می‌کند.`
            };
        },

        setHeaterTime: function (val) {
            val = parseInt(val, 10);
            if (isNaN(val) || val < 10 || val > 60) {
                return { success: false, msg: '⚠️ زمان ذوب (گرم‌سازی) باید در محدوده ۱۰ الی ۶۰ ثانیه باشد (پیشنهاد استاندارد: ۲۵ الی ۳۰ ثانیه).' };
            }
            var telem = getMachineTelemetry();
            var pId = telem.activeProgId;
            var payload = {
                RQ: 5, // UPD_PROG
                ID: pId,
                h: val,
                l: telem.coolTime,
                t: telem.targetTemp
            };
            this.sendWs(payload);
            if (window.selectedProgram) window.selectedProgram.h = val;
            var curH = document.getElementById('current_H');
            if (curH) curH.innerText = val;
            return {
                success: true,
                msg: `✅ زمان ذوب (گرم‌سازی) برای برنامه P${pId} روی ${val} ثانیه تنظیم و ذخیره شد.`
            };
        },

        setCoolingTime: function (val) {
            val = parseInt(val, 10);
            if (isNaN(val) || val < 10 || val > 60) {
                return { success: false, msg: '⚠️ زمان پرس و خنک‌کاری باید در محدوده ۱۰ الی ۶۰ ثانیه باشد (پیشنهاد استاندارد: ۳۰ الی ۳۵ ثانیه).' };
            }
            var telem = getMachineTelemetry();
            var pId = telem.activeProgId;
            var payload = {
                RQ: 5, // UPD_PROG
                ID: pId,
                h: telem.meltTime,
                l: val,
                t: telem.targetTemp
            };
            this.sendWs(payload);
            if (window.selectedProgram) window.selectedProgram.l = val;
            var curL = document.getElementById('current_L');
            if (curL) curL.innerText = val;
            return {
                success: true,
                msg: `✅ زمان خنک‌کاری و پرس زیر فک‌ها روی ${val} ثانیه تنظیم شد.`
            };
        },

        selectProgram: function (progNum) {
            progNum = parseInt(progNum, 10);
            if (isNaN(progNum) || progNum < 1 || progNum > 5) {
                return { success: false, msg: '⚠️ شماره برنامه کاری جوش باید بین ۱ تا ۵ باشد.' };
            }
            this.sendWs({ RQ: 2, ID: progNum }); // SET_PROG
            return { success: true, msg: `✅ برنامه شماره ${progNum} (P${progNum}) فعال شد.` };
        },

        setPedalCount: function (count) {
            count = parseInt(count, 10);
            if (count !== 3 && count !== 4) {
                return { success: false, msg: '⚠️ تعداد پدال فقط می‌تواند ۳ یا ۴ پدال باشد.' };
            }
            this.sendWs({ RQ: 10, pedal: count }); // CONFIG
            return { success: true, msg: `✅ توالی عملکرد پدال‌ها با موفقیت روی حالت ${count} پدال تنظیم شد.` };
        },

        calibrateTemp: function (offset) {
            offset = parseInt(offset, 10);
            if (isNaN(offset) || offset < -20 || offset > 20) {
                return { success: false, msg: '⚠️ مقدار کالیبراسیون دما باید در بازه ۲۰- الی ۲۰+ درجه سانتی‌گراد باشد.' };
            }
            this.sendWs({ RQ: 10, Tcal: offset }); // CONFIG
            return { success: true, msg: `✅ آفست کالیبراسیون سنسور دما با مقدار ${offset}°C اعمال گردید.` };
        }
    };

    // ── 4. Multilingual Dictionary ───────────────────────────────────────────
    const I18N = {
        FA: {
            greeting: 'درود! 👋 من دستیار هوشمند و متخصص صنعتی دستگاه جوش UPVC هافلر هستم.\nچه کمکی در تنظیمات دقیق یا عیب‌یابی گام‌به‌گام دستگاه نیاز دارید؟',
            chips: ['تنظیم دما', 'زمان ذوب و خنک‌کاری', 'عیب‌یابی رها شدن جک', 'عیب‌یابی المنت و SSR', 'کیفیت و شکستن جوش', 'مشخصات استاندارد'],
            profanity_warning: '⚠️ لطفاً شئونات گفتگو را رعایت فرمایید. به عنوان دستیار تخصصی صنعتی، تنها پاسخگوی سوالات فنی و تنظیمات دستگاه جوش هافلر هستم.',
            optimal_settings: '📊 مقادیر استاندارد و بهینه جوشکاری UPVC (استاندارد EN 514):\n\n' +
                              '• دمای اتو: ۲۴۵ الی ۲۵۵ درجه سانتی‌گراد\n' +
                              '• زمان ذوب (Melt Time): ۲۵ الی ۳۰ ثانیه\n' +
                              '• زمان پرس و خنک‌کاری (Cooling Time): ۳۰ الی ۳۵ ثانیه\n' +
                              '• فشار باد پنوماتیک: ۶ الی ۸ بار (Bar)\n' +
                              '• ضخامت پلیسه استاندارد: ۲.۵ الی ۳ میلی‌متر',
            how_to_weld: '🔥 مراحل استاندارد سیکل جوشکاری تک‌سر UPVC:\n\n' +
                         '۱. قرار دادن دو پروفیل ۴۵ درجه چسبیده به شابلون صفر\n' +
                         '۲. فشردن پدال ۱ (گیره چپ) و پدال ۲ (گیره راست)\n' +
                         '۳. فشردن پدال ۳ / استارت: پایین رفتن شابلون، بالا آمدن اتو و پیشروی میزها\n' +
                         '۴. ذوب کامل لبه‌ها → عقب رفتن اتو → پرس نهایی با جک ضربه\n' +
                         '۵. اتمام زمان خنک‌کاری → بالا رفتن فک‌ها و تحویل گوشه جوش خورده.',
            warranty_info: (d) => `🛡️ گارانتی و خدمات پس از فروش هافلر:\n\n• تاریخ فعال‌سازی سیستم: ${d}\n• دوره پوشش: ${SUPPORT_INFO.warranty_period}\n📞 دفتر مرکزی و پشتیبانی فنی: ${SUPPORT_INFO.phone}\n🌐 وب‌سایت: ${SUPPORT_INFO.website}`
        },
        EN: {
            greeting: 'Hello! I am Haffler UPVC Welder Intelligent Industrial Assistant.\nHow can I help you with machine parameters or step-by-step troubleshooting today?',
            chips: ['Set Temperature', 'Melting & Cooling Time', 'Jack Diagnostics', 'Heater & SSR Diagnostics', 'Weld Quality Issue', 'Standard Specs'],
            profanity_warning: '⚠️ Please maintain professional language. I only assist with machine settings and technical diagnostics.',
            optimal_settings: '📊 Optimal UPVC Welding Parameters (EN 514 Standard):\n\n• Heater Temp: 245 - 255 °C\n• Melting Time: 25 - 30 sec\n• Cooling Time: 30 - 35 sec\n• Air Pressure: 6 - 8 Bar\n• Standard Weld Bead: 2.5 - 3.0 mm',
            how_to_weld: '🔥 Standard Single-Head Welding Steps:\n\n1. Place profiles against 0-shablon\n2. Clamp left & right profile jacks\n3. Press Start: Shablon down, Heater up, Table forward\n4. Fusion melt timing → Heater retracts → Final mechanical press\n5. Cooling finish → Automatic clamp release.',
            warranty_info: (d) => `🛡️ Haffler Warranty & Service:\n\n• Activation Date: ${d}\n• Warranty: 18-month full replacement and 10-year parts support.\n📞 Hotline: ${SUPPORT_INFO.phone}\n🌐 Website: ${SUPPORT_INFO.website}`
        }
    };

    function getActiveLang() {
        return (window.language === 'EN') ? 'EN' : 'FA';
    }

    // ── 5. Multi-Turn Decision Trees (Step-by-Step Guided Workflows) ──────────

    // 5.1 Jack Fault Diagnostic
    function handleJackDiagnostic(lower) {
        diagState.tree = 'JACK_FAULT';

        // Step 0 -> 1: Identify Component
        if (diagState.step === 0) {
            diagState.step = 1;
            return {
                reply: `🔍 [گام ۱ از ۳: انتخاب جک دچار اختلال]\n\n` +
                       `کدام جک دچار عدم حرکت، کندی یا رها شدن وسط کار شده است؟\n\n` +
                       `۱. جک گیره پروفیل چپ (Press Left)\n` +
                       `۲. جک گیره پروفیل راست (Press Right)\n` +
                       `۳. جک اتوی المنت (Heater Element Jack)\n` +
                       `۴. جک میز پیشروی جوش (Welding Table Jack)\n` +
                       `۵. جک شابلون صفر (Shablon 0 Jack)`,
                chips: [
                    'جک گیره چپ',
                    'جک گیره راست',
                    'جک اتوی المنت',
                    'جک میز پیشروی',
                    'جک شابلون صفر'
                ]
            };
        }

        // Step 1 -> 2: Identify Failure Pattern
        if (diagState.step === 1) {
            let jName = 'جک گیره';
            if (lower.includes('چپ') || lower.includes('1') || lower.includes('۱')) jName = 'جک گیره چپ';
            else if (lower.includes('راست') || lower.includes('2') || lower.includes('۲')) jName = 'جک گیره راست';
            else if (lower.includes('اتو') || lower.includes('المنت') || lower.includes('3') || lower.includes('۳')) jName = 'جک اتوی المنت';
            else if (lower.includes('میز') || lower.includes('4') || lower.includes('۴')) jName = 'جک میز پیشروی';
            else if (lower.includes('شابلون') || lower.includes('5') || lower.includes('۵')) jName = 'جک شابلون صفر';

            diagState.data.jackName = jName;
            diagState.step = 2;

            return {
                reply: `🔍 [گام ۲ از ۳: الگوی رخداد خطا برای «${jName}»]\n\n` +
                       `الگوی بروز مشکل به چه شکل است؟\n\n` +
                       `۱. رها شدن مقطعی و تصادفی وسط جوشکاری\n` +
                       `۲. کلاً عمل نمی‌کند و هیچ حرکتی ندارد\n` +
                       `۳. فقط بعد از چند ساعت کار مداوم و گرم شدن تابلو قطع می‌شود\n` +
                       `۴. حرکت بسیار کند یا با لرزش شدید دارد`,
                chips: [
                    'رها شدن تصادفی وسط کار',
                    'کلاً حرکت نمیکند',
                    'بعد از چند ساعت کار داغ میکنه',
                    'حرکت کند و با لرزش'
                ]
            };
        }

        // Step 2 -> 3: Actionable Technical Resolution
        if (diagState.step === 2) {
            const jName = diagState.data.jackName || 'جک انتخاب‌شده';
            resetDiagnostic();

            if (lower.includes('تصادفی') || lower.includes('رندوم') || lower.includes('وسط') || lower.includes('1') || lower.includes('۱')) {
                return {
                    reply: `🛠️ [گام ۳ از ۳: دستورالعمل رفع عیب رها شدن تصادفی ${jName}]\n\n` +
                           `۱. شل بودن پیچ ترمینال ۲۴ ولت در اثر لرزش ضربه جک‌ها:\n` +
                           `پیچ‌های ترمینال خروجی PLC مربوط به شیربرقی این جک را با پیچ‌گوشتی سفت نمایید.\n\n` +
                           `۲. افت لحظه‌ای فشار باد پنوماتیک:\n` +
                           `هنگام تحریک همزمان چند جک، اگر فشار مخزن باد به زیر ۵.۵ بار سقوط کند، فک‌ها رها می‌شوند. رگولاتور باد را روی ۶.۵ الی ۷.۵ بار تنظیم کنید.\n\n` +
                           `۳. افت ولتاژ تغذیه ۲۴ ولت تابلو (پاور سوئیچینگ): ولتاژ خروجی پاور را زیر بار چک کنید.`,
                    chips: ['تنظیم زمان‌بندی جک‌ها', 'مشخصات استاندارد', 'منوی اصلی']
                };
            }

            if (lower.includes('کلا') || lower.includes('هیچ') || lower.includes('قطع') || lower.includes('2') || lower.includes('۲')) {
                return {
                    reply: `🛠️ [گام ۳ از ۳: دستورالعمل عدم حرکت کامل ${jName}]\n\n` +
                           `۱. بررسی سوکت و بوبین شیربرقی: هنگام فرمان، روشن شدن LED روی سوکت شیربرقی را بررسی فرمایید.\n` +
                           `۲. کلید قارچی استپ اضطراری (E-Stop): مطمئن شوید شستی امرجنسی فشرده نشده باشد.\n` +
                           `۳. اینترلاک ایمنی: جک اتو تا زمانی که هر دو فک چپ و راست قفل نشوند، مجاز به حرکت به بالا نیست.\n` +
                           `۴. گیر مکانیکی یا ورود براده PVC به سیلندر پنوماتیک.`,
                    chips: ['عیب‌یابی المنت و SSR', 'تماس با پشتیبانی', 'منوی اصلی']
                };
            }

            if (lower.includes('داغ') || lower.includes('ساعت') || lower.includes('3') || lower.includes('۳')) {
                return {
                    reply: `🛠️ [گام ۳ از ۳: پروتکشن حرارتی درایورها (> 75°C)]\n\n` +
                           `۱. درایورهای ماسفت خروجی در صورت افزایش دمای تابلو برق به بالای ۷۵ درجه جهت جلوگیری از سوختن قطع می‌کنند.\n` +
                           `۲. فن‌های تهویه تابلو برق و فیلترهای ورودی هوا را سرویس نمایید.\n` +
                           `۳. مقاومت اهمی بوبین شیربرقی داغ‌شده را اندازه بگیرید (در صورت سوختگی نیمه‌کاره تعویض گردد).`,
                    chips: ['مشخصات استاندارد', 'پشتیبانی فنی هافلر']
                };
            }

            return {
                reply: `🛠️ [گام ۳ از ۳: رفع کندی حرکت ${jName}]\n\n` +
                       `۱. بررسی فلوکنترل‌های سر جک و باز کردن ۱ دور پیچ تنظیم هوا.\n` +
                       `۲. بررسی روغن‌زن پنوماتیک (FRL Unit) و سلامت روغن هیدرولیک سبک.\n` +
                       `۳. بررسی تایمر ورود و خروج در صفحه «زمان‌بندی جک‌ها» (پیشنهاد: ۱۵۰۰ الی ۲۵۰۰ میلی‌ثانیه).`,
                chips: ['تنظیم زمان‌بندی جک‌ها', 'منوی اصلی']
            };
        }
        return null;
    }

    // 5.2 Heater & SSR Diagnostic
    function handleHeaterDiagnostic(lower) {
        diagState.tree = 'HEATER_SSR';

        if (diagState.step === 0) {
            diagState.step = 1;
            return {
                reply: `🌡️ [گام ۱ از ۳: عیب‌یابی هوشمند اتو و رله SSR]\n\n` +
                       `کدام وضعیت دمایی در دستگاه رخ داده است؟\n\n` +
                       `۱. دمای اتو بی‌وقفه بالا می‌رود و قطع نمی‌شود (بالای ۲۷۵°C)\n` +
                       `۲. المنت کلاً سرد است و گرم نمی‌شود\n` +
                       `۳. عدد دمای روی نمایشگر مقدار عجیب ۱۰۲۴ یا منفی نشان می‌دهد\n` +
                       `۴. دما نوسان زیادی دارد (گاهی داغ و گاهی سرد می‌شود)`,
                chips: [
                    'دما بی وقفه بالا میره',
                    'المنت گرم نمیشه و سرده',
                    'نمایش دمای ۱۰۲۴ یا منفی',
                    'نوسان زیاد دما'
                ]
            };
        }

        if (diagState.step === 1) {
            resetDiagnostic();

            if (lower.includes('بالا') || lower.includes('قطع نمیشه') || lower.includes('داغ') || lower.includes('1') || lower.includes('۱')) {
                return {
                    reply: `🚨 [هشدار ایمنی - بالازدگی دمای اتو (Thermal Runaway)]:\n\n` +
                           `۱. اتصال کوتاه رله حالت جامد (SSR شورت شده است):\n` +
                           `اگر LED فرمان SSR خاموش است ولی المنت همچنان داغ می‌شود، ترایاک داخلی SSR سوخته و دائم وصل است. SSR باید فوراً تعویض گردد.\n\n` +
                           `۲. جدا شدن نوک ترموکوپل از داخل بدنه آلومینیومی اتو:\n` +
                           `پیچ نگه‌دارنده سنسور ترموکوپل روی اتو را بررسی کنید تا نوک سنسور کاملاً با فلز در تماس باشد.\n\n` +
                           `۳. در صورت سلامت SSR، ضریب PID را از منوی تنظیمات عمومی کاهش دهید (مثلاً به ۱۲).`,
                    chips: ['تنظیم دما', 'تنظیم ضریب PID', 'منوی اصلی']
                };
            }

            if (lower.includes('سرد') || lower.includes('گرم نمیشه') || lower.includes('2') || lower.includes('۲')) {
                return {
                    reply: `🛠️ [دستورالعمل عدم گرمایش المنت]:\n\n` +
                           `۱. فیوز مینیاتوری قدرت المنت و کلید حرارتی تابلو برق را چک کنید.\n` +
                           `۲. بررسی LED ورودی رله SSR: آیا هنگام افت دما LED سبز/قرمز روی SSR روشن می‌شود؟\n` +
                           `۳. اندازه‌گیری مقاومت المنت با اهم‌متر (مقاومت نرمال المنت جوش UPVC حدود ۲۵ الی ۴۵ اهم است).\n` +
                           `۴. بررسی سلامت خروجی SSR_1 روی برد اصلی.`,
                    chips: ['تنظیم دما', 'پشتیبانی فنی هافلر']
                };
            }

            if (lower.includes('1024') || lower.includes('۱۰۲۴') || lower.includes('منفی') || lower.includes('3') || lower.includes('۳')) {
                return {
                    reply: `🛠️ [تشخیص خطای سنسور ترموکوپل MAX6675]:\n\n` +
                           `• عدد ۱۰۲۴ به معنی قطعی مدار ترموکوپل (Open Circuit) است.\n` +
                           `۱. سوکت و سیم ترموکوپل متصل به ترمینال برد را چک کنید (سیم زرد مثبت، قرمز منفی).\n` +
                           `۲. از سالم بودن کابل شیلددار نسوز در محل مفصل چرخان اتو اطمینان حاصل نمایید.\n` +
                           `۳. سیستم ایمنی فریمور در این حالت به صورت خودکار رله SSR را قطع می‌کند تا از آتش‌سوزی جلوگیری شود.`,
                    chips: ['مشخصات استاندارد', 'منوی اصلی']
                };
            }

            return {
                reply: `🛠️ [رفع نوسان دمای المنت]:\n\n` +
                       `۱. ضریب هوشمند PID روی ۱۵ تنظیم شود.\n` +
                       `۲. اگر نوسان ادامه‌دار بود: در صورت بالازدگی ضریب را کم (مثلاً ۱۲) و در صورت افت دما ضریب را زیاد (مثلاً ۱۸) کنید.\n` +
                       `۳. کالیبراسیون سنسور را با دماسنج مرجع بررسی فرمایید.`,
                chips: ['تنظیم دما روی ۲۵۰', 'کالیبراسیون دما', 'منوی اصلی']
            };
        }
        return null;
    }

    // 5.3 Weld Quality & Corner Breakage Diagnostic
    function handleWeldQualityDiagnostic(lower) {
        diagState.tree = 'WELD_QUALITY';

        if (diagState.step === 0) {
            diagState.step = 1;
            return {
                reply: `🔬 [گام ۱ از ۳: آنالیز کیفیت و استحکام جوشکاری UPVC]\n\n` +
                       `کدام نقص در گوشه جوش خورده مشاهده می‌شود؟\n\n` +
                       `۱. جوش ضعیف است و در زاویه‌سنج با فشار کم می‌شکند\n` +
                       `۲. درز جوش سیاه، زرد یا سوخته می‌شود\n` +
                       `۳. انحراف زاویه (گوشه ۹۰ درجه دقیق نمی‌شود)\n` +
                       `۴. ضخامت پلیسه جوش نامتقارن یا کمتر از ۲ میلی‌متر است`,
                chips: [
                    'جوش ضعیفه و میشکنه',
                    'درز جوش سیاه یا زرد میشه',
                    'انحراف زاویه ۹۰ درجه',
                    'پلیسه نامتقارن'
                ]
            };
        }

        if (diagState.step === 1) {
            resetDiagnostic();

            if (lower.includes('میشکنه') || lower.includes('ضعیف') || lower.includes('1') || lower.includes('۱')) {
                pendingRecommendation = { temp: 250, heaterTime: 28, coolTime: 33, label: 'دمای ۲۵۰°C، ذوب ۲۸ ثانیه و خنک‌کاری ۳۳ ثانیه' };
                return {
                    reply: `🛠️ [گام ۳ از ۳: رفع شکستگی و ضعف جوشکاری (استاندارد RAL-GZ 716)]:\n\n` +
                           `۱. زمان خنک‌کاری کوتاه است: مولکول‌های PVC تا قبل از رسیدن به دمای ۴۰ درجه نباید از زیر فشار فک آزاد شوند (حداقل ۳۰ الی ۳۵ ثانیه خنک‌کاری).\n` +
                           `۲. دمای ناکافی اتو: دمای اتو را به ۲۴۸ الی ۲۵۲ درجه برسانید.\n` +
                           `۳. تنظیم فشار باد پنوماتیک روی ۶.۵ الی ۷.۵ بار.\n\n` +
                           `💡 پیشنهاد هوشمند هافلر: ${pendingRecommendation.label}.\nآیا مایلید این مقادیر مستقیماً روی برنامه فعال اعمال شوند؟`,
                    chips: ['اعمال مقادیر پیشنهادی', 'تنظیم دستی زمان خنک‌کاری', 'منوی اصلی']
                };
            }

            if (lower.includes('سیاه') || lower.includes('زرد') || lower.includes('سوخته') || lower.includes('2') || lower.includes('۲')) {
                pendingRecommendation = { temp: 246, heaterTime: 25, label: 'دمای ۲۴۶°C و ذوب ۲۵ ثانیه' };
                return {
                    reply: `🛠️ [گام ۳ از ۳: رفع سوختگی و زردی درز جوش]:\n\n` +
                           `۱. دمای اتو بیش از ۲۶۵ درجه است یا زمان ذوب بیش از ۳۵ ثانیه طول کشیده و PVC دپلیmerize شده است.\n` +
                           `۲. تعویض پارچه تفلون اتو: تفلون سوخته یا کثیف باعث انتقال حرارت نامنظم و دود کردن می‌شود.\n\n` +
                           `💡 تنظیم پیشنهادی هافلر: ${pendingRecommendation.label}.\nآیا اعمال شود؟`,
                    chips: ['اعمال مقادیر پیشنهادی', 'تنظیم دما روی ۲۴۵', 'منوی اصلی']
                };
            }

            if (lower.includes('زاویه') || lower.includes('کج') || lower.includes('3') || lower.includes('۳')) {
                return {
                    reply: `🛠️ [گام ۳ از ۳: تنظیم دقیق زاویه ۹۰ درجه]:\n\n` +
                           `۱. شابلون صفر (صفرزن): پیچ‌های تنظیم زاویه شابلون ۴۵ درجه را با گونیای دقیق ساعت‌زده و فیکس کنید.\n` +
                           `۲. بررسی براده PVC در لبه تکیه‌گاه فک‌ها: قبل از قرار دادن هر شاخه، سطح میز را با فرچه تمیز نمایید.\n` +
                           `۳. هم‌راستایی جک‌های گیره چپ و راست.`,
                    chips: ['مشخصات استاندارد', 'منوی اصلی']
                };
            }

            return {
                reply: `🛠️ [گام ۳ از ۳: تنظیم پلیسه سرتاسری جوش]:\n\n` +
                       `۱. جک ۰.۲ ضربه (شاکر) را بررسی کنید تا زمان نشست مذاب ۳ میلی‌متر تثبیت شود.\n` +
                       `۲. زمان ذوب را روی ۲۵ الی ۲۸ ثانیه تنظیم نمایید.`,
                chips: ['مشخصات استاندارد', 'منوی اصلی']
            };
        }
        return null;
    }

    // ── 6. Main NLP & Conversational Pipeline ─────────────────────────────────
    function processUserMessage(rawText) {
        if (!rawText || !rawText.trim()) return { reply: '', chips: [] };

        const text = rawText.trim();
        const ascii = toAsciiDigits(text);
        const lower = ascii.toLowerCase();
        const lang = getActiveLang();
        const dict = I18N[lang] || I18N.FA;
        const telem = getMachineTelemetry();

        // 1. Profanity Check
        if (containsProfanity(text)) {
            return {
                reply: dict.profanity_warning,
                chips: ['مشخصات استاندارد', 'منوی اصلی']
            };
        }

        // 2. Multi-turn Step-by-Step State Routing
        if (diagState.active || diagState.tree) {
            if (lower.includes('لغو') || lower.includes('خروج') || lower.includes('کنسل') || lower.includes('cancel')) {
                resetDiagnostic();
                return { reply: 'عیب‌یابی مرحله‌ای متوقف شد. چطور می‌توانم کمکتان کنم؟', chips: dict.chips };
            }

            if (diagState.tree === 'JACK_FAULT') {
                const res = handleJackDiagnostic(lower);
                if (res) return res;
            } else if (diagState.tree === 'HEATER_SSR') {
                const res = handleHeaterDiagnostic(lower);
                if (res) return res;
            } else if (diagState.tree === 'WELD_QUALITY') {
                const res = handleWeldQualityDiagnostic(lower);
                if (res) return res;
            }
        }

        // 3. Recommendation Acceptance ("اعمال", "اره", "بله", "yes")
        if (pendingRecommendation && (lower.includes('اعمال') || lower.includes('بله') || lower.includes('اره') || lower.includes('تایید') || lower.includes('yes') || lower.includes('ok'))) {
            const rec = pendingRecommendation;
            pendingRecommendation = null;
            let rep = '✅ تغییرات پیشنهادی با موفقیت روی دستگاه اعمال شد:\n\n';
            if (rec.temp) {
                Executor.setHeaterTemp(rec.temp);
                rep += `• دمای اتو: ${rec.temp}°C\n`;
            }
            if (rec.heaterTime) {
                Executor.setHeaterTime(rec.heaterTime);
                rep += `• زمان ذوب: ${rec.heaterTime} ثانیه\n`;
            }
            if (rec.coolTime) {
                Executor.setCoolingTime(rec.coolTime);
                rep += `• زمان پرس و خنک‌کاری: ${rec.coolTime} ثانیه\n`;
            }
            return { reply: rep, chips: ['مشخصات استاندارد', 'عیب‌یابی جک‌ها', 'منوی اصلی'] };
        }

        // 4. Intent: Diagnostic Trees Triggers
        if (lower.includes('جک') || lower.includes('ول میکنه') || lower.includes('رها') || lower.includes('سیلندر') || lower.includes('شیربرقی') || lower.includes('گیره') || lower.includes('میز')) {
            diagState.active = true;
            return handleJackDiagnostic(lower);
        }

        if (lower.includes('المنت') || lower.includes('اتو') || lower.includes('داغ نمیشه') || lower.includes('گرم نمیشه') || lower.includes('ssr') || lower.includes('ترموکوپل') || lower.includes('۱۰۲۴') || lower.includes('1024')) {
            diagState.active = true;
            return handleHeaterDiagnostic(lower);
        }

        if (lower.includes('شکستن') || lower.includes('جوش میشکنه') || lower.includes('کیفیت') || lower.includes('سوختن درز') || lower.includes('سیاه شد') || lower.includes('زرد شد') || lower.includes('استحکام')) {
            diagState.active = true;
            return handleWeldQualityDiagnostic(lower);
        }

        // 5.0 Profile Recipes (وین تک، ویستابست، هافلر، پلاسپن، سیندژ، ...)
        for (let key in KNOWLEDGE_BASE.recipes) {
            const recipe = KNOWLEDGE_BASE.recipes[key];
            const matched = recipe.aliases.some(alias => lower.includes(alias));
            if (matched) {
                const actionCardHtml = 
                    `<div class="chat-action-card">` +
                    `  <div class="card-title">⚙️ تنظیمات بهینه پروفیل ${recipe.name}</div>` +
                    `  <div class="card-desc">دمای اتو: ${recipe.temp}°C | زمان ذوب: ${recipe.melt} ثانیه | خنک‌کاری: ${recipe.cool} ثانیه | فشار باد: ${recipe.pressure}</div>` +
                    `  <button class="card-act-btn" onclick="window.HafflerChatbot.applyRecipe('${key}')">⚡ اعمال مستقیم روی دستگاه</button>` +
                    `</div>`;

                const reply = 
                    `📋 مشخصات فنی و تنظیمات بهینه جوشکاری برای «${recipe.name}»:\n\n` +
                    `• دمای هدف اتو: ${recipe.temp} درجه سانتی‌گراد\n` +
                    `• زمان ذوب (Melt Time): ${recipe.melt} ثانیه\n` +
                    `• زمان خنک‌کاری و پرس: ${recipe.cool} ثانیه\n` +
                    `• فشار باد پنوماتیک: ${recipe.pressure}\n` +
                    `• ضخامت پلیسه استاندارد: ${recipe.bead}\n\n` +
                    `💡 توضیحات: ${recipe.desc}\n\n` +
                    actionCardHtml;

                return {
                    reply: reply,
                    chips: [`اعمال ${recipe.name}`, 'مشخصات ویستابست', 'مشخصات وین تک', 'منوی اصلی']
                };
            }
        }

        if (lower.includes('پروفیل') || lower.includes('profile')) {
            let listText = '📋 لیست پروفیل‌های معتبر تعریف شده در دستیار هافلر:\n\n';
            KNOWLEDGE_BASE.PROFILE_RECIPES.forEach(p => {
                listText += `• ${p.name}: دمای ${p.temp}°C | ذوب ${p.melt}s | خنک‌کاری ${p.cool}s\n`;
            });
            listText += '\nبرای مشاهده جزییات هر کدام، نام پروفیل (مثلاً «وین تک» یا «ویستابست») را بگویید.';
            return {
                reply: listText,
                chips: ['مشخصات وین تک', 'مشخصات ویستابست', 'مشخصات هافلر', 'منوی اصلی']
            };
        }

        // 5. Direct Numerical Commands
        // 5.1 Temperature Tuning: "دما ۲۵۰", "temp 250", "دمای اتو ۲۴۸"
        if (lower.includes('دما') || lower.includes('temp') || lower.includes('حرارت')) {
            const match = lower.match(/\d+/);
            if (match) {
                const target = parseInt(match[0], 10);
                const res = Executor.setHeaterTemp(target);
                return { reply: res.msg, chips: ['زمان ذوب و خنک‌کاری', 'مشخصات استاندارد', 'منوی اصلی'] };
            } else {
                return {
                    reply: `🌡️ دمای فعلی اتو: ${telem.heaterTemp}°C (دمای هدف: ${telem.targetTemp}°C).\nبرای تغییر بگویید: «دما ۲۵۰» یا «دمای اتو ۲۴۸».`,
                    chips: ['دما ۲۴۵', 'دما ۲۵۰', 'دما ۲۵۵', 'کالیبراسیون دما']
                };
            }
        }

        // 5.2 Melting Time: "زمان ذوب ۲۵", "melt time 28", "گرم سازی ۳۰"
        if (lower.includes('ذوب') || lower.includes('melt') || lower.includes('گرم سازی') || lower.includes('زمان اتو')) {
            const match = lower.match(/\d+/);
            if (match) {
                const target = parseInt(match[0], 10);
                const res = Executor.setHeaterTime(target);
                return { reply: res.msg, chips: ['زمان خنک‌کاری ۳۲', 'تنظیم دما', 'منوی اصلی'] };
            } else {
                return {
                    reply: `⏱️ زمان ذوب فعلی: ${telem.meltTime} ثانیه.\nمحدوده مجاز: ۱۰ الی ۶۰ ثانیه (استاندارد: ۲۵ الی ۳۰ ثانیه).\nبرای تغییر بگویید: «زمان ذوب ۲۸».`,
                    chips: ['زمان ذوب ۲۵', 'زمان ذوب ۲۸', 'زمان ذوب ۳۰']
                };
            }
        }

        // 5.3 Cooling Time: "زمان خنک کاری ۳۲", "cool time 35", "خنک کاری ۳۰"
        if (lower.includes('خنک') || lower.includes('cool') || lower.includes('پرس')) {
            const match = lower.match(/\d+/);
            if (match) {
                const target = parseInt(match[0], 10);
                const res = Executor.setCoolingTime(target);
                return { reply: res.msg, chips: ['تنظیم دما', 'زمان ذوب ۲۸', 'منوی اصلی'] };
            } else {
                return {
                    reply: `⏱️ زمان خنک‌کاری فعلی: ${telem.coolTime} ثانیه.\nمحدوده مجاز: ۱۰ الی ۶۰ ثانیه (استاندارد: ۳۰ الی ۳۵ ثانیه).\nبرای تغییر بگویید: «خنک‌کاری ۳۲».`,
                    chips: ['خنک‌کاری ۳۰', 'خنک‌کاری ۳۲', 'خنک‌کاری ۳۵']
                };
            }
        }

        // 5.4 Program Select: "برنامه ۲", "program 3", "P1"
        if (lower.includes('برنامه') || lower.includes('program') || lower.match(/^p[1-5]$/)) {
            const match = lower.match(/[1-5]/);
            if (match) {
                const pId = parseInt(match[0], 10);
                const res = Executor.selectProgram(pId);
                return { reply: res.msg, chips: ['مشخصات برنامه فعال', 'تغییر دما', 'منوی اصلی'] };
            } else {
                return {
                    reply: `📋 برنامه فعال فعلی: P${telem.activeProgId}.\nبرای انتخاب بگویید: «برنامه ۱» الی «برنامه ۵».`,
                    chips: ['برنامه ۱', 'برنامه ۲', 'برنامه ۳', 'برنامه ۴', 'برنامه ۵']
                };
            }
        }

        // 6. General Knowledge & Guides
        if (lower.includes('استاندارد') || lower.includes('بهینه') || lower.includes('پارامتر') || lower.includes('optimal')) {
            return { reply: dict.optimal_settings, chips: ['تنظیم دما روی ۲۵۰', 'زمان ذوب ۲۸', 'خنک‌کاری ۳۲'] };
        }

        if (lower.includes('مراحل') || lower.includes('چطور جوش') || lower.includes('روش کار') || lower.includes('نحوه جوش')) {
            return { reply: dict.how_to_weld, chips: ['عیب‌یابی رها شدن جک', 'تنظیم دما', 'منوی اصلی'] };
        }

        if (lower.includes('گارانتی') || lower.includes('پشتیبانی') || lower.includes('تماس') || lower.includes('تلفن') || lower.includes('خدمات')) {
            return { reply: dict.warranty_info(telem.activationDate), chips: ['مشخصات استاندارد', 'منوی اصلی'] };
        }

        if (lower.includes('سلام') || lower.includes('درود') || lower.includes('hello') || lower.includes('hi') || lower.includes('شروع')) {
            return { reply: dict.greeting, chips: dict.chips };
        }

        // Default Fallback with Helpful Guidance
        return {
            reply: `من متوجه این پیام نشدم. شما می‌توانید دستوراتی مانند موارد زیر بنویسید یا روی گزینه‌های پیشنهادی کلیک کنید:\n\n` +
                   `• «دما ۲۵۰» برای تنظیم دمای اتو\n` +
                   `• «زمان ذوب ۲۸» برای تنظیم تایمر گرم‌سازی\n` +
                   `• «خنک‌کاری ۳۲» برای تنظیم زمان پرس\n` +
                   `• «عیب‌یابی جک» یا «عیب‌یابی المنت» برای بررسی گام‌به‌گام`,
            chips: dict.chips
        };
    }

    function applyRecipe(recipeId) {
        const recipe = KNOWLEDGE_BASE.recipes[recipeId];
        if (!recipe) return false;
        const telem = getMachineTelemetry();
        const pId = telem.activeProgId;
        Executor.sendWs({
            RQ: 5, // UPD_PROG
            ID: pId,
            h: recipe.melt,
            l: recipe.cool,
            t: recipe.temp
        });
        if (window.selectedProgram) {
            window.selectedProgram.t = recipe.temp;
            window.selectedProgram.h = recipe.melt;
            window.selectedProgram.l = recipe.cool;
        }
        var curT = document.getElementById('current_T');
        if (curT) curT.innerText = recipe.temp + '°';
        var curH = document.getElementById('current_H');
        if (curH) curH.innerText = recipe.melt;
        var curL = document.getElementById('current_L');
        if (curL) curL.innerText = recipe.cool;

        var confirmMsg = `✅ تنظیمات بهینه پروفیل «${recipe.name}» روی برنامه P${pId} با موفقیت اعمال شد.`;
        if (typeof window.appendMessage === 'function') {
            window.appendMessage(confirmMsg, false, false);
        }
        if (typeof window.speakBot === 'function') {
            window.speakBot(confirmMsg);
        }
        return true;
    }

    // Expose to window
    window.HafflerChatbot = {
        process: processUserMessage,
        I18N: I18N,
        Executor: Executor,
        KNOWLEDGE_BASE: KNOWLEDGE_BASE,
        applyRecipe: applyRecipe
    };

    console.log('[HafflerChatbot] Engine V2.5.0 initialized successfully');
})();
