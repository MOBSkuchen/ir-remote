const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>IR Remote</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;600;700&family=Outfit:wght@300;500;700&display=swap');

*{margin:0;padding:0;box-sizing:border-box}

:root{
  --bg:#0a0a0c;
  --surface:#131318;
  --surface2:#1a1a22;
  --border:#2a2a35;
  --text:#e8e8ed;
  --text2:#8888a0;
  --accent:#ff3b5c;
  --accent2:#ff6b4a;
  --green:#22c55e;
  --mono:'JetBrains Mono',monospace;
  --sans:'Outfit',sans-serif;
}

body{
  font-family:var(--sans);
  background:var(--bg);
  color:var(--text);
  min-height:100vh;
  overflow-x:hidden;
}

.grain{
  position:fixed;inset:0;pointer-events:none;z-index:999;
  opacity:.03;
  background-image:url("data:image/svg+xml,%3Csvg viewBox='0 0 256 256' xmlns='http://www.w3.org/2000/svg'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.9' numOctaves='4' stitchTiles='stitch'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23n)'/%3E%3C/svg%3E");
}

header{
  padding:2rem 1.5rem 1rem;
  border-bottom:1px solid var(--border);
  background:linear-gradient(180deg,#12121a 0%,var(--bg) 100%);
}

header h1{
  font-family:var(--mono);
  font-size:1.1rem;
  font-weight:700;
  letter-spacing:-.02em;
  display:flex;align-items:center;gap:.6rem;
}

header h1 .dot{
  width:8px;height:8px;border-radius:50%;
  background:var(--accent);
  box-shadow:0 0 12px var(--accent);
  animation:pulse 2s ease infinite;
}

@keyframes pulse{0%,100%{opacity:1}50%{opacity:.4}}

header p{font-size:.8rem;color:var(--text2);margin-top:.3rem;font-family:var(--mono)}

.container{max-width:540px;margin:0 auto;padding:1rem 1.5rem 4rem}

section{margin-top:1.5rem}

section h2{
  font-family:var(--mono);font-size:.7rem;font-weight:600;
  text-transform:uppercase;letter-spacing:.12em;color:var(--text2);
  margin-bottom:.75rem;
}

.card{
  background:var(--surface);
  border:1px solid var(--border);
  border-radius:10px;
  padding:1.2rem;
  transition:border-color .2s;
}

.card:hover{border-color:#3a3a48}

.btn{
  font-family:var(--mono);font-size:.78rem;font-weight:600;
  border:none;border-radius:7px;padding:.65rem 1.2rem;
  cursor:pointer;transition:all .15s;
  display:inline-flex;align-items:center;gap:.45rem;
}

.btn-primary{
  background:linear-gradient(135deg,var(--accent),var(--accent2));
  color:#fff;
}
.btn-primary:hover{filter:brightness(1.15);transform:translateY(-1px)}
.btn-primary:active{transform:translateY(0)}

.btn-ghost{
  background:var(--surface2);color:var(--text);
  border:1px solid var(--border);
}
.btn-ghost:hover{border-color:var(--text2)}

.btn-sm{padding:.45rem .8rem;font-size:.72rem}

.btn:disabled{opacity:.4;pointer-events:none}

input[type="text"]{
  font-family:var(--mono);font-size:.82rem;
  background:var(--bg);border:1px solid var(--border);
  border-radius:7px;padding:.6rem .9rem;color:var(--text);
  width:100%;outline:none;transition:border-color .2s;
}
input[type="text"]:focus{border-color:var(--accent)}
input[type="text"]::placeholder{color:var(--text2);opacity:.5}

.learn-area{text-align:center}
.learn-area .status{
  font-family:var(--mono);font-size:.75rem;color:var(--text2);
  margin-top:.8rem;min-height:1.2rem;
}
.learn-area .raw-preview{
  font-family:var(--mono);font-size:.65rem;color:var(--text2);
  background:var(--bg);border:1px solid var(--border);border-radius:6px;
  padding:.6rem;margin-top:.6rem;
  max-height:80px;overflow:auto;word-break:break-all;
  text-align:left;display:none;
}

.save-row{display:flex;gap:.5rem;margin-top:.8rem}
.save-row input{flex:1}

.signal-list{display:flex;flex-direction:column;gap:.5rem}

.signal-item{
  display:flex;align-items:center;gap:.6rem;
  background:var(--surface2);border:1px solid var(--border);
  border-radius:8px;padding:.7rem .9rem;
  transition:border-color .2s;
}
.signal-item:hover{border-color:#3a3a48}

.signal-item .name{
  font-family:var(--mono);font-size:.82rem;font-weight:600;flex:1;
}

.signal-item .actions{display:flex;gap:.35rem}

.empty{
  font-family:var(--mono);font-size:.75rem;color:var(--text2);
  text-align:center;padding:2rem 0;
}

.toast{
  position:fixed;bottom:1.5rem;left:50%;transform:translateX(-50%) translateY(100px);
  font-family:var(--mono);font-size:.75rem;
  background:var(--surface);border:1px solid var(--border);
  border-radius:8px;padding:.6rem 1.2rem;
  z-index:1000;transition:transform .3s cubic-bezier(.22,1,.36,1);
  white-space:nowrap;
}
.toast.show{transform:translateX(-50%) translateY(0)}
.toast.ok{border-color:var(--green);color:var(--green)}
.toast.err{border-color:var(--accent);color:var(--accent)}

@keyframes spin{to{transform:rotate(360deg)}}
.spinner{
  width:14px;height:14px;border:2px solid transparent;
  border-top-color:currentColor;border-radius:50%;
  animation:spin .6s linear infinite;display:inline-block;
}
</style>
</head>
<body>
<div class="grain"></div>

<header>
  <h1><span class="dot"></span>IR REMOTE</h1>
  <p>learn &middot; save &middot; send</p>
</header>

<div class="container">
  <section>
    <h2>Capture Signal</h2>
    <div class="card learn-area">
      <button class="btn btn-primary" id="learnBtn" onclick="learn()">
        &#9679; Learn Signal
      </button>
      <div class="status" id="learnStatus"></div>
      <div class="raw-preview" id="rawPreview"></div>
      <div class="save-row" id="saveRow" style="display:none">
        <input type="text" id="saveName" placeholder="signal name">
        <button class="btn btn-ghost btn-sm" onclick="save()">Save</button>
        <button class="btn btn-ghost btn-sm" onclick="sendLearned()">Send</button>
      </div>
    </div>
  </section>

  <section>
    <h2>Saved Signals</h2>
    <div id="signalList" class="signal-list">
      <div class="empty">No saved signals yet</div>
    </div>
  </section>
</div>

<div class="toast" id="toast"></div>

<script>
let currentRaw = null;

function toast(msg, type) {
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.className = 'toast ' + type + ' show';
  setTimeout(() => t.classList.remove('show'), 2200);
}

async function learn() {
  const btn = document.getElementById('learnBtn');
  const st = document.getElementById('learnStatus');
  const rp = document.getElementById('rawPreview');
  const sr = document.getElementById('saveRow');

  btn.disabled = true;
  btn.innerHTML = '<span class="spinner"></span> Listening...';
  st.textContent = 'Point remote at receiver and press a button';
  rp.style.display = 'none';
  sr.style.display = 'none';
  currentRaw = null;

  try {
    const r = await fetch('/learn');
    if (!r.ok) throw new Error(r.status === 408 ? 'Timeout' : 'Error');
    const d = await r.json();
    currentRaw = d.raw;
    st.textContent = d.protocol + ' \u00b7 ' + d.bits + ' bits';
    rp.textContent = d.raw;
    rp.style.display = 'block';
    sr.style.display = 'flex';
    toast('Signal captured', 'ok');
  } catch (e) {
    st.textContent = e.message;
    toast('Capture failed', 'err');
  }

  btn.disabled = false;
  btn.innerHTML = '&#9679; Learn Signal';
}

async function save() {
  const name = document.getElementById('saveName').value.trim();
  if (!name || !currentRaw) return;

  try {
    const r = await fetch('/save', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({name, raw: currentRaw})
    });
    if (!r.ok) throw new Error('Save failed');
    document.getElementById('saveName').value = '';
    toast('Saved: ' + name, 'ok');
    loadList();
  } catch (e) {
    toast(e.message, 'err');
  }
}

async function sendLearned() {
  if (!currentRaw) return;
  await sendSignal(currentRaw);
}

async function sendSignal(raw) {
  try {
    const r = await fetch('/send', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({raw, hz: 38})
    });
    if (!r.ok) throw new Error('Send failed');
    toast('Signal sent', 'ok');
  } catch (e) {
    toast(e.message, 'err');
  }
}

async function deleteSignal(name) {
  try {
    await fetch('/delete', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({name})
    });
    toast('Deleted', 'ok');
    loadList();
  } catch (e) {
    toast('Delete failed', 'err');
  }
}

async function loadList() {
  const el = document.getElementById('signalList');
  try {
    const r = await fetch('/list');
    const list = await r.json();
    if (list.length === 0) {
      el.innerHTML = '<div class="empty">No saved signals yet</div>';
      return;
    }
    el.innerHTML = list.map(s => `
      <div class="signal-item">
        <span class="name">${s.name}</span>
        <div class="actions">
          <button class="btn btn-primary btn-sm" onclick="sendSignal('${s.raw}')">Send</button>
          <button class="btn btn-ghost btn-sm" onclick="deleteSignal('${s.name}')">&times;</button>
        </div>
      </div>
    `).join('');
  } catch (e) {
    el.innerHTML = '<div class="empty">Failed to load</div>';
  }
}

loadList();
</script>
</body>
</html>
)rawliteral";
