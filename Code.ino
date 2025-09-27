#include <Wire.h>
#include <MPU6050_tockn.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP_Mail_Client.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// --------------------- Wi-Fi Credentials ---------------------
#define WIFI_SSID "DSR"
#define WIFI_PASSWORD "123456789"

// --------------------- Gmail SMTP Setup ----------------------
#define AUTHOR_EMAIL "qeupz079@gmail.com"
#define AUTHOR_PASSWORD "wimojgojnwoxrtwu"
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

// --------------------- Sensor & Audio Setup -------------------
MPU6050 mpu6050(Wire);
SoftwareSerial dfSerial(D6, D5); // DFPlayer TX → D6, RX → D5
DFRobotDFPlayerMini dfPlayer;

unsigned long lastUploadTime = 0;
String lastCommand = "";

// --------------------- SMTP Client ---------------------------
SMTPSession smtp;
SMTP_Message message;

void setup() {
  Serial.begin(115200);
  Wire.begin(D2, D1);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi!");

  // Initialize MPU6050
  mpu6050.begin();
  delay(1000);
  mpu6050.calcGyroOffsets(true);
  Serial.println("✅ MPU6050 Initialized");

  // Initialize DFPlayer Mini
  dfSerial.begin(9600);
  delay(1000);
  if (!dfPlayer.begin(dfSerial)) {
    Serial.println("❌ DFPlayer init failed!");
  } else {
    Serial.println("✅ DFPlayer Ready");
    dfPlayer.volume(20); // Volume 0-30
    dfPlayer.EQ(DFPLAYER_EQ_NORMAL); // Optional EQ setting
  }
}

void loop() {
  mpu6050.update();

  unsigned long currentMillis = millis();
  if (currentMillis - lastUploadTime >= 1000) {
    lastUploadTime = currentMillis;

    float accX = mpu6050.getAccX() * 9.81;
    float accY = mpu6050.getAccY() * 9.81;
    float accZ = mpu6050.getAccZ() * 9.81;

    String command = detectHandCommand(accX, accY, accZ);

    if (command != "Neutral" && command != lastCommand) {
      lastCommand = command;

      Serial.println("\n==========================");
      Serial.print("➡️ Gesture Detected: ");
      Serial.println(command);
      Serial.println("==========================\n");

      playAudio(command);

      if (command == "Please help me!") {
        sendEmergencyEmail();
        delay(2000); // Optional cooldown
      }
    } else {
      Serial.println("⚠️ No new gesture or neutral.");
    }
  }
}

// ------------------ Detect Hand Gesture ------------------
String detectHandCommand(float accX, float accY, float accZ) {
  if (abs(accX) < 3.0 && abs(accY) < 3.0 && abs(accZ - 9.81) < 2.0) {
    return "Neutral";
  }

  if (accZ > 11.0) {
    return "Thank you very much";
  } else if (accZ < -9.5) {
    return "Hello";
  } else if (accX > 6.0) {
    return "How are you?";
  } else if (accX < -6.0) {
    return "I am fine";
  } else if (accY > 6.0) {
    return "Come here";
  } else if (accY < -6.0) {
    return "Please help me!";
  }

  return "Neutral";
}

// ------------------ Play Audio Based on Gesture ------------------
void playAudio(String command) {
  Serial.print("🔊 Playing: ");
  Serial.println(command);

  if (command == "Thank you very much") {
    dfPlayer.play(1);
  } else if (command == "Hello") {
    dfPlayer.play(2);
  } else if (command == "I am fine") {
    dfPlayer.play(3);
  } else if (command == "How are you?") {
    dfPlayer.play(4);
  } else if (command == "Come here") {
    dfPlayer.play(5);
  } else if (command == "Please help me!") {
    dfPlayer.play(6);
  }
}

// ------------------ Send Email Function ------------------
void sendEmergencyEmail() {
  ESP_Mail_Session session;

  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";

  message.sender.name = "ESP8266 Gesture System";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "🚨 Emergency: Please Help Me!";
  message.addRecipient("You", AUTHOR_EMAIL);

  message.text.content = "The user triggered the 'Please help me!' gesture. Immediate help may be needed.";
  message.text.charSet = "utf-8";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  if (!smtp.connect(&session)) {
    Serial.println("❌ SMTP connection failed");
    return;
  }

  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.print("❌ Email Failed: ");
    Serial.println(smtp.errorReason());
  } else {
    Serial.println("✅ Emergency Email Sent!");
  }

  smtp.closeSession();
}
