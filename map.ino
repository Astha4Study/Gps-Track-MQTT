#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>

// Konfigurasi WiFi
const char* ssid = "yourWifi";          // Nama WiFi
const char* password = "wifiPassword"; // Password WiFi

// Konfigurasi Server MQTT
const char* mqttServer = "yourMqttServer"; // Alamat MQTT Server
const int mqttPort = yourPort;             // Port MQTT Server
const char* mqttUser = "yourUser";         // Username MQTT
const char* mqttPassword = "UserPassword"; // Password MQTT
const char* pubTopic = "YourTopic";        // Topik untuk mempublikasikan data GPS

// Konfigurasi GPS dan Pin ESP8266
#define RX_PIN D5   // Pin RX untuk GPS
#define TX_PIN D6   // Pin TX untuk GPS
#define GPS_BAUD 9600 // Baudrate untuk komunikasi serial dengan GPS

// Interval waktu untuk mengirim data (ms)
const unsigned long publishInterval = 25000;

// Objek untuk WiFi, MQTT, dan GPS
WiFiClientSecure espClient;         // Koneksi WiFi untuk MQTT
PubSubClient client(espClient);     // Client MQTT
TinyGPSPlus gps;                    // Objek GPS
SoftwareSerial gpsSerial(RX_PIN, TX_PIN); // Objek komunikasi serial untuk GPS

// Variabel untuk mencatat waktu terakhir pengiriman data
unsigned long lastPublishTime = 0;

void setup() {
  // Inisialisasi Serial Monitor dan Serial GPS
  Serial.begin(115200);
  gpsSerial.begin(GPS_BAUD);

  Serial.println("*****************************************************");
  Serial.println("Program Start: ESP8266 publishes GPS position over MQTT");
  Serial.printf("Connecting to WiFi: %s\n", ssid);

  // Koneksi ke WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\n-> WiFi connected");
  Serial.print("-> IP Address: ");
  Serial.println(WiFi.localIP());

  // Konfigurasi MQTT tanpa validasi sertifikat
  espClient.setInsecure();

  // Konfigurasi server MQTT dan callback
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void loop() {
  // Memastikan koneksi MQTT tetap aktif
  if (!client.connected()) reconnect();
  client.loop(); // Menjaga koneksi MQTT

  // Membaca data GPS dan mengirim jika interval tercapai
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read()) && gps.location.isUpdated()) {
      unsigned long now = millis();
      if (now - lastPublishTime >= publishInterval) {
        publishGPSData();
        lastPublishTime = now; // Update waktu pengiriman terakhir
      }
    }
  }

  // Jika GPS tidak tersedia dalam 5 detik
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No GPS detected: Check wiring."));
    while (true); // Hentikan program jika GPS tidak terdeteksi
  }
}

// Fungsi untuk mengirim data GPS ke MQTT
void publishGPSData() {
  if (gps.location.isValid()) {
    double latitude = gps.location.lat();
    double longitude = gps.location.lng();

    // Membuat payload dalam format JSON
    char mqtt_payload[150];
    snprintf(mqtt_payload, sizeof(mqtt_payload),
             "{\"latitude\": %.6f, \"longitude\": %.6f, \"satellites\": %d, \"altitude\": %.2f, \"speed\": %.2f}",
             latitude, longitude, gps.satellites.value(), gps.altitude.meters(), gps.speed.kmph());

    // Mencetak dan mengirim data ke MQTT
    Serial.println("********** Publish MQTT Data **********");
    Serial.print("Publish Message: ");
    Serial.println(mqtt_payload);

    client.publish(pubTopic, mqtt_payload); // Kirim data ke topik MQTT
    Serial.println("> MQTT Data Published");
    Serial.println("****************************************");
  } else {
    Serial.println(F("GPS Data Invalid"));
  }
}

// Callback untuk menangani pesan yang diterima dari MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("Message arrived [%s]: ", topic);
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Fungsi untuk menghubungkan kembali ke broker MQTT jika terputus
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Menghubungkan ke MQTT menggunakan ID, username, dan password
    if (client.connect("ESP8266Client", mqttUser, mqttPassword)) {
      Serial.println("-> MQTT Connected");
      client.subscribe(pubTopic); // Berlangganan ke topik GPS
    } else {
      Serial.printf("Failed, rc=%d. Retrying in 5 seconds...\n", client.state());
      delay(5000); // Tunggu sebelum mencoba lagi
    }
  }
}
