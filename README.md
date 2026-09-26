Real-Time Multisensor Room Monitoring SystemProject Overview

This project is a simulated ESP32 room-monitoring system built from scratch using PlatformIO and the ESP-IDF FreeRTOS framework. It monitors temperature, humidity, ambient light, and motion, providing user navigation via a rotary encoder and an OLED display.   

Hardware / Simulated Components   
ESP32 (Main microcontroller)
DHT22 (Temperature and humidity)
Photoresistor / LDR (Ambient light)
PIR Sensor (Motion detection)
FreeRTOS Architecture
The system utilizes five primary tasks communicating via FreeRTOS Queues and Event Groups.
