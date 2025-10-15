#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>

// ========= WiFi =========
const char* ssid     = "Frank_2G";
const char* password = "160051485";

// ========= WS2811 =========
// Si es un único WS2811 para un módulo RGB analógico: 1 LED lógico.
#define NUM_LEDS     4
#define DATA_PIN     25
#define LED_TYPE     WS2811
// Compilamos en GRB (muy común en WS2811)
#define COMPILED_COLOR_ORDER GRB

CRGB leds[NUM_LEDS];
WebServer server(80);

// Estado actual
uint8_t curR = 150, curG = 0, curB = 255;
uint8_t curIntensity = 100;

// Orden deseado del *hardware* (lo que realmente espera el chip/tira).
// Lo usaremos para REMAPEAR los bytes antes de enviarlos.
// Valores válidos: "RGB","RBG","GRB","GBR","BRG","BGR"
String desiredOrder = "GRB"; // empieza igual al compilado

static inline uint8_t clamp255(long v) {
  if (v < 0)   return 0;
  if (v > 255) return 255;
  return (uint8_t)v;
}

// Selecciona el canal correspondiente a 'R','G' o 'B'
static inline uint8_t pick(char ch, uint8_t R, uint8_t G, uint8_t B){
  if (ch=='R') return R;
  if (ch=='G') return G;
  return B;
}

/*
  Remapeo por software:
  Compilado en GRB => FastLED enviará bytes en orden [CRGB.g, CRGB.r, CRGB.b].
  Si el hardware espera 'XYZ' (p.ej. 'RGB'), forzamos:
    CRGB.g = valor de X
    CRGB.r = valor de Y
    CRGB.b = valor de Z
  De este modo, aunque el orden de compilación sea fijo (GRB),
  el LED físico termina recibiendo el orden deseado.
*/
void applyMappedColor(uint8_t R, uint8_t G, uint8_t B, uint8_t intensity) {
  curR = R; curG = G; curB = B;
  curIntensity = intensity;
  FastLED.setBrightness(curIntensity);

  // desiredOrder debe ser longitud 3 con letras R/G/B
  char X = desiredOrder.length()>0 ? desiredOrder[0] : 'G';
  char Y = desiredOrder.length()>1 ? desiredOrder[1] : 'R';
  char Z = desiredOrder.length()>2 ? desiredOrder[2] : 'B';

  // Compilado en GRB: bytes => [g, r, b]
  // Por tanto:
  uint8_t send_g = pick(X, R, G, B);
  uint8_t send_r = pick(Y, R, G, B);
  uint8_t send_b = pick(Z, R, G, B);

  CRGB c(send_r, send_g, send_b);
  for (int i=0;i<NUM_LEDS;i++) leds[i] = c;
  FastLED.show();
}

String jsonState() {
  String s = "{";
  s += "\"r\":" + String(curR) + ",";
  s += "\"g\":" + String(curG) + ",";
  s += "\"b\":" + String(curB) + ",";
  s += "\"intensity\":" + String(curIntensity) + ",";
  s += "\"order\":\"" + desiredOrder + "\"}";
  return s;
}

// ========= UI (HTML) =========
void handleRoot() {
  String html = R"HTML(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1"/>
<title>Control WS2811</title>
<link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.0/dist/css/bootstrap.min.css" rel="stylesheet">
<style>.swatch{height:48px;border-radius:10px;border:1px solid #ddd}</style>
</head>
<body class="bg-light">
<div class="container py-4">
  <h1 class="text-center mb-4">Control de LEDs (WORLDSEMI WS2811)</h1>

  <div class="row g-3">
    <div class="col-12 col-md-6">
      <div class="card shadow-sm">
        <div class="card-body">
          <h5 class="card-title">RGB + Intensidad</h5>
          <div class="row g-2">
            <div class="col-4">
              <label class="form-label">Rojo</label>
              <input id="r" type="number" class="form-control" min="0" max="255" value="150">
            </div>
            <div class="col-4">
              <label class="form-label">Verde</label>
              <input id="g" type="number" class="form-control" min="0" max="255" value="0">
            </div>
            <div class="col-4">
              <label class="form-label">Azul</label>
              <input id="b" type="number" class="form-control" min="0" max="255" value="255">
            </div>
            <div class="col-12">
              <label class="form-label">Intensidad (0–255)</label>
              <input id="intensity" type="range" class="form-range" min="0" max="255" value="100"
                     oninput="document.getElementById('ival').innerText=this.value">
              <small>Valor: <span id="ival">100</span></small>
            </div>

            <div class="col-12 d-flex gap-2">
              <button id="apply" class="btn btn-primary">Asignar</button>
              <button id="get" class="btn btn-outline-secondary">Obtener</button>
              <button id="test" class="btn btn-warning">Test de Colores</button>
            </div>

            <div class="col-12">
              <div id="swatch" class="swatch"></div>
            </div>

            <div class="col-12">
              <label class="form-label">Orden de color</label>
              <select id="order" class="form-select">
                <option>GRB</option><option>RGB</option><option>RBG</option>
                <option>GBR</option><option>BRG</option><option>BGR</option>
              </select>
              <small class="text-muted">Cámbialo si los colores no coinciden.</small>
            </div>
          </div>
          <hr>
          <div class="d-flex gap-2">
            <button class="btn btn-danger preset" data-r="255" data-g="0" data-b="150">Floración</button>
            <button class="btn btn-primary preset" data-r="150" data-g="0" data-b="255">Crecimiento</button>
            <button class="btn btn-dark preset" data-r="255" data-g="255" data-b="255">Blanco</button>
            <button class="btn btn-secondary preset" data-r="0" data-g="0" data-b="0">Apagar</button>
          </div>
        </div>
      </div>
    </div>

    <div class="col-12 col-md-6">
      <div class="card shadow-sm">
        <div class="card-body">
          <h5 class="card-title">Respuesta ESP32</h5>
          <pre id="status" class="bg-light p-3 rounded border small">{...}</pre>
        </div>
      </div>
    </div>
  </div>
</div>

<script>
function clamp(v){v=parseInt(v||0); if(isNaN(v)) v=0; return Math.min(255, Math.max(0, v));}
function syncSwatch(){
  const r=clamp(document.getElementById('r').value);
  const g=clamp(document.getElementById('g').value);
  const b=clamp(document.getElementById('b').value);
  document.getElementById('swatch').style.backgroundColor = `rgb(${r},${g},${b})`;
}
async function refreshStatus(){
  try{
    const res = await fetch('/state');
    document.getElementById('status').textContent = await res.text();
  }catch(e){ document.getElementById('status').textContent = 'Error al obtener estado'; }
}
document.addEventListener('input', e=>{
  if(['r','g','b','intensity'].includes(e.target.id)) syncSwatch();
});
document.querySelectorAll('.preset').forEach(btn=>{
  btn.addEventListener('click', ()=>{
    document.getElementById('r').value = clamp(btn.dataset.r);
    document.getElementById('g').value = clamp(btn.dataset.g);
    document.getElementById('b').value = clamp(btn.dataset.b);
    syncSwatch();
  });
});
document.getElementById('apply').addEventListener('click', async ()=>{
  const payload = {
    r: clamp(document.getElementById('r').value),
    g: clamp(document.getElementById('g').value),
    b: clamp(document.getElementById('b').value),
    intensity: clamp(document.getElementById('intensity').value)
  };
  try{
    const res = await fetch('/api/color', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify(payload)
    });
    document.getElementById('status').textContent = await res.text();
  }catch(e){ document.getElementById('status').textContent = 'Error al asignar'; }
});
document.getElementById('get').addEventListener('click', refreshStatus);
document.getElementById('test').addEventListener('click', async ()=>{
  try{
    const res = await fetch('/api/test');
    document.getElementById('status').textContent = await res.text();
  }catch(e){ document.getElementById('status').textContent = 'Error en test'; }
});
document.getElementById('order').addEventListener('change', async (e)=>{
  try{
    const res = await fetch('/api/order?order='+encodeURIComponent(e.target.value));
    document.getElementById('status').textContent = await res.text();
    await refreshStatus();
  }catch(err){ document.getElementById('status').textContent = 'No se pudo cambiar orden'; }
});
syncSwatch(); refreshStatus();
</script>
</body>
</html>
)HTML";
  server.send(200, "text/html; charset=utf-8", html);
}

// ========= Handlers =========
void handlePostColor() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Access-Control-Allow-Methods", "POST, OPTIONS");
  if (server.method() == HTTP_OPTIONS) { server.send(204); return; }

  uint8_t r, g, b, intensity;
  if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b")) {
    r = clamp255(server.arg("r").toInt());
    g = clamp255(server.arg("g").toInt());
    b = clamp255(server.arg("b").toInt());
    intensity = server.hasArg("intensity") ? clamp255(server.arg("intensity").toInt()) : curIntensity;
  } else {
    String body = server.arg("plain");
    auto findNum = [&](const char* key, int defVal)->int {
      int idx = body.indexOf(String("\"") + key + "\""); if (idx < 0) return defVal;
      idx = body.indexOf(':', idx); if (idx < 0) return defVal;
      int p = idx + 1; while (p < (int)body.length() && (body[p]==' '||body[p]=='\t')) p++;
      long val = 0; bool any=false; int sign=1;
      if (p < (int)body.length() && body[p]=='-'){ sign=-1; p++; }
      while (p < (int)body.length() && isdigit((unsigned char)body[p])) { val = val*10 + (body[p]-'0'); p++; any=true; }
      if (!any) return defVal; return (int)(val*sign);
    };
    r = clamp255(findNum("r", curR));
    g = clamp255(findNum("g", curG));
    b = clamp255(findNum("b", curB));
    intensity = clamp255(findNum("intensity", curIntensity));
  }
  applyMappedColor(r,g,b,intensity);
  server.send(200, "application/json; charset=utf-8", jsonState());
}

void handleState() {
  server.send(200, "application/json; charset=utf-8", jsonState());
}

void handleOrder() {
  if (!server.hasArg("order")) {
    server.send(400, "application/json; charset=utf-8", "{\"error\":\"falta 'order'\"}");
    return;
  }
  String o = server.arg("order");
  if (o=="RGB"||o=="RBG"||o=="GRB"||o=="GBR"||o=="BRG"||o=="BGR") {
    desiredOrder = o;
    // Reaplica el color con el nuevo remapeo
    applyMappedColor(curR,curG,curB,curIntensity);
    server.send(200, "application/json; charset=utf-8", jsonState());
  } else {
    server.send(400, "application/json; charset=utf-8", "{\"error\":\"order inválido\"}");
  }
}

void handleTest() {
  uint8_t br = curIntensity;
  struct Step { uint8_t r,g,b; } steps[] = {
    {255,0,0}, {0,255,0}, {0,0,255}, {255,255,255}, {0,0,0}
  };
  for (auto &s : steps) { applyMappedColor(s.r, s.g, s.b, br); delay(600); }
  applyMappedColor(curR, curG, curB, br);
  server.send(200, "application/json; charset=utf-8", "{\"ok\":true,\"msg\":\"Test: R,G,B,Blanco,Off\"}");
}

// ========= Setup / Loop =========
void setup() {
  Serial.begin(115200);
  delay(100);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COMPILED_COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(curIntensity);
  applyMappedColor(curR, curG, curB, curIntensity);

  WiFi.begin(ssid, password);
  Serial.print("Conectando a "); Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi conectado. IP: " + WiFi.localIP().toString());

  server.on("/", HTTP_GET, [](){ handleRoot(); });
  server.on("/api/color", HTTP_POST, [](){ handlePostColor(); });
  server.on("/api/color", HTTP_OPTIONS, [](){ server.send(204); });
  server.on("/api/order", HTTP_GET, [](){ handleOrder(); });
  server.on("/api/test",  HTTP_GET, [](){ handleTest(); });
  server.on("/state",     HTTP_GET, [](){ handleState(); });

  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

void loop() {
  server.handleClient();
  delay(2);
}
