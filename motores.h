#pragma once

#include "config.h"

// ============================================================
//  TABELA DE COMANDOS
//  Uma linha por movimento: rota, nome no log, sinal lado esq/dir.
//  Para acrescentar um movimento, basta inserir uma linha.
// ============================================================
struct Comando {
  const char* rota;
  const char* nome;
  int8_t esq;     // -1 = ré,  +1 = frente
  int8_t dir;
};

const Comando COMANDOS[] = {
  {"/frente",    "FRENTE",    +1, +1},
  {"/tras",      "TRÁS",      -1, -1},
  {"/esquerda",  "ESQUERDA",  -1, +1},
  {"/direita",   "DIREITA",   +1, -1},
};

const size_t TOTAL_COMANDOS = sizeof(COMANDOS) / sizeof(COMANDOS[0]);

// ============================================================
//  CONTROLE DOS MOTORES
// ============================================================

// Um lado do driver: vel > 0 anda, vel < 0 ré, vel = 0 parado.
void motorLado(int inA, int inB, int vel) {
  digitalWrite(inA, vel > 0);
  digitalWrite(inB, vel < 0);
}

// Único ponto do programa que comanda os motores.
// esq/dir: -255..+255 (sinal = sentido, módulo = PWM)
void mover(int esq, int dir, const char* nome) {
  analogWrite(ENA, abs(esq));
  analogWrite(ENB, abs(dir));

  motorLado(IN1, IN2, esq);
  motorLado(IN3, IN4, dir);

  dbg("[MOTOR] %-9s  Esq: %+4d  Dir: %+4d", nome, esq, dir);
}

void parar() {
  mover(0, 0, "PARAR");
}

// Aplica a tabela de comandos escalada pela velocidade atual.
void executar(const Comando& c) {
  mover(c.esq * velocidade, c.dir * velocidade, c.nome);
}

// ============================================================
//  SETUP
// ============================================================
void iniciarMotores() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

#ifdef ESP8266
  // Redundante na core 3.x (padrão já é 255), mas essencial na 2.x
  // (padrão 1023 (2.x) = motor limitado a 25% da potência se definido para 255).
  analogWriteRange(PWM_MAX);
#endif

  parar();
}
