# AGENTS.md — instruções para agentes de IA

> Arquivo destinado a **agentes de código** (e a quem revisa o trabalho deles).
> Para uma visão humana do projeto, leia [`README.md`](README.md).

---

## 1. Objetivo do projeto

Fazer um carrinho robótico 4WD andar comandado pelo navegador de um celular,
**sem internet**, usando um NodeMCU ESP8266 como Access Point + servidor web +
controlador de motores, tudo num único sketch.

Critério de sucesso:

1. O carrinho responde ao toque em **tempo real** (latência percebida < 150 ms).
2. O carrinho **para sozinho** se a conexão cair (failsafe).
3. O código continua **legível**: a lógica de movimento cabe num arquivo de
   ~75 linhas lido de ponta a ponta.

A dor original do projeto era o contrário disso: 561 linhas num `.ino` só,
57% do arquivo sendo HTML, 9 lambdas idênticas de rota e 9 funções de uma linha
escondendo a lógica real. **A refatoração existe para consertar isso — não
reverta para o formato monolítico.**

---

## 2. Arquitetura

```
4wd/
├── 4wd.ino    setup() e loop()            ~58 linhas
├── config.h   pinagem, Wi-Fi, constantes  ~57 linhas
├── motores.h  movimento + tabela          ~75 linhas  ← fonte da verdade
├── rotas.h    registro HTTP               ~47 linhas
├── web.h      interface HTML/CSS/JS       ~320 linhas
├── README.md  doc para humanos
└── AGENTS.md  este arquivo
```

### Fluxo de um comando

```
toque na seta
  └► JS go() ── heartbeat 250ms ──► GET /frente
        └► rotas.h: lambda da tabela COMANDOS[]
              └► motores.h executar() → mover(esq, dir, nome)
                    ├─ analogWrite(ENA, |esq|)     PWM
                    ├─ analogWrite(ENB, |dir|)
                    ├─ motorLado(IN1,IN2,esq)      sentido
                    ├─ motorLado(IN3,IN4,dir)
                    └─ dbg("[MOTOR] ...")          1 linha de log
              └► ultimoComando = millis()          rearma failsafe
```

```
loop() ── se millis() - ultimoComando > 600ms ──► parar() + log FAILSAFE
```

### Tabela de comandos (fonte da verdade)

`motores.h` define `COMANDOS[]`. **Rotas HTTP e movimentos vêm da mesma
tabela** — o laço em `rotas.h` gera as rotas a partir dela.

```cpp
const Comando COMANDOS[] = {
  {"/frente",    "FRENTE",    +1, +1},
  {"/tras",      "TRÁS",      -1, -1},
  {"/esquerda",  "ESQUERDA",  -1, +1},
  {"/direita",   "DIREITA",   +1, -1},
};
```

Adicionar um movimento = **1 linha nesta tabela + 1 botão no `web.h`**.
Não crie uma função nova nem uma lambda nova.

---

## 3. Invariantes — NÃO quebrar

Teste mental obrigatório antes de entregar qualquer mudança:

| # | Invariante | Onde vive |
|---|---|---|
| 1 | Toda rota de movimento **deve** atribuir `ultimoComando = millis()` | `rotas.h` |
| 2 | `/parar` define `ultimoComando = 0` (desarma o failsafe) | `rotas.h` |
| 3 | `TIMEOUT_MS` (600) deve ficar **muito acima** do heartbeat JS (250). Se mexer em um, mexa no outro | `config.h` + `web.h` |
| 4 | Formatos de resposta: `/ping`→`pong`, movimentos→`OK`, `/speed`→**o número** da velocidade | `rotas.h` |
| 5 | O JS envia **no máximo 1 requisição em voo**, com fila de 1 posição — senão o ESP trava os sockets e o `parar` é perdido | `web.h` |
| 6 | O HTML **tem** de ficar em `PROGMEM` — a RAM da ESP8266 é só 80 KB | `web.h` |
| 7 | `analogWriteRange(PWM_MAX)` **não pode ser removido** — redundante na core 3.x, mas essencial na 2.x (padrão 1023 = motor a 25%) | `motores.h` |
| 8 | `parar()` deve ser chamado no fim de `iniciarMotores()` (motores parados no boot) | `motores.h` |
| 9 | Os 4 arquivos `.h` usam `#pragma once` e são incluídos uma única vez (definições de objeto global) | todos |
| 10 | Só **4** botões de movimento — sem diagonais, sem botão ■ parar | `web.h` |

---

## 4. Decisões de design — e o porquê

Registre aqui qualquer mudança de direção para o próximo agente.

### Pinagem

| Decisão | Motivo |
|---|---|
| **ENB em D8 (GPIO15)**, não em D3 (GPIO0) | GPIO0 é *strapping* de boot: precisa estar HIGH para sair do flash, e é o pino do botão FLASH. GPIO15 exige LOW no boot — que é exatamente o estado desejado (EN desligado = motores parados), e a placa já tem pull-down que garante isso. |
| **D0 (GPIO16) rejeitado para PWM** | O PWM é por software. Nos pinos 0–15 cada borda é **1 escrita** em `GPOS`/`GPOC` (barramento rápido); no GPIO16 exige **escrita extra** em `GP16O` (domínio RTC) — verificado em `core_esp8266_waveform_pwm.cpp` da core 3.1.2. Além disso a NodeMCU V3 pode ter o LED onboard nesse pino (carrega o PWM e faz piscar), não há relato oficial de instabilidade com múltiplos canais PWM, e GPIO16 é o único pino **sem pull-up interno**. *(Correção: a hipótese de que o nível baixo de ~1V ficaria "fora da spec" do L298N está ERRADA — o L298N aceita LOW até 1.5V, então 1V está dentro. Não usar esse argumento.)* |
| **TX (GPIO1) / RX (GPIO3) intocáveis** | Ocupados pela Serial de debug e pelo chip USB-serial. |
| **GPIO6–11 intocáveis** | Ligados à flash — uso derruba o sketch. |
| Pinos D1 (GPIO5) e D2 (GPIO4) preferidos para sinais | Únicos que ficam LOW durante todo o boot. |

### Comportamento

| Decisão | Motivo |
|---|---|
| `PWM_FREQ_HZ = 20000` Não usar: sobreaquece a L928N | 1 kHz (padrão) zumba em motor CC. 20 kHz fica fora da faixa audível. A doc da core avisa que 40 kHz já sobrecarrega a CPU — com 2 saídas, 20 kHz é folgado. |
| Failsafe de 600 ms + heartbeat de 250 ms | Cobertura de 2,4×: mesmo com 1-2 heartbeats perdidos, o carrinho não segue andando. Se alterar um, mantenha essa margem. |
| **Sem botão ■ parar** | `pointerup` já dispara `stop()` e o failsafe cobre quedas de Wi-Fi. Menos UI = menos superfície de erro. |
| Fila de 1 posição no `send()` do JS | O código original descartava silenciosamente o `parar` se uma requisição do heartbeat estivesse em voo → o carrinho seguia até o failsafe (até 600 ms andando após soltar o dedo). |
| Uma requisição em voo por vez | O `ESP8266WebServer` é sensível a conexões concorrentes. |
| Logs "enxutos" (`dbg()` 1 linha/ação) | Debug em 115200 legível. `logPins()` foi **removido**: usava `analogRead()` em pino de saída, que não retorna o duty cycle escrito. |
| Tabela em vez de 9 funções + 9 lambdas | Eliminou ~90 linhas duplicadas e fez rotas e movimentos terem **uma** fonte de verdade. |
| Multi-arquivo | Separação por responsabilidade. Não colapsar de volta num `.ino` só. |

### Bugs corrigidos na refatoração (não reintroduzir)

1. `analogRead(ENA/ENB)` em pino de saída dentro de `logPins()`.
2. ENB em D3 (GPIO0) — conflito de boot.
3. 9 lambdas `server.on()` quase idênticas.
4. `#ifdef USE_PWM` espalhado em 4 pontos (hoje a lógica é única, em `mover()`).
5. Descarte silencioso do comando `parar` no JS.

### Segurança elétrica — curto, sobrecarga e resistores

Análise verificada contra o datasheet do ESP8266EX, o datasheet/módulo do L298N
e o esquema do módulo genérico de L298N.

**Sobrecarga de corrente — sem risco**

O limite do ESP8266 é **12 mA por pino** (I/O max, datasheet ESP8266EX). As 6
saídas ligam em **entradas TTL** do L298N (alta impedância) → consumo de
**µA**, não mA. Folga superior a 99%. Não há resistor de limitação a montante.

**Tensão — sem risco**

| Estado | ESP8266 entrega | L298N exige | Folga |
|---|---|---|---|
| HIGH | 3,3 V | ≥ 2,3 V | 1,0 V |
| LOW | ≈ 0,1 V | ≤ 1,5 V | 1,4 V |

Não há conversão de nível: 3,3 V satisfaz o limiar TTL de 2,3 V com folga.

**Resistores — nenhum é necessário**

| Pino | Resistor emulado | Veredito |
|---|---|---|
| D1, D2, D5, D6 → IN1..IN4 | — | Entradas TTL passivas. **Nada a adicionar.** |
| D7 (GPIO13) → ENA | **nenhum na placa** | Ver "boot" abaixo — ver reforço opcional. |
| D8 (GPIO15) → ENB | **pull-down externo da placa** | Obrigatório para o boot do ESP8266 → o resistor que se precisaria já vem soldado. |
| D3 (GPIO0), D4 (GPIO2) | pull-up externo 10 kΩ da placa | Não estão em uso. |
| D0 (GPIO16) | pull-down **interno**, **sem pull-up interno** | É o único pino que *exigiria* resistor externo se precisasse de pull-up. |

> **Origem provável da memória de que "o D0 dispensa resistor":** GPIO16 é o
> único pino com pull-down de fábrica; GPIO0–15 só têm pull-up interno. Isso é
> verdade — mas **não ajuda aqui**, porque o D8 também já vem com pull-down
> (externo, na placa), e nenhum sinal do projeto é entrada com pull-up.

**Único risco real de curto: o jumper ENA/ENB**

O módulo de L298N liga, pelo jumper, o pino EN ao **plano de +5V**. Com o
GPIO ligado nesse mesmo pino e dirigindo LOW, cria-se um caminho de baixa
impedância 5V → saída de 3,3 V → sobrecorrente e **dano permanente no GPIO**
(ESP8266 **não tolera 5V**).

- Jumper **removido** + GPIO ligado → EN vira entrada TTL passiva, µA. **Seguro.**
- Jumper **colocado** + GPIO ligado → **curto. Perigoso.**
- Jumper **colocado** + *sem* GPIO → inofensivo (só ignora o PWM).

**Boot — por que nada gira mesmo sem pull-down no ENA**

| Pino | Estado medido no boot | Efeito no L298N |
|---|---|---|
| D1/D2 (IN1/IN2) | **LOW** | lado esquerdo: ambos LOW = **freio** |
| D5/D6 (IN3/IN4) | **HIGH** (~110 ms) | lado direito: ambos HIGH = **freio** |
| D8 (GPIO15) → ENB | **LOW** (pull-down da placa) | lado direito **desabilitado** |
| D7 (GPIO13) → ENA | **flutua** (placa não tem pull) | irrelevante: entradas em freio |

Verdade da tabela L298N: entradas iguais = motor parado (freio rápido). Como
cada lado nasce com as duas entradas no mesmo nível, **nenhum lado gira no
boot**, mesmo com o ENA flutuando.

**Reforço opcional (não obrigatório):** um pull-down de 10 kΩ de ENA
(D7/GPIO13) para GND torna o comportamento à prova de ruído caso as entradas
mudem de nível durante o boot. O ENB (D8) não precisa — a placa já tem.

**Outras causas de dano, fora dos GPIO**

- **GND comum ausente** entre NodeMCU e driver → referência flutuante,
  comportamento imprevisível e possível retroalimentação.
- **Tensão de motor num GPIO** (fiação trocada) → destruição imediata.
- **Polaridade invertida** na alimentação dos motores.
- L298N em curto/stall → esquenta e rebaixa a tensão (queda de 2–3 V é
  normal do bipolar), mas isso é problema do driver, não do ESP8266.

**Conflitos de pino a considerar no futuro**

| Pinos | Função alternativa |
|---|---|
| D5/D6/D7/D8 (GPIO14/12/13/15) | **SPI completa** (SCLK/MISO/MOSI/CS) — um display SPI não teria pinos livres |
| D1/D2 (GPIO5/GPIO4) | **I2C padrão** (SCL/SDA) — um OLED I2C conflita com IN1/IN2 |
| TX/RX (GPIO1/GPIO3) | Únicos realmente livres, hoje ocupados pela Serial |

---

## 5. Como compilar e testar após mudanças

### 5.1 Compilação (obrigatória, não requer a placa)

```powershell
$cli = "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
$sk  = "C:\Users\04513901055\Documents\Arduino\4wd"

& $cli compile --fqbn esp8266:esp8266:nodemcuv2 $sk
$LASTEXITCODE    # <-- DEVE SER 0
```

> **Atenção:** o arduino-cli escreve a tabela de memória no *stderr*. No
> PowerShell isso aparece como `NativeCommandError` e **não** significa falha.
> **O único sinal de sucesso é `$LASTEXITCODE -eq 0`.** Não confie na saída
> textual, confie no código de saída.

**Critérios de aprovação:**

| Verificar | Valor |
|---|---|
| `$LASTEXITCODE` | `0` |
| Avisos de compilação | **nenhum** |
| RAM | `< 80192` (baseline: 29 212) |
| Flash | `< 1048576` (baseline: 301 088) |
| IRAM | `< 65536` (baseline: 61 659) |

Uma subida grande de RAM costuma ser String/heap ou HTML fora de `PROGMEM`.

### 5.2 Gravar e testar (requer a placa)

```powershell
& $cli board list                          # achar a porta
& $cli upload --fqbn esp8266:esp8266:nodemcuv2 -p COMx $sk
& $cli monitor -p COMx -c 115200           # Ctrl+C sai do monitor
```

### 5.3 Checklist de teste manual

Marque todos antes de dizer que está pronto:

**Boot / Serial**
- [ ] Sem crash no boot; sai a banner `CARRINHO 4WD — Iniciando`
- [ ] Sai `[WiFi] IP: 192.168.4.1` e `[mDNS] carrinho.local ativo`
- [ ] Sai `[HTTP] Servidor iniciado`
- [ ] **Nenhum motor gira no boot**

**Interface**
- [ ] Rede `Carrinho_4WD` aparece e conecta com `12345678`
- [ ] `192.168.4.1` carrega a página; pontinho **verde** "Conectado"
- [ ] Cruz de **4** setas, sem botão ■ parar, sem diagonais
- [ ] Segurar ▲ → log `[MOTOR] FRENTE  Esq: +255  Dir: +255` e o carrinho anda
- [ ] **Soltar → carrinho para na hora** (tipicamente < 100 ms; no pior caso,
      antes dos 600 ms do failsafe — o ponto é **não** depender do failsafe)
- [ ] ◄ e ► giram no próprio eixo (lados em sentidos opostos)
- [ ] Slider muda a velocidade; ao soltar, aplica (log com valores menores)

**Failsafe (teste obrigatório — é a segurança do projeto)**
- [ ] Com o carrinho andando, **fechar a aba** → para em ≤ 600 ms e log
      `[FAILSAFE] Timeout de comando acionado`
- [ ] Com o carrinho andando, **desconectar o Wi-Fi do celular** → para ≤ 600 ms

**HTTP**
- [ ] `Invoke-RestMethod "http://192.168.4.1/ping"` retorna `pong`
- [ ] `.../frente` retorna `OK`, depois `.../parar` retorna `OK`
- [ ] `.../speed?v=100` retorna `100`
- [ ] Rota inexistente → `404 Não encontrado`

**Regressão de conexão**
- [ ] Segurar uma seta por **30 s** seguidos sem travar a página
      (valida o heartbeat de 250 ms e a fila de 1 requisição)

### 5.4 Teste rápido sem mexer no hardware

```powershell
# Num terminal, com o PC na rede Carrinho_4WD:
Invoke-RestMethod "http://192.168.4.1/ping"
1..20 | ForEach-Object { Invoke-RestMethod "http://192.168.4.1/frente"; Start-Sleep -Milliseconds 200 }
Invoke-RestMethod "http://192.168.4.1/parar"
# Pare o Invoke-RestMethod acima por 1s e veja se o FAILSAFE dispara.
```

---

## 6. Convenções de código

- **Idioma:** comentários, logs e textos da UI em **português**.
- **Nomenclatura:** funções e variáveis em português (`mover`, `parar`,
  `velocidade`, `ultimoComando`). Estruturas em PascalCase (`Comando`).
- **Pinos:** sempre documentar como **D + GPIO** (`D8 (GPIO15)`), nunca só um.
  É a causa #1 de erro de fiação na ESP8266.
- **Constantes:** `constexpr` em `config.h`, em MAIÚSCULAS com underscore.
- **Log:** usar a macro `dbg()` — nunca `Serial.printf` solto. Para silenciar,
  `#define DEBUG 0`.
- **Movimento:** passar sempre por `mover()`. Não chamar `digitalWrite`/`analogWrite`
  fora dela — é o único ponto que aplica PWM + sentido + log.
- **Escopo:** não adicionar bibliotecas novas sem justificativa; a RAM é curta.
- **Estilo:** manter o padrão dos arquivos existentes (blocos de cabeçalho com
  `// ====`, comentários explicando o *porquê*, não o *o quê*).

---

## 7. Armadilhas conhecidas

| Armadilha | Detalhe |
|---|---|
| `NativeCommandError` no PowerShell | É só o *stderr* do arduino-cli. Verifique `$LASTEXITCODE`. |
| `analogWriteRange` removido | Na core 2.x o padrão é 1023 → velocidade máxima cai para 25%. Manter a chamada. |
| `analogWrite` no GPIO16 | Lento (escrita extra no registrador RTC por borda) e pode acender o LED onboard da NodeMCU V3. Não usar para enable de driver. |
| Capturar variável de laço por referência em lambda | `rotas.h` captura `[c]` **por valor** (o ponteiro é para `COMANDOS`, que é global). Usar `[&]` com a variável de laço seria *dangling*. |
| HTML fora de `PROGMEM` | Estoura a RAM (80 KB). Usar `send_P`. |
| `String` no loop | Fragmenta o heap da ESP8266. Evitar concatenação repetida; o `server.send` com `String` pontual é aceitável. |
| Mover `TIMEOUT_MS` sem mexer no heartbeat | Se `TIMEOUT_MS` < 250 ms o carrinho engasga andando; se o heartbeat subir acima de `TIMEOUT_MS`, o failsafe dispara constantemente. |
| Esquecer GND comum | Lado dos motores não responde e o driver se comporta de forma errática. |
| Deixar o jumper ENA/ENB no driver **com o GPIO ligado** | É o **único risco real de curto** do projeto: o jumper amarra o pino EN ao plano de **+5V**. GPIO dirigindo LOW → 5V direto na saída de 3.3V → sobrecorrente e dano permanente. Jumper ligado **sem** GPIO no pino é inofensivo (só ignora o PWM). |
| Religar ENB em D3 (GPIO0) | Reintroduz o conflito de boot original. |

---

## 8. Checklist de entrega para o agente

Antes de concluir uma tarefa:

1. [ ] `$LASTEXITCODE -eq 0` no `compile`
2. [ ] Nenhum aviso novo do compilador
3. [ ] Invariantes da seção 3 continuam verdadeiras
4. [ ] Nenhum bug da seção 4 voltou
5. [ ] README.md e AGENTS.md refletem a mudança (se afetar pinagem, rotas,
       comandos ou procedimento de teste)
6. [ ] Se a pinagem mudou: avisar o usuário sobre a **ação de hardware**
       (recolocação de fios) — código sozinho não resolve fiação
