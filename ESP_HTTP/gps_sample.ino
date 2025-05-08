#include <TinyGPS++.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// Variables for storing GPS Data
float latitude;
float longitude;
float speed;
float satellites;
String direction;
String GPS_SERIAL = "91827372he";

static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
unsigned int move_index = 1;

// Define a new serial port for the GPS module
#define SerialGPS Serial2

// Web server instance
WebServer server(80);

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

// Connection status
bool isConnected = false;

//I2C pins for LCD
#define SDA_PIN 21
#define SCL_PIN 22

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2); // Adjust the address if needed

void setup() {
  // Set console baud rate
  Serial.begin(9600);
  delay(10);
  
  // Initialize GPS serial connection
  SerialGPS.begin(GPSBaud, SERIAL_8N1, 13, 14); // RX, TX
  Serial.println("GPS Serial initialized");

  // Initialize I2C for LCD
  Wire.begin(SDA_PIN, SCL_PIN);

  // Initialize LCD with correct parameters
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Initializing...");

  // Initialize EEPROM
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("[ERROR] Failed to initialize EEPROM");
    return;
  }
  
  // Try to connect to stored networks
  if (connectToStoredNetworks()) {
    Serial.println("[INFO] Connected to a stored network");
  } else {
    Serial.println("[INFO] No stored networks available");
    startAP();
    startWebServer();
  }
}

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 3000; // Update every 60 seconds

void loop() {
  while (SerialGPS.available() > 0) {
    char c = SerialGPS.read();
    if (gps.encode(c)) {
      displayInfo();
      unsigned long currentTime = millis();
      if (currentTime - lastUpdateTime >= updateInterval) {
        if (gps.location.isValid()) {
          sendLocationUpdate();
        } else {
          sendGpsLostUpdate();
        }
        lastUpdateTime = currentTime;
      }
    }
  }

  if (!isConnected) {
    server.handleClient();
  }
  Serial.println("Attempting to send GPS data...");
  sendGPSData();
}

void sendGpsLostUpdate() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi is connected. Sending GPS lost update...");
    HTTPClient http;
    http.begin("http://5.181.217.90/BikonomiAPI/update_gps_to_lost.php");
    http.addHeader("Content-Type", "application/json");

    // Create JSON object
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["gps_serial"] = GPS_SERIAL;

    String requestBody;
    serializeJson(jsonDoc, requestBody);

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(response);
    } else {
      Serial.println("Error on HTTP request");
    }
    http.end();
  } else {
    Serial.println("WiFi not connected. Cannot send GPS lost update.");
  }
}

void sendLocationUpdate() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi is connected. Sending location update...");
    HTTPClient http;
    http.begin("http://5.181.217.90/BikonomiAPI/update_location.php");
    http.addHeader("Content-Type", "application/json");

    // Create JSON object
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["gps_serial"] = GPS_SERIAL;
    jsonDoc["latitude"] = latitude;
    jsonDoc["longitude"] = longitude;

    String requestBody;
    serializeJson(jsonDoc, requestBody);

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(response);
    } else {
      Serial.println("Error on HTTP request");
    }
    http.end();
  } else {
    Serial.println("WiFi not connected. Cannot send location update.");
  }
}

void sendGPSData() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi is connected. Sending GPS data...");
    HTTPClient http;
    http.begin("http://5.181.217.90/BikonomiAPI/gps_check_rental.php");
    http.addHeader("Content-Type", "application/json");

    // Create JSON object
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["gps_serial"] = GPS_SERIAL;

    String requestBody;
    serializeJson(jsonDoc, requestBody);

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(response);

      // Parse JSON response
      StaticJsonDocument<200> responseDoc;
      DeserializationError error = deserializeJson(responseDoc, response);

      if (!error) {
        bool success = responseDoc["success"];
        String message = responseDoc["message"];
        String startTime = responseDoc["start_time"];
        String expectedEndTime = responseDoc["expected_end_time"];
        String now = responseDoc["time_now"];

        if (success) {
          calculateAndDisplayTime(startTime, expectedEndTime, now);
        } else {
          lcd.clear();
          lcd.print("No rental found");
        }
      }
    } else {
      Serial.println("Error on HTTP request");
    }
    http.end();
  } else {
    Serial.println("WiFi not connected. Cannot send GPS data.");
  }
}

void calculateAndDisplayTime(String startTime, String expectedEndTime, String nowTime) {
  // Convert start and end times to time_t
  struct tm tmStart, tmEnd, tmNow;
  strptime(startTime.c_str(), "%Y-%m-%d %H:%M:%S", &tmStart);
  strptime(expectedEndTime.c_str(), "%Y-%m-%d %H:%M:%S", &tmEnd);
  strptime(nowTime.c_str(), "%Y-%m-%d %H:%M:%S", &tmNow);

  time_t start = mktime(&tmStart);
  time_t end = mktime(&tmEnd);
  time_t now = mktime(&tmNow);
  Serial.print("Start Time: ");
  Serial.println(start);
  Serial.print("End Time: ");
  Serial.println(end);
  Serial.print("Now: ");
  Serial.println(now);
  long remainingTime = difftime(end, now);
  long overdueTime = difftime(now, end);

  lcd.clear();
  if (remainingTime > 0) {
    lcd.print("Time left: ");
    lcd.setCursor(0, 1);
    lcd.print(remainingTime / 60);
    lcd.print(" mins");
  } else {
    lcd.print("Overdue: ");
    lcd.setCursor(0, 1);
    lcd.print(overdueTime / 60);
    lcd.print(" mins");
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

void displayInfo()
{
  if (gps.location.isValid())
  {
    latitude = (gps.location.lat());
    longitude = (gps.location.lng());

    Serial.print("LAT:  ");
    Serial.println(latitude, 6);
    Serial.print("LONG: ");
    Serial.println(longitude, 6);

    speed = gps.speed.kmph();
    Serial.print("Speed: ");
    Serial.println(speed);

    direction = TinyGPSPlus::cardinal(gps.course.value());
    Serial.print("Direction: ");
    Serial.println(direction);
    
    satellites = gps.satellites.value();
    Serial.print("Satellites: ");
    Serial.println(satellites);
  }
  Serial.println();
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