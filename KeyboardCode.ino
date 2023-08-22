// Libraries
#include <PicoGamepad.h>
#include <FastLED.h>

// Pins
#define PIN_BTN1      6
#define PIN_BTN2      7
#define PIN_BTN5      8
#define PIN_BTN6      9
#define PIN_BTN7      10
#define PIN_LED       16
#define PIN_MUX_CTRL  17
#define PIN_BTN10     18
#define PIN_BTN9      19
#define PIN_BTN8      20
#define PIN_BTN4      21
#define PIN_BTN3      22
#define PIN_MUX1      26
#define PIN_MUX2      27

// Parameters
#define NUM_LEDS      26
#define X_MIN         200
#define X_MAX         1200
#define Y1_MIN        900
#define Y1_MAX        1800
#define Y2_MIN        300
#define Y2_MAX        900
#define Z_MIN         200
#define Z_MAX         1200
#define JOYSTICK_MIN  -32767
#define JOYSTICK_MAX  32767
#define MAX_RATE_T    1800
#define MAX_RATE_S    100
#define SWITCH_TIME   3000

// Objects
PicoGamepad gamepad;
CRGB leds[NUM_LEDS];

// Variables
volatile int x, y, z, z1, x1, y11, y2;
volatile int up, down, left, right;
volatile int btn5, btn6, btn7, btn8, btn9, btn10;
volatile int last_up, last_down, last_left, last_right;
volatile int dynamic_x_min, dynamic_left_min, dynamic_right_min, dynamic_z_min, last_x, last_y1, last_y2, last_z, last_y;
int start_time = 0;

// Different modes
bool debug_mode         = false;
bool experimental_mode  = true; 
bool rate_limiter       = true;
bool rate_return        = true;
bool analog_mode        = false;

// Functions
void UpdateJoystick()
{
  // Updating the 3 axis first
  gamepad.SetX(x);  // Throttle
  gamepad.SetY(y);  // Steering
  gamepad.SetZ(z);  // Brake
  
  // Updating all of the buttons
  gamepad.SetButton(0, btn5);
  gamepad.SetButton(1, btn6);
  gamepad.SetButton(2, btn7);
  gamepad.SetButton(3, btn8);
  gamepad.SetButton(4, btn9);
  gamepad.SetButton(5, btn10);

  // Sending the updated positions to the PC
  gamepad.send_update();
}

void ReadInputs()
{
  // Reading all of the inputs
  digitalWrite(PIN_MUX_CTRL, 0);
  y11   = analogRead(PIN_MUX1);
  z1    = analogRead(PIN_MUX2);
  digitalWrite(PIN_MUX_CTRL, 1);
  up    = digitalRead(PIN_BTN1);
  left  = digitalRead(PIN_BTN2);
  down  = digitalRead(PIN_BTN3);
  right = digitalRead(PIN_BTN4);
  btn5  = digitalRead(PIN_BTN5);
  btn6  = digitalRead(PIN_BTN6);
  btn7  = digitalRead(PIN_BTN7);
  btn8  = digitalRead(PIN_BTN8);
  btn9  = digitalRead(PIN_BTN9);
  btn10 = digitalRead(PIN_BTN10);
  x1    = analogRead(PIN_MUX1);
  y2    = analogRead(PIN_MUX2);
  digitalWrite(PIN_MUX_CTRL, 0);

  // We can't have both left and right steering at the same time
  if(left == right)
  {
    y = 0;
  }
  else if(right == 1)
  {
    y = map(y2, Y2_MIN, Y2_MAX, 0, JOYSTICK_MAX);
  }
  else
  {
    y = map(y11, Y1_MIN, Y1_MAX, 0, JOYSTICK_MIN);
  }

  if(up == 0)
  {
    x = JOYSTICK_MIN;
  }
  else
  {
    x = map(x1, X_MIN, X_MAX, JOYSTICK_MIN, JOYSTICK_MAX);
  }

  if(down == 0)
  {
    z = JOYSTICK_MIN;
  }
  else
  {
    z = map(z1, Z_MIN, Z_MAX, JOYSTICK_MIN, JOYSTICK_MAX);
  }

  // Double checking that the values are in bounds
  if(x < JOYSTICK_MIN)    x = JOYSTICK_MIN;
  if(x > JOYSTICK_MAX)    x = JOYSTICK_MAX;
  if(y < JOYSTICK_MIN)    y = JOYSTICK_MIN;
  if(y > JOYSTICK_MAX)    y = JOYSTICK_MAX;
  if(z < JOYSTICK_MIN)    z = JOYSTICK_MIN;
  if(z > JOYSTICK_MAX)    z = JOYSTICK_MAX;
  // Additional 2 checks for the steering axis
  if(left == 1 && y > 0)  y = 0;
  if(right == 1 && y < 0) y = 0;
}

void ReadInputsExperimental()
{
  // Reading all of the inputs
  digitalWrite(PIN_MUX_CTRL, 0);
  y11   = analogRead(PIN_MUX1);
  z1    = analogRead(PIN_MUX2);
  digitalWrite(PIN_MUX_CTRL, 1);
  up    = digitalRead(PIN_BTN1);
  left  = digitalRead(PIN_BTN2);
  down  = digitalRead(PIN_BTN3);
  right = digitalRead(PIN_BTN4);
  btn5  = digitalRead(PIN_BTN5);
  btn6  = digitalRead(PIN_BTN6);
  btn7  = digitalRead(PIN_BTN7);
  btn8  = digitalRead(PIN_BTN8);
  btn9  = digitalRead(PIN_BTN9);
  btn10 = digitalRead(PIN_BTN10);
  x1    = analogRead(PIN_MUX1);
  y2    = analogRead(PIN_MUX2);
  digitalWrite(PIN_MUX_CTRL, 0);

  // Switching modes
  // To switch the modes, you need to hold down the left, right, and the first button on both sides
  if(left == 1 && right == 1 && btn5 == 1 && btn8 == 1)
  {
    if(millis() - start_time >= SWITCH_TIME) 
    {
      analog_mode = !analog_mode; 
      start_time = millis();
    }
  }
  else
  {
    start_time = millis();
  }

  if(analog_mode == true)
  {
    // Dynamic updating of the minimum values for all axis
    if(up == 1 && last_up == 0)       dynamic_x_min       = x1;
    if(down == 1 && last_down == 0)   dynamic_z_min       = z1;
    if(right == 1 && last_right == 0) dynamic_right_min   = y2;
    if(left == 1 && last_left == 0)   dynamic_left_min    = y11;
  
    // Because the FSR won't always return to 0, even if we start from it, we need to check whether we're higher than the bare minimum threshold
    if(dynamic_x_min < X_MIN)         dynamic_x_min       = X_MIN;
    if(dynamic_z_min < Z_MIN)         dynamic_z_min       = Z_MIN;
    if(dynamic_left_min < Y1_MIN)     dynamic_left_min    = Y1_MIN;
    if(dynamic_right_min < Y2_MIN)    dynamic_right_min   = Y2_MIN;
  
    // After all of the checks, update last_ flags
    last_up     = up;
    last_down   = down;
    last_left   = left;
    last_right  = right;
  
  
    // We can't have both left and right steering at the same time
    if(left == right)
    {
      y = 0;
    }
    else if(right == 1)
    {
      y = map(y2, dynamic_right_min, Y2_MAX, 0, JOYSTICK_MAX);
    }
    else
    {
      y = map(y11, dynamic_left_min, Y1_MAX, 0, JOYSTICK_MIN);
    }
  
    if(up == 0)
    {
      x = JOYSTICK_MIN;
    }
    else
    {
      x = map(x1, dynamic_x_min, X_MAX, JOYSTICK_MIN, JOYSTICK_MAX);
    }
  
    if(down == 0)
    {
      z = JOYSTICK_MIN;
    }
    else
    {
      z = map(z1, dynamic_z_min, Z_MAX, JOYSTICK_MIN, JOYSTICK_MAX);
    }
  
    // Rate limiter
    if(rate_limiter == true)
    {
      if(x - last_x > MAX_RATE_T)                 x = last_x + MAX_RATE_T;
      if(z - last_z > MAX_RATE_T)                 z = last_z + MAX_RATE_T;
      if(right == 1 && y - last_y > MAX_RATE_S)   y = last_y + MAX_RATE_S;
      if(left == 1 && last_y - y > MAX_RATE_S)    y = last_y - MAX_RATE_S;
      // Return steering wheel slowly to center
      if(rate_return == true)
      {
        if(right == 0 && left == 0 && last_y - y > MAX_RATE_S)  y = last_y - MAX_RATE_S;
        if(right == 0 && left == 0 && y - last_y > MAX_RATE_S)  y = last_y + MAX_RATE_S;
      }
    }
  
    // Double checking that the values are in bounds
    if(x < JOYSTICK_MIN)    x = JOYSTICK_MIN;
    if(x > JOYSTICK_MAX)    x = JOYSTICK_MAX;
    if(y < JOYSTICK_MIN)    y = JOYSTICK_MIN;
    if(y > JOYSTICK_MAX)    y = JOYSTICK_MAX;
    if(z < JOYSTICK_MIN)    z = JOYSTICK_MIN;
    if(z > JOYSTICK_MAX)    z = JOYSTICK_MAX;
    // Additional 2 checks for the steering axis
    if(left == 1 && y > 0)  y = 0;
    if(right == 1 && y < 0) y = 0;
  
    if(rate_limiter == true)
    {
      last_x  = x;
      last_y  = y;
      last_z  = z;
    }
  }
  else
  {
    if(up == 1)
    {
      x = JOYSTICK_MAX;
    }
    else
    {
      x = JOYSTICK_MIN;
    }

    if(down == 1)
    {
      z = JOYSTICK_MAX;
    }
    else
    {
      z = JOYSTICK_MIN;
    }

    if(right == left)
    {
      y = 0;
    }
    else if(right == 1)
    {
      y = JOYSTICK_MAX;
    }
    else
    {
      y = JOYSTICK_MIN;
    }
    
  }
}

void DebugPrint()
{
  Serial.println("--------------------------------------------------------------------------------");
  Serial.println("Arrow Keys");
  Serial.print("Up Button: ");
  Serial.println(up);
  Serial.print("Left Button: ");
  Serial.println(left);
  Serial.print("Down Button: ");
  Serial.println(down);
  Serial.print("Right Button: ");
  Serial.println(right);
  Serial.println("Forces on the buttons");
  Serial.print("Up force: ");
  Serial.println(x1);
  Serial.print("Down force: ");
  Serial.println(z1);
  Serial.print("Left force: ");
  Serial.println(y11);
  Serial.print("Right force: ");
  Serial.println(y2);
  Serial.print("Y axis: ");
  Serial.println(y);
  /*
  Serial.println("Other buttons");
  Serial.print("Button5: ");
  Serial.println(btn5);
  Serial.print("Button6: ");
  Serial.println(btn6);
  Serial.print("Button7: ");
  Serial.println(btn7);
  Serial.print("Button8: ");
  Serial.println(btn8);
  Serial.print("Button9: ");
  Serial.println(btn9);
  Serial.print("Button10: ");
  Serial.println(btn10);*/
}

void LED_back(int r, int g, int b)
{
  // This section includes these LEDs: 0, 1, 2, 3, 4
  for(int i = 0; i < 5; i++)
  {
    leds[i] = CRGB(g, r, b);
  }
  FastLED.show();
}

void LED_right(int r, int g, int b)
{
  // This section includes these LEDs: 5, 6, 7, 8, 9, 10
  for(int i = 5; i < 11; i++)
  {
    leds[i] = CRGB(g, r, b);
  }
  FastLED.show();
}

void LED_front(int r, int g, int b)
{
  // This section includes these LEDs: 11, 12, 13, 14, 15, 16, 17, 18
  for(int i = 11; i < 19; i++)
  {
    leds[i] = CRGB(g, r, b);
  }
  FastLED.show();
}

void LED_left(int r, int g, int b)
{
  // This section includes these LEDs: 19, 20, 21, 22, 23, 24, 25
  for(int i = 19; i < 26; i++)
  {
    leds[i] = CRGB(g, r, b);
  }
  FastLED.show();
}

void LED_all(int r, int g, int b)
{
  for(int i = 0; i < 26; i++)
  {
    leds[i] = CRGB(g, r, b);
  }
  FastLED.show();
}

void UpdateLEDs()
{

  if(analog_mode == true)
  {
    // Upper LEDs
    if(up == 1)
    {
      if(x == JOYSTICK_MAX)
      {
        LED_front(250, 0, 0);
      }
      else
      {
        int color_val = map(x, JOYSTICK_MIN, JOYSTICK_MAX, 0, 255);
        LED_front(0, color_val, color_val);
      }
    }
    else
    {
      LED_front(0, 0, 0);
    }
  
    // Left LEDs
    if(left == 1)
    {
      if(y == JOYSTICK_MIN)
      {
        LED_left(250, 0, 0);
      }
      else
      {
        int color_val = map(y, 0, JOYSTICK_MIN, 0, 255);
        LED_left(0, color_val, color_val);
      }
    }
    else
    {
      LED_left(0, 0, 0);
    }
  
    // Right LEDs
    if(right == 1)
    {
      if(y == JOYSTICK_MAX)
      {
        LED_right(250, 0, 0);
      }
      else
      {
        int color_val = map(y, 0, JOYSTICK_MAX, 0, 255);
        LED_right(0, color_val, color_val);
      }
    }
    else
    {
      LED_right(0, 0, 0);
    }
  
    // Back LEDs
    if(down == 1)
    {
      if(z == JOYSTICK_MAX)
      {
        LED_back(250, 0, 0);
      }
      else
      {
        int color_val = map(z, JOYSTICK_MIN, JOYSTICK_MAX, 0, 255);
        LED_back(0, color_val, color_val);
      }
    }
    else
    {
      LED_back(0, 0, 0);
    }

  }
  else
  {
    LED_front(0, 200, 0);
    LED_back(0, 200, 0);
    LED_left(0, 200, 0);
    LED_right(0, 200, 0);
    if(up == 1)     LED_front(250, 60, 0);
    if(down == 1)   LED_back(250, 60, 0);
    if(left == 1)   LED_left(250, 60, 0);
    if(right == 1)  LED_right(250, 60, 0);
  }
  
}

void setup() {
  // Setting up serial
  Serial.begin(115200);

  // Setting up pins
  pinMode(PIN_BTN1, INPUT);
  pinMode(PIN_BTN2, INPUT);
  pinMode(PIN_BTN3, INPUT);
  pinMode(PIN_BTN4, INPUT);
  pinMode(PIN_BTN5, INPUT);
  pinMode(PIN_BTN6, INPUT);
  pinMode(PIN_BTN7, INPUT);
  pinMode(PIN_BTN8, INPUT);
  pinMode(PIN_BTN9, INPUT);
  pinMode(PIN_BTN10, INPUT);
  pinMode(PIN_MUX1, INPUT);
  pinMode(PIN_MUX2, INPUT);
  pinMode(PIN_MUX_CTRL, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // Configuring the ADC
  analogReadResolution(12);

  // Configuring the LED strip
  FastLED.addLeds<WS2813, PIN_LED, RGB>(leds, NUM_LEDS);

  // Configuring the starting value for the multiplexer
  digitalWrite(PIN_MUX_CTRL, 0);

  // Configuring variables

  // Experimental mode
  last_up     = 0;
  last_down   = 0;
  last_left   = 0;
  last_right  = 0;

  // Rate limiter
  last_x      = 0;
  last_y1     = 0;
  last_y2     = 0;
  last_z      = 0;

  // Initializing the start time
  start_time  = millis();
}

void loop() {
  // Reading all of the inputs
  if(experimental_mode == false)
  {
    ReadInputs();
  }
  else
  {
    ReadInputsExperimental();
  }
  // Printing out the data if we're in the debug mode
  if(debug_mode == true) DebugPrint();
  // Sending data to the PC
  UpdateJoystick();
  // Updating the LEDs
  UpdateLEDs();
}
