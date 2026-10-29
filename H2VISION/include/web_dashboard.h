#ifndef H2_VISION_WEB_DASHBOARD_H
#define H2_VISION_WEB_DASHBOARD_H

#include <Arduino.h>

// Dashboard HTML / CSS / JS hébergé directement en mémoire Flash (PROGMEM) de l'ESP32
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<title>H2-Vision – Poste de stand</title>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Barlow+Condensed:wght@500;700&family=IBM+Plex+Sans:wght@400;500;600&display=swap">
<style>
:root{--bg:#eef3f7;--panel:#fff;--line:#d3dde6;--ink:#10202e;--mute:#5d7285;--h2:#0a8fb0;--amb:#c77a00;--ok:#1b8a4b;--bad:#c62f3b;--badbg:#fde8ea;--rec:#d62839;
box-sizing:border-box;padding-top:env(safe-area-inset-top,0px);padding-bottom:env(safe-area-inset-bottom,0px)}
@media (prefers-color-scheme:dark){:root:not([data-theme="light"]){color-scheme:dark;--bg:#0e1a26;--panel:#152536;--line:#25394d;--ink:#e6eef5;--mute:#8aa3b8;--h2:#3fd0f0;--amb:#f3b23c;--ok:#4cd08a;--bad:#ff6b76;--badbg:#3a1d26;--rec:#ff4d5e}}
*{box-sizing:border-box}
html{scroll-padding-top:calc(env(safe-area-inset-top,0px) + 64px);scroll-behavior:smooth}
body{margin:0;background:var(--bg);color:var(--ink);font:400 15px/1.45 "IBM Plex Sans",system-ui,sans-serif}
nav{position:sticky;top:env(safe-area-inset-top,0px);z-index:9;background:var(--panel);border-bottom:1px solid var(--line)}
.nav{max-width:1180px;margin:0 auto;padding:8px 16px;display:flex;flex-wrap:wrap;gap:10px;align-items:center;justify-content:space-between}
.links{display:flex;gap:4px;align-items:center;flex-wrap:wrap}
.links b{font:700 22px "Barlow Condensed",sans-serif;margin-right:10px}.links b span{color:var(--h2)}
.links a{color:var(--mute);text-decoration:none;padding:6px 10px;border-radius:6px;font-weight:500;font-size:14px}
.links a:hover{background:var(--bg);color:var(--ink)}
.bar{display:flex;gap:8px;align-items:center;flex-wrap:wrap}
.wrap{max-width:1180px;margin:0 auto;padding:16px}
section.blk{margin-bottom:34px}
h2.t{font:700 26px "Barlow Condensed",sans-serif;margin:0 0 12px;letter-spacing:.02em}
.pill{display:inline-flex;gap:6px;align-items:center;padding:5px 11px;border:1px solid var(--line);border-radius:99px;background:var(--bg);font-size:13px;color:var(--mute)}
.pill b{color:var(--ink);font-weight:600}
.dot{width:9px;height:9px;border-radius:50%;background:var(--ok)}.dot.off{background:var(--bad)}
button{font:500 14px "IBM Plex Sans",sans-serif;padding:7px 14px;border-radius:7px;border:1px solid var(--line);background:var(--panel);color:var(--ink);cursor:pointer}
button:hover{background:var(--bg)}
button.main{background:var(--h2);border-color:var(--h2);color:#04202a;font-weight:600}
button.rec{background:var(--rec);border-color:var(--rec);color:#fff}
button:disabled{opacity:.45;cursor:not-allowed}
kbd{font:600 12px "IBM Plex Sans",sans-serif;border:1px solid currentColor;border-radius:4px;padding:0 5px;margin-left:6px}
#alarm{display:none;margin-bottom:12px;padding:10px 14px;border-radius:8px;background:var(--badbg);color:var(--bad);border:1px solid var(--bad);font-weight:600}
#recbar{display:flex;flex-wrap:wrap;gap:12px;align-items:center;justify-content:space-between;padding:12px 16px;border-radius:10px;border:1px solid var(--line);background:var(--panel);margin-bottom:12px}
#recbar.on{border-color:var(--rec);box-shadow:inset 4px 0 0 var(--rec)}
#recbar .big{font:700 34px "Barlow Condensed",sans-serif}
.grid{display:grid;gap:12px;grid-template-columns:repeat(12,1fr)}
.card{background:var(--panel);border:1px solid var(--line);border-radius:10px;padding:14px;min-width:0}
.card h3{font:500 13px "IBM Plex Sans",sans-serif;color:var(--mute);margin:0 0 6px}
.val{font:700 44px/1 "Barlow Condensed",sans-serif}
.val small{font:500 16px "IBM Plex Sans",sans-serif;color:var(--mute);margin-left:5px}
.sub{font-size:12.5px;color:var(--mute);margin-top:6px}
.speed{grid-column:span 4}.kpi{grid-column:span 2}.third{grid-column:span 4}.half{grid-column:span 6}.full{grid-column:span 12}
.speed .val{font-size:84px;color:var(--h2)}
.meter{height:8px;border-radius:5px;background:var(--line);overflow:hidden;margin-top:10px}
.meter i{display:block;height:100%;width:0;background:var(--h2);transition:width .2s}
canvas{width:100%;height:190px;display:block}
.legend{display:flex;flex-wrap:wrap;gap:14px;font-size:12.5px;color:var(--mute);margin-top:6px}
.legend i{display:inline-block;width:10px;height:10px;border-radius:2px;margin-right:5px}
.wide{overflow-x:auto}
table{width:100%;border-collapse:collapse;font-size:13.5px;min-width:560px}
td,th{padding:7px 6px;text-align:right;border-bottom:1px solid var(--line);white-space:nowrap}
td:first-child,th:first-child{text-align:left}
th{color:var(--mute);font-weight:500}
td.best{color:var(--ok);font-weight:600}
.sw{display:inline-block;width:11px;height:11px;border-radius:3px;margin-right:7px;vertical-align:-1px}
.x{padding:2px 8px}
#verdict{padding:12px 16px;border-radius:10px;border:1px solid var(--ok);background:var(--panel);margin-bottom:12px}
.empty{color:var(--mute);padding:18px 0}
select{font:inherit;padding:4px 8px;border-radius:6px;border:1px solid var(--line);background:var(--panel);color:var(--ink)}
td small{font-size:11.5px;margin-left:4px}
#ana li{margin:4px 0}
#toast{position:fixed;left:50%;bottom:20px;transform:translateX(-50%);background:var(--ink);color:var(--bg);padding:9px 16px;border-radius:8px;font-size:13.5px;opacity:0;transition:opacity .25s;pointer-events:none;z-index:99}
@media (max-width:900px){.speed,.half,.third{grid-column:span 12}.kpi{grid-column:span 6}}
</style>
</head>
<body>
<nav><div class="nav">
  <div class="links"><b>H2<span>-Vision</span></b><a href="#live">Direct</a><a href="#graphes">Graphiques</a><a href="#tours">Tours</a><a href="#comparaison">Comparaison</a></div>
  <div class="bar">
    <span class="pill"><i class="dot" id="ldot"></i>Wi-Fi ESP32 · RSSI <b id="rssi">-45</b> dBm</span>
    <span class="pill">Session <b id="clock">00:00</b></span>
    <button class="main" id="go">Démarrer</button>
  </div>
</div></nav>

<div class="wrap">
<section class="blk" id="live">
  <h2 class="t">Direct (Wi-Fi ESP32)</h2>
  <div id="alarm" role="alert"></div>
  <div id="recbar">
    <div><div class="sub" style="margin:0" id="recstate">Enregistrement du tour : inactif</div><div class="big" id="rectime">0.0 s</div></div>
    <div class="sub" style="margin:0">Appuie sur <kbd>L</kbd> pour démarrer, puis sur <kbd>L</kbd> pour arrêter et enregistrer le tour.</div>
    <button id="recbtn" disabled>Enregistrer un tour<kbd>L</kbd></button>
  </div>
  <div class="grid">
    <div class="card speed"><h3>Vitesse (capteur Hall)</h3><div class="val"><span id="spd">0.0</span><small>km/h</small></div>
      <div class="sub"><span id="rpm">0</span> tr/min roue</div><div class="meter"><i id="spdbar"></i></div></div>
    <div class="card kpi"><h3>Puissance</h3><div class="val"><span id="p">–</span><small>W</small></div><div class="sub"><span id="v">–</span> V · <span id="i">–</span> A</div></div>
    <div class="card kpi"><h3>Température pile</h3><div class="val"><span id="t">–</span><small>°C</small></div><div class="sub">Seuil alarme 55 °C</div></div>
    <div class="card kpi"><h3>Gâchette pilote</h3><div class="val"><span id="g">0</span><small>%</small></div><div class="meter"><i id="gbar" style="background:var(--amb)"></i></div></div>
    <div class="card kpi"><h3>Énergie cumulée</h3><div class="val"><span id="wh">0.00</span><small>Wh</small></div><div class="sub">H₂ estimé <b id="h2ml">0</b> mL</div></div>
    <div class="card third"><h3>Distance parcourue (session)</h3><div class="val"><span id="dist">0</span><small>m</small></div><div class="sub"><span id="distkm">0.000</span> km depuis le début de la session</div></div>
    <div class="card third"><h3>Distance du tour en cours</h3><div class="val"><span id="ldist">0</span><small>m</small></div><div class="sub" id="ldsub">Aucun tour en cours d’enregistrement</div></div>
    <div class="card third"><h3>Vitesse moyenne de la session</h3><div class="val"><span id="vavg">0.0</span><small>km/h</small></div><div class="sub">Distance ÷ temps écoulé</div></div>
  </div>
</section>

<section class="blk" id="graphes">
  <h2 class="t">Graphiques en direct</h2>
  <div class="grid">
    <div class="card half"><h3>Puissance et gâchette</h3><canvas id="c1"></canvas>
      <div class="legend"><span><i style="background:var(--h2)"></i>Puissance (W)</span><span><i style="background:var(--amb)"></i>Gâchette (%)</span></div></div>
    <div class="card half"><h3>Température pile et vitesse</h3><canvas id="c2"></canvas>
      <div class="legend"><span><i style="background:var(--bad)"></i>Température (°C)</span><span><i style="background:var(--ok)"></i>Vitesse (km/h)</span></div></div>
    <div class="card full"><h3>Hydrogène (Loi de Faraday : n(H₂) = ∫I·dt / 2F)</h3>
      <div class="sub" style="margin:0 0 6px">Reste estimé : <b id="h2left">100</b> %</div><div class="meter" style="margin:0"><i id="h2bar"></i></div></div>
  </div>
</section>

<section class="blk" id="tours">
  <h2 class="t">Tours enregistrés</h2>
  <div class="card wide"><table>
    <thead><tr><th>Tour</th><th>Temps</th><th>Distance</th><th>Énergie</th><th>V moy.</th><th>V max</th><th>P moy.</th><th>T max</th><th>H₂</th><th>Rendement</th><th></th></tr></thead>
    <tbody id="lapsb"></tbody></table>
    <div class="empty" id="noLap">Aucun tour enregistré. Démarre la session, puis appuie sur L au début et à la fin d’un tour.</div></div>
</section>

<section class="blk" id="comparaison">
  <h2 class="t">Comparaison des tours</h2>
  <div id="verdict" style="display:none"></div>
  <div class="grid">
    <div class="card full"><div style="display:flex;flex-wrap:wrap;gap:12px;align-items:center;justify-content:space-between"><div class="legend" id="leg3" style="margin:0"></div><label class="sub" style="margin:0">Tour de référence <select id="refsel"></select></label></div></div>
    <div class="card full wide"><h3>Comparaison détaillée</h3><table id="cmpt"></table></div>
    <div class="card full"><h3>Analyse automatique</h3><ul id="ana" style="margin:0;padding-left:18px"></ul></div>
    <div class="card half"><h3>Vitesse (km/h)</h3><canvas id="c3"></canvas><div class="legend cleg"></div></div>
    <div class="card half"><h3>Puissance (W)</h3><canvas id="c4"></canvas><div class="legend cleg"></div></div>
    <div class="card half"><h3>Énergie cumulée (Wh)</h3><canvas id="c5"></canvas><div class="legend cleg"></div></div>
    <div class="card half"><h3>Température de la pile (°C)</h3><canvas id="c6"></canvas><div class="legend cleg"></div></div>
    <div class="card full"><h3>Gâchette pilote (%)</h3><canvas id="c7"></canvas><div class="legend cleg"></div></div>
  </div>
</section>
</div>
<div id="toast"></div>

<script>
const $=id=>document.getElementById(id);
const KEY="l",N_CELLS=8,F=96485,CAP_ML=10000,TMAX=55,WHEEL_M=0.065,COL=["#0a8fb0","#c77a00","#1b8a4b","#c62f3b","#7a4fd0","#d0509a"];
let run=false,t0=0,timer,S,hist=[],rec=null,laps=[],lapN=0,realData=null;
function reset(){S={e:0,q:0,h2:0,dist:0,pk:0,lost:0,speed:0,temp:26,gs:0};hist=[]}reset();
const fmt=s=>String(Math.floor(s/60)).padStart(2,"0")+":"+String(Math.floor(s%60)).padStart(2,"0");
function toast(m){const e=$("toast");e.textContent=m;e.style.opacity=1;clearTimeout(toast.t);toast.t=setTimeout(()=>e.style.opacity=0,2200)}

async function fetchTelemetry(){
  try{
    const res=await fetch("/data",{cache:"no-store"});
    if(res.ok){
      realData=await res.json();
      $("ldot").className="dot";
    }
  }catch(e){$("ldot").className="dot off";}
}
setInterval(fetchTelemetry, 200);

function sample(t){
  if(realData){
    const g=realData.g||0, spd=realData.spd||0, V=realData.V||0, I=realData.I||0, P=realData.P||(V*I), T=realData.T||25, rpm=realData.rpm||0;
    return{g,us:1000+g*10,spd,I,V,P,T,rpm};
  }
  const tg=Math.max(0,Math.min(1,.55+.4*Math.sin(t/3.2)+.12*Math.sin(t*1.7)));
  S.gs+=(tg-S.gs)*.35;const g=S.gs,spd=(S.speed+=((g*17+1)-S.speed)*.12);
  const I=Math.max(.2,g*g*9+g*1.4+Math.random()*.25),V=8.2-.11*I-Math.random()*.05;
  S.temp+=(26+I*3.4-S.temp)*.012;
  return{g:g*100,us:1000+g*1000,spd,I,V,P:I*V,T:S.temp,rpm:spd/3.6/(Math.PI*WHEEL_M)*60};
}

function step(){
  const dt=.2,t=(Date.now()-t0)/1000;S.pk++;
  const d=sample(t);S.e+=d.P*dt/3600;S.q+=d.I*dt;S.h2=N_CELLS*S.q/(2*F)*24500;S.dist+=d.spd/3.6*dt;
  hist.push(d);if(hist.length>300)hist.shift();
  if(rec){rec.s.push({t:rec.s.length*dt,spd:d.spd,P:d.P,T:d.T,I:d.I,g:d.g});rec.e+=d.P*dt/3600;rec.q+=d.I*dt;rec.d+=d.spd/3.6*dt;
    $("rectime").textContent=(rec.s.length*dt).toFixed(1)+" s"}
  $("spd").textContent=d.spd.toFixed(1);$("rpm").textContent=Math.round(d.rpm);$("dist").textContent=Math.round(S.dist);$("distkm").textContent=(S.dist/1000).toFixed(3);$("ldist").textContent=rec?Math.round(rec.d):0;$("ldsub").textContent=rec?"Enregistrement en cours":"Aucun tour en cours d’enregistrement";$("vavg").textContent=(S.dist/Math.max(t,.2)*3.6).toFixed(1);$("spdbar").style.width=Math.min(100,d.spd/20*100)+"%";
  $("v").textContent=d.V.toFixed(2);$("i").textContent=d.I.toFixed(2);$("p").textContent=Math.round(d.P);$("t").textContent=d.T.toFixed(1);
  $("g").textContent=Math.round(d.g);$("gbar").style.width=d.g+"%";$("wh").textContent=S.e.toFixed(2);$("h2ml").textContent=Math.round(S.h2);
  $("h2bar").style.width=Math.min(100,S.h2/CAP_ML*100)+"%";$("h2left").textContent=Math.max(0,100-S.h2/CAP_ML*100).toFixed(1);
  $("clock").textContent=fmt(t);
  const al=$("alarm");if(d.T>TMAX){al.style.display="block";al.textContent="Alarme : température de la pile à "+d.T.toFixed(1)+" °C, seuil de "+TMAX+" °C dépassé."}else al.style.display="none";
  drawLive()
}

function toggleRec(){
  if(!run){toast("Démarre d’abord la session");return}
  if(!rec){rec={s:[],e:0,q:0,d:0};$("recbar").classList.add("on");$("recbtn").classList.add("rec");$("recstate").textContent="Enregistrement du tour : EN COURS";$("recbtn").firstChild.textContent="Terminer le tour"}
  else{endRec()}
}
function endRec(){
  const r=rec;rec=null;$("recbar").classList.remove("on");$("recbtn").classList.remove("rec");$("recstate").textContent="Enregistrement du tour : inactif";$("recbtn").firstChild.textContent="Enregistrer un tour";$("rectime").textContent="0.0 s";
  if(r.s.length<5){toast("Tour trop court, ignoré");return}
  const n=r.s.length,time=n*.2;
  laps.push({n:++lapN,s:r.s,time,dist:r.d,wh:r.e,h2:N_CELLS*r.q/(2*F)*24500,vavg:r.d/time*3.6,vmax:Math.max(...r.s.map(x=>x.spd)),pavg:r.s.reduce((a,x)=>a+x.P,0)/n,tmax:Math.max(...r.s.map(x=>x.T)),on:true});
  laps[laps.length-1].eff=laps[laps.length-1].dist/Math.max(.001,laps[laps.length-1].wh);
  toast("Tour "+lapN+" enregistré");renderLaps()
}
function renderLaps(){
  $("noLap").style.display=laps.length?"none":"block";
  const best=(k,min)=>laps.length?laps.reduce((a,b)=>(min?b[k]<a[k]:b[k]>a[k])?b:a).n:0;
  const bt=best("time",1),be=best("wh",1),br=best("eff",0);
  $("lapsb").innerHTML=laps.map(l=>"<tr><td><input type=checkbox "+(l.on?"checked":"")+" data-n="+l.n+"> <i class=sw style='background:"+COL[(l.n-1)%6]+"'></i>Tour "+l.n+"</td>"+
   "<td class='"+(l.n==bt?"best":"")+"'>"+l.time.toFixed(1)+" s</td><td>"+Math.round(l.dist)+" m</td><td class='"+(l.n==be?"best":"")+"'>"+l.wh.toFixed(2)+" Wh</td><td>"+l.vavg.toFixed(1)+"</td><td>"+l.vmax.toFixed(1)+"</td><td>"+Math.round(l.pavg)+" W</td><td>"+l.tmax.toFixed(1)+" °C</td><td>"+Math.round(l.h2)+" mL</td><td class='"+(l.n==br?"best":"")+"'>"+l.eff.toFixed(1)+" m/Wh</td><td><button class=x data-del="+l.n+">Supprimer</button></td></tr>").join("");
  const v=$("verdict");
  if(laps.length>=2){
    const rank=l=>[...laps].sort((a,b)=>a.time-b.time).indexOf(l)+[...laps].sort((a,b)=>a.wh-b.wh).indexOf(l)+[...laps].sort((a,b)=>b.eff-a.eff).indexOf(l);
    const w=laps.reduce((a,b)=>rank(b)<rank(a)?b:a);
    v.style.display="block";v.innerHTML="<b>Meilleur compromis : tour "+w.n+"</b> ("+w.time.toFixed(1)+" s, "+w.wh.toFixed(2)+" Wh, "+w.eff.toFixed(1)+" m/Wh)."
  }else{v.style.display=laps.length?"block":"none";v.textContent=laps.length?"Enregistre un deuxième tour pour lancer la comparaison.":""}
  drawCmp()
}
$("lapsb").addEventListener("change",e=>{const l=laps.find(x=>x.n==e.target.dataset.n);if(l){l.on=e.target.checked;drawCmp()}});
$("lapsb").addEventListener("click",e=>{const n=e.target.dataset.del;if(n){laps=laps.filter(x=>x.n!=n);renderLaps()}});
function setup(id){const c=$(id),r=devicePixelRatio||1,w=c.clientWidth,h=c.clientHeight;c.width=w*r;c.height=h*r;const x=c.getContext("2d");x.scale(r,r);
  const cs=getComputedStyle(document.documentElement);x.strokeStyle=cs.getPropertyValue("--line");x.lineWidth=1;for(let k=1;k<4;k++){x.beginPath();x.moveTo(0,h*k/4);x.lineTo(w,h*k/4);x.stroke()}return{x,w,h,cs}}
function path(x,pts,max,xmax,col,w,h){x.beginPath();pts.forEach((p,i)=>{const px=p[0]/xmax*w,py=h-4-(p[1]/max)*(h-12);i?x.lineTo(px,py):x.moveTo(px,py)});x.strokeStyle=col;x.lineWidth=2;x.stroke()}
function pane(id,a){const{x,w,h,cs}=setup(id);a.forEach(s=>path(x,hist.map((d,i)=>[i,d[s[0]]]),s[1],299,cs.getPropertyValue(s[2]).trim(),w,h))}
function drawLive(){pane("c1",[["P",80,"--h2"],["g",100,"--amb"]]);pane("c2",[["T",70,"--bad"],["spd",20,"--ok"]])}
let refN=null;
const avg=(l,k)=>l.s.reduce((a,p)=>a+p[k],0)/l.s.length;
const MET=[
 ["Temps","s",1,l=>l.time,-1],["Distance","m",0,l=>l.dist,0],["Énergie","Wh",2,l=>l.wh,-1],
 ["Énergie par 100 m","Wh",2,l=>l.wh/l.dist*100,-1],["Rendement","m/Wh",1,l=>l.eff,1],["H₂ consommé","mL",0,l=>l.h2,-1],
 ["Vitesse moyenne","km/h",1,l=>l.vavg,1],["Vitesse max","km/h",1,l=>l.vmax,1],
 ["Puissance moyenne","W",0,l=>l.pavg,0],["Puissance max","W",0,l=>Math.max(...l.s.map(p=>p.P)),0],["Courant moyen","A",1,l=>avg(l,"I"),0],
 ["Gâchette moyenne","%",0,l=>avg(l,"g"),0],["Temps plein gaz (>90 %)","%",0,l=>100*l.s.filter(p=>p.g>90).length/l.s.length,0],
 ["Température max","°C",1,l=>l.tmax,-1],["Échauffement","°C",1,l=>l.s[l.s.length-1].T-l.s[0].T,-1]
];
function drawCmp(){
  const sel=laps.filter(l=>l.on),c=l=>COL[(l.n-1)%6];
  if(!sel.length){$("cmpt").innerHTML="";$("ana").innerHTML="<li>Aucun tour sélectionné.</li>";$("refsel").innerHTML="";$("leg3").textContent="Aucun tour sélectionné";document.querySelectorAll(".cleg").forEach(e=>e.textContent="Aucun tour sélectionné");["c3","c4","c5","c6","c7"].forEach(id=>setup(id));return}
  const xm=Math.max(5,...sel.map(l=>l.time)),ref=sel.find(l=>l.n==refN)||sel.reduce((a,b)=>b.time<a.time?b:a,sel[0]);
  $("leg3").innerHTML=sel.map(l=>"<span><i style='background:"+c(l)+"'></i>Tour "+l.n+"</span>").join("");
  document.querySelectorAll(".cleg").forEach(e=>e.innerHTML=$("leg3").innerHTML);
  $("refsel").innerHTML=sel.map(l=>"<option value="+l.n+(l===ref?" selected":"")+">Tour "+l.n+"</option>").join("");
  $("refsel").onchange=e=>{refN=+e.target.value;drawCmp()};
  const cum=l=>{let a=0;return l.s.map(p=>[p.t,a+=p.P*.2/3600])};
  [["c3",l=>l.s.map(p=>[p.t,p.spd]),20],["c4",l=>l.s.map(p=>[p.t,p.P]),80],["c5",cum,Math.max(.01,...sel.map(l=>l.wh))*1.05],["c6",l=>l.s.map(p=>[p.t,p.T]),70],["c7",l=>l.s.map(p=>[p.t,p.g]),100]]
   .forEach(a=>{const{x,w,h}=setup(a[0]);sel.forEach(l=>path(x,a[1](l),a[2],xm,c(l),w,h))});
  const vals=MET.map(m=>sel.map(l=>m[3](l))),ri=sel.indexOf(ref);
  $("cmpt").innerHTML="<thead><tr><th>Critère</th>"+sel.map(l=>"<th><i class=sw style='background:"+c(l)+"'></i>Tour "+l.n+(l===ref?" (réf.)":"")+"</th>").join("")+"</tr></thead><tbody>"+
   MET.map((m,r)=>{const best=m[4]?(m[4]<0?Math.min(...vals[r]):Math.max(...vals[r])):null,rv=vals[r][ri];
    return "<tr><td>"+m[0]+" ("+m[1]+")</td>"+vals[r].map((v,i)=>{const d=v-rv,col=m[4]&&d*m[4]>0?"var(--ok)":m[4]&&d*m[4]<0?"var(--bad)":"var(--mute)";
     return "<td class='"+(best!==null&&v===best&&sel.length>1?"best":"")+"'>"+v.toFixed(m[2])+(i===ri?"":"<small style='color:"+col+"'>"+(d>0?"+":"")+d.toFixed(m[2])+"</small>")+"</td>"}).join("")+"</tr>"}).join("")+"</tbody>";
  $("ana").innerHTML=sel.length<2?"<li>Sélectionne au moins deux tours pour obtenir une analyse.</li>":sel.filter(l=>l!==ref).map(l=>{
   const dt=l.time-ref.time,e0=ref.wh/ref.dist,de=(l.wh/l.dist-e0)/e0*100,dg=avg(l,"g")-avg(ref,"g"),dT=l.tmax-ref.tmax;
   const verdict=dt<0&&de<0?"meilleur sur le temps et sur l’énergie":dt<0?"plus rapide, mais le gain de temps est payé en énergie":de<0?"plus économe, mais plus lent":"moins bon sur le temps et sur l’énergie";
   return "<li><b>Tour "+l.n+" face au tour "+ref.n+"</b> : "+verdict+". "+(dt<0?"Plus rapide de ":"Plus lent de ")+Math.abs(dt).toFixed(1)+" s, "+(de>0?"consomme ":"économise ")+Math.abs(de).toFixed(1)+" % d’énergie par mètre.</li>"}).join("")
}
$("go").onclick=()=>{
  if(!run){reset();t0=Date.now();run=true;timer=setInterval(step,200);$("go").textContent="Arrêter";$("ldot").classList.remove("off");$("recbtn").disabled=false}
  else{if(rec)endRec();run=false;clearInterval(timer);$("go").textContent="Démarrer";$("ldot").classList.add("off");$("recbtn").disabled=true}
};
$("recbtn").onclick=toggleRec;
addEventListener("keydown",e=>{if(e.key.toLowerCase()===KEY&&!e.repeat&&!e.ctrlKey&&!e.metaKey&&!/INPUT|TEXTAREA/.test(e.target.tagName)){e.preventDefault();toggleRec()}});
addEventListener("resize",()=>{drawLive();drawCmp()});
drawLive();renderLaps();
</script>
</body>
</html>
)rawliteral";

#endif // H2_VISION_WEB_DASHBOARD_H
