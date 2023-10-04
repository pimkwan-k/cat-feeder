#include "time.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <Servo.h>

#define SERVO_SIGNAL_PIN 13
#define SERVO_CUTOFF_PIN 14
#define MANUAL_ON_SWITCH_PIN 27

#define BLYNK_TEMPLATE_ID "TMPL6dB1DrQBg"
#define BLYNK_TEMPLATE_NAME "Cat Feeder v2"
#define BLYNK_AUTH_TOKEN "fTYcShLUDrDjyt0XZffhzefAFXUMgsb5"
#include <BlynkSimpleEsp32.h>

#define TIME_CHECK_INTERVAL 30000 // (30 secs * 1000 ms)
uint32_t latestProgramTimeCheck;
uint32_t latestPinCheckTime;
uint32_t latestTimerActiveTime;
uint32_t latestServoActive;

// WiFi credentials.
char ssid[] = "PimKwan_2.4G";
char pass[] = "Blink@2019";

// NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 25200;
const int   daylightOffset_sec = 0;

// Memory variables
uint32_t currentDayTimeSec;
uint32_t timerTriggerSet;
time_t latestDateTimeSec;
struct tm currentDateTime;
time_t currentDateTimeSec;
uint8_t pos1 = 90;
float ye1;

// Control Flags
bool isTimerActive = false;
bool isSpin = false;
bool isCutoffActive = false;
bool isServoOn = false;
bool isManualOnSwitchActive = false;
bool prevTimerActive = true;

// Instances
BlynkTimer blynkTimer;
Servo myservo = Servo();

void setup() {
  // Debug console
  Serial.begin(115200);

  // Pin Setup
  pinMode(SERVO_CUTOFF_PIN, INPUT_PULLUP);
  pinMode(MANUAL_ON_SWITCH_PIN, INPUT_PULLUP);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Setup a function to be called every second
  blynkTimer.setInterval(1000L, blynkTimer1Event);

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  if(!getLocalTime(&currentDateTime)){
    Serial.println("Failed to obtain time");
  } else {
    currentDateTimeSec = mktime(&currentDateTime);

    Serial.println();
    Serial.print("Start time: ");
    Serial.println(&currentDateTime, "%A, %B %d %Y %H:%M:%S");
  }
}

void loop() {
  uint32_t currentProgramTime = millis();
  double timeDiff;

  Blynk.run();
  blynkTimer.run();
  
  // Work when millis() is overflow.
  if (currentProgramTime <= 2000) {
    latestProgramTimeCheck = currentProgramTime;
    latestPinCheckTime = currentProgramTime;
  }

  if ((currentProgramTime >= latestServoActive + 10)) {
    latestServoActive = currentProgramTime;

    // Servo Control
    if (isSpin) pos1 = 82;
    else if (isCutoffActive) pos1 = 90;
    else pos1 = 90;

    if (isServoOn) {
      ye1 = myservo.write(SERVO_SIGNAL_PIN, 85);
      isSpin = false;

    } else {
      ye1 = myservo.write(SERVO_SIGNAL_PIN, 90);

    }

  }

  if (currentProgramTime >= latestTimerActiveTime + 1200) {
    latestTimerActiveTime = currentProgramTime;
    isTimerActive = false;
  }

  if (currentProgramTime >= latestPinCheckTime + 10) {
    latestPinCheckTime = currentProgramTime;
    isCutoffActive = digitalRead(SERVO_CUTOFF_PIN) == LOW;
    isManualOnSwitchActive = digitalRead(MANUAL_ON_SWITCH_PIN) == LOW;
    isServoOn = isManualOnSwitchActive || !isCutoffActive || (isCutoffActive && isTimerActive);
  }

  // Debouncing, should work once per defined interval
  if (currentProgramTime >= latestProgramTimeCheck + TIME_CHECK_INTERVAL) {

    // Polling current date time from NTP server.
    getLocalTime(&currentDateTime);
    currentDateTimeSec = mktime(&currentDateTime);

    timeDiff = difftime(currentDateTimeSec, latestDateTimeSec);
  
    latestProgramTimeCheck = currentProgramTime;
    latestDateTimeSec = currentDateTimeSec;
    currentDayTimeSec = (currentDateTime.tm_hour * 60 * 60) + (currentDateTime.tm_min * 60) + (currentDateTime.tm_sec);

    // Reset Timer flags on the start of the day, everyday.
    if (currentDayTimeSec <= 100) {
      isTimerActive = false;
      prevTimerActive = false;

      Serial.println("Timer flags resetted!");
    }

    // Timer Trigger Action!, then flag it.
    if (currentDayTimeSec >= timerTriggerSet && !prevTimerActive) {
      prevTimerActive = true;
      isTimerActive = true;

      Serial.println("Action trigged!");
    }
  }
}

// This function is called every time the device is connected to the Blynk.Cloud
BLYNK_CONNECTED() { 
  Blynk.syncVirtual(V0);
}

BLYNK_WRITE(V0) {
// Called when the datastream V1 value changes

  // param[0] is the user time value selected in seconds.
  timerTriggerSet = param[0].asInt();

  if (timerTriggerSet >= currentDayTimeSec && currentDayTimeSec > 0) {
    Serial.println("Timer trigger flags resetted!");
    isTimerActive = false;
    prevTimerActive = false;
  }

  Serial.print("Timer trigger start is set to: "); 
  Serial.print(timerTriggerSet);
  Serial.println(" (secs of a day)");
}

// This function sends Arduino's uptime every second to Virtual Pin 2.
void blynkTimer1Event() {
  // Blynk.virtualWrite(V2, millis() / 1000);
}
