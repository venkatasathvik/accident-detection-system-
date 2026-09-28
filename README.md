# accident-detection-system

A real-time embedded tracking and accident detection system designed to monitor vehicle impact and automatically transmit location-based alerts. 

###  Hardware Architecture
* **Microcontroller:** ESP32 
* **Impact Sensor:** ADXL345 3-axis accelerometer (communicating via I2C/SPI)
* **Location Tracking:** NEO-6M GPS module (communicating via UART)

### Software & Firmware
* **Languages:** C/C++ 
* **Core Logic:** Custom I2C/UART driver logic for sensor interfacing and data parsing
* **Telemetry:** Automated webhooks via Make.com to dispatch minimal-latency alert payloads

###  System Workflow
1. The ESP32 continuously reads the ADXL345 for G-force variations.
2. Upon crossing a hard-coded impact threshold, the system triggers a threshold alert.
3. The ESP32 parses current latitude and longitude from the NEO-6M GPS module via UART.
4. A serialized payload containing the coordinates is dispatched via a Make.com webhook to notify emergency contacts.
