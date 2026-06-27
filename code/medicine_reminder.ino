
// PIN CONNECTIONS:
// A4 (SDA) -> RTC SDA & LCD SDA
// A5 (SCL) -> RTC SCL & LCD SCL
// D2       -> Hour Button (set alarm hour)
// D3       -> Minute Button (set alarm minute)
// D4       -> Stop Button (dismiss alarm)
// D5       -> LED (visual alert)
// D6       -> Buzzer (audio alert)

// REQUIRED LIBRARIES (Install via Library Manager):
// 1. RTClib by Adafruit
// 2. LiquidCrystal_I2C by Frank de Brabander

#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

#define LCD_ADDRESS 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

#define HOUR_BUTTON 2
#define MINUTE_BUTTON 3
#define STOP_BUTTON 4
#define LED_PIN 5
#define BUZZER_PIN 6

#define DEFAULT_HOUR 8
#define DEFAULT_MINUTE 0
#define DEBOUNCE_DELAY 50

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);
RTC_DS3231 rtc;

int alarmHour = DEFAULT_HOUR;
int alarmMinute = DEFAULT_MINUTE;
bool alarmTriggered = false;

bool lastHourState = HIGH;
bool lastMinuteState = HIGH;
bool lastStopState = HIGH;

void setup() {
  Serial.begin(9600);

  pinMode(HOUR_BUTTON, INPUT_PULLUP);
  pinMode(MINUTE_BUTTON, INPUT_PULLUP);
  pinMode(STOP_BUTTON, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  if (!rtc.begin()) {
    lcd.print("RTC ERROR!");
    lcd.setCursor(0, 1);
    lcd.print("Check Wiring");
    while (1);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  lcd.setCursor(0, 0);
  lcd.print("Smart Medicine");
  lcd.setCursor(0, 1);
  lcd.print("Reminder System");
  delay(2000);
  lcd.clear();

  Serial.println("Smart Medicine Reminder System Ready!");
  Serial.print("Alarm set to: ");
  Serial.print(alarmHour);
  Serial.print(":");
  Serial.println(alarmMinute);
}

void loop() {
  DateTime now = rtc.now();
  int currentHour = now.hour();
  int currentMinute = now.minute();
  int currentSecond = now.second();

  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  printTwoDigits(currentHour);
  lcd.print(":");
  printTwoDigits(currentMinute);
  lcd.print(":");
  printTwoDigits(currentSecond);

  lcd.setCursor(0, 1);
  lcd.print("Alarm: ");
  printTwoDigits(alarmHour);
  lcd.print(":");
  printTwoDigits(alarmMinute);

  if (!alarmTriggered && currentHour == alarmHour && currentMinute == alarmMinute) {
    Serial.println("ALARM TRIGGERED! Medicine time!");
    triggerAlarm();
  }

  readButtons();
  delay(500);
}

void printTwoDigits(int number) {
  if (number < 10) {
    lcd.print("0");
  }
  lcd.print(number);
}

void triggerAlarm() {
  alarmTriggered = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  MEDICINE");
  lcd.setCursor(0, 1);
  lcd.print("  TIME!  STOP");

  unsigned long lastToggleTime = 0;
  bool state = false;

  while (alarmTriggered) {
    unsigned long currentMillis = millis();

    if (currentMillis - lastToggleTime >= 300) {
      lastToggleTime = currentMillis;
      state = !state;
      digitalWrite(LED_PIN, state ? HIGH : LOW);
      digitalWrite(BUZZER_PIN, state ? HIGH : LOW);
    }

    if (digitalRead(STOP_BUTTON) == LOW) {
      delay(DEBOUNCE_DELAY);
      if (digitalRead(STOP_BUTTON) == LOW) {
        stopAlarm();
        break;
      }
    }
  }
}

void stopAlarm() {
  alarmTriggered = false;
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Medicine");
  lcd.setCursor(0, 1);
  lcd.print("  Taken!  ");

  Serial.println("Alarm stopped - Medicine taken");
  delay(2000);
  lcd.clear();
}

void readButtons() {
  bool hourState = digitalRead(HOUR_BUTTON);
  bool minuteState = digitalRead(MINUTE_BUTTON);
  bool stopState = digitalRead(STOP_BUTTON);

  if (hourState == LOW && lastHourState == HIGH && !alarmTriggered) {
    delay(DEBOUNCE_DELAY);
    if (digitalRead(HOUR_BUTTON) == LOW) {
      alarmHour = (alarmHour + 1) % 24;
      showAlarmSetMessage("Hour", alarmHour);
      Serial.print("Alarm Hour set to: ");
      Serial.println(alarmHour);
    }
  }
  lastHourState = hourState;

  if (minuteState == LOW && lastMinuteState == HIGH && !alarmTriggered) {
    delay(DEBOUNCE_DELAY);
    if (digitalRead(MINUTE_BUTTON) == LOW) {
      alarmMinute = (alarmMinute + 1) % 60;
      showAlarmSetMessage("Minute", alarmMinute);
      Serial.print("Alarm Minute set to: ");
      Serial.println(alarmMinute);
    }
  }
  lastMinuteState = minuteState;

  if (stopState == LOW && lastStopState == HIGH && alarmTriggered) {
    delay(DEBOUNCE_DELAY);
    if (digitalRead(STOP_BUTTON) == LOW) {
      stopAlarm();
    }
  }
  lastStopState = stopState;
}

void showAlarmSetMessage(String type, int value) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Alarm ");
  lcd.print(type);
  lcd.print(" Set:");
  lcd.setCursor(0, 1);
  if (value < 10) lcd.print("0");
  lcd.print(value);
  delay(500);
  lcd.clear();
}
