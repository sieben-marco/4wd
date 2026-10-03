#pragma once

#include "config.h"
#include "motores.h"
#include "web.h"

// ============================================================
//  ROTAS HTTP
//
//  As 4 rotas de movimento sao geradas a partir da tabela
//  COMANDOS[] — uma linha cada, sem lambda repetida.
// ============================================================
void registrarRotas() {

  // Interface de controle (servida da flash)
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
  });

  // Keep-alive da interface (ping a cada 4s)
  server.on("/ping", HTTP_GET, []() {
    server.send(200, "text/plain", "pong");
  });

  // Movimentos: /frente  /tras  /esquerda  /direita
  for (size_t i = 0; i < TOTAL_COMANDOS; i++) {
    const Comando* c = &COMANDOS[i];     // ponteiro fixo: COMANDOS e global
    server.on(c->rota, HTTP_GET, [c]() {
      executar(*c);
      ultimoComando = millis();          // rearma o failsafe
      server.send(200, "text/plain", "OK");
    });
  }

  // Parada explicita (desarma o failsafe)
  server.on("/parar", HTTP_GET, []() {
    parar();
    ultimoComando = 0;
    server.send(200, "text/plain", "OK");
  });

  // Velocidade 0-255
  server.on("/speed", HTTP_GET, []() {
    if (server.hasArg("v")) {
      velocidade = constrain(server.arg("v").toInt(), 0, PWM_MAX);
    }
    server.send(200, "text/plain", String(velocidade));
  });

  server.onNotFound([]() {
    server.send(404, "text/plain", "Não encontrado");
  });
}
