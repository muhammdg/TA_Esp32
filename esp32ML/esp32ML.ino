#include <max6675.h>
#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <FirebaseESP32.h> // Memasukkan Library Firebase ESP32

// Provide the token generation process info.
#include "addons/TokenHelper.h"

// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// #define WIFI_SSID "OldCity_Fam_plus"
// #define WIFI_PASSWORD "Ppopoopo12Zxx"

// #define WIFI_SSID "pengungsian"
// #define WIFI_PASSWORD "awasadabolu"

#define WIFI_SSID "reca"
#define WIFI_PASSWORD "password"

// #define WIFI_SSID "Redmi A1" 
// #define WIFI_PASSWORD "cobainkak"

// Insert Firebase project API Key
#define API_KEY "AIzaSyDByniTMlVXLBN9puuef9n_e8fX1svB7ss"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://bara-database-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Inisialisasi objek LCD dengan alamat I2C 0x27, 16 kolom, dan 2 baris
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Definisi pin untuk sensor suhu MAX6675 di ESP32
#define SCK 5
#define CS 18
#define SO 19

// Inisialisasi objek MAX6675
MAX6675 thermocouple(SCK, CS, SO);

// Define Firebase Data object.
FirebaseData fbdo;

// Define firebase authentication.
FirebaseAuth auth;

// Define firebase configuration.
FirebaseConfig config;

// Millis variable to send/store data to firebase database.
unsigned long sendDataPrevMillis = 0;

// Boolean variable for sign in status.
bool signupOK = false;

void setup() {
  Serial.begin(115200);

  // Mulai LCD
  lcd.init();
  lcd.backlight();

  // Tampilkan pesan awal di LCD
  lcd.print("Connecting");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Clear the initial message
  lcd.clear();

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  // Sign up
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());//menampilkan pesan kesalahan pendaftaran yang disimpan dalam objek config 
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 200 || sendDataPrevMillis == 0)) { //memperbarui waktu terakhir kali data dikirim, yang digunakan untuk menghitung waktu interval pengiriman berikutnya.
    sendDataPrevMillis = millis();

    // Baca suhu dari sensor MAX6675
    float temperature = thermocouple.readCelsius();

    // Koefisien polinomial
    float weight_x10 = -3.027154768823266e-16;
    float weight_x9 = 1.4821485582020052e-13;
    float weight_x8 = -2.8575840957978992e-11;
    float weight_x7 = 2.709176396087686e-09;
    float weight_x6 = -1.263056173780334e-07;
    float weight_x5 = 2.318137574234441e-06;
    float weight_x4 = 1.4790077371335862e-07;
    float weight_x3 = 5.34117044842422e-09;
    float weight_x2 = 1.3569687512139095e-10;
    float weight_x1 = 9.488204883082852e-09;
    float bias = 78.2335622527153;

    // Hitung waktu yang tersisa berdasarkan persamaan polinomial
    float remainingTime = weight_x10 * pow(temperature, 10) +
                          weight_x9 * pow(temperature, 9) +
                          weight_x8 * pow(temperature, 8) +
                          weight_x7 * pow(temperature, 7) +
                          weight_x6 * pow(temperature, 6) +
                          weight_x5 * pow(temperature, 5) +
                          weight_x4 * pow(temperature, 4) +
                          weight_x3 * pow(temperature, 3) +
                          weight_x2 * pow(temperature, 2) +
                          weight_x1 * temperature +
                          bias;

    // Tampilkan suhu di LCD
    lcd.setCursor(0, 0);
    lcd.print("Temp : ");
    lcd.print(temperature);
    lcd.print("C");

    // Tampilkan durasi di LCD
    lcd.setCursor(0, 1);
    lcd.print("Time : ");
    if (temperature > 120 || remainingTime <= 0) {
      lcd.print("Finish      "); // Tambahkan spasi untuk menimpa karakter sebelumnya
    } else {
      lcd.print(remainingTime);
      lcd.print(" minutes  "); // Tambahkan spasi untuk menimpa karakter sebelumnya
    }

    // Print suhu dan durasi ke Serial Monitor
    Serial.print("Temp: ");
    Serial.print(temperature);
    Serial.print(" C, Time Predict: ");
    if (temperature > 120 || remainingTime <= 0) {
      Serial.println("Finish");
    } else {
      Serial.print(remainingTime);
      Serial.println(" minutes");
    }

    delay(2000);

    // Use Firebase.RTDB directly
    if (Firebase.setFloat(fbdo, "ESP32/Temp", temperature)) {
      Serial.println();
      Serial.print(temperature);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }

    String timeStr = (temperature > 120 || remainingTime <= 0) ? "Finish" : String(remainingTime) + " minutes";

    if (Firebase.setString(fbdo, "ESP32/Time", timeStr)) {
      Serial.println();
      Serial.print(timeStr);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
    } else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }
  }
}
