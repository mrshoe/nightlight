// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// released under the GPLv3 license to match the rest of the AdaFruit NeoPixel library

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
    #include <avr/power.h>
#endif

#include <Encoder.h>

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN                        8

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS            60

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

class Color {
    public:
    int r, g, b;

    Color(int rr, int gg, int bb) : r(rr), g(gg), b(bb) {}

    uint32_t pxcolor() {
        return pixels.Color(r, g, b);
    }

    friend Color operator*(Color lhs, int rhs) {
        return Color((rhs*lhs.r) / 255, (rhs*lhs.g) / 255, (rhs*lhs.b) / 255);
    }
};

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
        digitalWrite(p, HIGH);
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

class RotaryEncoder {
    Encoder enc;
    int value = 255;
    int last_enc = 0;
    int mult;

    public:
    RotaryEncoder(int e0, int e1, int m) : enc(e0, e1), mult(m) {}

    void update() {
        int curr_enc = enc.read();
        int change = curr_enc - last_enc;
        last_enc = curr_enc;
        value -= change * mult;
        value = min(255, max(0, value));
    }

    void set_mult(int m) { mult = m; }
    int get_value() { return value; }
};
RotaryEncoder rotary_enc(3, 4, 4);

class Mode {
    int pixel_offset = 0;
    int frame_delay = 15;
    int brightness = 255;
    bool transitioning = false;

    public:
    void setup() {
        pixel_offset = 0;
        transitioning = true;
    };
    virtual Color pixel_color(int idx) {}
    unsigned long loop() {
        unsigned long result = frame_delay;
        if (transitioning) {
            Color color = pixel_color(pixel_offset) * brightness;
            pixels.setPixelColor(pixel_offset, color.pxcolor());
            result = 5;
        }
        else {
            for (int i=0; i < NUMPIXELS; i++) {
                int px = (pixel_offset + i) % NUMPIXELS;
                Color color = pixel_color(i) * brightness;
                pixels.setPixelColor(px, color.pxcolor());
            }
        }
        pixels.show();
        pixel_offset++;
        if (pixel_offset >= NUMPIXELS) {
            if (transitioning) {
                Serial.println("transition complete");
            }
            transitioning = false;
            pixel_offset = 0;
        }
        return result;
    }
    void handle_knob_pushed() {}
    void set_brightness(int b) { brightness = max(8, b); }
};

class SolidMode : public Mode {
    int r, g, b;
    public:
    SolidMode(int rr, int gg, int bb) : r(rr), g(gg), b(bb) {}

    Color pixel_color(int idx) {
        return Color(r, g, b);
    }
};
SolidMode warm_mode(140, 110, 60);
SolidMode red_mode(100, 0, 0);
SolidMode white_mode(50, 50, 50);
SolidMode off_mode(0, 0, 0);

class RainbowMode : public Mode {
    Color pixel_color(int i) {
        int maxval = 80;
        int np3 = NUMPIXELS/3;
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

        return Color(red,green,blue);
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
    Color pixel_color(int i) {
        int *color = segments[i/ (NUMPIXELS/NUMSEGMENTS)];
        return Color(color[0], color[1], color[2]);
    }
};
SegmentMode segment_mode;

class DashesMode : public Mode {
    int width;
    int r, g, b;
    public:
    DashesMode(int rr, int gg, int bb) : width(4), r(rr), g(gg), b(bb) {}

    Color pixel_color(int i) {
        return (i%width) < (width/2) ? Color(0, 0, 0) : Color(r, g, b);
    }
};
DashesMode dashes_mode(0, 0, 80);

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

int curr_mode_idx = 0;
Mode *curr_mode = &off_mode;
unsigned long next_tick = 0;
void switch_mode(Mode *new_mode) {
    curr_mode = new_mode;
    curr_mode->setup();
    next_tick = 0;
}

void setup() {
#if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
    pixels.begin();
    pixels.show();
    Serial.begin(9600);
}

void loop() {
    unsigned long tick_delay = 15;
    power_button.update();
    mode_button.update();
    knob_button.update();
    rotary_enc.update();

    if (power_button.is_pushed()) {
        if (curr_mode == &off_mode) {
            Serial.println("power on");
            switch_mode(modes[curr_mode_idx]);
        }
        else {
            Serial.println("power off");
            curr_mode_idx = 0;
            switch_mode(&off_mode);
        }
    }
    else {
        if (mode_button.is_pushed()) {
            curr_mode_idx = (curr_mode_idx+1) % MODE_MAX;
            switch_mode(modes[curr_mode_idx]);
        }
        else if (knob_button.is_pushed()) {
            curr_mode->handle_knob_pushed();
        }
        if (millis() >= next_tick) {
            curr_mode->set_brightness(rotary_enc.get_value());
            next_tick = curr_mode->loop() + millis();
        }
        unsigned long till_next = next_tick - millis();
        tick_delay = min(15, till_next);
    }
    delay(tick_delay);
}
