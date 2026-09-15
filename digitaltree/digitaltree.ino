#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

const int waterButtonPin = 2;
const int hugButtonPin = 3;
const int breatheButtonPin = 4;
const int ldrPin = A0;

struct GameState {
  bool initialized;
  int growthStage;
  bool isWithered;
  bool hasFruit;
  unsigned long gameTimeHours;
  unsigned long lastWateredHour;
  unsigned long lastHuggedHour;
  unsigned long lastHarvestHour;
  int waterCount48h;
  unsigned long waterWindowStartHour;
};

GameState state;

bool isShowingSankalpa = false;
unsigned long sankalpaStartTime = 0;
int currentSankalpaIndex = 0;

unsigned long lastWaterDebounce = 0;
unsigned long lastHugDebounce = 0;
unsigned long debounceDelay = 50;

int lastWaterState = HIGH;
int lastHugState = HIGH;

unsigned long lastTickMillis = 0;
unsigned long breathingStartTime = 0;
bool isBreathing = false;
int breathRadius = 0;
bool breathExpanding = true;

const char* sankalpak[] = {
  "Beke van benned.",
  "Csodalatos vagy!",
  "Eros vagy.",
  "Minden rendben."
};

void saveState() {
  EEPROM.put(0, state);
}

void loadState() {
  EEPROM.get(0, state);
  if (!state.initialized) {
    state.initialized = true;
    state.growthStage = 0;
    state.isWithered = false;
    state.hasFruit = false;
    state.gameTimeHours = 0;
    state.lastWateredHour = 0;
    state.lastHuggedHour = 0;
    state.lastHarvestHour = 0;
    state.waterCount48h = 0;
    state.waterWindowStartHour = 0;
    saveState();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(waterButtonPin, INPUT_PULLUP);
  pinMode(hugButtonPin, INPUT_PULLUP);
  pinMode(breatheButtonPin, INPUT_PULLUP);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while(1);
  }
  
  loadState();
  drawScene();
}

void loop() {
  if (millis() - lastTickMillis > 1000) {
    state.gameTimeHours++;
    lastTickMillis = millis();

    if (state.gameTimeHours - state.waterWindowStartHour >= 48) {
      state.waterCount48h = 0;
      state.waterWindowStartHour = state.gameTimeHours;
    }

    if (!state.isWithered) {
      unsigned long lastInteraction = state.lastWateredHour > state.lastHuggedHour ? state.lastWateredHour : state.lastHuggedHour;
      if (state.gameTimeHours > 0 && (state.gameTimeHours - lastInteraction >= 48)) {
        state.isWithered = true;
      }

      if (state.gameTimeHours % 24 == 0) {
        if ((state.gameTimeHours - state.lastHuggedHour <= 24) && (state.gameTimeHours - state.lastWateredHour <= 12)) {
          if (state.growthStage < 12) state.growthStage++;
        }
      }

      if (state.growthStage >= 12 && !state.hasFruit && (state.gameTimeHours - state.lastHarvestHour >= 18)) {
        state.hasFruit = true;
      }
    }
    
    saveState();
    
    if (isShowingSankalpa && millis() - sankalpaStartTime > 5000) {
      isShowingSankalpa = false;
    }

    if (!isBreathing) {
      drawScene();
    }
  }

  int wRead = digitalRead(waterButtonPin);
  int hRead = digitalRead(hugButtonPin);
  int bRead = digitalRead(breatheButtonPin);
  int ldrValue = analogRead(ldrPin);

  if (state.hasFruit && ldrValue < 200 && !isShowingSankalpa && !state.isWithered) {
    state.hasFruit = false;
    state.lastHarvestHour = state.gameTimeHours;
    isShowingSankalpa = true;
    sankalpaStartTime = millis();
    currentSankalpaIndex = state.gameTimeHours % 4;
    saveState();
    drawScene();
  }

  if (wRead != lastWaterState) lastWaterDebounce = millis();
  if (hRead != lastHugState) lastHugDebounce = millis();

  if ((millis() - lastWaterDebounce) > debounceDelay) {
    if (wRead == LOW && lastWaterState == HIGH) {
      if (!state.isWithered) {
        state.lastWateredHour = state.gameTimeHours;
        state.waterCount48h++;
        if (state.waterCount48h >= 10) {
          state.isWithered = true;
        } else {
          if (state.growthStage == 0) state.growthStage = 1;
        }
        saveState();
        drawScene();
      }
    }
  }

  if ((millis() - lastHugDebounce) > debounceDelay) {
    if (hRead == LOW && lastHugState == HIGH) {
      if (!state.isWithered) {
        state.lastHuggedHour = state.gameTimeHours;
        if (state.growthStage == 1) state.growthStage = 2;
        saveState();
        drawScene();
      }
    }
  }

  if (bRead == LOW) {
    if (state.isWithered) {
      if (!isBreathing) {
        isBreathing = true;
        breathingStartTime = millis();
        breathRadius = 5;
        breathExpanding = true;
      }
      
      if (millis() - breathingStartTime > 100) {
        breathingStartTime = millis();
        if (breathExpanding) {
          breathRadius += 2;
          if (breathRadius >= 20) breathExpanding = false;
        } else {
          breathRadius -= 2;
          if (breathRadius <= 5) breathExpanding = true;
        }
        
        display.clearDisplay();
        display.drawCircle(64, 32, breathRadius, SSD1306_WHITE);
        display.setCursor(20, 55);
        display.setTextColor(SSD1306_WHITE);
        display.print("Lelegezz...");
        display.display();
      }

      if (breathRadius >= 20 && !breathExpanding) {
        state.isWithered = false;
        state.waterCount48h = 0;
        state.waterWindowStartHour = state.gameTimeHours;
        state.lastWateredHour = state.gameTimeHours;
        state.lastHuggedHour = state.gameTimeHours;
        saveState();
        isBreathing = false;
        drawScene();
      }
    }
  } else {
    if (isBreathing) {
      isBreathing = false;
      drawScene();
    }
  }

  lastWaterState = wRead;
  lastHugState = hRead;
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

  if (state.isWithered) {
    display.drawLine(64, 62, 64, 50, SSD1306_WHITE);
    display.drawLine(64, 55, 66, 59, SSD1306_WHITE);
    display.drawLine(64, 58, 62, 60, SSD1306_WHITE);
  } else {
    if (state.growthStage == 0) {
      display.drawPixel(64, 62, SSD1306_WHITE);
      display.drawPixel(63, 62, SSD1306_WHITE);
      display.drawPixel(65, 62, SSD1306_WHITE);
      display.drawPixel(64, 61, SSD1306_WHITE);
    } else if (state.growthStage == 1) {
      display.drawLine(64, 62, 64, 58, SSD1306_WHITE);
      display.drawPixel(65, 59, SSD1306_WHITE);
      display.drawPixel(63, 60, SSD1306_WHITE);
    } else if (state.growthStage >= 2) {
      int h = 12 + (state.growthStage * 2);
      display.drawLine(64, 62, 64, 62 - h, SSD1306_WHITE);
      display.drawLine(64, 62 - (h/2), 68, 62 - (h/2) - 4, SSD1306_WHITE);
      display.drawLine(64, 62 - (h/4), 60, 62 - (h/4) - 4, SSD1306_WHITE);
      display.drawCircle(64, 62 - h - 4, 4 + (state.growthStage/2), SSD1306_WHITE);

      if (state.hasFruit) {
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
  display.print(state.gameTimeHours);
  display.print(" S:");
  display.print(state.growthStage);
  
  display.display();
}