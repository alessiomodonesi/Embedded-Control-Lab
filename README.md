# 📘 Embedded Control Lab – STM32 + ESP32 + Analog Electronics

Questa repository documenta il percorso di apprendimento e i progetti sviluppati nell'ambito del controllo embedded, utilizzando **STM32 Nucleo-F446RE**, **M5StickC Plus2 (ESP32)** e componenti elettronici discreti. L'obiettivo è implementare controllori (PI/PID) su sistemi simulati e reali, esplorando campionamento, interrupt, comunicazione e condizionamento del segnale.

---

## 🛠 Hardware Setup

### STM32

* **Scheda:** STMicroelectronics NUCLEO-F446RE (MCU: STM32F446RET6, 180MHz, Cortex-M4)
* **Connessione:** Cavo USB Type-A/C to **Mini-B**
* **OS:** macOS (Apple Silicon M1/M2/M3)
* **Pinout Reference:** [Link al Datasheet Ufficiale](datasheet.pdf) o vedi la serigrafia sulla scheda.

### ESP32 (M5StickC Plus2)

* **Core:** ESP32-PICO-V3-02 (240MHz dual-core, WiFi/BLE)
* **Display:** LCD 1.14" (135x240)
* **Sensori:** IMU (MPU6886), Microfono
* **Pinout Grove:** G32 (SDA/IO), G33 (SCL/IO) – utilizzabile per I2C/UART/GPIO
* **LED interno:** G19 (attivo basso)
* **Pulsanti:** A (G37), B (G39), C (G35)
* **Alimentazione:** Batteria LiPo integrata, USB-C

### Componenti Elettronici

* Breadboard 830 punti, jumper wire (M-M, M-F, F-F)
* LED, resistenze (220Ω, 1kΩ, 10kΩ), condensatori (100nF, 100µF)
* Transistor NPN (S8050), diodo 1N4007, relè 5V (Omron G5V-1)
* Shift register 74HC595
* Op-amp LM358, microfono a elettrete
* Potenziometro 10kΩ, pulsanti

---

## 💻 Software & IDE

### STM32

* **IDE Principale:** STM32CubeIDE (Eclipse-based)
* **Configuratore:** STM32CubeMX (integrato o standalone)
* **Debugger:** ST-LINK integrato (aggiornare firmware su Mac)

### ESP32

* **Ambiente di sviluppo:** PlatformIO (VS Code) o Arduino IDE
* **Firmware di base:** UIFlow 2.0 / Stock (per test rapidi) oppure codice C++ custom
* **Tool di flashing:** M5Burner (per firmware stock) o `esptool.py` / PlatformIO

### 🐛 Troubleshooting: Menu "STM32 Project" Mancante su macOS

* **Soluzione:** Andare su **File -> New -> Other...** e cercare `STM32` nella barra di ricerca.
* **Alternativa:** Usare **STM32CubeMX** esterno e importare il progetto.

---

## 📂 Struttura della Repository

```text
Embedded_Control_Lab/
├── .gitignore
├── README.md
│
├── stm32/
│   ├── 01_Blink_LED/
│   ├── 02_Button_Input/
│   ├── ...
│   ├── 15_RC_Time_Constant/
│   └── 16_Motor_Control_Sim/
│
├── esp32/
│   ├── 01_Blink_LED/
│   ├── 02_PWM_Dimmer/
│   ├── 03_ADC_Read/
│   ├── 04_UART_Comm/
│   └── 05_Motor_Simulator/
│
└── analog/
    ├── 01_OpAmp_Motor_Model/
    └── ...
```

> **Nota:** Ogni progetto è indipendente. Per STM32, creare la sottocartella con STM32CubeMX selezionando `stm32/` come root. Per ESP32, usare PlatformIO con progetto dentro `esp32/`.

---

## 📝 Snippet di Codice Utili – STM32

Tutto il codice utente va scritto in `Core/Src/main.c` tra i commenti `/* USER CODE BEGIN ... */` e `/* USER CODE END ... */`.

### 1. Lampeggio LED (Blink)

* **Pin:** `PA5` (LD2_Pin)

```c
HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
HAL_Delay(500);
```

### 2. Lettura Pulsante (Digital Input)

* **Pin:** `PC13` (B1_Pin, active low)

```c
if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET)
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
else
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
```

### 3. PWM Breathing LED (Effetto Respiro)

* **Pin:** `PA5` (TIM2_CH1), clock 100 MHz
* **Parametri Timer (1 kHz PWM):** PSC = 99, ARR = 999

```c
HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

for(int duty = 0; duty < 1000; duty += 10) {
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, duty);
    HAL_Delay(10);
}
for(int duty = 1000; duty > 0; duty -= 10) {
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, duty);
    HAL_Delay(10);
}
```

### 4. Interrupt Pulsante (EXTI)

* **Configurazione CubeMX:** `PC13` → GPIO_EXTI13; NVIC abilitare EXTI line[15:10].

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == B1_Pin) {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        for(int i=0; i<100000; i++); // debounce didattico
    }
}
```

### 5. Comunicazione Seriale (UART)

* **Periferica:** USART2, `PA2` (TX), `PA3` (RX), 115200 8N1

```c
#include <string.h>
char msg[] = "Hello from STM32!\r\n";
HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
HAL_Delay(1000);
```

### 6. Internal Temp Sensor (ADC) & Non-Blocking I/O

* **Periferica:** ADC1, canale Temperature Sensor, sampling 480 cicli.
* **Logica:** `HAL_GetTick()` per gestire invio periodico senza bloccare.

```c
#define TS_CAL1_ADDR ((uint16_t*)((uint32_t)0x1FFF7A2C))
#define TS_CAL2_ADDR ((uint16_t*)((uint32_t)0x1FFF7A2E))
uint8_t system_active = 1;
uint32_t last_time_sent = 0;

while (1) {
    if (HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET) {
        system_active = !system_active;
        HAL_Delay(300);
    }
    if (system_active && (HAL_GetTick() - last_time_sent > 1000)) {
        last_time_sent = HAL_GetTick();
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
            uint16_t raw = HAL_ADC_GetValue(&hadc1);
            int32_t cal1 = *TS_CAL1_ADDR;
            int32_t cal2 = *TS_CAL2_ADDR;
            float temp = ((float)(raw - cal1) / (cal2 - cal1)) * 80.0f + 30.0f;
            int p_int = (int)temp;
            int p_dec = (int)((temp - p_int) * 100);
            char msg[50];
            sprintf(msg, "Stato: ON | Temp CPU: %d.%02d C\r\n", p_int, p_dec);
            HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
            HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        }
        HAL_ADC_Stop(&hadc1);
    }
    if (!system_active) HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
}
```

### 7. Real Time Clock (RTC) & Alarm (LSE)

* **Configurazione:** RCC LSE Crystal; RTC Asynchronous Prediv 127, Synchronous Prediv 255; NVIC RTC alarm EXTI17.

```c
RTC_TimeTypeDef sTime = {0};
sTime.Hours = 0x12; sTime.Minutes = 0x00; sTime.Seconds = 0x00;
HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD);

RTC_AlarmTypeDef sAlarm = {0};
sAlarm.AlarmTime.Hours = 0x12;
sAlarm.AlarmTime.Minutes = 0x00;
sAlarm.AlarmTime.Seconds = 0x10;
sAlarm.Alarm = RTC_ALARM_A;
sAlarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;
HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BCD);

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc) {
    char msg[] = "\r\n[ALARM] DRIIIN! SVEGLIA !!!\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
}
```

### 8. External Blink (GPIO Output)

* **Pin:** `PA0`, LED + resistenza 220Ω verso GND.

```c
HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);
HAL_Delay(500);
```

### 9. Il Potenziometro (ADC Input)

* **Pin:** `PA0` (ADC1_IN0), trimmer 10kΩ tra 3.3V e GND, wiper su PA0.

```c
HAL_ADC_Start(&hadc1);
if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
    uint32_t raw = HAL_ADC_GetValue(&hadc1);
    float voltage = (float)raw / 4095.0f * 3.3f;
    int v_int = (int)voltage;
    int v_dec = (int)((voltage - v_int) * 100);
    char msg[50];
    sprintf(msg, "Raw: %4lu | Volt: %d.%02d V\r", raw, v_int, v_dec);
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
}
HAL_ADC_Stop(&hadc1);
HAL_Delay(100);
```

### 10. Pulsante Esterno & Debouncing Hardware

* **Pin:** `PA10` (MY_BTN), pull-down 10kΩ, condensatore 100nF verso GND, pulsante tra 3.3V e PA10.

```c
if (HAL_GPIO_ReadPin(MY_BTN_GPIO_Port, MY_BTN_Pin) == GPIO_PIN_SET)
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
else
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
HAL_Delay(10);
```

#### 🔬 Deep Dive: Visualizzare il Rimbalzo

Rimuovere il condensatore e usare un contatore di fronti di salita per osservare i rimbalzi.

```c
uint8_t stato_precedente = 0;
uint32_t counter = 0;

while (1) {
    uint8_t stato_attuale = HAL_GPIO_ReadPin(MY_BTN_GPIO_Port, MY_BTN_Pin);
    if (stato_attuale == GPIO_PIN_SET && stato_precedente == GPIO_PIN_RESET) {
        counter++;
        char msg[50];
        sprintf(msg, "Segnali rilevati: %lu\r\n", counter);
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 10);
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    }
    stato_precedente = stato_attuale;
}
```

### 11. Transistor come Interruttore (BJT NPN)

* **Pin:** `PA0` → resistenza 1kΩ → base S8050; emettitore a GND; collettore ai catodi dei LED (anodi a 5V).

```c
HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);
HAL_Delay(500);
```

### 12. Controllo Relè & Diodo di Ricircolo

* **Pin:** `PA0` → 1kΩ → base S8050; collettore → bobina relè + anodo diodo 1N4007; catodo diodo a 5V; emettitore a GND.

```c
HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);
HAL_Delay(1000);
```

#### Relè con Toggle e Feedback Esterno

* **Pin:** `PA0` (relè), `PA1` (LED esterno), `PA10` (pulsante).

```c
if (stato_pulsante_att == GPIO_PIN_SET && stato_pulsante_prec == GPIO_PIN_RESET) {
    stato_sistema = !stato_sistema;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, stato_sistema ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, stato_sistema ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_Delay(50);
}
```

### 13. Shift Register (74HC595) – Effetto Supercar

* **Pin:** `PA9` → DS (14), `PC7` → ST_CP (12), `PB6` → SH_CP (11); VCC(16) e MR(10) a 3.3V; GND(8) e OE(13) a GND.

```c
void ShiftOut(uint8_t data) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
    for (int i = 7; i >= 0; i--) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
        if ((data >> i) & 1) HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
        else                 HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    }
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
}

for (int i = 0; i < 8; i++) {
    ShiftOut(1 << i);
    HAL_Delay(100);
}
```

### 14. Orecchio Elettronico (Microfono & Op-Amp LM358)

* **Pin:** `PA0` (ADC1_IN0), configurazione non-invertente, guadagno ≈1000, bias a 1.65V.
* **Risultati:** silenzio ~2000-2050, voce 1200-3000.

```c
HAL_ADC_Start(&hadc1);
while (1) {
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        uint32_t val = HAL_ADC_GetValue(&hadc1);
        char msg[20];
        sprintf(msg, "%lu\r\n", val);
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 10);
    }
    HAL_Delay(1);
}
```

### 15. Costante di Tempo RC (Capacimetro Scientifico)

* **Pin:** `PA0` (sonda), `PA1` (alimentazione); R=10kΩ, C=100µF → τ=1s.

```c
while (1) {
    // Scarica
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    for(int i=0; i<200; i++) {
        uint32_t val = HAL_ADC_GetValue(&hadc1);
        char msg[50];
        sprintf(msg, "0 4095 %lu\r\n", val);
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 10);
        HAL_Delay(50);
    }
    // Carica
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
    for(int i=0; i<200; i++) {
        if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
            uint32_t val = HAL_ADC_GetValue(&hadc1);
            char msg[50];
            sprintf(msg, "0 4095 %lu\r\n", val);
            HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 10);
        }
        HAL_Delay(50);
    }
}
```

---

## 🎛️ Fase 5: Controllo di un Motore DC (PI/PID) – Simulazione con ESP32

### 🎯 Obiettivo

Implementare un controllore PI/PID su STM32 per regolare la velocità di un motore simulato. La simulazione può essere realizzata tramite **ESP32** (modello matematico) oppure con un **circuito analogico** (op-amp). Inizieremo con l'ESP32 per semplicità e debug.

### 🛠 Architettura del Sistema

```text
STM32 (Controllore PID) <--- (Feedback UART) --- ESP32 (Modello Motore)
       |                                              ^
       +------------------ (Comando UART) -------------+
```

> **Nota:** Abbiamo scelto **UART** per la comunicazione perché è più robusta e facile da debugare rispetto a un feedback analogico. In futuro si potrà valutare l'uso del DAC dell'ESP32 (o PWM+filtro) per un feedback analogico.

### 🔌 Setup Hardware e Pinout (STM32F446RE)

* **Comando Motore (Output):** `PA0` (TIM2_CH1 o TIM5_CH1) configurato come **PWM Generation** (per la modalità analogica) oppure **UART TX** (per la modalità seriale).
* **Feedback Motore (Input):** `PA1` (ADC1_IN1) per la modalità analogica, oppure **UART RX** per la modalità seriale.
* **Timer di Campionamento:** `TIM6` o `TIM7` con interrupt ogni **10 ms** (100 Hz).
* **UART:** USART2 (PA2/PA3) per comunicare con ESP32 o PC.

### 📂 Struttura della Cartella

```text
stm32/16_Motor_Control_Sim/
├── Core/
│   ├── Inc/
│   │   └── pid.h
│   └── Src/
│       ├── main.c
│       └── pid.c
└── ...
```

### 💻 Implementazione del Controllore PID (C)

**`pid.h`**

```c
#ifndef PID_H
#define PID_H

typedef struct {
    float Kp, Ki, Kd;
    float tau;              // Costante di tempo filtro derivativo
    float lim_min, lim_max; // Limiti di saturazione
    float T;                // Periodo di campionamento (s)

    float integrator;
    float prev_error;
    float differentiator;
    float prev_measurement;
} PID_Controller;

void PID_Init(PID_Controller *pid, float Kp, float Ki, float Kd, float T, float tau, float lim_min, float lim_max);
float PID_Compute(PID_Controller *pid, float setpoint, float measurement);

#endif
```

**`pid.c`** (anti-windup condizionale, filtro derivativo sulla misura)

```c
#include "pid.h"

void PID_Init(PID_Controller *pid, float Kp, float Ki, float Kd, float T, float tau, float lim_min, float lim_max) {
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->T = T;
    pid->tau = tau;
    pid->lim_min = lim_min;
    pid->lim_max = lim_max;
    pid->integrator = 0.0f;
    pid->prev_error = 0.0f;
    pid->differentiator = 0.0f;
    pid->prev_measurement = 0.0f;
}

float PID_Compute(PID_Controller *pid, float setpoint, float measurement) {
    float error = setpoint - measurement;

    float proportional = pid->Kp * error;

    // Integrale (con anti-windup condizionale)
    float integral = pid->integrator + 0.5f * pid->Ki * pid->T * (error + pid->prev_error);

    // Derivativo (filtro passa-basso sulla misura)
    float derivative = -(2.0f * pid->Kd * (measurement - pid->prev_measurement) +
                         (2.0f * pid->tau - pid->T) * pid->differentiator) /
                       (2.0f * pid->tau + pid->T);

    float output = proportional + integral + derivative;

    // Saturazione e anti-windup condizionale
    if (output > pid->lim_max) {
        output = pid->lim_max;
        integral = pid->integrator; // non integrare oltre
    } else if (output < pid->lim_min) {
        output = pid->lim_min;
        integral = pid->integrator;
    }

    pid->integrator = integral;
    pid->prev_error = error;
    pid->prev_measurement = measurement;
    pid->differentiator = derivative;

    return output;
}
```

### 🔗 Codice di “Colla”: Timer Callback, ADC/UART e PWM

Esempio di callback periodico (100 Hz) che legge il feedback, calcola il PID e aggiorna il comando PWM.

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6) {
        // 1. Leggi feedback (modalità analogica)
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 1);
        uint32_t raw = HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
        float measurement = (float)raw / 4095.0f * 100.0f; // 0-100%

        // 2. Setpoint (es. da potenziometro o fisso)
        float setpoint = 50.0f; // oppure leggi un altro ADC

        // 3. Calcola PID
        float out = PID_Compute(&pid, setpoint, measurement);

        // 4. Applica PWM (0-100% -> 0-ARR)
        uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim2);
        uint32_t duty = (uint32_t)((out / 100.0f) * arr);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, duty);
    }
}
```

> Per la modalità UART, sostituire la lettura ADC con una ricezione UART e l'invio del comando con una trasmissione UART.

### 🤖 Modello del Motore su ESP32 (Primo Ordine)

Esempio di modello semplificato da implementare sull'ESP32.

```cpp
// ESP32 – modello motore del primo ordine
float speed = 0.0f;
float tau = 0.5f;   // costante di tempo (s)
float K = 1.0f;     // guadagno

void update_motor(float command, float dt) {
    // command: 0-1 (duty cycle)
    float target = K * command * 100.0f; // 0-100%
    speed += (target - speed) * (dt / tau);
    // Invia speed allo STM32 via UART (es. Serial2.print(speed))
}
```

### 🧪 Esperimenti Proposti

1. **Open-Loop (Caratterizzazione)**
   * Impostare duty cycle fissi (0%, 25%, 50%, 75%, 100%) e registrare la velocità di feedback.
   * Tracciare la curva **Velocità vs Duty Cycle** e osservare l'eventuale dead zone.

2. **Closed-Loop (PI/PID)**
   * Inizializzare con `Kp=1.0, Ki=0.5, Kd=0.0`, setpoint al 50%.
   * Sintonizzare: prima P, poi I per eliminare l'errore a regime, infine D per smorzare le oscillazioni.
   * Applicare disturbi (es. frenare manualmente) e osservare il ripristino.

3. **Analisi della Saturazione**
   * Impostare setpoint alto (90%) e verificare la saturazione al 100%.
   * Testare l'anti-windup: riportando il setpoint a un valore inferiore, l'uscita deve rientrare rapidamente senza ritardi dovuti all'integrale accumulato.

> **Suggerimento:** Usare il **Serial Plotter** dell'Arduino IDE (o un tool simile) per visualizzare in tempo reale setpoint, misura e uscita del PID.

---

## 🟣 Fase 6: ESP32 – Setup e Controllo Base

L'ESP32 (M5StickC Plus2) viene utilizzato come coprocessore per simulare il motore o per esperimenti di controllo autonomi.

### 6.1 Setup Ambiente

* Installare **PlatformIO** (VS Code) o **Arduino IDE**.
* Selezionare la board `M5Stick-C` (o `esp32dev` con pinout corretto).
* Installare le librerie `M5StickCPlus2` (per display e periferiche) e `Arduino.h`.

### 6.2 Esperimenti Base

* **Blink LED interno (G19):** verifica del toolchain.
* **PWM Dimmer:** generazione PWM su un pin Grove (es. G32) per pilotare un LED esterno.
* **ADC Read:** lettura di un potenziometro su un pin ADC (es. G33) e stampa su seriale.
* **UART Communication:** scambio di dati con STM32 (es. comandi e feedback).

### 6.3 ESP32 come Simulatore di Motore

* Implementare il modello del primo ordine (vedi sopra).
* Ricevere il comando PWM (o valore di duty) via UART.
* Inviare la velocità calcolata via UART allo STM32.
* Aggiungere un pulsante (es. tasto A) per applicare un disturbo (frenata).

### 6.4 Web Control (Opzionale)

* Creare un piccolo web server sull'ESP32 per monitorare setpoint e velocità da smartphone.
* Utile per debug e visualizzazione senza Serial Plotter.

---

## 🔵 Fase 7: Circuito Analogico – Modello di Motore con Op-Amp

### 🎯 Obiettivo

Realizzare un modello fisico di motore DC su breadboard, per affrontare problemi reali (rumore, offset, saturazione) e validare il controllore su un sistema non ideale.

### 🛠 Circuito Proposto

* **Integratore:** op-amp LM358 in configurazione integratore (simula l'inerzia meccanica).
* **Amplificatore:** secondo stadio per scalare il segnale.
* **Ingresso:** comando PWM filtrato (o DAC) dallo STM32.
* **Uscita:** tensione proporzionale alla “velocità”, letta dall'ADC dello STM32.

### 📐 Schema a Blocchi

```text
STM32 PWM → Filtro RC → Integratore (LM358) → Amplificatore → STM32 ADC
```

> **Nota:** La funzione di trasferimento risultante è approximately `1/(sτ+1)`, simile a un motore del primo ordine.

### 🧪 Esperimenti

* Ripetere open-loop e closed-loop sul circuito analogico.
* Confrontare le prestazioni con la simulazione ESP32.
* Analizzare l'effetto del rumore e della saturazione dell'op-amp.

---

## ⚙️ Gestione Git (.gitignore)

```text
# Compilazione
Debug/
Release/
build/
Bin/

# IDE
.settings/
.mxproject
*.launch
*.su
*.d
*.o
*.elf
*.map

# macOS
.DS_Store

# PlatformIO
.pio/
.pioenvs/
.piolibdeps/
```

---

## 🧪 Esperimenti “Stand-alone” (Senza Breadboard) – STM32

Questi esperimenti sfruttano l'hardware integrato della Nucleo per esplorare periferiche avanzate.

1. **Manipolazione del Clock (RCC):** modificare la frequenza della CPU da CubeMX e osservare l'effetto sul blink.
2. **PWM & Breathing LED:** generare PWM su PA5 (TIM2) e variare il duty cycle per fading.
3. **Interrupt Esterni (EXTI):** configurare PC13 in interrupt e gestire il LED in callback.
4. **Comunicazione Seriale (UART):** inviare messaggi al PC tramite USART2.
5. **Sensori Interni (ADC):** leggere temperatura interna e VREFINT.
6. **RTC & Allarme:** configurare RTC con LSE e un allarme dopo 10 s.

---

## 🚀 Roadmap

### ⚪️ Fase 0: On-Board (STM32)

* [x] 0. Setup Ambiente e Firmware Update
* [x] 1. Hello World (Blink LED)
* [x] 2. Clock & Frequenze
* [x] 3. PWM Breathing LED
* [x] 4. Interrupts (EXTI)
* [x] 5. UART Communication
* [x] 6. Internal Temp Sensor (ADC)
* [x] 7. RTC & Alarm

### 🟢 Fase 1: Breadboard Fundamentals (STM32)

* [x] 8. External Blink
* [x] 9. Il Potenziometro
* [x] 10. Pulsante Esterno & Debounce

### 🟡 Fase 2: Potenza & Switching (STM32)

* [x] 11. Transistor come Interruttore
* [x] 12. Relè & Diodo di Ricircolo

### 🟠 Fase 3: Logica Digitale (STM32)

* [x] 13. Shift Register (74HC595)

### 🔴 Fase 4: Analogica Avanzata (STM32)

* [x] 14. Microfono & Op-Amp
* [x] 15. Costante di Tempo RC

### 🟣 Fase 5: Controllo PID (STM32 + ESP32)

* [ ] 16. Motor Control Simulation (ESP32)
  * [ ] Configurare ESP32 come simulatore di motore
  * [ ] Stabilire comunicazione UART tra STM32 ed ESP32
  * [ ] Implementare e testare open-loop
* [ ] 17. Closed-Loop PID (Simulazione)
  * [ ] Integrare modulo PID su STM32
  * [ ] Sintonizzare Kp, Ki, Kd
* [ ] 18. Circuito Analogico (Op-Amp)
  * [ ] Progettare e costruire il modello di motore su breadboard
  * [ ] Ripetere open-loop e closed-loop

### 🔵 Fase 6: ESP32 – Setup e Controllo Base

* [ ] 19. Setup PlatformIO e test LED
* [ ] 20. PWM Dimmer su pin Grove
* [ ] 21. ADC Read (potenziometro)
* [ ] 22. UART Communication con STM32

### 🟤 Fase 7: ESP32 come Simulatore di Motore

* [ ] 23. Implementazione modello primo ordine
* [ ] 24. Ricezione comando e invio feedback via UART
* [ ] 25. Integrazione con PID su STM32

### ⚫️ Fase 8: Circuito Analogico – Modello di Motore

* [ ] 26. Progettazione integratore con LM358
* [ ] 27. Realizzazione su breadboard
* [ ] 28. Validazione con PID

---

## ⚠️ Disclaimer

Gli esperimenti con elettronica di potenza (relè, transistor) e tensioni superiori a 3.3V devono essere eseguiti con cautela. Verificare sempre i collegamenti prima di alimentare il circuito.
