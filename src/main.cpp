#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// --- Wi-Fi and Server Setup ---
const char* ssid = "Wokwi-GUEST";
const char* password = "";
// Using the Linux IP address we found earlier
const char* serverName = "http://192.168.1.12:8080"; 

// --- DHT22 Setup ---
#define DHT_PIN 4
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

// --- BMP180 Setup ---
Adafruit_BMP085 bmp;

// --- RTOS Variables & Configurations ---
const float DANGER_TEMP = 35.0; // The threshold that triggers the interrupt-like alarm
TaskHandle_t TaskWiFi;
TaskHandle_t TaskWatchdog;
SemaphoreHandle_t sensorMutex;  // The lock to protect the I2C bus

// =======================================================
// FORWARD DECLARATIONS (Fixes the PlatformIO Scope Error)
// =======================================================
void loopWiFiSend(void * parameter);
void loopWatchdog(void * parameter);

void setup() {
  Serial.begin(115200);
  
  // 1. Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wokwi-GUEST");

  // 2. Initialize Sensors
  Wire.begin(21, 22); // Standard ESP32 I2C pins
  dht.begin();
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP180 sensor, check wiring!");
  }
  
  // 3. Create the Mutex for the I2C Bus
  sensorMutex = xSemaphoreCreateMutex();

  // 4. Create Task 1: The Heavy Wi-Fi Sender (Priority 1, Core 1)
  xTaskCreatePinnedToCore(
    loopWiFiSend,   
    "TaskWiFi",     
    8192,           // Large stack size for JSON formatting
    NULL,           
    1,              
    &TaskWiFi,      
    1               
  );

  // 5. Create Task 2: The High-Priority Watchdog (Priority 2, Core 0)
  xTaskCreatePinnedToCore(
    loopWatchdog,   
    "TaskWatchdog", 
    2048,           
    NULL,           
    2,              
    &TaskWatchdog,  
    0               
  );
}

// In FreeRTOS, the standard loop is not needed. We delete it to save memory.
void loop() {
  vTaskDelete(NULL); 
}

// ==========================================
// TASK 1: Normal JSON Wi-Fi Sender (Core 1)
// ==========================================
void loopWiFiSend(void * parameter) {
  for(;;) {
    float h = 0, t = 0, bmpTemp = 0, pressure = 0, altitude = 0;

    // --- MUTEX LOCK ---
    // Safely claim the I2C bus to read the sensors
    if (xSemaphoreTake(sensorMutex, portMAX_DELAY)) {
      h = dht.readHumidity();
      t = dht.readTemperature();
      bmpTemp = bmp.readTemperature();
      pressure = bmp.readPressure();
      altitude = bmp.readAltitude(101325);
      
      xSemaphoreGive(sensorMutex); // Return the lock immediately
    }
    // --- END MUTEX ---

    // Build the JSON Payload
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["dht_temp"] = isnan(t) ? 0 : t;
    jsonDoc["dht_humidity"] = isnan(h) ? 0 : h;
    jsonDoc["bmp_temp"] = bmpTemp;
    jsonDoc["bmp_pressure"] = pressure;
    jsonDoc["bmp_altitude"] = altitude;
    
    String jsonPayload;
    serializeJson(jsonDoc, jsonPayload);
    
    // Send over Wi-Fi
    if(WiFi.status() == WL_CONNECTED){
      HTTPClient http;
      http.begin(serverName);
      http.addHeader("Content-Type", "application/json");
      
      int httpResponseCode = http.POST(jsonPayload);
      
      if (httpResponseCode > 0) {
        Serial.print("Data Sent! HTTP Code: ");
        Serial.println(httpResponseCode);
      } else {
        Serial.print("Failed to send. Error code: ");
        Serial.println(httpResponseCode);
      }
      http.end();
    }
    
    // Wait 5 seconds before building and sending the next payload
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

// ==========================================
// TASK 2: High-Priority Watchdog (Core 0)
// ==========================================
void loopWatchdog(void * parameter) {
  for(;;) {
    float currentTemp = 0;

    // --- MUTEX LOCK ---
    // Ask for the I2C bus. If the Wi-Fi task is using it, wait.
    if (xSemaphoreTake(sensorMutex, portMAX_DELAY)) {
      currentTemp = bmp.readTemperature(); // BMP is faster to read than DHT
      xSemaphoreGive(sensorMutex);
    }
    // --- END MUTEX ---

    // --- REAL-TIME THRESHOLD LOGIC ---
    if (currentTemp >= DANGER_TEMP) {
      Serial.println("\n!!! ALARM: TEMPERATURE DANGER ZONE !!!");
      Serial.print("Current Temp: ");
      Serial.print(currentTemp);
      Serial.println(" °C");
      
      // Add your emergency hardware triggers here (e.g., Relay, Buzzer)
    }

    // Cycle this check rapidly (every 500 milliseconds)
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}