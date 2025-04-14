#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Motor control pins
#define IN1 D1
#define IN2 D2
#define IN3 D3
#define IN4 D4
#define ENA D5
#define ENB D6

// Variables to manage control
String controllingClient = "";   // IP/MAC of the controlling client
String policeClient = "";        // IP/MAC of the police client
String policePassword = "police123";  // Password to assign police control
bool policeConnected = false;

// Wi-Fi AP credentials
const char *ssid = "Car_Hotspot";
const char *password = "carpassword";

// Web server on port 80
ESP8266WebServer server(80);

// HTML page (embedded)
const char* webPage = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Car Control</title>
  <style>
    body { font-family: Arial, sans-serif; }
    button { margin: 5px; }
    #status { margin-top: 20px; color: green; }
    #error { margin-top: 20px; color: red; }
  </style>
</head>
<body>
  <h1>Car Control Interface</h1>

  <!-- Control buttons -->
  <button onclick="sendCommand('forward')">Forward</button>
  <button onclick="sendCommand('backward')">Backward</button>
  <button onclick="sendCommand('left')">Left</button>
  <button onclick="sendCommand('right')">Right</button>
  <button onclick="sendCommand('stop')">Stop</button>

  <!-- Speed control -->
  <input type="range" id="speed" min="0" max="255" value="0" onchange="updateSpeedDisplay()">
  <span id="speedValue">0</span>
  <input type="hidden" id="currentSpeed" value="0">

  <!-- Police Login -->
  <h3>Login as Police:</h3>
  <input type="password" id="police_password" placeholder="Police Password">
  <button onclick="loginAsPolice()">Login</button>

  <div id="status"></div>
  <div id="error"></div>

  <script>
    function updateStatus(message, isError) {
      const statusElement = document.getElementById('status');
      const errorElement = document.getElementById('error');
      if (isError) {
        errorElement.innerText = message;
        statusElement.innerText = '';
      } else {
        statusElement.innerText = message;
        errorElement.innerText = '';
      }
    }

    function sendCommand(command) {
      fetch(`/${command}`, { method: 'GET' })
        .then(response => response.text())
        .then(data => updateStatus(data))
        .catch(error => updateStatus('Error sending command.', true));
    }

    function updateSpeedDisplay() {
      const speed = document.getElementById('speed').value;
      document.getElementById('speedValue').innerText = speed;
      setSpeed(speed);
    }

    function setSpeed(speed) {
      fetch(`/setSpeed?value=${speed}`, { method: 'GET' })
        .then(response => response.text())
        .then(data => updateStatus(data))
        .catch(error => updateStatus('Error setting speed.', true));
    }

    function loginAsPolice() {
      const password = document.getElementById('police_password').value;
      fetch(`/policeLogin?password=${password}`, { method: 'GET' })
        .then(response => response.text())
        .then(data => updateStatus(data))
        .catch(error => updateStatus('Error logging in as police.', true));
    }
  </script>
</body>
</html>
)=====";

// Function to get the client's IP address
String getClientIP() {
  return server.client().remoteIP().toString();
}

// Function to detect and handle client connections
void handleClientConnections() {
  int clientCount = WiFi.softAPgetStationNum();
  if (clientCount == 1) {
      policeConnected = false;
      controllingClient = ""; // Only one client, give them control
  }
}

// Handle forward command
void handleForward() {
  String clientIP = getClientIP();
  if (clientIP == controllingClient || (!policeConnected && controllingClient == "")) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    server.send(200, "text/plain", "Moving Forward");
  } else {
    server.send(403, "text/plain", "Access Denied.");
  }
}

// Handle backward command
void handleBackward() {
  String clientIP = getClientIP();
  if (clientIP == controllingClient || (!policeConnected && controllingClient == "")) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    server.send(200, "text/plain", "Moving Backward");
  } else {
    server.send(403, "text/plain", "Access Denied.");
  }
}

// Handle left command
void handleLeft() {
  String clientIP = getClientIP();
  if (clientIP == controllingClient || (!policeConnected && controllingClient == "")) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    server.send(200, "text/plain", "Turning Left");
  } else {
    server.send(403, "text/plain", "Access Denied.");
  }
}

// Handle right command
void handleRight() {
  String clientIP = getClientIP();
  if (clientIP == controllingClient || (!policeConnected && controllingClient == "")) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    server.send(200, "text/plain", "Turning Right");
  } else {
    server.send(403, "text/plain", "Access Denied.");
  }
}

// Handle stop command
void handleStop() {
  String clientIP = getClientIP();
  if (clientIP == controllingClient || (!policeConnected && controllingClient == "")) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    server.send(200, "text/plain", "Car Stopped");
  } else {
    server.send(403, "text/plain", "Access Denied.");
  }
}

// Handle speed command
void handleSetSpeed() {
  String clientIP = getClientIP();
  if (server.hasArg("value")) {
    int speed = server.arg("value").toInt();
    if (clientIP == controllingClient || (!policeConnected && controllingClient == "")) {
      analogWrite(ENA, speed);
      analogWrite(ENB, speed);
      server.send(200, "text/plain", "Speed set to " + String(speed));
    } else {
      server.send(403, "text/plain", "Access Denied.");
    }
  } else {
    server.send(400, "text/plain", "Speed value required.");
  }
}

// Handle police login
void handlePoliceLogin() {
  if (server.hasArg("password")) {
    String enteredPassword = server.arg("password");
    if (enteredPassword == policePassword) {
      policeConnected = true;
      policeClient = getClientIP();  // Capture the police IP
      controllingClient = policeClient;  // Police has control
      server.send(200, "text/plain", "Police control granted.");
    } else {
      server.send(403, "text/plain", "Incorrect password.");
    }
  } else {
    server.send(400, "text/plain", "Password required.");
  }
}

void setup() {
  Serial.begin(115200);

  // Set up motor control pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  // Set up Wi-Fi
  WiFi.softAP(ssid, password);
  Serial.println("Wi-Fi Access Point started");

  // Set up server routes
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", webPage);
  });

  server.on("/forward", HTTP_GET, handleForward);
  server.on("/backward", HTTP_GET, handleBackward);
  server.on("/left", HTTP_GET, handleLeft);
  server.on("/right", HTTP_GET, handleRight);
  server.on("/stop", HTTP_GET, handleStop);
  server.on("/setSpeed", HTTP_GET, handleSetSpeed);
  server.on("/policeLogin", HTTP_GET, handlePoliceLogin);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  handleClientConnections();
}
