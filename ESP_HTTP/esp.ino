#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
// For Arduino boards with WiFi shield, use:
// #include <SPI.h>
// #include <WiFi.h>
// #include <WebServer.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>  // Changed from HTTPClient.h to ESP8266HTTPClient.h
#include <ArduinoJson.h>

// Change WebServer to ESP8266WebServer if using ESP8266
ESP8266WebServer server(80);  // For ESP8266
// WebServer server(80);  // For ESP32

// EEPROM settings
#define EEPROM_SIZE 512
#define MAX_NETWORKS 3
#define SSID_SIZE 32
#define PASS_SIZE 64
#define NETWORK_SIZE (SSID_SIZE + PASS_SIZE)
#define NETWORKS_START 0
#define NETWORK_COUNT_ADDR (EEPROM_SIZE - 1)
#define CONFIG_FLAG_ADDR (EEPROM_SIZE - 2)

// Access Point credentials
const char* apSSID = "ESP32-Config";
const char* apPassword = "12345678";

// IP Address details
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

unsigned long lastUpdateTime1 = 0;
const unsigned long updateInterval = 2000; // Update every 2 seconds

// Connection status
bool isConnected = false;

const int PIR1 = D1;
const int PIR2 = D2;
const int BUTTON1 = D7;  // Button for room1
const int BUTTON2 = D3;  // Button for room2
 
const int LED1 = D5;
const int LED2 = D6;

const int account_id = 2;  // Added 'int' type to account_id

// Variables to manage the timing
unsigned long motionDetectedTime1 = 0;
unsigned long motionDetectedTime2 = 0;
const unsigned long timeoutDuration = 5000;  // Timeout duration in milliseconds (10 seconds)

bool room1State = false;
bool room1Auto = true;
bool room2State = false;
bool room2Auto = true;

// Remove duplicate updateInterval
unsigned long lastUpdateTime = 0;
// const unsigned long updateInterval = 3000; // Removed duplicate

// Forward declarations for functions
void sendRoom1Data(bool light_state);

void setup() {
  // Set the LED pin as output
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  
  // Set the PIR sensor pin as input
  pinMode(PIR1, INPUT);
  pinMode(PIR2, INPUT);

  // Set the button pins as input
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);

  // Initialize EEPROM - ESP8266 EEPROM.begin doesn't return a bool
  EEPROM.begin(EEPROM_SIZE);
  
  // Try to connect to stored networks
  if (connectToStoredNetworks()) {
    Serial.println("[INFO] Connected to a stored network");
  } else {
    Serial.println("[INFO] No stored networks available");
    startAP();
    startWebServer();
  }
  
  // Start serial communication for debugging (optional)
  Serial.begin(9600);
}

bool lastButton1State = HIGH;  // Variable to store the last button state for button1
bool lastButton2State = HIGH;  // Variable to store the last button state for button2

unsigned long button1PressTime = 0;  // Variable to store the time when button1 is pressed
unsigned long button2PressTime = 0;  // Variable to store the time when button2 is pressed



void loop() {
  // Read the PIR sensor's output
  int pir1State = digitalRead(PIR1);
  int pir2State = digitalRead(PIR2);
  
  unsigned long current1Time = millis();

  // Read the button states
  int button1State = digitalRead(BUTTON1);
  int button2State = digitalRead(BUTTON2);

  // Handle button1 for room1
  if (button1State == LOW) {
    if (lastButton1State == HIGH) {
      button1PressTime = millis();  // Record the time when button1 is first pressed
    }
  } else {
    if (lastButton1State == LOW) {
      if (millis() - button1PressTime >= 3000) {
        room1Auto = !room1Auto;  // Toggle automatic mode for room1
        Serial.println(room1Auto ? "ROOM 1 AUTO MODE ON" : "ROOM 1 AUTO MODE OFF");
      } else {
        if (!room1Auto) {
          room1State = !room1State;  // Toggle room1State
          digitalWrite(LED1, room1State ? HIGH : LOW);  // Turn LED on or off based on room1State
          Serial.print("Room 1 state: ");
          Serial.println(room1State);

          // In your loop function, fix this part:
          if (current1Time - lastUpdateTime >= updateInterval) {
            sendRoom1Data(room1State);
            lastUpdateTime = current1Time;  // Fix: use lastUpdateTime instead of lastUpdateTime1
          }
        }
      }
    }
    if (!isConnected) {
      server.handleClient();
    }
    Serial.println("Attempting to send ROOM1 data...");
    // sendGPSData(); // Remove or replace this function call
  }

  // Update the last button state for button1
  lastButton1State = button1State;

  // Handle button2 for room2
  if (button2State == LOW) {
    if (lastButton2State == HIGH) {
      button2PressTime = millis();  // Record the time when button2 is first pressed
    }
  } else {
    if (lastButton2State == LOW) {
      if (millis() - button2PressTime >= 3000) {
        room2Auto = !room2Auto;  // Toggle automatic mode for room2
        Serial.println(room2Auto ? "ROOM 2 AUTO MODE ON" : "ROOM 2 AUTO MODE OFF");
      } else {
        if (!room2Auto) {
          room2State = !room2State;  // Toggle room2State
          digitalWrite(LED2, room2State ? HIGH : LOW);  // Turn LED on or off based on room2State
          Serial.print("Room 2 state: ");
          Serial.println(room2State);
        }
      }
    }
  }

  // Update the last button state for button2
  lastButton2State = button2State;

  // If motion is detected in room1
  if (pir1State == HIGH) {
    if (room1Auto) {
      digitalWrite(LED1, HIGH);  // Turn LED on
    }
    motionDetectedTime1 = millis();  // Record the time when motion is detected
    Serial.println("MOTION DETECTED IN ROOM 1");
  } else {
    // Check if no motion has been detected for the timeout duration
    if (millis() - motionDetectedTime1 >= timeoutDuration) {
      if (room1Auto) {
        digitalWrite(LED1, LOW);  // Turn LED off if timeout exceeded
      }
      Serial.println("NO MOTION DETECTED IN ROOM 1");
    }
  }
  
  // If motion is detected in room2
  if (pir2State == HIGH) {
    if (room2Auto) {
      digitalWrite(LED2, HIGH);  // Turn LED on
    }
    motionDetectedTime2 = millis();  // Record the time when motion is detected
    Serial.println("MOTION DETECTED IN ROOM 2");
  } else {
    // Check if no motion has been detected for the timeout duration
    if (millis() - motionDetectedTime2 >= timeoutDuration) {
      if (room2Auto) {
        digitalWrite(LED2, LOW);  // Turn LED off if timeout exceeded
      }
      Serial.println("NO MOTION DETECTED IN ROOM 2");
    }
  }

  delay(100);  // Small delay to stabilize the sensor reading
}


// Fix the sendRoom1Data function
void sendRoom1Data(bool light_state) {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi is connected. Sending room data...");
    
    WiFiClient client;
    HTTPClient http;
    
    // Use the new begin method with WiFiClient
    http.begin(client, "http://5.181.217.90/MovLitAPI/room_lighting.php");
    http.addHeader("Content-Type", "application/json");

    // Create JSON object
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["room_id"] = 1;
    jsonDoc["light_state"] = light_state ? 1 : 0;  // Convert boolean to integer (1/0)
    jsonDoc["account_id"] = account_id;

    String requestBody;
    serializeJson(jsonDoc, requestBody);
    
    // Print the request body for debugging
    Serial.print("Sending request: ");
    Serial.println(requestBody);

    int httpResponseCode = http.POST(requestBody);
    
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("Server response: ");
      Serial.println(response);

      // Only try to parse if we have a response
      if (response.length() > 0) {
        // Parse JSON response
        StaticJsonDocument<200> responseDoc;
        DeserializationError error = deserializeJson(responseDoc, response);

        if (!error) {
          bool success = responseDoc["success"];
          String message = responseDoc["message"];

          if (success) {
            Serial.println("Success: " + message);
          } else {
            Serial.println("Error: " + message);
          }
        } else {
          Serial.print("JSON parsing failed: ");
          Serial.println(error.c_str());
        }
      } else {
        Serial.println("Empty response from server");
      }
    } else {
      Serial.print("Error on HTTP request. Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi not connected. Cannot send room1 data.");
  }
}


void startAP() {
  Serial.println("\n[INFO] Starting Configuration Access Point...");
  
  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
    Serial.println("[ERROR] AP Config Failed");
    return;
  }
  
  if (!WiFi.softAP(apSSID, apPassword)) {
    Serial.println("[ERROR] AP Start Failed");
    return;
  }
  
  Serial.println("[INFO] Access Point Started Successfully");
  Serial.print("[INFO] AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void startWebServer() {
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.on("/reset", handleReset);
  server.begin();
  Serial.println("[INFO] HTTP server started");
}

void handleRoot() {
  String html = "<html><body>"
                "<h1>WiFi Configuration</h1>"
                "<form action='/save' method='POST'>"
                "SSID: <input type='text' name='ssid' required><br>"
                "Password: <input type='password' name='password' required><br>"
                "<input type='submit' value='Save & Connect'>"
                "</form>"
                "<p><a href='/reset'>Reset Configuration</a></p>"
                "</body></html>";
  server.send(200, "text/html", html);
}

void handleSave() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  
  if (ssid.length() == 0 || password.length() == 0) {
    server.send(400, "text/plain", "SSID and password cannot be empty");
    return;
  }
  
  // Save credentials to EEPROM
  writeNetworkToEEPROM(ssid, password);
  
  // Try to connect
  if (connectToWiFi(ssid, password)) {
    server.send(200, "text/plain", "Connected successfully!");
  } else {
    server.send(500, "text/plain", "Connection failed, please try again");
  }
}

void handleReset() {
  clearEEPROM();
  server.send(200, "text/plain", "Configuration reset. Restarting...");
  delay(1000);
  ESP.restart();
}

bool connectToWiFi(String ssid, String password) {
  Serial.println("\n[INFO] Attempting to connect to WiFi...");
  WiFi.begin(ssid.c_str(), password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[INFO] WiFi connected!");
    Serial.print("[INFO] IP address: ");
    Serial.println(WiFi.localIP());
    isConnected = true;
    return true;
  } else {
    Serial.println("\n[ERROR] Connection failed");
    return false;
  }
}

void writeEEPROM(int addr, String data) {
  for (int i = 0; i < data.length(); i++) {
    EEPROM.write(addr + i, data[i]);
  }
  EEPROM.write(addr + data.length(), '\0');
}

String readEEPROM(int addr) {
  String data;
  char ch;
  for (int i = 0; i < 63; i++) {
    ch = EEPROM.read(addr + i);
    if (ch == '\0') break;
    data += ch;
  }
  return data;
}

void clearEEPROM() {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
}

bool isNetworksStored() {
  uint8_t networkCount = EEPROM.read(NETWORK_COUNT_ADDR);
  return (networkCount > 0 && networkCount < 255);
}
// Add the missing connectToStoredNetworks function
bool connectToStoredNetworks() {
  uint8_t networkCount = EEPROM.read(NETWORK_COUNT_ADDR);
  
  if (networkCount == 0 || networkCount == 255) {
    Serial.println("[INFO] No networks stored");
    return false;
  }
  
  Serial.printf("[INFO] Found %d stored networks\n", networkCount);
  
  for (int i = 0; i < networkCount && i < MAX_NETWORKS; i++) {
    String ssid = readNetworkSSID(i);
    String password = readNetworkPassword(i);
    
    Serial.printf("[INFO] Trying to connect to %s\n", ssid.c_str());
    
    if (connectToWiFi(ssid, password)) {
      return true;
    }
  }
  
  return false;
}

// Add the missing network EEPROM functions
String readNetworkSSID(int index) {
  int startAddr = NETWORKS_START + (index * NETWORK_SIZE);
  return readEEPROMString(startAddr, SSID_SIZE);
}

String readNetworkPassword(int index) {
  int startAddr = NETWORKS_START + (index * NETWORK_SIZE) + SSID_SIZE;
  return readEEPROMString(startAddr, PASS_SIZE);
}

String readEEPROMString(int startAddr, int maxSize) {
  String result = "";
  for (int i = 0; i < maxSize; i++) {
    char c = EEPROM.read(startAddr + i);
    if (c == 0) break;
    result += c;
  }
  return result;
}

void writeNetworkToEEPROM(String ssid, String password) {
  uint8_t networkCount = EEPROM.read(NETWORK_COUNT_ADDR);
  
  if (networkCount == 255) networkCount = 0;
  
  // Check if network already exists
  for (int i = 0; i < networkCount && i < MAX_NETWORKS; i++) {
    if (readNetworkSSID(i) == ssid) {
      // Update existing network
      writeNetworkSSID(i, ssid);
      writeNetworkPassword(i, password);
      EEPROM.commit();
      return;
    }
  }
  
  // Add new network if there's space
  if (networkCount < MAX_NETWORKS) {
    writeNetworkSSID(networkCount, ssid);
    writeNetworkPassword(networkCount, password);
    EEPROM.write(NETWORK_COUNT_ADDR, networkCount + 1);
    EEPROM.commit();
  } else {
    // Replace the first network (you could implement a different replacement strategy)
    writeNetworkSSID(0, ssid);
    writeNetworkPassword(0, password);
    EEPROM.commit();
  }
}

void writeNetworkSSID(int index, String ssid) {
  int startAddr = NETWORKS_START + (index * NETWORK_SIZE);
  writeEEPROMString(startAddr, ssid, SSID_SIZE);
}

void writeNetworkPassword(int index, String password) {
  int startAddr = NETWORKS_START + (index * NETWORK_SIZE) + SSID_SIZE;
  writeEEPROMString(startAddr, password, PASS_SIZE);
}

void writeEEPROMString(int startAddr, String data, int maxSize) {
  for (int i = 0; i < maxSize; i++) {
    if (i < data.length()) {
      EEPROM.write(startAddr + i, data[i]);
    } else {
      EEPROM.write(startAddr + i, 0);
    }
  }
}