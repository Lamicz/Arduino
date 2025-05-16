// https://github.com/adafruit/Adafruit_NeoPixel
// https://github.com/adafruit/Adafruit_NeoPixel_ZeroDMA
#include <Adafruit_NeoPixel_ZeroDMA.h>
#include <APA102.h> // https://github.com/pololu/apa102-arduino
#include <PinButton.h> // https://github.com/poelstra/arduino-multi-button
#include <Adafruit_FreeTouch.h> // https://github.com/adafruit/Adafruit_FreeTouch 

const byte NEOPIXEL_DATA = 4;
const word STRIP_LENGTH = 288;
const byte BUTTON_SWITCH = 0;
const byte BUTTON_TOUCH = A0;
const byte DOTSTAR_DATA = 7;
const byte DOTSTAR_CLK = 8;
const byte PIXELS_CNT = 44; // pixelsMinMax[1] - 1;

// casovace [ms]
const unsigned long timerPixelCnt = 120000;
const unsigned long timerPixelFlash = 60000;

byte pixelCurrentMode = 1; // [0 - no flash, 1 - flash, 2 - flash reduced pixels (power saving), 3 - RGB]

Adafruit_FreeTouch buttonTouch = Adafruit_FreeTouch(BUTTON_TOUCH, OVERSAMPLE_4, RESISTOR_50K, FREQ_MODE_NONE); 

PinButton buttonSwitch(BUTTON_SWITCH);

APA102<DOTSTAR_DATA, DOTSTAR_CLK> onboardDotStarLed;
rgb_color onboardDotstarLedColors[1];

Adafruit_NeoPixel_ZeroDMA strip(STRIP_LENGTH, NEOPIXEL_DATA, NEO_GRB);

const byte stripTopColumnStart = 216;
const byte pixelRGBColor[3] = {174, 109, 30}; // pro SK6812, {189, 120, 34} pro WS2812
const word pixelSpeedHigh[2] = {750, 670}; // standard rychlost
const word pixelSpeedMedium[2] = {630, 450}; // rychlejsi
const word pixelSpeedLow[2] = {450, 396}; // nejrychlejsi
const byte pixelSpeedDelayInit = 4; // timeout po touch eventu [s]
const byte pixelSpeedDelayStep = 10; // timeout mezi kroky [s]
const byte pixelsMinMax[2] = {30, 45}; // pocet pixelu najednou (min a max interval)
const byte pixelsMinMaxReduced[2] = {5, 25}; // pocet pixelu najednou pro mode 2 (min a max interval)
const byte stripPixelsFlash[15] = {0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; // pocet pixelu na zablesk
const byte stripPixelsFlashLength = 15; // pocet prvku v stripPixelsFlash[]
const byte stripPixelsFlashStartIndex = 5; // start index alespon 1x zablesk z pole stripPixelsFlash[]
const byte flashColors[3][3] = {{255, 162, 46}, {128, 81, 23}, {85, 54, 15}}; // zableskove barvy
const byte flashTimesMinMax[2] = {1, 7}; // kolikrat se ma bliknout
const byte pixelWaitCycles = 30; // prodleva pixelu na mbr pro pixel mode 1 a 2 [pocet cyklu]

typedef struct Timer
{
  unsigned long tick = 0;
  unsigned long pixelSubmode = 0;
  unsigned long pixelCnt = 0;
  unsigned long pixelFlash = 0;
  bool pixelFlashEnabled = true;
  unsigned long touchEventPixelSpeed = 0;
} Timer;

Timer timer;

typedef struct EggPixel
{
  byte timer = 0;
  byte status = 0;
  word position = 0;
  float currentBrightness = 0.0;
  float stepChangeBrightness = 0.0;
  bool maxBrightnessHigh = false;
  word maxBrightness = 0;
  byte flashTimes = 0;
  bool flash = true;
  byte rgb[3];
} EggPixel;

EggPixel pixels[PIXELS_CNT];
word buttonTouchCoef = 0;
bool pixelSpeedTrigger = false;
bool timer1Trigger = true;
byte pixelsCntActive = 0;
byte stripFlash = 0;
byte stripFlashTotal = 0;
bool stripFreePositions[STRIP_LENGTH];
byte pixelSpeedIndex = 0;
byte pixelSpeedDelay = 0;
byte pixelDifferentBrightness = 0;
byte pixelsCntCurrent = 0;
byte pixelDifferentBrightnesses = 0;
word pixelCurrentSpeed[2];
byte flashColorsCoef = 0;
float speedCoef;

void adjustPixelsCntByStep(byte *varToAdjust)
{
  if (varToAdjust == 0) {
    
    *varToAdjust = (pixelCurrentMode == 2)
      ? random(pixelsMinMaxReduced[0], pixelsMinMaxReduced[1])
      : random(pixelsMinMax[0], pixelsMinMax[1]);
    return;
  }

  byte currentStep = random(4);
  if (random(11) > 4) {
    *varToAdjust = *varToAdjust + currentStep;
  } else {
    *varToAdjust = *varToAdjust - currentStep;
  }
  if (*varToAdjust > pixelsMinMax[1] - 1) {
    *varToAdjust = pixelsMinMax[1] - 1;
    return;
  }
  if (*varToAdjust < pixelsMinMax[0]) {
    *varToAdjust = pixelsMinMax[0];
    return;
  }
}

void setPixelCurrentSpeed(const word pixelSpeed[])
{
  pixelCurrentSpeed[0] = pixelSpeed[0];
  pixelCurrentSpeed[1] = pixelSpeed[1];
}

void pixelProcess(EggPixel &eggPixel)
{
  if (eggPixel.flashTimes > 0 && eggPixel.flash == true && eggPixel.status > 1) {

    if (pixelCurrentMode == 3) {

      strip.setPixelColor(
        eggPixel.position,
        strip.Color(
          eggPixel.rgb[0],
          eggPixel.rgb[1],
          eggPixel.rgb[2]
        )
      );

    } else {
      
      flashColorsCoef = random(0, 3);

      strip.setPixelColor(
        eggPixel.position,
        strip.Color(flashColors[flashColorsCoef][0], flashColors[flashColorsCoef][1], flashColors[flashColorsCoef][2])
      );
    }

    eggPixel.flashTimes--;
    eggPixel.flash = false;

  }else{

    if(eggPixel.flash == false){
      eggPixel.flash = true;
    }

    if (eggPixel.timer > 0) {

      if (eggPixel.timer > pixelWaitCycles) {
        eggPixel.status = 2;
        eggPixel.timer = 0;
      } else {
        eggPixel.timer++;
      }
    }

    if (eggPixel.status == 1) {

      eggPixel.currentBrightness += eggPixel.stepChangeBrightness;
      
      if (eggPixel.currentBrightness > eggPixel.maxBrightness) {
        eggPixel.currentBrightness = eggPixel.maxBrightness;
      }
      
      if (eggPixel.currentBrightness == eggPixel.maxBrightness) {
        eggPixel.timer = 1;
        eggPixel.status = 3;
      }
    
    } else {
      
      eggPixel.currentBrightness -= eggPixel.stepChangeBrightness;
      
      if (eggPixel.currentBrightness < 0) {
        eggPixel.currentBrightness = 0;
      }
    }

    strip.setPixelColor(
      eggPixel.position,
      strip.Color(
        eggPixel.rgb[0] * eggPixel.currentBrightness / 255,
        eggPixel.rgb[1] * eggPixel.currentBrightness / 255,
        eggPixel.rgb[2] * eggPixel.currentBrightness / 255
      )
    );
  }

  if (eggPixel.currentBrightness == 0 && eggPixel.status == 2) {

    pixelsCntActive--;

    if (eggPixel.maxBrightnessHigh) {
      pixelDifferentBrightness--;
    }

    eggPixel.flashTimes = 0;
    eggPixel.status = 0;

    stripFreePositions[eggPixel.position] = true;
  }
}

void pixelCreate(EggPixel &eggPixel)
{
  if (pixelsCntActive < pixelsCntCurrent) {

    pixelsCntActive++;

    bool isPosFree = false;
    word rndPos;
    do {
      rndPos = random(STRIP_LENGTH);
      isPosFree = stripFreePositions[rndPos];
    } while (!isPosFree);
    
    eggPixel.position = rndPos;
    stripFreePositions[eggPixel.position] = false;

    eggPixel.maxBrightness = random(51, 87);
    eggPixel.maxBrightnessHigh = false;

    if (pixelDifferentBrightness < pixelDifferentBrightnesses) {

      eggPixel.maxBrightness = random(170, 256);
      eggPixel.maxBrightnessHigh = true;

      pixelDifferentBrightness++;
    }

    if ((pixelCurrentMode > 0) && (stripFlash < stripFlashTotal)) {

      stripFlash++;

      eggPixel.flashTimes = random(flashTimesMinMax[0], flashTimesMinMax[1] + 1);
    }

    if ((eggPixel.position >= stripTopColumnStart) && (eggPixel.maxBrightness > 65)) {
      eggPixel.maxBrightness = 65;
    }

    if (pixelSpeedIndex > 0) {
      eggPixel.maxBrightness += 40;
    }

    speedCoef = random(pixelCurrentSpeed[1], pixelCurrentSpeed[0] + 1) + 0.0;
    eggPixel.stepChangeBrightness = eggPixel.maxBrightness / speedCoef;
    if(eggPixel.stepChangeBrightness <= 0.03){
      eggPixel.stepChangeBrightness = 0.03;
    }

    if (pixelCurrentMode == 3) {
      eggPixel.rgb[0] = random(0, 256);
      eggPixel.rgb[1] = random(0, 256);
      eggPixel.rgb[2] = random(0, 256);
    }

    if (eggPixel.maxBrightness > 255) {
      eggPixel.maxBrightness = 255;
    }

    eggPixel.currentBrightness = 0;
    eggPixel.timer = 0;
    eggPixel.status = 1;
  }
}

void stripReset(EggPixel pixels[])
{
  strip.clear();

  setPixelCurrentSpeed(pixelSpeedHigh);
  adjustPixelsCntByStep(&pixelsCntCurrent);

  pixelDifferentBrightnesses = round(pixelsCntCurrent / 4);

  pixelDifferentBrightness = 0;
  stripFlashTotal = 0;
  stripFlash = 0;
  pixelsCntActive = 0;

  word b = 0;
  while (b < PIXELS_CNT) {

    pixels[b] = EggPixel();
    pixels[b].rgb[0] = (pixelCurrentMode == 3) ? random(0, 256) : pixelRGBColor[0];
    pixels[b].rgb[1] = (pixelCurrentMode == 3) ? random(0, 256) : pixelRGBColor[1];
    pixels[b].rgb[2] = (pixelCurrentMode == 3) ? random(0, 256) : pixelRGBColor[2];

    b++;
  }

  b = 0;
  while (b < STRIP_LENGTH) {
    stripFreePositions[b] = true;
    b++;
  }
}

void pixelModeSwitcher()
{
    if (pixelCurrentMode == 3) {
      pixelCurrentMode = 0;
    } else {
      pixelCurrentMode++;
    }

    switch (pixelCurrentMode) {
      case 0:
        onboardDotstarLedColors[0].red = 0;
        onboardDotstarLedColors[0].green = 127;
        onboardDotstarLedColors[0].blue = 0;
        break;
      case 1:
        onboardDotstarLedColors[0].red = 127;
        onboardDotstarLedColors[0].green = 0;
        onboardDotstarLedColors[0].blue = 0;
        break;
      case 2:
        onboardDotstarLedColors[0].red = 0;
        onboardDotstarLedColors[0].green = 0;
        onboardDotstarLedColors[0].blue = 0;
        break;
      case 3:
        onboardDotstarLedColors[0].red = 0;
        onboardDotstarLedColors[0].green = 0;
        onboardDotstarLedColors[0].blue = 127;
        break;
    }
    onboardDotStarLed.write(onboardDotstarLedColors, 1, 1);

    stripReset(pixels);
}

void touchEvent()
{
  if (buttonTouchCoef > 500) {

    if (pixelSpeedIndex == 0) {

      digitalWrite(LED_BUILTIN, HIGH);

      timer.touchEventPixelSpeed = timer.tick;
      timer.pixelFlashEnabled = false;

      pixelSpeedTrigger = true;
      pixelSpeedDelay = pixelSpeedDelayInit;
    }
  }

  if (pixelSpeedTrigger || (pixelSpeedIndex > 0)) {

    if ((timer.tick - timer.touchEventPixelSpeed) > (pixelSpeedDelay * 1000)) {

      if (pixelSpeedTrigger) {

        pixelSpeedTrigger = false;
        pixelSpeedDelay = pixelSpeedDelayStep;

        if(stripFlashTotal == 0){
          stripFlashTotal = stripPixelsFlash[random(stripPixelsFlashStartIndex, stripPixelsFlashLength)];
          stripFlash = 0;
        }
      }

      if (pixelSpeedIndex == 0) {

        setPixelCurrentSpeed(pixelSpeedLow);
        pixelSpeedIndex = 1;

      } else if (pixelSpeedIndex == 1) {

        setPixelCurrentSpeed(pixelSpeedMedium);
        pixelSpeedIndex = 2;

      } else {

        setPixelCurrentSpeed(pixelSpeedHigh);
        pixelSpeedIndex = 0;

        timer.pixelFlashEnabled = true;

        digitalWrite(LED_BUILTIN, LOW);
      }

      timer.touchEventPixelSpeed = timer.tick;
    }
  }
}

void timers()
{
  if (timer.tick - timer.pixelCnt >= timerPixelCnt) {
    timer.pixelCnt = timer.tick;
    if (pixelSpeedIndex == 0 && stripFlashTotal == 0) {
      adjustPixelsCntByStep(&pixelsCntCurrent);
      pixelDifferentBrightnesses = round(pixelsCntCurrent / 4);
    }
  }

  if (timer.tick - timer.pixelFlash >= timerPixelFlash) {
    timer.pixelFlash = timer.tick;

    if (timer.pixelFlashEnabled && pixelCurrentMode > 0) {
      stripFlashTotal = stripPixelsFlash[random(0, stripPixelsFlashLength)];
      stripFlash = 0;
    }

    digitalWrite(LED_BUILTIN, (digitalRead(LED_BUILTIN) == LOW) ? HIGH : LOW);
  }
}

void setup()
{
  // onboard red LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // touch
  randomSeed(analogRead(BUTTON_TOUCH));
  buttonTouch.begin();

  // onboard DotStar pixel
  onboardDotstarLedColors[0].red = 0;
  onboardDotstarLedColors[0].green = 127;
  onboardDotstarLedColors[0].blue = 0;
  onboardDotStarLed.write(onboardDotstarLedColors, 1, 1);

  strip.begin();
  strip.show();
  strip.setBrightness(255);

  stripReset(pixels);
}

void loop()
{
  buttonTouchCoef = buttonTouch.measure();

  timer.tick = millis();

  buttonSwitch.update();

  if (buttonSwitch.isLongClick()) {
    pixelModeSwitcher();
  }

  if(buttonTouchCoef > 500 || pixelSpeedTrigger || pixelSpeedIndex > 0){
    touchEvent();
  }

  timers();

  for (byte cnt = 0; cnt < PIXELS_CNT; cnt++) {

    if (pixels[cnt].status > 0) {

      pixelProcess(pixels[cnt]);

    } else {

      pixelCreate(pixels[cnt]);
    }
  }

  strip.show();
}
