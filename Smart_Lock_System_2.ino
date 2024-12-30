#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <MFRC522.h>
#include <vector>  
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#define SS_PIN 4
#define RST_PIN 15
#define RELAY_PIN 33
#define PIR_PIN 13
#define LED1_PIN 25
#define LED2_PIN 32
#define BUZZ_PIN 26
#define BUTTON_PIN 27

MFRC522 rfid(SS_PIN, RST_PIN);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define WIFI_SSID "UPT_TIK"
#define WIFI_PASS "gaspoll23#"

const char* botToken = "7730985699:AAHFgLJZi73bv4sk8Y9JSteURVxqXRQ6yuw";
const char* chat_id = "1159408196";
WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

std::vector<String> validCardUIDs = {"23 64 31 16", "05 84 64 37 50 A1 00", "02 A9 AF 71 F0 C7 B0", "02 A7 FF 31 71 16 50"};
String lastScannedID;

String logAccess = "";

unsigned long lastTimeBotRan = 0;
const unsigned long botInterval = 1000;

void setup() {
  Serial.begin(9600);
  if (!display.begin(SSD1306_PAGEADDR, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  SPI.begin();
  rfid.PCD_Init();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    display.clearDisplay();
    display.setCursor(0, 28);
    display.setTextSize(1);
    display.print("Connecting to WiFi...");
    display.display();
  }
  Serial.println("\nConnected To WiFi");
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZ_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT_PULLDOWN);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  display.clearDisplay();
  display.setCursor(39, 14);
  display.setTextSize(2);
  display.print("SCAN");
  display.setCursor(13, 32);
  display.setTextSize(2);
  display.print("YOUR CARD");
  display.display();

  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(BUZZ_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);
}

void loop() {
  if (millis() - lastTimeBotRan > botInterval) {
    handleNewMessages();
    lastTimeBotRan = millis();
  }
  
  handleRFID();
  handleButton();
  handlePIR();
}

void handleRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  String cardID = getCardID();

  if (digitalRead(PIR_PIN) == HIGH) {
    if (!isCardRegister(cardID)) {
      warningIntruder();
      String message = "WARNING!!! \nID Card: " + cardID + "\nPenyusup Terdeteksi Dengan ID Yang Tidak Terdaftar";
      bot.sendMessage(chat_id, message, "");
      Serial.println(message);
    }
  } else if (isCardRegister(cardID)) {
    unlockDoor();
    String message = "ID Card: " + cardID + " Berhasil Masuk.";
    bot.sendMessage(chat_id, message, "");
    logAccess += "ID Card: " + cardID + " | Status: Granted\n";
    Serial.println(message);
  } else {
    denyAccess();
    // updateFirebaseStatus("Akses Ditolak", cardID);
    String message = "ID Card: " + cardID + " Akses Ditolak.";
    bot.sendMessage(chat_id, message, "");
    Serial.println(message);
  }
}

String getCardID() {
  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  uid.trim();
  return uid;
}

bool isCardRegister(String cardUID) {
  return std::find(validCardUIDs.begin(), validCardUIDs.end(), cardUID) != validCardUIDs.end();
}

void handleButton() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    unlockDoor();
    String message = "Pintu telah dibuka menggunakan tombol manual.";
    bot.sendMessage(chat_id, message, "");
    Serial.println(message);
  }
}

void handlePIR() {
  if (digitalRead(PIR_PIN) == HIGH) {
    warningIntruder();
    String message = "WARNING!!!\nPenyusup Terdeteksi.";
    bot.sendMessage(chat_id, message, "");
    Serial.println(message);
  }
}

void unlockDoor() {
  digitalWrite(RELAY_PIN, HIGH);
  display.clearDisplay();
  display.setCursor(37, 14);
  display.setTextSize(2);
  display.print("PINTU");
  display.setCursor(24, 32);
  display.setTextSize(2);
  display.print("TERBUKA");
  display.display();

  for (int i = 0; i < 2; i++) {
    digitalWrite(LED1_PIN, HIGH);
    tone(BUZZ_PIN, 3000, 100);
    delay(100);
    digitalWrite(LED1_PIN, LOW);
    delay(100);
  }

  delay(5000);
  digitalWrite(RELAY_PIN, LOW);
  display.clearDisplay();
  display.setCursor(39, 14);
  display.setTextSize(2);
  display.print("SCAN");
  display.setCursor(13, 32);
  display.setTextSize(2);
  display.print("YOUR CARD");
  display.display();
}

void denyAccess() {
  display.clearDisplay();
  display.setCursor(37, 14);
  display.setTextSize(2);
  display.print("AKSES");
  display.setCursor(24, 32);
  display.setTextSize(2);
  display.print("DITOLAK");
  display.display();

  for (int i = 0; i < 3; i++) {
    digitalWrite(LED2_PIN, HIGH);
    tone(BUZZ_PIN, 3000, 200);
    delay(200);
    digitalWrite(LED2_PIN, LOW);
    delay(200);
  }
  display.clearDisplay();
  display.setCursor(39, 14);
  display.setTextSize(2);
  display.print("SCAN");
  display.setCursor(13, 32);
  display.setTextSize(2);
  display.print("YOUR CARD");
  display.display();
}

void warningIntruder() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED2_PIN, HIGH);
    tone(BUZZ_PIN, 3000, 700);
    display.clearDisplay();
    display.setCursor(23, 14);
    display.setTextSize(2);
    display.print("WARNING!");
    display.setCursor(21, 32);
    display.setTextSize(2);
    display.print("PENYUSUP");
    display.display();
    delay(700);
    digitalWrite(LED2_PIN, LOW);
    noTone(BUZZ_PIN);
    delay(500);
  }
  display.clearDisplay();
  display.setCursor(39, 14);
  display.setTextSize(2);
  display.print("SCAN");
  display.setCursor(13, 32);
  display.setTextSize(2);
  display.print("YOUR CARD");
  display.display();
}

void handleNewMessages() {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    if (text == "/start") {
      String welcome = "Welcome to The Smart Lock System\n";
      welcome += "Gunakan Perintah Berikut:\n";
      welcome += "/info - Untuk Melihat Log Akses\n";
      welcome += "/buka_pintu - Untuk Membuka Pintu\n";
      bot.sendMessage(chat_id, welcome, "");
    } else if (text == "/info") {
      if (logAccess.length() > 0) {
        bot.sendMessage(chat_id, "Log akses:\n" + logAccess, "");
      } else {
        bot.sendMessage(chat_id, "Belum ada log akses.", "");
      }
    } else if (text == "/buka_pintu") {
      unlockDoor();
      String message = "Pintu telah dibuka melalui Telegram.";
      bot.sendMessage(chat_id, message, "");
      Serial.println(message);
    } else {
      bot.sendMessage(chat_id, "Perintah tidak dikenali.", "");
    }     
  }
}

