#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

const int waterButtonPin = 2;
const int hugButtonPin = 3;
const int breatheButtonPin = 4;
const int ldrPin = A0;

int growthStage = 0;
bool isWithered = false;
bool hasFruit = false;
bool isShowingSankalpa = false;
unsigned long sankalpaStartTime = 0;
int currentSankalpaIndex = 0;

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
unsigned long lastHarvestHour = 0;
int waterCount48h = 0;
unsigned long waterWindowStartHour = 0;

const char* sankalpak[] = {
  "Beke van benned.",
  "Csodalatos vagy!",
  "Eros vagy.",
  "Minden rendben."
};

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
      unsigned long lastInteraction = lastWateredHour > lastHuggedHour ? lastWateredHour : lastHuggedHour;
      if (gameTimeHours > 0 && (gameTimeHours - lastInteraction >= 48)) {
        isWithered = true;
      }

      if (gameTimeHours % 24 == 0) {
        if ((gameTimeHours - lastHuggedHour <= 24) && (gameTimeHours - lastWateredHour <= 12)) {
          if (growthStage < 12) growthStage++;
        }
      }

      if (growthStage >= 12 && !hasFruit && (gameTimeHours - lastHarvestHour >= 18)) {
        hasFruit = true;
      }
    }
    
    if (isShowingSankalpa && millis() - sankalpaStartTime > 5000) {
      isShowingSankalpa = false;
    }

    drawScene();
  }

  int wRead = digitalRead(waterButtonPin);
  int hRead = digitalRead(hugButtonPin);
  int bRead = digitalRead(breatheButtonPin);
  int ldrValue = analogRead(ldrPin);

  if (hasFruit && ldrValue < 200 && !isShowingSankalpa && !isWithered) {
    hasFruit = false;
    lastHarvestHour = gameTimeHours;
    isShowingSankalpa = true;
    sankalpaStartTime = millis();
    currentSankalpaIndex = gameTimeHours % 4;
    drawScene();
  }

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
        lastWateredHour = gameTimeHours;
        lastHuggedHour = gameTimeHours;
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
  
  if (isShowingSankalpa) {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 30);
    display.print(sankalpak[currentSankalpaIndex]);
    display.display();
    return;
  }

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

      if (hasFruit) {
        int fx = 68;
        int fy = 62 - (h/2) - 3;
        display.drawPixel(fx-1, fy-1, SSD1306_WHITE);
        display.drawPixel(fx+1, fy-1, SSD1306_WHITE);
        display.drawLine(fx-2, fy, fx+2, fy, SSD1306_WHITE);
        display.drawLine(fx-1, fy+1, fx+1, fy+1, SSD1306_WHITE);
        display.drawPixel(fx, fy+2, SSD1306_WHITE);
      }
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