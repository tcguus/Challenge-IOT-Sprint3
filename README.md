# Challenge-IOT

## Objetivo

Simulação de um sistema **IoT** para monitoramento de **três vagas de motos** (`vagaA`, `vagaB`, `vagaC`) utilizando **ESP32**, sensor ultrassônico **HC-SR04** e **DHT22**.

Cada ESP32 publica **estado** e **telemetria via MQTT** para um **dashboard em Node-RED**, além de acionar **LEDs locais** (ocupação e manutenção) e **buzzer**.

---

## Tecnologias Utilizadas

* **ESP32 DevKit v1 (Wokwi)**
* **Sensores:**

  * **HC-SR04** (distância)
  * **DHT22** (temperatura / umidade)
* **Atuadores:**

  * **LEDs**

    * **Vermelho** → ocupação
    * **Amarelo** → manutenção
  * **Botão** (toggle manutenção)
  * **Buzzer**
* **MQTT:** broker público `broker.hivemq.com`
* **Node-RED:** dashboard, alertas em tempo real e persistência em **SQLite**
* **Bibliotecas (ESP32):** `WiFi.h`, `PubSubClient.h`, `ArduinoJson.h`, `Ultrasonic.h`, `DHT.h`
* **Paleta Node-RED:** `node-red-dashboard`, `node-red-contrib-better-sqlite3`

---

## Como Executar

### 1. Wokwi (3 projetos — um por vaga)

Abra as três simulações:

* **Vaga A:** [https://wokwi.com/projects/431339679872230401](https://wokwi.com/projects/431339679872230401)
* **Vaga B:** [https://wokwi.com/projects/441481087607085057](https://wokwi.com/projects/441481087607085057)
* **Vaga C:** [https://wokwi.com/projects/441481157217856513](https://wokwi.com/projects/441481157217856513)

**Em cada aba do Wokwi:**

1. Clique **Start**.
2. Mova a régua do sensor **HC-SR04** e observe os LEDs:

   * **LED Vermelho (GPIO 2):** acende quando a vaga está **ocupada** (≤ ~18–20 cm de distância medida).
   * **LED Amarelo (GPIO 14):** indica **manutenção**. Esse estado alterna pelo botão no **GPIO 13**.
3. O **buzzer (GPIO 15)** pode ser acionado via comando MQTT (ver seção **Tópicos e Payloads**).

---

### 2. Teste / Dashboard — Node-RED

1. Inicie o **Node-RED** e acesse `http://localhost:1880`.
2. Menu → **Import** (ou "Import > Clipboard") e importe o arquivo `flow-node-red.json` deste repositório.
3. Clique em **Deploy (Implementar)**.

O fluxo criará automaticamente a base de dados **SQLite** `parking.db` na pasta onde o Node-RED está rodando.

4. Acesse `http://localhost:1880/ui`.

Você verá o dashboard completo com duas seções principais:

* **Telemetria detalhada** (Vaga A, Vaga B, Vaga C): mostra distância, temperatura, umidade, RSSI, etc.
* **Visão Geral do Pátio (grid):** mostra o status de cada vaga em tempo real (**Livre**, **Ocupada**, **Manutenção**).

---

## Persistência de Dados

O fluxo Node-RED persiste todos os dados de **estado** e **telemetria** em um banco de dados **SQLite** (`parking.db`).

Inclui:

* Ocupação da vaga
* Flag de manutenção
* Distância medida pelo HC-SR04
* Temperatura e umidade do DHT22
* RSSI (intensidade do sinal Wi-Fi)
* Timestamp (`ts`)

---

## Tópicos e Payloads

### Base dos tópicos

```text
mottu/v1/parking/<deviceId>/...
```

Onde `<deviceId>` pode ser: `vagaA`, `vagaB`, `vagaC`.

---

### `status` (LWT, retain)

Payload:

```text
"online"
"offline"
```

Usado como **Last Will and Testament (LWT)** do MQTT para indicar disponibilidade do dispositivo.

---

### `state` (retain, QoS1)

Exemplo:

```json
{
  "deviceId": "vagaA",
  "status": "ocupada",
  "maintenance": false,
  "ts": 1699999999
}
```

Campos:

* **status:** `"ocupada"` ou `"livre"`
* **maintenance:** `true` / `false`
* **ts:** timestamp Unix

---

### `telemetry` (QoS1)

Exemplo:

```json
{
  "deviceId": "vagaA",
  "distance_cm": 12.3,
  "temp_c": 27.5,
  "hum_pct": 60.2,
  "rssi": -58,
  "maintenance": false,
  "ts": 1699999999
}
```

Campos:

* **distance_cm:** distância medida pelo HC-SR04
* **temp_c:** temperatura em °C (DHT22)
* **hum_pct:** umidade relativa em %
* **rssi:** intensidade do sinal Wi-Fi
* **maintenance:** estado de manutenção
* **ts:** timestamp Unix

---

### `cmd`

Tópico de comando recebido pelo ESP32.

Exemplo:

```json
{ "led": "on", "buzzer": "off" }
```

Possibilidades:

* **led:** `"on"` / `"off"`
* **buzzer:** `"on"` / `"off"` (quando `"on"`, o buzzer toca)

---

## Casos de Teste

### Ocupada

* Distância medida: **≤ ~18–20 cm**
* LED vermelho: **aceso**
* `state.status = "ocupada"`
* No dashboard (grid "Pátio"): vaga **Ocupada** (vermelho)

### Livre

* Distância medida: **≥ ~30 cm**
* LED vermelho: **apagado**
* `state.status = "livre"`
* No dashboard (grid "Pátio"): vaga **Livre** (verde)

### Ruído (histerese)

* Se a distância oscila entre **18–22 cm**, o sistema evita trocas rápidas de estado
* Usa **histerese + média móvel** para estabilidade

### Manutenção

* Pressionar o botão no **GPIO 13** alterna o modo manutenção
* LED amarelo acende/apaga
* Campo `maintenance` alterna entre `true` / `false`

### Alerta de Manutenção

* Ao ativar manutenção:

  * Aparece um **alerta popup (toast)** no dashboard
  * A vaga correspondente no grid "Pátio" fica **amarela e piscando**

### Comando Remoto

* Publicar em `.../cmd` com, por exemplo:

```json
{ "buzzer": "on" }
```

* Resultado: o **buzzer** toca na vaga correspondente

### Disponibilidade (LWT)

* Ao fechar uma aba do Wokwi (desligar um ESP32 simulado):

  * O tópico `status` daquela vaga muda para `"offline"`
  * O dashboard pode indicar que a vaga está fora do ar

---

## Vídeo Pitch

**ASSISTIR VÍDEO DA ENTREGA FINAL NO YOUTUBE**
(Insira aqui o link do vídeo de apresentação)

---

## Integrantes

**Gustavo Camargo de Andrade**
RM555562
2TDSPF

**Rodrigo Souza Mantovanello**
RM555451
2TDSPF

**Leonardo Cesar Rodrigues Nascimento**
RM558373
2TDSPF
