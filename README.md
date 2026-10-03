# Carrinho 4WD — controle por Wi-Fi

Carrinho robótico de 4 rodas controlado pelo celular ou computador,
sem precisar de internet. O **NodeMCU ESP8266** cria a própria rede Wi-Fi e
serve uma interface de controle no navegador.

```
┌──────────────┐      Wi-Fi (AP)       ┌──────────────┐
│  Celular /   │ ────────────────────► │  NodeMCU     │      ┌─────────────┐
│  Navegador   │   192.168.4.1         │  ESP8266     │ ───► │ Driver      │
│              │ ◄──────────────────── │              │ PWM  │ L298N/etc   │ ──► 4 motores
└──────────────┘      resposta         └──────────────┘      └─────────────┘
```

---

## Como funciona

1. O ESP8266 sobe um **ponto de acesso** chamado `Carrinho_4WD`.
2. Você conecta o celular nessa rede e abre `192.168.4.1` (ou `carrinho.local`).
3. A interface mostra uma **cruz de 4 setas** e um **slider de velocidade**.
4. Segurando uma seta, o navegador reenvia o comando **a cada 250 ms**.
5. Se nenhum comando chegar em **600 ms**, o carrinho **para sozinho**
   (*failsafe*) — cobre Wi-Fi caindo, navegador fechando ou página travando.
6. Soltar o dedo já dispara o comando de parada.

### Rede

| Item | Valor |
|---|---|
| SSID | `Carrinho_4WD` |
| Senha | `12345678` |
| IP | `192.168.4.1` |
| Hostname | `carrinho.local` (mDNS) |

---

## Hardware

- **NodeMCU ESP8266** (ESP-12E) — placa usada para desenvolver e testar
- **Driver de motores** com entradas `IN1..IN4` e enables `ENA`/`ENB`
  (L298N ou equivalente)
- **4 motores CC** de 12V (ou a tensão da sua bateria) — 2 por lado
- Bateria / fonte para os motores
- Fios

> **ESP8266 não tolera 5V.** Os jumpers `ENA`/`ENB` do driver **devem estar
> removidos**, senão os 5V do driver vão direto para o GPIO e queimam a placa.
> O enable precisa ser alimentado pelos 3,3V do próprio NodeMCU.

### Pinagem

| Função | Pino | GPIO | Observação |
|---|---|---|---|
| IN1 — motor esquerdo | **D1** | GPIO5 | Pino mais seguro da placa |
| IN2 — motor esquerdo | **D2** | GPIO4 | Pino mais seguro da placa |
| IN3 — motor direito | **D5** | GPIO14 | |
| IN4 — motor direito | **D6** | GPIO12 | |
| ENA — PWM esquerdo | **D7** | GPIO13 | Velocidade do lado esquerdo |
| ENB — PWM direito | **D8** | GPIO15 | Velocidade do lado direito |

**Lado esquerdo** = IN1 + IN2 + ENA · **Lado direito** = IN3 + IN4 + ENB

Pinos que **não podem** ser usados:

| Pino | Motivo |
|---|---|
| GPIO6–GPIO11 | Ligados à memória flash — uso causa crash |
| **TX** (GPIO1) / **RX** (GPIO3) | Ocupados pela Serial e pelo chip USB-serial |
| **A0** (ADC0) | Só entrada analógica — não gera PWM |
| **D0** (GPIO16) | PWM lento (registrador RTC), nível baixo medido de ~1V e LED onboard na NodeMCU V3 |

---

## Como fazer funcionar

### 1. Instalar

- [Arduino IDE](https://www.arduino.cc/en/software) (v2.x)
- No IDE: **Arquivo → Preferências → URLs adicionais de Gerenciador de Placas**,
  cole:

  ```
  https://arduino.esp8266.com/stable/package_esp8266com_index.json
  ```

- **Ferramentas → Placa → Gerenciador de Placas** → instale
  **esp8266 by ESP8266 Community** (este projeto foi compilado na **3.1.2**)
- **Ferramentas → Placa → esp8266 → NodeMCU 1.0 (ESP-12E Module)**

### 2. Ligar os fios

Siga a tabela de pinagem acima. Confira:

- [ ] Pino de enable direito do driver ligado em **D8 (GPIO15)** — *não em D3*
- [ ] Jumpers `ENA`/`ENB` do driver **removidos**
- [ ] **GND comum** entre NodeMCU e driver (obrigatório)
- [ ] Alimentação dos motores na fonte/bateria do driver, **não** no NodeMCU

### 3. Gravar

Abra `4wd.ino` no Arduino IDE, selecione a placa e a porta, clique em **→**.

Via terminal (arduino-cli):

```powershell
$cli = "C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"

# Compilar (não precisa da placa conectada)
& $cli compile --fqbn esp8266:esp8266:nodemcuv2 "C:\Users\04513901055\Documents\Arduino\4wd"

# Descobrir a porta
& $cli board list

# Gravar (substitua COMx)
& $cli upload --fqbn esp8266:esp8266:nodemcuv2 -p COMx "C:\Users\04513901055\Documents\Arduino\4wd"
```

### 4. Usar

1. No celular: **Wi-Fi → conectar em `Carrinho_4WD`**, senha `12345678`.
2. Abrir **`http://192.168.4.1`** (ou `http://carrinho.local`).
3. **Segure** uma seta para andar. **Solte** para parar.
4. O pontinho verde confirma conexão; vermelho = desconectado.

> **Dica:** no iPhone/Android, se a rede não tiver internet o sistema às vezes
> desconecta sozinho. Ative *"usar dados móveis simultaneamente"* ou conecte
> pela rede 5 GHz/2,4 GHz do celular em modo manual.

---

## Controles

```
          ▲  frente
          │
   ◄ ─────┼───── ►   esquerda / direita = gira no próprio eixo
          │
          ▼  trás
```

| Ação | Resultado |
|---|---|
| Segurar ▲ | Anda para frente (ambos os lados) |
| Segurar ▼ | Ré |
| Segurar ◄ | Gira à esquerda: lado esquerdo anda de ré, direito para frente |
| Segurar ► | Gira à direita |
| Soltar | Para |
| Slider | Velocidade de 10% a 100% (PWM 0–255) |

---

## Comandos HTTP

Todos `GET`. Útil para testar no terminal ou integrar com outra coisa.

| Rota | Efeito | Resposta |
|---|---|---|
| `/` | Interface web | HTML |
| `/frente` | Anda para frente | `OK` |
| `/tras` | Ré | `OK` |
| `/esquerda` | Gira à esquerda | `OK` |
| `/direita` | Gira à direita | `OK` |
| `/parar` | Para e desarma o failsafe | `OK` |
| `/speed?v=0..255` | Define a velocidade | valor atual |
| `/ping` | Keep-alive da interface | `pong` |

```powershell
Invoke-RestMethod "http://192.168.4.1/frente"
Invoke-RestMethod "http://192.168.4.1/speed?v=180"
Invoke-RestMethod "http://192.168.4.1/parar"
```

---

## Estrutura do código

O projeto é dividido em 5 arquivos — cada um com uma responsabilidade:

| Arquivo | Responsabilidade |
|---|---|
| `config.h` | Pinagem, Wi-Fi, constantes e a macro de log `dbg()` |
| `motores.h` | Tabela de comandos + `mover()` + `iniciarMotores()` |
| `rotas.h` | Registro das rotas HTTP |
| `web.h` | Interface HTML/CSS/JS (`INDEX_HTML` em PROGMEM) |
| `4wd.ino` | `setup()` e `loop()` |

**Para entender como o carrinho anda, leia `motores.h` inteiro** — são ~75
linhas e toda a lógica de movimento está lá.

---

## Ajustes rápidos

Tudo o que costuma mudar está em `config.h`:

| Quero... | Onde |
|---|---|
| Mudar nome/senha do Wi-Fi | `WIFI_SSID`, `WIFI_SENHA` |
| Mudar o hostname | `MDNS_NOME` |
| Mudar o tempo do failsafe | `TIMEOUT_MS` (padrão 600) |
| Mudar a velocidade máxima | `PWM_MAX` (padrão 255) |
| Tirar o zumbido do motor | `PWM_FREQ_HZ` — já está em 20000 |
| Desligar os logs | `#define DEBUG 0` |

Para **remover os logs por completo** (economiza flash), troque `DEBUG` para `0`.

---

## Solução de problemas

| Sintoma | Causa provável |
|---|---|
| Não sobe do flash / entra em modo de gravação sozinho | Algo puxando **GPIO0 (D3)** para baixo — confira se o fio saiu de D3 |
| Anda sempre em velocidade máxima | Jumper `ENA`/`ENB` colocados no driver (PWM ignorado) |
| Um lado não anda | Confira IN1/IN2 (D1/D2) ou IN3/IN4 (D5/D6); confira GND comum |
| Anda ao contrário | Inverta os fios de um dos motores **ou** inverta os sinais na tabela `COMANDOS[]` |
| Para sozinho depois de ~0,6 s | Failsafe normal: falta o heartbeat — veja se a página está aberta |
| LED vermelho "Desconectado" | Celular saiu da rede `Carrinho_4WD` |
| Zumbido no motor | `PWM_FREQ_HZ` baixo — use 20000 |
| Carrega e reinicia ao acelerar | Fonte/bateria fraca — os motores puxam a 3,3V |

### Monitor serial

Ferramentas → Monitor Serial → **115200 baud**. Você verá:

```
[WiFi] SSID: Carrinho_4WD
[WiFi] IP:   192.168.4.1
[mDNS] carrinho.local ativo
[HTTP] Servidor iniciado
[MOTOR] FRENTE      Esq: +255  Dir: +255
[FAILSAFE] Timeout de comando acionado
```

---

## Memória (compilado na 3.1.2 / NodeMCU 1.0)

| Segmento | Uso | Limite |
|---|---|---|
| RAM (globais) | 29 212 B | 80 192 B (36%) |
| Flash | 301 088 B | 1 048 576 B (28%) |
| IRAM | 61 659 B | 65 536 B (94%) |

A interface HTML fica em **PROGMEM** (flash), não na RAM — por isso a RAM
ficou em 36%. Se você adicionar HTML, mantenha `PROGMEM`.
