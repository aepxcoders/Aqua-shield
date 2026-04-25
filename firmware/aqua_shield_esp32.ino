#include <WiFi.h>
#include <HTTPClient.h>

// ---------- WIFI ----------
const char* ssid = "Es iPhone";
const char* password = "12345678";

// ---------- BACKEND ----------
// const char* serverURL = "http://172.20.10.14:5000/update";

const char* serverURL = "https://aqua-shield-5jam.onrender.com/update";

// ---------- FLOW SENSOR PINS ----------
#define INLET_FLOW_PIN   18
#define OUTLET_FLOW_PIN  19

// ---------- ULTRASONIC SENSOR PINS ----------
#define INLET_TRIG_PIN   5
#define INLET_ECHO_PIN   17

#define OUTLET_TRIG_PIN  16
#define OUTLET_ECHO_PIN  4

// ---------- TURBIDITY PIN ----------
#define TURBIDITY_PIN    34

// ---------- VARIABLES ----------
volatile unsigned long inletPulseCount = 0;
volatile unsigned long outletPulseCount = 0;

float inletFlowRate = 0.0;
float outletFlowRate = 0.0;

float inletDistance = 0.0;
float outletDistance = 0.0;

int turbidityValue = 0;
String statusText = "Normal";

unsigned long lastCheckTime = 0;
unsigned long lastSendTime = 0;

// ---------- SETTINGS ----------
const unsigned long checkInterval = 500;
const unsigned long sendInterval = 500;

// ---------- FLOW INTERRUPTS ----------
void IRAM_ATTR countInletPulse() {
  inletPulseCount++;
}

void IRAM_ATTR countOutletPulse() {
  outletPulseCount++;
}

// ---------- WIFI CONNECT ----------
void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi Connection Failed");
  }
}

// ---------- ULTRASONIC FUNCTION ----------
float readDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 15000);

  if (duration == 0) {
    return -1;
  }

  return duration * 0.0343 / 2.0;
}

// ---------- WATER TYPE ----------
String getWaterType(int turbidity) {
  if (turbidity < 1200) {
    return "Clean Water";
  } else if (turbidity < 2500) {
    return "Mixed Water";
  } else {
    return "Contaminated Water";
  }
}

// ---------- QUALITY LEVEL ----------
String getQualityLevel(int turbidity) {
  if (turbidity < 1200) {
    return "Low Risk";
  } else if (turbidity < 2500) {
    return "Moderate Risk";
  } else {
    return "High Risk";
  }
}

// ---------- PIPELINE STATUS LOGIC ----------
String calculateStatus(float flowLossPercent, float levelMismatch) {
  float severity = max(flowLossPercent, levelMismatch);

  if (flowLossPercent <= 10 && levelMismatch <= 10) {
    return "Normal";
  } 
  else if (severity <= 15) {
    return "Monitor";
  } 
  else if (severity <= 20) {
    return "Service Required";
  } 
  else if (severity <= 30) {
    return "High Priority";
  } 
  else {
    return "Immediate Resolve";
  }
}

// ---------- RECOMMENDATION ----------
String getRecommendation(String status) {
  if (status == "Normal") {
    return "Readings are within the 10-unit margin. Continue normal monitoring.";
  } 
  else if (status == "Monitor") {
    return "Minor mismatch detected. Keep monitoring and inspect during routine check.";
  } 
  else if (status == "Service Required") {
    return "Leakage is above acceptable margin. Schedule repair soon.";
  } 
  else if (status == "High Priority") {
    return "Water loss is serious. Dispatch maintenance team and resolve same day.";
  } 
  else {
    return "Critical leakage detected. Isolate the pipeline section and repair immediately.";
  }
}

// ---------- SEND DATA TO DASHBOARD ----------
void sendDataToDashboard(float inletFlow, float outletFlow, float inletDist, float outletDist, int turbidity) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    connectWiFi();
    return;
  }

  float flowDifference = inletFlow - outletFlow;
  if (flowDifference < 0) flowDifference = 0;

  float flowLossPercent = 0;
  if (inletFlow > 0) {
    flowLossPercent = (flowDifference / inletFlow) * 100.0;
  }

  float levelDifference = outletDist - inletDist;
  if (levelDifference < 0) levelDifference = 0;

  String waterType = getWaterType(turbidity);
  String qualityLevel = getQualityLevel(turbidity);

  String status = calculateStatus(flowLossPercent, levelDifference);
  String recommendation = getRecommendation(status);

  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");

  String jsonData = "{";
  jsonData += "\"flow1\":" + String(inletFlow, 2) + ",";
  jsonData += "\"flow2\":" + String(outletFlow, 2) + ",";
  jsonData += "\"loss\":" + String(flowDifference, 2) + ",";
  jsonData += "\"flowLossPercent\":" + String(flowLossPercent, 2) + ",";
  jsonData += "\"sourceDrop\":" + String(inletDist, 2) + ",";
  jsonData += "\"outletRise\":" + String(outletDist, 2) + ",";
  jsonData += "\"levelMismatch\":" + String(levelDifference, 2) + ",";
  jsonData += "\"turbidity\":" + String(turbidity) + ",";
  jsonData += "\"waterType\":\"" + waterType + "\",";
  jsonData += "\"qualityLevel\":\"" + qualityLevel + "\",";
  jsonData += "\"status\":\"" + status + "\",";
  jsonData += "\"recommendation\":\"" + recommendation + "\"";
  jsonData += "}";

  int httpResponseCode = http.POST(jsonData);

  Serial.print("HTTP Code: ");
  Serial.println(httpResponseCode);

  Serial.print("Sent JSON: ");
  Serial.println(jsonData);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("Server Response: ");
    Serial.println(response);
  } else {
    Serial.println("Failed to send data to dashboard");
  }

  http.end();
}

void setup() {
  Serial.begin(115200);

  pinMode(INLET_FLOW_PIN, INPUT_PULLUP);
  pinMode(OUTLET_FLOW_PIN, INPUT_PULLUP);

  pinMode(INLET_TRIG_PIN, OUTPUT);
  pinMode(INLET_ECHO_PIN, INPUT);

  pinMode(OUTLET_TRIG_PIN, OUTPUT);
  pinMode(OUTLET_ECHO_PIN, INPUT);

  pinMode(TURBIDITY_PIN, INPUT);

  attachInterrupt(digitalPinToInterrupt(INLET_FLOW_PIN), countInletPulse, FALLING);
  attachInterrupt(digitalPinToInterrupt(OUTLET_FLOW_PIN), countOutletPulse, FALLING);

  connectWiFi();

  Serial.println("Aqua-Shield Pipeline Monitoring Started");
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastCheckTime >= checkInterval) {
    noInterrupts();

    unsigned long inletPulses = inletPulseCount;
    unsigned long outletPulses = outletPulseCount;

    inletPulseCount = 0;
    outletPulseCount = 0;

    interrupts();

    inletFlowRate = inletPulses / 7.5;
    outletFlowRate = outletPulses / 7.5;

    inletDistance = readDistance(INLET_TRIG_PIN, INLET_ECHO_PIN);
    outletDistance = readDistance(OUTLET_TRIG_PIN, OUTLET_ECHO_PIN);

    turbidityValue = analogRead(TURBIDITY_PIN);

    Serial.println("---------- AQUA-SHIELD DATA ----------");
    Serial.print("Inlet Flow: ");
    Serial.println(inletFlowRate);

    Serial.print("Outlet Flow: ");
    Serial.println(outletFlowRate);

    Serial.print("Inlet Distance: ");
    Serial.println(inletDistance);

    Serial.print("Outlet Distance: ");
    Serial.println(outletDistance);

    Serial.print("Turbidity: ");
    Serial.println(turbidityValue);

    Serial.println("--------------------------------------");

    lastCheckTime = currentTime;
  }

  if (currentTime - lastSendTime >= sendInterval) {
    sendDataToDashboard(
      inletFlowRate,
      outletFlowRate,
      inletDistance,
      outletDistance,
      turbidityValue
    );

    lastSendTime = currentTime;
  }
}