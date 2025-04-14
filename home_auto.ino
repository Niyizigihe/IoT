#include <WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <HardwareSerial.h>

#define TRIG_PIN 5       // GPIO pin for TRIG
#define ECHO_PIN 18      // GPIO pin for ECHO
#define SOIL_MOISTURE_PIN 34 // GPIO pin for Soil Moisture sensor
#define LED_PIN 2        // Onboard LED pin
#define LAMP1_PIN 15     // GPIO pin for Lamp 1
#define LAMP2_PIN 22     // GPIO pin for Lamp 2

// Use HardwareSerial on the ESP32
HardwareSerial mySerial(1); // UART1

const char* ssid = "TECNO POP 7";       // Replace with your network SSID
const char* password = "nizeyimana"; // Replace with your network password

const char* serverUrl = "https://doorlock.x10.mx/temperature/index.php";

void setup() {
  Serial.begin(115200);
 // Initialize UART1 on GPIO16 (RX) and GPIO17 (TX)
  mySerial.begin(9600, SERIAL_8N1, 26, 27);
  delay(1000);

  // Send initialization commands to the GSM module
  sendATCommand("AT+CMGF=1", 1000);
  sendATCommand("AT+CNMI=1,2,0,0,0", 100);
  sendATCommand("AT", 2000);
  sendATCommand("AT+CSQ", 2000);
  sendATCommand("AT+CCID", 2000);
  sendATCommand("AT+CREG?", 2000);

  
  // Initialize the sensor and LED pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LAMP1_PIN, OUTPUT);
  pinMode(LAMP2_PIN, OUTPUT);

  // Initialize the Soil Moisture sensor pin
  pinMode(SOIL_MOISTURE_PIN, INPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to Wi-Fi");

  // Print the IP address
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  updateSerial();
  
  long duration, distance;
  int soilMoistureValue;

  // Trigger the measurement for distance
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Measure the duration
  duration = pulseIn(ECHO_PIN, HIGH);

  // Calculate the distance
  distance = (duration / 2) * 0.0343;

  // Read the soil moisture value
  soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);

  // Convert the soil moisture value to percentage (assuming a 12-bit ADC)
  float soilMoisturePercent = ((float)soilMoistureValue / 4095.0) * 100;

  // Turn LED on if distance is less than 32 cm and soil moisture is greater than 80%
  if (distance < 32 && soilMoisturePercent > 80) {
    digitalWrite(LED_PIN, HIGH); // Turn on LED
    SendMessage();
  } else {
    digitalWrite(LED_PIN, LOW); // Turn off LED
  }

  // Send data to the remote server
  sendToServer(distance, soilMoisturePercent);

  // Check for commands from server
  checkCommands();

  delay(1000); // Send data every second
}

void sendToServer(long distance, float soilMoisturePercent) {
  if (WiFi.status() == WL_CONNECTED) { // Check Wi-Fi connection status
    HTTPClient http;

    http.begin(serverUrl); // Specify the URL
    http.addHeader("Content-Type", "application/json"); // Specify content-type header

    // Prepare JSON payload
    String json = "{\"distance\":" + String(distance) + ",\"soilMoisture\":" + String(soilMoisturePercent) + "}";

    // Send HTTP POST request
    int httpResponseCode = http.POST(json);

    // Check response
    if (httpResponseCode > 0) {
      String response = http.getString(); // Get the response to the request
      Serial.println(httpResponseCode); // Print return code
      Serial.println(response); // Print request answer
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end(); // Free resources
  } else {
    Serial.println("WiFi Disconnected");
  }
}

void checkCommands() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(serverUrl) + "?commands=true";

    http.begin(url); // Specify the URL
    int httpResponseCode = http.GET(); // Send GET request

    if (httpResponseCode > 0) {
      String payload = http.getString(); // Get the response
      if (payload.indexOf("lamp1_on") != -1) {
        digitalWrite(LAMP1_PIN, HIGH); // Turn on Lamp 1
      } else if (payload.indexOf("lamp1_off") != -1) {
        digitalWrite(LAMP1_PIN, LOW); // Turn off Lamp 1
      }
      if (payload.indexOf("lamp2_on") != -1) {
        digitalWrite(LAMP2_PIN, HIGH); // Turn on Lamp 2
      } else if (payload.indexOf("lamp2_off") != -1) {
        digitalWrite(LAMP2_PIN, LOW); // Turn off Lamp 2
      }
    }

    http.end(); // Free resources
  }
}



void updateSerial() {
  delay(500);
  while (Serial.available()) {
    mySerial.write(Serial.read()); // Forward what Serial received to Hardware Serial Port
  }
  while (mySerial.available()) {
    Serial.write(mySerial.read()); // Forward what Hardware Serial received to Serial Port
  }
}

void sendATCommand(const char* command, int timeout) {
  mySerial.println(command);
  Serial.println(command); // Print the command for debugging
  long int time = millis();
  while ((time + timeout) > millis()) {
    while (mySerial.available()) {
      char c = mySerial.read();
      Serial.print(c);
    }
  }
  Serial.println();
}

void SendMessage() {
  Serial.println("Initializing...");
  delay(1000);
  sendATCommand("AT", 2000); // Handshake test
  sendATCommand("AT+CMGF=1", 2000); // Configuring TEXT mode
  sendATCommand("AT+CMGS=\"+250787303380\"", 2000); // Replace with your phone number
  mySerial.print("Someone is there!"); // Message content
  delay(2000); // Wait for the message to be sent
  mySerial.write(26); // ASCII code for Ctrl+Z to send the SMS
  updateSerial();
  Serial.println("Message sent.");
}
