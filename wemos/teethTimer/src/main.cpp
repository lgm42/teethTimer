#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
 
#define LED_PIN_NUMBER  D3
#define WAKE_UP_PIN_NUMBER  D4
#define NUMBER_OF_TIMERS  3
#define PIXELS_PER_TIMER  4
#define PIXEL_STATUS_OFFSET  0
#define PIXEL_PROGRESS_1_OFFSET  1
#define PIXEL_PROGRESS_2_OFFSET  2
#define PIXEL_PROGRESS_3_OFFSET  3

const int BUTTONS_PIN_NUMBER[] = {D4, D5, D6};

#define LOOP_DELAY_MS             100
#define TIME_FOR_SHUTDOWN_ALL     5000
#define TIME_FOR_SHUTDOWN_TIMER   2000
#define TIME_FOR_START_TIMER      100

#define TIMER_DURATION_MS         24 * 1000   //3 * 60 * 1000
#define FINISHED_ANIMATION_DURATION_MS  20 * 1000
#define TIME_TO_WAIT_BEFORE_SHUTDOWN  5 * 1000//60 * 1000
void refreshLeds();
void changeState();

enum TimerStatus
{
  kDeactivated,
  kRunning,
  kFinished
};
struct IndividualTimer
{
  TimerStatus status;
  int elapsedTime;
  int attachedGpio;
  int gpioPushedTimeCounter;
};

//Ticker _ticker;
IndividualTimer _timers[NUMBER_OF_TIMERS];
bool _hasToDeepSleep;
Adafruit_NeoPixel _pixels(4 * 4, LED_PIN_NUMBER, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  Serial.println("setup ...");
  _hasToDeepSleep = false;
  for (int i = 0; i < NUMBER_OF_TIMERS; ++i)
  {
    _timers[i].status = kDeactivated;
    _timers[i].elapsedTime = 0;
    _timers[i].attachedGpio = BUTTONS_PIN_NUMBER[i];
    _timers[i].gpioPushedTimeCounter = 0;
    pinMode(BUTTONS_PIN_NUMBER[i], INPUT_PULLUP);
  }

  _pixels.begin();
}

void loop() {
  //in the same time we check if the timer is running or has been finished since not enough time to deep sleep 
  bool hasToDeepSleep = true;
  for (int i = 0; i < NUMBER_OF_TIMERS; ++i)
  {
    //button survey
    if (digitalRead(_timers[i].attachedGpio) == 0)
    {
      Serial.printf("button pressed %d\n", i);
      hasToDeepSleep = false;
      //button is pressed
      _timers[i].gpioPushedTimeCounter+= LOOP_DELAY_MS;
    }
    else
    {
      // button is released
      Serial.printf("timer [%d] count = %d\n", i, _timers[i].gpioPushedTimeCounter);
      if (_timers[i].gpioPushedTimeCounter > TIME_FOR_SHUTDOWN_ALL)
      {
        Serial.println("Shutdown all");
      }
      else if (_timers[i].gpioPushedTimeCounter > TIME_FOR_SHUTDOWN_TIMER)
      {
        Serial.printf("Shutdown timer %d\n", i);
        _timers[i].status = kDeactivated;
      }
      else if ((_timers[i].gpioPushedTimeCounter > TIME_FOR_START_TIMER) && (_timers[i].status != kRunning))
      {
        Serial.printf("Starting timer %d\n", i);
        _timers[i].status = kRunning;
        _timers[i].elapsedTime = 0;
      }
      _timers[i].gpioPushedTimeCounter = 0;
    }

    if (_timers[i].status == kRunning)
    {
      hasToDeepSleep = false;
      _timers[i].elapsedTime += LOOP_DELAY_MS;
      Serial.printf("timer [%d] elapsedTime = %d\n", i, _timers[i].elapsedTime);
      if (_timers[i].elapsedTime > TIMER_DURATION_MS)
      {
        _timers[i].status = kFinished;
        //we re-use elapsed attr for finished animation
        _timers[i].elapsedTime = 0;
      }
    }

    if (_timers[i].status == kFinished)
    {
      _timers[i].elapsedTime += LOOP_DELAY_MS;
      if (_timers[i].elapsedTime < TIME_TO_WAIT_BEFORE_SHUTDOWN)
        hasToDeepSleep = false;
    }
  }

  refreshLeds();

  if (hasToDeepSleep)
  {
    //TODO : cut off leds
    //ESP.deepSleep(0);
  }

  delay(LOOP_DELAY_MS);
}

void refreshLeds()
{
  //we fill pixel array from data
  _pixels.clear();
  const uint32_t progressColor = Adafruit_NeoPixel::Color(0x00, 0xFF, 0x00);
  for (int i = 0; i < NUMBER_OF_TIMERS; ++i)
  {
    if (_timers[i].status == kDeactivated)
    {
      //nothing to do
    }
    else if (_timers[i].status == kRunning)
    {
      uint32_t color; 
      if ((_timers[i].elapsedTime / 1000) & 1)
        color = Adafruit_NeoPixel::Color(0x00, 0x00, 0x00);
      else
        color = Adafruit_NeoPixel::Color(0xFF, 0x00, 0x00);
      
      _pixels.setPixelColor(i * PIXELS_PER_TIMER + PIXEL_STATUS_OFFSET, color);

      if (_timers[i].elapsedTime > TIMER_DURATION_MS / 3)
        _pixels.setPixelColor(i * PIXELS_PER_TIMER + PIXEL_PROGRESS_1_OFFSET, progressColor);

      if (_timers[i].elapsedTime > 2 * TIMER_DURATION_MS / 3)
        _pixels.setPixelColor(i * PIXELS_PER_TIMER + PIXEL_PROGRESS_2_OFFSET, progressColor);
    }
    else if (_timers[i].status == kFinished)
    { 
      if (_timers[i].elapsedTime < FINISHED_ANIMATION_DURATION_MS)
      {
        uint32_t color; 
        if ((_timers[i].elapsedTime / 1000) & 1)
          color = Adafruit_NeoPixel::Color(0x00, 0x00, 0x00);
        else
          color = Adafruit_NeoPixel::Color(0x00, 0xFF, 0x00);
        
        _pixels.setPixelColor(i * PIXELS_PER_TIMER + PIXEL_STATUS_OFFSET, color);
        _pixels.setPixelColor(i * PIXELS_PER_TIMER + PIXEL_PROGRESS_1_OFFSET, color);
        _pixels.setPixelColor(i * PIXELS_PER_TIMER + PIXEL_PROGRESS_2_OFFSET, color);
        _pixels.setPixelColor(i * PIXELS_PER_TIMER + PIXEL_PROGRESS_3_OFFSET, color);
      }
      else
      {
        //leds are all off
      }
      
      
    }
  }
  _pixels.show();
}