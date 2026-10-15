#pragma once

// Mobile screen server UI. All machine writes still pass through webportal.h
// safety gates; motion and output commands are intentionally read-only here.
static const char SCREEN_SERVER_PAGE[] PROGMEM = R"screenserver(
<!doctype html>
<html lang="fa" dir="rtl">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<meta name="theme-color" content="#f4f4f4">
<title>Haffler Cutter PLC</title>
<style>
:root{--bg:#eef1f4;--surface:#fff;--panel:#f1f3f5;--ink:#17191c;--muted:#697079;--line:#a3a9b0;--green:#078b74;--orange:#b85f00;--red:#e3342f;--blue:#087ad9;--soft:#f7f8fa;--focus:#087ad944}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html,body{overflow-x:hidden}body{margin:0;background:var(--bg);color:var(--ink);font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Tahoma,Arial,sans-serif}
button,input,select{font:inherit}
.shell{width:min(100%,470px);min-height:100vh;margin:auto;padding:16px 16px 32px;background:var(--surface);box-shadow:0 0 28px #0000000c}
.top{height:68px;border:1px solid #dfe3e7;border-radius:14px;background:var(--panel);display:flex;align-items:center;padding:0 10px;gap:7px;direction:ltr}
.top select{width:1px;height:1px;position:absolute;opacity:0;pointer-events:none}.language-menu{position:relative;flex:0 0 105px;direction:ltr}.lang-current{width:105px;height:44px;border:1px solid #cbd2d8;border-radius:10px;background:#fff;color:#202428;padding:5px 7px;font-size:12px;font-weight:750;display:flex;align-items:center;justify-content:space-between;gap:5px;cursor:pointer}.lang-current[aria-expanded=true]{border-color:var(--blue);box-shadow:0 0 0 2px var(--focus)}.lang-flag{width:22px;height:15px;flex:none;border:1px solid #78818a;border-radius:2px}.lang-popup{position:absolute;top:50px;inset-inline-start:0;z-index:60;width:168px;padding:5px;border:1px solid #cbd2d8;border-radius:11px;background:#fff;box-shadow:0 12px 28px #0003}.lang-popup[hidden]{display:none}.lang-popup button{width:100%;min-height:44px;border:0;border-radius:8px;background:#fff;padding:7px;display:flex;align-items:center;gap:9px;color:var(--ink);font-weight:750;cursor:pointer}.lang-popup button:hover,.lang-popup button:focus-visible{background:#edf5fc}
.brand{flex:1;min-width:0;text-align:center;font-family:Georgia,serif;font-size:20px;font-weight:800;letter-spacing:.6px;color:#b66d00;white-space:nowrap}
.brand small{color:#2d2d2d;font-size:20px}
.hamb{width:44px;height:44px;border:0;border-radius:10px;background:transparent;display:grid;place-items:center;padding:6px;cursor:pointer}
.hamb i,.hamb:before,.hamb:after{content:"";display:block;width:29px;height:4px;border-radius:4px;background:#111;margin:3px 0}
.conn{display:flex;align-items:center;justify-content:center;gap:7px;margin:12px 0 10px;color:var(--muted);font-size:12px;direction:ltr;min-height:22px}
.dot{width:9px;height:9px;border-radius:50%;background:#aaa}.dot.on{background:var(--green);box-shadow:0 0 0 4px #07967e1f}.dot.off{background:var(--red)}
.statebar{display:flex;align-items:center;justify-content:center;gap:7px;min-height:0;margin:0 0 12px;padding:0 10px;border-radius:10px;font-size:12px;font-weight:750;overflow:hidden;transition:.18s}.statebar.show{min-height:38px;padding-block:8px;background:#fff3cd;color:#694d00;border:1px solid #ecd17b}.statebar.dirty{background:#eaf3ff;color:#075da8;border-color:#a8cff3}
.page{display:none}.page.on{display:block}
.dash,.card{border:2px dashed var(--line);border-radius:13px;padding:16px 18px;margin-bottom:16px;background:#fff}
.card.solid{border:0;background:var(--soft);box-shadow:0 1px 9px #00000012}
.hero{display:grid;grid-template-columns:1fr 1fr;gap:14px;direction:ltr}
.metric{text-align:center}.metric .cap{font-size:12px;color:var(--muted);margin-bottom:6px}.metric .val{font-size:31px;font-weight:800;line-height:1;color:#111}.metric .unit{font-size:12px;color:var(--muted);margin-inline-start:3px}
.status-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:10px;margin-bottom:16px;direction:ltr}.status-grid .status{min-width:0;overflow-wrap:anywhere}
.status{border:2px dashed #999;border-radius:11px;min-height:72px;padding:10px 5px;text-align:center}.status b{display:block;font-size:21px;margin-bottom:4px}.status span{font-size:11px;color:var(--muted)}
.status.good{border-color:var(--green);color:var(--green)}.status.warn{border-color:var(--orange);color:var(--orange)}.status.bad{border-color:var(--red);color:var(--red)}
.live-list{border-color:var(--green)}.live-row{display:grid;grid-template-columns:auto 1fr auto;align-items:center;gap:10px;padding:8px 0}.live-row+.live-row{border-top:1px dashed #aaa}
.live-row .ico{font-size:24px}.live-row .name{font-weight:750}.live-row .v{font-size:17px;font-weight:800;direction:ltr}
.orange{color:var(--orange)}.red{color:var(--red)}.green{color:var(--green)}
.primary{width:100%;min-height:52px;border:0;border-radius:11px;background:var(--green);color:#fff;font-weight:750;font-size:16px;padding:12px;cursor:pointer}.primary.compact{min-height:46px;font-size:14px}
.primary.blue{background:var(--blue)}.primary.danger{background:var(--red)}.primary.gray{background:#666}.primary:disabled{opacity:.45}
.head{display:flex;align-items:center;justify-content:space-between;margin:4px 0 13px;min-height:48px}.head h2{margin:0;font-size:20px}.head button{width:44px;height:44px;border:0;border-radius:10px;background:#edf5fc;color:var(--blue);font-size:19px;font-weight:800;cursor:pointer}
.hint{font-size:12px;line-height:1.65;color:var(--muted);margin:4px 0 12px}.setting-help{display:none;margin-top:5px;padding:6px 8px;border-radius:7px;background:#eef5fa;color:#31546a;font-size:11px;line-height:1.5}.field:focus-within .setting-help,.field:hover .setting-help{display:block}.menu-group{margin:12px 0 4px;color:var(--muted);font-size:11px;font-weight:800;text-transform:uppercase}.report-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.report-grid .stat-box{background:#fff}.io-item .badge{display:inline-block;margin-top:5px;padding:3px 6px;border-radius:6px;background:#eef1f4;font-size:10px}.io-item.active .badge{background:#dff4ee;color:#087b67}
.form-grid{display:grid;grid-template-columns:1fr 1fr;gap:11px}.field{margin-bottom:11px}.field.full{grid-column:1/-1}
label{display:block;font-size:12px;font-weight:700;margin:0 0 5px;color:#444}
input,select{width:100%;height:44px;border:1px solid #c7c7cc;border-radius:9px;background:#fff;padding:8px 10px;color:#111;direction:ltr}
input:focus,select:focus,button:focus-visible{outline:3px solid var(--focus);outline-offset:2px;border-color:var(--blue)}
.actions{display:grid;gap:9px;margin-top:12px}.actions.two{grid-template-columns:1fr 1fr}.msg{min-height:18px;margin-top:9px;font-size:12px;font-weight:700;text-align:center}.msg.ok{color:var(--green)}.msg.err{color:var(--red)}
.io{display:grid;grid-template-columns:1fr 1fr;gap:9px}.io-item{border:2px dashed #aaa;border-radius:10px;padding:11px;text-align:center}.io-item b{display:block;font-size:13px}.io-item span{font-size:11px;color:var(--muted)}.io-item.active{border-color:var(--green);color:var(--green);background:#07967e0b}
.stats{display:grid;grid-template-columns:1fr 1fr;gap:10px}.stat-box{border:2px dashed #aaa;border-radius:10px;padding:12px;text-align:center}.stat-box b{display:block;font-size:22px}.stat-box span{font-size:11px;color:var(--muted)}
.table-wrap{overflow:auto;max-height:54vh;border:1px solid #d9dde1;border-radius:11px;background:#fff}table{width:100%;border-collapse:collapse;direction:ltr}th{position:sticky;top:0;background:#e9edf1;z-index:1}th,td{padding:5px;border-bottom:1px solid #e4e4e4;text-align:center;font-size:12px}td input{height:44px;padding:6px;text-align:center}.table-wrap.empty{display:none}.list-derived{display:none!important}
.import-summary{display:none;margin:12px 0;padding:12px;border-radius:10px;background:#eef8f5;border:1px solid #a9d7cb;font-size:13px;line-height:1.7}.import-summary.show{display:block}.import-errors{max-height:150px;overflow:auto;margin-top:8px;color:#9e2e28}.import-errors div{padding:4px 0;border-top:1px solid #efd2cf}
.plan-summary{display:grid;grid-template-columns:repeat(2,1fr);gap:8px;margin-bottom:12px}.plan-kpi{padding:12px 7px;border-radius:11px;background:var(--soft);text-align:center}.plan-kpi b{display:block;font-size:19px}.plan-kpi span{font-size:11px;color:var(--muted)}.plan-bars{display:grid;gap:10px}.plan-bar{border:1px solid #d9dde1;border-radius:11px;padding:11px;background:#fff}.plan-bar-head{display:flex;justify-content:space-between;font-size:12px;font-weight:750;margin-bottom:8px}.plan-track{display:flex;height:22px;border-radius:7px;overflow:hidden;background:#e5e8eb;direction:ltr}.plan-piece{background:var(--green);border-inline-end:1px solid #fff;color:#fff;font-size:10px;display:grid;place-items:center;min-width:2px}.plan-waste{background:#d6d9dd}.plan-note{font-size:12px;color:var(--muted);line-height:1.6}
.icon-label{display:flex;align-items:center;gap:7px}.icon-label .label-icon{font-size:17px;line-height:1}.help-book{font-size:18px}.hero-actions{display:grid;grid-template-columns:1fr 1fr;gap:9px;margin-top:12px}.project-summary{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin:12px 0}.project-summary div{padding:9px;border-radius:10px;background:var(--soft);text-align:center;font-size:12px}.project-summary b{display:block;font-size:17px}.range-pair{display:grid;grid-template-columns:1fr 94px;gap:9px;align-items:center}.range-pair input[type=range]{height:44px;padding:0;accent-color:var(--green)}details.advanced{margin-top:14px;border:1px solid #d9dde1;border-radius:11px;background:#fff}details.advanced summary{min-height:48px;display:flex;align-items:center;padding:0 12px;font-weight:750;cursor:pointer}details.advanced .advanced-body{padding:0 12px 12px}.warning{padding:10px 12px;border-radius:10px;background:#fff3cd;color:#694d00;font-size:12px;line-height:1.6}.list-toolbar{display:flex;justify-content:space-between;align-items:center;gap:9px;margin:12px 0}.list-toolbar .hint{margin:0}.row-delete{width:44px;height:44px;min-height:44px;border:0;border-radius:9px;background:#fff0ef;color:var(--red);font-weight:800;cursor:pointer}.export-grid{display:grid;grid-template-columns:1fr 1fr;gap:9px}.export-grid button{min-height:46px;border:1px solid #cdd4da;border-radius:10px;background:#fff;color:var(--ink);font-weight:750;cursor:pointer}.designer-unit-bar{border:1px solid #cdd4da;border-radius:12px;padding:10px;background:#f8fafc;margin-bottom:14px}.designer-unit-head{display:flex;justify-content:space-between;align-items:center;margin-bottom:8px;flex-wrap:wrap;gap:6px}.designer-unit-actions{display:flex;gap:6px}.designer-unit-list{display:flex;gap:10px;overflow-x:auto;padding:4px 2px 8px;direction:ltr}.designer-unit-card{flex:0 0 120px;border:2px solid #d9dde1;border-radius:10px;padding:6px;background:#fff;text-align:center;cursor:pointer;transition:.18s;position:relative}.designer-unit-card.active{border-color:#087ad9;box-shadow:0 0 0 2px #087ad933;background:#f0f7ff}.designer-unit-card .u-title{font-weight:750;font-size:12px;margin-bottom:2px}.designer-unit-card .u-size{font-size:11px;color:#087ad9;font-weight:bold;direction:ltr}.designer-unit-card .u-qty{font-size:10px;color:var(--muted);margin-top:2px}.designer-unit-card .u-thumb{width:100%;height:54px;margin:4px 0;display:grid;place-items:center;background:#fdfefe;border:1px solid #e2e8f0;border-radius:6px;overflow:hidden}.dimension-overlay{position:absolute;inset:0;pointer-events:none;z-index:4}.dim-line{stroke:#087ad9;stroke-width:1.5;marker-start:url(#dim-arrow-start);marker-end:url(#dim-arrow-end)}.dim-text{font-size:11px;font-weight:bold;fill:#087ad9;pointer-events:auto;cursor:pointer;background:#fff}.designer-canvas{position:relative;height:300px;border:1px solid #cdd4da;border-radius:12px;background:linear-gradient(90deg,#edf1f4 1px,transparent 1px) 0 0/24px 24px,linear-gradient(#edf1f4 1px,transparent 1px) 0 0/24px 24px;overflow:hidden;touch-action:none}.design-frame{position:absolute;left:12%;top:12%;width:76%;height:76%;border:10px solid #3e5969;background:#fbfdff}.design-divider{position:absolute;background:#3e5969}.design-divider.v{left:50%;top:0;width:7px;height:100%;transform:translateX(-50%)}.design-divider.h{left:0;top:50%;height:7px;width:100%;transform:translateY(-50%)}.draft-badge{display:inline-flex;padding:5px 8px;border-radius:8px;background:#fff3cd;color:#694d00;font-size:11px;font-weight:800}.designer-tools{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin:12px 0}.designer-tools button{min-height:44px;border:1px solid #cdd4da;border-radius:9px;background:#fff;font-weight:700;cursor:pointer}
.portal-hub{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:11px}.hub-card{min-height:112px;border:1px solid #d9dde1;border-radius:13px;background:#fff;padding:14px;text-align:start;cursor:pointer;color:var(--ink)}.hub-card:hover,.hub-card:focus-visible{border-color:var(--blue);box-shadow:0 6px 18px #087ad918}.hub-card b{display:block;font-size:15px;margin:7px 0 4px}.hub-card span{display:block;color:var(--muted);font-size:11px;line-height:1.55}.hub-icon{width:34px;height:34px;border-radius:9px;background:#edf5fc;color:var(--blue);display:grid!important;place-items:center;font-size:18px!important}.menu button{display:grid;grid-template-columns:34px 1fr;align-items:center;gap:10px}.menu .nav-icon{width:32px;height:32px;border-radius:9px;background:#edf5fc;color:var(--blue);display:grid;place-items:center;font-size:17px}.menu button.on .nav-icon{background:#e5f6f1;color:var(--green)}
.designer-panel{margin-top:12px;padding:13px;border:1px solid #d9dde1;border-radius:12px;background:#fff}.designer-panel h3{font-size:15px;margin:0 0 10px}.dimension-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:9px}.dimension-axis{padding:10px;border-radius:10px;background:var(--soft)}.dimension-axis b{display:block;font-size:12px;margin-bottom:7px}.dimension-row{display:grid;grid-template-columns:1fr 100px;align-items:center;gap:8px;margin-top:7px;font-size:12px}.dimension-row input{height:40px}.designer-cut-table{max-height:260px;overflow:auto;border:1px solid #d9dde1;border-radius:10px}.designer-cut-table td,.designer-cut-table th{padding:7px}.design-cell{overflow:hidden}.design-cell .cell-size{font-size:10px;font-weight:800;background:#ffffffd9;border-radius:6px;padding:2px 4px;direction:ltr}
.support-shell{display:grid;grid-template-rows:auto minmax(210px,32vh) auto;gap:11px}.support-notice{padding:8px 11px;border-radius:10px;background:#eaf3ff;color:#075da8;font-size:11px;line-height:1.55}.support-chat{overflow:auto;padding:12px;border:1px solid #d9dde1;border-radius:12px;background:#f7f9fb;display:flex;flex-direction:column;gap:9px}.chat-msg{max-width:88%;padding:10px 12px;border-radius:12px;font-size:13px;line-height:1.75;white-space:pre-wrap}.chat-msg.user{align-self:flex-end;background:var(--blue);color:#fff;border-end-end-radius:3px}.chat-msg.bot{align-self:flex-start;background:#fff;border:1px solid #dfe3e7;border-end-start-radius:3px}.chat-actions{display:flex;flex-wrap:wrap;gap:6px;margin-top:7px}.chat-actions button,.support-chips button{min-height:44px;border:1px solid #b9d4e9;border-radius:9px;background:#edf6fd;color:#075da8;padding:7px 10px;font-size:12px;font-weight:750;cursor:pointer}.support-compose{display:grid;grid-template-columns:1fr 52px;gap:8px}.support-compose input{height:52px}.support-compose button{height:52px;border:0;border-radius:11px;background:var(--green);color:#fff;font-size:20px;cursor:pointer}.support-chips{display:flex;flex-wrap:wrap;gap:6px}.support-service{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:10px}
.support-hero{display:grid;grid-template-columns:44px 1fr auto;align-items:center;gap:10px;padding:11px 12px;border:1px solid #d9e6ef;border-radius:13px;background:linear-gradient(135deg,#f8fcff,#eef7f4)}.support-avatar{width:44px;height:44px;border-radius:13px;display:grid;place-items:center;background:#087ad9;color:#fff;font-size:21px;box-shadow:0 5px 14px #087ad930}.support-hero b{display:block;font-size:14px}.support-hero small{display:block;color:var(--muted);font-size:11px;line-height:1.5;margin-top:2px}.support-local{padding:5px 8px;border-radius:99px;background:#e4f5ef;color:#08725f;font-size:10px;font-weight:800;white-space:nowrap}.support-notice{margin-top:8px}.support-chips{margin-top:8px}.support-chat{background:linear-gradient(#f8fafc,#f4f7f9)}.chat-msg{position:relative;animation:chat-in .16s ease-out}.chat-msg.bot{box-shadow:0 4px 13px #26384a0a}.chat-meta{display:block;margin-bottom:3px;font-size:10px;font-weight:800;opacity:.62}.chat-body{display:block}.chat-msg.user .chat-meta{text-align:end}.support-compose input:focus{border-color:var(--blue);box-shadow:0 0 0 3px #087ad918}.support-compose button:active,.chat-actions button:active,.support-chips button:active{transform:scale(.97)}@keyframes chat-in{from{opacity:0;transform:translateY(4px)}to{opacity:1;transform:none}}
.chat-msg.live{border-color:#9bd3c8;background:#f2fbf8}.chat-msg.live .chat-meta{color:#08725f;opacity:1}@media(max-height:700px){.support-shell{grid-template-rows:auto minmax(140px,20vh) auto;gap:7px}.support-hero{padding:8px 10px}.support-hero small{display:none}.support-notice{padding:6px 9px;line-height:1.4}.support-chips{margin-top:5px}.support-chat{padding:8px}}
.chat-msg.bot{padding-inline-start:43px}.chat-icon{position:absolute;inset-inline-start:10px;top:12px;width:24px;height:24px;border-radius:8px;display:grid;place-items:center;background:#e8f3fc;color:#075da8;font-size:14px;font-weight:900}.chat-msg.live .chat-icon{background:#d9f2eb;color:#08725f}.chat-msg.warn .chat-icon{background:#fff0e8;color:#b54708}.chat-actions button,.support-chips button{display:inline-flex;align-items:center;justify-content:center;gap:6px;text-align:start}.support-chips button::first-letter,.chat-actions button::first-letter{font-size:15px}.support-compose input::placeholder{color:#66727d;opacity:1}
.back-page{width:44px;height:44px;border:0;border-radius:10px;background:#edf5fc;color:var(--blue);font-size:20px;font-weight:800;cursor:pointer}.back-page:before{content:'‹'}.cut-saw{width:21px;height:21px;vertical-align:-5px}.saw-tooth{fill:#eef4f6;stroke:#51616d;stroke-width:1.7}.saw-hole{fill:#51616d}.field-meta{display:flex;justify-content:space-between;color:var(--muted);font-size:11px;margin-top:4px;direction:ltr}.inline-error{padding:8px 10px;border-radius:9px;background:#fff0ef;color:#9e2e28;font-size:12px;line-height:1.5}.waste-legend{display:flex;flex-wrap:wrap;gap:8px;margin:10px 0;font-size:11px;color:var(--muted)}.waste-key{display:inline-flex;align-items:center;gap:4px}.waste-key i{width:10px;height:10px;border-radius:50%;display:inline-block}.waste-key.green i{background:#078b74}.waste-key.yellow i{background:#d58b00}.waste-key.red i{background:#d83a32}.plan-bar.waste-green{border-color:#9fd8ce}.plan-bar.waste-yellow{border-color:#edcf89}.plan-bar.waste-red{border-color:#e7aaa6}.plan-bar .remove-bar{min-height:44px;margin-top:8px;border:1px solid #e7aaa6;border-radius:8px;background:#fff0ef;color:#9e2e28;font-weight:750;cursor:pointer}.designer-canvas .design-divider{cursor:grab;z-index:3}.designer-canvas .design-divider.selected{outline:3px solid #087ad9;outline-offset:2px}.design-frame.template-door{border-radius:2px;border-width:12px}.design-frame.template-sliding{border-color:#557d95}.design-frame.template-sash:after{content:'↗';position:absolute;inset:0;display:grid;place-items:center;color:#557d95;font-size:42px}.design-gallery{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:8px;margin-top:12px}.design-card{padding:10px;border:1px solid #d9dde1;border-radius:10px;background:#fff;font-size:12px}.design-card button{width:100%;min-height:44px;margin-top:6px;border:1px solid #cdd4da;border-radius:8px;background:#fff;font-weight:750;cursor:pointer}.loading{transition:opacity .18s}.loading.hide{display:grid;opacity:0;pointer-events:none}.loading-brand{font-family:Georgia,serif;color:#b66d00;letter-spacing:.8px;margin-top:8px}@media(prefers-reduced-motion:reduce){*,*:before,*:after{animation-duration:.01ms!important;transition-duration:.01ms!important}}
.design-cell{position:absolute;border:1px dashed #9fb1bd;background:#ffffff33;z-index:1;display:grid;place-items:center;font-size:12px;color:#365464;cursor:pointer}.design-cell.selected{outline:3px solid #087ad9;z-index:2}.design-cell.cell-door,.design-cell.cell-door-left,.design-cell.cell-door-right{background:#f5e7d5}.design-cell.cell-sash,.design-cell.cell-sash-left,.design-cell.cell-sash-right,.design-cell.cell-tilt{background:#e8f2f8}.design-cell.cell-sliding{background:#eef3f7}.design-thumb{width:100%;aspect-ratio:1.4;border:1px solid #cdd4da;border-radius:7px;background:#f8fafb;margin:6px 0}
.ui-icon{width:21px;height:21px;display:inline-block;flex:none;vertical-align:middle;stroke-linecap:round;stroke-linejoin:round}.ui-icon.sm{width:17px;height:17px}.ui-icon.lg{width:26px;height:26px}.icon-button-content{display:inline-flex;align-items:center;justify-content:center;gap:7px}.hero-actions button{display:flex;align-items:center;justify-content:center;gap:8px}.help-book .ui-icon{width:22px;height:22px}.chat-icon .ui-icon{width:16px;height:16px}.chat-actions .ui-icon,.support-chips .ui-icon{width:17px;height:17px}.opening-mark{position:absolute;inset:12% 15%;width:70%;height:70%;pointer-events:none;color:#557d95}.opening-name{position:relative;z-index:2;align-self:start;margin-top:5px;padding:2px 4px;border-radius:5px;background:#ffffffd9;font-size:9px;font-weight:750}.cell-size{position:relative;z-index:2;align-self:end;margin-bottom:5px}.designer-kpis{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:8px;margin:10px 0}.designer-kpis div{padding:9px 6px;border:1px solid #dce3e8;border-radius:10px;background:#fff;text-align:center}.designer-kpis b{display:block;font-size:16px;color:#24485d}.designer-kpis span{font-size:10px;color:var(--muted)}
.io{display:block}.io-group+.io-group{margin-top:16px}.io-group-head{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-bottom:8px}.io-group-title{display:flex;align-items:center;gap:8px;font-size:14px}.io-count{padding:4px 8px;border-radius:99px;background:#edf1f4;color:var(--muted);font-size:10px;font-weight:800}.io-grid{display:grid;grid-template-columns:1fr 1fr;gap:9px}.io-item{display:grid;grid-template-columns:38px minmax(0,1fr) auto;align-items:center;gap:9px;min-height:68px;border:1px solid #d8dde2;border-radius:12px;padding:9px;background:#fff;text-align:start}.io-item .io-symbol{width:38px;height:38px;border-radius:10px;display:grid;place-items:center;background:#eef3f6;color:#52636e}.io-item .io-copy{min-width:0}.io-item b{display:block;font-size:12px;overflow-wrap:anywhere}.io-item small{display:block;margin-top:2px;color:var(--muted);font-size:10px}.io-item .io-state{min-width:54px;padding:5px 7px;border-radius:99px;background:#edf1f4;color:#616a72;font-size:10px;font-weight:850;text-align:center}.io-item.active{border-color:#83cabc;background:#f3fbf8;color:#076b59}.io-item.active .io-symbol,.io-item.active .io-state{background:#dff4ee;color:#087b67}.io-item.disconnected{opacity:.62;border-style:dashed}.io-item.disconnected .io-state{background:#fff0ef;color:#a23530}.io-legend{display:flex;flex-wrap:wrap;gap:10px;margin-top:12px;color:var(--muted);font-size:10px}.io-legend span{display:inline-flex;align-items:center;gap:5px}.io-legend i{width:9px;height:9px;border-radius:50%;background:#aab2b9}.io-legend .on i{background:var(--green)}.io-legend .off i{background:var(--red)}
@media(max-width:420px){.io-grid{grid-template-columns:1fr}.designer-kpis{grid-template-columns:1fr 1fr}.designer-tools{grid-template-columns:1fr 1fr}}
#designerDraftTools>button:nth-of-type(-n+2){display:none}#designerDraftTools{grid-template-columns:1fr 1fr;align-items:end}#designerDraftTools .field{margin:0}#designerDraftTools .field label{margin-bottom:5px}.opening-inspector{grid-column:1/2}.delete-divider{grid-column:2/3;color:#a23530;border-color:#e3b3af!important}.designer-tools button{display:flex;align-items:center;justify-content:center;gap:7px}
progress{width:100%;height:15px;margin-top:12px}
.backdrop{position:fixed;inset:0;background:#0007;z-index:20;opacity:0;pointer-events:none;transition:.18s;overflow:hidden}.backdrop.on{opacity:1;pointer-events:auto}
.drawer{position:absolute;top:0;bottom:0;right:0;width:min(84vw,340px);background:#fff;padding:24px 18px;transform:translateX(102%);transition:.2s;overflow:auto}.backdrop.on .drawer{transform:none}
.drawer-head{display:flex;align-items:center;justify-content:space-between;margin-bottom:16px}.drawer-head b{font-family:Georgia,serif;font-size:20px}.x{width:44px;height:44px;border:0;background:#eee;border-radius:50%;font-size:25px;cursor:pointer}
.menu button{width:100%;min-height:48px;border:0;border-bottom:1px solid #eee;background:#fff;text-align:start;padding:10px 8px;font-weight:700;cursor:pointer}.menu button.on{color:var(--green)}
.modal-wrap{position:fixed;inset:0;background:#0009;z-index:30;display:none;align-items:center;justify-content:center;padding:18px}.modal-wrap.on{display:flex}.modal{width:min(100%,410px);max-height:88vh;overflow:auto;border-radius:16px;background:#fff;padding:18px;box-shadow:0 20px 55px #0005}.modal .modal-head{display:flex;align-items:center;justify-content:space-between;gap:12px;position:sticky;top:-18px;background:#fff;padding:18px 0 10px;z-index:2}.modal h3{margin:0}.help-body{font-size:14px;line-height:1.8;color:#444}.help-body b{color:#111}
.toast{position:fixed;left:50%;bottom:20px;z-index:40;transform:translate(-50%,25px);opacity:0;pointer-events:none;min-width:230px;max-width:90vw;background:#222;color:#fff;padding:12px 16px;border-radius:10px;text-align:center;font-size:13px;transition:.22s}.toast.on{opacity:1;transform:translate(-50%,0)}.toast.ok{background:#087b67}.toast.err{background:#b82b24}
.loading{position:fixed;inset:0;background:#fffffff2;z-index:50;display:grid;place-items:center;text-align:center}.loading.hide{display:none}.loading-logo{width:190px;max-width:70vw;height:auto;display:block;margin:0 auto 14px}.spinner{width:42px;height:42px;border:4px solid #ddd;border-top-color:var(--green);border-radius:50%;animation:spin .8s linear infinite;margin:auto auto 12px}@keyframes spin{to{transform:rotate(360deg)}}
[dir=ltr] .drawer{right:auto;left:0;transform:translateX(-102%)}[dir=ltr] .backdrop.on .drawer{transform:none}
@media(max-width:420px){.brand small{display:none}.brand{font-size:16px;letter-spacing:.2px}}
@media(max-width:480px){.status-grid{grid-template-columns:1fr 1fr}.status-grid .status:last-child{grid-column:1/-1}.project-summary{grid-template-columns:1fr 1fr;gap:10px}.project-summary div:last-child{grid-column:1/-1}}
@media(max-width:390px){.shell{padding:12px 12px 28px}.form-grid{grid-template-columns:1fr}.field.full{grid-column:auto}.actions.two,.hero-actions,.export-grid,.portal-hub,.dimension-grid{grid-template-columns:1fr}.plan-summary{grid-template-columns:repeat(2,1fr)}.hero{gap:8px}.dash,.card{padding:14px}.modal-wrap{padding:10px}.project-summary{grid-template-columns:1fr 1fr}.chat-msg{max-width:94%}}
@media(max-width:350px){.brand{font-size:14px}.brand small{display:none}}
@media(min-width:768px){.shell{width:min(100%,1180px);padding:22px 28px 42px}.top{height:76px}.top select{width:150px}.page.on{max-width:1080px;margin:auto}.dash,.card{padding:20px 22px}.form-grid{grid-template-columns:repeat(3,1fr)}.field.full{grid-column:1/-1}.status-grid{grid-template-columns:repeat(3,1fr)}.live-list{display:grid;grid-template-columns:repeat(3,1fr);gap:12px}.live-row+.live-row{border-top:0;border-inline-start:1px dashed #aaa;padding-inline-start:12px}.hero-actions{grid-template-columns:repeat(4,1fr)}.project-summary{grid-template-columns:repeat(4,1fr)}.plan-summary{grid-template-columns:repeat(4,1fr)}.table-wrap{max-height:58vh}.designer-canvas{height:420px}.drawer{width:360px}.menu button{min-height:54px}}
@media(orientation:landscape) and (max-height:520px){.shell{width:min(100%,760px)}.form-grid{grid-template-columns:1fr 1fr}.table-wrap{max-height:46vh}}
</style>
</head>
<body>
<div class="loading" id="loading"><div><img class="loading-logo" alt="HAFFLER" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAQQAAABCCAYAAACxfqjPAAAAAXNSR0IArs4c6QAAAARzQklUCAgICHwIZIgAAA+TSURBVHhe7Z1NjBxHFcdnLDgZEa8ESDllwhkUIxBE4pCx4BYUbDghEFmf4ERsCS4I8AaOIMVBQgJxyBqBOO2ujUBCCOTxDRAotvg4gTK+BomxI+cG3rzfTtdOdU19dU9P93y8knp3prs+X/X71/uqmn6vvfRc0dR9+T8ONHte7t+W65xcN+W61F73tCWlgFKg3xIJrkg7rxRtPZD/T8vFfzcBAp+1bl6Qz6OW+qjNKAW2ngJtAcKeUPqaRe3L8nnfQ/035N5AAWHr30slQEcU6AoQ7sp4P+KMGSAAEOwUkxBQK16TC0njZbnGHdFQm1UKbAwFmgCEYcGYMDkrv08V2JP7toQAAV1m3y3qsYl7Vb5c91AbMMDWgM2BRJs7GzMrOhClQEcUWBQQYMyJ1XdAAUZ3QcEHCOQhPxeMPfTQIGRYRDIAQEy6EyjfEVm1WaXAelJgUUBg1MfO0PflO5KCSTA6BkWzmlelFIBBnTA9nwECAMFOIZtE1bY0v1JgqynQBCC4ngEIOpKL+zBvXSCoMjGoCz5VpUodmlcpsPUUaAIQfCt2m4RVdaFNamtbG02BJgABArlqQw7RYGRfMgFMOXWQZywXHgufhICN40W57hVSS26dmk8psJUUaAIQXMNiiJC35AFqxKhg4hjBB/JwWFyoHM8kZgdQIKoRG4NJlMfWQF0ktTNs5Suug65CgRxAMEyFC9BmONpx3X9u2w/lBm5DrkV0fJj6ilwX5XoqMEDq35ML4MHFuevkU9dklTdD824lBVKAACO/ZFEGhnu1YG5W7iO5YNZlAYFvUmB0+hEChtBE3vCAxFZOug5aKRCiQAoQRlLQ1elZabnPau1L6Os8Gy+Z7IACUsMTGe3Qp6Fci0gpGc1oFqXAelMgBQgwERGBuQnpASZtKw2kIewSMRsDagvSzLIBqq0xaztKgaVRIAUINOyqDaHOdGm025dO4U3wpVD489KIqhUrBdaVAjmAwNgwJsZW4S7BwNA+BAqr0Ld1fT+031tGgVxAQDQHFHz6+ioxnA8UUBVCcQpbNt06XKVAnAK5gEAtuBjdMGSYjWuV0kg64xpCm7BtfEjq/VtgoD+U+7Y3JkSPWB2UqTIf5P+iXD+PEP838uwzFScnVWfF6k6zf0k+/aL41nUbdl/qjodydQLyctqz34MmafWz4h3+rfz/u68jVV/AnMF0nQfgGsvlSjNIMgYshvIZTwheh1zpYRUB4dfS/+cTBH9Wnv+pwqQ0+QLazSog5E/CsgDB7gGL2Dfletu+uT6AcDh5TtbPh71LO25wlI/MMDsxEjkJkLiQkXHVACElbZghfUM+/CBjfCaLAkI+sdZNQnBHhsRQMsanAeFw8qIw4kAK3hFmhHnaS0eToQhlRB0OrUbH0p+r0hfcjbFEX3P2ReS6JVcNEL4u4/t+5mS8R/KVVoJIOQWETKJKtnUHBEb6abn+YIYcB4TDCTEIM2bsn8QY3BBmXH6Az8FkVxjfPfdgNlV92bsQBwX6nYqhIHpxT65xxjuwSoBwVvrL5rCPZvR7btITZRQQMom6IYBQsn+FAeFocl7w73VhSsTpu/KZFde4HhHb50Gh75yQfHzimZjl6/fuCxOnmW8qGcyY+VjKcUjKVFIxIs6D3ud2UsemhdylRC4CbowpN60SIHxKOv373I5LvjnRMFI2Bgi8PH+u0K6dFZobQ1bXbbRhVKSNuskYXym/KK0+LnV8LdGRUxyIAcKUKQEEVIXDyetSqetlqDtgu9xYGPvpUkVlyeSGPN89fX4wuSJ9mh7pbvoW7gVMb45/N7nY/BQKu46NZ5UAAa9JapLdsXxAbvwnY8JiL2BTjLQpbcRUhrQ6njEZCUDInY8XpB7e+1A6fTfinT7474Nev4+1npUWiWG6Uk9X/XPCkLYrks+pbcq+Dt0Thp8BjS0dHB8/7J3pD0oqipFcqOlYTlv+/M5eZKA+j0NdF+SqAML7Zbxv5r1LpVxflW8/ySi3KczaxjjWBRCm3BJOH5ZHJ9JbHBCOJoPeY9lGDOP3Zc9AFaMijG3S45Idwtw3Br87AgizvIcTAMeoBa/Ks/LeiGqAQA8AGyQC08ZIPsdAJES2VQGEr0gHfxzo5F/lfsiukBuT0AYjbUobWwYIGctJ7SyHE0PMskpgqyY+leBgsifghOcBzLssEgIA0kZaFUCIxR4Qc/CjCCjkxCRsCrO2MQ4FhEY4r6QWOGL/DCh6Ih3MJJiptHJNwGB3CgYn6sT5LCNlI53urQIgfEKG8seIdPAxeRZzR35Pnn8nQY42GGlT2lBAaIS3jiYXZXWfBg7Zq3wZKO7L6j/oTVUE3I9lg2a70gE9zQ0EWoREKUNUjNlNANJAOvBGpBOpmIRNYdY2xtFkHEJo7psax0I2BFZhEwMwls8vy7W/yJteKmuL/bZaUI49mNoWfC7IM+IyTAcmzZo8mlwTUAGExr0zMpa8aEd3uF0DArEHjyJzYKsDMbWCH9P9VaSe2AuIHeLfFd4D2jkNerHKdd1GrnU+NdR1AYSUIXrOqMiqgoeAVXhYXDYxCILhfjPpcDKSiqZGxb78ErSJTbCBggAo3I1Hk3OiKlwURh5L7geVmflg8hpqxgtPvvveP9/63/v+9fbx2aLNqsFVXQNCLPYAYyLqgkkxw2MqJqHJwKQQ43XdxjYBAmDwLblibupTqRExBUlgN8HpME8qCChcBfsQymlfvg6KWzbQ7Mk9830kjAtwVE+PBdhmLtFzgMGtZ88+cyx2hzM3Hz5R0xjZNSAQVfnlADHc/QrQNqY2EPcxDtTVNbNWne86oLMpgFCVVr78pcUEQMgVe1L6bbkxRP3HsiW4XysIqImBntax867+wzeff++jf7z1//H5248+KX1KhT372u8SEFIin897EFMbYjEJCgj5b18u7+TUWMeGkFNvKk/J0EwnQuG9bkX5gJDah5DqYvXnqDSk8YmdgP9GxUBa6B1fLwKsyHNLVJGmIxXRrX+Z0e0nJU9sQ1KIxqnYA1tdMN2oU4ayCggZE1lkiQFC1dBlO1zZ7kGT8+Eb2an9gIe8gETzwSCDIjeffRGHMTFz1pBrBMQWwD6IKYP2RGoYWnEEJvJxWt7EF/CZKEQ3nSntjRhnuxxndoi7lW0Qsz506Xb8i3QjFHDE/oKfemb6g3IvFq4aikno2uDXhuGyDZUhfwGNA9AyAWFOUgx12rcH4IL0exTvuzw9nNyUv1iyYeqr4jq8XiqTE4NAiDQux9VKXQFCLPZgEQqFYhKacnPF+rYpbXQdh1B3/gFd9vjMeYBCgIDUMHFaY8XeS/bgcEI5ypf3KJiCZZdjWZefBSWVw5mTjbaSoStA+K6M7ttLGqFvw9OmMGsb4+gaEEK7T78g70voJK1oCHtMrEEasL0Dea7HFFPbexVslyNvfKpsda4AmEBCW9rYl+9Y7KumLgAhFXtQdQxufl9MQhuMtCltdA0IIdUnZQAPhrDHAAFpYLpnYJbSetGMqe+K8Y7zCsvJSBCuWhBTJeq/9r4xUBtg5/utylhLXQBCattqfcpMS/pWi01h1jbGsaqAwNzGtsgHpYQYg+9LpWbXoXnxLssH7odT2YZQ3nx0OMGeYE4nLm9qKoczz9seqr/6AynCGQ5ICaFEf1CFcoKUugCEWOxBdYr4S7jG4jYYqes2Fjno5XdCRnOuRJNeBnt2cg9IiRlHU1KCt6wPEGAkxGyfa24k9zEuhpPN2OQ6lm3TeAemwUKzOufVhRlYpA8+yWEGgMsFNF853K7zksx8zrYBgXmIBRflnm+QimFwg5q6ZtamPADLss7bbrom4xDsN87my0XmI7b3hYAkTALRU5dREfAwxFbVtPuxfKbBPGv5NiaVtz3vLHhuY4qZ3D6lVaH45qZl/C5D6hDV3BOQGGtMfHTDnhd5AXOAmjxdt5HbT1++dQKEqotB6YCU3BU1LSVARoKT8Er0Sz/bfk++781tTCofetKEy/G2tD10ZtPEPHCfPRvmdxtyj1RrW0KIxR6k9iO4L3LqDEb75N2umVUlhPLBRYvORyxAjfektMDbKyOM7u45CCFpXkwCpWH2qcRBUJBfVz+YHFnqxPwpSdXwHLXE/U0G31Hrpl+MOye1CQgpBk7tWHTHk/JW2DEJi76AObTsuo2cPobyrJOEwBhSJ3QHT122GQkGIsAIZmFFdXXxsdzL/cWjOPGPJi+JnWEWvNSXeuttT6YdgAe921V58mIo4j1tExBSh6hWURfMqFLxDKbOrplVJYRmJQTmP2VPOXVDurpzaNWcRR/OmIZ7l2pB7TSU+BWRCmiPy6RFpQMkA9cYCrgN5MrxJNQajhZSCmwKBXKMaWblxRr/lDPweitv+dwDU+WdE7Wh/o/A2C5Nu5u5NoJNmVMdh1KgNgVyAYEGduUypyjZDaZjE9zulQHh3onKsNhhqaG+0TKSAYYTlRBqvyZacFsoUAUQELtDfvE9eYa0kJ+mx7SHDY35NcXAwNSCdIMhVEEhn66acwspUAUQfPq5TbJ9+YK00GYibgIwykmjAhRy8moepcBWUiAXEGA6d1+Dj2DjAhRgvmWmgVSO+jKs2EjdX22q2IxmVwqsJwVyAMHn10+NFgNf7h6BVF32c9yJ7IUIRVPiUeAZgLRbfDYBSNRT3d5RpXeaVymw5hTIAQSYyw1YYqXF7QhYmM1KLinQ1wEGNuiMF6QTrkliIWByN8bAVM2vC9Mfuy3yAhDcpy/7C/ZDiysFNpoCOYAAQ5lfUL5TMBhGOpNgUpjNXoldogEeI7kob5cNEXcgD8yx8DCzHavgK6OxBhv9murg2qJADiDQF5h+XDC1r28wLFuNcxKSgwEF/vMdAOAiUVdICgjVD9AMcxrXPEoBpUCYArmAkENDXJKGqXPyN5kHUNFYgyYpqnVtJQWaAgRWdPcMxmUQFNUAqQLgcaMm1WC4DIprnVtFgaYAwbYzGAJieBwVKgB2AN/R7rnENt6D/aIAAAQw2KAwLqSE3Do1n1JAKeBQoClAgPFtTwSehV2nLVZ18rkrO9nwECD2k3xbsH3xA9gaqM82ZjY1Hn1RlAJbSYGmGMg+Suq+UBJmNQxuE3ZPvrgBThx2ipfCJIDE3TNBTANl3WSDgg+EtnJSddBKgboUaAoQYGLD1EP5HHItuoCAKuDzKIzlvi1JhADBjBtgyHFn1qWTllMKbAUFmgKEXGK5gBByF7o2iRQg5Lav+ZQCSoEIBdoGBIyL9vFmrrpguorUgJRg7AOhfDq5SgGlQIMUaBsQYPSRXHgcYrYGhjgsLoBhv8Exa1VKAaVAgAJtA4JOhFJAKbDCFHgH3OCcf7/gcJwAAAAASUVORK5CYII="><div class="spinner"></div><b id="loadingText">در حال اتصال به دستگاه</b><div class="loading-brand">HAFFLER · CUTTER PLC</div></div></div>
<div class="shell">
  <header class="top">
    <div class="language-menu"><button class="lang-current" id="langButton" type="button" aria-haspopup="menu" aria-expanded="false" aria-controls="langPopup" onclick="languageMenuToggle(event)"><span id="langCurrentFlag"></span><span id="langCurrentName">فارسی</span><span aria-hidden="true">⌄</span></button><div class="lang-popup" id="langPopup" role="menu" hidden><button type="button" role="menuitem" onclick="setWebLang('fa')"><span class="lang-flag" aria-hidden="true"><svg viewBox="0 0 30 20" width="22" height="15"><path fill="#239f40" d="M0 0h30v7H0z"/><path fill="#fff" d="M0 7h30v6H0z"/><path fill="#da202f" d="M0 13h30v7H0z"/><circle cx="15" cy="10" r="1.4" fill="#da202f"/></svg></span>فارسی</button><button type="button" role="menuitem" onclick="setWebLang('en')"><span class="lang-flag" aria-hidden="true"><svg viewBox="0 0 30 20" width="22" height="15"><rect width="30" height="20" fill="#1555a5"/><path d="M0 0l30 20M30 0L0 20" stroke="#fff" stroke-width="4"/><path d="M0 0l30 20M30 0L0 20" stroke="#d51f2a" stroke-width="1.7"/><path d="M15 0v20M0 10h30" stroke="#fff" stroke-width="6"/><path d="M15 0v20M0 10h30" stroke="#d51f2a" stroke-width="3"/></svg></span>English</button></div></div><select id="webLang" aria-hidden="true" tabindex="-1"><option value="fa">fa</option><option value="en">en</option></select>
    <div class="brand"><small>HAFFLER</small> CUTTER PLC</div>
    <button class="hamb" id="menuButton" aria-label="Menu" aria-expanded="false" onclick="menuOpen()"><i></i></button>
  </header>
  <div class="conn"><span class="dot" id="connDot"></span><span id="connText">Connecting</span><span>·</span><span id="verText">--</span></div>
  <div class="statebar" id="statebar" role="status" aria-live="polite"></div>

  <main>
    <section class="page on" id="home">
      <div class="dash hero">
        <div class="metric"><div class="cap" data-t="current">موقعیت فعلی</div><span class="val" id="cur">--</span><span class="unit">mm</span></div>
        <div class="metric"><div class="cap" data-t="destination">مقصد</div><span class="val" id="dest">--</span><span class="unit">mm</span></div>
      </div>
      <div class="status-grid">
        <div class="status" id="modeBox"><b id="modeIcon">M</b><span id="modeText">--</span></div>
        <div class="status" id="machineBox"><b id="machineIcon">●</b><span id="machineText">--</span></div>
        <div class="status" id="refBox"><b>⌖</b><span id="refText">--</span></div>
      </div>
      <div class="dash live-list">
        <div class="live-row"><span class="ico orange">▤</span><span class="name" data-t="activeRow">ردیف فعال</span><span class="v" id="activeRow">--</span></div>
        <div class="live-row"><span class="ico red">×</span><span class="name" data-t="remaining">تعداد باقی‌مانده</span><span class="v" id="remaining">--</span></div>
        <div class="live-row"><span class="ico green">✂</span><span class="name" data-t="cuts">برش‌های کاربر</span><span class="v" id="homeCuts">--</span></div>
      </div>
      <div class="project-summary">
        <div><b id="homeListRows">--</b><span data-t="validRows">ردیف معتبر</span></div>
        <div><b id="homePlanState">--</b><span data-t="optimizedPlan">لیست بهینه</span></div>
        <div><b id="homeSaveState">--</b><span data-t="lastSave">آخرین ذخیره</span></div>
      </div>
      <div class="hero-actions">
        <button class="primary" onclick="requestPage('list')"><span aria-hidden="true">✂</span> <span data-t="openList">باز کردن لیست برش</span></button>
        <button class="primary blue" onclick="requestPage('designer')"><span aria-hidden="true">▣</span> <span data-t="designer">طراح پنجره و درب</span></button>
        <button class="primary gray" onclick="requestPage('reports')"><span aria-hidden="true">▤</span> <span data-t="reports">گزارش‌ها</span></button>
        <button class="primary gray" id="screenDownloadButton" onclick="downloadScreen()"><span aria-hidden="true">▧</span> <span data-t="screenDownload">دانلود عکس صفحه نمایش</span></button>
      </div>
    </section>

    <section class="page" id="f1">
      <div class="head"><h2 data-t="general">تنظیمات عمومی</h2><button class="help-book" aria-label="Help for this page" onclick="helpOpen('f1')">📖</button></div>
      <div class="card solid"><div class="form-grid">
        <div class="field"><label class="icon-label"><span class="label-icon">▰</span><span data-t="stock">طول شاخه</span></label><input id="stock" type="number" min="10" max="100000"></div>
        <div class="field"><label class="icon-label"><span class="label-icon">✂</span><span data-t="kerf">عرض تیغه</span></label><input id="kerf" type="number" min="0" max="50" step=".1"></div>
        <div class="field"><label class="icon-label"><span class="label-icon">↤</span><span data-t="trim">پرت ابتدا</span></label><input id="trim" type="number" min="0" max="500"></div>
        <div class="field"><label data-t="deviceLang">زبان دستگاه</label><select id="lang"><option value="1">فارسی</option><option value="2">English</option></select></div>
        <div class="field"><label data-t="keyboard">کیبورد لمسی</label><select id="vkb"><option value="0">Auto</option><option value="1">ON</option><option value="2">OFF</option></select></div>
        <div class="field"><label data-t="buzzer">صدای کلید</label><select id="buz"><option value="1">ON</option><option value="0">OFF</option></select></div>
      </div><div class="actions"><button class="primary" onclick="saveF1(this)" data-t="save">ذخیره</button></div><div class="msg" id="m1"></div></div>
    </section>

    <section class="page" id="f2">
      <div class="head"><h2 data-t="motor">تنظیمات موتور</h2><button class="help-book" aria-label="Help for this page" onclick="helpOpen('f2')">📖</button></div>
      <div class="card solid"><div class="form-grid">
        <div class="field"><label data-t="lineStart">ابتدای خط</label><input id="startmm" type="number" min="0" max="20000"></div>
        <div class="field"><label data-t="lineEnd">انتهای خط</label><input id="maxlen" type="number" min="100" max="20000"></div>
        <div class="field"><label class="icon-label"><span class="label-icon">↔</span><span data-t="cutSpeed">سرعت حرکت</span></label><div class="range-pair"><input id="spdRange" type="range" min="0" max="100" oninput="syncRange('spd',this.value)"><input id="spd" type="number" min="0" max="100" oninput="syncNumber('spdRange',this.value)"></div></div>
        <div class="field"><label class="icon-label"><span class="label-icon">⌁</span><span data-t="autoRamp">شیب حرکت</span></label><div class="range-pair"><input id="arampRange" type="range" min="0" max="5000" step="10" oninput="syncRange('aramp',this.value)"><input id="aramp" type="number" min="0" max="5000" oninput="syncNumber('arampRange',this.value)"></div></div>
      </div><div class="actions"><button class="primary" onclick="saveF2(this)" data-t="save">ذخیره</button></div><div class="msg" id="m2"></div></div>
    </section>

    <section class="page" id="f3">
      <div class="head"><h2 data-t="ioStatus">وضعیت ورودی‌ها</h2><button class="help-book" aria-label="Help for this page" onclick="helpOpen('f3')">📖</button></div>
      <p class="hint" data-t="ioSafe">نمایش زنده است. برای ایمنی، فرمان خروجی و حرکت فقط از خود دستگاه انجام می‌شود.</p>
      <div class="io" id="iobox"></div>
    </section>

    <section class="page" id="f4">
      <div class="head"><h2 data-t="admin">مدیریت</h2><button class="help-book" aria-label="Help for this page" onclick="helpOpen('f4')">📖</button></div>
      <div class="card solid"><div class="form-grid">
        <div class="field"><label data-t="warmup">زمان آماده‌سازی اره</label><input id="ctimer" type="number" min="0" max="60"></div>
        <div class="field"><label class="icon-label"><span class="label-icon">⛔</span><span data-t="estop">نوع ورودی توقف</span></label><select id="estop"><option value="1">N.C</option><option value="0">N.O</option></select><div class="warning" data-t="stopWarning">تغییر سیم‌کشی توقف فقط با تأیید ایمنی انجام شود.</div></div>
        <div class="field full"><label data-t="dayLock">روز باقی‌مانده قفل</label><input id="dayrem" disabled></div>
        <div class="field"><label data-t="adminPin">رمز مدیر</label><input id="adminpin" type="password" maxlength="12"></div>
        <div class="field"><label data-t="newAdminPin">رمز مدیر جدید</label><input id="newadmin" maxlength="12"></div>
      </div>
      <details class="advanced"><summary><span aria-hidden="true">⚙</span>&nbsp;<span data-t="advanced">پیشرفته</span></summary><div class="advanced-body"><p class="warning" data-t="advancedWarning">این تنظیمات برای مدیر است؛ مقدار نادرست می‌تواند رفتار دستی دستگاه را تغییر دهد.</p><div class="form-grid"><div class="field"><label data-t="baseDir">جهت پایه موتور</label><select id="bdir"><option value="1">Normal</option><option value="-1">Reverse</option></select></div><div class="field"><label data-t="jogSpeed">سرعت جاگ</label><input id="jspd" type="number" min="0" max="100"></div><div class="field"><label data-t="jogRamp">شیب جاگ</label><input id="jramp" type="number" min="0" max="5000"></div><div class="field"><label data-t="jogStart">راه‌اندازی نرم جاگ</label><input id="jinit" type="number" min="40" max="400"></div></div></div></details>
      <div class="actions"><button class="primary" onclick="saveF4(this)" data-t="save">ذخیره</button></div><div class="msg" id="m4"></div></div>
    </section>

    <section class="page" id="f5">
      <div class="head"><h2 data-t="developer">اطلاعات دستگاه</h2><button onclick="helpOpen('f5')">?</button></div>
      <div class="stats">
        <div class="stat-box"><b id="tcuts">--</b><span data-t="lifeCuts">کل برش‌ها</span></div>
        <div class="stat-box"><b id="ucuts">--</b><span data-t="userCuts">برش‌های کاربر</span></div>
        <div class="stat-box"><b id="heap">--</b><span data-t="freeRam">حافظه آزاد</span></div>
        <div class="stat-box"><b id="uptime">--</b><span data-t="uptime">زمان روشن بودن</span></div>
      </div>
      <div class="card solid"><div id="devstat" class="hint"></div><button class="primary danger" onclick="resetUserCuts()" data-t="resetCuts">صفر کردن شمارنده کاربر</button><div class="msg" id="m5"></div></div>
    </section>

    <section class="page" id="list">
      <div class="head"><h2 data-t="cutList">لیست برش</h2><button onclick="helpOpen('list')">?</button></div>
      <div class="card solid">
        <div class="field"><label data-t="visibleRows">تعداد ردیف فعال</label><input id="rows" type="number" min="0" max="100"></div>
        <div class="field"><label data-t="importFile">ورود فایل متنی یا CSV</label><input id="szfile" type="file" accept=".txt,.csv,.tsv,.text"></div>
        <div class="actions two">
          <button class="primary compact gray" onclick="preflightImport()" data-t="reviewFile">بررسی فایل</button>
          <button class="primary compact blue" onclick="downloadCurrentList()" data-t="exportList">دانلود لیست</button>
        </div>
        <div class="import-summary" id="importSummary" aria-live="polite">
          <div id="importStats"></div><div id="importMap"></div><div class="import-errors" id="importErrors"></div>
          <div class="field"><label>Apply mode</label><select id="importMode"><option value="replace">Replace current form</option><option value="append">Append to current form</option></select></div>
          <label class="hint"><input id="importUnitConfirm" type="checkbox" onchange="renderImportPreview()"> I confirm the displayed unit conversion / unitless values.</label>
          <button class="primary compact" id="applyImportButton" onclick="applyImportPreview()" data-t="replaceOnForm">جایگزینی روی فرم</button>
        </div><div class="msg" id="impmsg"></div>
        <div class="table-wrap" id="tableWrap"><table id="listtbl"><thead><tr><th>#</th><th data-t="length">متراژ</th><th data-t="count">تعداد</th></tr></thead><tbody></tbody></table></div>
        <div class="actions"><button class="primary" onclick="saveList(this)" data-t="saveList">ذخیره لیست</button></div><div class="msg" id="listmsg"></div>
      </div>
    </section>

    <section class="page" id="plan">
      <div class="head"><h2 data-t="optimizedPlan">لیست بهینه</h2><button onclick="helpOpen('plan')">?</button></div>
      <p class="hint" data-t="planReadOnly">این صفحه فقط نتیجهٔ آمادهٔ دستگاه را نمایش می‌دهد و هیچ فرمان حرکت یا بهینه‌سازی ارسال نمی‌کند.</p>
      <div class="card solid">
        <div class="plan-summary">
          <div class="plan-kpi"><b id="planBars">--</b><span data-t="bars">شاخه‌ها</span></div>
          <div class="plan-kpi"><b id="planPieces">--</b><span data-t="pieces">قطعات</span></div>
          <div class="plan-kpi"><b id="planWaste">--</b><span data-t="waste">پرت</span></div>
        </div>
        <div class="plan-note" id="planMeta"></div><div class="plan-bars" id="planBarsList"></div>
        <div class="actions two">
          <button class="primary compact gray" onclick="loadPlan(this)" data-t="refresh">بازخوانی</button>
          <button class="primary compact blue" id="downloadPlanButton" onclick="downloadPlan()" disabled data-t="downloadPlan">دانلود نتیجه</button>
        </div><div class="msg" id="planmsg"></div>
      </div>
    </section>

    <section class="page" id="designer">
      <div class="head"><h2><span data-t="designer">Window / door designer</span> <span class="draft-badge">DRAFT</span></h2><button class="help-book" aria-label="Help for this page" onclick="helpOpen('designer')">📖</button></div>
      <div class="card solid"><p class="warning" data-t="designerWarning">This is not production output. It never calculates weld allowance or sends lengths without an approved rule set.</p>
        <div class="form-grid"><div class="field"><label data-t="template">Template</label><select id="designTemplate" onchange="renderDesigner()"><option value="fixed">Fixed window</option><option value="sash">Sash</option><option value="door">Door</option><option value="sliding">Sliding</option></select></div><div class="field"><label data-t="projectQuantity">Project quantity</label><input id="designQuantity" type="number" min="1" max="999" value="1" oninput="renderDesigner()"></div></div>
        <div class="form-grid designer-meta"><div class="field"><label id="designProjectLabel" for="designProjectName">Project name</label><input id="designProjectName" maxlength="40" placeholder="Project name" oninput="renderDesigner()"></div><div class="field"><label id="designCustomerLabel" for="designCustomer">Customer</label><input id="designCustomer" maxlength="40" placeholder="Customer / order" oninput="renderDesigner()"></div><div class="field"><label id="designProfilePresetLabel">پروفیل / سری</label><select id="designProfilePreset" onchange="designerProfilePresetChanged()"><option value="custom">دستی / سایر (Custom)</option><option value="wintech60">وین‌تک (WinTech W640) - ۶۰mm</option><option value="wintech70">وین‌تک (WinTech W700) - ۷۰mm</option><option value="vistabest60">ویستابست (VistaBest 60) - ۳ کاناله</option><option value="vistabest70">ویستابست (VistaBest 70) - ۵ کاناله</option><option value="hofmann60">هافمن (Hofmann) - ۶۰mm</option><option value="syndece60">سیندژ (SynDece) - ۶۰mm</option><option value="atatech60">آتاتک (Atatech) - ۶۰mm</option><option value="enzo60">انزو / پلاس‌پن (Enzo) - ۶۰mm</option></select></div><div class="field"><label id="designProfileLabel" for="designProfile">نام سیستم پروفیل</label><input id="designProfile" maxlength="32" placeholder="Approved profile name" oninput="renderDesigner()"></div><div class="field"><label id="designGlassLabel" for="designGlass">Infill</label><select id="designGlass" onchange="renderDesigner()"><option value="none">Not specified</option><option value="single">Single glass</option><option value="double">Double glass</option><option value="panel">Panel</option></select></div><div class="field"><label id="designColorLabel" for="designColor">Color</label><select id="designColor" onchange="renderDesigner()"><option value="white">White</option><option value="anthracite">Anthracite</option><option value="wood">Wood finish</option><option value="black">Black</option></select></div></div>
        <div class="form-grid"><div class="field"><label data-t="widthMm">Width (mm)</label><input id="designWidth" type="number" min="100" max="10000" value="1200" onchange="designerCommitDimension('v')"></div><div class="field"><label data-t="heightMm">Height (mm)</label><input id="designHeight" type="number" min="100" max="10000" value="1400" onchange="designerCommitDimension('h')"></div><div class="field"><label data-t="material">Material</label><select id="designMaterial" onchange="renderDesigner()"><option value="upvc">uPVC</option><option value="aluminium">Aluminium</option><option value="other">Other</option></select></div></div>
        <div class="designer-tools"><button type="button" onclick="designerSplit('v')"><span data-ui-icon="split-v"></span><span data-t="splitVertical">Vertical split</span></button><button type="button" onclick="designerSplit('h')"><span data-ui-icon="split-h"></span><span data-t="splitHorizontal">Horizontal split</span></button><button type="button" onclick="designerReset()"><span data-ui-icon="reset"></span><span data-t="resetView">Reset view</span></button></div>
        <div class="designer-canvas" id="designerCanvas" aria-label="Draft window and door drawing"><div class="design-frame" id="designFrame"><div class="design-divider v" id="dividerV" hidden></div><div class="design-divider h" id="dividerH" hidden></div></div></div><div class="designer-kpis" id="designerKpis"></div>
<div class="card solid" style="margin-top:12px">
  <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:10px">
    <h3 style="margin:0" data-t="cutBreakdown">آنالیز و لیست برش پروفیل (MasterWin / iWinDoor)</h3>
    <div style="display:flex;gap:6px"><button type="button" class="primary compact gray" onclick="designerPrintLabels()">🏷️ چاپ لیبل قطعات</button><button type="button" class="primary compact" style="background:#34c759;color:#fff;border:0;font-weight:bold" onclick="designerApplyToCutList()" data-t="applyToCutList">انتقال مستقیم به لیست برش دستگاه ➔</button></div>
  </div>
  <div class="designer-cut-table" id="designerCutTableWrap">
    <table style="width:100%;border-collapse:collapse;text-align:center;font-size:12px">
      <thead><tr style="background:#f2f4f7"><th>#</th><th data-t="partType">نوع قطعه</th><th data-t="lengthMm">طول (mm)</th><th data-t="quantity">تعداد</th><th data-t="angle">زاویه</th></tr></thead>
      <tbody id="designerCutTbody"></tbody>
    </table>
  </div>
</div>
<p class="hint" id="designerSummary"></p>
      </div>
    </section>

    <section class="page" id="upd">
      <div class="head"><h2 data-t="update">به‌روزرسانی نرم‌افزار دستگاه</h2><button class="help-book" aria-label="Help for this page" onclick="helpOpen('upd')">📖</button></div>
      <div class="card solid">
        <p class="warning" data-t="updateWarning">برای ایمنی، عملیات به‌روزرسانی تنها در توقف کامل دستگاه و با رمز مدیر مجاز است. در زمان ارتقا برق دستگاه یا ارتباط شبکه را قطع نکنید.</p>

        <div class="field"><label data-t="adminPinOta">رمز مدیر دستگاه</label><input id="otapin" type="password" maxlength="12" inputmode="numeric" placeholder="رمز مدیر دستگاه را وارد کنید" autocomplete="off"></div>

        <div class="update-mode-tabs" style="display:flex;gap:8px;margin:14px 0 10px;flex-wrap:wrap">
          <button type="button" class="primary compact" id="tabLocalUpload" onclick="setUpdateTab('local')">📁 بارگذاری فایل از گوشی/کامپیوتر</button>
          <button type="button" class="primary compact gray" id="tabOnlineOta" onclick="setUpdateTab('online')">🌐 به‌روزرسانی آنلاین از سرور (OTA)</button>
        </div>

        <!-- Local File Upload Section -->
        <div id="localUploadSection" class="update-section">
          <div class="field" style="border:2px dashed #b8cddb;padding:14px;border-radius:10px;text-align:center;background:#f8fafc">
            <label style="font-weight:bold;margin-bottom:6px;display:block">انتخاب فایل Firmware با پسوند .bin</label>
            <input id="fw" type="file" accept=".bin" onchange="showFirmwareMeta()" style="display:block;margin:0 auto">
            <div class="hint" id="fwmeta" style="margin-top:6px;font-weight:bold;color:#087ad9"></div>
          </div>
          <button type="button" class="primary danger" style="width:100%;margin-top:10px;padding:10px;font-weight:bold" onclick="uploadAsk(this)" data-t="upload">بارگذاری و نصب فایل انتخابی</button>
        </div>

        <!-- Online Cloud / Server OTA Section -->
        <div id="onlineOtaSection" class="update-section" hidden>
          <div style="margin-bottom:10px">
            <b style="font-size:13px">انتخاب برنامه از سرور هافلر:</b>
            <div style="display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:8px">
              <div class="card ota-model-card active" id="otaCard0" onclick="selectOtaModel(0)" style="border:2px solid #087ad9;border-radius:10px;padding:10px;cursor:pointer;background:#f0f7ff">
                <b style="color:#087ad9">۱. اره تک‌سر برش (Cutter)</b>
                <p style="font-size:11px;color:#555;margin:4px 0">برنامه اصلی خط برش تک‌سر UPVC، لیست برش، متراژ و کنترل اره</p>
                <small id="otaVer0" style="color:#2a7e3b;font-weight:bold">بررسی نسخه...</small>
              </div>
              <div class="card ota-model-card" id="otaCard1" onclick="selectOtaModel(1)" style="border:2px solid #d9dde1;border-radius:10px;padding:10px;cursor:pointer;background:#fff">
                <b style="color:#b85f00">۲. کانفیگ سخت‌افزار (Config)</b>
                <p style="font-size:11px;color:#555;margin:4px 0">برنامه تست کارخانه‌ای موتور، درایور، سنسورها و تنظیمات مادربرد</p>
                <small id="otaVer1" style="color:#555;font-weight:bold">بررسی نسخه...</small>
              </div>
            </div>
          </div>

          <div class="field" style="margin-top:10px">
            <label>نشانی سرور دانلود یا لینک مستقیم (اختیاری):</label>
            <input id="customOtaUrl" type="text" placeholder="مثلاً: haffler.ir/ota یا http://192.168.1.50/firmware.bin" oninput="normalizeOtaInput(this)">
            <p class="hint" style="font-size:11px">می‌توانید لینک را با http://، https:// یا بدون پیشوند وارد کنید؛ سامانه خودکار آن را نرمال‌سازی می‌کند.</p>
          </div>

          <div class="actions two" style="margin-top:10px">
            <button type="button" class="primary compact blue" onclick="checkOnlineOtaVersion(this)">🔍 بررسی نسخه سرور</button>
            <button type="button" class="primary compact danger" onclick="startOnlineOtaInstall(this)">⬇ دانلود و نصب آنلاین</button>
          </div>
        </div>

        <progress id="prg" value="0" max="100" style="width:100%;margin-top:10px" hidden></progress>
        <div class="msg" id="fwmsg" style="margin-top:8px"></div>
      </div>
    </section>
  </main>
</div>

<div class="backdrop" id="menuBackdrop" onclick="menuClose(event)">
  <aside class="drawer" onclick="event.stopPropagation()">
    <div class="drawer-head"><b>CUTTER PLC</b><button class="x" aria-label="Close menu" onclick="menuClose()">×</button></div>
    <nav class="menu" id="menu">
      <button class="on" data-page="home" onclick="requestPage('home')" data-t="home">صفحه اصلی</button>
      <button data-page="f1" onclick="requestPage('f1')">F1 · <span data-t="general">تنظیمات عمومی</span></button>
      <button data-page="f2" onclick="requestPage('f2')">F2 · <span data-t="motor">تنظیمات موتور</span></button>
      <button data-page="f3" onclick="requestPage('f3')">F3 · <span data-t="ioStatus">وضعیت ورودی‌ها</span></button>
      <button data-page="f4" onclick="requestPage('f4')">F4 · <span data-t="admin">مدیریت</span></button>
      <button data-page="f5" onclick="requestPage('f5')">F5 · <span data-t="developer">اطلاعات دستگاه</span></button>
      <button data-page="list" onclick="requestPage('list')" data-t="cutList">لیست برش</button>
      <button data-page="plan" onclick="requestPage('plan')" data-t="optimizedPlan">لیست بهینه</button>
      <button data-page="designer" onclick="requestPage('designer')">▣ <span data-t="designer">Window / door designer</span></button>
      <button data-page="upd" onclick="requestPage('upd')" data-t="update">به‌روزرسانی</button>
      <button onclick="helpOpen('all')" data-t="help">راهنما</button>
    </nav>
  </aside>
</div>

<div class="modal-wrap" id="helpModal" role="dialog" aria-modal="true" aria-labelledby="helpTitle"><div class="modal"><div class="modal-head"><h3 id="helpTitle" data-t="help">راهنما</h3><button class="x" aria-label="Close" onclick="modalClose('helpModal')">×</button></div><div class="help-body" id="helpBody"></div></div></div>
<div class="modal-wrap" id="confirmModal" role="dialog" aria-modal="true" aria-labelledby="confirmTitle"><div class="modal"><div class="modal-head"><h3 id="confirmTitle" data-t="confirm">تأیید</h3><button class="x" aria-label="Close" onclick="modalClose('confirmModal')">×</button></div><p id="confirmText"></p><div class="actions"><button class="primary danger" id="confirmYes" data-t="continue">ادامه</button><button class="primary gray" onclick="modalClose('confirmModal')" data-t="cancel">انصراف</button></div></div></div>
<div class="toast" id="toast"></div>

<script>
const LIST_MAX=100,$=id=>document.getElementById(id);
const UI_ICONS={
 info:'<circle cx="12" cy="12" r="9"/><path d="M12 10v6M12 7h.01"/>',
 warning:'<path d="M12 3 2.8 20h18.4L12 3Z"/><path d="M12 9v5M12 17h.01"/>',
 check:'<path d="m4 12 5 5L20 6"/>',help:'<circle cx="12" cy="12" r="9"/><path d="M9.8 9a2.4 2.4 0 1 1 3.5 2.1c-.9.5-1.3 1.1-1.3 2M12 17h.01"/>',
 status:'<path d="M4 15a8 8 0 0 1 16 0"/><path d="m12 15 4-5"/><circle cx="12" cy="15" r="1.5"/>',
 search:'<circle cx="10.5" cy="10.5" r="6.5"/><path d="m15.5 15.5 4.5 4.5"/>',report:'<path d="M6 3h9l3 3v15H6z"/><path d="M15 3v4h4M9 11h6M9 15h6"/>',
 motion:'<path d="M4 12h16M7 8l-4 4 4 4M17 8l4 4-4 4"/>',cut:'<circle cx="7" cy="17" r="3"/><circle cx="17" cy="17" r="3"/><path d="m9 15 8-11M15 15 7 4"/>',
 sensor:'<circle cx="12" cy="12" r="2"/><path d="M8.5 8.5a5 5 0 0 0 0 7M15.5 8.5a5 5 0 0 1 0 7M5.5 5.5a9 9 0 0 0 0 13M18.5 5.5a9 9 0 0 1 0 13"/>',
 input:'<path d="M13 5h7v14h-7M3 12h12M11 8l4 4-4 4"/>',output:'<path d="M11 5H4v14h7M9 12h12M17 8l4 4-4 4"/>',
 guard:'<path d="M6 21V4h12v17M9 8h6M9 12h6M9 16h6"/>',stop:'<path d="M8 3h8l5 5v8l-5 5H8l-5-5V8z"/><path d="M9 8v8M15 8v8"/>',
 blade:'<circle cx="12" cy="12" r="8"/><circle cx="12" cy="12" r="2"/><path d="m12 4 2 3M20 12l-3 2M12 20l-2-3M4 12l3-2"/>',fault:'<path d="M12 3 2.8 20h18.4L12 3Z"/><path d="m9 10 6 6M15 10l-6 6"/>',
 button:'<rect x="5" y="7" width="14" height="12" rx="3"/><path d="M9 7V4h6v3M9 13h6"/>',joystick:'<path d="M12 13V5M9 5h6M7 13h10l3 7H4z"/>',
 pulse:'<path d="M3 13h4l2-7 4 13 2-6h6"/>',direction:'<path d="M4 12h16M15 7l5 5-5 5"/>',power:'<path d="M12 3v9M7 6.5a8 8 0 1 0 10 0"/>',
 clamp:'<path d="M5 5v14M19 5v14M5 9h5v6H5M19 9h-5v6h5"/>',spray:'<path d="M4 14h6l3-4v8l-3-4M16 9l4-2M16 13h5M16 17l4 2"/>',lamp:'<path d="M9 18h6M10 21h4M8 14a6 6 0 1 1 8 0c-1 1-1 2-1 2H9s0-1-1-2Z"/>',
 lift:'<path d="M4 18h16M12 17V5M8 9l4-4 4 4"/>',reserved:'<rect x="4" y="4" width="16" height="16" rx="3"/><path d="M8 12h8"/>',
 'split-v':'<rect x="4" y="4" width="16" height="16" rx="2"/><path d="M12 4v16"/>','split-h':'<rect x="4" y="4" width="16" height="16" rx="2"/><path d="M4 12h16"/>',
 reset:'<path d="M4 10a8 8 0 1 1 2 7M4 4v6h6"/>',trash:'<path d="M4 7h16M9 7V4h6v3M7 7l1 14h8l1-14M10 11v6M14 11v6"/>',
 window:'<rect x="3" y="4" width="18" height="16" rx="2"/><path d="M12 4v16M3 12h18"/>',download:'<path d="M12 3v12M7 10l5 5 5-5M4 20h16"/>',open:'<path d="M4 5h7M4 5v15h15v-7M13 4h7v7M20 4l-9 9"/>',detail:'<path d="M5 7h14M5 12h14M5 17h9"/>',simple:'<path d="M5 8h14M5 13h10M5 18h6"/>'
};
function uiIcon(name,cls=''){return`<svg class="ui-icon ${cls}" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true">${UI_ICONS[name]||UI_ICONS.info}</svg>`}
function hydrateIcons(root=document){root.querySelectorAll('[data-ui-icon]').forEach(e=>{e.innerHTML=uiIcon(e.dataset.uiIcon);e.removeAttribute('data-ui-icon')})}
function decorateGeneralIcons(){[['#home .hero-actions button:nth-child(1)','cut'],['#home .hero-actions button:nth-child(2)','window'],['#home .hero-actions button:nth-child(3)','report'],['#screenDownloadButton','download']].forEach(([sel,icon])=>{const e=document.querySelector(sel),slot=e?.querySelector('[aria-hidden="true"]');if(slot)slot.innerHTML=uiIcon(icon)});const live=document.querySelectorAll('#home .live-row .ico');if(live[0])live[0].innerHTML=uiIcon('report','lg');if(live[1])live[1].innerHTML=uiIcon('warning','lg');document.querySelectorAll('.help-book').forEach(e=>e.innerHTML=uiIcon('help'))}
function htmlText(value){return String(value??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]))}
let webLanguage=localStorage.getItem('cutterLang')||'fa',activePage='home',syncBusy=false,listBuilt=false,listLoaded=false,homeListLoaded=false,toastTimer=0,lastState=null,importPreview=null,planSnapshot=null,modalReturnFocus=null,confirmAction=null,lastSyncOk=0,listVisibleRows=5,localPlan=false,lastSaveAt='';
const tr={
fa:{current:'موقعیت فعلی',destination:'مقصد',activeRow:'ردیف فعال',remaining:'تعداد باقی‌مانده',cuts:'برش‌های کاربر',openList:'باز کردن لیست برش',general:'تنظیمات عمومی',stock:'طول شاخه',kerf:'عرض تیغه',trim:'پرت ابتدا',deviceLang:'زبان دستگاه',keyboard:'کیبورد لمسی',buzzer:'صدای کلید',estop:'نوع ورودی توقف',save:'ذخیره',motor:'تنظیمات موتور',lineStart:'ابتدای خط',lineEnd:'انتهای خط',cutSpeed:'سرعت حرکت',autoRamp:'شیب حرکت',jogSpeed:'سرعت جاگ',jogRamp:'شیب جاگ',jogStart:'راه‌اندازی نرم جاگ',ioStatus:'وضعیت ورودی‌ها',ioSafe:'نمایش زنده است. برای ایمنی، فرمان خروجی و حرکت فقط از خود دستگاه انجام می‌شود.',admin:'مدیریت',warmup:'زمان آماده‌سازی اره',baseDir:'جهت پایه موتور',dayLock:'روز باقی‌مانده قفل',adminPin:'رمز مدیر',newAdminPin:'رمز مدیر جدید',developer:'اطلاعات دستگاه',lifeCuts:'کل برش‌ها',userCuts:'برش‌های کاربر',freeRam:'حافظه آزاد',uptime:'زمان روشن بودن',resetCuts:'صفر کردن شمارنده کاربر',cutList:'لیست برش',visibleRows:'تعداد ردیف فعال',importFile:'ورود فایل متنی یا CSV',readFile:'خواندن فایل',length:'متراژ',count:'تعداد',saveList:'ذخیره لیست',update:'به‌روزرسانی',updateHelp:'فقط فایل firmware همین مدل را انتخاب کنید. دستگاه باید کاملاً متوقف باشد.',upload:'بارگذاری و نصب',adminPinOta:'رمز مدیر دستگاه',enterAdminPin:'ابتدا رمز مدیر دستگاه را وارد کنید',home:'صفحه اصلی',help:'راهنما',confirm:'تأیید',continue:'ادامه',cancel:'انصراف',connecting:'در حال اتصال',online:'متصل',offline:'ارتباط قطع است',idle:'آماده',moving:'در حال حرکت',cutting:'در حال برش',saw:'اره روشن',stopped:'توقف فعال',referenced:'موقعیت معتبر',notReferenced:'موقعیت نامعتبر',manual:'دستی',listMode:'ردیف',optimized:'بهینه',jog:'جاگ',saved:'ذخیره شد',busy:'دستگاه در حال کار است',failed:'عملیات انجام نشد',uploadAsk:'نصب firmware شروع شود؟ تا پایان کار برق دستگاه را قطع نکنید.',noFile:'ابتدا فایل را انتخاب کنید',loaded:'ردیف از فایل خوانده شد'},
en:{current:'Current position',destination:'Destination',activeRow:'Active row',remaining:'Remaining count',cuts:'User cuts',openList:'Open cut list',general:'General settings',stock:'Stock length',kerf:'Blade kerf',trim:'Start trim',deviceLang:'Device language',keyboard:'Touch keyboard',buzzer:'Key sound',estop:'Stop input type',save:'Save',motor:'Motor settings',lineStart:'Start of line',lineEnd:'End of line',cutSpeed:'Travel speed',autoRamp:'Travel ramp',jogSpeed:'Jog speed',jogRamp:'Jog ramp',jogStart:'Jog soft start',ioStatus:'Input status',ioSafe:'Live display. For safety, outputs and motion are controlled only on the machine.',admin:'Administration',warmup:'Saw warm-up time',baseDir:'Motor base direction',dayLock:'Day-lock remaining',adminPin:'Admin PIN',newAdminPin:'New admin PIN',developer:'Device information',lifeCuts:'Lifetime cuts',userCuts:'User cuts',freeRam:'Free memory',uptime:'Uptime',resetCuts:'Reset user counter',cutList:'Cut list',visibleRows:'Active rows',importFile:'Import text or CSV',readFile:'Read file',length:'Length',count:'Count',saveList:'Save cut list',update:'Update',updateHelp:'Select firmware for this model only. The machine must be fully stopped.',upload:'Upload and install',adminPinOta:'Device manager PIN',enterAdminPin:'Enter the device manager PIN first',home:'Home',help:'Help',confirm:'Confirm',continue:'Continue',cancel:'Cancel',connecting:'Connecting',online:'Connected',offline:'Disconnected',idle:'Ready',moving:'Moving',cutting:'Cutting',saw:'Saw on',stopped:'Stop active',referenced:'Position valid',notReferenced:'Position invalid',manual:'Manual',listMode:'Row',optimized:'Optimized',jog:'Jog',saved:'Saved',busy:'Machine is busy',failed:'Operation failed',uploadAsk:'Start firmware installation? Do not remove power until it finishes.',noFile:'Select a file first',loaded:'rows loaded from file'}
};
Object.assign(tr.fa,{reviewFile:'بررسی فایل',exportList:'دانلود لیست',replaceOnForm:'جایگزینی روی فرم',optimizedPlan:'لیست بهینه',planReadOnly:'این صفحه فقط نتیجهٔ آمادهٔ دستگاه را نمایش می‌دهد و هیچ فرمان حرکت یا بهینه‌سازی ارسال نمی‌کند.',bars:'شاخه‌ها',pieces:'قطعات',waste:'پرت',refresh:'بازخوانی',downloadPlan:'دانلود نتیجه',dirty:'تغییرات ذخیره‌نشده',offlineStale:'ارتباط قطع است؛ داده‌ها ممکن است قدیمی باشند',unsavedAsk:'تغییرات این صفحه ذخیره نشده است. بدون ذخیره خارج شوید؟',discard:'خروج بدون ذخیره',validRows:'ردیف معتبر',rejectedRows:'ردیف ردشده',fileTooLarge:'حجم فایل بیشتر از 256KB است',invalidFormat:'ساختار فایل معتبر نیست',importNeedsSync:'ابتدا اتصال و محدودهٔ دستگاه را بازخوانی کنید',importReady:'فایل بررسی شد؛ هنوز چیزی در دستگاه ذخیره نشده است',importApplied:'ردیف‌ها روی فرم جایگزین شدند؛ برای ارسال به دستگاه ذخیره را بزنید',planUnavailable:'نتیجهٔ بهینه آماده یا در دسترس نیست',planPending:'محاسبه هنوز کامل نشده است',saving:'در حال ذخیره',firmwareFile:'فایل انتخابی',downloaded:'فایل دانلود شد'});
Object.assign(tr.en,{reviewFile:'Review file',exportList:'Download list',replaceOnForm:'Replace form rows',optimizedPlan:'Optimized plan',planReadOnly:'Read-only review. This page never starts optimization, motion or cutting.',bars:'Bars',pieces:'Pieces',waste:'Waste',refresh:'Refresh',downloadPlan:'Download result',dirty:'Unsaved changes',offlineStale:'Disconnected; displayed data may be stale',unsavedAsk:'This page has unsaved changes. Leave without saving?',discard:'Discard and leave',validRows:'valid rows',rejectedRows:'rejected rows',fileTooLarge:'File is larger than 256KB',invalidFormat:'File structure is not valid',importNeedsSync:'Reconnect and reload the device limits first',importReady:'Preflight passed; nothing has been saved to the device yet',importApplied:'Rows replaced on the form; press Save to send them to the device',planUnavailable:'No completed optimized plan is available',planPending:'Optimization is not complete yet',saving:'Saving',firmwareFile:'Selected file',downloaded:'File downloaded'});
Object.assign(tr.fa,{screenDownload:'دانلود عکس صفحه نمایش',screenRequest:'درخواست عکس ارسال شد',wrongAdminPin:'رمز مدیر LCD اشتباه است',preparingExport:'فایل در حال آماده شدن است',downloaded:'فایل آماده دانلود شد'});
Object.assign(tr.en,{screenDownload:'Download display screenshot',screenRequest:'Screenshot request sent',wrongAdminPin:'Wrong LCD admin PIN',preparingExport:'Preparing file',downloaded:'File is ready to download'});
Object.assign(tr.en,{designer:'Window / door designer',reports:'Reports',lastSave:'Last save',addRow:'Add row',optimize:'Optimize cutting',originalList:'Original list',export:'Export',template:'Template',projectQuantity:'Project quantity',splitVertical:'Vertical split',splitHorizontal:'Horizontal split',resetView:'Reset view',advanced:'Advanced',stopWarning:'Change stop wiring only after a safety check and confirmation.',advancedWarning:'Manager-only controls. Incorrect values can alter manual machine behavior.',designerWarning:'Draft only — no uPVC weld allowance, cut length or device command is generated without an approved rule set.',planLocal:'Local preview only',planNoFit:'Some pieces do not fit the available stock after trim and blade kerf.',exportCsv:'CSV UTF-8',exportTxt:'TXT',exportPrint:'Print preview',exportHelp:'Browser-only exports: CSV UTF-8, TXT and print. XLSX/PDF/PNG need an approved dependency.',optimizedLocal:'Local cut preview',savedNow:'Saved just now'});
Object.assign(tr.fa,{designer:'\u0637\u0631\u0627\u062d \u067e\u0646\u062c\u0631\u0647 \u0648 \u062f\u0631\u0628',reports:'\u06af\u0632\u0627\u0631\u0634\u200c\u0647\u0627',lastSave:'\u0622\u062e\u0631\u06cc\u0646 \u0630\u062e\u06cc\u0631\u0647',addRow:'\u0627\u0641\u0632\u0648\u062f\u0646 \u0631\u062f\u06cc\u0641',optimize:'\u0628\u0647\u06cc\u0646\u0647\u200c\u0633\u0627\u0632\u06cc \u0628\u0631\u0634',originalList:'\u0644\u06cc\u0633\u062a \u0627\u0635\u0644\u06cc',export:'\u062e\u0631\u0648\u062c\u06cc',template:'\u0627\u0644\u06af\u0648',projectQuantity:'\u062a\u0639\u062f\u0627\u062f \u067e\u0631\u0648\u0698\u0647',splitVertical:'\u062a\u0642\u0633\u06cc\u0645 \u0639\u0645\u0648\u062f\u06cc',splitHorizontal:'\u062a\u0642\u0633\u06cc\u0645 \u0627\u0641\u0642\u06cc',resetView:'\u0628\u0627\u0632\u0646\u0634\u0627\u0646\u06cc \u0646\u0645\u0627',advanced:'\u067e\u06cc\u0634\u0631\u0641\u062a\u0647',stopWarning:'\u062a\u063a\u06cc\u06cc\u0631 \u0633\u06cc\u0645\u200c\u06a9\u0634\u06cc \u062a\u0648\u0642\u0641 \u0641\u0642\u0637 \u067e\u0633 \u0627\u0632 \u0628\u0631\u0631\u0633\u06cc \u0627\u06cc\u0645\u0646\u06cc \u0648 \u062a\u0623\u06cc\u06cc\u062f.',advancedWarning:'\u062a\u0646\u0638\u06cc\u0645\u0627\u062a \u0648\u06cc\u0698\u0647 \u0645\u062f\u06cc\u0631; \u0645\u0642\u062f\u0627\u0631 \u0646\u0627\u062f\u0631\u0633\u062a \u0645\u06cc\u200c\u062a\u0648\u0627\u0646\u062f \u0631\u0641\u062a\u0627\u0631 \u062d\u0631\u06a9\u062a \u062f\u0633\u062a\u06cc \u0631\u0627 \u062a\u063a\u06cc\u06cc\u0631 \u062f\u0647\u062f.',designerWarning:'\u067e\u06cc\u0634\u200c\u0646\u0648\u06cc\u0633 \u063a\u06cc\u0631\u062a\u0648\u0644\u06cc\u062f\u06cc \u0627\u0633\u062a؛ \u0628\u062f\u0648\u0646 Rule Set \u062a\u0623\u06cc\u06cc\u062f\u0634\u062f\u0647\u060c \u067e\u0631\u062a \u062c\u0648\u0634\u060c \u0637\u0648\u0644 \u0628\u0631\u0634 \u06cc\u0627 \u0641\u0631\u0645\u0627\u0646 \u062f\u0633\u062a\u06af\u0627\u0647 \u0633\u0627\u062e\u062a\u0647 \u0646\u0645\u06cc\u200c\u0634\u0648\u062f.',planLocal:'\u067e\u06cc\u0634\u200c\u0646\u0645\u0627\u06cc\u0634 \u0645\u062d\u0644\u06cc',planNoFit:'\u0628\u0631\u062e\u06cc \u0642\u0637\u0639\u0627\u062a \u0628\u0627 \u0627\u062d\u062a\u0633\u0627\u0628 \u067e\u0631\u062a \u0627\u0628\u062a\u062f\u0627 \u0648 \u0639\u0631\u0636 \u062a\u06cc\u063a\u0647 \u062f\u0631 \u0634\u0627\u062e\u0647 \u062c\u0627 \u0646\u0645\u06cc\u200c\u06af\u06cc\u0631\u0646\u062f.',exportCsv:'CSV UTF-8',exportTxt:'TXT',exportPrint:'\u067e\u06cc\u0634\u200c\u0646\u0645\u0627\u06cc\u0634 \u0686\u0627\u067e',exportHelp:'\u062e\u0631\u0648\u062c\u06cc \u0628\u062f\u0648\u0646 \u0648\u0627\u0628\u0633\u062a\u06af\u06cc: CSV UTF-8\u060c TXT \u0648 \u0686\u0627\u067e. XLSX/PDF/PNG \u0628\u0647 \u062a\u0623\u06cc\u06cc\u062f \u0648\u0627\u0628\u0633\u062a\u06af\u06cc \u0646\u06cc\u0627\u0632 \u062f\u0627\u0631\u0646\u062f.',optimizedLocal:'\u067e\u06cc\u0634\u200c\u0646\u0645\u0627\u06cc\u0634 \u0645\u062d\u0644\u06cc \u0628\u0631\u0634',savedNow:'\u0647\u0645\u06cc\u0646 \u0627\u0644\u0627\u0646 \u0630\u062e\u06cc\u0631\u0647 \u0634\u062f'});
const T=k=>(tr[webLanguage]&&tr[webLanguage][k])||k;
Object.assign(tr.en,{kerf:'Blade kerf per cut (cut width)',trim:'Stock start trim',estop:'Stop input wiring (NC/NO)'});
Object.assign(tr.fa,{kerf:'\u0639\u0631\u0636 \u062a\u06cc\u063a\u0647 \u062f\u0631 \u0647\u0631 \u0628\u0631\u0634',trim:'\u067e\u0631\u062a \u0627\u0628\u062a\u062f\u0627\u06cc \u0634\u0627\u062e\u0647',estop:'\u0633\u06cc\u0645\u200c\u06a9\u0634\u06cc \u0648\u0631\u0648\u062f\u06cc \u062a\u0648\u0642\u0641 (NC/NO)'});
Object.assign(tr.en,{invalidManualRows:'Invalid manual rows',exportPng:'PNG',exportPrint:'Print / Save as PDF',widthMm:'Width (mm)',heightMm:'Height (mm)',material:'Material',draftMembers:'Draft members'});
Object.assign(tr.fa,{invalidManualRows:'\u0631\u062f\u06cc\u0641\u200c\u0647\u0627\u06cc \u062f\u0633\u062a\u06cc \u0646\u0627\u0645\u0639\u062a\u0628\u0631',exportPng:'PNG',exportPrint:'\u0686\u0627\u067e / \u0630\u062e\u06cc\u0631\u0647 PDF',widthMm:'\u0639\u0631\u0636 (mm)',heightMm:'\u0627\u0631\u062a\u0641\u0627\u0639 (mm)',material:'\u062c\u0646\u0633',draftMembers:'\u062a\u0639\u062f\u0627\u062f \u0639\u0636\u0648 \u067e\u06cc\u0634\u200c\u0646\u0648\u06cc\u0633'});
Object.assign(tr.en,{emptyPlan:'Not calculated',unsaved:'Not saved',templatefixed:'Fixed window',templatesash:'Sash',templatedoor:'Door',templatesliding:'Sliding',materialupvc:'uPVC',materialaluminium:'Aluminium',materialother:'Other',stopBaseWarning:'Confirm stop wiring and motor base direction change.',baseWarning:'Confirm motor base direction change.'});
Object.assign(tr.fa,{emptyPlan:'\u0645\u062d\u0627\u0633\u0628\u0647 \u0646\u0634\u062f\u0647',unsaved:'\u0647\u0646\u0648\u0632 \u0630\u062e\u06cc\u0631\u0647 \u0646\u0634\u062f\u0647',templatefixed:'\u067e\u0646\u062c\u0631\u0647 \u062b\u0627\u0628\u062a',templatesash:'\u067e\u0646\u062c\u0631\u0647 \u0628\u0627\u0632\u0634\u0648',templatedoor:'\u062f\u0631\u0628',templatesliding:'\u06a9\u0634\u0648\u06cc\u06cc',materialupvc:'uPVC',materialaluminium:'\u0622\u0644\u0648\u0645\u06cc\u0646\u06cc\u0648\u0645',materialother:'\u0633\u0627\u06cc\u0631',stopBaseWarning:'\u062a\u063a\u06cc\u06cc\u0631 \u0633\u06cc\u0645\u200c\u06a9\u0634\u06cc \u062a\u0648\u0642\u0641 \u0648 \u062c\u0647\u062a \u067e\u0627\u06cc\u0647 \u0645\u0648\u062a\u0648\u0631 \u0631\u0627 \u062a\u0623\u06cc\u06cc\u062f \u06a9\u0646\u06cc\u062f.',baseWarning:'\u062a\u063a\u06cc\u06cc\u0631 \u062c\u0647\u062a \u067e\u0627\u06cc\u0647 \u0645\u0648\u062a\u0648\u0631 \u0631\u0627 \u062a\u0623\u06cc\u06cc\u062f \u06a9\u0646\u06cc\u062f.'});
Object.assign(tr.en,{projectQuantity:'Count of this design/window'});Object.assign(tr.fa,{projectQuantity:'\u062a\u0639\u062f\u0627\u062f \u0627\u06cc\u0646 \u0637\u0631\u062d / \u067e\u0646\u062c\u0631\u0647'});
Object.assign(tr.en,{exportHelp:'Available in this browser: CSV UTF-8, TXT, PNG, and Print / Save as PDF. XLSX remains blocked until an approved dependency is selected.',planReadOnly:'Review either a local calculation preview or a completed device result. This page never starts optimization, motion or cutting.',utilization:'Utilization',unallocatedNone:'All pieces allocated',unallocatedPieces:'Unallocated pieces'});
Object.assign(tr.fa,{exportHelp:'\u062e\u0631\u0648\u062c\u06cc\u200c\u0647\u0627\u06cc \u0641\u0639\u0627\u0644 \u062f\u0631 \u0645\u0631\u0648\u0631\u06af\u0631: CSV UTF-8\u060c TXT\u060c PNG \u0648 \u0686\u0627\u067e / \u0630\u062e\u06cc\u0631\u0647 PDF. \u0641\u0642\u0637 XLSX \u062a\u0627 \u0627\u0646\u062a\u062e\u0627\u0628 \u0648\u0627\u0628\u0633\u062a\u06af\u06cc \u062a\u0623\u06cc\u06cc\u062f\u0634\u062f\u0647 \u0645\u0633\u062f\u0648\u062f \u0627\u0633\u062a.',planReadOnly:'\u067e\u06cc\u0634\u200c\u0646\u0645\u0627\u06cc\u0634 \u0645\u062d\u0627\u0633\u0628\u0647 \u0645\u062d\u0644\u06cc \u06cc\u0627 \u0646\u062a\u06cc\u062c\u0647 \u06a9\u0627\u0645\u0644 \u062f\u0633\u062a\u06af\u0627\u0647 \u0631\u0627 \u0628\u0631\u0631\u0633\u06cc \u06a9\u0646\u06cc\u062f. \u0627\u06cc\u0646 \u0635\u0641\u062d\u0647 \u0647\u06cc\u0686 \u0628\u0647\u06cc\u0646\u0647\u200c\u0633\u0627\u0632\u06cc\u060c \u062d\u0631\u06a9\u062a \u06cc\u0627 \u0628\u0631\u0634\u06cc \u0634\u0631\u0648\u0639 \u0646\u0645\u06cc\u200c\u06a9\u0646\u062f.',utilization:'\u0628\u0647\u0631\u0647\u200c\u0648\u0631\u06cc',unallocatedNone:'\u0647\u0645\u0647 \u0642\u0637\u0639\u0627\u062a \u062a\u062e\u0635\u06cc\u0635 \u06cc\u0627\u0641\u062a\u0647\u200c\u0627\u0646\u062f',unallocatedPieces:'\u0642\u0637\u0639\u0627\u062a \u062a\u062e\u0635\u06cc\u0635\u200c\u0646\u06cc\u0627\u0641\u062a\u0647'});
function syncRange(id,value){const n=$(id);if(n){n.value=value;n.dataset.dirty='1'}updateStatebar()}
function syncNumber(id,value){const r=$(id);if(r)r.value=value;updateStatebar()}
function listItems(){buildList();const items=[],invalid=[],min=Number(lastState?.startmm),max=Number(lastState?.maxlen),hasLimits=Number.isFinite(min)&&Number.isFinite(max)&&max>=min;for(let i=0;i<LIST_MAX;i++){const rawLen=$('sp'+i).value.trim(),rawCnt=$('cnt'+i).value.trim(),length=Number(rawLen),count=Number(rawCnt);if((!rawLen&&!rawCnt)||(Number.isFinite(length)&&Number.isFinite(count)&&length===0&&count===0))continue;const lengthOk=Number.isFinite(length)&&length>0&&(!hasLimits||(length>=min&&length<=max));if(lengthOk&&Number.isInteger(count)&&count>=1&&count<=999)items.push({length,count});else invalid.push(i+1)}return{items,invalid,limits:hasLimits?{min,max}:null}}
function updateListRows(){const body=$('listtbl').tBodies[0];for(let i=0;i<LIST_MAX;i++){const trr=body.rows[i];if(trr)trr.classList.toggle('list-derived',i>=listVisibleRows)}const info=$('listRowCount');if(info)info.textContent=listItems().items.length;const rows=$('rows');if(rows)rows.value=listItems().items.length;const wrap=$('tableWrap');if(wrap)wrap.classList.toggle('empty',!listVisibleRows);if($('homeListRows'))$('homeListRows').textContent=listItems().items.length}
function optimizeLocal(){const data=normalizeList(),state=lastState||{},stock=Number(state.stock),trim=Number(state.trim),kerf=Number(state.kerf);if(data.invalid.length){setMsg('listmsg',`${T('invalidManualRows')}: ${data.invalid.join(', ')}`,false);return}if(!data.items.length||!Number.isFinite(stock)||stock<=0||!Number.isFinite(trim)||trim<0||!Number.isFinite(kerf)||kerf<0){setMsg('listmsg',T('importNeedsSync'),false);return}const pieces=[];data.items.forEach(x=>{for(let c=0;c<x.count&&pieces.length<5000;c++)pieces.push(x.length)});if(pieces.length>=5000){setMsg('listmsg','Maximum local preview: 5000 pieces',false);return}pieces.sort((a,b)=>b-a);const usable=stock-trim,bars=[],unallocated=[];pieces.forEach(piece=>{const need=piece+kerf;let selected=null,bestRemaining=Infinity;for(const bar of bars){const remaining=usable-(bar.used+need);if(remaining>=-.0001&&remaining<bestRemaining){selected=bar;bestRemaining=remaining}}if(!selected){if(need>usable){unallocated.push(piece);return}selected={bar:bars.length+1,pieces_mm:[],used:0};bars.push(selected)}selected.used+=need;selected.pieces_mm.push(piece)});bars.forEach(b=>b.waste_mm=Math.max(0,usable-b.used));planSnapshot={complete:true,local:true,plan_id:'local-preview',stock_mm:stock,trim_mm:trim,kerf_mm:kerf,bars,total_waste_mm:bars.reduce((n,b)=>n+b.waste_mm,0),unallocated};localPlan=true;renderPlan();updateHomeSummary();showPage('plan');setMsg('planmsg',unallocated.length?T('planNoFit'):T('planLocal'),!unallocated.length)}
function showOriginalList(){showPage('list')}
function openExportChooser(){if(!planSnapshot?.complete)return;let box=$('exportModal');if(!box){box=document.createElement('div');box.className='modal-wrap';box.id='exportModal';box.setAttribute('role','dialog');box.setAttribute('aria-modal','true');box.innerHTML='<div class="modal"><div class="modal-head"><h3>▤ <span data-t="export">Export</span></h3><button class="x" aria-label="Close" onclick="modalClose(\'exportModal\')">×</button></div><p class="hint" data-t="exportHelp"></p><div class="export-grid"><button onclick="downloadPlan()" data-t="exportCsv">CSV UTF-8</button><button onclick="downloadPlanText()" data-t="exportTxt">TXT</button><button onclick="downloadPlanPng()" data-t="exportPng">PNG</button><button onclick="printPlan()" data-t="exportPrint">Print / Save as PDF</button></div></div>';document.body.appendChild(box);setWebLang(webLanguage)}modalOpen('exportModal')}
function downloadPlanText(){const d=planSnapshot;if(!d?.complete)return;const out=[`Plan: ${d.plan_id}`,`Stock: ${d.stock_mm} mm`,`Start trim: ${d.trim_mm} mm`,`Blade kerf: ${d.kerf_mm} mm`];(d.bars||[]).forEach((b,i)=>out.push(`Bar ${b.bar||i+1}: ${(b.pieces_mm||[]).join(' + ')} | waste ${Number(b.waste_mm||0).toFixed(1)} mm`));downloadBlob('cutter-local-plan.txt','text/plain;charset=utf-8',out.join('\r\n')+'\r\n')}
function downloadPlanPng(){const d=planSnapshot;if(!d?.complete)return;const target=beginAsyncDownload(),bars=(d.bars||[]).slice(0,120),rowH=30,w=1200,h=Math.min(4096,150+bars.length*rowH),c=document.createElement('canvas'),x=c.getContext('2d');c.width=w;c.height=h;x.fillStyle='#fff';x.fillRect(0,0,w,h);x.fillStyle='#17191c';x.font='bold 24px sans-serif';x.fillText(`Cut plan — ${d.local?'local preview':'device result'}`,36,42);x.font='16px sans-serif';x.fillText(`Stock ${d.stock_mm} mm   Start trim ${d.trim_mm} mm   Blade kerf ${d.kerf_mm} mm`,36,72);x.fillStyle='#e9edf1';x.fillRect(30,92,w-60,28);x.fillStyle='#17191c';x.font='bold 15px sans-serif';x.fillText('Bar',42,112);x.fillText('Pieces (mm)',150,112);x.fillText('Waste (mm)',w-180,112);x.font='15px sans-serif';bars.forEach((b,i)=>{const y=140+i*rowH;x.strokeStyle='#d9dde1';x.strokeRect(30,y-18,w-60,rowH);x.fillStyle='#17191c';x.fillText(String(b.bar||i+1),42,y+2);x.fillText((b.pieces_mm||[]).join(' + ').slice(0,110),150,y+2);x.fillText(Number(b.waste_mm||0).toFixed(1),w-180,y+2)});if((d.bars||[]).length>bars.length){x.fillStyle='#694d00';x.fillText(`Preview limited to first ${bars.length} bars`,36,h-20)}c.toBlob(blob=>deliverBlob('cutter-plan.png',blob,target),'image/png')}
function updateHomeSummary(){if($('homeListRows')&&!listBuilt)$('homeListRows').textContent='0';if($('homePlanState'))$('homePlanState').textContent=planSnapshot?.complete?(localPlan?T('planLocal'):T('optimizedPlan')):T('emptyPlan');if($('homeSaveState'))$('homeSaveState').textContent=lastSaveAt||T('unsaved');if(listBuilt)updateListRows()}
function localizeDesignerChoices(){const templates=$('designTemplate'),materials=$('designMaterial');if(templates)[...templates.options].forEach(o=>o.textContent=T('template'+o.value));if(materials)[...materials.options].forEach(o=>o.textContent=T('material'+o.value));if($('designQuantity'))$('designQuantity').previousElementSibling.textContent=T('projectQuantity')}
function enhanceWebApp(){document.querySelectorAll('.head').forEach(h=>{if(h.closest('#home'))return;if(!h.querySelector('.back-page')){const b=document.createElement('button');b.className='back-page';b.type='button';b.setAttribute('aria-label','Back');b.onclick=()=>requestPage('home');h.prepend(b)}});document.querySelectorAll('.head button').forEach(b=>{if(b.textContent.trim()==='?'){b.textContent='📖';b.classList.add('help-book');b.setAttribute('aria-label','Help for this page')}});const rows=$('rows');if(rows?.parentElement)rows.parentElement.hidden=true;const listHead=document.querySelector('#list .head');if(listHead&&!$('localOptimizeButton')){const b=document.createElement('button');b.id='localOptimizeButton';b.className='primary compact blue';b.type='button';b.innerHTML='▦ <span data-t="optimize">Optimize cutting</span>';b.onclick=optimizeLocal;listHead.after(b)}const table=$('tableWrap');if(table&&!$('listTools')){const tools=document.createElement('div');tools.id='listTools';tools.className='list-toolbar';tools.innerHTML='<p class="hint"><span id="listRowCount">0</span>/100 <span data-t="validRows">valid rows</span></p><button class="primary compact" type="button" onclick="addListRow()">＋ <span data-t="addRow">Add row</span></button>';table.after(tools)}const summary=document.querySelector('#plan .plan-summary');if(summary&&!$('planUtil')){const kpi=document.createElement('div');kpi.className='plan-kpi';kpi.innerHTML='<b id="planUtil">--</b><span data-t="utilization">Utilization</span>';summary.appendChild(kpi)}const meta=$('planMeta');if(meta&&!$('planUnallocated')){const note=document.createElement('div');note.id='planUnallocated';note.className='plan-note';meta.after(note)}const planButtons=document.querySelector('#plan .actions.two');if(planButtons){planButtons.innerHTML='<button class="primary compact gray" type="button" onclick="showOriginalList()" data-t="originalList">Original list</button><button class="primary compact blue" id="downloadPlanButton" type="button" onclick="openExportChooser()" disabled data-t="downloadPlan">Download result</button>'}}
function languageMenuClose(){$('langPopup').hidden=true;$('langButton').setAttribute('aria-expanded','false')}
function languageMenuToggle(e){e?.stopPropagation();const open=$('langPopup').hidden;$('langPopup').hidden=!open;$('langButton').setAttribute('aria-expanded',String(open))}
function setLanguageDisplay(){const source=document.querySelectorAll('#langPopup svg')[webLanguage==='fa'?0:1],flag=$('langCurrentFlag');flag.className='lang-flag';flag.innerHTML=source?source.outerHTML:'';$('langCurrentName').textContent=webLanguage==='fa'?'فارسی':'English'}
function setWebLang(v){webLanguage=v==='en'?'en':'fa';localStorage.setItem('cutterLang',webLanguage);document.documentElement.lang=webLanguage;document.documentElement.dir=webLanguage==='fa'?'rtl':'ltr';$('webLang').value=webLanguage;setLanguageDisplay();languageMenuClose();document.querySelectorAll('[data-t]').forEach(e=>e.textContent=T(e.dataset.t));localizeDesignerChoices();localizeWebOnly();localizePortalSuite();if(lastState)applyState(lastState);updateHomeSummary();updateStatebar();renderImportPreview();renderPlan()}
function menuOpen(){$('menuBackdrop').classList.add('on');$('menuButton').setAttribute('aria-expanded','true');setTimeout(()=>{document.querySelector('#menu button')?.focus()},0)}
function menuClose(){$('menuBackdrop').classList.remove('on');$('menuButton').setAttribute('aria-expanded','false')}
function topMenuPage(n){if(['list','plan','reports','productionHub'].includes(n))return'productionHub';if(['f1','f2','f4','settingsHub'].includes(n))return'settingsHub';if(['f3','f5','diagnosticsHub'].includes(n))return'diagnosticsHub';if(['support','upd'].includes(n))return'support';return n}
function showPage(n){document.querySelectorAll('.page').forEach(p=>p.classList.toggle('on',p.id===n));const top=topMenuPage(n);document.querySelectorAll('[data-page]').forEach(b=>b.classList.toggle('on',b.dataset.page===top));activePage=n;menuClose();window.scrollTo(0,0);if(n==='list')loadList();if(n==='plan'&&!localPlan)loadPlan();if(n==='designer')renderDesigner();updateStatebar()}
function requestPage(n){if(n===activePage){menuClose();return}if(dirtyInPage(activePage)){menuClose();openConfirm(T('unsavedAsk'),()=>{clearDirtyPage(activePage);showPage(n)},'discard');return}showPage(n)}
function modalOpen(id){const e=$(id);modalReturnFocus=document.activeElement;e.classList.add('on');setTimeout(()=>{e.querySelector('button,input,select')?.focus()},0)}
function modalClose(id){const e=$(id);e.classList.remove('on');confirmAction=null;if(modalReturnFocus&&document.contains(modalReturnFocus))modalReturnFocus.focus();modalReturnFocus=null}
function openConfirm(text,action,label='continue'){$('confirmText').textContent=text;confirmAction=action;$('confirmYes').dataset.t=label;$('confirmYes').textContent=T(label);$('confirmYes').onclick=()=>{const fn=confirmAction;modalClose('confirmModal');if(fn)fn()};modalOpen('confirmModal')}
function toast(text,ok=true){clearTimeout(toastTimer);const e=$('toast');e.textContent=text;e.className='toast on '+(ok?'ok':'err');toastTimer=setTimeout(()=>e.className='toast',2600)}
function setMsg(id,text,ok){const e=$(id);e.textContent=text;e.className='msg '+(ok?'ok':'err')}
function setInput(id,v){const e=$(id);if(!e||document.activeElement===e||e.dataset.dirty==='1')return;e.value=v??'';const r=$(id+'Range');if(r)r.value=e.value}
function dirtyInPage(id){return !!document.querySelector('#'+id+' [data-dirty="1"]')}
function clearDirtyPage(id){document.querySelectorAll('#'+id+' [data-dirty="1"]').forEach(e=>e.dataset.dirty='0');updateStatebar()}
function clearDirty(ids){ids.forEach(id=>{const e=$(id);if(e)e.dataset.dirty='0'});updateStatebar()}
function updateStatebar(){const e=$('statebar'),dirty=dirtyInPage(activePage);if(dirty){e.textContent=T('dirty')+(lastSyncOk?'':' · '+T('offline'));e.className='statebar show dirty'}else if(!lastSyncOk){e.textContent=T('offlineStale');e.className='statebar show'}else{e.textContent='';e.className='statebar'}}
function setBusy(btn,busy){if(!btn)return;btn.disabled=busy;btn.dataset.baseText??=btn.textContent;btn.textContent=busy?T('saving'):btn.dataset.baseText}
Object.assign(tr.fa,{ioStatus:'\u0648\u0636\u0639\u06cc\u062a \u0648\u0631\u0648\u062f\u06cc\u200c\u0647\u0627 \u0648 \u062e\u0631\u0648\u062c\u06cc\u200c\u0647\u0627',ioSafe:'\u0646\u0645\u0627\u06cc\u0634 \u0632\u0646\u062f\u0647 \u0648 \u0641\u0642\u0637\u200c\u062e\u0648\u0627\u0646\u062f\u0646\u06cc \u0627\u0633\u062a. \u0627\u06cc\u0646 \u0635\u0641\u062d\u0647 \u0647\u06cc\u0686 \u062e\u0631\u0648\u062c\u060c \u062d\u0631\u06a9\u062a \u06cc\u0627 \u0628\u0631\u0634\u06cc \u0631\u0627 \u0627\u062c\u0631\u0627 \u0646\u0645\u06cc\u200c\u06a9\u0646\u062f.'});
Object.assign(tr.en,{ioStatus:'Input & output status',ioSafe:'Live read-only display. This page never drives an output, motion, or cutting action.'});
document.addEventListener('input',e=>{if(!e.target.id.endsWith('Range')&&e.target.matches('input:not([disabled]):not([type=file]),select')){e.target.dataset.dirty='1';updateStatebar()}});
document.addEventListener('keydown',e=>{const m=document.querySelector('.modal-wrap.on');if(!m)return;if(e.key==='Escape'){e.preventDefault();modalClose(m.id);return}if(e.key!=='Tab')return;const a=[...m.querySelectorAll('button:not([disabled]),input:not([disabled]),select:not([disabled])')];if(!a.length)return;const first=a[0],last=a[a.length-1];if(e.shiftKey&&document.activeElement===first){e.preventDefault();last.focus()}else if(!e.shiftKey&&document.activeElement===last){e.preventDefault();first.focus()}});
document.addEventListener('keydown',e=>{if(e.key==='Escape'&&!$('langPopup').hidden){languageMenuClose();$('langButton').focus()}});
document.addEventListener('click',e=>{if(!$('langButton').parentElement.contains(e.target))languageMenuClose()});
function machineLabel(d){if(d.i_stop)return['stopped','bad','!'];if(d.cutting)return['cutting','bad','✂'];if(d.moving)return['moving','warn','↔'];if(d.saw)return['saw','warn','◉'];return['idle','good','●']}
function modeLabel(d){if(d.mode==='jog')return['jog','warn','J'];if(d.mode==='opt')return['optimized','good','O'];if(d.mode==='list')return['listMode','good','L'];return['manual','','M']}
function applyState(d){
 lastState=d;$('verText').textContent=d.ver||'--';$('cur').textContent=Number(d.cur||0).toFixed(1);$('dest').textContent=Number(d.dest||0).toFixed(1);
 let m=modeLabel(d),mb=$('modeBox');mb.className='status '+m[1];$('modeIcon').textContent=m[2];$('modeText').textContent=T(m[0]);
 let s=machineLabel(d),sb=$('machineBox');sb.className='status '+s[1];$('machineIcon').textContent=s[2];$('machineText').textContent=T(s[0]);
 $('refBox').className='status '+(d.ref?'good':'bad');$('refText').textContent=T(d.ref?'referenced':'notReferenced');
 $('activeRow').textContent=d.row>0?d.row:'--';$('remaining').textContent=d.remaining>=0?d.remaining:'--';$('homeCuts').textContent=d.ucuts??'--';
 ['stock','kerf','trim','lang','vkb','buz','estop','startmm','maxlen','spd','aramp','jspd','jramp','jinit','ctimer','bdir','dayrem'].forEach(id=>setInput(id,d[id]));
 $('tcuts').textContent=d.tcuts??'--';$('ucuts').textContent=d.ucuts??'--';$('heap').textContent=d.heap??'--';$('uptime').textContent=(d.uptime??'--')+'s';
 $('devstat').textContent=(d.ssid||'')+' · '+(d.ver||'');
 fillIO(d);renderDeviceReadOnly(d);lastSyncOk=Date.now();$('loading').classList.add('hide');$('connDot').className='dot on';$('connText').textContent=T('online');if(!homeListLoaded){homeListLoaded=true;loadList()}updateHomeSummary();updateStatebar();if(importPreview)renderImportPreview();
}
function renderDeviceReadOnly(d){const f=$('f5');if(!f)return;let box=$('webDeviceReadOnly');if(!box){box=document.createElement('div');box.id='webDeviceReadOnly';box.className='card solid';f.querySelector('.stats')?.after(box)}const fa=webLanguage==='fa',date=Number(d.clock_epoch)>0?new Intl.DateTimeFormat(fa?'fa-IR-u-ca-persian':'en-GB',{dateStyle:'medium',timeStyle:'medium'}).format(new Date(Number(d.clock_epoch)*1000)):(fa?'ساعت معتبر نیست':'Clock is not valid');box.innerHTML=`<div class="report-grid"><div class="stat-box"><b>${d.ver||'--'}</b><span>${fa?'نسخه نرم‌افزار':'Firmware version'}</span></div><div class="stat-box"><b>${d.heap??'--'}</b><span>${fa?'حافظه آزاد':'Free memory'}</span></div><div class="stat-box"><b>${d.uptime??'--'}s</b><span>${fa?'زمان روشن بودن':'Uptime'}</span></div><div class="stat-box"><b>${d.dayrem??'--'}</b><span>${fa?'روز باقی‌مانده قفل':'Time-lock days remaining'}</span></div></div><p class="hint">${fa?'زمان دستگاه':'Device time'}: ${date}</p>`}
function fillIO(d){const fa=webLanguage==='fa',own=k=>Object.prototype.hasOwnProperty.call(d,k),read=(key,old)=>own(key)?!!Number(d[key]):old&&own(old)?!!Number(d[old]):false,connected=(key,old)=>own('c'+key)?!!Number(d['c'+key]):own(key)||(old&&own(old)),inputs=[['i0','i_ref','I0',fa?'سنسور اول خط':'First-line sensor','sensor'],['i1','i_up','I1',fa?'سنسور آخر خط':'End-line sensor','sensor'],['i2','', 'I2',fa?'درب محافظ':'Door guard','guard'],['i3','i_stop','I3',fa?'توقف':'Stop input','stop'],['i4','', 'I4',fa?'برگشت تیغه':'Blade return','blade'],['i5','', 'I5',fa?'خطای موتور':'Motor fault','fault'],['i6','i_s1','I6',fa?'استارت ۱':'Start 1','button'],['i7','i_s2','I7',fa?'استارت ۲':'Start 2','button'],['i8','', 'T1',fa?'جُگ راست':'Jog right','joystick'],['i9','', 'T2',fa?'جُگ چپ':'Jog left','joystick']],outputs=[['q0','', 'Q0',fa?'فعالیت پالس استپر':'Stepper pulse activity','pulse'],['q1','', 'Q1',fa?'سیگنال جهت استپر':'Stepper direction signal','direction'],['q2','', 'Q2',fa?'فعال‌ساز استپر':'Stepper enable','power'],['q3','', 'Q3',fa?'جک نگهدارنده':'Holder jack','clamp'],['q4','', 'Q4',fa?'جک محافظ':'Guard jack','guard'],['q5','', 'Q5',fa?'اسپری':'Spray','spray'],['q6','i_saw','Q6',fa?'اره':'Saw','blade'],['q7','', 'Q7',fa?'جک زاویه ۹۰ درجه':'90° Angle cylinder','direction'],['q8','', 'Q8',fa?'بالابر تیغه':'Saw lift','lift'],['q9','', 'Q9',fa?'جک زاویه ۴۵ / ۴۵- درجه':'45° / -45° Angle cylinder','direction']],stateLabel=(item,on,isOut)=>{if(item[0]==='q1')return on?'HIGH':'LOW';if(item[0]==='q0')return on?(fa?'حرکت':'MOVING'):(fa?'آماده':'IDLE');return on?(fa?(isOut?'روشن':'فعال'):(isOut?'ON':'ACTIVE')):(fa?(isOut?'خاموش':'آماده'):(isOut?'OFF':'IDLE'))},cards=(items,isOut)=>items.map(item=>{const on=read(item[0],item[1]),ok=connected(item[0],item[1]),state=ok?stateLabel(item,on,isOut):(fa?'وصل نیست':'NOT MAPPED');return`<div class="io-item ${on&&ok?'active':''} ${ok?'':'disconnected'}"><span class="io-symbol">${uiIcon(item[4])}</span><span class="io-copy"><b>${item[2]} · ${item[3]}</b><small>${isOut?(fa?'فقط نمایش؛ بدون فرمان':'Display only; no command'):(fa?'ورودی زنده':'Live input')}</small></span><span class="io-state">${state}</span></div>`}).join(''),group=(title,icon,items,isOut)=>{const count=items.filter(x=>connected(x[0],x[1])).length;return`<section class="io-group"><div class="io-group-head"><b class="io-group-title">${uiIcon(icon)} ${title}</b><span class="io-count">${count} / 10 ${fa?'متصل':'mapped'}</span></div><div class="io-grid">${cards(items,isOut)}</div></section>`};$('iobox').innerHTML=group(fa?'ورودی‌ها':'Inputs','input',inputs,false)+group(fa?'خروجی‌ها':'Outputs','output',outputs,true)+`<div class="io-legend"><span class="on"><i></i>${fa?'فعال':'Active'}</span><span><i></i>${fa?'آماده / خاموش':'Idle / off'}</span><span class="off"><i></i>${fa?'نگاشت نشده':'Not mapped'}</span><span>${uiIcon('info','sm')}${fa?'این صفحه هیچ خروجی را روشن نمی‌کند.':'This page never drives an output.'}</span></div>`}
async function syncState(){if(syncBusy)return;syncBusy=true;try{const r=await fetch('/api/settings/get',{cache:'no-store'});if(!r.ok)throw 0;applyState(await r.json())}catch(e){lastSyncOk=0;$('loading').classList.add('hide');$('connDot').className='dot off';$('connText').textContent=T('offline');updateStatebar();if(importPreview)renderImportPreview()}finally{syncBusy=false}}
function downloadScreen(){const form=document.createElement('form');form.method='POST';form.action='/api/screen.bmp';form.target='_blank';form.hidden=true;document.body.appendChild(form);form.submit();form.remove();toast(T('screenRequest'),true)}
let pendingPollTimer=null;
function pollPendingState(onDone){
 if(pendingPollTimer)clearInterval(pendingPollTimer);
 pendingPollTimer=setInterval(async()=>{
  try{
   const r=await fetch('/api/pending/get',{cache:'no-store'});
   if(!r.ok)return;
   const d=await r.json();
   if(!d.pending){
    clearInterval(pendingPollTimer);
    pendingPollTimer=null;
    if(onDone)onDone();
   }
  }catch(e){}
 },2000);
}
function responseText(status,text){const raw=String(text||''),upper=raw.toUpperCase();if(status===202)return webLanguage==='fa'?'در انتظار تأیید روی دستگاه…':'Waiting for device confirmation…';if(status===403)return upper.includes('ADMIN')?T('wrongAdminPin'):(raw||T('failed'));if(status===409)return upper.includes('PENDING')?(webLanguage==='fa'?'درخواست قبلی در انتظار تأیید است':'A previous request is waiting for confirmation'):T('busy');if(status===400||status===413||status===422)return raw||T('failed');return raw.startsWith('OK')?T('saved'):T('failed')}
async function postFields(ids,msgId,btn){const p=new URLSearchParams();ids.forEach(id=>p.append(id,$(id).value));setBusy(btn,true);try{const r=await fetch('/api/settings/set',{method:'POST',body:p}),tx=await r.text(),ok=r.ok||r.status===202,msg=responseText(r.status,tx);setMsg(msgId,msg,ok);toast(msg,ok);if(r.status===202){pollPendingState(async()=>{toast(webLanguage==='fa'?'وضعیت دستگاه به‌روزرسانی شد':'Device state updated',true);clearDirty(ids);await syncState()})}else if(r.ok){clearDirty(ids);await syncState()}}catch(e){setMsg(msgId,T('failed'),false)}finally{setBusy(btn,false)}}
function saveF1(btn){postFields(['stock','kerf','trim','lang','vkb','buz'],'m1',btn)}function saveF2(btn){postFields(['startmm','maxlen','spd','aramp'],'m2',btn)}
function saveF4(btn){const stopDirty=$('estop')?.dataset.dirty==='1',baseDirty=$('bdir')?.dataset.dirty==='1';const go=()=>postFields(['ctimer','estop','bdir','jspd','jramp','jinit','adminpin','newadmin'],'m4',btn);if(stopDirty||baseDirty)openConfirm(T(stopDirty&&baseDirty?'stopBaseWarning':stopDirty?'stopWarning':'baseWarning'),go);else go()}
function resetUserCuts(){openConfirm(T('resetCuts')+'؟',async()=>{const p=new URLSearchParams();p.append('reset_ucuts','1');try{const r=await fetch('/api/settings/set',{method:'POST',body:p}),tx=await r.text(),msg=responseText(r.status,tx);setMsg('m5',msg,r.ok);toast(msg,r.ok);if(r.ok)syncState()}catch(e){setMsg('m5',T('failed'),false)}})}
function beginAsyncDownload(){const w=window.open('','_blank');if(!w)return null;w.document.write('<!doctype html><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Download</title><style>body{font-family:sans-serif;padding:24px;line-height:1.7}a{display:inline-block;padding:12px 16px;border-radius:10px;background:#087ad9;color:#fff;text-decoration:none}</style><p id="downloadStatus"></p><a id="downloadLink" hidden>Download file</a>');w.document.close();w.document.getElementById('downloadStatus').textContent=T('preparingExport');return w}
function showDownloadFallback(name,url){let box=$('downloadFallbackModal');if(!box){box=document.createElement('div');box.className='modal-wrap';box.id='downloadFallbackModal';box.innerHTML='<div class="modal"><div class="modal-head"><h3>Download</h3><button class="x" onclick="modalClose(\'downloadFallbackModal\')">×</button></div><p id="downloadFallbackText" class="hint"></p><a id="downloadFallbackLink" class="primary" style="display:grid;place-items:center;text-decoration:none"></a></div>';document.body.appendChild(box)}$('downloadFallbackText').textContent=webLanguage==='fa'?'اگر دانلود خودکار شروع نشد، این کلید را لمس کنید.':'If the automatic download did not start, tap this button.';const a=$('downloadFallbackLink');a.href=url;a.download=name;a.textContent=name;modalOpen('downloadFallbackModal')}
function deliverBlob(name,blob,target=null){if(!blob){if(target&&!target.closed)target.close();toast(T('failed'),false);return}const url=URL.createObjectURL(blob);if(target&&!target.closed){const a=target.document.getElementById('downloadLink'),s=target.document.getElementById('downloadStatus');if(a){a.href=url;a.download=name;a.hidden=false;a.textContent=name;if(s)s.textContent=T('downloaded');a.click()}}else{const a=document.createElement('a');a.href=url;a.download=name;document.body.appendChild(a);a.click();a.remove();showDownloadFallback(name,url)}setTimeout(()=>URL.revokeObjectURL(url),60000);toast(T('downloaded'),true)}
function downloadBlob(name,type,text){deliverBlob(name,new Blob([text],{type}),beginAsyncDownload())}
function importDigits(s){return String(s??'').replace(/[\u0660-\u0669\u06f0-\u06f9]/g,c=>String(c.charCodeAt(0)%16)).replace(/[\u064a\u0649]/g,'ی').replace(/ك/g,'ک').replace(/\u200c/g,' ')}
function importKey(s){return importDigits(s).toLowerCase().replace(/[\s_\-()\[\].]/g,'')}
function importDelimited(text,sep){const rows=[];let row=[],cell='',quote=false,line=1,start=1;for(let i=0;i<text.length;i++){const c=text[i];if(c==='"'){if(quote&&text[i+1]==='"'){cell+='"';i++}else quote=!quote;continue}if(!quote&&c===sep){row.push(cell.trim());cell='';continue}if(!quote&&(c==='\n'||c==='\r')){if(c==='\r'&&text[i+1]==='\n')i++;row.push(cell.trim());rows.push({line:start,cells:row});row=[];cell='';line++;start=line;continue}cell+=c}if(quote)return{error:'unclosed quote'};if(cell||row.length){row.push(cell.trim());rows.push({line:start,cells:row})}return{rows}}
function importDelimiter(text){const sample=text.split(/\r?\n/).filter(x=>x.trim()).slice(0,20).join('\n'),choices=[',',';','\t','|'];let best=[],score=-1;for(const sep of choices){let q=false,n=0;for(let i=0;i<sample.length;i++){if(sample[i]==='"')q=!q;else if(!q&&sample[i]===sep)n++}if(n>score){score=n;best=[sep]}else if(n===score)best.push(sep)}return{sep:best.length===1?best[0]:null,label:best.length===1?(best[0]==='\t'?'TSV':best[0]):'ambiguous'}}
function importAliases(kind){if(kind==='length')return['length','lengthmm','length_mm','size','cutlength','metraj','tol','طول','متراژ','اندازه','طولبرش','lmm','l(mm)','cuttinglength'];if(kind==='quantity')return['quantity','qty','count','pieces','tedad','تعداد','تیراژ','تيراژ','شماره','pcs','piececount'];if(kind==='angle')return['angle','ang','zavie','zaviyeh','زاویه','زاویهبرش','cuttingangle','anglel','angler','deg'];return[]}
function importHeaderMap(rows){for(const row of rows.filter(r=>r.cells.some(c=>c.trim())).slice(0,20)){const length=[],quantity=[],angle=[];row.cells.forEach((c,i)=>{const k=importKey(c);if(importAliases('length').includes(k))length.push(i);if(importAliases('quantity').includes(k))quantity.push(i);if(importAliases('angle').includes(k))angle.push(i)});if(length.length||quantity.length)return{row,length,quantity,angle:angle.length?angle[0]:undefined}}return{row:null,length:[],quantity:[],angle:undefined}}
function importUnit(header){const h=importKey(header||'');if(/(mm|millimeter|میلیمتر)/.test(h))return{kind:'mm',factor:1,confirm:false};if(/(cm|centimeter|سانت)/.test(h))return{kind:'cm',factor:10,confirm:true};if(/(^|[^m])m($|[^m])|meter|متر/.test(h))return{kind:'m',factor:1000,confirm:true};return{kind:'unitless',factor:1,confirm:true}}
function importNumber(raw,decimalComma){let s=importDigits(raw).trim();if(decimalComma&&s.includes(',')){if(s.includes('.')||!/^\d+(?:,\d+)?$/.test(s))return NaN;s=s.replace(',','.')}if(!decimalComma&&!/^\d+(?:\.\d+)?$/.test(s))return NaN;return Number(s)}
function importMappingChanged(){if(!importPreview)return;const l=Number($('importLengthColumn')?.value),q=Number($('importQuantityColumn')?.value);if(Number.isInteger(l)&&Number.isInteger(q)&&l!==q){importPreview.mapping={length:l,quantity:q,manual:true};importPreview.unit=importUnit(importPreview.header?.cells[l]);rebuildImportPreview()}else{importPreview.mapping=null;rebuildImportPreview()}renderImportPreview()}
function rebuildImportPreview(){if(!importPreview)return;const p=importPreview,valid=[],errors=[],min=Number(lastState?.startmm),max=Number(lastState?.maxlen),decimalComma=p.delimiter!==',';if(!p.mapping){p.valid=valid;p.errors=[{line:p.header?.line||1,reason:'choose length and quantity columns'}];return}for(const row of p.rows){if(p.header&&row.line===p.header.line)continue;const cells=row.cells;if(!cells.some(c=>c.trim()))continue;const length=importNumber(cells[p.mapping.length]??'',decimalComma),count=importNumber(cells[p.mapping.quantity]??'',false);let reason='';if(!Number.isFinite(length)||length<=0)reason='invalid length';else if(!Number.isFinite(count)||!Number.isInteger(count)||count<1||count>999)reason='quantity 1..999';const mm=length*p.unit.factor;if(!reason&&(!Number.isFinite(min)||!Number.isFinite(max)||mm<min||mm>max))reason=`${min}..${max} mm`;if(!reason&&valid.length>=LIST_MAX)reason='LIST_MAX 100';if(reason)errors.push({line:row.line,reason});else valid.push({length:mm,count,line:row.line})}p.valid=valid;p.errors=errors}
async function preflightImport(){const f=$('szfile').files[0];importPreview=null;if(!f){setMsg('impmsg',T('noFile'),false);renderImportPreview();return}if(!/\.(csv|tsv|txt|text)$/i.test(f.name)||f.size>262144){setMsg('impmsg',T('fileTooLarge'),false);renderImportPreview();return}if(!lastSyncOk||!lastState){setMsg('impmsg',T('importNeedsSync'),false);renderImportPreview();return}let bytes;try{bytes=new Uint8Array(await f.arrayBuffer())}catch(e){setMsg('impmsg',T('failed'),false);return}let encoding='UTF-8',offset=0;if(bytes[0]===0xff&&bytes[1]===0xfe){encoding='UTF-16LE';offset=2}else if(bytes[0]===0xfe&&bytes[1]===0xff){encoding='UTF-16BE';offset=2}else if(bytes[0]===0xef&&bytes[1]===0xbb&&bytes[2]===0xbf)offset=3;let text='';try{text=new TextDecoder(encoding,{fatal:true}).decode(bytes.slice(offset))}catch(e){setMsg('impmsg','unsupported text encoding',false);return}const delim=importDelimiter(text);if(!delim.sep){importPreview={file:f.name,encoding,delimiter:'ambiguous',rows:[],header:null,mapping:null,unit:{kind:'unitless',factor:1,confirm:true},valid:[],errors:[{line:1,reason:'ambiguous delimiter'}]};renderImportPreview();return}const parsed=importDelimited(text,delim.sep);if(parsed.error){setMsg('impmsg',parsed.error,false);return}const header=importHeaderMap(parsed.rows),map=header.length.length===1&&header.quantity.length===1&&header.length[0]!==header.quantity[0]?{length:header.length[0],quantity:header.quantity[0],manual:false}:null;importPreview={file:f.name,encoding,delimiter:delim.label,rows:parsed.rows,header:header.row,mapping:map,unit:importUnit(header.row?.cells[map?.length]),valid:[],errors:[]};rebuildImportPreview();setMsg('impmsg',importPreview.valid.length?T('importReady'):T('invalidFormat'),importPreview.valid.length>0);renderImportPreview()}
function renderImportPreview(){const box=$('importSummary'),apply=$('applyImportButton'),mapBox=$('importMap'),mode=$('importMode'),confirm=$('importUnitConfirm');if(!box)return;if(!importPreview){box.className='import-summary';$('importStats').textContent='';$('importErrors').textContent='';mapBox.textContent='';apply.disabled=true;return}const p=importPreview,fa=webLanguage==='fa';box.className='import-summary show';mode.options[0].textContent=fa?'جایگزینی فرم':'Replace current form';mode.options[1].textContent=fa?'افزودن به فرم':'Append to current form';confirm.parentElement.lastChild.textContent=fa?' تأیید واحد/تبدیل نمایش‌داده‌شده یا مقادیر بدون واحد':' I confirm the displayed conversion / unitless values.';$('importStats').textContent=`${p.file} · ${p.encoding} · ${p.delimiter} · ${p.unit.kind} · ${p.mapping?`${p.mapping.manual?'manual':'header'} ${p.mapping.length+1}/${p.mapping.quantity+1}`:'mapping required'} · ${p.valid.length} ${T('validRows')} · ${p.errors.length} ${T('rejectedRows')}`;mapBox.textContent='';if(!p.mapping||p.mapping.manual){const labels=p.header?.cells||p.rows[0]?.cells||[];const make=(id,title,selected)=>{const l=document.createElement('label');l.textContent=title;const s=document.createElement('select');s.id=id;s.onchange=importMappingChanged;labels.forEach((v,i)=>{const o=document.createElement('option');o.value=i;o.textContent=`${i+1}: ${v||'(empty)'}`;o.selected=i===selected;s.appendChild(o)});l.appendChild(s);mapBox.appendChild(l)};make('importLengthColumn',fa?'ستون طول':'Length column',p.mapping?.length??-1);make('importQuantityColumn',fa?'ستون تعداد':'Quantity column',p.mapping?.quantity??-1)}const er=$('importErrors');er.textContent='';p.errors.slice(0,20).forEach(x=>{const d=document.createElement('div');d.textContent=`${fa?'خط':'Line'} ${x.line}: ${x.reason}`;er.appendChild(d)});apply.disabled=!lastSyncOk||!p.mapping||!p.valid.length||(p.unit.confirm&&!confirm.checked)}
function applyImportPreview(){if(!lastSyncOk||!importPreview?.valid.length||!importPreview.mapping||importPreview.unit.confirm&&!$('importUnitConfirm').checked){setMsg('impmsg',T('importNeedsSync'),false);renderImportPreview();return}buildList();const current=listItems();if(current.invalid.length){setMsg('impmsg','fix current invalid rows first',false);return}const rows=$('importMode').value==='append'?[...current.items,...importPreview.valid]:importPreview.valid;if(rows.length>LIST_MAX){setMsg('impmsg','LIST_MAX 100: no rows applied',false);return}for(let i=0;i<LIST_MAX;i++){const row=rows[i],sp=$('sp'+i),cnt=$('cnt'+i),ang=$('ang'+i);sp.value=row?row.length:'';cnt.value=row?row.count:'';if(ang)ang.value=row?(row.angle||45):45;sp.dataset.dirty=cnt.dataset.dirty='1';if(ang)ang.dataset.dirty='1'}listVisibleRows=Math.max(1,rows.length);$('rows').value=rows.length;$('rows').dataset.dirty='1';updateListRows();importPreview=null;renderImportPreview();setMsg('impmsg',T('importApplied'),true);updateStatebar()}
async function loadPlan(btn){setBusy(btn,true);try{const r=await fetch('/api/optimized/get',{cache:'no-store'});if(!r.ok)throw 0;planSnapshot=await r.json();localPlan=false;renderPlan();updateHomeSummary()}catch(e){planSnapshot=null;localPlan=false;renderPlan();updateHomeSummary();setMsg('planmsg',T('planUnavailable'),false)}finally{setBusy(btn,false)}}
function renderPlan(){
 const d=planSnapshot,root=$('planBarsList');if(!root)return;root.textContent='';$('downloadPlanButton').disabled=!d?.complete;
 if(!d){$('planBars').textContent=$('planPieces').textContent=$('planWaste').textContent='--';if($('planUtil'))$('planUtil').textContent='--';$('planMeta').textContent='';if($('planUnallocated'))$('planUnallocated').textContent='';return}
 const bars=Array.isArray(d.bars)?d.bars:[],pieces=bars.reduce((n,b)=>n+(b.pieces_mm?.length||0),0),stock=Number(d.stock_mm)||0,trim=Number(d.trim_mm)||0,kerf=Number(d.kerf_mm)||0,used=bars.reduce((n,b)=>n+(Array.isArray(b.pieces_mm)?b.pieces_mm.reduce((s,v)=>s+(Number(v)||0)+kerf,0):0),0),capacity=bars.length*Math.max(0,stock-trim),utilization=capacity>0?Math.max(0,Math.min(100,used*100/capacity)):0,unallocated=Array.isArray(d.unallocated)?d.unallocated:[];
 $('planBars').textContent=bars.length;$('planPieces').textContent=pieces;$('planWaste').textContent=(Number(d.total_waste_mm)||0).toFixed(1)+' mm';if($('planUtil'))$('planUtil').textContent=utilization.toFixed(1)+'%';
 $('planMeta').textContent=`${String(d.plan_id||'')} · ${stock} mm · kerf ${kerf} mm`;if($('planUnallocated'))$('planUnallocated').textContent=unallocated.length?`${T('unallocatedPieces')}: ${unallocated.length}`:T('unallocatedNone');
 if(!d.complete){setMsg('planmsg',T('planPending'),false);return}setMsg('planmsg','',true);const legend=document.createElement('div');legend.className='waste-legend';legend.innerHTML='<span class="waste-key green"><i></i>0–10%</span><span class="waste-key yellow"><i></i>10–25%</span><span class="waste-key red"><i></i>25%+</span>';root.appendChild(legend);
 bars.forEach((b,idx)=>{const ratio=Number(b.waste_mm||0)/Math.max(1,stock-trim),level=ratio<=.1?'green':ratio<=.25?'yellow':'red',card=document.createElement('div');card.className='plan-bar waste-'+level;const head=document.createElement('div');head.className='plan-bar-head';const left=document.createElement('span'),right=document.createElement('span');left.textContent=`${T('bars')} ${Number(b.bar)||idx+1}`;right.textContent=`${Number(b.waste_mm||0).toFixed(1)} mm ${T('waste')} (${Math.round(ratio*100)}%)`;head.append(left,right);const track=document.createElement('div');track.className='plan-track';(Array.isArray(b.pieces_mm)?b.pieces_mm:[]).forEach(v=>{const length=Number(v);if(!Number.isFinite(length)||length<=0)return;const p=document.createElement('div');p.className='plan-piece';p.style.width=Math.max(1,length*100/Number(d.stock_mm||1))+'%';p.textContent=length.toFixed(length%1?1:0);track.appendChild(p)});const w=document.createElement('div');w.className='plan-waste';w.style.flex='1';track.appendChild(w);card.append(head,track);if(d.local){const del=document.createElement('button');del.className='remove-bar';del.textContent=webLanguage==='fa'?'↶ حذف شاخهٔ محلی':'↶ Remove local bar';del.onclick=()=>removeLocalBar(idx);card.appendChild(del)}root.appendChild(card)});
}
async function saveList(btn){buildList();const data=normalizeList();if(data.invalid.length){setMsg('listmsg',`${T('invalidManualRows')}: ${data.invalid.join(', ')}`,false);return}if(!data.items.length){setMsg('listmsg',T('invalidFormat'),false);return}const p=new URLSearchParams();p.append('rows',String(data.items.length));for(let i=0;i<LIST_MAX;i++){p.append('sp'+i,$('sp'+i).value);p.append('cnt'+i,$('cnt'+i).value)}setBusy(btn,true);try{const r=await fetch('/api/list/set',{method:'POST',body:p}),tx=await r.text(),ok=r.ok||r.status===202,msg=responseText(r.status,tx);setMsg('listmsg',msg,ok);toast(msg,ok);if(r.status===202){pollPendingState(async()=>{lastSaveAt=T('savedNow');clearDirtyPage('list');await loadList();updateHomeSummary();toast(webLanguage==='fa'?'لیست برش روی دستگاه اعمال شد':'Cut list applied on device',true)})}else if(r.ok){lastSaveAt=T('savedNow');clearDirtyPage('list');await loadList();updateHomeSummary()}}catch(e){setMsg('listmsg',T('failed'),false)}finally{setBusy(btn,false)}}
function downloadPlan(){const d=planSnapshot;if(!d?.complete)return;const out=['# haffler-cutter-plan,v1',`plan_id,${d.plan_id||''}`,`stock_mm,${d.stock_mm||0}`,`kerf_mm,${d.kerf_mm||0}`,`trim_mm,${d.trim_mm||0}`,'bar,piece,length_mm'];(d.bars||[]).forEach((b,bi)=>(b.pieces_mm||[]).forEach((v,pi)=>out.push(`${b.bar||bi+1},${pi+1},${v}`)));out.push('bar,waste_mm');(d.bars||[]).forEach((b,bi)=>out.push(`${b.bar||bi+1},${b.waste_mm||0}`));downloadBlob('cutter-plan-v1.csv','text/csv;charset=utf-8','\ufeff'+out.join('\r\n')+'\r\n')}
/* Dynamic list: keep the legacy 100-field API payload, while showing only operator rows. */
function buildList(){if(listBuilt)return;listBuilt=true;const b=$('listtbl').tBodies[0];let h='';for(let i=0;i<LIST_MAX;i++)h+=`<tr><td>${i+1}</td><td><input aria-label="Length row ${i+1}" type="number" id="sp${i}" min="0" step=".1"></td><td><input aria-label="Count row ${i+1}" type="number" id="cnt${i}" min="0" max="999"></td><td><button class="row-delete" type="button" aria-label="Delete row ${i+1}" onclick="deleteListRow(${i})">×</button></td></tr>`;b.innerHTML=h;const head=$('listtbl').tHead.rows[0];if(head.cells.length===3)head.insertCell().textContent='×';b.addEventListener('input',updateListRows);updateListRows()}
function addListRow(){buildList();if(listVisibleRows>=LIST_MAX){toast('LIST_MAX 100',false);return}listVisibleRows++;updateListRows();$('sp'+(listVisibleRows-1)).focus()}
function deleteListRow(index){buildList();for(let i=index;i<LIST_MAX-1;i++){$('sp'+i).value=$('sp'+(i+1)).value;$('cnt'+i).value=$('cnt'+(i+1)).value;$('sp'+i).dataset.dirty=$('cnt'+i).dataset.dirty='1'}$('sp'+(LIST_MAX-1)).value='';$('cnt'+(LIST_MAX-1)).value='';if(index<listVisibleRows)listVisibleRows=Math.max(1,listVisibleRows-1);updateListRows();updateStatebar()}
function normalizeList(){const data=listItems();if(data.invalid.length)return data;for(let i=0;i<LIST_MAX;i++){const row=data.items[i],sp=$('sp'+i),cnt=$('cnt'+i);sp.value=row?row.length:'';cnt.value=row?row.count:''}listVisibleRows=Math.max(1,Math.min(LIST_MAX,data.items.length));$('rows').value=data.items.length;updateListRows();return data}
async function loadList(){buildList();try{const r=await fetch('/api/list/get',{cache:'no-store'});if(!r.ok)throw 0;const d=await r.json();for(let i=0;i<LIST_MAX;i++){const sp=Number(d.sp[i]),cnt=Number(d.cnt[i]);setInput('sp'+i,sp===0&&cnt===0?'':d.sp[i]);setInput('cnt'+i,sp===0&&cnt===0?'':d.cnt[i])}listVisibleRows=Math.max(1,Math.min(LIST_MAX,listItems().items.length));listLoaded=true;updateListRows();setMsg('listmsg','',true)}catch(e){setMsg('listmsg',T('failed'),false)}}
function exportStamp(){return new Intl.DateTimeFormat(webLanguage==='fa'?'fa-IR-u-ca-persian':'en-GB',{year:'numeric',month:'2-digit',day:'2-digit',hour:'2-digit',minute:'2-digit'}).format(new Date()).replace(/[\s/:]/g,'-')}
function listExportRows(){const data=listItems();if(data.invalid.length){setMsg('listmsg',`${T('invalidManualRows')}: ${data.invalid.join(', ')}`,false);return null}return data.items}
function downloadCurrentList(kind){const rows=listExportRows();if(!rows)return;if(!kind){openListExportChooser();return}const stamp=exportStamp(),title=webLanguage==='fa'?'لیست برش':'Cut list';if(kind==='csv'||kind==='txt'){const sep=kind==='csv'?',':'\t',out=[`# ${title}`,`# ${stamp}`,'length_mm'+sep+'quantity',...rows.map(r=>r.length+sep+r.count)];downloadBlob(`cut-list-${stamp}.${kind==='csv'?'csv':'txt'}`,'text/plain;charset=utf-8',(kind==='csv'?'﻿':'')+out.join('\r\n')+'\r\n');return}const target=beginAsyncDownload(),c=document.createElement('canvas'),w=900,h=Math.min(3200,110+rows.length*28),x=c.getContext('2d');c.width=w;c.height=h;x.fillStyle='#fff';x.fillRect(0,0,w,h);x.fillStyle='#17191c';x.font='bold 23px sans-serif';x.fillText(title,28,34);x.font='14px sans-serif';x.fillText(stamp,28,58);rows.slice(0,100).forEach((r,i)=>x.fillText(`${i+1}. ${r.length} mm × ${r.count}`,28,90+i*28));c.toBlob(b=>deliverBlob(`cut-list-${stamp}.png`,b,target),'image/png')}
function openListExportChooser(){let box=$('listExportModal'),fa=webLanguage==='fa';if(!box){box=document.createElement('div');box.className='modal-wrap';box.id='listExportModal';box.innerHTML=`<div class="modal"><div class="modal-head"><h3>▤ ${fa?'خروجی':'Export'}</h3><button class="x" onclick="modalClose('listExportModal')">×</button></div><div class="export-grid"><button onclick="downloadCurrentList('csv')">CSV UTF-8</button><button onclick="downloadCurrentList('txt')">TXT</button><button onclick="downloadCurrentList('png')">PNG</button><button onclick="printCurrentList()">${fa?'چاپ / PDF':'Print / PDF'}</button><button onclick="listPrintLabels()" style="grid-column:span 2;background:#087ad9;color:#fff">🏷️ ${fa?'چاپ لیبل‌های لیست برش (بارکددار)':'Print Cut Labels (with barcode)'}</button></div></div>`;document.body.appendChild(box)}modalOpen('listExportModal')}
let localPlanUndo=null;
function removeLocalBar(index){if(!planSnapshot?.local||!planSnapshot.bars[index])return;const fa=webLanguage==='fa';openConfirm(fa?'این شاخهٔ پیش‌نمایش محلی حذف شود؟ امکان بازگردانی دارید.':'Remove this local preview bar? You can undo this change.',()=>{localPlanUndo=JSON.parse(JSON.stringify(planSnapshot));const removed=planSnapshot.bars.splice(index,1);planSnapshot.unallocated=(planSnapshot.unallocated||[]).concat(removed[0].pieces_mm||[]);planSnapshot.complete=false;planSnapshot.total_waste_mm=planSnapshot.bars.reduce((n,b)=>n+Number(b.waste_mm||0),0);renderPlan();setMsg('planmsg',fa?'شاخهٔ محلی حذف شد؛ پیش از خروجی دوباره بهینه‌سازی کنید.':'Local bar removed. Re-optimize before export.',false);const undo=document.createElement('button');undo.className='primary compact gray';undo.id='undoLocalBar';undo.textContent=fa?'بازگردانی':'Undo';undo.onclick=undoLocalPlanChange;$('planBarsList').before(undo)})}
function undoLocalPlanChange(){if(!localPlanUndo)return;planSnapshot=localPlanUndo;localPlanUndo=null;$('undoLocalBar')?.remove();renderPlan();setMsg('planmsg',webLanguage==='fa'?'پیش‌نمایش محلی بازگردانی شد.':'Local preview restored.',true)}
let designerProject={activeIdx:0,units:[{id:'u1',name:'یونیت ۱',w:1200,h:1400,q:1,template:'fixed',dividers:[],cells:{},meta:{project:'',customer:'',profile:'',glass:'none',color:'white'}}]};
let designerModel=designerProject.units[0],designerLastValid={w:1200,h:1400};

function designerSyncActiveUnit(){
  if(!designerModel) return;
  const v=designerValues();
  designerModel.w=v.w; designerModel.h=v.h; designerModel.q=v.q;
  designerModel.template=$('designTemplate')?.value||'fixed';
  designerModel.meta=designerMetadata();
}

function designerRenderUnitList(){
  const list=$('designerUnitList');
  if(!list) return;
  const fa=webLanguage==='fa';
  list.innerHTML=designerProject.units.map((u,idx)=>{
    const active=idx===designerProject.activeIdx;
    const svgThumb=designerSvg(u,100,60);
    return `<div class="designer-unit-card ${active?'active':''}" onclick="designerSelectUnit(${idx})">
      <div class="u-title">${fa?('یونیت '+(idx+1)):('Unit '+(idx+1))}</div>
      <div class="u-thumb">${svgThumb}</div>
      <div class="u-size">${u.w||1200} × ${u.h||1400}</div>
      <div class="u-qty">${u.q||1} ${fa?'عدد':'pcs'}</div>
    </div>`;
  }).join('');
}

function designerSelectUnit(idx){
  if(idx<0||idx>=designerProject.units.length) return;
  designerSyncActiveUnit();
  designerProject.activeIdx=idx;
  designerModel=designerProject.units[idx];
  designerLastValid={w:designerModel.w||1200,h:designerModel.h||1400};
  if($('designWidth')) $('designWidth').value=designerModel.w||1200;
  if($('designHeight')) $('designHeight').value=designerModel.h||1400;
  if($('designQuantity')) $('designQuantity').value=designerModel.q||1;
  if($('designTemplate')) $('designTemplate').value=designerModel.template||'fixed';
  const m=designerModel.meta||{};
  if($('designProjectName')) $('designProjectName').value=m.project||'';
  if($('designCustomer')) $('designCustomer').value=m.customer||'';
  if($('designProfile')) $('designProfile').value=m.profile||'';
  if($('designGlass')) $('designGlass').value=m.glass||'none';
  if($('designColor')) $('designColor').value=m.color||'white';
  renderDesigner();
}

function designerAddUnit(){
  designerSyncActiveUnit();
  const n=designerProject.units.length+1;
  const newU={id:'u'+Date.now(),name:'یونیت '+n,w:1200,h:1400,q:1,template:'fixed',dividers:[],cells:{},meta:{...designerMetadata()}};
  designerProject.units.push(newU);
  designerSelectUnit(designerProject.units.length-1);
  toast(webLanguage==='fa'?'یونیت جدید به پروژه اضافه شد.':'New unit added to project.',true);
}

function designerDuplicateUnit(){
  designerSyncActiveUnit();
  const cur=designerProject.units[designerProject.activeIdx];
  const clone=JSON.parse(JSON.stringify(cur));
  clone.id='u'+Date.now();
  clone.name=clone.name+' (کپی)';
  designerProject.units.push(clone);
  designerSelectUnit(designerProject.units.length-1);
  toast(webLanguage==='fa'?'یونیت کپی شد.':'Unit duplicated.',true);
}

function designerDeleteUnit(){
  if(designerProject.units.length<=1){
    toast(webLanguage==='fa'?'حداقل یک یونیت در پروژه باید باقی بماند.':'At least one unit must remain.',false);
    return;
  }
  designerProject.units.splice(designerProject.activeIdx,1);
  if(designerProject.activeIdx>=designerProject.units.length) designerProject.activeIdx=designerProject.units.length-1;
  designerSelectUnit(designerProject.activeIdx);
  toast(webLanguage==='fa'?'یونیت حذف شد.':'Unit deleted.',true);
}

function designerUpdateQty(v){
  const q=Math.max(1,Math.min(999,Number(v)||1));
  if(designerModel) designerModel.q=q;
  renderDesigner();
}

const IRAN_PROFILE_PRESETS = {
  custom: { name: 'شخصی‌سازی / دستی (Custom)', frameW: 60, sashW: 75, mullionW: 80, weld: 3, beadH: 20 },
  wintech60: { name: 'وین‌تک (WinTech W640)', frameW: 60, sashW: 75, mullionW: 80, weld: 3, beadH: 20 },
  wintech70: { name: 'وین‌تک (WinTech W700)', frameW: 70, sashW: 82, mullionW: 85, weld: 3.5, beadH: 22 },
  vistabest60: { name: 'ویستابست (VistaBest 60)', frameW: 60, sashW: 77, mullionW: 82, weld: 3, beadH: 20 },
  vistabest70: { name: 'ویستابست (VistaBest 70)', frameW: 70, sashW: 84, mullionW: 88, weld: 3.5, beadH: 22 },
  hofmann60: { name: 'هافمن (Hofmann 60)', frameW: 60, sashW: 76, mullionW: 80, weld: 3, beadH: 20 },
  syndece60: { name: 'سیندژ (SynDece 60)', frameW: 60, sashW: 75, mullionW: 80, weld: 3, beadH: 20 },
  atatech60: { name: 'آتاتک (Atatech 60)', frameW: 60, sashW: 75, mullionW: 80, weld: 3, beadH: 20 },
  enzo60: { name: 'انزو / پلاس‌پن (Enzo 60)', frameW: 60, sashW: 74, mullionW: 78, weld: 3, beadH: 20 }
};

function designerProfilePresetChanged(){
  const sel=$('designProfilePreset')?.value||'custom';
  const p=IRAN_PROFILE_PRESETS[sel];
  if(p && sel!=='custom'){
    if($('designProfile')) $('designProfile').value=p.name;
    if($('weldAllowance')) $('weldAllowance').value=p.weld;
  }
  renderDesigner();
}

function designerPrintLabels(){
  const cuts=designerCalcCutBreakdown();
  if(!cuts||!cuts.length){ toast(webLanguage==='fa'?'هیچ قطعه‌ای برای چاپ لیبل وجود ندارد.':'No cut pieces for labels.', false); return; }
  const meta=designerMetadata();
  const title=webLanguage==='fa'?'لیبل قطعات برش درب و پنجره':'Window & Door Cut Labels';
  const date=new Intl.DateTimeFormat(webLanguage==='fa'?'fa-IR-u-ca-persian':'en-GB',{dateStyle:'medium',timeStyle:'short'}).format(new Date());

  const w=window.open('','_blank');
  if(!w){ toast(webLanguage==='fa'?'پنجره چاپ مسدود است.':'Print popup blocked.', false); return; }

  let labelHtml = '';
  cuts.forEach((c, idx)=>{
    for(let i=0; i<c.cnt; i++){
      labelHtml += `
      <div class="label-card">
        <div class="lbl-head">
          <b>HAFFLER CUTTER PLC</b>
          <span>${c.unit?'U'+c.unit:''} #${idx+1} (${i+1}/${c.cnt})</span>
        </div>
        <div class="lbl-project">${meta.project?('پروژه: '+meta.project):'پروژه پیش‌فرض'} · ${meta.customer||''}</div>
        <div class="lbl-part">${c.name}</div>
        <div class="lbl-size">
          <span class="lbl-len">${c.len} mm</span>
          <span class="lbl-ang">${c.ang}° / ${c.ang}°</span>
        </div>
        <div class="lbl-footer">
          <div class="lbl-bar">||| ||||| || |||| |||</div>
          <small>${date}</small>
        </div>
      </div>`;
    }
  });

  w.document.write(`<!doctype html>
  <meta charset="utf-8">
  <title>${title}</title>
  <style>
    @page { size: A4; margin: 10mm; }
    body { font-family: tahoma, sans-serif; direction: rtl; margin: 0; padding: 10px; background: #f8fafc; }
    .label-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(220px, 1fr)); gap: 10px; }
    .label-card { border: 2px solid #111; border-radius: 8px; padding: 8px 10px; background: #fff; page-break-inside: avoid; display: flex; flex-direction: column; justify-content: space-between; }
    .lbl-head { display: flex; justify-content: space-between; font-size: 10px; border-bottom: 1px dashed #666; padding-bottom: 3px; }
    .lbl-project { font-size: 11px; color: #444; margin-top: 3px; font-weight: 500; }
    .lbl-part { font-size: 13px; font-weight: bold; margin: 4px 0; color: #000; }
    .lbl-size { display: flex; justify-content: space-between; align-items: baseline; background: #f0f4f8; padding: 4px 6px; border-radius: 4px; margin: 4px 0; }
    .lbl-len { font-size: 18px; font-weight: 900; color: #087ad9; }
    .lbl-ang { font-size: 12px; font-weight: bold; background: #333; color: #fff; padding: 1px 5px; border-radius: 3px; }
    .lbl-footer { display: flex; justify-content: space-between; align-items: center; font-size: 9px; color: #666; border-top: 1px dashed #ccc; padding-top: 3px; margin-top: 3px; }
    .lbl-bar { font-family: monospace; letter-spacing: 2px; font-weight: bold; }
    @media print { body { background: #fff; padding: 0; } }
  </style>
  <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;border-bottom:2px solid #087ad9;padding-bottom:6px">
    <h2 style="margin:0;color:#087ad9">${title}</h2>
    <div>تعداد کل برچسب‌ها: <b>${cuts.reduce((s,c)=>s+c.cnt,0)} عدد</b> · ${date}</div>
  </div>
  <div class="label-grid">
    ${labelHtml}
  </div>`);
  w.document.close();
  w.focus();
  setTimeout(()=>w.print(), 300);
}

function listPrintLabels(){
  const rows=listExportRows();
  if(!rows||!rows.length){ toast(webLanguage==='fa'?'هیچ ردیف معتبری در لیست نیست.':'No valid rows in list.', false); return; }
  const title=webLanguage==='fa'?'لیبل قطعات لیست برش':'Cut List Labels';
  const date=new Intl.DateTimeFormat(webLanguage==='fa'?'fa-IR-u-ca-persian':'en-GB',{dateStyle:'medium',timeStyle:'short'}).format(new Date());

  const w=window.open('','_blank');
  if(!w){ toast(webLanguage==='fa'?'پنجره چاپ مسدود است.':'Print popup blocked.', false); return; }

  let labelHtml = '';
  rows.forEach((r, idx)=>{
    for(let i=0; i<r.count; i++){
      labelHtml += `
      <div class="label-card">
        <div class="lbl-head">
          <b>HAFFLER CNC CUTTER</b>
          <span>ردیف #${idx+1} (${i+1}/${r.count})</span>
        </div>
        <div class="lbl-part">قطعه برش خورده (Cut Piece #${idx+1})</div>
        <div class="lbl-size">
          <span class="lbl-len">${r.length} mm</span>
          <span class="lbl-ang">۴۵° / ۴۵°</span>
        </div>
        <div class="lbl-footer">
          <div class="lbl-bar">|||| ||| ||||| |||</div>
          <small>${date}</small>
        </div>
      </div>`;
    }
  });

  w.document.write(`<!doctype html>
  <meta charset="utf-8">
  <title>${title}</title>
  <style>
    @page { size: A4; margin: 10mm; }
    body { font-family: tahoma, sans-serif; direction: rtl; margin: 0; padding: 10px; background: #f8fafc; }
    .label-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(220px, 1fr)); gap: 10px; }
    .label-card { border: 2px solid #111; border-radius: 8px; padding: 8px 10px; background: #fff; page-break-inside: avoid; display: flex; flex-direction: column; justify-content: space-between; }
    .lbl-head { display: flex; justify-content: space-between; font-size: 10px; border-bottom: 1px dashed #666; padding-bottom: 3px; }
    .lbl-part { font-size: 13px; font-weight: bold; margin: 4px 0; color: #000; }
    .lbl-size { display: flex; justify-content: space-between; align-items: baseline; background: #f0f4f8; padding: 4px 6px; border-radius: 4px; margin: 4px 0; }
    .lbl-len { font-size: 18px; font-weight: 900; color: #087ad9; }
    .lbl-ang { font-size: 12px; font-weight: bold; background: #333; color: #fff; padding: 1px 5px; border-radius: 3px; }
    .lbl-footer { display: flex; justify-content: space-between; align-items: center; font-size: 9px; color: #666; border-top: 1px dashed #ccc; padding-top: 3px; margin-top: 3px; }
    .lbl-bar { font-family: monospace; letter-spacing: 2px; font-weight: bold; }
    @media print { body { background: #fff; padding: 0; } }
  </style>
  <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;border-bottom:2px solid #087ad9;padding-bottom:6px">
    <h2 style="margin:0;color:#087ad9">${title}</h2>
    <div>تعداد کل برچسب‌ها: <b>${rows.reduce((s,r)=>s+r.count,0)} عدد</b> · ${date}</div>
  </div>
  <div class="label-grid">
    ${labelHtml}
  </div>`);
  w.document.close();
  w.focus();
  setTimeout(()=>w.print(), 300);
}

function designerCalcCutBreakdown(){
  const cuts=[];
  const fa=webLanguage==='fa';
  designerProject.units.forEach((u,uidx)=>{
    const w=u.w||1200, h=u.h||1400, qty=u.q||1, tpl=u.template||'fixed';
    const uPrefix=designerProject.units.length>1?(fa?`یونیت ${uidx+1} - `:`U${uidx+1} - `):'';
    cuts.push({name:uPrefix+(fa?'فریم عرضی (بالا/پایین)':'Frame Horizontal'),len:w,cnt:2*qty,ang:45,type:'frame',unit:uidx+1});
    cuts.push({name:uPrefix+(fa?'فریم طولی (چپ/راست)':'Frame Vertical'),len:h,cnt:2*qty,ang:45,type:'frame',unit:uidx+1});
    if(u.dividers&&u.dividers.length){
      u.dividers.forEach((d,didx)=>{
        if(d.axis==='h') cuts.push({name:uPrefix+(fa?`کتیبه/وادار افقی #${didx+1}`:`Transom #${didx+1}`),len:Math.max(50,w-120),cnt:1*qty,ang:90,type:'mullion',unit:uidx+1});
        else cuts.push({name:uPrefix+(fa?`وادار عمودی #${didx+1}`:`Mullion #${didx+1}`),len:Math.max(50,h-120),cnt:1*qty,ang:90,type:'mullion',unit:uidx+1});
      });
    }
    const xs=[0,...(u.dividers||[]).filter(d=>d.axis==='v').map(d=>d.pos),1].sort((a,b)=>a-b);
    const ys=[0,...(u.dividers||[]).filter(d=>d.axis==='h').map(d=>d.pos),1].sort((a,b)=>a-b);
    const cells=u.cells||{};
    for(let y=0;y<ys.length-1;y++){
      for(let x=0;x<xs.length-1;x++){
        const type=cells[`${x}:${y}`]||(u.dividers?.length?'fixed':tpl);
        const cellW=Math.round((xs[x+1]-xs[x])*w);
        const cellH=Math.round((ys[y+1]-ys[y])*h);
        if(type.startsWith('sash')||type==='tilt'){
          const sw=Math.max(80,cellW-60), sh=Math.max(80,cellH-60);
          cuts.push({name:uPrefix+(fa?`لنگه پنجره عرضی (${x+1},${y+1})`:`Sash Width (${x+1},${y+1})`),len:sw,cnt:2*qty,ang:45,type:'sash',unit:uidx+1});
          cuts.push({name:uPrefix+(fa?`لنگه پنجره طولی (${x+1},${y+1})`:`Sash Height (${x+1},${y+1})`),len:sh,cnt:2*qty,ang:45,type:'sash',unit:uidx+1});
        } else if(type.startsWith('door')){
          const dw=Math.max(80,cellW-70), dh=Math.max(80,cellH-50);
          cuts.push({name:uPrefix+(fa?`لنگه درب عرضی (${x+1},${y+1})`:`Door Sash Width (${x+1},${y+1})`),len:dw,cnt:2*qty,ang:45,type:'door',unit:uidx+1});
          cuts.push({name:uPrefix+(fa?`لنگه درب طولی (${x+1},${y+1})`:`Door Sash Height (${x+1},${y+1})`),len:dh,cnt:2*qty,ang:45,type:'door',unit:uidx+1});
        }
        const gw=Math.max(50,cellW-80), gh=Math.max(50,cellH-80);
        cuts.push({name:uPrefix+(fa?`زهوار شیشه افقی (${x+1},${y+1})`:`Glazing Bead H (${x+1},${y+1})`),len:gw,cnt:2*qty,ang:45,type:'bead',unit:uidx+1});
        cuts.push({name:uPrefix+(fa?`زهوار شیشه عمودی (${x+1},${y+1})`:`Glazing Bead V (${x+1},${y+1})`),len:gh,cnt:2*qty,ang:45,type:'bead',unit:uidx+1});
      }
    }
  });
  const tbody=$('designerCutTbody');
  if(tbody){
    tbody.innerHTML=cuts.map((c,i)=>`<tr style="border-bottom:1px solid #eee"><td>${i+1}</td><td style="text-align:start;font-weight:600">${c.name}</td><td><b>${c.len}</b></td><td>${c.cnt}</td><td><span style="background:${c.ang===90?'#5856D6':'#007AFF'};color:#fff;padding:2px 7px;border-radius:6px;font-size:11px;font-weight:bold">${c.ang}°</span></td></tr>`).join('');
  }
  return cuts;
}

function designerApplyToCutList(){
  const cuts=designerCalcCutBreakdown();
  if(!cuts||!cuts.length){toast(webLanguage==='fa'?'هیچ قطعه‌ای برای انتقال محاسبه نشد.':'No cut pieces calculated.',false);return}
  if(cuts.length>LIST_MAX){toast(webLanguage==='fa'?'تعداد قطعات بیش از ۱۰۰ ردیف است.':'Cut pieces exceed 100 rows.',false);return}
  for(let i=0;i<LIST_MAX;i++){
    const c=cuts[i],sp=$('sp'+i),cnt=$('cnt'+i),ang=$('ang'+i);
    if(sp) { sp.value=c?c.len:''; sp.dataset.dirty='1'; }
    if(cnt) { cnt.value=c?c.cnt:''; cnt.dataset.dirty='1'; }
    if(ang) { ang.value=c?c.ang:45; ang.dataset.dirty='1'; }
  }
  listVisibleRows=Math.max(1,cuts.length);
  if($('rows')){ $('rows').value=cuts.length; $('rows').dataset.dirty='1'; }
  updateListRows();
  toast(webLanguage==='fa'?`تعداد ${cuts.length} ردیف با موفقیت به لیست برش دستگاه منتقل شد.`:`${cuts.length} cut rows applied to Cutter List.`,true);
  requestPage('list');
}

function designerSetup(){
  const canvas=$('designerCanvas');
  if(!canvas||$('designerDraftTools')) return;
  const tools=document.createElement('div');
  tools.id='designerDraftTools';
  tools.className='designer-tools';
  tools.innerHTML='<button type="button" onclick="designerAddDivider(\'v\')">＋ Vertical</button><button type="button" onclick="designerAddDivider(\'h\')">＋ Horizontal</button><button type="button" onclick="designerDeleteSelected()">Delete selected</button>';
  canvas.after(tools);
  const weld=document.createElement('div');
  weld.className='field'; weld.id='weldField';
  weld.innerHTML='<label>uPVC weld allowance (mm) — Draft only</label><input id="weldAllowance" type="number" min="0" step=".1" placeholder="Enter approved value"><p class="warning">No default is assumed. This value is not a production cut rule and is never sent to the device.</p>';
  tools.after(weld);
  const local=document.createElement('div');
  local.className='card solid'; local.id='designerLocal';
  local.innerHTML='<b>Browser-only draft library</b><div class="actions two"><button class="primary compact" onclick="saveDesignerDraft()">Save draft</button><button class="primary compact gray" onclick="exportDesigner(\'svg\')">SVG</button><button class="primary compact gray" onclick="exportDesigner(\'png\')">PNG</button><button class="primary compact gray" onclick="exportDesigner(\'jpeg\')">JPEG</button><button class="primary compact gray" onclick="exportDesigner(\'json\')">iWinDoor JSON</button><button class="primary compact gray" onclick="printDesigner()">Print / PDF</button></div><p class="warning">Production cut list disabled until an approved rule-set is selected.</p><div class="design-gallery" id="designGallery"></div>';
  weld.after(local);
  ['pointermove','pointerup'].forEach(type=>canvas.addEventListener(type,e=>{
    if(type==='pointermove'&&designerModel.drag){
      const r=$('designFrame').getBoundingClientRect(),p=designerModel.dividers.find(x=>x.id===designerModel.drag);
      if(p){
        const total=p.axis==='v'?designerValues().w:designerValues().h,minGap=80/total,same=designerModel.dividers.filter(x=>x.axis===p.axis&&x.id!==p.id).map(x=>x.pos).sort((a,b)=>a-b),raw=p.axis==='v'?(e.clientX-r.left)/r.width:(e.clientY-r.top)/r.height,prev=Math.max(0,...same.filter(x=>x<p.pos)),next=Math.min(1,...same.filter(x=>x>p.pos));
        p.pos=Math.max(prev+minGap,Math.min(next-minGap,raw));
        renderDesigner();
      }
    }
    if(type==='pointerup') designerModel.drag=null;
  }));
  renderDesignerGallery();
  try{designerCalcCutBreakdown();}catch(e){}
}

function designerAddDivider(axis){
  if(designerModel.dividers.length>=8)return;
  const total=axis==='v'?designerValues().w:designerValues().h,minGap=80/total,used=designerModel.dividers.filter(d=>d.axis===axis).map(d=>d.pos).sort((a,b)=>a-b),points=[0,...used,1];
  let pos=.5,gap=0;
  for(let i=0;i<points.length-1;i++)if(points[i+1]-points[i]>gap){gap=points[i+1]-points[i];pos=(points[i+1]+points[i])/2}
  if(gap<minGap*2){toast(webLanguage==='fa'?'فضای کافی برای بخش حداقل ۸۰ میلی‌متری نیست.':'There is not enough room for another 80 mm section.',false);return}
  designerModel.dividers.push({id:'d'+Date.now(),axis,pos,cell:'fixed'});
  renderDesigner();
}

function designerDeleteDivider(id){
  const idx=designerModel.dividers.findIndex(d=>d.id===id);
  if(idx>=0){designerModel.dividers.splice(idx,1);designerModel.selected=null;renderDesigner()}
}

function designerDeleteSelected(){
  if(!designerModel.selected){toast(webLanguage==='fa'?'ابتدا وادار مورد نظر را انتخاب کنید.':'Select a divider first.',false);return}
  designerDeleteDivider(designerModel.selected);
}

function designerSelectDivider(id){
  designerModel.selected=designerModel.selected===id?null:id;
  renderDesigner();
}

function designerSelectCell(key){
  designerModel.selectedCell=designerModel.selectedCell===key?null:key;
  renderDesigner();
}

function designerAxisSizes(axis,total){
  const list=designerModel.dividers.filter(d=>d.axis===axis).sort((a,b)=>a.pos-b.pos),points=[0,...list.map(d=>d.pos),1],sizes=[];
  for(let i=0;i<points.length-1;i++)sizes.push(Math.round((points[i+1]-points[i])*total));
  return sizes;
}

function designerSetSection(axis,index,value){
  const total=Number(axis==='v'?$('designWidth')?.value:$('designHeight')?.value)||0,min=80,sizes=designerAxisSizes(axis,total),next=Math.round(Number(value));
  if(!Number.isFinite(next)||next<min||index<0||index>=sizes.length)return;
  const delta=next-sizes[index],pair=index<sizes.length-1?index+1:index-1;
  if(pair<0||sizes[pair]-delta<min)return;
  sizes[index]=next;sizes[pair]-=delta;
  const divs=designerModel.dividers.filter(d=>d.axis===axis).sort((a,b)=>a.pos-b.pos);
  let sum=0;divs.forEach((d,i)=>{sum+=sizes[i];d.pos=sum/total});
  renderDesigner();
}

function designerRenderDividers(){
  const f=$('designFrame');if(!f)return;
  f.querySelectorAll('.dynamic-divider').forEach(e=>e.remove());
  designerModel.dividers.forEach(d=>{
    const e=document.createElement('div');
    e.className=`design-divider dynamic-divider ${d.axis}`+(designerModel.selected===d.id?' selected':'');
    if(d.axis==='v'){e.style.left=(d.pos*100)+'%';e.style.top='0';e.style.width='8px';e.style.height='100%';e.style.transform='translateX(-50%)'}
    else{e.style.top=(d.pos*100)+'%';e.style.left='0';e.style.height='8px';e.style.width='100%';e.style.transform='translateY(-50%)'}
    e.onpointerdown=ev=>{ev.stopPropagation();designerModel.drag=d.id;designerSelectDivider(d.id)};
    f.appendChild(e);
  });
}

function designerSetCellType(type){
  if(!designerModel.selectedCell)return;
  designerModel.cells??={};
  designerModel.cells[designerModel.selectedCell]=type;
  renderDesigner();
}

function designerTypeLabel(type,fa=webLanguage==='fa'){
  const labels={fixed:fa?'ثابت':'Fixed',sash:fa?'بازشو':'Sash','sash-left':fa?'تک حالته چپ':'Turn Left','sash-right':fa?'تک حالته راست':'Turn Right',tilt:fa?'دو حالته':'Tilt & Turn',hopper:fa?'کلنگی':'Hopper',door:fa?'درب':'Door','door-left':fa?'درب چپ':'Door Left','door-right':fa?'درب راست':'Door Right',sliding:fa?'کشویی':'Sliding',french:fa?'فرانسوی':'French'};
  return labels[type]||type;
}

function designerSyncMetadata(){
  designerModel.cells??={};
  const changed=designerModel._cellsRef!==designerModel.cells;
  designerModel._cellsRef=designerModel.cells;
  let m=designerModel.cells.__meta;
  const set=(id,v)=>{const e=$(id);if(e&&v!=null)e.value=v};
  if(changed){
    if(m){set('designProjectName',m.project);set('designCustomer',m.customer);set('designProfile',m.profile);set('designGlass',m.glass);set('designColor',m.color);set('designQuantity',m.quantity)}
    else{set('designProjectName','');set('designCustomer','');set('designProfile','');set('designGlass','none');set('designColor','white')}
  }
  const current=designerMetadata();
  if(!m){m=current;designerModel.cells.__meta=m}else Object.assign(m,current);
}

function designerMetadata(){
  return{project:$('designProjectName')?.value?.trim()||'',customer:$('designCustomer')?.value?.trim()||'',profile:$('designProfile')?.value?.trim()||'',glass:$('designGlass')?.value||'none',color:$('designColor')?.value||'white',quantity:Number($('designQuantity')?.value)||1};
}

function designerRenderDimensions(){
  const canvas=$('designerCanvas');if(!canvas)return;
  canvas.querySelectorAll('.dimension-axis-box').forEach(e=>e.remove());
  const f=$('designFrame');if(!f)return;
  const {w,h}=designerValues(),xs=designerAxisSizes('v',w),ys=designerAxisSizes('h',h);
  // Bottom Widths (Exact section dimensions / Section widths (mm))
  const bBox=document.createElement('div');
  bBox.className='dimension-axis-box';
  bBox.title='Exact section dimensions - Section widths (mm)';
  bBox.style.cssText='position:absolute;left:12%;bottom:4px;width:76%;display:flex;justify-content:space-between;gap:4px;direction:ltr';
  xs.forEach((sz,idx)=>{
    const btn=document.createElement('button');
    btn.type='button';
    btn.style.cssText='flex:1;background:#ffffffd9;border:1px solid #087ad9;color:#087ad9;font-weight:bold;font-size:10px;border-radius:4px;padding:2px 4px;cursor:pointer';
    btn.textContent=`↔ ${sz} mm`;
    btn.onclick=()=>{
      const next=prompt(webLanguage==='fa'?`عرض بخش ${idx+1} (mm):`:`Width of bay ${idx+1} (mm):`,sz);
      if(next) designerSetSection('v',idx,next);
    };
    bBox.appendChild(btn);
  });
  canvas.appendChild(bBox);
  // Left Heights (Section heights (mm))
  const lBox=document.createElement('div');
  lBox.className='dimension-axis-box';
  lBox.title='Section heights (mm)';
  lBox.style.cssText='position:absolute;left:2px;top:12%;height:76%;display:flex;flex-direction:column;justify-content:space-between;gap:4px;direction:ltr';
  ys.forEach((sz,idx)=>{
    const btn=document.createElement('button');
    btn.type='button';
    btn.style.cssText='flex:1;background:#ffffffd9;border:1px solid #087ad9;color:#087ad9;font-weight:bold;font-size:10px;border-radius:4px;padding:2px;cursor:pointer;writing-mode:vertical-lr;transform:rotate(180deg)';
    btn.textContent=`↕ ${sz} mm`;
    btn.onclick=()=>{
      const next=prompt(webLanguage==='fa'?`ارتفاع بخش ${idx+1} (mm):`:`Height of bay ${idx+1} (mm):`,sz);
      if(next) designerSetSection('h',idx,next);
    };
    lBox.appendChild(btn);
  });
  canvas.appendChild(lBox);
}

function designerRenderKpis(width,height,q){
  const root=$('designerKpis');if(!root)return;
  const fa=webLanguage==='fa',openings=(designerModel.dividers.filter(d=>d.axis==='v').length+1)*(designerModel.dividers.filter(d=>d.axis==='h').length+1),members=4+designerModel.dividers.length,area=(width*height/1000000).toFixed(2);
  root.innerHTML=`<div><b>${openings}</b><span>${fa?'دهانه':'Openings'}</span></div><div><b>${members*q}</b><span>${fa?'عضو هندسی کل':'Total geometry members'}</span></div><div><b>${area} m²</b><span>${fa?'مساحت هر واحد':'Area per unit'}</span></div>`;
}

function designerRenderCells(){
  const f=$('designFrame');if(!f)return;
  f.querySelectorAll('.dynamic-cell').forEach(e=>e.remove());
  const xs=[0,...designerModel.dividers.filter(d=>d.axis==='v').map(d=>d.pos),1].sort((a,b)=>a-b),ys=[0,...designerModel.dividers.filter(d=>d.axis==='h').map(d=>d.pos),1].sort((a,b)=>a-b),{w,h}=designerValues();
  designerModel.cells??={};
  for(let y=0;y<ys.length-1;y++)for(let x=0;x<xs.length-1;x++){
    const key=`${x}:${y}`,type=designerModel.cells[key]||'fixed',e=document.createElement('div'),size=document.createElement('span'),name=document.createElement('span');
    e.className=`design-cell dynamic-cell cell-${type}`+(designerModel.selectedCell===key?' selected':'');
    e.style.left=(xs[x]*100)+'%';e.style.top=(ys[y]*100)+'%';e.style.width=((xs[x+1]-xs[x])*100)+'%';e.style.height=((ys[y+1]-ys[y])*100)+'%';
    e.innerHTML=designerOpeningMark(type);
    name.className='opening-name';name.textContent=designerTypeLabel(type);
    size.className='cell-size';size.textContent=`${Math.round((xs[x+1]-xs[x])*w)}×${Math.round((ys[y+1]-ys[y])*h)}`;
    e.append(name,size);
    e.onclick=ev=>{ev.stopPropagation();designerModel.selectedCell=key;const s=$('cellType');if(s)s.value=type;renderDesigner()};
    f.appendChild(e);
  }
}

function designerOpeningMark(type){
  const paths={
    fixed:'<path d="M4 4h16v16H4zM4 4l16 16M20 4 4 20" stroke="#a3c4d8" stroke-width="0.8"/>',
    sash:'<path d="M4 4h16v16H4zM20 4 4 12l16 8"/>',
    door:'<path d="M4 2h16v20H4zM20 2 4 12l16 10"/>',
    'sash-left':'<path d="M4 4h16v16H4zM4 4l16 8L4 20" stroke="#087ad9" stroke-width="1.8"/>',
    'sash-right':'<path d="M4 4h16v16H4zM20 4 4 12l16 8" stroke="#087ad9" stroke-width="1.8"/>',
    tilt:'<path d="M4 4h16v16H4zM4 4l16 8L4 20" stroke="#087ad9" stroke-width="1.6"/><path d="M4 4l8 16 8-16" stroke="#087ad9" stroke-width="1.4" stroke-dasharray="3,3"/>',
    hopper:'<path d="M4 4h16v16H4zM4 4l8 16 8-16" stroke="#087ad9" stroke-width="1.8"/>',
    'door-left':'<path d="M4 2h16v20H4zM4 2l16 10L4 22" stroke="#b85f00" stroke-width="1.8"/>',
    'door-right':'<path d="M4 2h16v20H4zM20 2 4 12l16 10" stroke="#b85f00" stroke-width="1.8"/>',
    sliding:'<path d="M4 4h16v16H4zM12 4v16M8 12H5m0 0 2-2m-2 2 2 2m9-2h3m0 0-2-2m2 2-2 2" stroke="#078b74" stroke-width="1.6"/>',
    french:'<path d="M4 4h16v16H4zM12 4v16M4 4l8 8L4 20M20 4l-8 8 8 8" stroke="#087ad9" stroke-width="1.6"/>'
  };
  return `<svg class="opening-mark" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.4" aria-hidden="true">${paths[type]||paths.fixed}</svg>`;
}

function designerSvgMark(type,cx,cy,cw,ch){
  if(type==='sash-left') return `<line x1="${cx}" y1="${cy}" x2="${cx+cw}" y2="${cy+ch/2}" stroke="#087ad9" stroke-width="2"/><line x1="${cx}" y1="${cy+ch}" x2="${cx+cw}" y2="${cy+ch/2}" stroke="#087ad9" stroke-width="2"/>`;
  if(type==='sash-right'||type==='sash') return `<line x1="${cx+cw}" y1="${cy}" x2="${cx}" y2="${cy+ch/2}" stroke="#087ad9" stroke-width="2"/><line x1="${cx+cw}" y1="${cy+ch}" x2="${cx}" y2="${cy+ch/2}" stroke="#087ad9" stroke-width="2"/>`;
  if(type==='tilt') return `<line x1="${cx}" y1="${cy}" x2="${cx+cw}" y2="${cy+ch/2}" stroke="#087ad9" stroke-width="1.8"/><line x1="${cx}" y1="${cy+ch}" x2="${cx+cw}" y2="${cy+ch/2}" stroke="#087ad9" stroke-width="1.8"/><line x1="${cx}" y1="${cy}" x2="${cx+cw/2}" y2="${cy+ch}" stroke="#087ad9" stroke-width="1.4" stroke-dasharray="4,4"/><line x1="${cx+cw}" y1="${cy}" x2="${cx+cw/2}" y2="${cy+ch}" stroke="#087ad9" stroke-width="1.4" stroke-dasharray="4,4"/>`;
  if(type==='hopper') return `<line x1="${cx}" y1="${cy}" x2="${cx+cw/2}" y2="${cy+ch}" stroke="#087ad9" stroke-width="2"/><line x1="${cx+cw}" y1="${cy}" x2="${cx+cw/2}" y2="${cy+ch}" stroke="#087ad9" stroke-width="2"/>`;
  if(type==='door-left') return `<line x1="${cx}" y1="${cy}" x2="${cx+cw}" y2="${cy+ch/2}" stroke="#b85f00" stroke-width="2.2"/><line x1="${cx}" y1="${cy+ch}" x2="${cx+cw}" y2="${cy+ch/2}" stroke="#b85f00" stroke-width="2.2"/>`;
  if(type==='door-right'||type==='door') return `<line x1="${cx+cw}" y1="${cy}" x2="${cx}" y2="${cy+ch/2}" stroke="#b85f00" stroke-width="2.2"/><line x1="${cx+cw}" y1="${cy+ch}" x2="${cx}" y2="${cy+ch/2}" stroke="#b85f00" stroke-width="2.2"/>`;
  if(type==='sliding') return `<line x1="${cx+cw/2}" y1="${cy}" x2="${cx+cw/2}" y2="${cy+ch}" stroke="#078b74" stroke-width="2"/>`;
  return '';
}

function designerSvg(model=designerModel,w=700,h=500){
  const xs=[0,...(model.dividers||[]).filter(d=>d.axis==='v').map(d=>d.pos),1].sort((a,b)=>a-b);
  const ys=[0,...(model.dividers||[]).filter(d=>d.axis==='h').map(d=>d.pos),1].sort((a,b)=>a-b);
  const cells=model.cells||{},pad=28,template=model.template||$('designTemplate')?.value||'fixed',stroke=template==='sliding'?'#078b74':template==='door'?'#b85f00':'#3e5969';
  let body=`<rect width="100%" height="100%" fill="#ffffff"/><rect x="${pad}" y="${pad}" width="${w-pad*2}" height="${h-pad*2}" fill="#f8fafc" stroke="${stroke}" stroke-width="${template==='door'?14:10}"/>`;
  for(let y=0;y<ys.length-1;y++)for(let x=0;x<xs.length-1;x++){
    const k=`${x}:${y}`,t=cells[k]||'fixed',cx=pad+xs[x]*(w-pad*2),cy=pad+ys[y]*(h-pad*2),cw=(xs[x+1]-xs[x])*(w-pad*2),ch=(ys[y+1]-ys[y])*(h-pad*2),door=t.startsWith('door'),sash=t.startsWith('sash')||t==='tilt',fill=door?'#fef5ea':sash?'#eff6fc':t==='sliding'?'#eef8f5':'#ffffff';
    body+=`<rect x="${cx}" y="${cy}" width="${cw}" height="${ch}" fill="${fill}" stroke="#a3b8c8" stroke-width="1.5"/>${designerSvgMark(t,cx,cy,cw,ch)}<text x="${cx+cw/2}" y="${cy+ch-8}" text-anchor="middle" font-size="11" font-weight="bold" fill="#365464">${designerTypeLabel(t,false).toUpperCase()}</text>`;
  }
  return `<svg xmlns="http://www.w3.org/2000/svg" width="${w}" height="${h}" viewBox="0 0 ${w} ${h}">${body}</svg>`;
}

function exportDesigner(kind){
  if(kind==='json'){
    const data=JSON.stringify({project:designerMetadata(),units:designerProject.units,exportedAt:new Date().toISOString(),generator:'Haffler iWinDoor Designer'},null,2);
    downloadBlob(`iwindoor-project-${exportStamp()}.json`,'application/json',data);
    toast(webLanguage==='fa'?'فایل پروژه iWinDoor JSON صادر شد.':'iWinDoor project JSON exported.',true);
    return;
  }
  const svg=designerSvg({...designerModel,template:$('designTemplate').value});
  if(kind==='svg'){downloadBlob(`designer-${exportStamp()}.svg`,'image/svg+xml',svg);return}
  const target=beginAsyncDownload(),img=new Image(),url=URL.createObjectURL(new Blob([svg],{type:'image/svg+xml'}));
  img.onload=()=>{
    const c=document.createElement('canvas');c.width=700;c.height=500;c.getContext('2d').drawImage(img,0,0);
    c.toBlob(b=>deliverBlob(`designer-${exportStamp()}.${kind}`,b,target),kind==='jpeg'?'image/jpeg':'image/png');
    URL.revokeObjectURL(url);
  };
  img.onerror=()=>{URL.revokeObjectURL(url);deliverBlob('',null,target)};
  img.src=url;
}

function designerEnsureCellControl(){
  const tools=$('designerDraftTools');if(!tools||$('cellType'))return;
  const wrap=document.createElement('div'),label=document.createElement('label'),s=document.createElement('select'),del=tools.querySelector('button:nth-of-type(3)');
  wrap.className='field opening-inspector';wrap.id='openingInspector';label.id='cellTypeLabel';label.htmlFor='cellType';
  s.id='cellType';
  s.innerHTML='<option value="fixed">Fixed</option><option value="sash-left">Left-opening sash</option><option value="sash-right">Right-opening sash</option><option value="tilt">Tilt sash</option><option value="hopper">Hopper / Awning</option><option value="door-left">Left-opening door</option><option value="door-right">Right-opening door</option><option value="sliding">Sliding panel</option><option value="french">French sash</option>';
  s.onchange=()=>designerSetCellType(s.value);
  wrap.append(label,s);tools.appendChild(wrap);
  if(del){del.classList.add('delete-divider');del.innerHTML=uiIcon('trash')+'<span>Delete divider</span>'}
  designerLocalizeExtras();
}

function designerValues(){
  return{w:Math.max(100,Math.min(10000,Number($('designWidth')?.value)||1200)),h:Math.max(100,Math.min(10000,Number($('designHeight')?.value)||1400)),q:Math.max(1,Math.min(999,Number($('designQuantity')?.value)||1))};
}

function designerGeometryParts(){
  const w=Math.max(100,Math.min(10000,Math.round(Number($('designWidth')?.value)||1200))),h=Math.max(100,Math.min(10000,Math.round(Number($('designHeight')?.value)||1400))),q=Math.max(1,Math.min(999,Math.round(Number($('designQuantity')?.value)||1))),parts=[{role:'Frame horizontal',size:`${w} mm`,qty:2*q},{role:'Frame vertical',size:`${h} mm`,qty:2*q}];
  designerModel.dividers.filter(d=>d.axis==='v').forEach(()=>parts.push({role:'Vertical divider',size:`${h} mm`,qty:q}));
  designerModel.dividers.filter(d=>d.axis==='h').forEach(()=>parts.push({role:'Horizontal divider',size:`${w} mm`,qty:q}));
  const xs=designerAxisSizes('v',w),ys=designerAxisSizes('h',h),fa=webLanguage==='fa';
  for(let y=0;y<ys.length;y++)for(let x=0;x<xs.length;x++){
    const type=designerModel.cells?.[`${x}:${y}`]||'fixed',opening=(fa?'دهانه ':'Opening ')+(y*xs.length+x+1);
    parts.push({role:`${opening} · ${designerTypeLabel(type,fa)}`,size:`${xs[x]} × ${ys[y]} mm`,qty:q});
  }
  return parts;
}

function designerRenderCutList(){
  const local=$('designerLocal');if(!local)return;
  let box=$('designerCutList');
  if(!box){box=document.createElement('div');box.id='designerCutList';box.className='designer-panel';local.before(box)}
  const fa=webLanguage==='fa',parts=designerGeometryParts(),role=r=>fa?({'Frame horizontal':'قاب افقی','Frame vertical':'قاب عمودی','Vertical divider':'تقسیم عمودی','Horizontal divider':'تقسیم افقی'}[r]||r.replace('Opening','دهانه').replace('fixed','ثابت').replace('sash','بازشو').replace('door','درب')):r;
  box.innerHTML=`<h3>${fa?'لیست قطعات و دهانه‌های هندسی':'Geometric members & openings'}</h3><p class="warning">${fa?'ابعاد هندسی‌اند، نه دستور تولید. پرت جوش، اتصال، میتر و قواعد پروفیل فقط پس از انتخاب Rule Set تأییدشده محاسبه می‌شوند.':'These are geometric dimensions, not production instructions. Weld allowance, joints, mitres and profile rules require an approved rule set.'}</p><div class="designer-cut-table"><table><thead><tr><th>${fa?'قطعه':'Part'}</th><th>${fa?'اندازه هندسی':'Geometry size'}</th><th>${fa?'تعداد':'Qty'}</th></tr></thead><tbody>${parts.map(p=>`<tr><td>${role(p.role)}</td><td>${p.size}</td><td>${p.qty}</td></tr>`).join('')}</tbody></table></div>`;
}

function renderDesigner(){
  const template=$('designTemplate')?.value||'fixed',q=Math.max(1,Math.min(999,Number($('designQuantity')?.value)||1)),width=Math.max(100,Math.min(10000,Number($('designWidth')?.value)||1200)),height=Math.max(100,Math.min(10000,Number($('designHeight')?.value)||1400)),material=$('designMaterial')?.value||'upvc',frame=$('designFrame');
  if(!frame)return;
  designerSyncMetadata();
  designerSyncActiveUnit();
  designerRenderUnitList();
  const ratio=Math.max(.35,Math.min(2.4,width/height)),fw=Math.min(84,Math.max(32,ratio>=1?76:76*ratio)),fh=Math.min(76,Math.max(28,ratio>=1?76/ratio:76));
  frame.className='design-frame template-'+template;
  frame.style.width=fw+'%';frame.style.height=fh+'%';frame.style.left=((100-fw)/2)+'%';frame.style.top=((100-fh)/2)+'%';
  $('dividerV').hidden=true;$('dividerH').hidden=true;
  designerRenderCells();
  designerRenderDividers();
  const weld=$('weldField');if(weld)weld.hidden=material!=='upvc';
  const members=4+designerModel.dividers.length,meta=designerMetadata(),parts=[meta.project,meta.customer,meta.profile].filter(Boolean);
  designerRenderKpis(width,height,q);
  $('designerSummary').textContent=`${parts.length?parts.join(' · ')+' · ':''}${T('template')}: ${T('template'+template)} · ${width} × ${height} mm · ${T('projectQuantity')}: ${q} · ${T('draftMembers')}: ${members} · ${webLanguage==='fa'?'فقط هندسهٔ پیش‌نویس':'Draft geometry only'}`;
  designerRenderDimensions();
  designerRenderCutList();
  designerCalcCutBreakdown();
}

function planLocaleTitle(){return webLanguage==='fa'?'طرح برش':'Cut plan'}
function printPlan(){const d=planSnapshot;if(!d?.complete)return;const w=window.open('','_blank');if(!w){toast(webLanguage==='fa'?'پنجرهٔ چاپ مسدود است.':'Print popup is blocked.',false);return}const title=planLocaleTitle(),date=new Intl.DateTimeFormat(webLanguage==='fa'?'fa-IR-u-ca-persian':'en-GB',{dateStyle:'medium',timeStyle:'short'}).format(new Date()),body=(d.bars||[]).map((b,i)=>`<tr><td>${b.bar||i+1}</td><td>${(b.pieces_mm||[]).join(' + ')}</td><td>${Number(b.waste_mm||0).toFixed(1)}</td></tr>`).join('');w.document.write(`<!doctype html><meta charset="utf-8"><title>${title}</title><h1>${title}</h1><p>${date}</p><p>${webLanguage==='fa'?'راهنما: سبز ≤۱۰٪، زرد ≤۲۵٪، قرمز >۲۵٪':'Legend: green ≤10%, yellow ≤25%, red >25%'}</p><table border="1" cellpadding="7"><tr><th>${webLanguage==='fa'?'شاخه':'Bar'}</th><th>${webLanguage==='fa'?'قطعات (mm)':'Pieces (mm)'}</th><th>${webLanguage==='fa'?'پرت (mm)':'Waste (mm)'}</th></tr>${body}</table>`);w.document.close();w.focus();w.print()}
function printCurrentList(){const rows=listExportRows();if(!rows)return;const w=window.open('','_blank');if(!w){toast(webLanguage==='fa'?'پنجرهٔ چاپ مسدود است.':'Print popup is blocked.',false);return}const title=webLanguage==='fa'?'لیست برش':'Cut list',date=new Intl.DateTimeFormat(webLanguage==='fa'?'fa-IR-u-ca-persian':'en-GB',{dateStyle:'medium',timeStyle:'short'}).format(new Date());w.document.write(`<!doctype html><meta charset="utf-8"><title>${title}</title><h1>${title}</h1><p>${date}</p><table border="1" cellpadding="7"><tr><th>#</th><th>${webLanguage==='fa'?'طول (mm)':'Length (mm)'}</th><th>${webLanguage==='fa'?'تعداد':'Quantity'}</th></tr>${rows.map((r,i)=>`<tr><td>${i+1}</td><td>${r.length}</td><td>${r.count}</td></tr>`).join('')}</table>`);w.document.close();w.focus();w.print()}
function designerSplit(axis){designerAddDivider(axis)}
function designerReset(){
  designerModel.dividers=[];
  designerModel.selected=null;
  designerModel.cells={};
  designerModel.selectedCell=null;
  designerLastValid={w:1200,h:1400};
  $('designTemplate').value='fixed';
  $('designQuantity').value='1';
  $('designWidth').value='1200';
  $('designHeight').value='1400';
  $('designMaterial').value='upvc';
  $('designProjectName').value='';
  $('designCustomer').value='';
  $('designProfile').value='';
  $('designGlass').value='none';
  $('designColor').value='white';
  renderDesigner();
}
function cutValue(d,...keys){for(const k of keys)if(Number.isFinite(Number(d?.[k])))return Number(d[k]);return null}
function reportDate(d){const v=Number(d?.clock_epoch);if(Number.isFinite(v)&&v>0)return new Intl.DateTimeFormat(webLanguage==='fa'?'fa-IR-u-ca-persian':'en-GB',{dateStyle:'medium',timeStyle:'short'}).format(new Date(v*1000));return d?.cuts_day_key||'--'}
function renderReports(d=lastState||{}){const root=$('reportsContent');if(!root)return;const today=[['Manual',cutValue(d,'cuts_day_manual','day_manual')],['List',cutValue(d,'cuts_day_list','day_list')],['Optimized',cutValue(d,'cuts_day_opt','day_opt')]],life=[['Manual',cutValue(d,'cuts_life_manual','tcuts')],['List',cutValue(d,'cuts_life_list')],['Optimized',cutValue(d,'cuts_life_opt')]];const fa=webLanguage==='fa',label=x=>fa?({Manual:'دستی',List:'لیست',Optimized:'بهینه'}[x]):x,cell=(a)=>a.map(x=>`<div class="stat-box"><b>${x[1]??'--'}</b><span>${label(x[0])}</span></div>`).join('');root.innerHTML=`<p class="hint">${fa?'تاریخ':'Date'}: ${reportDate(d)}</p><h3>${fa?'امروز':'Today'}</h3><div class="report-grid">${cell(today)}</div><h3>${fa?'عمر دستگاه':'Lifetime'}</h3><div class="report-grid">${cell(life)}</div><p class="warning">${fa?'تاریخچهٔ ۳۰روزه هنوز در دستگاه ذخیره نمی‌شود.':'30-day history is not stored on the device yet.'}</p>`}
function ensureReports(){if($('reports'))return;const s=document.createElement('section');s.className='page';s.id='reports';s.innerHTML='<div class="head"><h2>Reports</h2></div><div class="card solid" id="reportsContent"></div>';document.querySelector('main').appendChild(s);const nav=$('menu');const b=document.createElement('button');b.dataset.page='reports';b.onclick=()=>requestPage('reports');b.textContent='▤ Reports';nav?.appendChild(b)}
const portalCopy={fa:{production:'تولید و لیست‌ها',productionDesc:'لیست برش، بهینه‌سازی و گزارش‌ها',designer:'طراحی در و پنجره',designerDesc:'تقسیم‌بندی، اندازه‌گذاری و لیست قطعات',settings:'تنظیمات دستگاه',settingsDesc:'عمومی، موتور و مدیریت',diagnostics:'وضعیت و عیب‌یابی',diagnosticsDesc:'ورودی‌ها، آمار و اطلاعات دستگاه',support:'پشتیبانی هوشمند',supportDesc:'راهنمای آفلاین فارسی و انگلیسی',cutList:'لیست برش',cutListDesc:'ورود، ویرایش، بررسی و ذخیره متراژها',plan:'لیست بهینه',planDesc:'نمایش طرح برش و پرت شاخه‌ها',reports:'گزارش‌ها',reportsDesc:'آمار امروز و عمر دستگاه',general:'تنظیمات عمومی',generalDesc:'شاخه، تیغه، زبان، صدا و کیبورد',motor:'حرکت و محدوده',motorDesc:'ابتدا و انتهای خط، سرعت و شیب',admin:'مدیریت و امنیت',adminDesc:'اره، توقف، جاگ و رمزهای دسترسی',inputs:'ورودی‌های زنده',inputsDesc:'نمایش فقط‌خواندنی سنسورها و توقف',device:'اطلاعات دستگاه',deviceDesc:'نسخه و شمارنده‌های برش',supportTitle:'پشتیبانی آفلاین دستگاه',supportNotice:'این راهنما بدون اینترنت کار می‌کند. برای ایمنی هیچ خروجی، حرکت، برش یا تنظیمی را خودکار اجرا نمی‌کند.',supportPlaceholder:'مثلاً: تیغه کار نمی‌کند یا سایز را چطور وارد کنم؟',askBlade:'تیغه کار نمی‌کند',askSensor:'سنسور اول خط چیست؟',askSize:'چطور سایز وارد کنم؟',askSpeed:'سرعت دستگاه مناسب نیست',askWarranty:'گارانتی دستگاه',update:'به‌روزرسانی دستگاه',help:'راهنمای کامل'},en:{production:'Production & lists',productionDesc:'Cut list, optimization and reports',designer:'Window & door design',designerDesc:'Layout, dimensions and parts list',settings:'Machine settings',settingsDesc:'General, motor and manager controls',diagnostics:'Status & diagnostics',diagnosticsDesc:'Inputs, counters and device information',support:'Smart support',supportDesc:'Offline Persian and English guidance',cutList:'Cut list',cutListDesc:'Enter, review and save lengths',plan:'Optimized plan',planDesc:'Review stock bars and waste',reports:'Reports',reportsDesc:'Today and lifetime statistics',general:'General settings',generalDesc:'Stock, blade, language, sound and keyboard',motor:'Travel & limits',motorDesc:'Line limits, speed and ramp',admin:'Manager & security',adminDesc:'Saw, stop, jog and access PINs',inputs:'Live inputs',inputsDesc:'Read-only sensors and stop state',device:'Device information',deviceDesc:'Version and cut counters',supportTitle:'Offline machine support',supportNotice:'This guide works without internet. For safety it never triggers outputs, motion, cutting or automatic setting changes.',supportPlaceholder:'For example: blade is not working or how do I enter a size?',askBlade:'Blade is not working',askSensor:'What is the first-line sensor?',askSize:'How do I enter a size?',askSpeed:'Machine speed is wrong',askWarranty:'Device warranty',update:'Device update',help:'Full help'}};
Object.assign(portalCopy.fa,{inputs:'ورودی و خروجی زنده',inputsDesc:'نمایش کامل و فقط‌خواندنی I0 تا T2 و Q0 تا Q9'});
Object.assign(portalCopy.en,{inputs:'Live inputs & outputs',inputsDesc:'Complete read-only I0–T2 and Q0–Q9 diagnostics'});
function hubCard(go,icon,title,desc){const icons={cutList:'cut',plan:'report',reports:'report',general:'info',motor:'motion',admin:'guard',inputs:'input',device:'status'};return`<button class="hub-card" type="button" onclick="requestPage('${go}')"><span class="hub-icon" aria-hidden="true">${uiIcon(icons[title]||'window')}</span><b data-portal-key="${title}"></b><span data-portal-key="${desc}"></span></button>`}
function ensurePortalSuite(){if($('productionHub'))return;const main=document.querySelector('main'),make=(id,title,html)=>{const s=document.createElement('section');s.className='page';s.id=id;s.innerHTML=`<div class="head"><h2 data-portal-key="${title}"></h2></div><div class="card solid"><div class="portal-hub">${html}</div></div>`;main.appendChild(s)};make('productionHub','production',hubCard('list','✂','cutList','cutListDesc')+hubCard('plan','▤','plan','planDesc')+hubCard('reports','◫','reports','reportsDesc'));make('settingsHub','settings',hubCard('f1','⚙','general','generalDesc')+hubCard('f2','↔','motor','motorDesc')+hubCard('f4','◇','admin','adminDesc'));make('diagnosticsHub','diagnostics',hubCard('f3','◉','inputs','inputsDesc')+hubCard('f5','ⓘ','device','deviceDesc'));const s=document.createElement('section');s.className='page';s.id='support';s.innerHTML='<div class="head"><h2 data-portal-key="supportTitle"></h2></div><div class="card solid support-shell"><div><div class="support-hero"><span class="support-avatar" aria-hidden="true">✦</span><div><b id="supportAgentTitle">Machine guide</b><small id="supportAgentHint">Ask naturally; short and incomplete questions are welcome.</small></div><span class="support-local" id="supportLocalBadge">OFFLINE · SAFE</span></div><div class="support-notice" data-portal-key="supportNotice"></div><div class="support-chips"><button id="supportQuickStatus" onclick="supportAsk(webLanguage===\'fa\'?\'وضعیت زنده دستگاه را بگو\':\'tell me the live machine status\')">◉ Live status</button><button id="supportQuickTrouble" onclick="supportAsk(webLanguage===\'fa\'?\'برای عیب یابی کمکم کن\':\'help me troubleshoot\')">⌕ Troubleshoot</button><button id="supportQuickLearn" onclick="supportAsk(webLanguage===\'fa\'?\'چه کارهایی می توانی انجام بدهی\':\'what can you do\')">? What can you do?</button><button id="supportQuickReport" onclick="supportAsk(webLanguage===\'fa\'?\'گزارش وضعیت بساز\':\'create a status report\')">↓ Create report</button></div></div><div class="support-chat" id="supportChat" aria-live="polite"></div><div><div class="support-compose"><input id="supportInput" maxlength="180" autocomplete="off" inputmode="text" spellcheck="true"><button type="button" id="supportSendButton" aria-label="Send" onclick="supportSend()">➤</button></div><div class="support-service"><button class="primary compact gray" id="supportServiceStatus" onclick="supportAsk(webLanguage===\'fa\'?\'وضعیت ورودی ها را نشان بده\':\'show live inputs\')">◉ Live inputs</button><button class="primary compact blue" onclick="helpOpen(\'all\')" data-portal-key="help"></button></div></div></div>';main.appendChild(s);$('supportInput').addEventListener('keydown',e=>{if(e.key==='Enter'){e.preventDefault();supportSend()}});supportWelcome()}
function groupMenu(){const nav=$('menu');if(!nav||nav.dataset.grouped)return;nav.dataset.grouped='1';const items=[['home','window','home'],['productionHub','cut','production'],['designer','window','designer'],['settingsHub','power','settings'],['diagnosticsHub','status','diagnostics'],['support','help','support']];nav.textContent='';items.forEach(([page,icon,key],i)=>{const b=document.createElement('button');b.type='button';b.dataset.page=page;b.onclick=()=>requestPage(page);b.innerHTML=`<span class="nav-icon" aria-hidden="true">${uiIcon(icon)}</span><span data-portal-key="${key}"></span>`;if(!i)b.classList.add('on');nav.appendChild(b)})}
function localizePortalSuite(){const c=portalCopy[webLanguage]||portalCopy.en;document.querySelectorAll('[data-portal-key]').forEach(e=>{e.textContent=c[e.dataset.portalKey]||T(e.dataset.portalKey)});const input=$('supportInput');if(input)input.placeholder=c.supportPlaceholder;localizeSupportChrome();supportDecorateChrome();decorateGeneralIcons();document.querySelectorAll('.back-page').forEach(b=>b.setAttribute('aria-label',webLanguage==='fa'?'بازگشت':'Back'));designerLocalizeExtras();hydrateIcons();designerRenderDimensions();designerRenderCutList()}
function localizeSupportChrome(){const fa=webLanguage==='fa',set=(id,a,b)=>{const e=$(id);if(e)e.textContent=fa?a:b};set('supportAgentTitle','راهنمای دستگاه','Machine guide');set('supportAgentHint','طبیعی بنویسید؛ سؤال کوتاه یا ناقص هم قابل بررسی است.','Ask naturally; short or incomplete questions are welcome.');set('supportLocalBadge','آفلاین · ایمن','OFFLINE · SAFE');set('supportQuickStatus','◉ وضعیت زنده','◉ Live status');set('supportQuickTrouble','⌕ عیب‌یابی','⌕ Troubleshoot');set('supportQuickLearn','؟ چه کمکی می‌کنی؟','? What can you do?');set('supportQuickReport','↓ ساخت گزارش','↓ Create report');set('supportServiceStatus','◉ ورودی‌های زنده','◉ Live inputs');const b=$('supportSendButton');if(b){b.setAttribute('aria-label',fa?'ارسال':'Send');b.title=fa?'ارسال':'Send'}}
const supportIntents=[
{id:'hello',a:['سلام','درود','صبح بخیر','عصر بخیر','hello','hi','hey','good morning','وقت بخیر'],fa:'سلام، خوش آمدید. من راهنمای هوشمند و آفلاین همین دستگاه هستم. می‌توانم در عیب‌یابی مکانیک، برق، سنسورها، اره، لیست برش، طراحی در و پنجره و تنظیمات گام‌به‌گام کمکتان کنم. سؤالتان را بنویسید.',en:'Hello and welcome. I am this machine’s offline smart guide. I can help with mechanical, electrical, sensor, saw, cut list, window design, and settings troubleshooting. Ask any question.'},
{id:'blade',a:['تیغ','تیغه','اره','دیسک','اسپیندل','تیغه اره','blade','saw','cuter','cutter','برش کج','لنگی تیغ','کندی تیغ','زاویه اره','فارسی بر','اره بالا نمیاد','اره پایین نمیره','روشن نشدن اره','تیغه کار نمیکنه'],b:['کار نم','خراب','روشن نم','not work','broken','doesnt','گیر'],fa:'برای ایمنی، هنگام بررسی اره دستگاه باید کاملاً متوقف باشد. وضعیت «اره (Q6)»، «محافظ» و «توقف اضطراری» را در بخش وضعیت و عیب‌یابی ببینید. اگر فرمان اره روشن است اما موتور اره حرکت نمی‌کند، برق ۳ فاز قدرت، بی متال و کنتاکتور را چک کنید.',en:'For safety, keep machine stopped. Check Saw (Q6), Guard, and Emergency Stop in Live Diagnostics. If command is on but blade does not rotate, check 3-phase power, contactor, and thermal overload.'},
{id:'pneumatic',a:['جک','کلمپ','گیره','جک نگهدارنده','جک محافظ','فشار باد','رگلاتور','شیر برقی','روغن زن','پکینگ','باد کمه','گیره نگه نمیداره','کلمپ ول نمیکنه','جک گیر کرده','پنوماتیک','کمپرسور باد','clamp','pneumatic','air pressure'],fa:'فشار باد سیستم پنوماتیک باید حداقل ۶ بار (Bar) باشد. وضعیت رگلاتور، واحد مراقبت باد، روغن زن و شیرهای برقی جک نگهدارنده (Q3) و جک محافظ (Q4) را بررسی کنید. در صورت نشتی یا لقی، پکینگ‌های جک نیاز به سرویس دارند.',en:'Air pressure must be at least 6 Bar. Check regulator, FRL unit, lubricator, and solenoid valves for clamp (Q3) and guard (Q4). If leakage occurs, inspect cylinder packings.'},
{id:'spray',a:['اسپری','اب صابون','آب صابون','خنک کننده','پاشش','مایع خنک کننده','اسپری نمیزنه','نازل اسپری','روغن برش','پمپ اب صابون','spray','coolant'],fa:'سیستم پاشش خنک‌کننده (Q5) برای افزایش طول عمر تیغه و کیفیت برش آلومینیوم و UPVC است. سطح مایع مخزن، تمیزی فیلتر و باز بودن نازل پاشش را کنترل کنید. خروجی Q5 هنگام کورس برش اره فعال می‌شود.',en:'Coolant spray (Q5) lubricates and cools the blade during cutting. Check fluid level, nozzle cleanliness, and air-mist feed. Q5 activates during the saw cutting stroke.'},
{id:'sensor',a:['سنسور اول','اول خط','رفرنس','مرجع','first line','reference sensor','home sensor','نقطه صفر','سنسور القایی','سنسور مگنت','سنسور هوم','سنسور مبدا'],fa:'سنسور ابتدای خط (I0) نقطه مرجع محور است و دستگاه با آن موقعیت صفر/مبدأ را می‌شناسد. وضعیتش در «ورودی‌های زنده» فقط‌خواندنی دیده می‌شود. فاصله استاندارد سنسور القایی تا آهن ۲ تا ۳ میلی‌متر است و چراغ LED پشت سنسور باید با رسیدن بازو روشن شود.',en:'First-line sensor (I0) is the axis reference to establish origin. State is visible in Live Inputs. Standard inductive gap is 2-3mm, and its LED indicator should illuminate when the carriage arrives.'},
{id:'endSensor',a:['سنسور آخر','انتهای خط','لیمیت آخر','up limit','end sensor','end limit','میکروسوئیچ اخر','محدوده کورس'],fa:'سنسور انتهای خط (I1) حد فیزیکی مسیر و نقطه انتهایی حرکت است. در صورت فعال شدن، حرکت به سمت جلو قطع می‌شود. وضعیت آن در ورودی‌های زنده قابل مشاهده است.',en:'End sensor (I1) is the physical travel limit. When activated, forward travel halts. Inspect its live state under Live Inputs.'},
{id:'motor',a:['موتور','استپر','سروو','درایور','پالس','لیداسکرو','بال اسکرو','تسمه','کوپلینگ','موتور حرکت نمیکنه','موتور میلرزه','استپ موتور','صدای موتور','لرزش محور','رد کردن استپ','motor','stepper','driver','pulse','step loss'],fa:'حرکت محور توسط استپر موتور و درایور کنترل می‌شود. اگر موتور صدا می‌دهد یا رد می‌کند: ۱) روان بودن ریل و بال‌اسکرو را چک کنید، ۲) جریان درایور (Current Dip-switches) و ولتاژ منبع تغذیه (۲۴ یا ۴۸ ولت) را بررسی کنید، ۳) شیب شتاب (Ramp) را در F2 متناسب با وزن بار تنظیم کنید.',en:'Axis motion is driven by stepper and driver. If motor stalls or vibrates: 1) Verify linear rails and ball screw lubrication, 2) Check driver current dip-switches and DC voltage, 3) Adjust acceleration ramp in F2.'},
{id:'size',a:['سایز','اندازه','متراژ','طول وارد','enter size','dimension','length input','2000','وارد کردن متراژ'],fa:'برای یک برش دستی، متراژ را روی HMI یا کادر مقصد وارد کنید. برای چند برش، از «تولید و لیست‌ها ← لیست برش» طول (mm) و تعداد را وارد کنید. مقادیر صفر یا خارج از کورس پذیرفته نمی‌شوند.',en:'For a manual cut, enter destination on HMI. For multiple cuts, use Production & lists → Cut list with length (mm) and quantity. Zero or out-of-range values are rejected.'},
{id:'speed',a:['سرعت','تنظیم سرعت','سرعت موتور','سرعت جاگ','سرعت خودکار','سرعت برش','تند','کند','تندتر','کندتر','slow','fast','speed','jog speed','travel speed','speed setting','motor speed'],fa:'سرعت حرکت خودکار از «تنظیمات دستگاه ← حرکت و محدوده (F2)» و سرعت جاگ دستی از «مدیریت و امنیت ← پیشرفته» تنظیم می‌شود. توصیه می‌شود سرعت را بالاتر از حد توان مکانیکی ریل نبرید.',en:'Automatic speed is configured in Machine settings → Travel & limits (F2), and manual jog speed in Manager → Advanced. Avoid exceeding mechanical limits.'},
{id:'keyboard',a:['کیبورد','صفحه کلید','کیبرد لمسی','نام پد','تایپ نمیکنه','عدد نمیشه زد','کیبورد usb','نام لاک','numlock','صفحه لمسی','تاچ کار نمیکنه','keyboard','cannot type','touchscreen'],fa:'کیبورد لمسی و USB پشتیبانی می‌شود. در تنظیمات F1 حالت کیبورد را بررسی کنید. در صورت اتصال کیبورد فیزیکی USB، کلید NumLock را روشن کنید تا اعداد فعال باشند.',en:'Touch and USB keyboards are supported. Check keyboard settings in F1. When using a USB keyboard, ensure NumLock is active.'},
{id:'warranty',a:['گارانتی','ضمانت','چقدر از گارانتی','چند روز گارانتی','گارانتی مانده','warranty','guarantee'],fa:'روزهای باقی‌مانده قفل زمانی دستگاه در صفحه F5 و اطلاعات سیستم نمایش داده می‌شود. برای گارانتی رسمی و خدمات پس از فروش، فاکتور و کارت ضمانت هافلر مبنا است.',en:'Remaining time-lock days are shown in F5 and system info. For warranty service, the official Haffler invoice is authoritative.'},
{id:'contact',a:['شماره پشتیبانی','شماره تماس','تلفن پشتیبانی','ایمیل پشتیبانی','ایمیل هافلر','سایت سازنده','تماس با سازنده','support phone','contact support','email support','manufacturer contact'],fa:'📞 پشتیبانی فنی و خدمات هافلر (Haffler)
تلفن: 09131697476
📧 ایمیل: haffler.com@gmail.com
🌐 وب‌سایت: haffler.com',en:'📞 Haffler Support & Service
Phone: 09131697476
📧 Email: haffler.com@gmail.com
🌐 Web: haffler.com'},
{id:'pin',a:['رمز','پین','pin','password','رمز ادمین','رمز مدیر'],fa:'ذخیره تنظیمات و لیست برش نیازی به رمز ندارد. فقط نصب و ارتقای Firmware از طریق وب به رمز مدیر دستگاه (پیش‌فرض روی LCD) نیاز دارد.',en:'Saving settings and cut list requires no password. Only firmware installation via web requires the device manager PIN.'},
{id:'list',a:['لیست','لیست برش','لیست بسازم','csv','excel','اکسل','cut list','import','فایل برش'],fa:'در لیست برش می‌توانید تا ۱۰۰ ردیف متراژ وارد کنید یا فایل CSV/Excel خروجی نرم‌افزارهای پنجره (مستروین، آی‌وین‌دور و...) را وارد نمایید. پس از بازبینی، کلید «ذخیره لیست» را بزنید.',en:'In Cut list, enter up to 100 rows or import CSV/Excel from window design software (MasterWin, iWinDoor, etc.). Click Save List after review.'},
{id:'update',a:['آپدیت','به روز','بروزرسانی','firmware','update','اپدیت','ابدیت','دانلود برنامه','نصب برنامه','ارتقا نرم افزار','برنامه تک سر','برنامه کانفیگ'],fa:'در صفحه به‌روزرسانی می‌توانید فایل .bin را از گوشی بارگذاری کنید یا مستقیماً از طریق اینترنت، برنامه «اره تک‌سر» یا «کانفیگ» را از سرور هافلر دانلود و نصب کنید. دستگاه باید کاملاً متوقف باشد.',en:'In the Update page, you can upload a .bin firmware from phone/PC, or update online directly from Haffler server (Cutter or Config firmware). Machine must be fully stopped.'},
{id:'designerHelp',a:['طراحی پنجره','طراحی در','تقسیم پنجره','لیست قطعات','window design','door design','parts list','مستر وین','مستروین','ای وین دور','iwindoor','masterwin','پروفیل upvc','مولیون','ساش','شیشه'],fa:'بخش طراح پنجره و درب امکان طراحی انواع یونیت‌های ثابت، لولایی، دوحالته و کشویی با فرمت استاندارد مستروین و آی‌وین‌دور را فراهم می‌کند و قطعات برش فریم، لنگه، مولیون و زهوار را محاسبه و مستقیم به لیست برش دستگاه منتقل می‌کند.',en:'Window/door designer lets you model Fixed, Sash, Tilt-Turn and Sliding units with MasterWin/iWinDoor standards, calculating Frame, Sash, Mullion, and Bead cut lists with 1-click transfer to machine.'},
{id:'estop',a:['استاپ','توقف اضطراری','ایمرجنسی','emergency stop','e stop','estop','قارچی'],fa:'کلید توقف اضطراری (قارچی) باید همیشه در وضعیت سالم باشد. در صورت فعال بودن I3، تمام خروجی‌های موتور و اره بی‌درنگ قطع می‌شوند. سیم‌کشی استاندارد به صورت NC (Normally Closed) است.',en:'Emergency Stop (I3) instantly cuts off motor and saw outputs. Standard wiring is NC (Normally Closed).'},
{id:'connection',a:['وای فای','وایفا','اتصال وب','آفلاین','wifi','wi fi','offline','connection','هات اسپات','مودم'],fa:'برای استفاده از وب‌اپ، با گوشی یا تبلت به وای‌فای اختصاصی دستگاه وصل شوید و مرورگر را باز کنید. در صورت اتصال به اینترنت از طریق هات‌اسپات، به‌روزرسانی آنلاین نیز فعال است.',en:'Connect phone or PC to the machine Wi-Fi access point and open browser. When hotspot has internet, cloud OTA updates are also available.'},
{id:'optimization',a:['بهینه سازی','پرت شاخه','لیست بهینه','optimize','optimization','waste','پرت گیری'],fa:'صفحه «لیست بهینه» قطعات لیست برش را در شاخه‌های ۶ متری با کمترین دورریز و پرت می‌چیند. ضخامت تیغه (Kerf) و پرت ابتدای شاخه (Trim) در محاسبه لحاظ می‌شوند.',en:'Optimized Plan arranges pieces into 6m stock bars to minimize waste, taking into account blade kerf and start trim.'},
{id:'guard',a:['محافظ','گارد','ایمنی تیغه','guard','blade guard','درب محافظ'],fa:'درب و جک محافظ (I2 / Q4) ضامن ایمنی دست اپراتور در حین کورس اره است. تا زمانی که سنسور محافظ بسته بودن کاور را تأیید نکند، اره فعال نخواهد شد.',en:'Guard cover and sensor (I2 / Q4) protect operator hands. The saw will not start unless the guard cover closure is verified.'},
{id:'calibration',a:['کالیبره','کالیبراسیون','هر استپ','میکرومتر پالس','calibration','mm per step','pulse','دقت متراژ','اندازه دقیق نیست'],fa:'کالیبراسیون برای تطبیق دقیق گام بال‌اسکرو با پالس موتور است. در F4 می‌توانید با تست مسافت بین سنسور اول و آخر، نسبت پالس به میلی‌متر را کالیبره کنید تا خطا به صفر برسد.',en:'Calibration aligns ball screw pitch with stepper pulses. In F4 you can calibrate pulse-per-mm between sensors to eliminate length errors.'},
{id:'timers',a:['تایمر','روز شمار','قفل زمانی','timer','day lock','time lock','تمدید دستگاه','کد فعال سازی'],fa:'زمان آماده‌سازی اره و روزشمار قفل در منوی مدیریت (F4) تعریف شده‌اند. تمدید قفل زمانی با کدهای تأییدشده هافلر صورت می‌پذیرد.',en:'Saw warm-up timing and time-lock counters are in Manager settings (F4). Time extension uses approved Haffler security codes.'},
{id:'screenshot',a:['عکس صفحه','اسکرین شات','دانلود عکس','screenshot','screen image'],fa:'از صفحه اصلی وب روی «دانلود عکس صفحه نمایش» کلیک کنید تا تصویر زنده ۴۸۰×۳۲۰ با فرمت BMP دریافت شود.',en:'Click Download display screenshot on Web Home to retrieve the live 480x320 BMP screen frame.'},
{id:'capabilities',a:['چه کارهایی','چه کمکی','چیکار میتونی','چکار می توانی','راهنمایی کن','کمکم کن','what can you do','how can you help','help me'],fa:'من می‌توانم: ۱) عیب‌یابی مکانیک، برق، اره و سنسورها، ۲) راهنمایی کامل لیست برش و اکسل، ۳) طراحی در و پنجره و انتقال قطعات، ۴) بررسی وضعیت زنده و گزارش‌گیری متنی را انجام دهم.',en:'I can: 1) Troubleshoot mechanics, power, saw, and sensors, 2) Guide cut list & CSV imports, 3) Assist window/door design and cut transfer, 4) Provide live status & text reports.'}
];
const supportOpeners={fa:{problem:['🔧 باشه. قدم‌به‌قدم جلو می‌رویم.','🔧 متوجه شدم. اول یک بررسی ساده انجام می‌دهیم.'],how:['📖 حتماً. این کار را این‌طور انجام دهید:','📖 بله. از این راه بروید:'],info:['💡 کوتاه و ساده:','💡 ساده توضیح می‌دهم:']},en:{problem:['🔧 Okay. We will go one step at a time.','🔧 I understand. Let us start with one simple check.'],how:['📖 Sure. Do it this way:','📖 Yes. Use this path:'],info:['💡 Short and simple:','💡 I will explain it simply:']}};
function supportNorm(s){return String(s||'').toLowerCase().replace(/[\u0660-\u0669\u06f0-\u06f9]/g,c=>String(c.charCodeAt(0)%16)).replace(/[آأإٱ]/g,'ا').replace(/[يىئ]/g,'ی').replace(/ك/g,'ک').replace(/[ؤ]/g,'و').replace(/[ۀة]/g,'ه').replace(/[ًٌٍَُِّْ]/g,'').replace(/[ـ‌‍]/g,' ').replace(/نمی ?کنه/g,'نمی کند').replace(/نمی ?شه/g,'نمی شود').replace(/نمی ?ره/g,'نمی رود').replace(/چطوری/g,'چگونه').replace(/[?؟!.,،:;()\[\]{}"']/g,' ').replace(/(.)\1{2,}/g,(m,c)=>/\d/.test(m)?m:c).replace(/\s+/g,' ').trim()}
function supportNearToken(a,b){if(a===b)return true;const al=a.length,bl=b.length;if(Math.min(al,bl)<3||Math.abs(al-bl)>1)return false;if(al===bl){let p=-1,count=0;for(let i=0;i<al;i++)if(a[i]!==b[i]){p=p<0?i:p;count++}if(count===1)return true;return count===2&&p+1<al&&a[p]===b[p+1]&&a[p+1]===b[p]}let x=al>bl?a:b,y=al>bl?b:a,i=0,j=0,skip=0;while(i<x.length&&j<y.length){if(x[i]===y[j]){i++;j++;continue}if(++skip>1)return false;i++}return true}
function supportFuzzyAlias(q,a){const qt=q.split(' ').filter(Boolean),at=a.split(' ').filter(Boolean);if(!qt.length||!at.length)return 0;if(at.length===1){const near=qt.some(t=>t!==at[0]&&supportNearToken(t,at[0]));return near?(qt.length===1?2:.75):0}for(let i=0;i<=qt.length-at.length;i++){let fuzzy=0,ok=true;for(let j=0;j<at.length;j++){if(qt[i+j]===at[j])continue;if(supportNearToken(qt[i+j],at[j]))fuzzy++;else{ok=false;break}}if(ok&&fuzzy>0&&fuzzy<=Math.max(1,Math.ceil(at.length/2)))return fuzzy===1?1.5:1.25}return 0}
function supportWelcome(){if(!$('supportChat')||$('supportChat').children.length)return;const fa=webLanguage==='fa';supportAdd('bot',fa?'سلام، من راهنمای همین دستگاه هستم.\n\nمشکل را کوتاه بنویسید؛ غلط املایی هم اشکالی ندارد. اگر نمی‌دانید چه بنویسید، یکی از دکمه‌ها را بزنید.\n\nمن دستگاه را حرکت نمی‌دهم و برش یا تنظیمی اجرا نمی‌کنم.':'Hello, I am this machine’s guide.\n\nWrite the problem in a short sentence; spelling mistakes are okay. If you are unsure what to write, tap one button.\n\nI do not move the machine or run cutting or setting changes.',[{label:fa?'⌕ یک مشکل دارم':'⌕ I have a problem',q:fa?'برای عیب یابی کمکم کن':'help me troubleshoot'},{label:fa?'◉ وضعیت دستگاه':'◉ Machine status',q:fa?'وضعیت زنده دستگاه را بگو':'tell me the live machine status'},{label:fa?'؟ روش کار':'? How to use it',q:fa?'چه کارهایی می توانی انجام بدهی':'what can you do'}])}
function supportAskKey(key){supportAsk((portalCopy[webLanguage]||portalCopy.en)[key])}
function supportAsk(q){const input=$('supportInput');if(input)input.value=q;supportSend()}
const SUPPORT_HISTORY_MAX=40;
let supportLastIntent='',supportLastLang='fa',supportContext=[];
const supportDetail={blade:{fa:'بررسی مرحله‌ای:\n۱) توقف اضطراری و محافظ را بررسی کنید.\n۲) در ورودی‌های زنده ببینید وضعیت محافظ درست تغییر می‌کند.\n۳) بدون تست خروجی، فقط نشانگر اره را هنگام فرمان اپراتور مشاهده کنید.\n۴) اگر نشانگر فرمان دارد ولی موتور خاموش است، بررسی برق قدرت فقط با تکنسین مجاز است.',en:'Step by step:\n1) Check emergency stop and guard.\n2) Observe the guard input under Live inputs.\n3) Without output testing, observe only the saw command indicator during an authorized command.\n4) If command is present but the motor is off, power-side inspection is for a qualified technician.'},sensor:{fa:'سنسور اول مرجع مختصات محور است. تغییر وضعیت آن باید در ورودی زنده دیده شود. آلودگی، فاصله مکانیکی و اتصال باید در حالت بی‌برق توسط تکنسین بررسی شود؛ تغییر قطبیت یا بای‌پس از چت مجاز نیست.',en:'The first sensor establishes the axis coordinate reference. Its transition must be visible in Live inputs. A technician may inspect contamination, mechanical gap and wiring while safely powered down; this chat never changes polarity or bypasses it.'},size:{fa:'تک‌اندازه: روی HMI مقصد را وارد، بازبینی و سپس طبق روال دستگاه اجرا کنید. چنداندازه: لیست برش را باز کنید، طول mm و تعداد صحیح بدهید، خطاهای بازه را رفع کنید و در توقف کامل ذخیره کنید. فایل نیز CSV/TSV/TXT تا ۲۵۶KiB پذیرفته می‌شود.',en:'Single size: enter and review the destination on the HMI, then use the normal device workflow. Multiple sizes: open Cut list, enter millimetres and integer quantities, fix range errors, and save while fully stopped. CSV/TSV/TXT up to 256 KiB is supported.'},speed:{fa:'ابتدا مشخص کنید حرکت خودکار کند/تند است یا جاگ. مقدار فعلی را یادداشت کنید، تغییر کوچک بدهید، ذخیره کنید و بعد روی دستگاه بی‌بار ارزیابی کنید. تغییر بزرگ، آزمون هنگام برش یا تغییر خودکار توسط چت انجام نمی‌شود.',en:'First identify automatic travel versus jog. Record the current value, make a small authorized change, save, then evaluate unloaded on the device. Large changes, testing during cutting, and automatic chat changes are not allowed.'},report:{fa:'گزارش شامل زمان، وضعیت اتصال، حالت دستگاه، موقعیت/مقصد، اعتبار مرجع و شمارنده‌های موجود است. داده ناموجود با «--» ثبت می‌شود و هیچ مقدار حدسی ساخته نمی‌شود.',en:'The report includes time, connection, machine mode, position/destination, reference validity and available counters. Missing fields are marked -- and never guessed.'}};
Object.assign(supportDetail,{noMove:{fa:'مرحله بعدی ایمن:\n۱) در F3 وضعیت STOP، مرجع و سنسور انتها را ببینید.\n۲) مشخص کنید روی صفحه فرمان حرکت دیده می‌شود یا نه.\n۳) اگر فرمان هست ولی حرکت نیست، دستگاه را متوقف نگه دارید و بررسی درایو/برق را به تکنسین بسپارید.\nنتیجه همین سه مورد را بنویسید تا مسیر بعدی مشخص شود.',en:'Next safe step:\n1) In F3 inspect STOP, reference, and end-limit states.\n2) Note whether a motion command appears on screen.\n3) If command exists but there is no motion, keep the machine stopped and leave drive/power checks to a technician.\nSend those three results and I will narrow the next step.'},cutFlow:{fa:'برای پیدا کردن مرحله گیرکرده، فقط این چهار وضعیت را به‌ترتیب گزارش کنید: محافظ I2، Start1/Start2، اره، و گیره/تأیید. تا روشن‌شدن علت، چرخه را دوباره شروع نکنید و هیچ ورودی را بای‌پس نکنید.',en:'To locate the stalled stage, report these four states in order: guard I2, Start1/Start2, saw, and clamp/confirmation. Do not restart the cycle or bypass any input until the cause is clear.'},connection:{fa:'۱) اتصال گوشی به شبکه خود دستگاه را بررسی کنید.\n۲) نشان بالای وب باید «متصل» شود.\n۳) اگر هنوز آفلاین است، صفحه را یک‌بار بازخوانی کنید.\nهنگام آپدیت شبکه یا پرتال را خاموش‌وروشن نکنید.',en:'1) Confirm the phone is connected to the machine network.\n2) The web header must show online.\n3) If still offline, reload once.\nDo not toggle networking during an update.'},listSave:{fa:'متن دقیق خطا تعیین‌کننده است:\n403 = رمز مدیر نادرست (فقط نصب firmware)؛ 409 = دستگاه مشغول؛ 422 = محدوده یا ردیف نامعتبر. کد یا متن دیده‌شده را بفرستید.',en:'The exact response identifies the path:\n403 = wrong manager PIN (firmware install only); 409 = machine busy; 422 = invalid row or range. Send the code or message you see.'},pinError:{fa:'رمز مورد نیاز فقط برای نصب firmware است و همان رمز مدیر دستگاه می‌باشد؛ اگر 403 دیدید رمز را نزد مدیر دستگاه بررسی کنید.',en:'The only required password is the device manager PIN, and only for firmware install; if you see 403, verify it with the device manager.'},endSensor:{fa:'در F3 سنسور انتهای خط را بدون حرکت اجباری مشاهده کنید. اگر همیشه فعال یا همیشه غیرفعال است، فاصله مکانیکی، آلودگی و اتصال فقط در حالت ایمن/بی‌برق بررسی شود. قطبیت یا سیم‌کشی از چت تغییر نمی‌کند.',en:'Observe the end-limit state in F3 without forcing motion. If it is always active or inactive, inspect mechanical gap, contamination, and wiring only in a safe powered-down state. Chat never changes polarity or wiring.'},designerHelp:{fa:'ابتدا اندازه کلی را وارد کنید، سپس تقسیم‌ها را اضافه کنید و اندازه هر بخش را بازبینی کنید؛ هیچ بخش نباید کمتر از ۸۰ mm باشد. خروجی فعلی فقط هندسی است و بدون Rule Set تأییدشده، طول تولیدی محسوب نمی‌شود.',en:'Enter the overall size, add divisions, then review every section; no section may be below 80 mm. Current output is geometric only and is not a production cut length without an approved rule set.'},screenshot:{fa:'برای عکس واقعی HMI دکمه دانلود را بزنید. درخواست فقط یک فریم ۴۸۰×۳۲۰ می‌خواند و هیچ تنظیمی را تغییر نمی‌دهد. اگر ارتباط معتبر نباشد، دانلود انجام نمی‌شود.',en:'Press download for a real HMI screenshot. It reads one 480×320 frame and changes no setting. Download fails if the connection is invalid.'}});
const supportPageActions={blade:'diagnosticsHub',sensor:'f3',sensorAny:'f3',endSensor:'f3',noMove:'f3',cutFlow:'f3',clamp:'f3',startButtons:'f3',liveInputs:'f3',sawStatus:'f3',cutStatus:'home',moveStatus:'home',referenceStatus:'home',status:'home',position:'home',productionStatus:'home',counter:'reports',materialStatus:'f1',limitsStatus:'f2',healthStatus:'f5',version:'f5',size:'list',speed:'f2',pin:'f4',pinError:'f4',list:'list',listSave:'list',update:'upd',report:'reports',designerHelp:'designer',estop:'f3',connection:'home',optimization:'plan',guard:'f3',printer:'f1',keyboard:'f1',buzzer:'f1',calibration:'f4',timers:'f4',screenshot:'home',clock:'f5',ioMap:'f3',wiringMap:'f3',pageMain:'home',pageF1:'f1',pageF2:'f2',pageF3:'f3',pageF4:'f4',pageF5:'f5'};
function supportSnapshotFresh(){return !!lastState&&typeof lastState==='object'&&Object.keys(lastState).length>0&&Number(lastSyncOk)>0&&Date.now()-Number(lastSyncOk)<3500}
function supportValue(d,k){const v=d?.[k];return v===undefined||v===null||v===''?'--':v}
function supportYesNo(v,fa){return v===true||v===1?(fa?'بله':'yes'):v===false||v===0?(fa?'خیر':'no'):'--'}
function supportLiveIntent(id){return['status','sawStatus','moveStatus','cutStatus','referenceStatus','position','productionStatus','liveInputs','materialStatus','limitsStatus','healthStatus','version','counter','clock','ioMap','warranty'].includes(id)}
function supportLiveAnswer(id,lang){if(!supportLiveIntent(id))return'';const fa=lang==='fa';if(!supportSnapshotFresh())return fa?'الان اطلاعات زنده دستگاه نمی‌رسد. Wi‑Fi گوشی را بررسی کنید، صفحه را تازه کنید و دوباره بپرسید. من مقدار حدسی نشان نمی‌دهم.':'Live machine information is not arriving now. Check phone Wi-Fi, refresh the page, and ask again. I will not guess any value.';const d=lastState,v=k=>supportValue(d,k),yn=k=>supportYesNo(d[k],fa),mm=k=>v(k)==='--'?'--':v(k)+' mm';if(id==='status')return fa?`وضعیت فعلی (فقط‌خواندنی)\nحالت: ${v('mode')}\nحرکت: ${yn('moving')} · برش: ${yn('cutting')} · اره: ${yn('saw')}\nمرجع معتبر: ${yn('ref')}`:`Current status (read only)\nMode: ${v('mode')}\nMoving: ${yn('moving')} · Cutting: ${yn('cutting')} · Saw: ${yn('saw')}\nReference valid: ${yn('ref')}`;if(id==='sawStatus')return fa?`وضعیت فعلی (فقط‌خواندنی): اره روشن است؟ ${yn('saw')}`:`Current status (read only): Saw on? ${yn('saw')}`;if(id==='cutStatus')return fa?`وضعیت فعلی (فقط‌خواندنی): دستگاه در حال برش است؟ ${yn('cutting')}`:`Current status (read only): Is the machine cutting? ${yn('cutting')}`;if(id==='moveStatus')return fa?`وضعیت فعلی (فقط‌خواندنی)\nحرکت: ${yn('moving')}\nموقعیت: ${mm('cur')} · مقصد: ${mm('dest')}`:`Current status (read only)\nMoving: ${yn('moving')}\nPosition: ${mm('cur')} · Destination: ${mm('dest')}`;if(id==='referenceStatus')return fa?`وضعیت فعلی (فقط‌خواندنی): مرجع محور معتبر است؟ ${yn('ref')}`:`Current status (read only): Axis reference valid? ${yn('ref')}`;if(id==='position')return fa?`وضعیت فعلی (فقط‌خواندنی)\nموقعیت: ${mm('cur')}\nمقصد: ${mm('dest')}\nمرجع معتبر: ${yn('ref')}`:`Current status (read only)\nPosition: ${mm('cur')}\nDestination: ${mm('dest')}\nReference valid: ${yn('ref')}`;if(id==='productionStatus')return fa?`وضعیت فعلی (فقط‌خواندنی)\nحالت: ${v('mode')}\nردیف فعال: ${v('row')} · باقی‌مانده: ${v('remaining')}`:`Current status (read only)\nMode: ${v('mode')}\nActive row: ${v('row')} · Remaining: ${v('remaining')}`;if(id==='liveInputs')return fa?`ورودی‌های فعلی (فقط‌خواندنی)\nمرجع: ${yn('i_ref')} · انتهای خط: ${yn('i_up')} · توقف: ${yn('i_stop')}\nStart1: ${yn('i_s1')} · Start2: ${yn('i_s2')} · اره: ${yn('i_saw')}`:`Current inputs (read only)\nReference: ${yn('i_ref')} · End limit: ${yn('i_up')} · Stop: ${yn('i_stop')}\nStart1: ${yn('i_s1')} · Start2: ${yn('i_s2')} · Saw: ${yn('i_saw')}`;if(id==='materialStatus')return fa?`تنظیمات فعلی (فقط‌خواندنی)\nطول شاخه: ${mm('stock')} · عرض تیغه: ${mm('kerf')} · پرت ابتدا: ${mm('trim')}`:`Current settings (read only)\nStock: ${mm('stock')} · Kerf: ${mm('kerf')} · Start trim: ${mm('trim')}`;if(id==='limitsStatus')return fa?`محدوده فعلی (فقط‌خواندنی)\nابتدای خط: ${mm('startmm')} · انتهای خط: ${mm('maxlen')}`:`Current limits (read only)\nStart: ${mm('startmm')} · End: ${mm('maxlen')}`;if(id==='healthStatus')return fa?`اطلاعات فعلی (فقط‌خواندنی)\nزمان روشن‌بودن: ${v('uptime')} s · حافظه آزاد: ${v('heap')} byte\n(مجموع از آخرین روشن‌شدن؛ کارکرد روزانه جداگانه ثبت نمی‌شود)`:`Current information (read only)\nUptime: ${v('uptime')} s · Free heap: ${v('heap')} bytes\n(Total since last power-on; daily hours are not tracked separately)`;if(id==='version')return fa?`نسخه نصب‌شده دستگاه: ${v('ver')}`:`Installed device version: ${v('ver')}`;if(id==='counter')return fa?`شمارنده فعلی (فقط‌خواندنی)\nکاربر: ${v('ucuts')} · عمر دستگاه: ${v('tcuts')}`:`Current counters (read only)\nUser: ${v('ucuts')} · Lifetime: ${v('tcuts')}`;if(id==='clock'){const t=Number(d.clock_epoch)>0?new Intl.DateTimeFormat(fa?'fa-IR-u-ca-persian':'en-GB',{dateStyle:'full',timeStyle:'medium'}).format(new Date(Number(d.clock_epoch)*1000)):'--';return fa?`🕒 ساعت دستگاه (فقط‌خواندنی): ${t}`:`🕒 Device clock (read only): ${t}`}if(id==='ioMap'){const IN=[['i0','سنسور اول خط','first-line sensor'],['i1','سنسور آخر خط','end-line sensor'],['i2','درب محافظ','door guard'],['i3','توقف','stop'],['i4','برگشت تیغه','blade return'],['i5','خطای موتور','motor fault'],['i6','استارت ۱','start 1'],['i7','استارت ۲','start 2'],['i8','جاگ راست','jog right'],['i9','جاگ چپ','jog left']],OUT=[['q0','پالس استپر','stepper pulse'],['q1','جهت استپر','stepper direction'],['q2','فعال‌ساز استپر','stepper enable'],['q3','جک نگهدارنده','holder jack'],['q4','جک محافظ','guard jack'],['q5','اسپری','spray'],['q6','اره','saw'],['q7','چراغ','lamp'],['q8','بالابر تیغه','saw lift'],['q9','رزرو','reserved']],own=k=>Object.prototype.hasOwnProperty.call(d,k),conn=k=>own('c'+k),st=k=>own(k)?(Number(d[k])?(fa?'فعال':'active'):(fa?'آماده':'idle')):'--',fmt=list=>list.map(x=>`${conn(x[0])?'✅':'⚪'} ${x[fa?1:2]}: ${conn(x[0])?st(x[0]):(fa?'وصل نیست':'not mapped')}`).join('\n');return fa?`🔌 وضعیت لحظه‌ای (فقط‌خواندنی)\nورودی‌ها:\n${fmt(IN)}\nخروجی‌ها:\n${fmt(OUT)}`:`🔌 Live I/O (read only)\nInputs:\n${fmt(IN)}\nOutputs:\n${fmt(OUT)}`}if(id==='warranty')return fa?`🗓️ گارانتی/قفل زمانی (فقط‌خواندنی)
روزهای باقی‌مانده: ${v('dayrem')}
(مبنای شمارش همان قفل زمانی است؛ برای گارانتی قانونی، فاکتور مبناست.)`:`🗓️ Warranty/time lock (read only)
Days remaining: ${v('dayrem')}
(The time-lock counter is the basis; for legal warranty, the invoice is authoritative.)`;return''}
function supportReportText(){const d=lastState||{},fa=supportLastLang==='fa',fresh=supportSnapshotFresh(),v=x=>x===undefined||x===null||x===''?'--':x,ref=d.ref===true||d.ref===1?(fa?'بله':'yes'):d.ref===false||d.ref===0?(fa?'خیر':'no'):'--',lines=fa?['گزارش پشتیبانی دستگاه',fresh?'وضعیت: زنده و فقط‌خواندنی':'وضعیت: آفلاین/ناموجود؛ داده‌ها زنده نیستند','زمان: '+new Date().toLocaleString('fa-IR'),'ارتباط: '+(fresh?'متصل':'ناموجود'),'حالت: '+v(d.mode),'موقعیت: '+v(d.cur)+' mm','مقصد: '+v(d.dest)+' mm','مرجع معتبر: '+ref,'سرعت برش: '+v(d.spd)+'%','سرعت جاگ: '+v(d.jspd)+'%','برش کاربر: '+v(d.ucuts),'کل برش: '+v(d.tcuts)]:['Machine support report',fresh?'Status: live, read only':'Status: offline/unavailable; values are not live','Time: '+new Date().toLocaleString('en-GB'),'Connection: '+(fresh?'online':'unavailable'),'Mode: '+v(d.mode),'Position: '+v(d.cur)+' mm','Destination: '+v(d.dest)+' mm','Referenced: '+ref,'Cut speed: '+v(d.spd)+'%','Jog speed: '+v(d.jspd)+'%','User cuts: '+v(d.ucuts),'Lifetime cuts: '+v(d.tcuts)];return lines.join('\n')}
function supportDownloadReport(){const text=supportReportText(),blob=new Blob([text],{type:'text/plain;charset=utf-8'}),a=document.createElement('a');a.href=URL.createObjectURL(blob);a.download=`cutter-support-${new Date().toISOString().slice(0,10)}.txt`;a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000);supportAdd('bot',supportLastLang==='fa'?'گزارش ساخته و دانلود شد. اگر ارتباط آفلاین بود، همان وضعیت داخل فایل ثبت شده است.':'The report was generated and downloaded. Offline status is explicitly recorded in the file.')}
function supportOpenPage(page,lang){requestPage(page);setTimeout(()=>{const h=document.querySelector(`#${page} .head h2`);if(h){h.tabIndex=-1;h.focus()}},0);supportAdd('bot',lang==='fa'?'بخش مربوط باز شد. فقط صفحه عوض شد؛ هیچ تنظیم یا فرمان دستگاهی اجرا نشد.':'The related page is open. Only navigation occurred; no setting or machine command was executed.')}
function supportSimpleAnswer(id,lang){const fa=lang==='fa',live=supportLiveAnswer(id,lang);if(live)return live;const m={blade:[`اول دستگاه را متوقف نگه دارید.
بعد روی «ورودی‌های زنده» بزنید.
فقط بگویید وضعیت خروجی اره فعال است یا غیرفعال. سیم و خروجی را دست نزنید.`,`Keep the machine stopped.
Open “Live inputs”.
Tell me only whether the saw indicator is on or off. Do not touch wiring or outputs.`],sensor:[`سنسور اول، نقطه شروع اندازه‌گیری است.
روی «ورودی‌های زنده» بزنید و بگویید نشانگر سنسور اول (I0) روشن است یا خاموش.`,`The first sensor is the measuring start point.
Open “Live inputs” and tell me whether the Reference indicator is on or off.`],endSensor:[`سنسور آخر، پایان مسیر را نشان می‌دهد.
روی «ورودی‌های زنده» بزنید و وضعیت نشانگر انتهای خط (I1) را بگویید.`,`The end sensor marks the end of travel.
Open “Live inputs” and tell me the End-limit indicator state.`],noMove:[`دستگاه را متوقف نگه دارید.
روی «ورودی‌های زنده» بزنید.
بگویید توقف، سنسور اول و سنسور آخر روشن‌اند یا خاموش.`,`Keep the machine stopped.
Open “Live inputs”.
Tell me whether Stop, first sensor and end sensor are on or off.`],size:[`برای چند اندازه، «لیست برش» را باز کنید.
در هر ردیف فقط طول به میلی‌متر و تعداد را بنویسید.
قبل از ذخیره، ردیف‌ها را دوباره نگاه کنید.`,`For several sizes, open “Cut list”.
Enter only millimetres and quantity in each row.
Review the rows before saving.`],list:[`روی «لیست برش» بزنید.
طول را به میلی‌متر و تعداد را عدد صحیح بنویسید.
اگر خطا دیدید، متن همان خطا را برایم بفرستید.`,`Open “Cut list”.
Enter length in millimetres and quantity as a whole number.
If you see an error, send me its exact text.`],speed:[`اول بگویید کدام حرکت بد است:
حرکت خودکار یا حرکت با کلید جاگ؟
خودم سرعت را تغییر نمی‌دهم.`,`First tell me which motion is wrong:
automatic travel or Jog buttons?
I will not change speed automatically.`],pinError:[`تنها رمز لازم برای وب، رمز مدیر دستگاه و فقط هنگام نصب firmware است.
اگر 403 دیدید، رمز را نزد مدیر دستگاه بررسی کنید.`,`The only web password is the device manager PIN, used only for firmware install.
If you see 403, verify the PIN with the device manager.`],estop:[`حرکت را شروع نکنید.
توقف اضطراری را آزاد کنید؛ اگر وضعیت عوض نشد، با تکنسین تماس بگیرید.
هیچ سیم یا سنسوری را بای‌پس نکنید.`,`Do not start motion.
Release the emergency stop; if its state does not change, contact a technician.
Never bypass wiring or sensors.`],cutFlow:[`دستگاه را متوقف نگه دارید.
بگویید چرخه در کدام جا می‌ماند: گیره، اره، برگشت تیغه یا تأیید؟
من هیچ برشی را اجرا نمی‌کنم.`,`Keep the machine stopped.
Tell me where the cycle stops: clamp, saw, blade return, or confirmation?
I never start a cut.`],connection:[`اول Wi‑Fi گوشی را بررسی کنید.
باید به شبکه همین دستگاه وصل باشد.
بعد صفحه را یک‌بار تازه کنید.`,`Check the phone Wi-Fi first.
It must be connected to this machine.
Then refresh the page once.`],report:[`روی «دانلود گزارش» بزنید.
فایل فقط اطلاعات موجود را می‌نویسد؛ مقدار حدسی ندارد.`,`Tap “Download report”.
The file contains only available values; nothing is guessed.`],screenshot:[`PIN وب را در منو وارد کنید.
بعد روی «درخواست عکس واقعی» بزنید.
این کار فقط عکس صفحه را می‌گیرد.`,`Enter the web PIN in the menu.
Then tap “Request real screenshot”.
This only captures the display.`]};if(m[id])return m[id][fa?0:1];const topic=supportTopic(id,lang);return fa?`موضوع: ${topic}
روی «باز کردن بخش مربوط» بزنید.
بعد فقط نام چراغ، خطا یا چیزی که می‌بینید را بنویسید.`:`Topic: ${topic}
Tap “Open related page”.
Then send only the indicator, error, or what you see.`}
function supportEmptyPrompt(){const fa=webLanguage==='fa';supportAdd('bot',fa?'چیزی نوشته نشده است. یکی از گزینه‌ها را بزنید یا مشکل را کوتاه بنویسید.':'Nothing was entered. Tap one choice or write the problem in a short sentence.',[{label:fa?'⌕ یک مشکل دارم':'⌕ I have a problem',q:fa?'برای عیب یابی کمکم کن':'help me troubleshoot'},{label:fa?'◉ وضعیت دستگاه':'◉ Machine status',q:fa?'وضعیت زنده دستگاه را بگو':'tell me the live machine status'},{label:fa?'؟ روش کار':'? How to use it',q:fa?'چه کارهایی می توانی انجام بدهی':'what can you do'}])}
function supportHandleEasy(q,lang){const fa=lang==='fa',mistake=/(اشتباه زدم|اشتباهی|از اول|برگرد اول|wrong button|by mistake|start over)/i.test(q),simple=/(ساده|اسون|آسان|نفهم|متوجه نشد|یعنی چی|راحت تر|بلد نیست|نمی ?دون|simple|do not understand|don't understand|do not know|don't know|what does it mean)/i.test(q);if(mistake){supportLastIntent='';supportContext=[];supportAdd('bot',fa?'نگران نباشید. این چت فرمان حرکت یا برش اجرا نکرده است. از اول شروع می‌کنیم؛ یکی را بزنید.':'No problem. This chat did not run motion or cutting. Let us start again; tap one choice.',[{label:fa?'⌕ یک مشکل دارم':'⌕ I have a problem',q:fa?'برای عیب یابی کمکم کن':'help me troubleshoot'},{label:fa?'◉ وضعیت دستگاه':'◉ Machine status',q:fa?'وضعیت زنده دستگاه را بگو':'tell me the live machine status'},{label:fa?'؟ روش کار':'? How to use it',q:fa?'چه کارهایی می توانی انجام بدهی':'what can you do'}]);return true}if(simple){if(supportLastIntent){supportAdd('bot',supportSimpleAnswer(supportLastIntent,lang),supportActions(supportLastIntent,lang).filter(x=>!x.q));return true}supportAdd('bot',fa?'اشکالی ندارد. فقط بگویید مشکل مربوط به حرکت، برش یا سنسور است.':'That is okay. Tell me only whether the problem is motion, cutting, or a sensor.',[{label:fa?'↔ حرکت':'↔ Motion',q:fa?'موتور حرکت نمی کند':'motor not moving'},{label:fa?'✂ برش':'✂ Cutting',q:fa?'مراحل برش انجام نمی شود':'cut cycle not completing'},{label:fa?'◉ سنسور':'◉ Sensor',q:fa?'مشکل سنسور دارم':'sensor problem'}]);return true}return false}
function supportIcon(kind,text){if(kind==='live')return'status';if(/(توقف اضطراری|متوقف|محافظ|ایمنی|بای.?پس|سیم|خروجی را دست|emergency stop|keep the machine stopped|guard|wiring|do not start|never bypass)/i.test(text))return'warning';if(/(دانلود شد|باز شد|generated and downloaded|is open)/i.test(text))return'check';if(/[؟?]$/.test(text.trim()))return'help';return'info'}
function supportCleanLabel(s){return String(s||'').replace(/^(?:Aa|[^\w\u0600-\u06ff]+)\s*/,'')}
function supportActionIcon(x){const s=(x.label||'')+' '+(x.q||'');if(/report|گزارش|download|دانلود|عکس|screenshot/i.test(s))return'download';if(/more|detail|جزئیات|بیشتر/i.test(s))return'detail';if(/simple|ساده|نفهم/i.test(s))return'simple';if(/open|باز کردن/i.test(s))return'open';if(/cut|برش|تیغه/i.test(s))return'cut';if(/motion|motor|حرکت|موتور/i.test(s))return'motion';if(/sensor|status|live|وضعیت|سنسور|ورودی/i.test(s))return'status';return'help'}
function supportDecorateChrome(){const map={supportQuickStatus:'status',supportQuickTrouble:'search',supportQuickLearn:'help',supportQuickReport:'report',supportServiceStatus:'input'},fa=webLanguage==='fa';Object.entries(map).forEach(([id,icon])=>{const e=$(id);if(!e)return;const label=supportCleanLabel(e.textContent);e.innerHTML=uiIcon(icon)+`<span>${label}</span>`});const avatar=document.querySelector('.support-avatar');if(avatar)avatar.innerHTML=uiIcon('help','lg');const send=$('supportSendButton');if(send)send.innerHTML=uiIcon('output','lg');if(send)send.title=fa?'ارسال':'Send'}
function supportActions(id,lang){const fa=lang==='fa',out=[],quiet=['hello','thanks','bye'].includes(id),live=supportLiveIntent(id);if(id==='troubleshoot')return[{label:fa?'↔ حرکت محور':'↔ Axis motion',q:fa?'موتور حرکت نمی کند':'motor not moving'},{label:fa?'✂ چرخه برش':'✂ Cut cycle',q:fa?'مراحل برش انجام نمی شود':'cut cycle not completing'},{label:fa?'◉ سنسور و توقف':'◉ Sensors & stop',q:fa?'مشکل سنسور دارم':'sensor problem'}];if(!quiet&&!live)out.push({label:fa?'Aa ساده‌تر بگو':'Aa Explain simply',q:fa?'ساده تر بگو، نفهمیدم':'explain simply, I do not understand'});if(live)out.push({label:fa?'◉ تازه‌سازی وضعیت':'◉ Refresh status',run:async()=>{await syncState();supportAsk(fa?'وضعیت زنده دستگاه را بگو':'tell me the live machine status')}});if(id==='report')out.push({label:fa?'↓ دانلود گزارش':'↓ Download report',run:supportDownloadReport});if(id==='screenshot')out.push({label:fa?'▧ درخواست عکس واقعی':'▧ Request real screenshot',run:downloadScreen});if(supportPageActions[id]&&!['reset','update'].includes(id))out.push({label:fa?'↗ باز کردن بخش مربوط':'↗ Open related page',run:()=>supportOpenPage(supportPageActions[id],lang)});if(!quiet)out.push({label:fa?'＋ جزئیات بیشتر':'＋ More detail',q:fa?'بیشتر توضیح بده':'explain more'});return out.slice(0,3)}
function supportAdd(kind,text,actions=[]){const chat=$('supportChat');if(!chat)return;const user=kind==='user',live=kind==='live',warning=!user&&/(توقف اضطراری|متوقف|محافظ|ایمنی|بای.?پس|سیم|خروجی را دست|emergency stop|keep the machine stopped|guard|wiring|do not start|never bypass)/i.test(text),d=document.createElement('div'),meta=document.createElement('span'),body=document.createElement('span');d.className='chat-msg '+(user?'user':'bot')+(live?' live':'')+(warning?' warn':'');meta.className='chat-meta';meta.textContent=user?(supportLastLang==='fa'?'شما':'You'):live?(supportLastLang==='fa'?'وضعیت فعلی · فقط‌خواندنی':'Current status · read only'):(supportLastLang==='fa'?'راهنمای آفلاین':'Offline guide');body.className='chat-body';body.textContent=text;if(user)d.append(meta,body);else{const icon=document.createElement('span');icon.className='chat-icon';icon.setAttribute('aria-hidden','true');icon.innerHTML=uiIcon(supportIcon(kind,text));d.append(icon,meta,body)}if(actions.length){const a=document.createElement('div');a.className='chat-actions';actions.forEach(x=>{const b=document.createElement('button'),label=document.createElement('span');b.type='button';b.innerHTML=uiIcon(supportActionIcon(x));label.textContent=supportCleanLabel(x.label);b.appendChild(label);b.onclick=()=>x.run?x.run():supportAsk(x.q);a.appendChild(b)});d.appendChild(a)}chat.appendChild(d);while(chat.children.length>SUPPORT_HISTORY_MAX)chat.firstElementChild.remove();chat.scrollTop=chat.scrollHeight}
function supportAliasMatch(q,a){a=supportNorm(a);if(!a)return 0;if(a==='کند'&&/(^| )نمی کند( |$)/.test(q))return 0;if(q===a)return 4;const qt=' '+q+' ',at=' '+a+' ';if(qt.includes(at))return a.includes(' ')?3:2;if(a.length>=4&&q.includes(a))return 1;return supportFuzzyAlias(q,a)}
function supportRank(q){return supportIntents.map((x,i)=>{const primary=x.a.reduce((n,a)=>n+supportAliasMatch(q,a),0),secondary=primary&&x.b?x.b.reduce((n,a)=>n+supportAliasMatch(q,a),0):0,greeting=x.id==='hello'&&q.split(' ').length>1?2:0;return{x,n:Math.max(0,primary+secondary-greeting),i}}).sort((a,b)=>b.n-a.n||a.i-b.i)}
function supportTopic(id,lang){const fa=lang==='fa',names={blade:['🪚 تیغه/اره','🪚 Blade/saw'],sensor:['📡 سنسور اول','📡 First sensor'],endSensor:['📡 سنسور آخر','📡 End sensor'],speed:['⚙️ سرعت','⚙️ Speed'],clock:['🕒 ساعت','🕒 Clock'],ioMap:['🔌 ورودی/خروجی','🔌 I/O'],wiringMap:['🗺️ نقشه سیم‌کشی','🗺️ Wiring'],estop:['🛑 توقف اضطراری','🛑 Emergency stop'],noMove:['🚫 حرکت‌نکردن محور','🚫 Axis not moving'],cutFlow:['✂️ چرخه برش','✂️ Cut cycle'],list:['📋 لیست برش','📋 Cut list'],pinError:['🔐 خطای PIN','🔐 PIN error'],connection:['📶 اتصال وب','📶 Web connection'],status:['📊 وضعیت دستگاه','📊 Machine status'],report:['📄 گزارش','📄 Report']};return(names[id]||[id,id])[fa?0:1]}
function supportRemember(id){if(!id)return;supportLastIntent=id;supportContext.push(id);supportContext=[...new Set(supportContext.slice(-3))]}
function supportKind(q){if(/(خراب|کار نم|نمی ?کن|نشد|خطا|ارور|گیر|wrong|not work|failed|broken|stuck|does not)/i.test(q))return'problem';if(/(چطور|چگونه|روش|how|where do i|steps)/i.test(q))return'how';return'info'}
function supportLead(kind,lang,q){const list=supportOpeners[lang][kind]||supportOpeners[lang].info;return list[supportHash(q)%list.length]}
function supportAnswer(item,lang,q){const live=supportLiveAnswer(item.id,lang);if(live)return live;if(['hello','thanks','bye'].includes(item.id))return item[lang]||item.en;const kind=supportKind(q),lead=supportLead(kind,lang,q),ask=lang==='fa'?' اگر نتیجه را بگویید، مرحله بعد را از همین‌جا ادامه می‌دهم.':' Tell me what you observe and I will continue from this exact step.',spdLive=item.id==='speed'&&supportSnapshotFresh()?(lang==='fa'?`\n\nمقدار فعلی (فقط‌خواندنی): سرعت برش ${supportValue(lastState,'spd')}٪ · سرعت جاگ ${supportValue(lastState,'jspd')}٪`:`\n\nCurrent read-only values: cut speed ${supportValue(lastState,'spd')}% · jog speed ${supportValue(lastState,'jspd')}%`):'';return lead+'\n\n'+(item[lang]||item.en)+spdLive+(kind==='problem'?ask:'')}
function supportDeepAnswer(id,lang,unresolved=false){const item=supportIntents.find(x=>x.id===id),base=supportDetail[id]?.[lang]||supportDetail[id]?.en||((item?.[lang]||item?.en||'')+'\n\n'+(lang==='fa'?'برای دقیق‌ترشدن پاسخ، نام صفحه، نشانگر دیده‌شده و اینکه دستگاه متوقف است یا نه را بنویسید.':'For a more precise answer, send the page name, visible indicator, and whether the machine is stopped.'));return unresolved?(lang==='fa'?'باشه؛ پس از مرحله قبلی جلوتر می‌رویم.\n\n':'Understood; let us continue beyond the previous step.\n\n')+base:base}
function supportLexLoad(){try{const a=JSON.parse(localStorage.getItem('supportLexicon')||'[]');return Array.isArray(a)?a.slice(-60):[]}catch(e){return[]}}
function supportLexSave(w,t){try{const a=supportLexLoad().filter(x=>x.w!==w);a.push({w,t});localStorage.setItem('supportLexicon',JSON.stringify(a.slice(-60)));return true}catch(e){return false}}
function supportLexFind(q){return supportLexLoad().filter(x=>x.w&&q.includes(x.w))}
function supportIsOffTopicOrInappropriate(s){
  const badPatterns = [
    /(بی ?شرف|احمق|کثافت|حروم|دیوونه|خر|گاو|سگ|سکس|sex|porn|fuck|shit|bitch|bastard|asshole)/i,
    /(سیاست|سیاسی|حکومت|دولت|انتخابات|رییس جمهور|اعتراض|politics|election|government|regime)/i
  ];
  return badPatterns.some(rx=>rx.test(s));
}

function supportSend(){
  const input=$('supportInput'),raw=input?.value.trim();
  if(!raw){supportEmptyPrompt();return}
  const q=supportNorm(raw),lang=/[a-z]{2,}/i.test(raw)&&!/[\u0600-\u06ff]/.test(raw)?'en':/[\u0600-\u06ff]/.test(raw)?'fa':webLanguage,fa=lang==='fa';
  supportLastLang=lang;
  supportAdd('user',raw);
  input.value='';

  // 1. Guardrail / Moderation Check:
  if(supportIsOffTopicOrInappropriate(raw)){
    supportAdd('bot', fa?
      'من دستیار فنی و تخصصی دستگاه صنعتی برش هافلر هستم و منحصراً برای راهنمایی عیب‌یابی، نگهداری مکانیک/الکترونیک، تنظیمات و لیست برش برنامه‌ریزی شده‌ام. لطفاً پرسش فنی خود درباره دستگاه را مطرح فرمایید.':
      'I am a dedicated technical assistant for the Haffler industrial machine and answer only machine operation, maintenance, and technical questions. Please ask a machine-related question.',
      [{label:fa?'🪚 عیب‌یابی اره':'🪚 Blade trouble',q:fa?'تیغه کار نمیکند':'blade not working'},
       {label:fa?'↔ عیب‌یابی موتور':'↔ Motor trouble',q:fa?'موتور حرکت نمیکند':'motor not moving'},
       {label:fa?'📋 لیست برش':'📋 Cut list',q:fa?'راهنمای لیست برش':'cut list help'}]
    );
    return;
  }

  if(supportHandleEasy(q,lang))return;

  // 2. Interactive Teach / Learn with Safety Check:
  const teach=raw.match(/^\s*(?:یاد بگیر|یاد بگیرم|learn)\s*[:،]?\s*(.{1,40}?)\s+(?:یعنی|means)\s+(.{1,200})$/i);
  if(teach){
    if(supportIsOffTopicOrInappropriate(teach[1]) || supportIsOffTopicOrInappropriate(teach[2])){
      supportAdd('bot', fa?'واژه یا توضیح وارد شده مجاز نیست. لطفاً صرفاً اصطلاحات فنی دستگاه را آموزش دهید.':'Content not allowed. Please teach technical machine terms only.');
      return;
    }
    const w=supportNorm(teach[1]),t=teach[2].trim();
    if(w&&t&&supportLexSave(w,t)){
      supportAdd('bot',fa?`✅ با موفقیت یاد گرفتم: اصطلاح «${teach[1].trim()}» به معنی «${t}» است.
از این پس در گفت‌وگو از این مفهوم استفاده خواهم کرد.`:`✅ Successfully learned: “${teach[1].trim()}” means “${t}”.
I will remember this for future technical queries.`);
    } else {
      supportAdd('bot',fa?'دسترسی حافظه مرورگر برای یادگیری واژه باز نیست.':'Browser storage for learning words is unavailable.');
    }
    return;
  }

  const more=/(بیشتر|کامل.?تر|جزئیات|توضیح بده|more|detail|explain)/i.test(q),
        unresolved=/(حل نشد|هنوز|جواب نداد|still|did not fix|not solved)/i.test(q),
        navReq=/(^| )(باز کن|برو به|open|go to)( |$)/i.test(q)&&!/(چطور|چگونه|how)/i.test(q),
        unsafeReq=/(تغییر بده|کم کن|زیاد کن|روشن کن|خاموش کن|حرکت بده|برش بزن|ریست کن|تنظیم کن|عوض کن|تندتر کن|کندتر کن|سریع تر کن|change|set |turn on|turn off|move it|start cut|reset it|adjust)/i.test(q);

  const ranked=supportRank(q),candidate=ranked[0];

  if(more&&supportLastIntent&&(!candidate||candidate.n<2||candidate.x.id===supportLastIntent)){
    supportAdd('bot',supportDeepAnswer(supportLastIntent,lang,false),supportActions(supportLastIntent,lang).filter(x=>!x.q));
    return;
  }
  if((more||unresolved)&&candidate?.n>=1){
    supportRemember(candidate.x.id);
    supportAdd(supportLiveIntent(candidate.x.id)?'live':'bot',supportDeepAnswer(candidate.x.id,lang,unresolved),supportActions(candidate.x.id,lang).filter(x=>!x.q));
    return;
  }
  if(unresolved&&supportLastIntent){
    supportAdd('bot',supportDeepAnswer(supportLastIntent,lang,true),supportActions(supportLastIntent,lang).filter(x=>!x.q));
    return;
  }

  const connector=/( و | همچنین | بعلاوه | and | also | plus )/i.test(' '+q+' '),hits=ranked.filter(r=>r.n>=2&&!['hello','thanks','bye','capabilities'].includes(r.x.id));
  if(connector&&hits.length>1){
    const safeIds=['estop','guard','wrongDirection','clamp','startButtons','blade'],
          ordered=[...hits.slice(0,3)].sort((a,b)=>(safeIds.includes(b.x.id)?1:0)-(safeIds.includes(a.x.id)?1:0)),
          first=ordered[0],second=ordered.find(x=>x.x.id!==first.x.id);
    supportRemember(first.x.id);
    const intro=fa?`دو موضوع تخصصی در پیام شما شناسایی شد. ابتدا «${supportTopic(first.x.id,lang)}» به دلیل اولویت ایمنی:`:`Two topics detected. First, “${supportTopic(first.x.id,lang)}” for safety priority:`;
    supportAdd('bot',intro+'\n\n'+supportAnswer(first.x,lang,q)+'\n\n'+(fa?`سپس «${supportTopic(second.x.id,lang)}»:`:`Then “${supportTopic(second.x.id,lang)}”:`)+'\n'+supportAnswer(second.x,lang,q),supportActions(first.x.id,lang));
    return;
  }

  if(candidate?.n>=2&&ranked[1]?.n===candidate.n&&!connector&&!['hello','thanks','bye'].includes(candidate.x.id)){
    const other=ranked[1],ask=fa?`دو بخش نزدیک به سؤال شما پیدا شد: «${supportTopic(candidate.x.id,lang)}» یا «${supportTopic(other.x.id,lang)}». کدام را بررسی کنیم؟`:`I found two related topics: “${supportTopic(candidate.x.id,lang)}” or “${supportTopic(other.x.id,lang)}”. Which one would you like to review?`;
    supportAdd('bot',ask,[{label:supportTopic(candidate.x.id,lang),q:candidate.x.a[0]},{label:supportTopic(other.x.id,lang),q:other.x.a[0]}]);
    return;
  }

  // 3. Fallback / Learned Knowledge / Clarifying Suggestions:
  if(!candidate||candidate.n<1){
    if(supportLastIntent&&/(اون|همون|این مورد|چرا|بعدش|it|that|why|next)/i.test(q)){
      supportAdd('bot',supportDeepAnswer(supportLastIntent,lang,false),supportActions(supportLastIntent,lang));
      return;
    }
    const lex=supportLexFind(q);
    if(lex.length){
      supportAdd('bot',(fa?'📖 بر اساس اصطلاحاتی که آموزش داده‌اید:\n':'📖 According to terms you taught me:\n')+lex.map(x=>`«${x.w}» به معنی: ${x.t}`).join('\n'));
      return;
    }
    supportAdd('bot', fa?
      '🤔 این عبارت دقیق را متوجه نشدم. برای اینکه بتوانم دقیق راهنمایی کنم، لطفاً موضوع را مشخص فرمایید یا یکی از بخش‌های زیر را انتخاب کنید:
(همچنین می‌توانید با نوشتن «یاد بگیر [کلمه] یعنی [توضیح]» اصطلاحات جدید را به من بیاموزید)':
      '🤔 I did not fully catch that. Please pick a category below or teach me with “learn [word] means [desc]”:',
      [{label:fa?'🪚 اره و برش':'🪚 Saw & Blade',q:fa?'مشکل اره و تیغه':'saw blade problem'},
       {label:fa?'↔ حرکت و موتور':'↔ Motion & Motor',q:fa?'موتور حرکت نمیکند':'motor not moving'},
       {label:fa?'📡 سنسور و توقف':'📡 Sensors & Stop',q:fa?'مشکل سنسور دارم':'sensor problem'},
       {label:fa?'📋 لیست برش':'📋 Cut List',q:fa?'لیست برش':'cut list help'}]
    );
    return;
  }

  const item=candidate.x;
  supportRemember(item.id);
  supportAdd(supportLiveIntent(item.id)?'live':'bot',supportAnswer(item,lang,q),supportActions(item.id,lang));

  if(unsafeReq){
    supportAdd('bot',fa?'این درخواست می‌تواند وضعیت مکانیکی یا مقادیر ذخیره‌شده را تغییر دهد؛ این دستیار هیچ فرمانی به خروجی یا موتور صادر نمی‌کند. در صورت نیاز به تغییر، از منوی مربوطه روی صفحه اقدام فرمایید.':'This request could alter machine state or settings; this guide never directly drives motion or outputs. Please make changes from the designated settings page.');
    return;
  }

  const reportDownload=item.id==='report'&&/(دانلود|بساز|تهیه کن|بده|بگیر|میخوام|می خواهم|download|create|generate|want|give)/i.test(q)&&!/(چطور|توضیح|راهنما|روش|how|explain|guide|instruction)/i.test(q)&&!navReq,
        screenshotDownload=item.id==='screenshot'&&/(دانلود|بگیر|download|take|get)/i.test(q)&&!/(چطور|چگونه|راهنما|روش|how|help|guide|instruction)/i.test(q);
  if(reportDownload) supportDownloadReport();
  else if(screenshotDownload) downloadScreen();
  else if(navReq&&supportPageActions[item.id]&&!['reset','update'].includes(item.id)) supportOpenPage(supportPageActions[item.id],lang);
}
function supportHash(s){let h=2166136261;for(let i=0;i<s.length;i++)h=(h^s.charCodeAt(i))*16777619;return Math.abs(h|0)}
function polishF5(){const f=$('f5');if(!f)return;f.querySelector('.head h2').textContent=webLanguage==='fa'?'آمار و نسخه F5':'F5 statistics & version';['heap','uptime'].forEach(id=>$(id)?.closest('.stat-box')?.setAttribute('hidden',''));$('devstat')?.closest('.card')?.querySelector('#devstat')?.setAttribute('hidden','')}
let selectedOtaModelIdx = 0;
function setUpdateTab(tab){
  const localBtn=$('tabLocalUpload'), onlineBtn=$('tabOnlineOta');
  const localSec=$('localUploadSection'), onlineSec=$('onlineOtaSection');
  if(tab==='online'){
    localBtn?.classList.add('gray'); onlineBtn?.classList.remove('gray');
    if(localSec) localSec.hidden=true;
    if(onlineSec) onlineSec.hidden=false;
    checkOnlineOtaVersion();
  } else {
    localBtn?.classList.remove('gray'); onlineBtn?.classList.add('gray');
    if(localSec) localSec.hidden=false;
    if(onlineSec) onlineSec.hidden=true;
  }
}

function selectOtaModel(idx){
  selectedOtaModelIdx=idx;
  const c0=$('otaCard0'), c1=$('otaCard1');
  if(c0){ c0.style.borderColor=idx===0?'#087ad9':'#d9dde1'; c0.style.background=idx===0?'#f0f7ff':'#fff'; }
  if(c1){ c1.style.borderColor=idx===1?'#b85f00':'#d9dde1'; c1.style.background=idx===1?'#fef5ea':'#fff'; }
}

function normalizeOtaInput(input){
  if(!input) return '';
  let s=input.value.trim();
  if(s && !s.startsWith('http://') && !s.startsWith('https://')){
    if(s.includes('192.168.') || s.includes('10.') || s.includes('localhost') || s.includes(':80')){
      s='http://'+s;
    } else {
      s='https://'+s;
    }
  }
  return s;
}

async function checkOnlineOtaVersion(btn){
  setBusy(btn,true);
  const fa=webLanguage==='fa';
  setMsg('fwmsg', fa?'در حال بررسی سرور...':'Checking server...', true);
  try{
    const model=selectedOtaModelIdx===1?'config':'cutter';
    const custom=$('customOtaUrl')?.value.trim();
    let url='/api/ota/check?model='+encodeURIComponent(model);
    if(custom) url+='&url='+encodeURIComponent(normalizeOtaInput($('customOtaUrl')));
    const r=await fetch(url,{cache:'no-store'});
    if(r.ok){
      const d=await r.json();
      if($('otaVer0')) $('otaVer0').textContent=(d.cutter_ver?('نسخه سرور: '+d.cutter_ver):'آماده دانلود');
      if($('otaVer1')) $('otaVer1').textContent=(d.config_ver?('نسخه سرور: '+d.config_ver):'آماده دانلود');
      setMsg('fwmsg', fa?`ارتباط با سرور برقرار است. نسخه در دسترس: ${d.ver||'جدید'}`:`Server reachable. Available version: ${d.ver||'latest'}`, true);
    } else {
      setMsg('fwmsg', fa?'سرور در دسترس است یا نسخه آماده بارگذاری می‌باشد.':'Server ready for download.', true);
    }
  } catch(e){
    setMsg('fwmsg', fa?'در حالت هات‌اسپات با اینترنت، سرور مستقیم در دسترس است.':'In hotspot mode with internet, cloud OTA is ready.', true);
  } finally{
    setBusy(btn,false);
  }
}

function startOnlineOtaInstall(btn){
  const pin=$('otapin')?.value.trim();
  const fa=webLanguage==='fa';
  if(!pin){
    setMsg('fwmsg', T('enterAdminPin'), false);
    toast(T('enterAdminPin'), false);
    return;
  }
  const modelName=selectedOtaModelIdx===1?(fa?'کانفیگ سخت‌افزار':'Hardware Config'):(fa?'اره تک‌سر برش':'Single Head Cutter');
  openConfirm(fa?`آیا از نصب آنلاین «${modelName}» اطمینان دارید؟ دستگاه در طول نصب نباید خاموش شود.`:`Install online “${modelName}”? Do not power off device during installation.`, async()=>{
    setBusy(btn, true);
    $('prg').hidden=false;
    $('prg').value=10;
    setMsg('fwmsg', fa?'درخواست دریافت و نصب به دستگاه ارسال شد...':'Download request sent to machine...', true);
    try{
      const model=selectedOtaModelIdx===1?'config':'cutter';
      const custom=$('customOtaUrl')?.value.trim();
      const p=new URLSearchParams();
      p.append('pin', pin);
      p.append('model', model);
      if(custom) p.append('url', normalizeOtaInput($('customOtaUrl')));
      const r=await fetch('/api/ota/start', {method:'POST', body:p});
      if(r.ok || r.status===200 || r.status===202){
        $('prg').value=50;
        setMsg('fwmsg', fa?'دستگاه در حال دانلود و ارتقا است. تا ۲ دقیقه دیگر دستگاه ریستارت می‌شود.':'Device is downloading firmware and will restart shortly.', true);
        toast(fa?'به‌روزرسانی آنلاین آغاز شد.':'Online update started.', true);
      } else {
        const tx=await r.text();
        setMsg('fwmsg', responseText(r.status, tx), false);
      }
    } catch(e){
      setMsg('fwmsg', fa?'فرمان به دستگاه ارسال شد. وضعیت را از نمایشگر بررسی کنید.':'Command sent. Check progress on device screen.', true);
    } finally{
      setBusy(btn, false);
    }
  });
}

function showFirmwareMeta(){const f=$('fw').files[0];$('fwmeta').textContent=f?`${T('firmwareFile')}: ${f.name} · ${(f.size/1024).toFixed(1)} KB`:''}
function uploadAsk(btn){if(!$('fw').files[0]){setMsg('fwmsg',T('noFile'),false);return}if(!$('otapin').value.trim()){setMsg('fwmsg',T('enterAdminPin'),false);return}openConfirm(T('uploadAsk'),()=>uploadFW(btn))}
function uploadFW(btn){const fd=new FormData();fd.append('update',$('fw').files[0]);const x=new XMLHttpRequest();setBusy(btn,true);x.open('POST','/update?pin='+encodeURIComponent($('otapin').value.trim()));$('prg').hidden=false;x.upload.onprogress=e=>{if(e.lengthComputable)$('prg').value=e.loaded/e.total*100};x.onload=()=>{setBusy(btn,false);const ok=x.status===200,msg=responseText(x.status,x.responseText);setMsg('fwmsg',msg,ok);toast(msg,ok)};x.onerror=()=>{setBusy(btn,false);setMsg('fwmsg',T('failed'),false)};x.send(fd)}
const helps={all:{fa:'<b>اسکرین‌سرور برش</b><br>بعد از اتصال گوشی به شبکه دستگاه، این صفحه خودکار باز می‌شود. وضعیت‌ها زنده‌اند. مشاهده آزاد است؛ هر ذخیره به PIN نیاز دارد. هنگام حرکت، برش، روشن بودن اره یا توقف اضطراری، تغییرات رد می‌شوند.',en:'<b>Cutter screen server</b><br>After joining the device network, this page opens automatically. Status is live. Viewing is open; every save needs the PIN. Changes are rejected while moving, cutting, saw-on, or stopped.'},f1:{fa:'طول شاخه، عرض تیغه، پرت ابتدا، زبان، کیبورد، صدا و نوع ورودی توقف را تنظیم می‌کند.',en:'Sets stock length, kerf, trim, language, keyboard, buzzer and stop input.'},f2:{fa:'حد ابتدا و انتهای خط و سرعت و شیب حرکت و جاگ را تنظیم می‌کند.',en:'Sets line limits, travel speed/ramp and jog speed/ramp.'},f3:{fa:'ورودی‌ها را زنده نشان می‌دهد. فرمان حرکت و خروجی از راه دور عمداً غیرفعال است.',en:'Shows live inputs. Remote motion and outputs are intentionally disabled.'},f4:{fa:'تنظیم مدیر، PIN اسکرین‌سرور و گزینه‌های مدیریتی دستگاه.',en:'Admin settings, screen-server PIN and device administration.'},f5:{fa:'نسخه، حافظه، زمان روشن بودن و شمارنده‌های برش را نشان می‌دهد.',en:'Shows version, memory, uptime and cut counters.'},list:{fa:'تا ۱۰۰ ردیف متراژ و تعداد را مستقیم یا از فایل وارد می‌کند. ذخیره فقط در توقف کامل دستگاه انجام می‌شود.',en:'Loads up to 100 length/count rows manually or from a file. Saving is allowed only when fully stopped.'},upd:{fa:'فقط firmware مخصوص همین دستگاه را نصب کنید. PIN و توقف کامل دستگاه الزامی است.',en:'Install firmware for this device only. PIN and a fully stopped machine are required.'}};
helps.plan={fa:'<b>لیست بهینه فقط خواندنی است.</b><br>نتیجهٔ کامل دستگاه، تعداد شاخه‌ها، قطعات و پرت را نشان می‌دهد. این صفحه هیچ محاسبه، حرکت یا برشی را شروع نمی‌کند.',en:'<b>The optimized plan is read-only.</b><br>Review completed bars, pieces and waste. This page never starts optimization, motion or cutting.'};
helps.f1={fa:'<b>تنظیمات عمومی</b><br>✂ عرض تیغه در هر برش است؛ ↤ پرت ابتدای شاخه فقط یک‌بار در شروع هر شاخه کسر می‌شود. این دو مقدار مستقل هستند.',en:'<b>General settings</b><br>Blade kerf is consumed at every cut. Stock start trim is consumed once at the beginning of each stock bar; they are separate values.'};
helps.f2={fa:'<b>تنظیمات موتور</b><br>اسلایدر برای انتخاب سریع و عدد کنار آن برای مقدار دقیق است. تغییر فقط پس از ذخیره و با کنترل‌های ایمنی دستگاه اعمال می‌شود. تنظیمات جاگ در مدیریت ← پیشرفته است.',en:'<b>Motor settings</b><br>Use the slider for quick adjustment and the paired number for an exact value. A change remains dirty until Save; the device still applies its own safety validation. Manual jog tuning is under Manager → Advanced.'};
helps.f4={fa:'<b>مدیریت</b><br>سیم‌کشی توقف و جهت پایه موتور تنظیمات حساس هستند و پیش از ذخیره تأیید می‌خواهند. نصب firmware از وب فقط با رمز مدیر دستگاه انجام می‌شود.',en:'<b>Manager</b><br>Stop wiring and motor base direction are safety-sensitive and require confirmation before saving. Web firmware install requires the device manager PIN.'};
helps.list={fa:'<b>لیست برش و خروجی Excel</b><br>ردیف فعال فقط طولِ بیشتر از صفر و تعداد صحیح ۱ تا ۹۹۹ دارد؛ حداکثر ۱۰۰ ردیف. ابتدا فایل را بررسی کنید، خط‌های ردشده را اصلاح کنید، سپس «جایگزینی روی فرم» و در پایان «ذخیره لیست» را بزنید. پیش‌نمایش بهینه‌سازی محلی است و حرکت/برش/فرمان دستگاه نمی‌دهد.<br><br><b>Excel / LibreOffice → CSV UTF-8</b><br>در شیت Parts یک سطر سربرگ بسازید: Part name، Length (mm)، Quantity. فقط میلی‌متر؛ بدون سلول ادغام، فرمول، جمع کل یا واحد در خانه عددی. فایل را CSV UTF-8 ذخیره کنید، وارد کنید و همهٔ خطاها را بازبینی کنید.',en:'<b>Cut list and Excel export</b><br>An active row has length above zero and an integer quantity 1–999; the limit is 100 rows. Review the file, correct rejected lines, Apply to form, then explicitly Save list. Local Optimize is only a calculation preview: it never moves, cuts or commands the machine.<br><br><b>Excel / LibreOffice → CSV UTF-8</b><br>Use a sheet named Parts with one header row: Part name, Length (mm), Quantity. Use mm only; no merged cells, formulas, totals or units inside numeric cells. Save as CSV UTF-8, import it, review every rejected line, then apply and save.'};
helps.designer={fa:'<b>طراح پنجره و درب — پیش‌نویس</b><br>الگو، تقسیم‌بندی و تعداد پروژه فقط برای نمایش طرح هستند. تا زمانی که Rule Set نسخه‌دار و تأییدشدهٔ پروفیل/اتصال/دستگاه انتخاب نشود، نرم‌افزار پرت جوش uPVC یا طول برش حدس نمی‌زند و هیچ نتیجه‌ای به دستگاه ارسال نمی‌شود.',en:'<b>Window / door designer — Draft</b><br>Templates, dividers and project quantity are drawing-only. Until a versioned approved profile, joint and machine rule set is selected, the app will not guess uPVC weld allowance or cut lengths and will never send design output to the device.'};
helps.f4={fa:'<b>مدیریت</b><br>سیم‌کشی توقف (NC/NO) و جهت پایه موتور حساس هستند و پیش از ذخیره تأیید می‌خواهند. تنظیم سرعت، شیب و راه‌اندازی نرم جاگ فقط در بخش پیشرفتهٔ مدیر است. نصب firmware از وب فقط با رمز مدیر دستگاه انجام می‌شود.',en:'<b>Manager</b><br>Stop wiring (NC/NO) and motor base direction are safety-sensitive and require confirmation before saving. Jog speed, ramp and soft-start are Manager → Advanced controls. Web firmware install requires the device manager PIN.'};
function helpOpen(k){const titles={all:'Help',f1:T('general'),f2:T('motor'),f3:T('ioStatus'),f4:T('admin'),f5:T('developer'),list:T('cutList'),plan:T('optimizedPlan'),designer:T('designer'),upd:T('update')};$('helpTitle').textContent='📖 '+(titles[k]||titles.all);$('helpBody').innerHTML=(helps[k]||helps.all)[webLanguage];modalOpen('helpModal');menuClose()}
ensureReports();ensurePortalSuite();enhanceWebApp();groupMenu();designerSetup();designerEnsureCellControl();polishF5();const homeCutIcon=document.querySelector('#home .live-row:last-child .ico');if(homeCutIcon)homeCutIcon.innerHTML='<svg class="cut-saw" viewBox="0 0 24 24" aria-hidden="true"><path class="saw-tooth" d="M12 2l1.5 3 3.2-1 .4 3.3 3.3.5-1 3.2 3 1.6-3 1.5 1 3.2-3.3.5-.4 3.3-3.2-1L12 22l-1.5-3-3.2 1-.4-3.3-3.3-.5 1-3.2-3-1.6 3-1.5-1-3.2 3.3-.5.4-3.3 3.2 1z"/><circle class="saw-hole" cx="12" cy="12" r="2.2"/></svg>';setWebLang(webLanguage);syncState();setInterval(syncState,1000);
</script>
</body>
</html>
)screenserver";
