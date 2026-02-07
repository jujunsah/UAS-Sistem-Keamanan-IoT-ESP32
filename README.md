# UAS MIKROPROSESOR: SISTEM KEAMANAN IOT (ESP32)

**Nama:** Jujun Sahroni  
**NPM:** 23552011187  
**Mata Kuliah:** Mikroprosesor & Antarmuka  

![Foto Alat](LINK_FOTO_ALAT_DISINI)  
*(Foto Rangkaian Alat)*

---

## Deskripsi Proyek
Proyek ini adalah sistem keamanan cerdas yang menggunakan **Sensor Sentuh Analog** (Resistor Divider) dan mikrokontroler **ESP32**. Sistem ini memiliki fitur **Keamanan Aktif**, di mana alarm hanya akan bekerja jika diaktifkan melalui tombol fisik atau perintah MQTT dari internet.

### Fitur Utama
1.  **Sensor Sentuh Analog:** Mendeteksi sentuhan kulit manusia berdasarkan penurunan nilai ADC (Threshold < 2000).
2.  **Kontrol Ganda:** Bisa dinyalakan/dimatikan lewat **Tombol Fisik** atau **Dashboard MQTT**.
3.  **Monitoring Real-time:** Status "AMAN" atau "BAHAYA" dikirim ke HiveMQ setiap detik.
4.  **Kalibrasi Angka:** Menampilkan nilai raw sensor di Serial Monitor untuk akurasi.

---

## Komponen Hardware
| Komponen | Fungsi | Pin ESP32 |
| :--- | :--- | :--- |
| **Wemos D1 R32** | Otak pemrosesan & WiFi | - |
| **Resistor (2x)** | Sensor Sentuh (Voltage Divider) | IO36 (VP) |
| **Push Button** | Saklar On/Off Sistem | IO27 |
| **LED Built-in** | Indikator Status (Nyala=Waspada) | IO2 |

---

## 📄 LAPORAN LENGKAP

### BAB I: PENDAHULUAN
**1.1 Latar Belakang** Keamanan merupakan aspek vital dalam kehidupan sehari-hari. Seiring berkembangnya teknologi Internet of Things (IoT), sistem keamanan konvensional dapat ditingkatkan menjadi sistem cerdas yang dapat dipantau dan dikendalikan dari jarak jauh.

**1.2 Tujuan** Membuat purwarupa sistem keamanan yang mendeteksi sentuhan fisik dan dapat dipantau via Internet.

### BAB II: CARA KERJA
Sistem bekerja menggunakan **FreeRTOS** dengan pembagian tugas:
1.  **TaskSensor:** Membaca nilai analog. Jika Nilai < 2000 (disentuh), status menjadi BAHAYA.
2.  **TaskMQTT:** Mengirim status ke internet dan menerima perintah kontrol "AKTIFKAN" atau "NONAKTIFKAN".

### BAB III: HASIL PENGUJIAN
* **Nilai Diam:** ± 2400 (Stabil)
* **Nilai Disentuh:** ± 1500 (Turun)
* **Status MQTT:** Data terkirim real-time ke broker `hivemq.com`.

---

## Dokumentasi & Bukti
*(Silakan upload foto alat atau screenshot Serial Monitor di sini)*

---

## Daftar Pustaka
1. Espressif Systems. (2024). *ESP32 Series Datasheet*.
2. HiveMQ. (2024). *MQTT Essentials*.
3. Kurniawan, A. (2021). *Internet of Things dengan ESP32*.
