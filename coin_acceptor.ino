#include <Arduino.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>

const int coinPin = 2;
volatile int impulsCount = 0;
int coinType = 0;
float totalAmount = 0.0;
float coinValue;

int BUTTON_PIN = 3;
int BUZZER_PIN = 8;

unsigned long lastButtonPressTime = 0;
const unsigned long debounceDelay = 1000;

const int coinPulses[] = {1};
const float coinValues[] = {100};


int lookup(int pulses) {
  for (int i = 0; i < 2; i++) {
    if (pulses == coinPulses[i]) {
      return coinValues[i];
    }
  }
  return -1;
}

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(9600);
  pinMode(coinPin, INPUT_PULLUP); // Initialize coin pin
  pinMode(BUZZER_PIN, OUTPUT); // Initialize buzzer pin

  attachInterrupt(digitalPinToInterrupt(coinPin), coinInterrupt, FALLING); // Set up interrupt for coin pulses
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  lcd.init(); // Initialize the LCD
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Insert coins");
}

void coinInterrupt() {
  impulsCount++;
}


void loop() {
  int reading = digitalRead(BUTTON_PIN);
  if (reading == LOW && (millis() - lastButtonPressTime > debounceDelay)) {
    lastButtonPressTime = millis();
    resetCoinData();
  }
  handleCoins();
}

void handleCoins() {
  if (impulsCount > 0) {
    float coinValue = lookup(impulsCount);
    if (coinValue > 0) {
      totalAmount += coinValue;
      impulsCount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Coins: ");
      lcd.print(totalAmount);
      Serial.print("Total Amount: ");
      Serial.println(totalAmount, 2);
      tone(BUZZER_PIN, 1000, 100); // Optional: beep for each coin inserted
    }
  }
}

void resetCoinData() {
  if (totalAmount > 0) {
    Serial.print("Amount=");
    Serial.println(totalAmount);
  }

  totalAmount = 0.0;
  impulsCount = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Insert coins");

}
