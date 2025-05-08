#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "your_SSID"; // Replace with your network SSID
const char* password = "your_PASSWORD"; // Replace with your network password

const char* serverUrl = "http://your-server-address/api"; // Replace with your server URL

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi");
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverUrl);

        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        int lightState = 1; // Example light state
        String postData = "device_id=2&room_id=1&light_state=" + String(lightState);

        int httpResponseCode = http.POST(postData);

        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println("Response: " + response);
        } else {
            Serial.println("Error on sending POST: " + String(httpResponseCode));
        }

        http.end();
    } else {
        Serial.println("WiFi not connected");
    }

    delay(10000); // Send request every 10 seconds
}