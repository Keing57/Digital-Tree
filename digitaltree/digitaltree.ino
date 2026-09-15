#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

const int waterButtonPin = 2;
const int hugButtonPin = 3;
const int breatheButtonPin = 4;

int growthStage = 0;
bool isWithered = false;

unsigned long lastWaterDebounce = 0;
unsigned long lastHugDebounce = 0;
unsigned long lastBreatheDebounce = 0;
unsigned long debounceDelay = 50;

int lastWaterState = HIGH;
int lastHugState = HIGH;
int lastBreatheState = HIGH;

unsigned long gameTimeHours = 0;
unsigned long lastTickMillis = 0;

unsigned long lastWateredHour = 0;
unsigned long lastHuggedHour = 0;
int waterCount48h = 0;
unsigned long waterWindowStartHour = 0;

void setup() {
  Serial.begin(115200);
  pinMode(waterButtonPin, INPUT_PULLUP);
  pinMode(hugButtonPin, INPUT_PULLUP);
  pinMode(breatheButtonPin, INPUT_PULLUP);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while(1);
  }
  drawScene();
}

void loop() {
  if (millis() - lastTickMillis > 1000) {
    gameTimeHours++;
    lastTickMillis = millis();

    if (gameTimeHours - waterWindowStartHour >= 48) {
      waterCount48h = 0;
      waterWindowStartHour = gameTimeHours;
    }
    
    if (!isWithered) {
        if (gameTimeHours > 0 && gameTimeHours % 24 == 0) {
            if ((gameTimeHours - lastHuggedHour <= 24) && (gameTimeHours - lastWateredHour <= 12)) {
                if (growthStage < 12) growthStage++;
            }
        }
    }
    drawScene();
  }

  int wRead = digitalRead(waterButtonPin);
  int hRead = digitalRead(hugButtonPin);
  int bRead = digitalRead(breatheButtonPin);

  if (wRead != lastWaterState) lastWaterDebounce = millis();
  if (hRead != lastHugState) lastHugDebounce = millis();
  if (bRead != lastBreatheState) lastBreatheDebounce = millis();

  if ((millis() - lastWaterDebounce) > debounceDelay) {
    if (wRead == LOW && lastWaterState == HIGH) {
      if (!isWithered) {
          lastWateredHour = gameTimeHours;
          waterCount48h++;
          if (waterCount48h >= 10) {
            isWithered = true;
          } else {
            if (growthStage == 0) growthStage = 1;
          }
          drawScene();
      }
    }
  }

  if ((millis() - lastHugDebounce) > debounceDelay) {
    if (hRead == LOW && lastHugState == HIGH) {
      if (!isWithered) {
          lastHuggedHour = gameTimeHours;
          if (growthStage == 1) growthStage = 2;
          drawScene();
      }
    }
  }

  if ((millis() - lastBreatheDebounce) > debounceDelay) {
    if (bRead == LOW && lastBreatheState == HIGH) {
      if (isWithered) {
        isWithered = false;
        waterCount48h = 0;
        waterWindowStartHour = gameTimeHours;
        drawScene();
      }
    }
  }

  lastWaterState = wRead;
  lastHugState = hRead;
  lastBreatheState = bRead;
}

void drawScene() {
  display.clearDisplay();
  display.drawLine(0, 63, 128, 63, SSD1306_WHITE);

  if (isWithered) {
    display.drawLine(64, 62, 64, 50, SSD1306_WHITE);
    display.drawLine(64, 55, 66, 59, SSD1306_WHITE);
    display.drawLine(64, 58, 62, 60, SSD1306_WHITE);
  } else {
    if (growthStage == 0) {
      display.drawPixel(64, 62, SSD1306_WHITE);
      display.drawPixel(63, 62, SSD1306_WHITE);
      display.drawPixel(65, 62, SSD1306_WHITE);
      display.drawPixel(64, 61, SSD1306_WHITE);
    } else if (growthStage == 1) {
      display.drawLine(64, 62, 64, 58, SSD1306_WHITE);
      display.drawPixel(65, 59, SSD1306_WHITE);
      display.drawPixel(63, 60, SSD1306_WHITE);
    } else if (growthStage >= 2) {
      int h = 12 + (growthStage * 2);
      display.drawLine(64, 62, 64, 62 - h, SSD1306_WHITE);
      display.drawLine(64, 62 - (h/2), 68, 62 - (h/2) - 4, SSD1306_WHITE);
      display.drawLine(64, 62 - (h/4), 60, 62 - (h/4) - 4, SSD1306_WHITE);
      display.drawCircle(64, 62 - h - 4, 4 + (growthStage/2), SSD1306_WHITE);
    }
  }
  
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.print("H:");
  display.print(gameTimeHours);
  display.print(" S:");
  display.print(growthStage);
  
  display.display();
}