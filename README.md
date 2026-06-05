## About The Project

This project focuses on building an **IoT-based real-time environmental monitoring device** using an **ESP32** microcontroller. The system continuously tracks ambient conditions and is engineered with a real-time operating system to ensure deterministic, time-critical alerting in the event of hazardous environmental changes. 

I fully simulated this entire setup on the **Wokwi** platform.

### Hardware & Simulation Components

* **ESP32:** Its dual-core processor and built-in Wi-Fi to handle both local sensor processing and cloud telemetry.
* **DHT22 Sensor:** Used for capturing temperature and humidity data.
* **BMP180 Sensor:** Used for measuring barometric pressure and ambient temperature.

### Software Architecture & Real-Time Logic

To guarantee system reliability, I implemented **FreeRTOS** to manage real-time events. In safety-critical IoT applications, a delayed response to a hazard can be catastrophic. 

* **Deterministic Execution:** FreeRTOS ensures that critical tasks—such as hazard detection and alarm broadcasting—execute within strict, predictable timeframes.
* **Task Scheduling:** I configured the system tasks to run in a controlled, cooperative/non-preemptive manner where necessary, preventing resource starvation and ensuring that high-priority safety routines are never arbitrarily blocked or delayed by standard telemetry tasks.

### Features & Workflow

1.  **Continuous Telemetry:** The ESP32 constantly reads data from the DHT22 and BMP180 sensors and streams this environmental telemetry securely to the cloud.
2.  **Hazard Detection:** The system continuously evaluates incoming sensor thresholds for anomalous or dangerous spikes (e.g., extreme heat or pressure drops).
3.  **Local Broadcasting:** If a hazardous event is detected, the system immediately bypasses lower-priority tasks to broadcast a real-time audio/visual alert locally in the room.
