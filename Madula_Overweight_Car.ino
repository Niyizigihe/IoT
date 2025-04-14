#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "HX711.h"

// Motor Pins
#define IN1 D1
#define IN2 D2
#define IN3 D3
#define IN4 D7
#define ENA D5  // Enable pin for motor 1 (connected to PWM pin)
#define ENB D6  // Enable pin for motor 2 (connected to PWM pin)

// HX711 Load Cell Pins
#define LOADCELL_DOUT_PIN D8
#define LOADCELL_SCK_PIN D4

// Buzzer Pin
#define BUZZER_PIN D0

// Weight threshold in grams (500g)
#define WEIGHT_THRESHOLD 500
#define CALIBRATION_FACTOR 499.0987

HX711 scale;
int speed = 255; // Default speed value

ESP8266WebServer server(80);

// Wi-Fi credentials for Access Point
const char* ssid = "RobotCarAP";
const char* password = "12345678";

void setup() {
  Serial.begin(115200);

  // Set motor pins as output
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Initialize the motors to be off
  stopCar();

   // Initialize load cell
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale();   // Start with default value (no calibration)
  scale.tare();        // Reset the scale to 0

  // Set the calibration factor (adjust this until you get correct readings)
  scale.set_scale(CALIBRATION_FACTOR);

  // Set up Access Point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point started.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Set up server routes
  server.on("/", handleRoot);
  server.on("/forward", handleForward);
  server.on("/backward", handleBackward);
  server.on("/left", handleLeft);
  server.on("/right", handleRight);
  server.on("/stop", handleStop);
  server.on("/setSpeed", handleSetSpeed);
  server.on("/getWeight", handleGetWeight);

  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  server.handleClient();
}

// HTML and JavaScript for control interface
void handleRoot() {
  String html = "<html><head><title>Robot Car Control</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; }";
  html += "button { padding: 10px 20px; margin: 5px; font-size: 16px; }";
  html += "input[type=range] { width: 80%; margin: 20px 0; }";
  html += "#weight { font-size: 20px; color: blue; }";
  html += "#warning { font-size: 20px; color: red; display: none; }";
  html += "</style>";
  html += "</head><body>";
  html += "<h1>Control the Robot Car</h1>";
  html += "<p>Current Weight: <span id=\"weight\">0</span> g</p>";
  html += "<p id=\"warning\">Weight exceeds limit! Motors are blocked.</p>";
  html += "<button onclick=\"sendCommand('forward')\" id=\"forwardBtn\">Forward</button><br>";
  html += "<button onclick=\"sendCommand('left')\" id=\"leftBtn\">Left</button>";
  html += "<button onclick=\"sendCommand('stop')\" id=\"stopBtn\">Stop</button>";
  html += "<button onclick=\"sendCommand('right')\" id=\"rightBtn\">Right</button><br>";
  html += "<button onclick=\"sendCommand('backward')\" id=\"backwardBtn\">Backward</button><br>";
  html += "<label for=\"speed\">Speed: </label>";
  html += "<input type=\"range\" id=\"speed\" name=\"speed\" min=\"0\" max=\"255\" value=\"255\" onchange=\"updateSpeed(this.value)\">";
  html += "<span id=\"speedValue\">255</span>";
  html += "<script>";
  html += "function sendCommand(cmd) {";
  html += "  var xhttp = new XMLHttpRequest();";
  html += "  xhttp.open('GET', '/' + cmd, true);";
  html += "  xhttp.send();";
  html += "}";
  html += "function updateSpeed(value) {";
  html += "  document.getElementById('speedValue').innerText = value;";
  html += "  var xhttp = new XMLHttpRequest();";
  html += "  xhttp.open('GET', '/setSpeed?value=' + value, true);";
  html += "  xhttp.send();";
  html += "}";
  html += "function updateWeight() {";
  html += "  var xhttp = new XMLHttpRequest();";
  html += "  xhttp.onreadystatechange = function() {";
  html += "    if (this.readyState == 4 && this.status == 200) {";
  html += "      var weight = parseFloat(this.responseText);";
  html += "      document.getElementById('weight').innerText = weight;";
  html += "      if (weight > 500) {";
  html += "        document.getElementById('warning').style.display = 'block';";
  html += "      } else {";
  html += "        document.getElementById('warning').style.display = 'none';";
  html += "      }";
  html += "    }";
  html += "  };";
  html += "  xhttp.open('GET', '/getWeight', true);";
  html += "  xhttp.send();";
  html += "}";
  html += "setInterval(updateWeight, 3000);"; // Update weight every 3 seconds
  html += "</script></body></html>";

  server.send(200, "text/html", html);
}

// Function to return the current weight
void handleGetWeight() {
  float weight = scale.get_units(5); // Average 5 readings for stability
  weight+=633;
  server.send(200, "text/plain", String(weight));
}

// Functions to control the car
void handleForward() {
  if (!checkWeight()) {
    server.send(200, "text/plain", "Weight too high, cannot move forward");
    return;
  }
  applySpeed();  // Apply current speed value
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  server.send(200, "text/plain", "Moving Forward");
}

void handleBackward() {
  if (!checkWeight()) {
    server.send(200, "text/plain", "Weight too high, cannot move backward");
    return;
  }
  applySpeed();  // Apply current speed value
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  server.send(200, "text/plain", "Moving Backward");
}

void handleLeft() {
  if (!checkWeight()) {
    server.send(200, "text/plain", "Weight too high, cannot turn left");
    return;
  }
  applySpeed();  // Apply current speed value
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  server.send(200, "text/plain", "Turning Left");
}

void handleRight() {
  if (!checkWeight()) {
    server.send(200, "text/plain", "Weight too high, cannot turn right");
    return;
  }
  applySpeed();  // Apply current speed value
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  server.send(200, "text/plain", "Turning Right");
}

void handleStop() {
  stopCar();
  server.send(200, "text/plain", "Stopping");
}

void handleSetSpeed() {
  if (server.hasArg("value")) {
    speed = server.arg("value").toInt();
    speed = constrain(speed, 0, 255); // Ensure speed is within valid range
    Serial.print("Speed set to: ");
    Serial.println(speed); // Debugging output to check speed value
    server.send(200, "text/plain", "Speed set to " + String(speed));
  } else {
    server.send(400, "text/plain", "Speed value not provided");
  }
}

void stopCar() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void applySpeed() {
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

bool checkWeight() {
  float weight = scale.get_units(5);
  weight+=633;
  if (weight > WEIGHT_THRESHOLD) {
    digitalWrite(BUZZER_PIN, HIGH); // Turn on the buzzer
    return false;
  }
  digitalWrite(BUZZER_PIN, LOW); // Turn off the buzzer
  return true;
}
