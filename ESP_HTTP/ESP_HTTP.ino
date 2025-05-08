
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// WiFi credentials
const char* ssid = "Marfil"; // Replace with your network SSID
const char* password = "05022021"; // Replace with your network password

// Server URL
const char* serverUrl = "http://5.181.217.90/MovLitAPI/room_lighting.php"; // Update with the specific API endpoint

// Pin definitions
const int PIR1 = D1;
const int PIR2 = D2;
const int BUTTON1 = D7;  // Button for room1
const int BUTTON2 = D3;  // Button for room2
const int LED1 = D5;
const int LED2 = D6;

// Variables to manage the timing
unsigned long motionDetectedTime1 = 0;
unsigned long motionDetectedTime2 = 0;
const unsigned long timeoutDuration = 5000;  // Timeout duration in milliseconds (5 seconds)

bool room1State = false;
bool room1Auto = true;
bool room2State = false;
bool room2Auto = true;

bool lastButton1State = HIGH;  // Variable to store the last button state for button1
bool lastButton2State = HIGH;  // Variable to store the last button state for button2

unsigned long button1PressTime = 0;  // Variable to store the time when button1 is pressed
unsigned long button2PressTime = 0;  // Variable to store the time when button2 is pressed

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi");

    // Set the LED pin as output
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);

    // Set the PIR sensor pin as input
    pinMode(PIR1, INPUT);
    pinMode(PIR2, INPUT);

    // Set the button pins as input
    pinMode(BUTTON1, INPUT_PULLUP);
    pinMode(BUTTON2, INPUT_PULLUP);
}

void loop() {
    // Read the PIR sensor's output
    int pir1State = digitalRead(PIR1);
    int pir2State = digitalRead(PIR2);

    // Read the button states
    int button1State = digitalRead(BUTTON1);
    int button2State = digitalRead(BUTTON2);

    // Debugging: Print the states
    Serial.print("PIR1: ");
    Serial.print(pir1State);
    Serial.print(" PIR2: ");
    Serial.print(pir2State);
    Serial.print(" Button1: ");
    Serial.print(button1State);
    Serial.print(" Button2: ");
    Serial.println(button2State);

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
                    sendHttpRequest(1, room1State);
                }
            }
        }
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
                    sendHttpRequest(2, room2State);
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
            sendHttpRequest(1, true);  // Correctly uses roomId 1
        }
        motionDetectedTime1 = millis();  // Record the time when motion is detected
        Serial.println("MOTION DETECTED IN ROOM 1");
    } else {
        // Check if no motion has been detected for the timeout duration
        if (millis() - motionDetectedTime1 >= timeoutDuration) {
            if (room1Auto) {
                digitalWrite(LED1, LOW);  // Turn LED off if timeout exceeded
                sendHttpRequest(1, false);  // Correctly uses roomId 1
            }
            Serial.println("NO MOTION DETECTED IN ROOM 1");
        }
    }

    // If motion is detected in room2
    if (pir2State == HIGH) {
        if (room2Auto) {
            digitalWrite(LED2, HIGH);  // Turn LED on
            sendHttpRequest(2, true);
        }
        motionDetectedTime2 = millis();  // Record the time when motion is detected
        Serial.println("MOTION DETECTED IN ROOM 2");
    } else {
        // Check if no motion has been detected for the timeout duration
        if (millis() - motionDetectedTime2 >= timeoutDuration) {
            if (room2Auto) {
                digitalWrite(LED2, LOW);  // Turn LED off if timeout exceeded
                sendHttpRequest(2, false);
            }
            Serial.println("NO MOTION DETECTED IN ROOM 2");
        }
    }

    delay(100);  // Small delay to stabilize the sensor reading
}

void sendHttpRequest(int roomId, bool lightState) {
    if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client; // Create a WiFiClient object
        HTTPClient http;
        http.begin(client, serverUrl); // Use the updated begin method with WiFiClient

        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        // Prepare the POST data
        String postData = "action=device_control&room_id=" + String(roomId) + "&light_state=" + String(lightState ? 1 : 0);

        int httpResponseCode = http.POST(postData);

        if (httpResponseCode > 0) {
            String response = http.getString();
            if (response.length() > 0) {
                Serial.println("Response: " + response);
            } else {
                Serial.println("Warning: Received empty response");
            }
        } else {
            Serial.println("Error on sending POST: " + String(httpResponseCode));
        }

        http.end();
    } else {
        Serial.println("WiFi not connected");
    }
}
