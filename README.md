# Smart Emergency Response System  
### For Accident Victims — Vital Sign Monitoring And Data Transmission

Professional **ESP32-WROOM** embedded healthcare monitoring platform for accident victims and emergency response. The system continuously measures vital signs, shows them on a TFT dashboard, tracks GPS location, and sends GSM SMS alerts to caregivers.

---

## Developer

| | |
|---|---|
| **Name** | Engr. **Hanif Khan** |
| **Title** | Embedded Systems Engineer |
| **Contact** | **0311-8221686** |
| **WhatsApp / Call** | +92 311 8221686 |

---

## Project overview

This project implements a portable smart emergency response unit that can be used at accident sites or for continuous patient monitoring. At power-on, the LCD splash screen displays:

1. Smart Emergency Response System  
2. For Accident Victims  
3. Vital Sign Monitoring  
4. And Data Transmission  

---

## Features

- Live TFT dashboard (HX8357D) with vitals and ECG waveform  
- Body temperature in **°C and °F** (DS18B20)  
- Heart rate & SpO2 (MAX30102) + PulseSensor Amped  
- ECG monitoring (AD8232) with lead-off detection  
- GPS location (NEO-6M) for emergency coordinates  
- GSM SMS alerts (SIM800L) with vitals + location  
- Serial / SMS remote command: `CHECK` / `STATUS` → full multi-module report (4-part SMS)  
- Modular C++ firmware designed for ESP32-WROOM-32  

---

## Hardware

| Module | Role |
|--------|------|
| ESP32-WROOM-32 | Main controller |
| HX8357D TFT | Patient / emergency dashboard |
| DS18B20 | Body temperature (°C / °F) |
| MAX30102 | Heart rate & SpO2 |
| PulseSensor Amped | Optical pulse backup |
| AD8232 | ECG front-end |
| NEO-6M | GPS location |
| SIM800L | Emergency SMS |

### ESP32 pin map (summary)

| Function | GPIO |
|----------|------|
| TFT CS / DC / RST | 5 / 2 / 4 |
| TFT MOSI / SCK | 23 / 18 |
| MAX30102 I2C SDA / SCL | 21 / 22 |
| DS18B20 OneWire | 25 (+ 4.7 kΩ pull-up) |
| Pulse / ECG | 34 / 35 |
| GPS RX / TX | 17 / 16 |
| GSM RX / TX | 14 / 15 |

---

## How to build & upload

1. Install **Arduino IDE** + **ESP32** board package  
2. Install libraries:  
   - Adafruit GFX  
   - Adafruit HX8357  
   - OneWire  
   - DallasTemperature  
3. Open `Smart_Healthcare_System/Smart_Healthcare_System.ino`  
4. Board: **ESP32 Dev Module**  
5. Serial Monitor: **115200** baud  
6. Upload  

### Remote status commands

| Source | Command | Result |
|--------|---------|--------|
| Serial Monitor | `CHECK` / `STATUS` / `DATA` | Full report of all modules |
| SMS to device SIM | same words | 4 SMS reply with all sensor + GPS + GSM data |

---

## Emergency SMS

Alert / report number configured in firmware: **+92 311 8221686** (Engr. Hanif Khan).

---

## Repository structure

```
smart_heath_monitoring/
├── README.md
├── .gitignore
└── Smart_Healthcare_System/
    ├── Smart_Healthcare_System.ino
    ├── Config.h
    ├── TFT_Display.*
    ├── DS18B20_Module.*
    ├── MAX30102_Module.*
    ├── PulseSensor_Module.*
    ├── ECG_Module.*
    ├── GPS_Module.*
    ├── GSM_Module.*
    ├── Emergency.*
    └── Command_Module.*
```

---

## License & contact

Developed for academic / emergency-response demonstration use.

**Engr. Hanif Khan** — Embedded Systems Engineer  
Phone / WhatsApp: **0311-8221686**
