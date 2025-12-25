// ====================================================================
// ==   KODE ARDUINO PASANGAN UNTUK JUPYTER NOTEBOOK INTERAKTIF      ==
// ====================================================================
// Tugas:
// 1. Menerima perintah PWM dari skrip Python (yang dikontrol slider).
// 2. Menggerakkan motor sesuai perintah PWM tersebut.
// 3. Membaca sensor RPM.
// 4. Mengirim kembali data dalam format "PWM,RPM" yang diharapkan oleh Python.
// --------------------------------------------------------------------

// ===== 1. KONFIGURASI PIN & PARAMETER =====
const int pin_motor_1 = 27;
const int pin_motor_2 = 26;
const int pin_enable = 12; // Pin Kecepatan (PWM)
const int pin_sensor = 13; // Pin Sensor RPM

// PENTING: Sesuaikan nilai ini dengan spesifikasi sensor Anda!
// Jika sensor hanya memberi 1 pulsa per putaran, nilainya 1.0
const float PULSES_PER_REVOLUTION = 1.0;

// ===== 2. VARIABEL GLOBAL =====
volatile int rev_counter = 0;       // Penghitung putaran (volatile karena dipakai di interrupt)
unsigned long last_report_time = 0; // Waktu terakhir pengiriman data
int current_rpm = 0;                // Hasil kalkulasi RPM
int current_pwm_command = 0;        // Menyimpan nilai PWM terakhir yang diterima dari Python

// Konfigurasi PWM untuk ESP32
const int pwmChannel = 0;
const int pwmFreq = 30000;
const int pwmResolution = 8; // 8-bit = 0-255

// ===== 3. FUNGSI INTERRUPT (Sangat Cepat) =====
// Fungsi ini akan dipanggil oleh hardware setiap kali sensor mendeteksi pulsa
void IRAM_ATTR countPulse()
{
    rev_counter++;
}

// ===== 4. SETUP (Dijalankan sekali saat ESP32 nyala) =====
void setup()
{
    // Mulai komunikasi Serial dengan baud rate yang sama dengan Python
    Serial.begin(115200);

    // Set semua pin motor sebagai OUTPUT
    pinMode(pin_motor_1, OUTPUT);
    pinMode(pin_motor_2, OUTPUT);
    pinMode(pin_enable, OUTPUT);

    // Set pin sensor sebagai INPUT dengan PULLUP internal
    pinMode(pin_sensor, INPUT_PULLUP);

    // Konfigurasi hardware PWM ESP32
    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(pin_enable, pwmChannel);

    // Aktifkan interrupt di pin sensor
    // Panggil 'countPulse' setiap kali sinyal berubah dari HIGH ke LOW
    attachInterrupt(digitalPinToInterrupt(pin_sensor), countPulse, FALLING);

    // Set arah motor default (misalnya maju)
    digitalWrite(pin_motor_1, HIGH);
    digitalWrite(pin_motor_2, LOW);

    // Catat waktu awal untuk loop
    last_report_time = millis();
}

// ===== 5. LOOP (Dijalankan berulang-ulang selamanya) =====
void loop()
{
    // --- TUGAS A: MENERIMA PERINTAH PWM DARI PYTHON ---
    if (Serial.available() > 0)
    {
        // Baca angka integer yang masuk (0-255)
        int received_pwm = Serial.parseInt();

        // Simpan dan terapkan PWM setelah membatasinya ke rentang aman
        current_pwm_command = constrain(received_pwm, 0, 255);
        ledcWrite(pwmChannel, current_pwm_command);

        // Buang sisa karakter (seperti newline '\n') agar buffer serial bersih
        while (Serial.available() > 0)
        {
            Serial.read();
        }
    }

    // --- TUGAS B: MENGHITUNG & MENGIRIM DATA LAPORAN KE PYTHON ---
    // Lakukan ini setiap 500 milidetik (setengah detik)
    if (millis() - last_report_time >= 500)
    {
        // Matikan interrupt sebentar agar data tidak berubah saat sedang dihitung
        detachInterrupt(digitalPinToInterrupt(pin_sensor));

        // Hitung RPM menggunakan rumus yang akurat
        float elapsed_time_min = (millis() - last_report_time) / 60000.0;
        if (elapsed_time_min > 0)
        {
            current_rpm = (rev_counter / PULSES_PER_REVOLUTION) / elapsed_time_min;
        }
        else
        {
            current_rpm = 0;
        }

        // INI BAGIAN KUNCI: Kirim data dengan format "PWM,RPM" yang diharapkan Python
        Serial.print(current_pwm_command);
        Serial.print(",");
        Serial.println(current_rpm);

        // Reset penghitung putaran dan catat waktu untuk perhitungan berikutnya
        rev_counter = 0;
        last_report_time = millis();

        // Nyalakan interrupt lagi
        attachInterrupt(digitalPinToInterrupt(pin_sensor), countPulse, FALLING);
    }
}