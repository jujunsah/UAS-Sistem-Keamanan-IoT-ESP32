#include <WiFi.h>
#include <PubSubClient.h>

// ============================================================
// 1. KONFIGURASI PIN HARDWARE
// ============================================================
#define PIN_SENSOR_ANALOG 36    // Sensor di IO36 (VP)
#define PIN_TOMBOL_EKSTERNAL 27 // Tombol di IO27
#define PIN_LED_ONBOARD 2       // LED Indikator Status

// --- PENTING: KALIBRASI SENSOR ---
// Ganti angka ini sesuai hasil intip di Serial Monitor
// Jika disentuh angkanya turun dibawah 2000, biarkan segini.
int BATAS_AMBANG = 2000; 

// ============================================================
// 2. KONFIGURASI JARINGAN & MQTT
// ============================================================
const char* ssid = "ips.net";    
const char* password = "ipscomp2025";   

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* topik_kirim_status = "kampus/esp32/jujun_11187/status_sensor";
const char* topik_terima_kontrol = "UAS/esp32/jujun_11187/kontrol_led";

WiFiClient espClient;
PubSubClient client(espClient);

// Handle untuk FreeRTOS
TaskHandle_t TaskSensorHandle = NULL;
TaskHandle_t TaskMQTTHandle = NULL;
QueueHandle_t statusQueue;
SemaphoreHandle_t buttonSemaphore;

// Status Sistem (Logika Pintar)
bool sistemAktif = false; // Awalnya mati (Standby)

// ============================================================
// ISR (INTERUPSI TOMBOL)
// ============================================================
void IRAM_ATTR handleTombolISR() {
  xSemaphoreGiveFromISR(buttonSemaphore, NULL);
}

// ============================================================
// FUNGSI PENDUKUNG (WIFI & MQTT)
// ============================================================
void setup_wifi() {
  delay(10); Serial.print("\nKonek WiFi: "); Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("WiFi OK!");
}

// Fungsi Callback: Menerima Perintah dari Internet
void callback(char* topic, byte* payload, unsigned int length) {
  String pesan = "";
  for (int i = 0; i < length; i++) {
    pesan += (char)payload[i];
  }

  Serial.print(">>> PESAN MQTT MASUK: ");
  Serial.println(pesan);

  // LOGIKA KONTROL JARAK JAUH
  if (pesan == "AKTIFKAN") {
    sistemAktif = true;
    digitalWrite(PIN_LED_ONBOARD, HIGH); // Nyalakan LED Merah
    Serial.println("STATUS: Sistem DIAKTIFKAN via Internet");
  } 
  else if (pesan == "NONAKTIFKAN") {
    sistemAktif = false;
    digitalWrite(PIN_LED_ONBOARD, LOW);  // Matikan LED Merah
    Serial.println("STATUS: Sistem DIMATIKAN via Internet");
  }
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Konek MQTT...");
    String clientId = "Jujun-ESP32-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("OK!");
      // Subscribe ke topik kontrol agar bisa terima perintah
      client.subscribe(topik_terima_kontrol);
    } else {
      delay(2000);
    }
  }
}

// ============================================================
// TASK 1: SENSOR (LOGIKA TOUCH/SENTUH)
// ============================================================
void TaskSensor(void *pvParameters) {
  int nilaiRaw;
  char pesanStatus[50]; 

  for (;;) {
    nilaiRaw = analogRead(PIN_SENSOR_ANALOG);
    
    // TAMPILKAN ANGKA ASLI UNTUK CEK KONDISI
    Serial.print(">>> ANGKA SENSOR: ");
    Serial.print(nilaiRaw);
    Serial.print(" | BATAS: ");
    Serial.println(BATAS_AMBANG);

    if (sistemAktif == true) {
        // LOGIKA: Jika nilai TURUN dibawah batas (Disentuh) -> BAHAYA
        if (nilaiRaw < BATAS_AMBANG) {  
           sprintf(pesanStatus, "BAHAYA! Terdeteksi: %d", nilaiRaw);
        } else {
           sprintf(pesanStatus, "AMAN (%d)", nilaiRaw);
        }
    } else {
        strcpy(pesanStatus, "SISTEM SIAP (Standby)");
    }

    xQueueSend(statusQueue, &pesanStatus, portMAX_DELAY);
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Update tiap 1 detik
  }
}

// ============================================================
// TASK 2: MQTT & TOMBOL FISIK
// ============================================================
void TaskMQTT(void *pvParameters) {
  char pesanDiterima[50];
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); // Aktifkan fungsi penerima pesan

  for (;;) {
    if (WiFi.status() != WL_CONNECTED) setup_wifi();
    if (!client.connected()) reconnect_mqtt();
    client.loop();

    // 1. Kirim Status Sensor ke Internet
    if (xQueueReceive(statusQueue, &pesanDiterima, 0) == pdTRUE) {
      Serial.print("[STATUS] "); Serial.println(pesanDiterima);
      client.publish(topik_kirim_status, pesanDiterima);
    }

    // 2. Cek Tombol Fisik (Manual Control)
    if (xSemaphoreTake(buttonSemaphore, 0) == pdTRUE) {
       Serial.println("\n>>> TOMBOL DITEKAN! GANTI STATUS SISTEM <<<");
       
       sistemAktif = !sistemAktif; // Balik status (Hidup <-> Mati)

       if (sistemAktif) {
          digitalWrite(PIN_LED_ONBOARD, HIGH); // Nyalakan LED Merah
          client.publish(topik_kirim_status, "EVENT: SISTEM DIAKTIFKAN MANUAL");
       } else {
          digitalWrite(PIN_LED_ONBOARD, LOW);  // Matikan LED
          client.publish(topik_kirim_status, "EVENT: SISTEM DIMATIKAN MANUAL");
       }
       vTaskDelay(500 / portTICK_PERIOD_MS); // Debounce
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ============================================================
// SETUP UTAMA
// ============================================================
void setup() {
  Serial.begin(115200);
  
  pinMode(PIN_SENSOR_ANALOG, INPUT);
  pinMode(PIN_TOMBOL_EKSTERNAL, INPUT_PULLUP);
  pinMode(PIN_LED_ONBOARD, OUTPUT);
  digitalWrite(PIN_LED_ONBOARD, LOW); // Default Mati

  setup_wifi();

  statusQueue = xQueueCreate(5, sizeof(char) * 50);
  buttonSemaphore = xSemaphoreCreateBinary();

  attachInterrupt(digitalPinToInterrupt(PIN_TOMBOL_EKSTERNAL), handleTombolISR, FALLING);

  // Buat Task (Urutan & Nama Fungsi Sudah Benar)
  xTaskCreate(TaskSensor, "SensorTask", 3072, NULL, 1, &TaskSensorHandle);
  xTaskCreate(TaskMQTT,   "MQTTTask",   4096, NULL, 2, &TaskMQTTHandle);
}

void loop() {}