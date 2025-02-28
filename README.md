# IoT-Based Real-Time Patient Health Monitoring System

## Overview
This project focuses on developing an **IoT-based real-time patient health monitoring system** that continuously tracks vital health parameters such as **heartbeat, temperature, and blood pressure**. The system utilizes **ESP-32 microcontrollers** to collect sensor data and transmit it to a local web server and **IFTTT for notifications**.

## Features
- Real-time monitoring of **heartbeat, temperature, and blood pressure**.
- **ESP-32 microcontroller** for efficient sensor data processing and transmission.
- Local web server for **real-time data access and visualization**.
- **IFTTT integration** for automatic alerts in case of abnormal readings.
- **User-friendly dashboard** for monitoring patient vitals.

## Technologies Used
- **Hardware**: ESP-32 microcontroller, biomedical sensors (heart rate, temperature, BP sensors)
- **Software**: Arduino IDE, ExpressPCB, IFTTT
- **Communication Protocols**: HTTP

## System Architecture
1. **Data Collection**: Biomedical sensors connected to an **ESP-32** microcontroller collect real-time patient vitals.
2. **Data Transmission**: The ESP-32 transmits sensor data to a **local web server** and **sends alerts via IFTTT**.
3. **User Interface**: A web-based dashboard enables healthcare professionals to track patient vitals remotely.
4. **Alert System**: If critical conditions are detected, notifications are sent via the **IFTTT** service.

## Installation and Setup
### Prerequisites
- ESP-32 microcontroller
- Arduino IDE installed
- IFTTT account setup

### Steps
1. **Set up the ESP-32**:
   - Install **Arduino IDE** and add the ESP-32 board support package.
   - Upload the firmware code to the ESP-32.
2. **Connect Sensors**:
   - Wire the **heartbeat, temperature, and BP sensors** to the ESP-32.
3. **Configure IFTTT**:
   - Set up an **IFTTT applet** to trigger notifications based on sensor readings.
4. **Run the System**:
   - Power the ESP-32 and check the data being transmitted to the local web server.
   - Use the web dashboard to view real-time patient vitals.

## Future Improvements
- Integration of **AI-based anomaly detection** for critical health alerts.
- Enhanced security features using **encryption protocols**.
- Mobile application for remote monitoring.

## Contributors
- **[Saeedul Haque]** - Developer & Researcher

## License
This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

## Contact
For any queries or contributions, feel free to reach out at **your.email@example.com** or open an issue in this repository.
