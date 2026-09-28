#include <WiFi.h>              
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h> 
#include <TinyGPS++.h>

// ==========================================
//          USER CONFIGURATION
// ==========================================

// 1. Wi-Fi Credentials
const char* ssid = "wifi - username "; 
const char* password = "wifi - password ";

// 2. Make.com Webhook URL
String make_webhook_url = "https://hook.eu1.make.com/npg3ii363slduahhtpo3yhkmg03644r3"; 

// 3. Settings
const float CRASH_THRESHOLD_G = 4.0; // G-Force threshold
const int TEST_BUTTON_PIN = 4;       // Pin D4 connected to button -> GND

// ==========================================

// Global Objects
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345); 

TinyGPSPlus gps;
HardwareSerial gpsSerial(2); // Use UART2 for GPS

// State Variables
bool alertSent = false;

// --- Function Declarations ---
void connectToWiFi();
void sendWebhook(String lat, String lon);

void setup() {
  // 1. Start Debug Serial
  Serial.begin(115200);
  Serial.println("\n>>> ESP32 Accident Detection System Booting...");

  // 2. Start GPS Serial (RX=16, TX=17)
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("GPS Serial Started on RX:16, TX:17");

  // 3. Setup Test Button
  pinMode(TEST_BUTTON_PIN, INPUT_PULLUP);

  // 4. Initialize ADXL345
  if (!accel.begin()) {
    Serial.println("ERROR: ADXL345 not found! Check wiring.");
    while (1) { delay(10); }
  }
  Serial.println("ADXL345 Found & Ready.");
  
  // Set Range (16G is best for crash detection)
  accel.setRange(ADXL345_RANGE_16_G);

  // 5. Connect to Wi-Fi
  connectToWiFi();
}

void loop() {
  // --- STEP 1: Keep GPS "Fed" ---
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // --- STEP 2: Read Accelerometer ---
  sensors_event_t event; 
  accel.getEvent(&event);

  // Convert m/s^2 to G's (divide by 9.8)
  float x_g = event.acceleration.x / 9.8;
  float y_g = event.acceleration.y / 9.8;
  float z_g = event.acceleration.z / 9.8;
  
  // Calculate total G-force vector
  float total_g = sqrt(sq(x_g) + sq(y_g) + sq(z_g));

  // --- STEP 3: Check Triggers ---
  bool crashDetected = (total_g > CRASH_THRESHOLD_G);
  bool buttonPressed = (digitalRead(TEST_BUTTON_PIN) == LOW);

  // --- STEP 4: Handle Alert ---
  if ((crashDetected || buttonPressed) && !alertSent) {
    
    Serial.println("\n!!! TRIGGER DETECTED !!!");
    if(crashDetected) Serial.println("Cause: High G-Force Impact");
    if(buttonPressed) Serial.println("Cause: Manual Test Button");

    alertSent = true;
    
    // Get Location Data
    String latStr, lonStr;
    
    if (gps.location.isValid()) {
      latStr = String(gps.location.lat(), 6);
      lonStr = String(gps.location.lng(), 6);
      Serial.print("GPS Lock Found: ");
    } else {
      // Use 0,0 if GPS has no lock yet
      latStr = "0"; 
      lonStr = "0";
      Serial.print("No GPS Lock (Using Dummy 0,0): ");
    }
    Serial.print(latStr); Serial.print(", "); Serial.println(lonStr);

    // Send to Make.com
    sendWebhook(latStr, lonStr);

    Serial.println("System locked for 15 seconds to prevent spamming...");
    delay(15000); 
    alertSent = false; // Re-arm system
    Serial.println("System Re-Armed and Ready.\n");
  }
  
  delay(50);
}

// --- Helper: Wi-Fi Connection ---
void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retries++;
    if(retries > 20) {
      Serial.println("\nWi-Fi Connection Failed. Will try again in loop.");
      break; 
    }
  }
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("\nWi-Fi Connected!");
  }
}

// --- Helper: Send Webhook ---
void sendWebhook(String lat, String lon) {
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Construct the URL with query parameters
    String url = make_webhook_url + "?lat=" + lat + "&lon=" + lon;
    
    Serial.println("Sending Webhook...");
    http.begin(url);
    
    // Send HTTP GET request
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected. Cannot send webhook.");
  }
}
