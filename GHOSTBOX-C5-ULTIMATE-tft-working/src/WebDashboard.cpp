#include "WebDashboard.h"

#include <Arduino.h>
#include <ESP.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <pgmspace.h>

#include "CC1101Tools.h"
#include "PepeDraw.h"
#include "Pins.h"
#include "SignalTools.h"
#include "SoundUtils.h"

static constexpr const char* DASH_AP_SSID = "GHOSTBOX-AP";
static constexpr const char* DASH_AP_PASS = "admin1234";
static constexpr uint8_t DASH_AP_CHANNEL = 6;
static constexpr uint8_t DASH_AP_MAX_CLIENTS = 4;

static WebServer server(80);
static uint32_t dashboardStartedMs = 0;
static bool routesConfigured = false;

static constexpr uint16_t WEB_BEACON_INTERVAL_MS = 22;
static constexpr uint16_t WEB_BEACON_MAX_SECONDS = 30;

static bool webBeaconActive = false;
static uint32_t webBeaconStartedMs = 0;
static uint32_t webBeaconEndMs = 0;
static uint32_t webBeaconLastSendMs = 0;
static uint32_t webBeaconSent = 0;
static uint8_t webBeaconIndex = 0;
static String webBeaconCurrentSsid = "";

static const char* WEB_BEACON_SSIDS[] = {
    "💀GhostBox_Active_Node", "⚡GhostBox_Mesh_01", "👾Ghostbox_Grid",
    "📡Tachyon_Uplink", "🔥Skynet_Defense_Net", "👽Area51_Alien_Lab",
    "💣ZeroDay_Lab_WiFi", "🤖BladeRunner_Net", "🧠Neuralink_Mesh",
    "🔞DARPA_Classified", "⚠️Kernel_Panic_Alert", "🐍Hydra_SubNet",
    "💥DarkNet_Relay_01", "🔍Echelon_Surveillance", "🔥Matrix_Glitch_Detected",
    "💾Quantum_Core_Down", "🚨NSA_Listening_Post", "🖕HackThePlanet_99",
    "🤡Mr_Robot_fsociety", "🧼Null_Pointer_Exception", "🕵Interpol_Cyber_Unit",
    "👮Starfleet_Command", "🤮Rootkit_Active", "🍄Crypto_Miner_Locker",
    "🧨Trojan_Injector", "🌚Black_Hole_Horizon", "🥀ZeroTrust_Isolated",
    "🍗Free_Public_Proxy", "🍖GhostBox_AirGap", "🦶Satellite_Downlink_8",
    "👅Cisco_Backdoor_Lab", "🧟Zombie_Botnet_Relay", "👺DeadMans_Switch",
    "🍕Cyber_Vault_Unlocked", "🌑Dark_Matter_Flux", "💊Red_Or_Blue_Pill",
    "📉Buffer_Overflow_X", "📡Overwatch_Drone_04", "⚡Tesla_Wardenclyffe",
    "💀Defcon_Badge_WiFi"
};
static constexpr uint8_t WEB_BEACON_COUNT =
    sizeof(WEB_BEACON_SSIDS) / sizeof(WEB_BEACON_SSIDS[0]);

static uint8_t webBeaconFrame[128] = {
    0x80, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x02, 0x11, 0x22, 0x33, 0x44, 0x55,
    0x02, 0x11, 0x22, 0x33, 0x44, 0x55,
    0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x64, 0x00,
    0x01, 0x04,
    0x00, 0x00
};
static constexpr uint8_t WEB_BEACON_SSID_LEN_OFFSET = 37;
static constexpr uint8_t WEB_BEACON_SSID_OFFSET = 38;
static const uint8_t WEB_BEACON_TAIL[] = {
    0x01, 0x08,
    0x82, 0x84, 0x8B, 0x96, 0x24, 0x30, 0x48, 0x6C,
    0x03, 0x01, DASH_AP_CHANNEL
};

static const char WEB_DASHBOARD_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>ESP32-TOOLS-PRO Dashboard</title>
  <style>
    :root{color-scheme:dark;--bg:#050505;--panel:#101214;--line:#2a2f35;--text:#f4f7fb;--muted:#9da8b5;--cyan:#38d5ff;--green:#46e27b;--yellow:#f4d35e;--red:#ff5d5d}
    *{box-sizing:border-box}
    body{margin:0;background:var(--bg);color:var(--text);font-family:Inter,system-ui,-apple-system,Segoe UI,sans-serif;line-height:1.35}
    header{position:sticky;top:0;background:#050505e8;border-bottom:1px solid var(--line);backdrop-filter:blur(10px);z-index:2}
    .wrap{width:min(1120px,calc(100% - 24px));margin:0 auto}
    .top{display:flex;align-items:center;justify-content:space-between;gap:12px;padding:14px 0}
    h1{font-size:18px;margin:0;letter-spacing:0}
    .pill{border:1px solid var(--line);padding:6px 9px;color:var(--muted);font-size:12px}
    nav{display:flex;gap:8px;overflow:auto;padding:0 0 12px}
    nav button{white-space:nowrap}
    main{padding:16px 0 28px}
    .grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:10px}
    .two{display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1fr);gap:10px}
    section,.card{border:1px solid var(--line);background:var(--panel);padding:14px}
    section{margin-top:10px}
    h2{font-size:16px;margin:0 0 12px}
    h3{font-size:13px;margin:0 0 8px;color:var(--muted);font-weight:600}
    .value{font-size:22px;font-weight:800}
    .muted{color:var(--muted)}
    button,select,input{background:#050505;color:var(--text);border:1px solid var(--line);padding:9px 10px;font:inherit}
    button{cursor:pointer}
    button:hover{border-color:var(--cyan)}
    button.good{border-color:#1f7d3f;color:var(--green)}
    button.warn{border-color:#7d6820;color:var(--yellow)}
    button.bad{border-color:#7d2020;color:var(--red)}
    .row{display:flex;align-items:center;justify-content:space-between;gap:10px;border-top:1px solid var(--line);padding:10px 0}
    .row:first-child{border-top:0;padding-top:0}
    .row:last-child{padding-bottom:0}
    .actions{display:flex;gap:8px;flex-wrap:wrap;justify-content:flex-end}
    .tabs{display:none}
    .tabs.active{display:block}
    table{width:100%;border-collapse:collapse}
    th,td{text-align:left;border-bottom:1px solid var(--line);padding:8px;font-size:13px}
    th{color:var(--muted);font-weight:600}
    .bar{height:10px;background:#050505;border:1px solid var(--line);overflow:hidden}
    .bar span{display:block;height:100%;background:var(--cyan);width:0}
    .ok{color:var(--green)}.badtext{color:var(--red)}.yellow{color:var(--yellow)}.cyan{color:var(--cyan)}
    .stack{display:grid;gap:10px}
    .subgrid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:10px}
    .toolcard{border:1px solid var(--line);background:#08090a;padding:12px;display:grid;gap:8px}
    .toolcard h3{color:var(--text);margin:0}
    .toolcard p{margin:0;color:var(--muted);font-size:13px}
    .tag{display:inline-block;border:1px solid var(--line);color:var(--muted);font-size:11px;padding:3px 6px;width:max-content}
    .metric{display:grid;gap:6px}
    .big{font-size:34px;font-weight:900}
    @media(max-width:820px){.grid,.two,.subgrid{grid-template-columns:1fr}.top{align-items:flex-start;flex-direction:column}.actions{justify-content:flex-start}}
  </style>
</head>
<body>
  <header>
    <div class="wrap">
      <div class="top">
        <h1>GHOSTBOX Web Dashboard</h1>
        <div class="pill">AP: GHOSTBOX-AP / admin1234 / 192.168.4.1</div>
      </div>
      <nav>
        <button data-tab="home">Dashboard</button>
        <button data-tab="ir">IR Captures</button>
        <button data-tab="cc">CC1101 Monitor</button>
        <button data-tab="wifi">WiFi Tools</button>
      </nav>
    </div>
  </header>
  <main class="wrap">
    <div id="home" class="tabs active">
      <div class="grid">
        <div class="card"><h3>Uptime</h3><div class="value" id="uptime">--</div></div>
        <div class="card"><h3>Free heap</h3><div class="value" id="heap">--</div></div>
        <div class="card"><h3>Clients</h3><div class="value" id="clients">--</div></div>
        <div class="card"><h3>IR saved</h3><div class="value" id="irSaved">--</div></div>
      </div>
      <section>
        <h2>Hardware diag</h2>
        <div class="two">
          <div class="stack" id="pinInfo"></div>
          <div class="stack" id="liveInfo"></div>
        </div>
      </section>
    </div>

    <div id="ir" class="tabs">
      <section>
        <h2>IR saved captures</h2>
        <p class="muted">Replay, rename, or delete captures saved from Signal Tools.</p>
        <div id="irList"></div>
      </section>
    </div>

    <div id="cc" class="tabs">
      <section>
        <h2>CC1101 frequency monitor</h2>
        <div class="actions" style="justify-content:flex-start;margin-bottom:12px">
          <select id="ccFreq">
            <option value="315000">315.00 MHz</option>
            <option value="390000">390.00 MHz</option>
            <option value="433920" selected>433.92 MHz</option>
            <option value="868350">868.35 MHz</option>
            <option value="915000">915.00 MHz</option>
          </select>
          <button onclick="readCc()">Read now</button>
        </div>
        <div class="grid">
          <div class="card"><h3>Status</h3><div class="value" id="ccReady">--</div></div>
          <div class="card"><h3>RSSI</h3><div class="value" id="ccRssi">--</div></div>
          <div class="card"><h3>GDO0</h3><div class="value" id="ccGdo">--</div></div>
          <div class="card"><h3>MARC</h3><div class="value" id="ccMarc">--</div></div>
        </div>
        <section>
          <h2>Signal</h2>
          <div class="bar"><span id="ccBar"></span></div>
          <p class="muted" id="ccMeta" style="margin-top:10px">Waiting...</p>
        </section>
      </section>
    </div>

    <div id="wifi" class="tabs">
      <section>
        <h2>WiFi tools</h2>
        <p class="muted">Passive options function via web. Functions requiring dedicated WiFi radio control are accessible locally to maintain dashboard uptime.</p>
        <div class="subgrid" id="wifiTools"></div>
      </section>
      <section>
        <h2 id="wifiTitle">WiFi Scanner</h2>
        <div class="actions" style="justify-content:flex-start;margin-bottom:12px">
          <button onclick="scanWifi()">Scan now</button>
          <span class="muted" id="wifiState"></span>
        </div>
        <div id="wifiOutput"></div>
      </section>
      <section>
        <h2>Selected target</h2>
        <div id="wifiTarget"><p class="muted">Select a network from Scanner, Radar or Direction Finder.</p></div>
      </section>
    </div>
  </main>
  <script>
    const $ = id => document.getElementById(id);
    let activeTab = 'home';
    let irItems = [];
    let wifiMode = 'scanner';
    let wifiLast = null;
    let wifiTarget = null;
    let wifiRadarTimer = null;
    let dirReadings = {};
    let beaconActive = false;
    let beaconSent = 0;
    let beaconLeft = 0;
    let beaconSsid = '';
    document.querySelectorAll('nav button').forEach(b => b.onclick = () => {
      activeTab = b.dataset.tab;
      document.querySelectorAll('.tabs').forEach(t => t.classList.remove('active'));
      $(activeTab).classList.add('active');
      if (activeTab === 'ir') loadIr();
      if (activeTab === 'cc') readCc();
      if (activeTab === 'wifi') renderWifiTools();
    });

    async function api(path, opts){
      const res = await fetch(path, opts || {});
      if(!res.ok) throw new Error(await res.text());
      return res.json();
    }
    function esc(s){return String(s ?? '').replace(/[&<>"']/g,m=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[m]));}
    function fmtMs(ms){
      const s=Math.floor(ms/1000), h=Math.floor(s/3600), m=Math.floor((s%3600)/60);
      return h+'h '+m+'m '+(s%60)+'s';
    }
    function rssiPct(dbm){return Math.max(0,Math.min(100,Math.round((dbm+120)*1.35)));}
    function rssiColorClass(dbm){return dbm>-63?'ok':(dbm>-78?'yellow':'badtext');}

    async function loadStatus(){
      try{
        const d=await api('/api/status');
        $('uptime').textContent=fmtMs(d.uptimeMs);
        $('heap').textContent=Math.round(d.heap/1024)+' KB';
        $('clients').textContent=d.clients;
        $('irSaved').textContent=d.irSaved+'/'+d.irMax;
        beaconActive = d.beacon.active;
        beaconSent = d.beacon.sent;
        beaconLeft = d.beacon.leftMs;
        beaconSsid = d.beacon.current;
        $('pinInfo').innerHTML=
          '<div class="row"><span>IR TX</span><b>GPIO '+d.pins.irTx+'</b></div>'+
          '<div class="row"><span>IR RX</span><b>GPIO '+d.pins.irRx+'</b></div>'+
          '<div class="row"><span>CC1101 CSN</span><b>GPIO '+d.pins.ccCsn+'</b></div>'+
          '<div class="row"><span>CC1101 GDO0</span><b>GPIO '+d.pins.ccGdo0+'</b></div>';
        $('liveInfo').innerHTML=
          '<div class="row"><span>AP IP</span><b>'+esc(d.ip)+'</b></div>'+
          '<div class="row"><span>AP MAC</span><b>'+esc(d.mac)+'</b></div>'+
          '<div class="row"><span>IR RX level</span><b>'+d.irLevel+'</b></div>'+
          '<div class="row"><span>GDO0 level</span><b>'+d.gdo0Level+'</b></div>';
        if(activeTab==='wifi') renderWifiTools();
      }catch(e){}
    }

    async function loadIr(){
      const d=await api('/api/ir/list');
      irItems = d.items;
      if(!d.items.length){$('irList').innerHTML='<p class="muted">No saved IR captures yet.</p>';return;}
      $('irList').innerHTML=d.items.map(x=>
        '<div class="row"><div><b>'+esc(x.name)+'</b><div class="muted">Slot '+(x.slot+1)+' / '+x.count+' timings</div></div>'+
        '<div class="actions">'+
        '<button class="good" onclick="replayIr('+x.slot+')">Replay</button>'+
        '<button class="warn" onclick="renameIr('+x.slot+')">Rename</button>'+
        '<button class="bad" onclick="deleteIr('+x.slot+')">Delete</button>'+
        '</div></div>').join('');
    }
    async function replayIr(slot){await api('/api/ir/replay?slot='+slot,{method:'POST'});}
    async function deleteIr(slot){if(confirm('Delete this IR capture?')){await api('/api/ir/delete?slot='+slot,{method:'POST'});loadIr();loadStatus();}}
    async function renameIr(slot){
      const item = irItems.find(x => x.slot === slot);
      const current = item ? item.name : '';
      const name=prompt('New capture name',current||'');
      if(!name) return;
      await api('/api/ir/rename?slot='+slot+'&name='+encodeURIComponent(name),{method:'POST'});
      loadIr();loadStatus();
    }

    async function readCc(){
      try{
        const f=$('ccFreq').value;
        const d=await api('/api/cc1101?freq='+f);
        $('ccReady').textContent=d.ready?'READY':'NO';
        $('ccReady').className=d.ready?'value ok':'value badtext';
        $('ccRssi').textContent=d.rssiDbm+' dBm';
        $('ccGdo').textContent=d.gdo0;
        $('ccMarc').textContent=d.marc;
        $('ccBar').style.width=rssiPct(d.rssiDbm)+'%';
        $('ccMeta').textContent='Freq '+d.freqMHz+' MHz / PART 0x'+d.partnum+' / VERSION 0x'+d.version+' / PKT 0x'+d.pktStatus+' / LQI '+d.lqi;
      }catch(e){$('ccMeta').textContent='CC1101 read failed';}
    }

    function setWifiMode(mode){
      wifiMode=mode;
      dirReadings={};
      if(wifiRadarTimer){clearInterval(wifiRadarTimer);wifiRadarTimer=null;}
      const titles={scanner:'WiFi Scanner',channels:'Channel Scan',radar:'WiFi Radar',direction:'Direction Finder'};
      $('wifiTitle').textContent=titles[mode]||'WiFi Tools';
      $('wifiOutput').innerHTML='';
      $('wifiState').textContent='';
      renderWifiTools();
      if(mode==='radar'||mode==='direction') renderWifiTarget();
    }

    function renderWifiTools(){
      const items=[
        ['scanner','WiFi Scanner','Lists APs, RSSI, channel, security, and BSSID.','WEB'],
        ['channels','Channel Scan','Channel overview and AP distribution per channel.','WEB'],
        ['radar','WiFi Radar','Track AP proximity and signal RSSI in real time.','WEB'],
        ['direction','Direction Finder','Measure front/right/back/left to locate signal source.','WEB'],
        ['beacon','Beacon Spam','Web-controlled beacon spam demo with safety timer.','WEB LAB'],
        ['local','Deauther','Available directly from the hardware menu.','LOCAL'],
        ['local','Evil Portal','Available directly from the hardware menu.','LOCAL'],
        ['local','Probe Sniffer','Available directly from the hardware menu.','LOCAL'],
        ['local','KARMA Attack','Available directly from the hardware menu.','LOCAL']
      ];
      $('wifiTools').innerHTML=items.map(x=>{
        const active=x[0]===wifiMode;
        let btn='';
        let extra='';
        if(x[0]==='local'){
          btn='<span class="tag">LOCAL ONLY</span>';
        }else if(x[0]==='beacon'){
          btn=beaconActive
            ? '<button class="bad" onclick="stopBeacon()">Stop</button>'
            : '<button class="warn" onclick="startBeacon()">Start 120s</button>';
          extra='<p class="'+(beaconActive?'yellow':'muted')+'">'+
                (beaconActive
                  ? 'Running: '+beaconSent+' beacons / '+Math.ceil(beaconLeft/1000)+'s left / '+esc(beaconSsid)
                  : 'Stopped. Same channel as dashboard.')+
                '</p>';
        }else{
          btn='<button '+(active?'class="good"':'')+' onclick="setWifiMode(`'+x[0]+'`)">'+(active?'Active':'Open')+'</button>';
        }
        return '<div class="toolcard"><h3>'+x[1]+'</h3><p>'+x[2]+'</p>'+extra+'<div class="actions" style="justify-content:flex-start">'+btn+'<span class="tag">'+x[3]+'</span></div></div>';
      }).join('');
    }

    async function startBeacon(){
      await api('/api/wifi/beacon/start?seconds=25',{method:'POST'});
      await loadStatus();
    }

    async function stopBeacon(){
      await api('/api/wifi/beacon/stop',{method:'POST'});
      await loadStatus();
    }

    function renderWifiTarget(extra){
      if(!wifiTarget){
        $('wifiTarget').innerHTML='<p class="muted">No target selected yet.</p>';
        return;
      }
      const pct=rssiPct(wifiTarget.rssi);
      $('wifiTarget').innerHTML=
        '<div class="two"><div class="card"><h3>'+esc(wifiTarget.ssid)+'</h3>'+
        '<div class="muted">'+esc(wifiTarget.bssid)+' / CH '+wifiTarget.ch+' / '+esc(wifiTarget.auth)+'</div></div>'+
        '<div class="card metric"><h3>RSSI</h3><div class="big '+rssiColorClass(wifiTarget.rssi)+'">'+wifiTarget.rssi+' dBm</div><div class="bar"><span style="width:'+pct+'%"></span></div></div></div>'+
        (extra||'');
    }

    function selectWifiTarget(i, mode){
      if(!wifiLast||!wifiLast.networks[i])return;
      wifiTarget=wifiLast.networks[i];
      if(mode)setWifiMode(mode);
      renderWifiTarget();
      if(wifiMode==='radar') startRadar();
      if(wifiMode==='direction') renderDirectionPanel();
    }

    function renderScanner(){
      const nets=wifiLast.networks;
      $('wifiOutput').innerHTML='<table><thead><tr><th>SSID</th><th>CH</th><th>RSSI</th><th>Security</th><th>BSSID</th><th></th></tr></thead><tbody>'+
        nets.map((n,i)=>'<tr><td>'+esc(n.ssid)+'</td><td>'+n.ch+'</td><td class="'+rssiColorClass(n.rssi)+'">'+n.rssi+'</td><td>'+esc(n.auth)+'</td><td>'+esc(n.bssid)+'</td><td><button onclick="selectWifiTarget('+i+',`radar`)">Track</button></td></tr>').join('')+
        '</tbody></table>';
    }

    function renderChannels(){
      const d=wifiLast;
      $('wifiOutput').innerHTML='<h3>Channels</h3><table><thead><tr><th>CH</th><th>Nets</th><th>Open</th><th>Best RSSI</th><th>Load</th></tr></thead><tbody>'+
        d.channels.map(c=>'<tr><td>'+c.ch+'</td><td>'+c.count+'</td><td>'+c.open+'</td><td>'+c.best+'</td><td><div class="bar"><span style="width:'+Math.min(100,c.count*12)+'%"></span></div></td></tr>').join('')+
        '</tbody></table><h3 style="margin-top:16px">Networks</h3><table><thead><tr><th>SSID</th><th>CH</th><th>RSSI</th><th>Security</th><th>BSSID</th></tr></thead><tbody>'+
        d.networks.map(n=>'<tr><td>'+esc(n.ssid)+'</td><td>'+n.ch+'</td><td>'+n.rssi+'</td><td>'+esc(n.auth)+'</td><td>'+esc(n.bssid)+'</td></tr>').join('')+
        '</tbody></table>';
    }

    function renderRadarPicker(){
      const nets=wifiLast.networks;
      $('wifiOutput').innerHTML='<p class="muted">Choose one AP to track.</p><table><thead><tr><th>SSID</th><th>CH</th><th>RSSI</th><th>BSSID</th><th></th></tr></thead><tbody>'+
        nets.map((n,i)=>'<tr><td>'+esc(n.ssid)+'</td><td>'+n.ch+'</td><td>'+n.rssi+'</td><td>'+esc(n.bssid)+'</td><td><button onclick="selectWifiTarget('+i+',wifiMode)">Select</button></td></tr>').join('')+
        '</tbody></table>';
    }

    async function startRadar(){
      if(!wifiTarget)return;
      if(wifiRadarTimer)clearInterval(wifiRadarTimer);
      await updateRadar();
      wifiRadarTimer=setInterval(()=>{if(activeTab==='wifi'&&wifiMode==='radar')updateRadar();},3500);
    }

    async function updateRadar(){
      if(!wifiTarget)return;
      try{
        const d=await api('/api/wifi/target?bssid='+encodeURIComponent(wifiTarget.bssid));
        if(d.found) wifiTarget=d;
        const pct=rssiPct(wifiTarget.rssi);
        const dist=Math.max(0.4,Math.pow(10,(-45-wifiTarget.rssi)/28)).toFixed(1);
        renderWifiTarget('<div class="card" style="margin-top:10px"><h3>Radar</h3><div class="big">'+pct+'%</div><div class="bar"><span style="width:'+pct+'%"></span></div><p class="muted">Estimated distance ~'+dist+' m. Tap Scan now to choose another AP.</p></div>');
      }catch(e){}
    }

    function renderDirectionPanel(){
      const buttons=['front','right','back','left'].map(x=>'<button onclick="measureDirection(`'+x+'`)">'+x.toUpperCase()+'</button>').join('');
      renderWifiTarget('<div class="card" style="margin-top:10px"><h3>Direction Finder</h3><p class="muted">Point the device to each side and tap the matching button.</p><div class="actions" style="justify-content:flex-start">'+buttons+'</div><div id="dirResult" style="margin-top:10px"></div></div>');
      drawDirectionResult();
    }

    async function measureDirection(side){
      if(!wifiTarget)return;
      const d=await api('/api/wifi/target?bssid='+encodeURIComponent(wifiTarget.bssid));
      if(d.found){wifiTarget=d;dirReadings[side]=d.rssi;}
      renderDirectionPanel();
    }

    function drawDirectionResult(){
      const el=$('dirResult'); if(!el)return;
      const sides=['front','right','back','left'];
      let best=null;
      sides.forEach(s=>{if(dirReadings[s]!==undefined&&(best===null||dirReadings[s]>dirReadings[best]))best=s;});
      el.innerHTML=sides.map(s=>{
        const v=dirReadings[s];
        return '<div class="row"><span>'+s.toUpperCase()+'</span><b>'+(v===undefined?'--':v+' dBm')+'</b></div>';
      }).join('')+(best?'<p class="cyan">Strongest direction: '+best.toUpperCase()+'</p>':'');
    }

    async function scanWifi(){
      $('wifiState').textContent=' scanning...';
      $('wifiOutput').innerHTML='';
      try{
        const d=await api('/api/wifi/channels');
        wifiLast=d;
        $('wifiState').textContent=' found '+d.total+' networks';
        if(wifiMode==='channels')renderChannels();
        else if(wifiMode==='radar')renderRadarPicker();
        else if(wifiMode==='direction')renderRadarPicker();
        else renderScanner();
      }catch(e){$('wifiState').textContent=' scan failed';}
    }

    loadStatus(); loadIr();
    renderWifiTools();
    setInterval(loadStatus,1500);
    setInterval(()=>{if(activeTab==='cc')readCc();},2200);
  </script>
</body>
</html>
)HTML";

static String jsonEscape(const String& text) {
    String out;
    out.reserve(text.length() + 8);
    for (size_t i = 0; i < text.length(); i++) {
        char c = text[i];
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if ((uint8_t)c < 0x20) {}
        else out += c;
    }
    return out;
}

static String authName(wifi_auth_mode_t type) {
    switch (type) {
        case WIFI_AUTH_OPEN: return "OPEN";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-E";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
        default: return "UNKNOWN";
    }
}

static String hexByte(uint8_t value) {
    char buf[3];
    snprintf(buf, sizeof(buf), "%02X", value);
    return String(buf);
}

static void sendJson(const String& json) {
    server.sendHeader("Cache-Control", "no-store");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
}

static void sendError(uint16_t code, const String& message) {
    server.sendHeader("Cache-Control", "no-store");
    server.send(code, "application/json",
                "{\"ok\":false,\"error\":\"" + jsonEscape(message) + "\"}");
}

static void webBeaconStop() {
    if (!webBeaconActive) return;
    webBeaconActive = false;
    esp_wifi_set_promiscuous(false);
}

static void webBeaconSendOne(const char* ssid) {
    uint8_t ssidLen = strlen(ssid);
    if (ssidLen > 32) ssidLen = 32;

    uint8_t macTail = static_cast<uint8_t>(webBeaconSent & 0xFF);
    webBeaconFrame[10] = 0x02;
    webBeaconFrame[11] = 0x34;
    webBeaconFrame[12] = 0x56;
    webBeaconFrame[13] = 0x70 | (webBeaconIndex & 0x0F);
    webBeaconFrame[14] = static_cast<uint8_t>((webBeaconSent >> 8) & 0xFF);
    webBeaconFrame[15] = macTail;
    for (uint8_t i = 0; i < 6; i++) {
        webBeaconFrame[16 + i] = webBeaconFrame[10 + i];
    }

    webBeaconFrame[WEB_BEACON_SSID_LEN_OFFSET] = ssidLen;
    memcpy(&webBeaconFrame[WEB_BEACON_SSID_OFFSET], ssid, ssidLen);
    uint8_t tailOffset = WEB_BEACON_SSID_OFFSET + ssidLen;
    memcpy(&webBeaconFrame[tailOffset], WEB_BEACON_TAIL, sizeof(WEB_BEACON_TAIL));
    uint8_t frameLen = tailOffset + sizeof(WEB_BEACON_TAIL);

    esp_wifi_80211_tx(WIFI_IF_AP, webBeaconFrame, frameLen, false);
    webBeaconSent++;
    webBeaconCurrentSsid = String(ssid);
}

static void webBeaconStart(uint16_t seconds) {
    if (seconds == 0 || seconds > WEB_BEACON_MAX_SECONDS) {
        seconds = WEB_BEACON_MAX_SECONDS;
    }
    WiFi.mode(WIFI_AP_STA);
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_channel(DASH_AP_CHANNEL, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(true);
    webBeaconActive = true;
    webBeaconStartedMs = millis();
    webBeaconEndMs = webBeaconStartedMs + (uint32_t)seconds * 1000UL;
    webBeaconLastSendMs = 0;
    webBeaconSent = 0;
    webBeaconIndex = 0;
    webBeaconCurrentSsid = "";
}

static void webBeaconTick() {
    if (!webBeaconActive) return;
    uint32_t now = millis();
    if ((int32_t)(now - webBeaconEndMs) >= 0) {
        webBeaconStop();
        return;
    }
    if (now - webBeaconLastSendMs < WEB_BEACON_INTERVAL_MS) return;

    const char* ssid = WEB_BEACON_SSIDS[webBeaconIndex % WEB_BEACON_COUNT];
    webBeaconSendOne(ssid);
    webBeaconIndex = (webBeaconIndex + 1) % WEB_BEACON_COUNT;
    webBeaconLastSendMs = now;
}

static uint8_t countSavedIr() {
    uint8_t total = 0;
    for (uint8_t slot = 0; slot < signalToolsSavedIrMax(); slot++) {
        if (signalToolsLoadSavedIrInfo(slot, nullptr, nullptr)) total++;
    }
    return total;
}

static void handleRoot() {
    String page = FPSTR(WEB_DASHBOARD_HTML);
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "text/html", page);
}

static void handleStatus() {
    String json;
    json.reserve(640);
    json += "{";
    json += "\"uptimeMs\":" + String(millis() - dashboardStartedMs) + ",";
    json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"minHeap\":" + String(ESP.getMinFreeHeap()) + ",";
    json += "\"clients\":" + String(WiFi.softAPgetStationNum()) + ",";
    json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
    json += "\"mac\":\"" + WiFi.softAPmacAddress() + "\",";
    json += "\"irSaved\":" + String(countSavedIr()) + ",";
    json += "\"irMax\":" + String(signalToolsSavedIrMax()) + ",";
    json += "\"irLevel\":" + String(digitalRead(IR_RX_PIN)) + ",";
    json += "\"gdo0Level\":" + String(digitalRead(CC1101_GDO0_PIN)) + ",";
    json += "\"beacon\":{";
    json += "\"active\":" + String(webBeaconActive ? "true" : "false") + ",";
    json += "\"sent\":" + String(webBeaconSent) + ",";
    json += "\"leftMs\":" + String(webBeaconActive && (int32_t)(webBeaconEndMs - millis()) > 0
                                  ? webBeaconEndMs - millis()
                                  : 0) + ",";
    json += "\"channel\":" + String(DASH_AP_CHANNEL) + ",";
    json += "\"current\":\"" + jsonEscape(webBeaconCurrentSsid) + "\"},";
    json += "\"pins\":{";
    json += "\"irTx\":" + String(IR_TX_PIN) + ",";
    json += "\"irRx\":" + String(IR_RX_PIN) + ",";
    json += "\"ccCsn\":" + String(CC1101_CSN_PIN) + ",";
    json += "\"ccGdo0\":" + String(CC1101_GDO0_PIN) + ",";
    json += "\"ccTx\":" + String(CC1101_TX_DATA_PIN);
    json += "}}";
    sendJson(json);
}

static void handleIrList() {
    String json;
    json.reserve(640);
    json += "{\"items\":[";
    bool first = true;
    for (uint8_t slot = 0; slot < signalToolsSavedIrMax(); slot++) {
        String name;
        uint16_t count = 0;
        if (!signalToolsLoadSavedIrInfo(slot, &name, &count)) continue;
        if (!first) json += ",";
        first = false;
        json += "{\"slot\":" + String(slot) + ",";
        json += "\"name\":\"" + jsonEscape(name) + "\",";
        json += "\"count\":" + String(count) + "}";
    }
    json += "]}";
    sendJson(json);
}

static bool parseSlot(uint8_t* slotOut) {
    if (!slotOut || !server.hasArg("slot")) return false;
    int slot = server.arg("slot").toInt();
    if (slot < 0 || slot >= signalToolsSavedIrMax()) return false;
    *slotOut = static_cast<uint8_t>(slot);
    return true;
}

static void handleIrReplay() {
    uint8_t slot = 0;
    if (!parseSlot(&slot)) {
        sendError(400, "bad slot");
        return;
    }
    bool ok = signalToolsReplaySavedIrSlot(slot);
    sendJson(String("{\"ok\":") + (ok ? "true" : "false") + "}");
}

static void handleIrDelete() {
    uint8_t slot = 0;
    if (!parseSlot(&slot)) {
        sendError(400, "bad slot");
        return;
    }
    bool ok = signalToolsDeleteSavedIrSlot(slot);
    sendJson(String("{\"ok\":") + (ok ? "true" : "false") + "}");
}

static void handleIrRename() {
    uint8_t slot = 0;
    if (!parseSlot(&slot) || !server.hasArg("name")) {
        sendError(400, "bad rename");
        return;
    }
    String name = server.arg("name");
    name.trim();
    bool ok = signalToolsRenameSavedIrSlot(slot, name);
    sendJson(String("{\"ok\":") + (ok ? "true" : "false") + "}");
}

static void handleCc1101() {
    uint32_t freqKHz = 433920;
    if (server.hasArg("freq")) {
        freqKHz = static_cast<uint32_t>(server.arg("freq").toInt());
    }

    CC1101DashboardStatus st;
    cc1101DashboardRead(freqKHz, &st);

    String json;
    json.reserve(360);
    json += "{";
    json += "\"ready\":" + String(st.ready ? "true" : "false") + ",";
    json += "\"freqKHz\":" + String(st.freqKHz) + ",";
    json += "\"freqMHz\":\"" + String(st.freqKHz / 1000) + "." + String((st.freqKHz % 1000) / 10) + "\",";
    json += "\"partnum\":\"" + hexByte(st.partnum) + "\",";
    json += "\"version\":\"" + hexByte(st.version) + "\",";
    json += "\"marc\":\"" + hexByte(st.marc) + "\",";
    json += "\"pktStatus\":\"" + hexByte(st.pktStatus) + "\",";
    json += "\"lqi\":" + String(st.lqi) + ",";
    json += "\"rssiRaw\":" + String(st.rssiRaw) + ",";
    json += "\"rssiDbm\":" + String(st.rssiDbm) + ",";
    json += "\"gdo0\":" + String(st.gdo0);
    json += "}";
    sendJson(json);
}

static void handleWifiChannels() {
    webBeaconStop();
    WiFi.mode(WIFI_AP_STA);
    int found = WiFi.scanNetworks(false, true);
    if (found < 0) {
        sendError(500, "scan failed");
        return;
    }
    if (found > 60) found = 60;

    uint8_t counts[14] = {0};
    uint8_t openCounts[14] = {0};
    int best[14];
    for (uint8_t i = 0; i < 14; i++) best[i] = -127;

    for (int i = 0; i < found; i++) {
        int ch = WiFi.channel(i);
        if (ch < 1 || ch > 13) continue;
        counts[ch]++;
        wifi_auth_mode_t auth = WiFi.encryptionType(i);
        if (auth == WIFI_AUTH_OPEN) openCounts[ch]++;
        int rssi = WiFi.RSSI(i);
        if (rssi > best[ch]) best[ch] = rssi;
    }

    String json;
    json.reserve(2500 + found * 128);
    json += "{\"total\":" + String(found) + ",\"channels\":[";
    for (uint8_t ch = 1; ch <= 13; ch++) {
        if (ch > 1) json += ",";
        json += "{\"ch\":" + String(ch) + ",";
        json += "\"count\":" + String(counts[ch]) + ",";
        json += "\"open\":" + String(openCounts[ch]) + ",";
        json += "\"best\":" + String(counts[ch] ? best[ch] : 0) + "}";
    }
    json += "],\"networks\":[";
    for (int i = 0; i < found; i++) {
        if (i > 0) json += ",";
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) ssid = "<HIDDEN>";
        json += "{\"ssid\":\"" + jsonEscape(ssid) + "\",";
        json += "\"bssid\":\"" + WiFi.BSSIDstr(i) + "\",";
        json += "\"ch\":" + String(WiFi.channel(i)) + ",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"auth\":\"" + authName(WiFi.encryptionType(i)) + "\"}";
    }
    json += "]}";
    WiFi.scanDelete();
    sendJson(json);
}

static void handleWifiTarget() {
    webBeaconStop();
    if (!server.hasArg("bssid")) {
        sendError(400, "missing bssid");
        return;
    }

    String target = server.arg("bssid");
    target.toUpperCase();

    WiFi.mode(WIFI_AP_STA);
    int found = WiFi.scanNetworks(false, true);
    if (found < 0) {
        sendError(500, "scan failed");
        return;
    }

    String json;
    json.reserve(360);
    json += "{\"found\":false,\"bssid\":\"" + jsonEscape(target) + "\"}";

    for (int i = 0; i < found; i++) {
        String bssid = WiFi.BSSIDstr(i);
        String normalized = bssid;
        normalized.toUpperCase();
        if (normalized != target) continue;

        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) ssid = "<HIDDEN>";
        json = "{";
        json += "\"found\":true,";
        json += "\"ssid\":\"" + jsonEscape(ssid) + "\",";
        json += "\"bssid\":\"" + bssid + "\",";
        json += "\"ch\":" + String(WiFi.channel(i)) + ",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"auth\":\"" + authName(WiFi.encryptionType(i)) + "\"";
        json += "}";
        break;
    }

    WiFi.scanDelete();
    sendJson(json);
}

static void handleWifiBeaconStart() {
    uint16_t seconds = 120;
    if (server.hasArg("seconds")) {
        seconds = static_cast<uint16_t>(server.arg("seconds").toInt());
    }
    webBeaconStart(seconds);
    sendJson("{\"ok\":true,\"active\":true}");
}

static void handleWifiBeaconStop() {
    webBeaconStop();
    sendJson("{\"ok\":true,\"active\":false}");
}

static void handleNotFound() {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
}

static void startDashboardServer() {
    if (!routesConfigured) {
        server.on("/", HTTP_GET, handleRoot);
        server.on("/api/status", HTTP_GET, handleStatus);
        server.on("/api/ir/list", HTTP_GET, handleIrList);
        server.on("/api/ir/replay", HTTP_ANY, handleIrReplay);
        server.on("/api/ir/delete", HTTP_ANY, handleIrDelete);
        server.on("/api/ir/rename", HTTP_ANY, handleIrRename);
        server.on("/api/cc1101", HTTP_GET, handleCc1101);
        server.on("/api/wifi/channels", HTTP_GET, handleWifiChannels);
        server.on("/api/wifi/target", HTTP_GET, handleWifiTarget);
        server.on("/api/wifi/beacon/start", HTTP_ANY, handleWifiBeaconStart);
        server.on("/api/wifi/beacon/stop", HTTP_ANY, handleWifiBeaconStop);
        server.onNotFound(handleNotFound);
        routesConfigured = true;
    }
    server.begin();
}

static void drawDashboardScreen(bool full = true) {
    if (full) {
        tft.fillScreen(TFT_BLACK);
        tft.fillRect(0, 0, 320, 30, TFT_WHITE);
        drawStringCustom(10, 8, "WEB DASHBOARD", TFT_BLACK, 2);
        drawStringCustom(12, 46, "AP SSID", TFT_CYAN, 1);
        drawStringCustom(94, 46, DASH_AP_SSID, TFT_WHITE, 1);
        drawStringCustom(12, 66, "PASSWORD", TFT_CYAN, 1);
        drawStringCustom(94, 66, DASH_AP_PASS, TFT_WHITE, 1);
        drawStringCustom(12, 86, "URL", TFT_CYAN, 1);
        drawStringCustom(94, 86, "http://192.168.4.1", TFT_WHITE, 1);
        drawStringFit(12, 126, "Connect your phone to the AP and open the URL.",
                      TFT_DARKGREY, 292, 1);
        tft.drawFastHLine(0, 216, 320, TFT_WHITE);
        drawStringCustom(10, 224, "HOLD OK TO STOP", TFT_WHITE, 2);
    }

    tft.fillRect(12, 154, 296, 45, TFT_BLACK);
    drawStringCustom(12, 154, "CLIENTS : " + String(WiFi.softAPgetStationNum()),
                     TFT_YELLOW, 1);
    drawStringCustom(12, 174, "UPTIME  : " + String((millis() - dashboardStartedMs) / 1000) + "s",
                     TFT_WHITE, 1);
    drawStringCustom(170, 154, "HEAP: " + String(ESP.getFreeHeap() / 1024) + " KB",
                     TFT_GREEN, 1);
}

void runWebDashboard() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(DASH_AP_SSID, DASH_AP_PASS, DASH_AP_CHANNEL, false,
                DASH_AP_MAX_CLIENTS);
    dashboardStartedMs = millis();
    startDashboardServer();
    drawDashboardScreen(true);

    uint32_t lastDraw = 0;
    while (true) {
        webBeaconTick();
        server.handleClient();

        if (millis() - lastDraw > 1000) {
            drawDashboardScreen(false);
            lastDraw = millis();
        }

        if (digitalRead(BTN_OK) == LOW) {
            bool held = waitOkReleaseWasLong();
            if (held) break;
            beep(1200, 25);
        }

        delay(3);
    }

    server.close();
    webBeaconStop();
    cc1101DashboardStop();
    WiFi.scanDelete();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);

    tft.fillScreen(TFT_BLACK);
    drawStringCustom(40, 92, "WEB DASHBOARD", TFT_WHITE, 2);
    drawStringCustom(76, 122, "STOPPED", TFT_GREEN, 2);
    delay(500);
}
