#pragma once

// ============================================================
//  INTERFACE HTML — 100% offline, servida da flash (PROGMEM)
//  D-pad de 4 setas. Soltar o dedo já dispara o stop().
// ============================================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Carrinho 4WD</title>
  <style>
    :root {
      --bg: #0b0b1a;
      --surface: #13132b;
      --surface-light: #1c1c3a;
      --accent: #00e5ff;
      --accent-dim: #005f6b;
      --text: #e8eaed;
      --muted: #5a5e6e;
      --radius: 16px;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: ui-monospace, 'SF Mono', Consolas, monospace;
      background: var(--bg);
      color: var(--text);
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      min-height: 100dvh;
      overflow: hidden;
      -webkit-user-select: none;
      user-select: none;
      touch-action: none;
    }
    .container {
      width: 100%;
      max-width: 360px;
      padding: 20px 16px;
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 18px;
    }
    header { text-align: center; width: 100%; }
    header h1 {
      font-family: Impact, 'Arial Black', sans-serif;
      font-size: 2.2rem;
      letter-spacing: 0.12em;
      color: var(--accent);
      text-shadow: 0 0 24px rgba(0,229,255,0.25);
      margin-bottom: 4px;
    }
    .status {
      font-size: 0.7rem;
      color: var(--muted);
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 6px;
    }
    .dot {
      width: 8px; height: 8px;
      border-radius: 50%;
      background: #00c853;
      box-shadow: 0 0 8px rgba(0,200,83,0.6);
      animation: pulse 2s ease infinite;
    }
    .dot.off {
      background: #ff1744;
      box-shadow: 0 0 8px rgba(255,23,68,0.6);
      animation: none;
    }
    @keyframes pulse {
      0%,100% { opacity:1; }
      50%     { opacity:0.35; }
    }

    /* Cruz de 4 setas: 3x3 com o centro e os cantos vazios */
    .dpad {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 10px;
      width: 100%;
      max-width: 270px;
    }
    .dpad .vazio { pointer-events: none; }
    .btn {
      aspect-ratio: 1;
      font-size: 1.7rem;
      border: 2px solid var(--accent-dim);
      border-radius: var(--radius);
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      background: var(--surface);
      color: var(--accent);
      box-shadow: 0 4px 12px rgba(0,0,0,0.4);
      transition: background 0.08s, transform 0.08s;
    }
    .btn.on {
      background: var(--accent);
      color: var(--bg);
      border-color: var(--accent);
      box-shadow: 0 0 24px rgba(0,229,255,0.35);
      transform: scale(0.93);
    }

    .speed-box {
      width: 100%;
      max-width: 270px;
      background: var(--surface);
      border-radius: var(--radius);
      padding: 14px 18px;
      border: 1px solid rgba(255,255,255,0.05);
    }
    .speed-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 10px;
    }
    .speed-lbl {
      font-size: 0.65rem;
      letter-spacing: 0.1em;
      color: var(--muted);
    }
    .speed-num {
      font-size: 1rem;
      font-weight: 600;
      color: var(--accent);
    }
    input[type="range"] {
      -webkit-appearance: none;
      appearance: none;
      width: 100%;
      height: 6px;
      background: var(--surface-light);
      border-radius: 3px;
      outline: none;
      cursor: pointer;
      touch-action: auto;
    }
    input[type="range"]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 22px; height: 22px;
      background: var(--accent);
      border-radius: 50%;
      box-shadow: 0 0 10px rgba(0,229,255,0.4);
    }
    input[type="range"]::-moz-range-thumb {
      width: 22px; height: 22px;
      background: var(--accent);
      border: none;
      border-radius: 50%;
    }
    .foot {
      font-size: 0.6rem;
      color: var(--muted);
      text-align: center;
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>CARRINHO 4WD</h1>
      <div class="status">
        <span class="dot" id="dot"></span>
        <span id="stxt">Conectado</span>
      </div>
    </header>

    <div class="dpad" id="dpad">
      <span class="vazio"></span>
      <button class="btn" data-c="frente">&#x25B2;</button>
      <span class="vazio"></span>

      <button class="btn" data-c="esquerda">&#x25C4;</button>
      <span class="vazio"></span>
      <button class="btn" data-c="direita">&#x25BA;</button>

      <span class="vazio"></span>
      <button class="btn" data-c="tras">&#x25BC;</button>
      <span class="vazio"></span>
    </div>

    <div class="speed-box">
      <div class="speed-row">
        <span class="speed-lbl">VELOCIDADE</span>
        <span class="speed-num" id="sval">100%</span>
      </div>
      <input type="range" min="10" max="100" value="100" id="sld">
    </div>

    <div class="foot">192.168.4.1 &middot; carrinho.local</div>
  </div>

  <script>
    const dpad = document.getElementById('dpad');
    const dot  = document.getElementById('dot');
    const stxt = document.getElementById('stxt');
    const sld  = document.getElementById('sld');
    const sval = document.getElementById('sval');

    let cmdCur = null;
    let hb = null;
    let sending = false;
    let pendente = null;   // ultimo comando aguardando a requisicao em voo

    // Uma requisicao em voo por vez (evita travar sockets do ESP).
    // Se algo chegar enquanto ocupa, fica na fila — assim o "parar"
    // nunca e descartado e o carrinho nao segue andando apos soltar.
    function send(action) {
      if (sending) { pendente = action; return; }
      sending = true;
      const c = new AbortController();
      const t = setTimeout(() => c.abort(), 350);

      fetch('/' + action, { signal: c.signal })
        .then(() => online(true))
        .catch(() => {})
        .finally(() => {
          clearTimeout(t);
          sending = false;
          if (pendente) {
            const proximo = pendente;
            pendente = null;
            send(proximo);
          }
        });
    }

    let spdDb = null;
    function sendSpd(pct) {
      clearTimeout(spdDb);
      spdDb = setTimeout(() => {
        const c = new AbortController();
        const t = setTimeout(() => c.abort(), 350);
        fetch('/speed?v=' + Math.round(pct * 2.55), { signal: c.signal })
          .catch(() => {})
          .finally(() => clearTimeout(t));
      }, 70);
    }

    function online(ok) {
      dot.classList.toggle('off', !ok);
      stxt.textContent = ok ? 'Conectado' : 'Desconectado';
    }

    setInterval(() => {
      if (cmdCur) return; // Nao interfere durante o movimento
      const c = new AbortController();
      const t = setTimeout(() => c.abort(), 1000);
      fetch('/ping', { signal: c.signal })
        .then(() => online(true))
        .catch(() => online(false))
        .finally(() => clearTimeout(t));
    }, 4000);

    function go(cmd, btn) {
      if (cmdCur === cmd) return;
      if (hb) { clearInterval(hb); hb = null; }

      cmdCur = cmd;
      dpad.querySelectorAll('.on').forEach(b => b.classList.remove('on'));
      if (btn) btn.classList.add('on');

      send(cmd);
      // Repete o comando para manter o failsafe (600ms) sempre armado
      hb = setInterval(() => {
        if (cmdCur) send(cmdCur);
      }, 250);
    }

    function stop() {
      if (hb) { clearInterval(hb); hb = null; }
      if (!cmdCur) return;
      cmdCur = null;
      dpad.querySelectorAll('.on').forEach(b => b.classList.remove('on'));
      send('parar');
    }

    dpad.addEventListener('pointerdown', e => {
      const btn = e.target.closest('.btn');
      if (!btn) return;
      e.preventDefault();
      try { btn.setPointerCapture(e.pointerId); } catch (_) {}
      go(btn.dataset.c, btn);
    });

    dpad.addEventListener('pointerup', () => stop());
    dpad.addEventListener('pointercancel', () => stop());
    dpad.addEventListener('pointerleave', () => stop());

    sld.addEventListener('input', () => {
      const v = parseInt(sld.value);
      sval.textContent = v + '%';
      sendSpd(v);
    });

    // Permite touchmove apenas no slider de velocidade
    document.addEventListener('touchmove', e => {
      if (e.target === sld) return;
      e.preventDefault();
    }, { passive: false });
  </script>
</body>
</html>
)rawliteral";
