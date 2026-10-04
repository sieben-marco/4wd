#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// ============================================================
//  PINAGEM — NodeMCU ESP8266 (sempre D + GPIO)
//
//  ⚠  Os jumpers ENA/ENB do driver DEVEM estar removidos.
//     ESP8266 NÃO tolera 5V nos GPIO.
// ============================================================

// Motores esquerdos
constexpr int IN1 = D1;   // GPIO5  — IN1 do driver
constexpr int IN2 = D2;   // GPIO4  — IN2 do driver

// Motores direitos
constexpr int IN3 = D5;   // GPIO14 — IN3 do driver
constexpr int IN4 = D6;   // GPIO12 — IN4 do driver

// Velocidade — PWM nas entradas de enable do driver
constexpr int ENA = D7;   // GPIO13 — PWM lado esquerdo
constexpr int ENB = D8;   // GPIO15 — PWM lado direito
//   ENB estava em D3 (GPIO0): pino de strapping de boot e botão FLASH.
//   D8 (GPIO15) exige LOW no boot = motores desligados = seguro.

// ============================================================
//  WI-FI (Access Point)
// ============================================================
constexpr const char* WIFI_SSID  = "Carrinho_4WD";
constexpr const char* WIFI_SENHA = "12345678";
constexpr const char* MDNS_NOME  = "carrinho";

// ============================================================
//  COMPORTAMENTO
// ============================================================
constexpr unsigned long TIMEOUT_MS = 600;  // failsafe: sem comando, para
constexpr int PWM_MAX     = 255;           // faixa do analogWrite

// ============================================================
//  LOG — 1 linha por evento
// ============================================================
#define DEBUG 1
#define dbg(fmt, ...)                          \
  do {                                         \
    if (DEBUG) Serial.printf(fmt "\n", ##__VA_ARGS__); \
  } while (0)

// ============================================================
//  ESTADO GLOBAL
// ============================================================
ESP8266WebServer server(80);

unsigned long ultimoComando = 0;   // 0 = failsafe desarmado
int velocidade = PWM_MAX;          // 0–255, definido pela interface
