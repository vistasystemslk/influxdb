#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTT.h> // Replaces PubSubClient
#include <time.h>

// --- Network & Broker Details ---
const char* ssid = "STARLINK";
const char* password = "1234Qwer";

const char* mqtt_server = "40bbfbeb8fd548a5b4c9d54d2057bda6.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "kasun";
const char* mqtt_password = "kasua1U23";

// --- Device Details ---
const char* device_id = "EQ-001";
const char* telemetry_topic = "equipment/EQ-001/telemetry";

WiFiClientSecure espClient;
// Initialize the MQTT client with a buffer size of 256 bytes
MQTTClient client(256);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void connect() {
  Serial.print("Attempting MQTT connection...");
  while (!client.connect(device_id, mqtt_user, mqtt_password)) {
    Serial.print(".");
    delay(5000);
  }
  Serial.println("connected");
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  
  espClient.setInsecure();
  client.begin(mqtt_server, mqtt_port, espClient);

  // Initialize NTP to fetch UTC time
  configTime(0, 0, "pool.ntp.org");
  
  Serial.print("Waiting for NTP time sync...");
  time_t now = time(nullptr);
  // Wait until the epoch time is realistic
  while (now < 24 * 3600) {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println("\nTime synchronized");
}

void loop() {
  // Required to maintain the connection and process incoming packets
  client.loop();
  delay(10);

  if (!client.connected()) {
    connect();
  }

  // Non-blocking timer
  static unsigned long lastMillis = 0;
  if (millis() - lastMillis > 5000) {
    lastMillis = millis();

    // 1. Get the current precise epoch time
    time_t now;
    time(&now);

    // 2. Read sensors
    float voltage = 230.0 + random(-10, 10) / 10.0;
    float current = 4.5 + random(-5, 5) / 10.0;
    float temperature = 32.0 + random(-20, 20) / 10.0;

    // 3. Inject the timestamp into the JSON payload
    char payload[150];
    snprintf(payload, sizeof(payload), "{\"voltage\":%.1f, \"current\":%.1f, \"temperature\":%.1f, \"timestamp\":%ld}", voltage, current, temperature, now);

    Serial.print("Publishing to ");
    Serial.print(telemetry_topic);
    Serial.print(" (QoS 1): ");
    Serial.println(payload);
    
    // 4. Publish with QoS 1
    // Syntax: publish(topic, payload, retained, qos)
    client.publish(telemetry_topic, payload, false, 1);
  }
}