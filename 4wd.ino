// ============================================================
//  CARRINHO 4WD — NodeMCU ESP8266
//  Controle por Wi-Fi (Access Point) com interface web
//
//  Estrutura:
//    config.h  — pinagem, Wi-Fi e constantes
//    web.h     — interface HTML/CSS/JS
//    motores.h — movimentos + tabela de comandos
//    rotas.h   — servidor HTTP
//    4wd.ino   — setup() e loop()
// ============================================================

#include "config.h"
#include "web.h"
#include "motores.h"
#include "rotas.h"

// ============================================================
//  WI-FI
// ============================================================
void iniciarWiFi() {
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_SENHA);

  dbg("[WiFi] SSID: %s", WIFI_SSID);
  dbg("[WiFi] IP:   %s", WiFi.softAPIP().toString().c_str());

  if (MDNS.begin(MDNS_NOME)) {
    dbg("[mDNS] %s.local ativo", MDNS_NOME);
  }
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  dbg("");
  dbg("========================================");
  dbg("  CARRINHO 4WD — Iniciando");
  dbg("========================================");

  iniciarMotores();
  iniciarWiFi();

  registrarRotas();
  server.begin();

  dbg("[HTTP] Servidor iniciado");
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  server.handleClient();
  MDNS.update();

  // Failsafe: nenhum comando ha 600ms = carrinho para
  if (ultimoComando > 0 && (millis() - ultimoComando > TIMEOUT_MS)) {
    parar();
    ultimoComando = 0;
    dbg("[FAILSAFE] Timeout de comando acionado");
  }
}
