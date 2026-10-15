# 🎨 مستند جامع الگوهای سه‌بعدی و مدرن اینستاگرام (Instagram 3D & UI/UX Master Cookbook)
**Reusable Architecture, Mathematical Formulas & Zero-Dependency Code Snippets**

این سند مرجع رسمی و دائمی کلیه ایده‌ها، الگوهای تعاملی سه‌بعدی (3D Interactions) و تکنیک‌های رابط کاربری است که از تحلیل ویدیوهای برگزیده اینستاگرام استخراج، مهندسی معکوس و استانداردسازی شده است. هر ایجنت، مهندس فرانت‌اند یا سیستم هوش مصنوعی می‌تواند کدهای این مستند را مستقیماً کپی کرده و در هر پروژه وب، اپلیکیشن داشبورد، لندینگ پیج یا نرم‌افزار صنعتی پیاده‌سازی کند.

---

## 📑 فهرست الگوهای مستند شده

1. **الگوی اول (ریل `DbfG6fuPE9y`):** داک ناوبری مایع منیسکوس (Meniscus Fluid Liquid Dock)
2. **الگوی دوم (ریل `Da8LPoqoLY5`):** نمای انفجاری لایه‌ای سه‌بعدی (3D Exploded Layer Disassembly View)
3. **الگوی سوم (ریل `DZsZKSPMa2A`):** پارالاکس عمق‌دار وابسته به اسکرول (Scroll-Driven 3D Parallax)
4. **الگوی چهارم (ریل `DZkw6LWtG8k`):** گرید بنتو با حاشیه نورانی گردان (Bento Grid & Conic Shimmer)
5. **الگوی پنجم (ریل `DbSI4_ZRm7U`):** استیج ۳بعدی تعاملی با نورافکن پویا (Interactive 3D Stage & Lighting)
6. **الگوی ششم (ریل `DanzJmPowLT`):** زبان طراحی لوکس تیره اپل و اوواردز (Dark Luxury Industrial Rules)
7. **الگوی هفتم (ریل `DXpKhhWCP2M`):** فیزیک میراشونده شناور (Spring Lerp Damping & Micro-Interactions)
8. **الگوی هشتم (ریل `DdRUnfss4SJ`):** استودیو دوقلوی سه‌بعدی و تعقیبگر ماوس MotionSites (3D Cursor Follow & Specular Glare)

---

## 1. الگوی اول: داک مایع منیسکوس (Meniscus Liquid Dock)
* **منبع اینستاگرام:** https://www.instagram.com/reel/DbfG6fuPE9y/
* **مفهوم:** منوی شناور پایین صفحه در موبایل که نوار دور داک به نرمی دور آیکون فعال کش آمده و گوی شناور (Active Bead) به حالت ارگانیک روی آیکون انتخاب‌شده حرکت می‌کند.

### فرمول ریاضی انحنای مماس (Tangency Calculus)
برای اینکه لبه مایع بدون شکستگی و به شکل پیوسته (C1/C2 Continuous) به دایره آیکون متصل شود:
$$\\text{reach}(s, r_b, d) = \\sqrt{(s + r_b)^2 - (s - d)^2}$$

### پیاده‌سازی کلاینت بدون نیاز به پکیج (Zero-Dependency)
```javascript
function updateMeniscusDockPosition(index, dockId, beadId, pathId) {
  const dock = document.getElementById(dockId);
  const bead = document.getElementById(beadId);
  const path = document.getElementById(pathId);
  if (!dock || !bead || !path) return;

  const buttons = dock.querySelectorAll('.dock-tab');
  if (!buttons[index]) return;

  const targetBtn = buttons[index];
  const w = dock.offsetWidth;
  const topY = 16;
  const r = 24;      // شعاع فرو رفتگی منیسکوس
  const depth = 11;  // عمق کشش مایع
  const xPos = targetBtn.offsetLeft + targetBtn.offsetWidth / 2;
  const x = Math.max(r + 10, Math.min(w - r - 10, xPos));

  // ترسیم منحنی بزیر درجه سه در SVG
  const d = `M 0,${topY + 14} 
             Q 0,${topY} 14,${topY} 
             L ${x - r - 12},${topY} 
             C ${x - r},${topY} ${x - r * 0.7},${topY + depth} ${x},${topY + depth} 
             C ${x + r * 0.7},${topY + depth} ${x + r},${topY} ${x + r + 12},${topY} 
             L ${w - 14},${topY} 
             Q ${w},${topY} ${w},${topY + 14}`;

  path.setAttribute('d', d);
  bead.style.transform = `translateX(${x - 22}px)`;
}
```

---

## 2. الگوی دوم: نمای انفجاری لایه‌ای سه‌بعدی (3D Exploded View)
* **منبع اینستاگرام:** https://www.instagram.com/reel/Da8LPoqoLY5/
* **مفهوم:** نمایش کالبدشکافی محصول یا ماشین صنعتی در محور عمق (Z-Axis) با قابلیت کنترل درصد انفجار و چرخش زاویه دید.

### فرمول جداسازی در فضای سه بعدی
$$\\Delta Z_i = \\text{Gap} \\times (i - \\text{CenterIndex})$$
$$\\Delta Y_i = -\\text{Gap} \\times \\text{TiltFactor} \\times (i - \\text{CenterIndex})$$

### پیاده‌سازی کامل با CSS 3D Transforms (60 FPS با شتاب‌دهنده GPU)
```css
.stage-3d-viewport {
  perspective: 1400px;
  perspective-origin: 50% 45%;
}
.assembly-3d-object {
  transform-style: preserve-3d;
  transition: transform 0.15s ease-out;
}
.hardware-layer {
  position: absolute;
  top: 50%;
  left: 50%;
  transform-style: preserve-3d;
  transition: transform 0.4s cubic-bezier(0.2, 0.8, 0.2, 1);
  backdrop-filter: blur(14px);
}
```

```javascript
function applyExplosion(val) {
  const gap = val * 1.8; // ضریب فاصله پیکسلی
  document.getElementById('layer1').style.transform = `translateZ(${gap * 2}px) translateY(${-gap * 0.3}px)`;
  document.getElementById('layer2').style.transform = `translateZ(${gap * 1}px) translateY(${-gap * 0.15}px)`;
  document.getElementById('layer3').style.transform = `translateZ(0px)`;
  document.getElementById('layer4').style.transform = `translateZ(${-gap * 1}px) translateY(${gap * 0.15}px)`;
  document.getElementById('layer5').style.transform = `translateZ(${-gap * 2}px) translateY(${gap * 0.3}px)`;
}
```

---

## 3. الگوی سوم: استودیو سه‌بعدی و تعقیبگر ماوس MotionSites (Cursor Follow 3D Card)
* **منبع اینستاگرام:** https://www.instagram.com/reel/DdRUnfss4SJ/
* **مفهوم:** الهام‌گرفته از motionsites.ai؛ کارت‌ها و آبجکت‌ها هنگام حرکت ماوس یا لمس موبایل با شیب سه‌بعدی روان زاویه می‌گیرند و انعکاس نور خیره‌کننده (Specular Glare) روی بدنه آنها حرکت می‌کند. لایه‌های داخلی کارت در محور Z عمق می‌گیرند.

### پیاده‌سازی موتور سبک (Lightweight MotionSites 3D Engine)
```javascript
function init3DMotionCards() {
  const cards = document.querySelectorAll('[data-3d-tilt]');
  cards.forEach(card => {
    let bounds = null;
    let isHovered = false;
    let currentX = 0, currentY = 0;
    let targetX = 0, targetY = 0;
    let rafId = null;

    // لایه انعکاس نور پویا (Specular Glare Layer)
    let sheen = card.querySelector('.sheen-3d-layer');
    if (!sheen) {
      sheen = document.createElement('div');
      sheen.className = 'sheen-3d-layer pointer-events-none absolute inset-0 rounded-[inherit] opacity-0 transition-opacity duration-300';
      card.style.position = 'relative';
      card.style.transformStyle = 'preserve-3d';
      card.appendChild(sheen);
    }

    function updateLoop() {
      if (!isHovered && Math.abs(currentX) < 0.005 && Math.abs(currentY) < 0.005) {
        card.style.transform = 'perspective(1100px) rotateX(0deg) rotateY(0deg) translateZ(0px)';
        sheen.style.opacity = '0';
        rafId = null;
        return;
      }
      // فیزیک اینرسی با میرایی نرم (Lerp Damping)
      currentX += (targetX - currentX) * 0.14;
      currentY += (targetY - currentY) * 0.14;
      
      const maxTilt = parseFloat(card.dataset.tiltMax || '8');
      const rotX = -currentY * maxTilt;
      const rotY = currentX * maxTilt;
      
      card.style.transform = `perspective(1100px) rotateX(${rotX.toFixed(2)}deg) rotateY(${rotY.toFixed(2)}deg) translateZ(6px)`;
      
      // موقعیت‌سنجی کانون نور بازتابی
      const glareX = ((currentX + 1) / 2) * 100;
      const glareY = ((currentY + 1) / 2) * 100;
      sheen.style.background = `radial-gradient(circle at ${glareX.toFixed(1)}% ${glareY.toFixed(1)}%, rgba(6,182,212,0.2) 0%, transparent 60%)`;
      sheen.style.opacity = '1';

      rafId = requestAnimationFrame(updateLoop);
    }

    card.addEventListener('mouseenter', () => {
      bounds = card.getBoundingClientRect();
      isHovered = true;
      if (!rafId) rafId = requestAnimationFrame(updateLoop);
    });

    card.addEventListener('mousemove', (e) => {
      if (!bounds) bounds = card.getBoundingClientRect();
      const x = (e.clientX - bounds.left) / bounds.width;
      const y = (e.clientY - bounds.top) / bounds.height;
      targetX = (Math.max(0, Math.min(1, x)) - 0.5) * 2;
      targetY = (Math.max(0, Math.min(1, y)) - 0.5) * 2;
      if (!rafId) rafId = requestAnimationFrame(updateLoop);
    });

    card.addEventListener('mouseleave', () => {
      isHovered = false;
      targetX = 0;
      targetY = 0;
    });

    // پشتیبانی لمسی برای گوشی‌های موبایل (Touch Tilt)
    card.addEventListener('touchmove', (e) => {
      if (e.touches.length === 1) {
        if (!bounds) bounds = card.getBoundingClientRect();
        const touch = e.touches[0];
        const x = (touch.clientX - bounds.left) / bounds.width;
        const y = (touch.clientY - bounds.top) / bounds.height;
        targetX = (Math.max(0, Math.min(1, x)) - 0.5) * 2;
        targetY = (Math.max(0, Math.min(1, y)) - 0.5) * 2;
        isHovered = true;
        if (!rafId) rafId = requestAnimationFrame(updateLoop);
      }
    }, { passive: true });

    card.addEventListener('touchend', () => {
      isHovered = false;
      targetX = 0;
      targetY = 0;
    });
  });
}
```

---

## 4. الگوی چهارم: چرخش آزاد ۳۶۰ درجه با درگ ماوس و لمس موبایل (360° Orbit Assembly)
* **مفهوم:** کاربر می‌تواند ماوس یا انگشت خود را روی هر دستگاه یا قطعه صنعتی بکشد تا در تمام ابعاد سه‌بعدی به نرمی دوران کند.

```javascript
let rotX = -15, rotY = 25;
let isDragging = false;
let startX = 0, startY = 0;

function setup3DOrbit(stageId, assemblyId) {
  const stage = document.getElementById(stageId);
  const assembly = document.getElementById(assemblyId);
  if (!stage || !assembly) return;

  function applyTransform() {
    assembly.style.transform = `perspective(1000px) rotateX(${rotX.toFixed(2)}deg) rotateY(${rotY.toFixed(2)}deg)`;
  }

  stage.addEventListener('mousedown', (e) => {
    isDragging = true;
    startX = e.clientX;
    startY = e.clientY;
  });

  window.addEventListener('mouseup', () => isDragging = false);

  window.addEventListener('mousemove', (e) => {
    if (!isDragging) return;
    const dx = e.clientX - startX;
    const dy = e.clientY - startY;
    rotY += dx * 0.5;
    rotX -= dy * 0.5;
    startX = e.clientX;
    startY = e.clientY;
    applyTransform();
  });

  // تاچ چند لمسی موبایل
  stage.addEventListener('touchstart', (e) => {
    if (e.touches.length === 1) {
      isDragging = true;
      startX = e.touches[0].clientX;
      startY = e.touches[0].clientY;
    }
  }, { passive: true });

  stage.addEventListener('touchmove', (e) => {
    if (isDragging && e.touches.length === 1) {
      const dx = e.touches[0].clientX - startX;
      const dy = e.touches[0].clientY - startY;
      rotY += dx * 0.5;
      rotX -= dy * 0.5;
      startX = e.touches[0].clientX;
      startY = e.touches[0].clientY;
      applyTransform();
    }
  }, { passive: true });

  stage.addEventListener('touchend', () => isDragging = false);
}
```

---

## 5. الگوی پنجم: حاشیه نئونی گردان مخروطی (Conic Shimmer Border)
* **منبع اینستاگرام:** https://www.instagram.com/reel/DZkw6LWtG8k/
* **کد CSS:**
```css
@keyframes rotate-conic {
  from { transform: rotate(0deg); }
  to { transform: rotate(360deg); }
}

.conic-shimmer-card {
  position: relative;
  overflow: hidden;
  border-radius: 24px;
  background: #0f172a;
}

.conic-shimmer-card::before {
  content: '';
  position: absolute;
  top: -50%;
  left: -50%;
  width: 200%;
  height: 200%;
  background: conic-gradient(from 0deg, transparent 0deg, #06b6d4 90deg, transparent 180deg);
  animation: rotate-conic 4s linear infinite;
  pointer-events: none;
}

.conic-shimmer-card::after {
  content: '';
  position: absolute;
  inset: 1.5px;
  border-radius: 22.5px;
  background: rgba(15, 23, 42, 0.95);
  backdrop-filter: blur(12px);
  pointer-events: none;
}
```

---

## 6. الگوی ششم: استانداردهای طراحی لوکس صنعتی (@madadi_ai)
* **منبع اینستاگرام:** https://www.instagram.com/reel/DanzJmPowLT/
* **قوانین کلیدی:**
  1. **عمق تاریک چند لایه:** هرگز از مشکی خالص `#000000` به صورت تخت استفاده نکنید؛ از گرادینت‌های چندمرحله‌ای با پایه سرمه‌ای نفتی (`#070b14` تا `#0f172a`) استفاده کنید.
  2. **کنتراست فونت اعداد تله‌متری:** برای نمایش اعداد گیج، دور موتور و دما همیشه خاصیت `font-variant-numeric: tabular-nums` و فونت‌های مونو/سنس با ضخامت بولد بالا اعمال شود تا لرزش ارقام در وب‌سوکت باعث جابه‌جایی المان‌های همجوار نشود.
  3. **شعاع لبه‌ها (Border Radius):** المان‌های شاخص شعاع `28px` (حالت Apple Squircle) و دکمه‌ها شعاع `14px` الی `18px`.
  4. **نشانگرهای وضعیت متحرک:** هر تگ وضعیت شامل یک نقطه پینگ متحرک (`animate-ping`) و یک نقطه اصلی درخشان (`shadow: 0 0 10px`) باشد تا کاربر حس پویایی بلادرنگ را دریافت کند.

---

## 7. الگوی هفتم: پارالاکس عمق‌دار وابسته به اسکرول (Scroll-Driven 3D Depth)
* **منبع اینستاگرام:** https://www.instagram.com/reel/DZsZKSPMa2A/
* **کد جاوااسکریپت:**
```javascript
window.addEventListener('scroll', () => {
  const scrolled = window.pageYOffset;
  const hero3d = document.getElementById('hero3d');
  if (!hero3d) return;
  const rate = scrolled * 0.25;
  hero3d.style.transform = `perspective(1000px) rotateY(${rate}deg) translateY(${rate * 0.4}px) scale(${1 + scrolled * 0.0003})`;
});
```

---

## 8. الگوی هشتم: استیج نورافکن پویا و سایه کف (Dynamic Spotlight & Shadow Stage)
* **منبع اینستاگرام:** https://www.instagram.com/reel/DbSI4_ZRm7U/
* **کد پیاده‌سازی:**
```html
<div class="relative w-full h-80 rounded-3xl bg-slate-950 flex items-center justify-center overflow-hidden">
  <!-- Dynamic Spotlight -->
  <div class="absolute inset-0 bg-[radial-gradient(circle_at_50%_30%,_rgba(6,182,212,0.25),_transparent_70%)] pointer-events-none"></div>
  
  <!-- 3D Model with Floor Shadow -->
  <div class="relative" style="transform-style: preserve-3d;">
    <div id="modelContainer" style="transform: translateZ(40px);">
      <!-- 3D Object Graphic / SVG -->
    </div>
    <!-- Realistic Floor Ambient Shadow -->
    <div class="absolute -bottom-8 left-1/2 -translate-x-1/2 w-48 h-8 rounded-full bg-cyan-950/80 blur-xl"></div>
  </div>
</div>
```

---

## 🚀 راهنمای استفاده سریع در پروژه‌های جدید

1. **برای اضافه کردن افکت 3D به هر کارت:** فقط کافیست صفت `data-3d-tilt="true"` را به تگ اضافه کرده و تابع `init3DMotionCards()` را پس از بارگذاری صفحه فراخوانی کنید.
2. **برای استودیوی نمای انفجاری:** کلاس `stage-3d-viewport` را به کانتینر والد داده و لایه‌ها را با `translateZ` فاصله‌گذاری کنید.
3. **برای نوار پایین موبایل:** از کدهای بخش ۱ استفاده کنید تا بدون نصب هیچ کتابخانه حجیمی، ناوبری شناور مایع درجه یک داشته باشید.
