# Challenge-IOT — Monitoramento de Vagas (ESP32 + MQTT + Node-RED)
---

## Objetivo

Simulação de um **sistema IoT** para monitoramento de **três vagas de motos** (vagaA, vagaB, vagaC) com **ESP32**, **sensor ultrassônico HC-SR04** e **DHT22**.  
Cada ESP32 publica **estado** e **telemetria** via **MQTT** para um **Dashboard em Node-RED**, além de acionar **LEDs locais** (ocupação e manutenção) e **buzzer**.

---

## Tecnologias Utilizadas

- ESP32 DevKit v1 (Wokwi)
- HC-SR04 (distância) e DHT22 (temperatura/umidade)
- LEDs (vermelho = ocupação | amarelo = manutenção), botão (toggle manutenção) e buzzer
- MQTT (broker público **broker.hivemq.com**)
- **Node-RED** (dashboard + persistência em CSV)
- Arduino IDE (linguagem C++)  
- Bibliotecas: `WiFi.h`, `PubSubClient.h`, `ArduinoJson.h`, `Ultrasonic.h`, `DHT.h`

---

## Como Executar
### Wokwi (3 projetos, um por vaga)

Crie/duplique **três simulações** e em cada uma ajuste `DEVICE_ID`:

- **Vaga A:** _adicione o link do Wokwi aqui_
- **Vaga B:** _adicione o link do Wokwi aqui_
- **Vaga C:** _adicione o link do Wokwi aqui_

Em cada aba do Wokwi:
- Clique **Start**.
- Mova a régua do HC-SR04 e observe os LEDs:
  - **Vermelho (GPIO 2)**: acende quando **ocupada** (≤ ~18–20cm).
  - **Amarelo (GPIO 14)**: **manutenção** (toggle pelo botão no GPIO 13).
- O **buzzer (GPIO 15)** pode ser acionado via **comando MQTT** (ver seção de tópicos).

---

## Requisitos:
- Node-RED instalado
- Paleta: `node-red-dashboard` (instale via "Manage Palette")

### Teste/Dashboard — Node-RED

1. Inicie o Node-RED e acesse `http://localhost:1880`.
2. **Menu → Import →** cole o fluxo abaixo → **Import → Deploy**.
3. Abra `http://localhost:1880/ui`. Você verá **três colunas** (Vaga A | Vaga B | Vaga C) com:
   - Estado (livre/ocupada)
   - Manutenção (necessária/não necessária)
   - Distância: `XX cm`
   - Temperatura / Umidade
4. O fluxo **persiste** os dados em `parking_data.csv` na pasta onde o Node-RED está rodando.

**Fluxo Node-RED (salve também como `node-red-flow.json` se quiser versionar):**
```json
[
  {
    "id": "tabParking",
    "type": "tab",
    "label": "Challenge-IOT",
    "disabled": false,
    "info": "Dashboard 3 colunas: Vaga A | Vaga B | Vaga C"
  },
  {
    "id": "uiBase",
    "type": "ui_base",
    "theme": {
      "name": "theme-light",
      "lightTheme": {
        "default": "#0094CE",
        "baseColor": "#ffffff",
        "baseFont": "-apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen-Sans, Ubuntu, Cantarell, 'Helvetica Neue', Helvetica, Arial, sans-serif",
        "edited": true
      },
      "darkTheme": {
        "default": "#097479",
        "baseColor": "#222222",
        "baseFont": "-apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen-Sans, Ubuntu, Cantarell, 'Helvetica Neue', Helvetica, Arial, sans-serif"
      }
    },
    "site": {
      "name": "Challenge-IOT",
      "hideToolbar": "false",
      "allowSwipe": "true",
      "dateFormat": "DD/MM/YYYY",
      "sizes": { "sx": 48, "sy": 48, "gx": 6, "gy": 6, "cx": 6, "cy": 6, "px": 0, "py": 0 }
    },
    "locale": "pt-BR",
    "copy": "false",
    "title": "",
    "favicon": "true",
    "x": 140,
    "y": 20,
    "z": "tabParking",
    "wires": []
  },
  { "id": "uiTab", "type": "ui_tab", "name": "Painel", "icon": "dashboard", "disabled": false, "hidden": false, "base": "uiBase" },

  { "id": "grpA", "type": "ui_group", "name": "Vaga A", "tab": "uiTab", "order": 1, "disp": true, "width": "4", "collapse": false },
  { "id": "grpB", "type": "ui_group", "name": "Vaga B", "tab": "uiTab", "order": 2, "disp": true, "width": "4", "collapse": false },
  { "id": "grpC", "type": "ui_group", "name": "Vaga C", "tab": "uiTab", "order": 3, "disp": true, "width": "4", "collapse": false },

  {
    "id": "mqttBroker",
    "type": "mqtt-broker",
    "name": "HiveMQ",
    "broker": "broker.hivemq.com",
    "port": "1883",
    "clientid": "node-red-dashboard",
    "usetls": false,
    "compatmode": true,
    "keepalive": "60",
    "cleansession": true,
    "protocolVersion": "4",
    "birthTopic": "",
    "birthQos": "0",
    "birthPayload": "",
    "closeTopic": "",
    "closeQos": "0",
    "closePayload": "",
    "willTopic": "",
    "willQos": "0",
    "willPayload": ""
  },
  { "id": "inState", "type": "mqtt in", "z": "tabParking", "name": "States", "topic": "mottu/v1/parking/+/state", "qos": "1", "datatype": "auto", "broker": "mqttBroker", "nl": false, "rap": true, "rh": true, "x": 130, "y": 120, "wires": [["jsonState"]] },
  { "id": "inTel", "type": "mqtt in", "z": "tabParking", "name": "Telemetry", "topic": "mottu/v1/parking/+/telemetry", "qos": "1", "datatype": "auto", "broker": "mqttBroker", "nl": false, "rap": false, "rh": false, "x": 130, "y": 180, "wires": [["jsonTel"]] },

  { "id": "jsonState", "type": "json", "z": "tabParking", "name": "JSON state", "property": "payload", "action": "", "pretty": false, "x": 330, "y": 120, "wires": [["routeState", "persist"]] },
  { "id": "jsonTel", "type": "json", "z": "tabParking", "name": "JSON telemetry", "property": "payload", "action": "", "pretty": false, "x": 330, "y": 180, "wires": [["routeTel", "persist"]] },

  {
    "id": "routeState",
    "type": "switch",
    "z": "tabParking",
    "name": "State → vaga",
    "property": "payload.deviceId",
    "propertyType": "msg",
    "rules": [{ "t": "eq", "v": "vagaA", "vt": "str" }, { "t": "eq", "v": "vagaB", "vt": "str" }, { "t": "eq", "v": "vagaC", "vt": "str" }],
    "checkall": "true",
    "repair": false,
    "outputs": 3,
    "x": 520,
    "y": 120,
    "wires": [["fmtStateA"], ["fmtStateB"], ["fmtStateC"]]
  },
  {
    "id": "routeTel",
    "type": "switch",
    "z": "tabParking",
    "name": "Telemetry → vaga",
    "property": "payload.deviceId",
    "propertyType": "msg",
    "rules": [{ "t": "eq", "v": "vagaA", "vt": "str" }, { "t": "eq", "v": "vagaB", "vt": "str" }, { "t": "eq", "v": "vagaC", "vt": "str" }],
    "checkall": "true",
    "repair": false,
    "outputs": 3,
    "x": 540,
    "y": 180,
    "wires": [["fmtTelA"], ["fmtTelB"], ["fmtTelC"]]
  },

  { "id": "fmtStateA", "type": "function", "z": "tabParking", "name": "state A", "func": "const p=msg.payload; msg.state=p.status; msg.maintenance=p.maintenance?'necessária':'não necessária'; return msg;", "outputs": 1, "noerr": 0, "x": 730, "y": 100, "wires": [["uiStateA","uiMaintA"]] },
  { "id": "fmtStateB", "type": "function", "z": "tabParking", "name": "state B", "func": "const p=msg.payload; msg.state=p.status; msg.maintenance=p.maintenance?'necessária':'não necessária'; return msg;", "outputs": 1, "noerr": 0, "x": 730, "y": 120, "wires": [["uiStateB","uiMaintB"]] },
  { "id": "fmtStateC", "type": "function", "z": "tabParking", "name": "state C", "func": "const p=msg.payload; msg.state=p.status; msg.maintenance=p.maintenance?'necessária':'não necessária'; return msg;", "outputs": 1, "noerr": 0, "x": 730, "y": 140, "wires": [["uiStateC","uiMaintC"]] },

  { "id": "fmtTelA", "type": "function", "z": "tabParking", "name": "telemetry A", "func": "const p=msg.payload; msg.distance=p.distance_cm; msg.temp=p.temp_c; msg.hum=p.hum_pct; return msg;", "outputs": 1, "noerr": 0, "x": 740, "y": 180, "wires": [["uiDistA","uiTempA","uiHumA"]] },
  { "id": "fmtTelB", "type": "function", "z": "tabParking", "name": "telemetry B", "func": "const p=msg.payload; msg.distance=p.distance_cm; msg.temp=p.temp_c; msg.hum=p.hum_pct; return msg;", "outputs": 1, "noerr": 0, "x": 740, "y": 220, "wires": [["uiDistB","uiTempB","uiHumB"]] },
  { "id": "fmtTelC", "type": "function", "z": "tabParking", "name": "telemetry C", "func": "const p=msg.payload; msg.distance=p.distance_cm; msg.temp=p.temp_c; msg.hum=p.hum_pct; return msg;", "outputs": 1, "noerr": 0, "x": 740, "y": 260, "wires": [["uiDistC","uiTempC","uiHumC"]] },

  { "id": "uiStateA", "type": "ui_text", "z": "tabParking", "group": "grpA", "order": 1, "width": 0, "height": 0, "name": "Estado A", "label": "Estado", "format": "{{msg.state}}", "layout": "row-spread", "x": 950, "y": 80, "wires": [] },
  { "id": "uiMaintA", "type": "ui_text", "z": "tabParking", "group": "grpA", "order": 2, "width": 0, "height": 0, "name": "Manutenção A", "label": "Manutenção", "format": "{{msg.maintenance}}", "layout": "row-spread", "x": 970, "y": 100, "wires": [] },
  { "id": "uiDistA", "type": "ui_text", "z": "tabParking", "group": "grpA", "order": 3, "width": 0, "height": 0, "name": "Dist A", "label": "Distância", "format": "Distância: {{msg.distance}} cm", "layout": "row-spread", "x": 960, "y": 140, "wires": [] },
  { "id": "uiTempA", "type": "ui_text", "z": "tabParking", "group": "grpA", "order": 4, "width": 0, "height": 0, "name": "Temp A", "label": "Temperatura", "format": "{{msg.temp}} °C", "layout": "row-spread", "x": 960, "y": 160, "wires": [] },
  { "id": "uiHumA", "type": "ui_text", "z": "tabParking", "group": "grpA", "order": 5, "width": 0, "height": 0, "name": "Umid A", "label": "Umidade", "format": "{{msg.hum}} %", "layout": "row-spread", "x": 950, "y": 180, "wires": [] },

  { "id": "uiStateB", "type": "ui_text", "z": "tabParking", "group": "grpB", "order": 1, "width": 0, "height": 0, "name": "Estado B", "label": "Estado", "format": "{{msg.state}}", "layout": "row-spread", "x": 950, "y": 260, "wires": [] },
  { "id": "uiMaintB", "type": "ui_text", "z": "tabParking", "group": "grpB", "order": 2, "width": 0, "height": 0, "name": "Manutenção B", "label": "Manutenção", "format": "{{msg.maintenance}}", "layout": "row-spread", "x": 970, "y": 280, "wires": [] },
  { "id": "uiDistB", "type": "ui_text", "z": "tabParking", "group": "grpB", "order": 3, "width": 0, "height": 0, "name": "Dist B", "label": "Distância", "format": "Distância: {{msg.distance}} cm", "layout": "row-spread", "x": 960, "y": 320, "wires": [] },
  { "id": "uiTempB", "type": "ui_text", "z": "tabParking", "group": "grpB", "order": 4, "width": 0, "height": 0, "name": "Temp B", "label": "Temperatura", "format": "{{msg.temp}} °C", "layout": "row-spread", "x": 960, "y": 340, "wires": [] },
  { "id": "uiHumB", "type": "ui_text", "z": "tabParking", "group": "grpB", "order": 5, "width": 0, "height": 0, "name": "Umid B", "label": "Umidade", "format": "{{msg.hum}} %", "layout": "row-spread", "x": 950, "y": 360, "wires": [] },

  { "id": "uiStateC", "type": "ui_text", "z": "tabParking", "group": "grpC", "order": 1, "width": 0, "height": 0, "name": "Estado C", "label": "Estado", "format": "{{msg.state}}", "layout": "row-spread", "x": 950, "y": 440, "wires": [] },
  { "id": "uiMaintC", "type": "ui_text", "z": "tabParking", "group": "grpC", "order": 2, "width": 0, "height": 0, "name": "Manutenção C", "label": "Manutenção", "format": "{{msg.maintenance}}", "layout": "row-spread", "x": 970, "y": 460, "wires": [] },
  { "id": "uiDistC", "type": "ui_text", "z": "tabParking", "group": "grpC", "order": 3, "width": 0, "height": 0, "name": "Dist C", "label": "Distância", "format": "Distância: {{msg.distance}} cm", "layout": "row-spread", "x": 960, "y": 500, "wires": [] },
  { "id": "uiTempC", "type": "ui_text", "z": "tabParking", "group": "grpC", "order": 4, "width": 0, "height": 0, "name": "Temp C", "label": "Temperatura", "format": "{{msg.temp}} °C", "layout": "row-spread", "x": 960, "y": 520, "wires": [] },
  { "id": "uiHumC", "type": "ui_text", "z": "tabParking", "group": "grpC", "order": 5, "width": 0, "height": 0, "name": "Umid C", "label": "Umidade", "format": "{{msg.hum}} %", "layout": "row-spread", "x": 950, "y": 540, "wires": [] },

  { "id": "persist", "type": "file", "z": "tabParking", "name": "CSV", "filename": "parking_data.csv", "appendNewline": true, "createDir": true, "overwriteFile": "false", "encoding": "none", "x": 330, "y": 240, "wires": [[]] }
]
```

---

## Tópicos e Payloads (padrão)

Base dos tópicos:
```
mottu/v1/parking/<deviceId>/...
```

- **status** (LWT, retain): `"online"` / `"offline"`  
- **state** (retain, QoS1):
  ```json
  { "deviceId":"vagaA", "status":"ocupada", "maintenance": false, "ts": 1699999999 }
  ```
- **telemetry** (QoS1):
  ```json
  { "deviceId":"vagaA", "distance_cm": 12.3, "temp_c": 27.5, "hum_pct": 60.2, "rssi": -58, "maintenance": false, "ts": 1699999999 }
  ```
- **cmd**:
  ```json
  { "led":"on", "buzzer":"off" }
  ```

---

## Casos de Teste 

- **Ocupada**: ≤ ~18–20cm → LED vermelho aceso → `state=ocupada`.
- **Livre**: ≥ ~30cm → LED vermelho apagado → `state=livre`.
- **Ruído**: oscile 18–22cm → estado estável (histerese + média móvel).
- **Manutenção**: botão (GPIO 13) → LED amarelo ON/OFF e `maintenance` alterna.
- **Comando remoto**: publique em `.../cmd` com `{"buzzer":"on"}` → buzzer toca.
- **Disponibilidade (LWT)**: feche uma aba do Wokwi → `status=offline` daquela vaga.

---

## Vídeo Pitch
[Assitir no YouTube]()

---

## Nossos integrantes
- **Gustavo Camargo de Andrade**
- RM555562
- 2TDSPF
-------------------------------------------
- **Rodrigo Souza Mantovanello**
- RM555451
- 2TDSPF
-------------------------------------------
- **Leonardo Cesar Rodrigues Nascimento**
- RM558373
- 2TDSPF
