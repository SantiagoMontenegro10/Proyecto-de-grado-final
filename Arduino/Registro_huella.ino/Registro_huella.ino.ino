#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <Wire.h>

// ================= CONFIG =================
#define RXD2 16
#define TXD2 17
#define SERVO_PIN 13

// ===== WIFI =====
const char* ssid = "MONTENEGRO LOPEZ";
const char* password = "03101523";

// ===== SERVIDOR =====
String server = "http://192.168.1.16/gym/validar_huella.php";

// ===== OBJETOS =====
LiquidCrystal_I2C lcd(0x3F, 16, 2);
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger(&mySerial);
Servo puerta;

// ===== VARIABLES =====
bool modoRegistro = false;
uint8_t idRegistro = 1;

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  mySerial.begin(57600, SERIAL_8N1, RXD2, TXD2);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  puerta.attach(SERVO_PIN);
  puerta.write(0);

  lcd.print("Iniciando...");
  delay(2000);

  // WIFI
  WiFi.begin(ssid, password);
  lcd.clear();
  lcd.print("Conectando...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  lcd.clear();
  lcd.print("WiFi OK");
  delay(1500);

  // Sensor
  if (!finger.verifyPassword()) {
    lcd.clear();
    lcd.print("Error huella");
    while (1);
  }

  finger.getTemplateCount();
  idRegistro = finger.templateCount + 1;

  lcd.clear();
  lcd.print("Sistema listo");
  lcd.setCursor(0, 1);
  lcd.print("Coloque huella");

  Serial.println("Sistema listo");
  Serial.println("r = Registrar huella");
}

// ================= LOOP =================
void loop() {

  // ---- COMANDO ADMIN ----
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'r') {
      modoRegistro = true;
      registrarHuella();
    }
  }

  // ---- MODO NORMAL ----
  if (!modoRegistro) {
    int id = leerHuella();
    if (id > 0) {
      validarServidor(id);
    }
  }
}

// ================= FUNCIONES =================

int leerHuella() {
  if (finger.getImage() != FINGERPRINT_OK) return -1;
  if (finger.image2Tz() != FINGERPRINT_OK) return -1;

  if (finger.fingerFastSearch() != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Acceso denegado");
    delay(2000);
    lcd.clear();
    lcd.print("Coloque huella");
    return -1;
  }

  return finger.fingerID;
}

void registrarHuella() {
  lcd.clear();
  lcd.print("Registrar ID:");
  lcd.setCursor(0, 1);
  lcd.print(idRegistro);
  delay(2000);

  while (finger.getImage() != FINGERPRINT_OK);
  finger.image2Tz(1);

  lcd.clear();
  lcd.print("Quite el dedo");
  delay(2000);

  while (finger.getImage() != FINGERPRINT_NOFINGER);

  lcd.clear();
  lcd.print("Coloque otra");
  while (finger.getImage() != FINGERPRINT_OK);
  finger.image2Tz(2);

  if (finger.createModel() == FINGERPRINT_OK &&
      finger.storeModel(idRegistro) == FINGERPRINT_OK) {

    lcd.clear();
    lcd.print("Huella OK");
    lcd.setCursor(0, 1);
    lcd.print("ID: ");
    lcd.print(idRegistro);

    Serial.print("Huella registrada ID=");
    Serial.println(idRegistro);
    idRegistro++;
  } else {
    lcd.clear();
    lcd.print("Error registro");
  }

  delay(3000);
  lcd.clear();
  lcd.print("Coloque huella");
  modoRegistro = false;
}

void validarServidor(int id) {
  HTTPClient http;
  String url = server + "?id=" + String(id);

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String resp = http.getString();
    resp.trim();

    if (resp == "OK") {
      lcd.clear();
      lcd.print("Acceso OK");
      puerta.write(90);
      delay(3000);
      puerta.write(0);
    } else {
      lcd.clear();
      lcd.print("Membresia venc");
      delay(3000);
    }
  } else {
    lcd.clear();
    lcd.print("Error servidor");
    delay(3000);
  }

  http.end();
  lcd.clear();
  lcd.print("Coloque huella");
}
