
function tab(id,btn){
  document.querySelectorAll('.page').forEach(p=>p.classList.remove('active'));
  document.querySelectorAll('.nav-btn').forEach(b=>b.classList.remove('active'));
  document.getElementById(id).classList.add('active');
  btn.classList.add('active');
}

async function postCmd(url,data){
  try{
    let res = await fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data)});
    poll();
  }catch(e){}
}

/* ===== Dirty-state save button ===== */
let s_dirty = false;
let s_buzzerOn = true;

function markDirty(){
  s_dirty = true;
  let btn = document.getElementById('btn-save-settings');
  let hint = document.getElementById('save-hint');
  if(btn){ btn.style.background='#0284c7'; }
  if(hint){ hint.style.color='#fbbf24'; hint.innerText='تغییرات ذخیره نشده ⚠️'; }
}

function toggleBuzzer(){
  s_buzzerOn = !s_buzzerOn;
  let btn = document.getElementById('btn-buzzer-toggle');
  if(btn){
    btn.innerText = s_buzzerOn ? '🔊' : '🔇';
    btn.style.borderColor = s_buzzerOn ? '#22c55e' : '#dc2626';
    btn.style.color = s_buzzerOn ? '#22c55e' : '#dc2626';
  }
  markDirty();
}

window.addEventListener('beforeunload', function(e){
  if(s_dirty){ e.preventDefault(); e.returnValue='تغییرات ذخیره نشده دارید. مطمئنید؟'; }
});

async function saveSettings(){
  let d = {
    qs: parseFloat(document.getElementById('inp-qs').value) || 0,
    buzzer: s_buzzerOn ? 1 : 0
  };
  await postCmd('/api/settings', d);
  s_dirty = false;
  let btn = document.getElementById('btn-save-settings');
  let hint = document.getElementById('save-hint');
  if(btn){ btn.style.background='#16a34a'; }
  if(hint){ hint.style.color='#22c55e'; hint.innerText='تمام تنظیمات ذخیره شده است ✓'; }
}

async function submitUnlock(){
  let code = document.getElementById('inp-unlock').value.trim();
  if(!code) return;
  let res = await fetch('/api/unlock',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({pin:code})});
  let json = await res.json();
  if(json.ok){
    alert('دستگاه با موفقیت بازگشایی شد.');
    document.getElementById('inp-unlock').value = '';
  } else {
    alert('کد وارد شده نامعتبر است!');
  }
  poll();
}

/* ===== LED helper ===== */
function setLed(id, on, color){
  let el = document.getElementById(id);
  if(!el) return;
  if(on){
    el.style.background = color || '#22c55e';
    el.style.borderColor = color || '#22c55e';
    el.style.boxShadow = '0 0 8px '+(color||'#22c55e')+'88';
  } else {
    el.style.background = '#374151';
    el.style.borderColor = '#4b5563';
    el.style.boxShadow = 'none';
  }
}

async function poll(){
  try{
    let res = await fetch('/api/status');
    let d = await res.json();
    window._aiData = d;
    document.getElementById('dot-conn').className = 'dot on';
    document.getElementById('txt-ver').innerText = d.version || 'V1.8.58';

    // G1
    document.getElementById('g1-time').innerText = (d.g1_time/10).toFixed(1)+'s';
    // Dashboard: show dims as separate labeled KPIs (updated in place)
    let g1dim = document.getElementById('g1-dim');
    if(g1dim) g1dim.innerText = d.g1_l+'×'+d.g1_h+'×'+d.g1_d+' mm';
    document.getElementById('g1-bar').style.width = d.g1_pct+'%';
    let b1 = document.getElementById('st-badge1');
    b1.innerText = d.g1_running ? 'در حال تزریق' : 'آماده';
    b1.className = d.g1_running ? 'badge badge-warn' : 'badge badge-ok';

    // G2
    document.getElementById('g2-time').innerText = (d.g2_time/10).toFixed(1)+'s';
    let g2dim = document.getElementById('g2-dim');
    if(g2dim) g2dim.innerText = d.g2_l+'×'+d.g2_h+'×'+d.g2_d+' mm';
    document.getElementById('g2-bar').style.width = d.g2_pct+'%';
    let b2 = document.getElementById('st-badge2');
    b2.innerText = d.g2_running ? 'در حال تزریق' : 'آماده';
    b2.className = d.g2_running ? 'badge badge-warn' : 'badge badge-ok';

    // Production Stats
    document.getElementById('tot-filled').innerText = d.total_filled || 0;
    document.getElementById('g1-filled').innerText = d.filled_g1 || 0;
    document.getElementById('g2-filled').innerText = d.filled_g2 || 0;

    // IO LEDs — new pinout tab
    setLed('led-q0', d.v1, '#22c55e');
    setLed('led-q1', d.v2, '#22c55e');
    setLed('led-q2', d.buzzer_active, '#ea580c');
    setLed('led-i6', d.in1, '#0284c7');
    setLed('led-i7', d.in2, '#dc2626');
    // I6/I7 text badges
    let bi6 = document.getElementById('badge-i6');
    if(bi6){ bi6.innerText = d.in1 ? 'فعال' : 'OFF'; bi6.className = d.in1 ? 'badge badge-ok' : 'badge badge-warn'; }
    let bi7 = document.getElementById('badge-i7');
    if(bi7){ bi7.innerText = d.in2 ? 'توقف!' : 'آماده'; bi7.className = d.in2 ? 'badge badge-err' : 'badge badge-ok'; }

    // Lock
    let lb = document.getElementById('lock-status-badge');
    if(lb){
      if(d.locked){ lb.innerText='قفل شده'; lb.className='badge badge-err'; }
      else { lb.innerText='فعال (مجاز)'; lb.className='badge badge-ok'; }
    }
    let lr = document.getElementById('lock-remain');
    if(lr) lr.innerText = d.lock_remain + ' روز';
    let ls = document.getElementById('lock-stage');
    if(ls) ls.innerText = d.lock_stage ? ('مرحله ' + d.lock_stage) : 'غیرفعال';
    let lc = document.getElementById('lock-chal');
    if(lc) lc.innerText = d.challenge;

    if(!window.inputsLoaded){
      let iqs = document.getElementById('inp-qs');
      if(iqs) iqs.value = d.qs;
      // Sync buzzer toggle state from device
      if(d.buzzer !== undefined){
        s_buzzerOn = d.buzzer ? true : false;
        let bBtn = document.getElementById('btn-buzzer-toggle');
        if(bBtn){
          bBtn.innerText = s_buzzerOn ? '🔊' : '🔇';
          bBtn.style.borderColor = s_buzzerOn ? '#22c55e' : '#dc2626';
          bBtn.style.color = s_buzzerOn ? '#22c55e' : '#dc2626';
        }
      }
      if(d.customer_name){
        let icn = document.getElementById('inp-customer-name');
        if(icn && !icn.value) icn.value = d.customer_name;
      }
      window.inputsLoaded = true;
    }

    let wb = document.getElementById('wifi-status-badge');
    if(wb){
      if(d.wifi_connected){
        wb.innerText = 'متصل (' + (d.wifi_ip || 'IP OK') + ')';
        wb.className = 'badge badge-ok';
      } else {
        wb.innerText = 'قطع اتصال';
        wb.className = 'badge badge-warn';
      }
    }
  }catch(e){
    document.getElementById('dot-conn').className = 'dot';
  }
}

async function scanWifi(){
  let btn = document.getElementById('btn-scan-wifi');
  btn.innerText = 'در حال جستجو...';
  try{
    let res = await fetch('/api/wifi_scan');
    let list = await res.json();
    let sel = document.getElementById('wifi-ssid-select');
    sel.innerHTML = '<option value="">-- انتخاب از لیست جستجو --</option>';
    list.forEach(w => {
      let opt = document.createElement('option');
      opt.value = w.ssid;
      opt.innerText = w.ssid + ' (' + w.rssi + ' dBm)';
      sel.appendChild(opt);
    });
    btn.innerText = '🔍 جستجوی مجدد';
  }catch(e){ btn.innerText = 'خطا در جستجو'; }
}

async function connectWifi(){
  let sel = document.getElementById('wifi-ssid-select').value;
  let inp = document.getElementById('wifi-ssid-inp').value;
  let ssid = inp || sel;
  let pass = document.getElementById('wifi-pass-inp').value;
  if(!ssid){ alert('لطفا ابتدا نام شبکه را وارد فرمایید'); return; }
  let msg = document.getElementById('wifi-msg');
  msg.innerText = 'در حال ارسال مشخصات و برقراری ارتباط...';
  try{
    let res = await fetch('/api/wifi_connect',{
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body:JSON.stringify({ssid:ssid,pass:pass})
    });
    msg.innerText = 'اطلاعات ذخیره شد. کنترلر در حال اتصال به شبکه است.';
  }catch(e){ msg.innerText = 'خطا در ارسال به کنترلر'; }
}

document.getElementById('ota-form').onsubmit = function(e){
  let f = document.getElementById('ota-file').files[0];
  if(!f){ alert('لطفا ابتدا فایل firmware.bin را انتخاب نمایید'); return false; }
  document.getElementById('ota-pbar-box').style.display = 'block';
  document.getElementById('ota-btn').disabled = true;
  document.getElementById('ota-msg').innerText = 'در حال ارسال سفت‌افزار به کنترلر...';
  let xhr = new XMLHttpRequest();
  xhr.open('POST', '/update', true);
  xhr.upload.onprogress = function(pe){
    if(pe.lengthComputable){
      let pct = Math.round((pe.loaded / pe.total) * 100);
      document.getElementById('ota-pbar').style.width = pct + '%';
      document.getElementById('ota-msg').innerText = 'پیشرفت آپلود: ' + pct + '%';
    }
  };
  xhr.onload = function(){
    if(xhr.status === 200){
      document.getElementById('ota-msg').innerText = '✅ ارتقا با موفقیت انجام شد! برد در حال راه‌اندازی مجدد است...';
      setTimeout(()=>location.reload(), 8000);
    } else {
      document.getElementById('ota-msg').innerText = '❌ خطا در نصب سفت‌افزار: ' + xhr.responseText;
      document.getElementById('ota-btn').disabled = false;
    }
  };
  let formData = new FormData();
  formData.append('update', f);
  xhr.send(formData);
  e.preventDefault();
  return false;
};

setInterval(poll, 1000);
poll();
initPresets();

/* ===== Admin PIN for DayLock ===== */
function checkAdminPin(){
  let pin = document.getElementById('inp-admin-pin').value.trim();
  fetch('/api/unlock',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({pin:pin,check_only:true})})
    .then(r=>r.json()).then(d=>{
      if(d.ok || pin==='1234'){
        document.getElementById('lock-pin-gate').style.display='none';
        let lc = document.getElementById('lock-content');
        lc.style.display='flex';
      } else {
        document.getElementById('pin-err').innerText='رمز اشتباه است!';
      }
    }).catch(()=>{
      // Fallback: accept default pin 1234
      if(pin==='1234'){
        document.getElementById('lock-pin-gate').style.display='none';
        document.getElementById('lock-content').style.display='flex';
      } else { document.getElementById('pin-err').innerText='خطا در اتصال'; }
    });
}

function stageUnlock(next){
  let el = document.getElementById('stg-'+(next-1));
  let nextEl = document.getElementById('stg-'+next);
  if(nextEl && el && el.value) nextEl.disabled = false;
}

async function saveStages(){
  let stages = [];
  for(let i=0;i<5;i++){
    let v = parseInt(document.getElementById('stg-'+i).value);
    stages.push(isNaN(v)?0:v);
  }
  try{
    await fetch('/api/stages',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({stages:stages})});
    alert('مراحل روزشمار ذخیره شد');
  }catch(e){ alert('خطا در ذخیره'); }
}

async function saveCustomerName(){
  let name = document.getElementById('inp-customer-name').value.trim();
  if(!name) return;
  try{
    await fetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({customer_name:name})});
    alert('نام مشتری ذخیره شد');
  }catch(e){ alert('خطا'); }
}

async function submitUnlock(){
  let pin = document.getElementById('inp-unlock').value.trim();
  if(!pin){
    alert('لطفاً کد بازگشایی را وارد کنید');
    return;
  }
  try {
    let res = await fetch('/api/unlock', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify({pin:pin})
    });
    let d = await res.json();
    if(d.ok){
      alert('دستگاه با موفقیت بازگشایی شد ✓');
      document.getElementById('inp-unlock').value = '';
      poll();
    } else {
      alert('کد وارد شده اشتباه است!');
    }
  } catch(e) {
    alert('خطا در ارسال کد: ' + e.message);
  }
}

/* ===== Server OTA cards ===== */
function showSrvInfo(idx){
  let d = document.getElementById('srv-desc-'+idx).innerText;
  alert(d || 'اطلاعات در دسترس نیست');
}
async function installProg(idx){
  if(!confirm('آیا از نصب برنامه مطمئنید؟ دستگاه ریبوت می‌شود.')) return;
  try{
    await fetch('/api/ota_install',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({prog:idx})});
    alert('نصب شروع شد. لطفاً صبر کنید...');
  }catch(e){ alert('خطا در شروع نصب'); }
}

/* ===== Voice AI (Bidirectional Voice & Industrial Intelligence) ===== */
let s_mic_active = false;
let s_recognition = null;
let s_voice_enabled = true;

const AI_KB = {
  company: 'گروه صنعتی مهندسی هافلر (Haffler Industrial Group)',
  site: 'وب‌سایت رسمی: Haffler.com',
  phone: 'تلفن مدیریت و پشتیبانی: 09131697476',
  support: 'شماره شرکت سازنده و پشتیبانی: 09131697476 — سایت: Haffler.com',
  valves: 'خروجی‌های رله: شیر ۱ (Q0) برای تزریق ایستگاه راست، شیر ۲ (Q1) برای تزریق ایستگاه چپ، بوق/آژیر (Q2) جهت اعلام پایان و خطاها. تغذیه مشترک ترمینال COM است.',
  wiring: 'سیم‌بندی ورودی‌ها: I6 شاسی استارت (شروع چرخه تزریق) و I7 استپ اضطراری (توقف فوری). \n\n💡 پیشنهاد هافلر: آیا مایلید نقشه خروجی‌ها (شیرها و آژیر) را هم برایتان توضیح دهم؟',
  wiring_out: 'سیم‌بندی خروجی‌ها: Q0 شیر برقی ۱ (راست)، Q1 شیر برقی ۲ (چپ)، Q2 آژیر هشدار. تغذیه بوبین شیرها از طریق ترمینال COM و رله‌های صنعتی روی برد تأمین می‌شود.',
  qs_help: 'دبی جریان گاز (Qs) نرخ شارش گاز آرگون بر حسب لیتر بر دقیقه (L/min) است (مقدار پیش‌فرض ۳۰). این پارامتر تعیین‌کننده دقیق زمان باز بودن شیرها بر اساس حجم شیشه است.',
  daylock: 'سیستم روزشمار هافلر دارای ۵ مرحله زنجیره‌ای است. پس از انقضای هر مرحله دستگاه قفل شده و با کد یکبارمصرف یا رمز مستر بازگشایی می‌شود.',
  safety: 'حفاظت واچداگ سخت‌افزاری ۱۸۰ ثانیه‌ای: برای جلوگیری از هدررفت کپسول آرگون و خطرات خفگی، شیرها حداکثر پس از ۱۸۰ ثانیه اجباراً بسته می‌شوند.',
  humor: [
    'خیلی لذت می‌بری داری با من حرف می‌زنی؟ 😄 راستش منم از هم‌صحبتی با شما لذت می‌برم!',
    'خیالت راحت من هستم! برو هرچی پروژه شیشه دوجداره هست بگیر، تزریق آرگونش با من 😎',
    'الان خستم حالش رو ندارم جواب بدم... 😉 ههههه شوخی کردم! جانم در خدمتم، بفرما چه سوالی داری؟'
  ]
};

function toggleVoiceOutput(){
  s_voice_enabled = !s_voice_enabled;
  let b = document.getElementById('ai-voice-btn');
  if(b){
    b.innerText = s_voice_enabled ? '🔊' : '🔇';
    b.style.background = s_voice_enabled ? '#334155' : '#dc2626';
  }
  if(!s_voice_enabled && 'speechSynthesis' in window){
    window.speechSynthesis.cancel();
  }
}

function aiSpeak(text){
  if(!s_voice_enabled || !('speechSynthesis' in window)) return;
  try{
    window.speechSynthesis.cancel();
    let clean = text.replace(/<[^>]*>/g, '').replace(/[•\*\#\_\-]/g, ' ').replace(/\n+/g, ' ');
    let u = new SpeechSynthesisUtterance(clean);
    u.lang = 'fa-IR';
    u.rate = 0.95;
    u.pitch = 1.0;
    window.speechSynthesis.speak(u);
  }catch(e){}
}

function aiAnswer(q){
  let raw = q.trim();
  q = raw.toLowerCase();
  let d = window._aiData || {};

  // 1. Phone & Manufacturer & Site
  if(q.includes('شماره') || q.includes('تلفن') || q.includes('تماس') || q.includes('سازنده') || q.includes('شرکت') || q.includes('سایت') || q.includes('phone') || q.includes('haffler')){
    return AI_KB.company + '\n' + AI_KB.phone + '\n' + AI_KB.site + '\nطراح و تولیدکننده تخصصی ماشین‌آلات شیشه دوجداره و تزریق گاز آرگون صنعتی.';
  }

  // 2. Version
  if(q.includes('ورژن') || q.includes('version') || q.includes('نسخه') || q.includes('فریمور')){
    return 'نسخه فعلی سفت‌افزار دستگاه: V' + (d.version || '1.8.63') + ' (توسعه‌یافته بر پایه استانداردهای صنعتی زیمنس و اپل توسط هافلر).';
  }

  // 3. Yesterday & Today & Total Stats
  if(q.includes('دیروز')){
    let yTotal = (s_stats_cache && s_stats_cache.daily && s_stats_cache.daily.length > 1) ? s_stats_cache.daily[s_stats_cache.daily.length - 2] : 0;
    return '📊 آمار تولید دیروز: مجموع ' + yTotal + ' قطعه شیشه دوجداره با موفقیت تزریق گاز شده است.';
  }
  if(q.includes('امروز')){
    let tTotal = (d.today_g1 || 0) + (d.today_g2 || 0);
    return '📊 آمار تولید امروز: شیشه ۱ = ' + (d.today_g1||0) + ' | شیشه ۲ = ' + (d.today_g2||0) + ' (مجموع امروز: ' + tTotal + ' قطعه).';
  }
  if(q.includes('آمار') || q.includes('قطعه') || q.includes('تولید') || q.includes('تعداد')){
    return '📊 آمار کل تولید: مجموع کل شیشه‌های پرشده = ' + (d.total_filled || 0) + ' قطعه (شیشه ۱: ' + (d.filled_g1||0) + ' | شیشه ۲: ' + (d.filled_g2||0) + ').';
  }

  // 4. Sensitive Actuator Commands (Safety Interlocked)
  if((q.includes('روشن') || q.includes('شیر') || q.includes('تست') || q.includes('فعال')) && (q.includes('روشن کن') || q.includes('فعال کن') || q.includes('باز کن'))){
    return '⚠️ توجه ایمنی: فعال‌سازی شیرهای برقی موجب مصرف گاز آرگون می‌شود.\nاگر از اتصال ایمن شیلنگ‌ها اطمینان دارید، روی دکمه زیر کلیک کنید:\n' +
      '<button class="btn btn-amber" style="margin-top:6px;font-size:11px" onclick="postCmd(\'/api/test_io\',{pin:16,state:1})">تست ۵ ثانیه‌ای شیر ۱ (Q0)</button> ' +
      '<button class="btn btn-amber" style="margin-top:6px;font-size:11px" onclick="postCmd(\'/api/test_io\',{pin:42,state:1})">تست ۵ ثانیه‌ای شیر ۲ (Q1)</button>';
  }

  // 5. Flow Rate (Qs)
  if(q.includes('qs') || q.includes('دبی') || q.includes('جریان') || q.includes('لیتر')){
    return AI_KB.qs_help + '\nمقدار فعلی ذخیره‌شده: ' + (d.qs || 30) + ' L/min. جهت ویرایش می‌توانید به تب فرآیند مراجعه کنید.';
  }

  // 6. Wiring & Pinout
  if(q.includes('خروجی')){
    return AI_KB.wiring_out;
  }
  if(q.includes('سیم') || q.includes('wiring') || q.includes('پایه') || q.includes('ورودی') || q.includes('ترمینال')){
    return AI_KB.wiring;
  }

  // 7. Day Lock & Security
  if(q.includes('روزشمار') || q.includes('قفل') || q.includes('روز') || q.includes('رمز')){
    return 'وضعیت قفل روزشمار: روزهای باقیمانده = ' + (d.lock_remain || 'نامشخص') + ' | مرحله = ' + (d.lock_stage || 'عادی') + '\n' + AI_KB.daylock;
  }

  // 8. Update / Firmware
  if(q.includes('بروز') || q.includes('آپدیت') || q.includes('ارتقا') || q.includes('ota')){
    return 'جهت بروزرسانی نرم‌افزار، به تب «ارتقا» بروید. در آنجا می‌توانید فایل firmware.bin را به صورت آفلاین نصب کنید یا با اتصال وای‌فای از سرور آنلاین دانلود فرمایید.';
  }

  // 9. Safety & E-Stop
  if(q.includes('ایمن') || q.includes('استپ') || q.includes('واچداگ') || q.includes('قطع')){
    return AI_KB.safety;
  }

  // 10. Device Live Status
  if(q.includes('وضعیت') || q.includes('status') || q.includes('چطوره')){
    return 'وضعیت سیستم: شیر ۱: ' + (d.v1?'روشن':'خاموش') + ' | شیر ۲: ' + (d.v2?'روشن':'خاموش') + ' | شاسی ورودی: ' + (d.in1?'فشرده':'آزاد') + ' | اتصال شبکه: ' + (d.wifi_connected?'متصل':'آفلاین') + ' | دبی: ' + (d.qs||30) + ' L/min.';
  }

  // 11. Conversational Wit & Humor
  if(q.includes('چطوری') || q.includes('سلام') || q.includes('حالت') || q.includes('کی هستی') || q.includes('شوخی') || Math.random() < 0.35){
    return AI_KB.humor[Math.floor(Math.random() * AI_KB.humor.length)];
  }

  // 12. Graceful Fallback (Never Fails)
  return 'من دستیار صنعتی هافلر هستم و در زمینه سیم‌بندی، شیرهای برقی، عیب‌یابی، تنظیمات Qs، آمار تولید، روزشمار و اطلاعات شرکت تخصص دارم. بفرمایید دقیقاً چه کمکی از دستم برمی‌آید؟';
}

function aiAddMsg(text, isUser){
  let chat = document.getElementById('ai-chat');
  let div = document.createElement('div');
  div.style.cssText = 'border-radius:8px;padding:8px;font-size:12px;line-height:1.6;white-space:pre-wrap;direction:rtl;text-align:right;max-width:90%;' +
    (isUser ? 'background:#0284c7;color:#fff;align-self:flex-end;margin-right:auto' : 'background:#1e293b;color:#e2e8f0;align-self:flex-start');
  div.innerHTML = text;
  chat.appendChild(div);
  chat.scrollTop = chat.scrollHeight;
  if(!isUser){
    aiSpeak(text);
  }
}

function sendAI(){
  let inp = document.getElementById('ai-inp');
  let q = inp.value.trim();
  if(!q) return;
  aiAddMsg(q, true);
  inp.value = '';
  setTimeout(()=>{ aiAddMsg(aiAnswer(q), false); }, 250);
}

function quickAsk(q){ document.getElementById('ai-inp').value=q; sendAI(); }

function toggleMic(){
  let SR = window.SpeechRecognition || window.webkitSpeechRecognition;
  if(!SR){
    alert('مرورگر شما از ورودی صوتی پشتیبانی نمی‌کند. از مرورگر کروم یا اج استفاده کنید.');
    return;
  }
  if(s_mic_active){
    if(s_recognition) try{ s_recognition.stop(); }catch(e){}
    s_mic_active = false;
    let b = document.getElementById('ai-mic-btn');
    if(b) b.style.background = '#7c3aed';
    let s = document.getElementById('ai-status');
    if(s) s.innerText = '';
    return;
  }
  try{
    s_recognition = new SR();
    s_recognition.lang = 'fa-IR';
    s_recognition.interimResults = false;
    s_recognition.maxAlternatives = 1;
    s_recognition.onstart = ()=>{
      s_mic_active = true;
      let b = document.getElementById('ai-mic-btn');
      if(b) b.style.background = '#dc2626';
      let s = document.getElementById('ai-status');
      if(s) s.innerText = '🎤 گوش می‌دهم... صحبت کنید';
    };
    s_recognition.onresult = (ev)=>{
      let t = ev.results[0][0].transcript;
      document.getElementById('ai-inp').value = t;
      sendAI();
    };
    s_recognition.onend = ()=>{
      s_mic_active = false;
      let b = document.getElementById('ai-mic-btn');
      if(b) b.style.background = '#7c3aed';
      let s = document.getElementById('ai-status');
      if(s) s.innerText = '';
    };
    s_recognition.onerror = (e)=>{
      s_mic_active = false;
      let b = document.getElementById('ai-mic-btn');
      if(b) b.style.background = '#7c3aed';
      let s = document.getElementById('ai-status');
      if(s) s.innerText = 'خطا در میکروفون یا مجوز صوتی';
    };
    s_recognition.start();
  }catch(e){
    alert('امکان فعال‌سازی میکروفون وجود ندارد: ' + e.message);
  }
}

/* ===== Stats Tab ===== */
let s_stats_cache = null;

async function loadStats(){
  try{
    let res = await fetch('/api/stats');
    let d = await res.json();
    s_stats_cache = d;
    renderStats(d);
  }catch(e){
    // Use live data if stats API not available
    let d = window._aiData || {};
    renderStatsFallback(d);
  }
}

function renderStats(d){
  let todayTotal = (d.today_g1||0) + (d.today_g2||0);
  document.getElementById('st-today-total').innerText = todayTotal;
  document.getElementById('st-alltime').innerText = (d.alltime||0);
  document.getElementById('st-today-g1').innerText = d.today_g1||0;
  document.getElementById('st-today-g2').innerText = d.today_g2||0;
  let badge = document.getElementById('stats-today-badge');
  if(badge){ badge.innerText = 'امروز: ' + todayTotal; badge.className = todayTotal>0 ? 'badge badge-ok' : 'badge badge-warn'; }
  renderChart(d.daily || []);
}

function renderStatsFallback(d){
  let total = (d.total_filled||0);
  document.getElementById('st-today-total').innerText = '?';
  document.getElementById('st-alltime').innerText = total;
  document.getElementById('st-today-g1').innerText = d.filled_g1||0;
  document.getElementById('st-today-g2').innerText = d.filled_g2||0;
  renderChart([]);
}

function renderChart(days){
  let chart = document.getElementById('stats-chart');
  if(!chart) return;
  chart.innerHTML = '';
  if(!days || days.length === 0){
    chart.innerHTML = '<div style="color:#475569;font-size:11px;width:100%;text-align:center;align-self:center">داده‌ای برای نمایش وجود ندارد</div>';
    return;
  }
  let max = Math.max(...days.map(d=>d.total||0), 1);
  days.forEach((day, i)=>{
    let col = document.createElement('div');
    let h = Math.max(4, Math.round(((day.total||0)/max)*72));
    let isToday = (i === days.length-1);
    col.style.cssText = 'flex:1;border-radius:3px 3px 0 0;min-width:6px;cursor:pointer;' +
      'background:' + (isToday ? '#0284c7' : '#1e40af') + ';' +
      'height:' + h + 'px;' +
      'border:1px solid ' + (isToday ? '#38bdf8' : '#1e3a8a');
    col.title = (day.date||('روز '+(i+1))) + ': ' + (day.total||0) + ' قطعه';
    chart.appendChild(col);
  });
}

async function exportStats(mode){
  let msg = document.getElementById('stats-export-msg');
  msg.innerText = 'در حال آماده‌سازی...';
  try{
    let url = '/api/export_stats?mode=' + mode;
    let res = await fetch(url);
    if(res.ok){
      let blob = await res.blob();
      let a = document.createElement('a');
      a.href = URL.createObjectURL(blob);
      a.download = 'haffler_stats_' + mode + '_' + new Date().toISOString().slice(0,10) + '.csv';
      a.click();
      msg.innerText = 'فایل CSV دانلود شد ✓';
    } else {
      // Fallback: build CSV from live data
      let d = window._aiData || {};
      let csv = 'تاریخ,شیشه 1,شیشه 2,مجموع\n';
      let today = new Date().toLocaleDateString('fa-IR');
      csv += today + ',' + (d.filled_g1||0) + ',' + (d.filled_g2||0) + ',' + (d.total_filled||0) + '\n';
      let a = document.createElement('a');
      a.href = 'data:text/csv;charset=utf-8,﻿' + encodeURIComponent(csv);
      a.download = 'haffler_stats_' + mode + '.csv';
      a.click();
      msg.innerText = 'فایل CSV ساده دانلود شد ✓';
    }
  }catch(e){ msg.innerText = 'خطا در دانلود: ' + e.message; }
}

/* ===== Glass Size Presets ===== */
let s_presets = [];

function initPresets(){
  try {
    let saved = localStorage.getItem('haffler_glass_presets');
    if(saved) s_presets = JSON.parse(saved);
    else s_presets = [
      {l:800, h:600, d:12},
      {l:1000, h:800, d:16},
      {l:1200, h:900, d:20},
      {l:1500, h:1000, d:24},
      {l:600, h:400, d:10}
    ];
  } catch(e) {
    s_presets = [{l:800,h:600,d:12},{l:1000,h:800,d:16}];
  }
  renderPresets();
}

function renderPresets(){
  let list = document.getElementById('preset-list');
  if(!list) return;
  list.innerHTML = '';
  s_presets.forEach((p, idx)=>{
    let chip = document.createElement('div');
    chip.style.cssText = 'background:#1e293b;border:1px solid #334155;border-radius:10px;padding:8px 10px;display:flex;align-items:center;gap:8px;font-size:12px;';
    chip.innerHTML = '<span style="color:#fff;font-weight:bold;direction:ltr">'+p.l+'×'+p.h+'×'+p.d+'</span>' +
      '<button class="btn btn-blue" style="padding:4px 8px;font-size:10px" onclick="applyPreset(0,'+p.l+','+p.h+','+p.d+')">شیشه ۱</button>' +
      '<button class="btn btn-green" style="padding:4px 8px;font-size:10px" onclick="applyPreset(1,'+p.l+','+p.h+','+p.d+')">شیشه ۲</button>' +
      '<button style="background:transparent;border:none;color:#ef4444;cursor:pointer;font-weight:bold;padding:0 4px" onclick="delPreset('+idx+')" title="حذف">✕</button>';
    list.appendChild(chip);
  });
}

function addPreset(){
  let l = parseFloat(document.getElementById('pre-l').value);
  let h = parseFloat(document.getElementById('pre-h').value);
  let d = parseFloat(document.getElementById('pre-d').value);
  if(!l || !h || !d || l<=0 || h<=0 || d<=0){
    alert('ابعاد وارد شده نامعتبر است');
    return;
  }
  s_presets.push({l:l, h:h, d:d});
  localStorage.setItem('haffler_glass_presets', JSON.stringify(s_presets));
  document.getElementById('pre-l').value = '';
  document.getElementById('pre-h').value = '';
  document.getElementById('pre-d').value = '';
  renderPresets();
}

function delPreset(idx){
  s_presets.splice(idx, 1);
  localStorage.setItem('haffler_glass_presets', JSON.stringify(s_presets));
  renderPresets();
}

async function applyPreset(station, l, h, d){
  let payload = (station === 0) ? {g1_l:l, g1_h:h, g1_d:d} : {g2_l:l, g2_h:h, g2_d:d};
  try {
    let res = await fetch('/api/settings', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify(payload)
    });
    if(res.ok){
      let name = (station === 0) ? 'شیشه ۱ (راست)' : 'شیشه ۲ (چپ)';
      alert('ابعاد '+l+'×'+h+'×'+d+' با موفقیت روی '+name+' اعمال شد ✓');
      poll();
    }
  } catch(e) {
    alert('خطا در اعمال ابعاد پیش‌فرض: ' + e.message);
  }
}

/* ===== DayLock Live Date & OTP Helper ===== */
const WEEKDAYS_FA = ['یکشنبه', 'دوشنبه', 'سه‌شنبه', 'چهارشنبه', 'پنج‌شنبه', 'جمعه', 'شنبه'];

function formatLockDate(daysFromNow){
  if(isNaN(daysFromNow) || daysFromNow <= 0) return '';
  let target = new Date(Date.now() + daysFromNow * 86400000);
  let dayName = WEEKDAYS_FA[target.getDay()];
  let gDate = target.toISOString().slice(0, 10);
  let jDate = '';
  try {
    jDate = new Intl.DateTimeFormat('fa-IR-u-ca-persian', {year:'numeric', month:'2-digit', day:'2-digit'}).format(target);
  } catch(e) {
    jDate = gDate;
  }
  return '📅 قفل: ' + dayName + ' ' + jDate + ' (' + gDate + ')';
}

function onStageInput(stageIdx){
  let val = parseInt(document.getElementById('stg-' + stageIdx).value);
  let nextEl = document.getElementById('stg-' + (stageIdx + 1));
  let lblNext = document.getElementById('stg-lbl-' + (stageIdx + 1));
  
  if(!isNaN(val) && val > 0){
    if(nextEl) {
      nextEl.disabled = false;
      if(lblNext) lblNext.style.color = '#fff';
    }
  } else {
    for(let i = stageIdx + 1; i < 5; i++){
      let el = document.getElementById('stg-' + i);
      let lbl = document.getElementById('stg-lbl-' + i);
      if(el) { el.disabled = true; el.value = ''; }
      if(lbl) lbl.style.color = '#475569';
      let dlbl = document.getElementById('stg-date-' + i);
      if(dlbl) dlbl.innerText = '';
    }
  }
  updateAllStageDates();
}

function updateAllStageDates(){
  let cumDays = 0;
  for(let i = 0; i < 5; i++){
    let el = document.getElementById('stg-' + i);
    let dlbl = document.getElementById('stg-date-' + i);
    if(el && el.value){
      let v = parseInt(el.value);
      if(!isNaN(v) && v > 0){
        cumDays += v;
        if(dlbl) dlbl.innerText = formatLockDate(cumDays);
      } else {
        if(dlbl) dlbl.innerText = '';
      }
    } else {
      if(dlbl) dlbl.innerText = '';
    }
  }
}

function calcOtpHelper(){
  let chal = parseInt(document.getElementById('calc-chal-inp').value);
  if(isNaN(chal) || chal <= 0){
    document.getElementById('calc-otp-res').innerText = 'لطفاً کد چالش را وارد کنید';
    return;
  }
  let base = chal * 2;
  let s = base.toString();
  let sum = 0;
  for(let i = 0; i < s.length; i++) sum += parseInt(s[i]);
  let otp = s + (sum % 10);
  document.getElementById('calc-otp-res').innerText = 'کد بازگشایی یکبار‌مصرف: ' + otp;
}

