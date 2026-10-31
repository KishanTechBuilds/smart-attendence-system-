#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>

// -------- RFID Reader Pins --------
#define ENTRY_SS   5
#define ENTRY_RST  27
#define EXIT_SS    4
#define EXIT_RST   26
#define RFID_SCK   18
#define RFID_MOSI  23
#define RFID_MISO  19

// -------- OLED Pins --------
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// -------- LED --------
#define LED_PIN 2

MFRC522 entryRFID(ENTRY_SS, ENTRY_RST);
MFRC522 exitRFID(EXIT_SS, EXIT_RST);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Change these UID/name/roll entries to match your cards.
// UID must match the uppercase UID shown in Serial Monitor.
struct Student {
  const char* uid;
  const char* name;
  int roll;
};

Student students[] = {
  {"A191B30A", "Krushna", 43},
  {"C189CA0A", "Rushiraj", 38},
  {"06AC1307", "Sarvesh", 56}
};
const int studentCount = sizeof(students) / sizeof(students[0]);

unsigned long lastScanTime = 0;
String lastUID = "";
const unsigned long scanCooldown = 1800;

void showMessage(const String &l1, const String &l2 = "",
                 const String &l3 = "", const String &l4 = "") {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);  display.println(l1);
  display.setCursor(0, 16); display.println(l2);
  display.setCursor(0, 30); display.println(l3);
  display.setCursor(0, 44); display.println(l4);
  display.display();
}

int findStudent(const String &uid) {
  for (int i = 0; i < studentCount; i++) {
    if (uid == students[i].uid) return i;
  }
  return -1;
}

String getClockText() {
  struct tm timeInfo;
  if (getLocalTime(&timeInfo, 20)) {
    char buf[20];
    strftime(buf, sizeof(buf), "%H:%M:%S", &timeInfo);
    return String(buf);
  }
  return "Time unavailable";
}

String getDateText() {
  struct tm timeInfo;
  if (getLocalTime(&timeInfo, 20)) {
    char buf[16];
    strftime(buf, sizeof(buf), "%d-%m-%Y", &timeInfo);
    return String(buf);
  }
  return "Date unavailable";
}

String readUID(MFRC522 &reader) {
  String uid = "";
  for (byte i = 0; i < reader.uid.size; i++) {
    if (reader.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(reader.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  return uid;
}

void displayAttendance(const String &name, int roll, const String &uid,
                       const String &status) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("RFID ATTENDANCE");

  display.setCursor(0, 10);
  display.print("Name: "); display.println(name);

  display.setCursor(0, 20);
  display.print("Roll: "); display.println(roll);

  display.setCursor(0, 30);
  display.print("UID: "); display.println(uid);

  display.setCursor(0, 40);
  display.print("Status: "); display.println(status);

  display.setCursor(0, 50);
  display.print("Date: "); display.println(getDateText());

  // OLED is 64 px high; show time in place of date after a short pause.
  display.display();
  delay(1300);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);  display.println("RFID ATTENDANCE");
  display.setCursor(0, 16); display.print("Name: "); display.println(name);
  display.setCursor(0, 30); display.print("Status: "); display.println(status);
  display.setCursor(0, 44); display.print("Time: "); display.println(getClockText());
  display.display();
}

void processReader(MFRC522 &reader, const String &direction) {
  if (!reader.PICC_IsNewCardPresent()) return;
  if (!reader.PICC_ReadCardSerial()) return;

  String uid = readUID(reader);
  Serial.print(direction);
  Serial.print(" reader UID: ");
  Serial.println(uid);

  // Basic duplicate-scan cooldown to avoid repeated reads of the same card.
  if (!(uid == lastUID && millis() - lastScanTime < scanCooldown)) {
    lastUID = uid;
    lastScanTime = millis();

    int index = findStudent(uid);
    if (index >= 0) {
      displayAttendance(students[index].name, students[index].roll,
                        uid, direction);

      digitalWrite(LED_PIN, HIGH);
      delay(180);
      digitalWrite(LED_PIN, LOW);
    } else {
      showMessage("UNKNOWN CARD", "UID:", uid, "Not registered");
      Serial.println("Unknown card: add its UID to students[]");
    }
  }

  reader.PICC_HaltA();
  reader.PCD_StopCrypto1();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED not detected. Check wiring/address.");
    while (true) delay(100);
  }

  showMessage("RFID ATTENDANCE", "Starting...");
  SPI.begin(RFID_SCK, RFID_MISO, RFID_MOSI);

  // Both readers share SPI pins but have separate SS and RST pins.
  entryRFID.PCD_Init();
  delay(50);
  exitRFID.PCD_Init();
  delay(50);

  Serial.println("Dual RFID attendance ready.");
  Serial.println("Scan at entry or exit reader; check UIDs here.");
  showMessage("RFID ATTENDANCE", "System Ready", "Scan your card", "ENTRY / EXIT");
}

void loop() {
  processReader(entryRFID, "ENTRY");
  processReader(exitRFID, "EXIT");
}
