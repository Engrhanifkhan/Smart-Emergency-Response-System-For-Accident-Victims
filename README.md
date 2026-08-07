# Smart Emergency Response System  
## For Accident Victims — Vital Sign Monitoring And Data Transmission

ESP32-WROOM patient / accident-victim monitoring system with live TFT dashboard, sensors, GPS location, and GSM SMS alerts.

### Boot screen project name (LCD)
1. Smart Emergency Response System  
2. For Accident Victims  
3. Vital Sign Monitoring  
4. And Data Transmission  

### Hardware
- ESP32-WROOM-32
- HX8357D TFT display
- MAX30102 (HR / SpO2)
- DS18B20 (body temperature °C / °F)
- PulseSensor Amped
- AD8232 ECG
- NEO-6M GPS
- SIM800L GSM

### Features
- Live vitals dashboard on TFT
- Emergency SMS with vitals + GPS
- Serial / SMS command `CHECK` or `STATUS` for full multi-module report
- Temperature shown in Celsius and Fahrenheit

### Open in Arduino IDE
Open `Smart_Healthcare_System/Smart_Healthcare_System.ino`  
Board: **ESP32 Dev Module** · Serial: **115200**
