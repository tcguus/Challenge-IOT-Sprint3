# Challenge-IOT
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
- Bibliotecas: `WiFi.h`, `PubSubClient.h`, `ArduinoJson.h`, `Ultrasonic.h`, `DHT.h`

---

## Como Executar
### Wokwi (3 projetos, um por vaga)

Abra as **três simulações**:

- **Vaga A:** https://wokwi.com/projects/431339679872230401
- **Vaga B:** https://wokwi.com/projects/441481087607085057
- **Vaga C:** https://wokwi.com/projects/441481157217856513

Em cada aba do Wokwi:
- Clique **Start**.
- Mova a régua do HC-SR04 e observe os LEDs:
  - **Vermelho (GPIO 2)**: acende quando **ocupada** (≤ ~18–20cm).
  - **Amarelo (GPIO 14)**: **manutenção** (toggle pelo botão no GPIO 13).
- O **buzzer (GPIO 15)** pode ser acionado via **comando MQTT** (ver seção de tópicos).

---

## Requisitos:
- Node-RED instalado
- Paleta: `node-red-dashboard`

### Teste/Dashboard — Node-RED

1. Inicie o Node-RED e acesse `http://localhost:1880`.
2. **Menu → Import →** [este JSON do fluxo](./flow-node-red.json) → **Import → Deploy**.
3. Abra `http://localhost:1880/ui`. Você verá **três colunas** (Vaga A | Vaga B | Vaga C) com:
   - Estado (livre/ocupada)
   - Manutenção (necessária/não necessária)
   - Distância: `XX cm`
   - Temperatura / Umidade
4. O fluxo **persiste** os dados em `parking_data.csv` na pasta onde o Node-RED está rodando.

---

## Tópicos e Payloads

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
