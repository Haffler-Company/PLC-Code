(function(){
  var LIST_MAX=100, ROWS=5;
  var REF={home:"refs/ref_home.png",main:"refs/ref_main.png",list:"refs/ref_list.png",f1:"refs/ref_f1_idle.png",f1np:"refs/ref_f1.png",opt:"refs/ref_opt.png"};
  var HITS={
    home:[[170,200,140,56,"ref-start"]],
    main:[[3,2,46,46,"goto-list"],[428,6,44,44,"goto-f1"],[111,7,250,83,"edit-sp"],[5,78,470,167,"move"]],
    list:[[3,2,44,39,"goto-main"],[55,2,44,39,"help"],[321,2,44,39,"plus"],[373,2,44,39,"minus"],[425,2,44,39,"opt"]],
    f1:[[6,6,44,39,"goto-main"],[55,6,44,39,"help"],[316,6,44,39,"lang"],[364,6,44,39,"save"],[84,67,136,46,"edit-stock"],[232,67,132,46,"edit-kerf"],[12,250,56,40,"webapp"]],
    opt:[[8,6,44,39,"goto-list"],[56,6,44,39,"help"],[40,278,80,34,"opt-prev"],[130,278,80,34,"opt-next"],[220,278,90,34,"opt-move"],[320,278,80,34,"opt-skip"]]
  };
  var state={
    scr:"main", referenced:true, posBefore:120, pos:120, sp:60, inPos:false, fine:false, jogging:false,
    count:"--", selected:-1, userCuts:0, stock:6000, kerf:3.0, trim:0, lang:"fa",
    npOpen:false, npField:"stock", npBuf:"", npCtx:"f1",
    list:{visible:5,page:0,sp:[2000,1500,580,860,1200].concat(Array(95).fill(0)),cnt:[6,10,20,8,6].concat(Array(95).fill(0)),focus:null},
    io:{I:[0,1,1,0,0,0,0,0],Q:[0,0,0,0,0,0]},
    f2:{speed:70,init:40,startMm:0,endMm:6000,baseDir:1},
    f4:{portal:false,pin:"1234"},
    optPacks:[], optIdx:0, optWaste:0, optBranches:0, optMode:false,
    help:false, sensor:false, refFail:false, toast:"", debug:false, listDirty:false, mainDirty:false,
    compare:false, swipeX:null, _editIdx:0
  };
  var bg=document.getElementById("bg"), live=document.getElementById("live"), hits=document.getElementById("hits");
  var logEl=document.getElementById("log"), fine=document.getElementById("fine"), np=document.getElementById("np");
  var toastEl=document.getElementById("toast"), badge=document.getElementById("badge"), cmp=document.getElementById("cmp");

  function log(m,c){var d=document.createElement("div");d.className=c||"info";d.textContent="["+new Date().toLocaleTimeString("en-GB",{hour12:false})+"] "+m;logEl.prepend(d);}
  function maxTop(){return state.list.visible<=ROWS?0:state.list.visible-ROWS;}
  function clampPage(){state.list.page=Math.max(0,Math.min(state.list.page,maxTop()));}
  function pageChange(d){var n=state.list.page+d;if(n<0||n>maxTop()){log("end list","bad");return false;}state.list.page=n;state.listDirty=true;log("top="+n+"/"+maxTop(),"ok");return true;}
  function addRow(){if(state.list.visible>=LIST_MAX){log("full","bad");return false;}var i=state.list.visible;state.list.sp[i]=0;state.list.cnt[i]=0;state.list.visible++;state.list.page=Math.max(0,state.list.visible-ROWS);clampPage();state.listDirty=true;log("+ row "+state.list.visible,"ok");return true;}
  function remRow(){if(state.list.visible<=0)return false;state.list.visible--;clampPage();state.listDirty=true;log("- row","ok");return true;}
  function planLeft(t){return ["leg1->"+(t-5),"leg2->"+t];}
  function moveToSp(){var cur=state.pos,t=state.sp;log("move "+cur+"->"+t+": "+(t<cur?planLeft(t).join(" | "):"direct"),"ok");state.pos=t;state.inPos=true;state.mainDirty=true;}
  function toast(m){state.toast=m;toastEl.textContent=m;toastEl.classList.add("on");setTimeout(function(){toastEl.classList.remove("on");state.toast="";},1500);}
  function setScr(s){state.scr=s;document.querySelectorAll("[data-scr]").forEach(function(b){b.classList.toggle("on",b.dataset.scr===s);});}
  function listDefaultMatch(){var d=[2000,1500,580,860,1200],c=[6,10,20,8,6];if(state.list.visible!==5||state.list.page!==0||state.list.focus!=null||state.selected>=0)return false;for(var i=0;i<5;i++)if(state.list.sp[i]!==d[i]||state.list.cnt[i]!==c[i])return false;return !state.listDirty;}
  function mainDefaultMatch(){return !state.mainDirty && state.sp===60 && state.pos===120 && state.count==="--" && state.selected<0 && !state.fine && !state.jogging;}

  function runFFD(){
    var pieces=[];
    for(var i=0;i<state.list.visible;i++){var L=state.list.sp[i],C=state.list.cnt[i];if(L<=0||C<=0)continue;if(L>state.stock-state.trim){toast("row "+(i+1)+" > stock");return false;}for(var k=0;k<C;k++)pieces.push({L:L,src:i});}
    if(!pieces.length){toast("empty");return false;}
    pieces.sort(function(a,b){return b.L-a.L;});
    var useful=state.stock-state.trim, branches=[], waste=0;
    for(var p=0;p<pieces.length;p++){var placed=false,need=pieces[p].L+state.kerf;for(var b=0;b<branches.length;b++){if(branches[b].left>=need){branches[b].left-=need;branches[b].items.push(pieces[p]);placed=true;break;}}
      if(!placed){if(pieces[p].L>useful){toast("too long");return false;}branches.push({left:useful-pieces[p].L-state.kerf,items:[pieces[p]]});}}
    for(var b2=0;b2<branches.length;b2++)waste+=Math.max(0,branches[b2].left);
    state.optPacks=branches;state.optIdx=0;state.optWaste=Math.round(waste);state.optBranches=branches.length;state.optMode=true;log("FFD B="+branches.length+" waste="+state.optWaste,"ok");return true;
  }

  function renderListLive(){
    if(state.scr!=="list")return ""; if(!state.listDirty&&listDefaultMatch())return "";
    var base=state.list.page,n=Math.max(0,Math.min(ROWS,state.list.visible-base)),rows="",i,g;
    for(i=0;i<n;i++){g=base+i;rows+='<div class="lrow '+(i%2?"odd":"even")+(state.selected===g?" sel":"")+'" style="top:'+(i*44)+'px"><div class="idx'+(state.selected===g?" sel":"")+'">'+String(g+1).padStart(2,"0")+'</div><div class="sp'+(state.list.focus==="sp-"+g?" focus":"")+'">'+state.list.sp[g]+'</div><div class="xx">x</div><div class="cq'+(state.list.focus==="cnt-"+g?" focus":"")+'">'+state.list.cnt[g]+'</div></div>';}
    var mt=maxTop(),th=mt<=0?220:Math.max(24,Math.round(220*ROWS/Math.max(ROWS,state.list.visible)));
    var ty=mt<=0?0:Math.round((220-th)*(state.list.page/mt));
    return '<div class="panel">'+rows+'</div>'+(mt>0?('<div class="scroll"><div class="thumb" style="height:'+th+'px;top:'+ty+'px"></div></div>'):"");
  }

  function renderMainLive(){
    if(state.scr!=="main"||mainDefaultMatch())return "";
    var cnt=state.count==="--"? "---- ----" : String(state.count);
    return '<div class="mainlive"><div class="ml-sp">'+state.sp+'</div><div class="ml-pos">'+state.pos+'</div><div class="ml-cnt">'+cnt+'</div></div>';
  }

  function tabsHtml(active){
    return ["F1","F2","F3","F4","F5"].map(function(t){return '<div class="ftab'+(t.toLowerCase()===active?" on":"")+'" data-act="tab" data-t="'+t+'">'+t+'</div>';}).join("");
  }

  function renderF2(){
    var s=state.f2;
    return '<div class="draw"><div class="fpage"><div class="ftitle">Speed / Limits</div><div class="f2box">'+
      '<div class="f2row"><span>Speed %</span><input type="range" min="8" max="100" value="'+s.speed+'" data-act="f2-speed"/><span class="f2val" id="f2sp">'+s.speed+'</span></div>'+
      '<div class="f2row"><span>Init %</span><input type="range" min="8" max="100" value="'+s.init+'" data-act="f2-init"/><span class="f2val" id="f2in">'+s.init+'</span></div>'+
      '<div class="f2row"><span>Start mm</span><button class="f2btn" type="button" data-act="f2-edit" data-f="startMm">'+s.startMm+'</button></div>'+
      '<div class="f2row"><span>End mm</span><button class="f2btn" type="button" data-act="f2-edit" data-f="endMm">'+s.endMm+'</button></div>'+
      '<div class="f2row"><span>Base dir</span><button class="f2btn" type="button" data-act="f2-dir">'+(s.baseDir>0?"+":"-")+'</button></div>'+
      '<div class="f2row"><button class="f2btn" type="button" data-act="save">Save F2</button><span style="color:#8E8E93;font-size:11px">motion params</span></div>'+
      '</div></div><div class="ftabs">'+tabsHtml("f2")+'</div></div>';
  }

  function renderF3(){
    var rows="", labs=["Door","Home sn","End sn","Stop"];
    for(var i=0;i<4;i++) rows+='<div class="f3row" style="top:'+(8+i*36)+'px"><span class="led'+(state.io.I[i]?" on":"")+'" data-act="tog-I" data-i="'+i+'"></span>'+labs[i]+'<span class="pin">I'+i+'</span></div>';
    rows+='<div class="f3row" style="top:152px;width:50%"><span class="led'+(state.io.I[6]?" on":"")+'" data-act="tog-I" data-i="6"></span>Start1<span class="pin">I6</span></div>';
    rows+='<div class="f3row" style="top:152px;left:50%;width:50%"><span class="led'+(state.io.I[7]?" on":"")+'" data-act="tog-I" data-i="7"></span>Start2<span class="pin">I7</span></div>';
    rows+='<div class="f3row" style="top:200px;width:50%"><span class="led'+(state.io.I[4]?" on":"")+'" data-act="tog-I" data-i="4"></span>Left<span class="pin">I4</span></div>';
    rows+='<div class="f3row" style="top:200px;left:50%;width:50%"><span class="led'+(state.io.I[5]?" on":"")+'" data-act="tog-I" data-i="5"></span>Right<span class="pin">I5</span></div>';
    var outs=[["Q5","Spray"],["Q3","Clamp"],["Q6","Saw"],["Q4","Guard"],["Q8","Lift"],["Q7","Lamp"]];
    var pos=[[184,48],[296,48],[184,128],[296,128],[184,208],[296,208]], qs="";
    for(var j=0;j<6;j++) qs+='<div class="f3q'+(state.io.Q[j]?" on":"")+'" style="left:'+pos[j][0]+'px;top:'+pos[j][1]+'px" data-act="tog-Q" data-i="'+j+'"><span class="b">'+outs[j][0]+'</span>'+outs[j][1]+'</div>';
    return '<div class="draw"><div class="fpage"><div class="ftitle">Manual IO Test</div><div class="f3ins">'+rows+'</div>'+qs+'</div><div class="ftabs">'+tabsHtml("f3")+'</div></div>';
  }

  function renderF4(){
    return '<div class="draw"><div class="fpage"><div class="ftitle">Web / Portal</div><div class="f2box">'+
      '<div class="f2row"><span>Portal</span><button class="f2btn" type="button" data-act="portal-tog">'+(state.f4.portal?"ON":"OFF")+'</button></div>'+
      '<div class="f2row"><span>PIN</span><button class="f2btn" type="button" data-act="edit-pin">'+state.f4.pin+'</button></div>'+
      '<div style="margin-top:12px;color:#8E8E93;font:12px Tahoma;line-height:1.5">AP portal stays until reboot. Settings need PIN. Import list via web/serial.</div>'+
      '</div></div><div class="ftabs">'+tabsHtml("f4")+'</div></div>';
  }

  function renderF5(){
    return '<div class="draw"><div class="fpage"><div class="ftitle">Service</div><div class="f2box">'+
      '<div class="f2row"><span>Shift cuts</span><span class="f2val">'+state.userCuts+'</span><button class="f2btn" type="button" data-act="reset-shift">Reset</button></div>'+
      '<div class="f2row"><button class="f2btn" type="button" data-act="factory-hint" style="background:#FF3B30">Factory reset hint</button></div>'+
      '<div style="margin-top:12px;color:#8E8E93;font:12px Tahoma;line-height:1.5">Hold CANCEL 5s on panel for factory dialog (firmware). Simulator only shows hint.</div>'+
      '</div></div><div class="ftabs">'+tabsHtml("f5")+'</div></div>';
  }

  function renderOptLive(){
    if(!state.optPacks.length) return "";
    var b=state.optPacks[state.optIdx]||state.optPacks[0], lines="";
    for(var i=0;i<(b?b.items.length:0);i++) lines+='<div class="optline'+(i===0?" cur":"")+'">'+b.items[i].L+'</div>';
    return '<div class="optlive"><div style="position:absolute;left:112px;top:10px;width:206px;height:34px;background:#fff;border-radius:10px;display:grid;place-items:center;font:700 16px Tahoma;color:#111">Opt Cut</div><div class="optcard"><div style="display:flex;justify-content:space-between;margin-bottom:8px"><span style="background:#1C9B67;color:#fff;padding:4px 10px;border-radius:8px;font:700 13px Tahoma">B '+(state.optIdx+1)+'/'+state.optBranches+'</span><span style="background:#c62828;color:#fff;padding:4px 10px;border-radius:8px;font:700 13px Tahoma;text-align:center">Waste<br>'+state.optWaste+'</span></div>'+lines+'</div><div class="optbar"><button style="background:#8E8E93" type="button" data-act="opt-prev">Prev</button><button style="background:#0A84FF" type="button" data-act="opt-next">Next</button><button style="background:#1C9B67" type="button" data-act="opt-move">Move</button><button style="background:#FF9500;color:#111" type="button" data-act="opt-skip">Skip</button></div></div>';
  }

  function renderHits(){
    var list=HITS[state.scr]||[], html="";
    for(var i=0;i<list.length;i++){var h=list[i];html+='<button type="button" class="hit'+(state.debug?" debug":"")+'" data-act="'+h[4]+'" style="left:'+h[0]+'px;top:'+h[1]+'px;width:'+h[2]+'px;height:'+h[3]+'px"></button>';}
    if(state.scr==="list"){var base=state.list.page,n=Math.max(0,Math.min(ROWS,state.list.visible-base));for(var r=0;r<n;r++){var g=base+r,y=88+r*44;html+='<button type="button" class="hit'+(state.debug?" debug":"")+'" data-act="select-row" data-i="'+g+'" style="left:7px;top:'+(y+3)+'px;width:50px;height:36px"></button>';html+='<button type="button" class="hit'+(state.debug?" debug":"")+'" data-act="focus-sp" data-i="'+g+'" style="left:76px;top:'+(y+5)+'px;width:142px;height:30px"></button>';html+='<button type="button" class="hit'+(state.debug?" debug":"")+'" data-act="focus-cnt" data-i="'+g+'" style="left:271px;top:'+(y+5)+'px;width:138px;height:30px"></button>';}}
    hits.innerHTML=html;
  }

  function renderNp(){var keys=["1","2","3","4","5","6","7","8","9",".","0","BS"];np.innerHTML=keys.map(function(k){return '<button type="button" data-act="np" data-k="'+k+'">'+k+'</button>';}).join("")+'<button class="ok" type="button" data-act="np-ok">OK</button>';np.classList.toggle("on",state.npOpen&&(state.scr==="f1"||state.scr==="list"||state.scr==="main"||state.scr==="f2"));}

  function helpContent(){var t=document.getElementById("helpTitle"),b=document.getElementById("helpBody");
    if(state.scr==="list"||state.scr==="opt"){t.textContent="List / Opt";b.innerHTML='<div class="sec">Select</div><div>Green index -> load to main.</div><div class="sec">Swipe</div><div>Horizontal page.</div><div class="sec">FFD</div><div>Descending pack into stock. Prev/Next branch, Move, Skip.</div>';}
    else if(state.scr.indexOf("f")===0){t.textContent="Settings";b.innerHTML='<div class="sec">F1</div><div>Stock/kerf/web.</div><div class="sec">F2</div><div>Speed/limits.</div><div class="sec">F3</div><div>Toggle I/Q.</div><div class="sec">F4</div><div>Portal/PIN.</div><div class="sec">F5</div><div>Shift reset.</div>';}
    else if(state.scr==="home"){t.textContent="Reference";b.innerHTML='<div class="sec">2 stages</div><div>Approach + creep back. Fail restores pos.</div>';}
    else{t.textContent="Main";b.innerHTML='<div class="sec">Move</div><div>Touch red number. Left backlash 5mm.</div><div class="sec">Jog</div><div>Jog L/R + Stop. Fine /6.</div><div class="sec">Cut</div><div>Cut OK decrements qty.</div>';}
  }

  function renderCompare(){
    if(!state.compare){cmp.classList.remove("on");return;}
    cmp.classList.add("on");
    cmp.textContent=JSON.stringify({scr:state.scr,ref:state.referenced,pos:state.pos,sp:state.sp,count:state.count,sel:state.selected,listVis:state.list.visible,page:state.list.page,stock:state.stock,kerf:state.kerf,optB:state.optBranches,waste:state.optWaste,f2:state.f2,cuts:state.userCuts,io:state.io},null,2);
  }

  function render(){
    var useImg=["home","main","list","f1","opt"].indexOf(state.scr)>=0;
    if(useImg){var src=REF[state.scr];if(state.scr==="f1"&&state.npOpen&&state.npCtx==="f1")src=REF.f1np;bg.style.display="block";bg.src=src;}else bg.style.display="none";
    var html="";
    if(state.scr==="list")html=renderListLive();
    if(state.scr==="main")html=renderMainLive();
    if(state.scr==="f2")html=renderF2();
    if(state.scr==="f3")html=renderF3();
    if(state.scr==="f4")html=renderF4();
    if(state.scr==="f5")html=renderF5();
    if(state.scr==="opt"){if(!state.optPacks.length)runFFD();html=renderOptLive();}
    if(state.scr==="home")html='<button class="homebtn" type="button" data-act="ref-start">Start Ref</button>';
    live.innerHTML=html;fine.classList.toggle("on",state.fine&&state.scr==="main");
    document.getElementById("help").classList.toggle("on",state.help);
    document.getElementById("sensor").classList.toggle("on",state.sensor);
    document.getElementById("refFail").classList.toggle("on",state.refFail);
    if(state.help)helpContent();renderHits();renderNp();renderCompare();
    badge.textContent=state.scr.toUpperCase()+" | ref="+(state.referenced?1:0)+" | sel="+(state.selected<0?"-":state.selected+1)+" | cuts="+state.userCuts+" | jog="+(state.jogging?1:0)+(state.optMode?" | OPT":"");
  }

  function selfTest(){
    var fail=0,bak=JSON.parse(JSON.stringify(state.list));
    state.list.visible=0;state.list.page=0;if(!addRow()||state.list.sp[0]!==0)fail++;
    state.list.visible=ROWS;state.list.page=0;if(pageChange(1)!==false)fail++;
    state.list.visible=ROWS+1;state.list.page=0;if(!pageChange(1)||state.list.page!==1)fail++;
    if(planLeft(500)[0].indexOf("495")<0)fail++;
    state.list=JSON.parse(JSON.stringify(bak));
    if(!runFFD()||state.optBranches<1)fail++;
    var bi=state.optIdx;onAct("opt-next",{});if(state.optBranches>1&&state.optIdx===bi)fail++;
    state.list=bak;state.listDirty=false;state.optPacks=[];state.optMode=false;
    if(fail===0){log("selfTest OK","ok");toast("logic OK");}else{log("FAIL "+fail,"bad");toast("FAIL");}
    render();return fail===0;
  }

  function confirmCut(){
    if(state.selected<0){for(var i=0;i<state.list.visible;i++)if(state.list.cnt[i]>0){state.selected=i;break;}}
    if(state.selected<0){toast("no row");return;}
    var i=state.selected;if(state.list.cnt[i]<=0){toast("qty 0");return;}
    state.list.cnt[i]--;state.listDirty=true;state.mainDirty=true;state.userCuts++;state.count=String(state.list.cnt[i]);
    log("cut row "+(i+1)+" left="+state.list.cnt[i],"ok");toast("cut -1");
    if(state.optMode)log("print label "+state.sp+" B"+(state.optIdx+1),"ok");
    if(state.list.cnt[i]<=0){var next=-1;for(var j=i+1;j<state.list.visible;j++)if(state.list.cnt[j]>0){next=j;break;}if(next<0)for(var j2=0;j2<state.list.visible;j2++)if(state.list.cnt[j2]>0){next=j2;break;}
      if(next>=0){state.selected=next;state.sp=state.list.sp[next];state.inPos=false;log("auto next "+(next+1),"ok");}else{toast("list done");state.selected=-1;}}
    render();
  }

  function onAct(act,el){
    if(act==="goto-list"){setScr("list");state.npOpen=false;}
    if(act==="goto-main"){setScr("main");state.npOpen=false;state.help=false;}
    if(act==="goto-f1"){setScr("f1");state.npOpen=false;}
    if(act==="opt"){setScr("opt");state.npOpen=false;runFFD();}
    if(act==="plus")addRow(); if(act==="minus")remRow();
    if(act==="help")state.help=true; if(act==="close-help")state.help=false;
    if(act==="close-sensor"){state.sensor=false;log("sensor closed","ok");}
    if(act==="ref-start"){state.posBefore=state.pos;log("ref stage1","ok");log("ref stage2","ok");state.referenced=true;state.pos=0;state.mainDirty=true;setScr("main");toast("ref OK");log("referenced pos=0","ok");}
    if(act==="ref-fail-ok"){state.refFail=false;state.pos=state.posBefore;state.referenced=false;state.mainDirty=true;log("pos restored "+state.pos,"ok");}
    if(act==="ref-retry"){state.refFail=false;onAct("ref-start",el);return;}
    if(act==="edit-sp"){state.npCtx="main-sp";state.npBuf=String(state.sp);state.npOpen=true;}
    if(act==="move"){if(!state.referenced){toast("not referenced");return;}if(state.jogging){toast("jogging");return;}if(!state.inPos)moveToSp();else toast("in position");}
    if(act==="select-row"){var i=+el.dataset.i;state.selected=i;state.sp=state.list.sp[i];state.count=String(state.list.cnt[i]);state.inPos=false;state.mainDirty=true;setScr("main");toast("row -> main");log("select "+(i+1),"ok");}
    if(act==="focus-sp"){state.list.focus="sp-"+el.dataset.i;state.listDirty=true;state.npCtx="list-sp";state.npBuf=String(state.list.sp[+el.dataset.i]);state.npOpen=true;state._editIdx=+el.dataset.i;}
    if(act==="focus-cnt"){state.list.focus="cnt-"+el.dataset.i;state.listDirty=true;state.npCtx="list-cnt";state.npBuf=String(state.list.cnt[+el.dataset.i]);state.npOpen=true;state._editIdx=+el.dataset.i;}
    if(act==="edit-stock"){state.npCtx="f1";state.npField="stock";state.npBuf=String(state.stock);state.npOpen=true;}
    if(act==="edit-kerf"){state.npCtx="f1";state.npField="kerf";state.npBuf=String(state.kerf);state.npOpen=true;}
    if(act==="webapp"){setScr("f4");toast("web app");}
    if(act==="np"){var k=el.dataset.k;if(k==="BS")state.npBuf=state.npBuf.slice(0,-1);else if(state.npBuf.length<6)state.npBuf+=k;}
    if(act==="np-ok"){var n=parseFloat(state.npBuf);if(!isNaN(n)){if(state.npCtx==="f1"&&state.npField==="stock")state.stock=Math.round(n);else if(state.npCtx==="f1"&&state.npField==="kerf")state.kerf=Math.round(n*10)/10;else if(state.npCtx==="main-sp"){state.sp=Math.round(n);state.inPos=false;state.mainDirty=true;}else if(state.npCtx==="list-sp"){state.list.sp[state._editIdx]=Math.round(n);state.listDirty=true;state.list.focus="cnt-"+state._editIdx;state.npCtx="list-cnt";state.npBuf=String(state.list.cnt[state._editIdx]);render();return;}else if(state.npCtx==="list-cnt"){state.list.cnt[state._editIdx]=Math.round(n);state.listDirty=true;state.list.focus=null;}else if(state.npCtx==="f2"){state.f2[state.npField]=Math.round(n);}else if(state.npCtx==="pin"){state.f4.pin=String(Math.round(n));}}state.npOpen=false;log("numpad OK","ok");}
    if(act==="save")toast("saved");
    if(act==="lang"){state.lang=state.lang==="fa"?"en":"fa";toast("lang="+state.lang);}
    if(act==="tab"){var t=el.dataset.t.toLowerCase();setScr(t);}
    if(act==="tog-I"){var ii=+el.dataset.i;state.io.I[ii]=state.io.I[ii]?0:1;log("I"+ii+"="+state.io.I[ii],"ok");}
    if(act==="tog-Q"){var qi=+el.dataset.i;state.io.Q[qi]=state.io.Q[qi]?0:1;log("Q"+qi+"="+state.io.Q[qi],"ok");}
    if(act==="f2-speed"){state.f2.speed=+el.value;var sp=document.getElementById("f2sp");if(sp)sp.textContent=state.f2.speed;return;}
    if(act==="f2-init"){state.f2.init=+el.value;var inn=document.getElementById("f2in");if(inn)inn.textContent=state.f2.init;return;}
    if(act==="f2-edit"){state.npCtx="f2";state.npField=el.dataset.f;state.npBuf=String(state.f2[el.dataset.f]);state.npOpen=true;}
    if(act==="f2-dir"){state.f2.baseDir=state.f2.baseDir>0?-1:1;log("baseDir="+state.f2.baseDir,"ok");}
    if(act==="portal-tog"){state.f4.portal=!state.f4.portal;log("portal="+state.f4.portal,"ok");}
    if(act==="edit-pin"){state.npCtx="pin";state.npBuf=state.f4.pin;state.npOpen=true;}
    if(act==="reset-shift"){state.userCuts=0;toast("shift 0");log("shift reset","ok");}
    if(act==="factory-hint"){toast("hold CANCEL 5s");log("factory dialog is panel-only","info");}
    if(act==="opt-prev"){if(state.optIdx>0)state.optIdx--;else toast("first B");}
    if(act==="opt-next"){if(state.optIdx<state.optBranches-1)state.optIdx++;else toast("last B");}
    if(act==="opt-move"){var b=state.optPacks[state.optIdx];if(!b||!b.items.length){toast("empty B");return;}if(!state.referenced){toast("not referenced");return;}state.sp=b.items[0].L;state.mainDirty=true;state.inPos=false;moveToSp();toast("moved "+state.sp);}
    if(act==="opt-skip"){var b2=state.optPacks[state.optIdx];if(b2&&b2.items.length){b2.items.shift();log("skip piece","ok");if(!b2.items.length)onAct("opt-next",el);} }
    render();
  }

  document.getElementById("screen").addEventListener("click",function(ev){var el=ev.target.closest("[data-act]");if(!el)return;onAct(el.getAttribute("data-act"),el);});
  document.getElementById("screen").addEventListener("input",function(ev){var el=ev.target.closest("[data-act]");if(!el)return;onAct(el.getAttribute("data-act"),el);});
  document.getElementById("screen").addEventListener("pointerdown",function(e){if(state.scr==="list")state.swipeX=e.clientX;});
  document.getElementById("screen").addEventListener("pointerup",function(e){if(state.scr!=="list"||state.swipeX==null)return;var dx=e.clientX-state.swipeX;state.swipeX=null;if(Math.abs(dx)<70)return;pageChange(dx<0?1:-1);render();});
  document.querySelectorAll("[data-scr]").forEach(function(b){b.addEventListener("click",function(){setScr(b.dataset.scr);state.npOpen=false;state.help=false;if(b.dataset.scr==="opt")runFFD();render();log("-> "+state.scr);});});
  document.getElementById("btnTest").onclick=selfTest;
  document.getElementById("btnFine").onclick=function(){state.fine=!state.fine;state.mainDirty=true;setScr("main");log("fine="+state.fine,state.fine?"ok":"info");render();};
  document.getElementById("btnSensor").onclick=function(){setScr("main");state.sensor=true;render();};
  document.getElementById("btnHelp").onclick=function(){state.help=true;render();};
  document.getElementById("btnDebug").onclick=function(){state.debug=!state.debug;render();};
  document.getElementById("btnCmp").onclick=function(){state.compare=!state.compare;render();};
  document.getElementById("btnRefFail").onclick=function(){state.posBefore=state.pos;state.refFail=true;setScr("home");render();log("ref fail demo","bad");};
  document.getElementById("btnJogL").onclick=function(){if(!state.referenced){toast("not referenced");return;}state.jogging=true;setScr("main");log("jog LEFT"+(state.fine?" fine/6":""),"ok");toast("Jog L");render();};
  document.getElementById("btnJogR").onclick=function(){if(!state.referenced){toast("not referenced");return;}state.jogging=true;setScr("main");log("jog RIGHT"+(state.fine?" fine/6":""),"ok");toast("Jog R");render();};
  document.getElementById("btnStop").onclick=function(){state.jogging=false;log("keyboardStopJog","ok");toast("Stop");render();};
  document.getElementById("btnCut").onclick=confirmCut;
  document.getElementById("btnNext").onclick=function(){var start=state.selected<0?0:state.selected+1,next=-1;for(var i=start;i<state.list.visible;i++)if(state.list.cnt[i]>0){next=i;break;}if(next<0){toast("list done");return;}state.selected=next;state.sp=state.list.sp[next];state.count=String(state.list.cnt[next]);state.inPos=false;state.mainDirty=true;setScr("main");log("next "+(next+1),"ok");render();};
  document.getElementById("btnUnref").onclick=function(){state.referenced=false;setScr("home");toast("need ref");render();};
  window.__hmiSim={state:state,render:render,selfTest:selfTest,goto:function(s){setScr(s);render();},getScreenEl:function(){return document.getElementById("screen");},runFFD:runFFD};
  render();log("Complete twin v3 ready","ok");
})();
