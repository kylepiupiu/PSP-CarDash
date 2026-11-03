#include <WiFi.h>
#include <WiFiUdp.h>
#include <math.h>

static const char *AP_SSID = "PSP-CAR-DASH";
static const uint16_t UDP_PORT = 4210;

WiFiUDP udp;
IPAddress broadcastIp(192, 168, 4, 255);

uint32_t seqNo = 0;
uint32_t lastSend = 0;

void setup() {
  Serial.begin(115200);
  delay(300);

  WiFi.mode(WIFI_AP);

  // No password: compatible with the PSP's old Wi-Fi stack.
  bool ok = WiFi.softAP(AP_SSID);

  Serial.println();
  Serial.println("PSP CAR DASH v0.3 ESP32 LINK");
  Serial.printf("SoftAP: %s\n", ok ? "OK" : "FAILED");
  Serial.printf("SSID: %s\n", AP_SSID);
  Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("UDP: %u\n", UDP_PORT);

  udp.begin(UDP_PORT);
}

void loop() {
  const uint32_t now = millis();

  if (now - lastSend < 100)
    return;

  lastSend = now;

  const float t = now / 1000.0f;

  int speed = 78 + (int)(20.0f * sinf(t * 0.65f));
  int rpm = 2700 + (int)(1050.0f * sinf(t * 1.05f));
  int coolant = 92 + (int)(2.0f * sinf(t * 0.09f));
  int throttle = 29 + (int)(20.0f * sinf(t * 0.81f));
  int voltage10 = 141 + (int)(2.0f * sinf(t * 0.17f));
  int intake = 31 + (int)(2.0f * sinf(t * 0.21f));
  int load = 42 + (int)(18.0f * sinf(t * 0.73f));
  int fuel10 = 70 + (int)(18.0f * sinf(t * 0.58f));
  int oilTemp = 96 + (int)(2.0f * sinf(t * 0.11f));
  int oilPressure10 = 36 + rpm / 1800;

  if (speed < 0) speed = 0;
  if (rpm < 700) rpm = 700;
  if (throttle < 0) throttle = 0;
  if (throttle > 100) throttle = 100;
  if (load < 0) load = 0;
  if (load > 100) load = 100;

  char packet[180];

  snprintf(
    packet,
    sizeof(packet),
    "PCD1,%lu,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
    (unsigned long)seqNo++,
    speed,
    rpm,
    coolant,
    throttle,
    voltage10,
    intake,
    load,
    fuel10,
    oilTemp,
    oilPressure10
  );

  udp.beginPacket(broadcastIp, UDP_PORT);
  udp.write((const uint8_t *)packet, strlen(packet));
  udp.endPacket();

  Serial.println(packet);
}
