// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// released under the GPLv3 license to match the rest of the AdaFruit NeoPixel library

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN            8

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      60

class Timer {
  unsigned long _duration;
  unsigned long _start;

public:
  void set(unsigned long dur) { _duration = dur; }
  void start() { _start = millis(); }
  bool fired() { return (millis() - _start) > _duration; }
};

enum {
  TIMER_MODE,
  TIMER_MAX,
};

Timer timers[TIMER_MAX];

class Button {
  int pin;
  unsigned long last_push;
  bool pushed;
  int state;
  
  public:
  Button(int p) : pin(p) {
    pinMode(p, INPUT);
  }

  void update() {
    int new_state = digitalRead(pin);
    pushed = false;
    
    if (new_state && !state) {
      pushed = true;
      last_push = millis();
    }
    state = new_state;
  }

  bool is_pushed() { return pushed; }
};

Button power_button(10), mode_button(11), knob_button(12);

class LED {
  int pin;

  public:
  LED(int p) : pin(p) {
    pinMode(p, OUTPUT);
  }
  void set_brightness(int brightness) {
    analogWrite(pin, brightness);
  }
};

LED power_led(4), mode_led(5);

class RGBLED {
  int rpin, gpin, bpin;

  public:
  RGBLED(int rp, int gp, int bp) : rpin(rp), gpin(gp), bpin(bp) {
    pinMode(rp, OUTPUT);
    pinMode(gp, OUTPUT);
    pinMode(bp, OUTPUT);
  }
  void set_color(int r, int g, int b) {
    analogWrite(rpin, r);
    analogWrite(gpin, g);
    analogWrite(bpin, b);
  }
};

RGBLED knob_led(6, 7, 8);

class Mode {
  unsigned long yield_time;

  public:
  int start_pixel;

  void setup() {
    yield_time = 0;
    start_pixel = 0;
    update_pixels(true);
  };
  void update_pixels(bool transitioning) {};
  void loop() {
    if (millis() > yield_time) {
      update_pixels(false);
    }
  }
  void yield(unsigned long ms) { yield_time = millis() + ms; }
  void handle_knob_pushed() {}
};

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

class SolidMode : public Mode {
  int r, g, b;
  public:
  SolidMode(int rr, int gg, int bb) : r(rr), g(gg), b(bb) {}
  
  void update_pixels(bool t) {
    for(int i=0;i<NUMPIXELS;i++) {
      pixels.setPixelColor(i, pixels.Color(r,g,b));
      pixels.show();
      delay(5);
    }
    yield(10000);
  }
};
SolidMode warm_mode(140, 110, 60);
SolidMode red_mode(100, 0, 0);
SolidMode white_mode(50, 50, 50);

class RainbowMode : public Mode {
  void update_pixels(bool transitioning) {
    for(int i=0;i<NUMPIXELS;i++) {
      int maxval = 80;
      int np3 = NUMPIXELS/3;
      int px = (start_pixel + i) % NUMPIXELS;
      int red = 0, green = 0, blue = 0;

      if (i < np3) {
        red = maxval - ((i*maxval) / np3);
        green = (i*maxval) / np3;
        blue = 0;
      }
      else if (i < (np3*2)) {
        int newi = i - np3;
        red = 0;
        green = maxval - ((newi*maxval) / np3);
        blue = (newi*maxval) / np3;
      }
      else if (i < NUMPIXELS) {
        int newi = i - (np3*2);
        red = (newi*maxval) / np3;
        blue = maxval - ((newi*maxval) / np3);
        green = 0;
      }

      pixels.setPixelColor(px, pixels.Color(red,green,blue));
      if (transitioning) {
        pixels.show();
        delay(5);
      }
    }
    pixels.show();
    start_pixel++;
    yield(50);
  }
};
RainbowMode rainbow_mode;

#define NUMSEGMENTS (6)
int seg_max = 80;
int segments[NUMSEGMENTS][3] = {
  {seg_max, 0, 0},
  {seg_max, seg_max, 0},
  {0, seg_max, 0},
  {0, seg_max, seg_max},
  {0, 0, seg_max},
  {seg_max, 0, seg_max},
};

class SegmentMode : public Mode {
  void update_pixels(bool transitioning) { 
    for(int i=0;i<NUMPIXELS;i++) {

      int *color = segments[i/ (NUMPIXELS/NUMSEGMENTS)];
      int px = (start_pixel + i) % NUMPIXELS;
      pixels.setPixelColor(px, pixels.Color(color[0], color[1], color[2]));
    
      if (transitioning) {
        pixels.show();
        delay(5);
      }
    }
    pixels.show();
    yield(30);
  }
};
SegmentMode segment_mode;

class DashesMode : public Mode {
  int width;
  public:
  DashesMode() : width(4) {}
  
  void update_pixels(bool transitioning) { 
    for(int i=0;i<NUMPIXELS;i++) {
      int color = (i%width) < (width/2) ? 0 : 80;
      int px = (start_pixel + i) % NUMPIXELS;
      pixels.setPixelColor(px, pixels.Color(0, 0, color));
    
      if (transitioning) {
        pixels.show();
        delay(5);
      }
    }
    pixels.show();
    yield(250);
  }
};
DashesMode dashes_mode;

typedef enum {
  MODE_WARM,
  MODE_RED,
  MODE_WHITE,
  MODE_RAINBOW,
  MODE_SEGMENTS,
  MODE_DASHES,
  

  MODE_MAX,
} LightMode;

Mode *modes[MODE_MAX] = {
  &warm_mode,
  &red_mode,
  &white_mode,
  &rainbow_mode,
  &segment_mode,
  &dashes_mode,
};

int curr_mode = 0;

void setup() {
  Serial.begin(9600);
  timers[TIMER_MODE].set(5000);
  timers[TIMER_MODE].start();

}

void loop() {
  static int tick_delay = 15;
  power_button.update();
  mode_button.update();
  knob_button.update();

  if (power_button.is_pushed()) {
    if (curr_mode < 0) {
      Serial.println("power on");
      curr_mode = MODE_WARM;
      //modes[curr_mode]->setup();
      tick_delay = 15;
    }
    else {
      Serial.println("power off");
      curr_mode = -1;
      tick_delay = 15;/*
      for(int i=NUMPIXELS-1;i>=0;i--) {
        pixels.setPixelColor(i, pixels.Color(0,0,0));
        pixels.show();
        delay(5);
      }*/
    }
  }
  else if (curr_mode >= 0) {
    if (mode_button.is_pushed()) {
      curr_mode = (curr_mode+1) % MODE_MAX;
      modes[curr_mode]->setup();
    }
    else if (knob_button.is_pushed()) {
      modes[curr_mode]->handle_knob_pushed();
    }
    modes[curr_mode]->loop();
  }
  delay(tick_delay);

/*
  if (timers[TIMER_MODE].fired()) {
    timers[TIMER_MODE].start();
    curr_mode = (curr_mode+1) % MODE_MAX;
    modes[curr_mode]->setup();
  }
  */
}
