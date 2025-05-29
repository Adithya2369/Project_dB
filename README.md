# Environmental Noise Monitoring Alert System  
*A Hybrid Edge-Cloud IoT Architecture*

---

## What is This Project?

This system lets you monitor urban noise levels in real time, check if they comply with your city’s legal limits, and get alerts when they don’t. It’s designed to be scalable, low-cost, and easy to deploy anywhere—even without GPS hardware. Instead, it uses WiFi-based geolocation and cloud APIs to keep everything dynamic and up-to-date.

---

## How It Works

- **Edge Node (dsPIC30F4013 + Microphone):**  
  Continuously measures sound levels, checks if they exceed the city’s legal threshold, and indicates compliance with LEDs.

- **Gateway (ESP32):**  
  - Determines its city using WiFi MAC addresses and Google Cloud Platform (GCP) Geolocation APIs (no GPS needed).
  - Fetches the current noise threshold for that city from a cloud server.
  - Sends the threshold to all connected edge nodes.
  - Receives alerts from nodes if the noise is too high and can escalate them to authorities or log them in the cloud.

- **Cloud Backend (Flask API):**  
  - Maintains a database mapping cities to their legal noise limits.
  - Provides endpoints for threshold retrieval and alert logging.
  - Can be updated remotely to reflect new regulations.

---

## Quick Start Guide

### 1. Hardware Setup

- **You’ll Need:**
  - ESP32 development board (with WiFi)
  - dsPIC30F4013 microcontroller for each measurement node
  - MAX4466 or similar microphone module (one per node)
  - LEDs for visual feedback (green/red)
  - Power supply for each device

- **Wiring:**
  - Connect the microphone to the dsPIC.
  - Connect LEDs to dsPIC output pins.
  - Set up UART between ESP32 and each dsPIC node.

### 2. Firmware Installation

- **ESP32:**
  - Flash the provided firmware.
  - Configure your WiFi credentials in the code.
  - Set the API endpoint for the cloud server.

- **dsPIC:**
  - Flash the edge node firmware.
  - Ensure sampling rate is set to 48kHz for accurate sound measurement.

### 3. Cloud Backend Setup

- Deploy the Flask API (provided in the repo) to Azure, GCP, or any hosting service.
- Populate the city-to-threshold JSON database (see `thresholds.json` sample).
- Make sure your API endpoints are accessible to the ESP32 device.

### 4. Using the System

1. **Power on all devices.**
2. **ESP32 connects to WiFi, collects nearby MAC addresses, and queries the GCP Geolocation API.**
3. **ESP32 determines its city, requests the current noise threshold from the cloud, and sends it to all dsPIC nodes.**
4. **Each dsPIC node:**
   - Continuously samples sound.
   - Calculates the A-weighted sound level (LEQ) every 500ms.
   - If the measured dB exceeds the threshold:
     - Turns on the red LED.
     - Notifies the ESP32.
   - If within limits:
     - Keeps the green LED on.
5. **When ESP32 receives an exceedance alert:**
   - It can log the event to the cloud or notify authorities, depending on your configuration.

### 5. Customization

- **Add or update city thresholds:**  
  Edit the `thresholds.json` file in your cloud backend to add new cities or change dB limits.

- **Change alerting behavior:**  
  Modify the ESP32 firmware to send SMS, email, or integrate with municipal systems.

- **Expand deployment:**  
  Add more edge nodes or gateways as needed. The system automatically adapts to new locations.

---

## Example Thresholds Database (`thresholds.json`)
{
    "mumbai": 75,
    "delhi": 70,
    "bangalore": 68,
    "hyderabad": 72,
    "chennai": 70,
}

-------------------------------------------------------------------

*You can update this remotely to reflect new regulations or add new cities.*

---

## Troubleshooting

- **ESP32 not connecting:**  
  Check WiFi credentials and cloud API endpoint.

- **No city detected:**  
  Ensure ESP32 can scan nearby WiFi networks and reach the GCP API.

- **Threshold not updating:**  
  Verify cloud backend is running and accessible.

- **No LED response:**  
  Check dsPIC firmware and wiring. Make sure the threshold is received from ESP32.

---

## Extending the System

- Integrate with air quality or other environmental sensors.
- Add machine learning for anomaly detection.
- Connect to smart city dashboards or alerting systems.

---

## Support

For setup help, bug reports, or feature requests, please open an issue in this repository.

---

*This project is designed for easy adaptation and deployment in any urban environment. If you have suggestions or improvements, contributions are welcome!*

