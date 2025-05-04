#include <Adafruit_NeoPixel.h>
#include <APA102.h>
#include <arduino-timer.h>
#include <PinButton.h>
#include <Adafruit_FreeTouch.h>

const byte PIXELS_CNT = 35; // pixelsMinMax[1]
const byte NEOPIXEL_DATA = 1;
const word STRIP_LENGTH = 288;
const byte BUTTON_SWITCH = 0;
const byte BUTTON_TOUCH = A3;
const byte DOTSTAR_DATA = 7;
const byte DOTSTAR_CLK = 8;
const byte ONBOARD_LED = 13;

byte pixelCurrentMode = 1; // [0 - no flash, 1 - flash, 2 - flash reduced pixels (power saving), 3 - RGB]
byte pixelCurrentSubmode = 0; // [0 - adjust val by step, 1 - adjust val by random]

Adafruit_FreeTouch buttonTouch = Adafruit_FreeTouch(BUTTON_TOUCH, OVERSAMPLE_4, RESISTOR_50K, FREQ_MODE_NONE); 

PinButton buttonSwitch(BUTTON_SWITCH);

APA102<DOTSTAR_DATA, DOTSTAR_CLK> onboardDotStarLed;
rgb_color onboardDotstarLedColors[1];

Adafruit_NeoPixel strip(STRIP_LENGTH, NEOPIXEL_DATA, NEO_GRB + NEO_KHZ800);

auto timer = timer_create_default();

const byte stripTopColumnStart = 216;
const byte pixelRGBColor[] = {174, 109, 30};
const byte pixelSpeedHigh[2] = {100, 90};
const byte pixelSpeedMedium[2] = {70, 50};
const byte pixelSpeedLow[2] = {50, 44};
const byte pixelSpeedDelayInit = 2; // timeout po touch eventu [s]
const byte pixelSpeedDelayStep = 10; // timeout mezi kroky [s]
const byte pixelsMinMax[2] = {25, 36}; // pocet pixelu najednou (min a max interval)
const byte pixelsMinMaxReduced[2] = {5, 20}; // pocet pixelu najednou pro mode 2 (min a max interval)
const byte stripCurrentModeTimer[2] = {70, 255}; // [s] interval pro random mode timer
const byte stripPixelsFlash[15] = {0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; // pocet pixelu na zablesk
const byte stripPixelsFlashLength = 15;
const byte flashTimesMinMax[2] = {1, 7}; // kolikrat se ma bliknout
const byte stripPixelsFlashStartIndex = 5; // index alespon 1x zablesk
const byte stripFlashTimer[2] = {15, 60}; // [s] interval pro random flash timer
const byte pixelWaitCycles = 15; // prodleva pixelu na mbr pro mode 1 a 2 [pocet cyklu]

typedef struct EggPixel
{
  byte timer = 0;
  byte status = 0;
  word position = 0;
  float currentBrightness = 0;
  float stepChangeBrightness = 0;
  bool maxBrightnessHigh = false;
  int maxBrightness = 0;
  byte flashTimes = 0;
  byte rgb[3];
} EggPixel;

EggPixel pixels[PIXELS_CNT];
word buttonTouchCoef = 0;
word stripCurrentModeTimerSet = 30;
bool pixelSpeedTrigger = false;
bool pixelRGBColorChange = false;
bool timer1Trigger = true;
byte pixelsCntActive = 0;
byte stripFlash = 0;
byte stripFlashTotal = 0;
byte stripFlashTimerSet = 30;
word stripFreePositions[STRIP_LENGTH];
byte pixelSpeedIndex = 0;
unsigned long pixelSpeedTimer = 0;
byte pixelSpeedDelay = 0;
byte pixelDifferentBrightness = 0;
byte pixelsCntCurrent = 0;
byte pixelDifferentBrightnesses = 0;
byte pixelCurrentSpeed[2];

void turnOnboardLEDOn()
{
  digitalWrite(ONBOARD_LED, HIGH);
}

void turnOnboardLEDOff()
{
  digitalWrite(ONBOARD_LED, LOW);
}

void adjustValByRandomOrStep(byte *varToAdjust)
{
  if (varToAdjust == 0 || pixelCurrentSubmode == 1) {
    
    *varToAdjust = (pixelCurrentMode == 2)
      ? random(pixelsMinMaxReduced[0], pixelsMinMaxReduced[1])
      : random(pixelsMinMax[0], pixelsMinMax[1]);
    return;
  }

  byte currentStep = random(3);
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

void setPixelCurrentSpeed(const byte pixelSpeed[])
{
  pixelCurrentSpeed[0] = pixelSpeed[0];
  pixelCurrentSpeed[1] = pixelSpeed[1];
}

void pixelProcess(EggPixel &eggPixel)
{
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

  if (eggPixel.flashTimes > 0) {

    if ((eggPixel.currentBrightness == eggPixel.maxBrightness) || (eggPixel.status == 2)) {

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

        byte flashColors[3];
        byte flashColorsCoef = random(1, 4);

        flashColors[0] = round(255 / flashColorsCoef);
        flashColors[1] = round(162 / flashColorsCoef);
        flashColors[2] = round(46 / flashColorsCoef);

        strip.setPixelColor(
          eggPixel.position,
          strip.Color(flashColors[0], flashColors[1], flashColors[2])
        );
      }
      strip.show();

      eggPixel.flashTimes--;
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

  if ((eggPixel.currentBrightness == 0) && (eggPixel.status == 2)) {

    pixelsCntActive--;

    if (eggPixel.maxBrightnessHigh) {
      pixelDifferentBrightness--;
    }

    eggPixel.flashTimes = 0;
    eggPixel.status = 0;

    stripFreePositions[eggPixel.position] = eggPixel.position;
  }
}

void pixelCreate(EggPixel &eggPixel)
{

  if (pixelsCntActive < pixelsCntCurrent) {

    pixelsCntActive++;

    do {
      eggPixel.position = stripFreePositions[random(STRIP_LENGTH)];
    } while (eggPixel.position == 500);

    stripFreePositions[eggPixel.position] = 500;

    eggPixel.maxBrightness = random(51, 87);
    eggPixel.maxBrightnessHigh = false;

    if (pixelDifferentBrightness < pixelDifferentBrightnesses) {

      eggPixel.maxBrightness = random(170, 256);
      eggPixel.maxBrightnessHigh = true;

      pixelDifferentBrightness++;
    }

    if ((pixelCurrentMode > 0) && (stripFlash < stripFlashTotal)) {

      eggPixel.flashTimes = random(flashTimesMinMax[0], flashTimesMinMax[1] + 1);

      stripFlash++;

      if (stripFlash == stripFlashTotal) {
        stripFlashTotal = 0;
        stripFlash = 0;
      }
    }

    if ((eggPixel.position >= stripTopColumnStart) && (eggPixel.maxBrightness > 65)) {
      eggPixel.maxBrightness = 65;
    }
    if (pixelSpeedIndex > 0) {
      eggPixel.maxBrightness += 40;
    }

    float rnd = random(round(eggPixel.maxBrightness / pixelCurrentSpeed[0]), round(eggPixel.maxBrightness / pixelCurrentSpeed[1]));
    if (rnd <= 0) {
      rnd = 0.5;
    }
    eggPixel.stepChangeBrightness = rnd;

    if (pixelRGBColorChange) {

      eggPixel.rgb[0] = random(0, 256);
      eggPixel.rgb[1] = random(0, 256);
      eggPixel.rgb[2] = random(0, 256);

      pixelRGBColorChange = false;
    }

    if (eggPixel.maxBrightness > 255) {
      eggPixel.maxBrightness = 255;
    }

    eggPixel.currentBrightness = 0;
    eggPixel.timer = 0;
    eggPixel.status = 1;
  }
}

bool eventTimer3(void *)
{
  if (pixelCurrentMode == 3 && !pixelRGBColorChange) {
    pixelRGBColorChange = true;
  }
  return true;
}

bool eventTimer2(void *)
{
  if (pixelSpeedIndex == 0) {

    adjustValByRandomOrStep(&pixelsCntCurrent);

    pixelDifferentBrightnesses = round(pixelsCntCurrent / 5);
    stripCurrentModeTimerSet = random(stripCurrentModeTimer[0], stripCurrentModeTimer[1]);
  }
  return true;
}

bool eventTimer1(void *)
{
  if (timer1Trigger && stripFlash == 0 && pixelCurrentMode > 0) {
    stripFlashTotal = stripPixelsFlash[random(0, stripPixelsFlashLength)];
    stripFlashTimerSet = random(stripFlashTimer[0], stripFlashTimer[1]);
  }
  return true;
}

void stripReset(EggPixel pixels[])
{
  strip.clear();

  setPixelCurrentSpeed(pixelSpeedHigh);
  adjustValByRandomOrStep(&pixelsCntCurrent);

  pixelDifferentBrightnesses = round(pixelsCntCurrent / 5);

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
    stripFreePositions[b] = b;
    b++;
  }
}

void setup()
{
  // onboard red LED
  pinMode(ONBOARD_LED, OUTPUT);
  turnOnboardLEDOff();

  // touch
  randomSeed(analogRead(BUTTON_TOUCH));
  buttonTouch.begin();

  // timers
  timer.every(15 * 1000, eventTimer3);
  timer.every(stripCurrentModeTimerSet * 1000, eventTimer2);
  timer.every(stripFlashTimerSet * 1000, eventTimer1);

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

  timer.tick();

  buttonSwitch.update();

  if(buttonSwitch.isLongClick()){

    if(pixelCurrentSubmode == 0){
      pixelCurrentSubmode = 1;
      turnOnboardLEDOn();
    }else{
      pixelCurrentSubmode = 0;
      turnOnboardLEDOff();
    }
  }

  if (buttonSwitch.isClick()) {

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

  // touch event
  if (buttonTouchCoef > 500) {

    if (pixelSpeedIndex == 0) {

      //turnOnboardLEDOn();

      pixelSpeedTimer = millis();
      pixelSpeedTrigger = true;
      pixelSpeedDelay = pixelSpeedDelayInit;

      // prevent timer 1 event
      timer1Trigger = false;
    }
  }

  if (pixelSpeedTrigger || (pixelSpeedIndex > 0)) {

    if ((millis() - pixelSpeedTimer) > (pixelSpeedDelay * 1000)) {

      if (pixelSpeedTrigger) {

        pixelSpeedTrigger = false;
        pixelSpeedDelay = pixelSpeedDelayStep;

        if (stripFlash == 0) {

          stripFlashTotal = stripPixelsFlash[random(stripPixelsFlashStartIndex, stripPixelsFlashLength)];
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

        // restore timer 1 event
        timer1Trigger = true;

        //turnOnboardLEDOff();
      }

      pixelSpeedTimer = millis();
    }
  }

  for (byte cnt = 0; cnt < PIXELS_CNT; cnt++) {

    if (pixels[cnt].status > 0) {

      pixelProcess(pixels[cnt]);

    } else {

      pixelCreate(pixels[cnt]);
    }
  }

  strip.show();
}
