/* CSG 5717 emulator

    Emulates the CSG 5717 controller (inside a Commodore 1351 mouse)
    on an Arduino Nano

  (c) 2019 Martin Emrich <martin@martinemrich.me>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <avr/io.h>
#include <avr/interrupt.h>
//#include <math.h>

/* run calibration at startup */
//#define CALIBRATE

/* number of samples to take to calibrate quad sensors */
#define CALIBRATION_ROUNDS 1000

/* Calibration values (maximum voltage for each sensor) for YOUR mouse sensors */
#define MAX_X0 739
#define MAX_X1 656
#define MAX_Y0 546
#define MAX_Y1 973

/* Hysteresis: plus/minus this from the middle between low and high range for sensors */
#define QUAD_HYSTERESIS 20

/*
   Pin connections
*/

// Analog inputs: Quadrature Encoder wheels (photosensors)
#define QUAD_X0 A0
#define QUAD_X1 A1
#define QUAD_Y0 A2
#define QUAD_Y1 A3

#define RIGHT_BUTTON 4

//#define SCHMITT_TRIGGER_ON_2

#ifdef SCHMITT_TRIGGER_ON_2
// sync input (POT_Y from C64)
#define SYNC_INPUT 2
#define SYNC_MODE LOW
#else
#define SYNC_INPUT 3
#define SYNC_MODE LOW
#endif

// Joystick directions (Up is sent for Right button!)
#define JOY_UP 5
#define JOY_DOWN 6
#define JOY_RIGHT 7
#define JOY_LEFT 8

// Outputs
#define POT_X 9
#define POT_Y 10

// Quad raw (analog values)
uint16_t raw_x0, raw_x1, raw_y0, raw_y1;

uint8_t x0 = 0;
uint8_t x1 = 0;
uint8_t y0 = 0;
uint8_t y1 = 0;

uint8_t old_x = 0;
uint8_t old_y = 0;

volatile uint8_t x_pos = 0;
volatile uint8_t y_pos = 0;

//#define raw2bit(Q,R,MAX_R) { if (Q == 0) { if (R > (MAX_R/2+QUAD_HYSTERESIS)) Q = 1; } else { if (R < (MAX_R/2-QUAD_HYSTERESIS)) Q = 0; } }
#define raw2bit(Q,R,MAX_R) { if (R > (MAX_R/2+QUAD_HYSTERESIS)) Q = 1; if (R < (MAX_R/2-QUAD_HYSTERESIS)) Q = 0; }

/* Unused alternative approach: calculate phase angle of quad signal */
#define norm_x0 ((raw_x0 - (MAX_X0/2.0f))/(MAX_X0/2.0f))
#define norm_x1 ((raw_x1 - (MAX_X1/2.0f))/(MAX_X1/2.0f))
#define norm_y0 ((raw_y0 - (MAX_Y0/2.0f))/(MAX_Y0/2.0f))
#define norm_y1 ((raw_y1 - (MAX_Y1/2.0f))/(MAX_Y1/2.0f))
#define angle_y() (int16_t)(atanf(norm_y0/norm_y1)*100)
#define angle_x() (int16_t)(atanf(norm_x0/norm_x1)*100)

void update_pos_y()
{
  raw_y0 = analogRead(QUAD_Y0);
  raw_y1 = analogRead(QUAD_Y1);

  raw2bit(y0, raw_y0, MAX_Y0);
  raw2bit(y1, raw_y1, MAX_Y1);

  /* Gray code to binary */
  y0 = (y1 ? y0 : 1 - y0);

  /* to single two-bit-number */
  uint8_t y = y1 * 2 + y0;
  int8_t y_dir = 0;

  switch (old_y) {
    case 0b00: if (y == 0b01) y_dir = +1; else if (y == 0b11) y_dir = -1 ; break;
    case 0b01: if (y == 0b10) y_dir = +1; else if (y == 0b00) y_dir = -1 ; break;
    case 0b10: if (y == 0b11) y_dir = +1; else if (y == 0b01) y_dir = -1 ; break;
    case 0b11: if (y == 0b00) y_dir = +1; else if (y == 0b10) y_dir = -1 ; break;
    default: break;
  }

  // note the minus to avoid inverted-y-axis, it's not a flight simulator!
  y_pos = (y_pos - y_dir) & 0x3F;

  old_y = y;
}


void update_pos_x()
{
  raw_x0 = analogRead(QUAD_X0);
  raw_x1 = analogRead(QUAD_X1);
  raw2bit(x0, raw_x0, MAX_X0);
  raw2bit(x1, raw_x1, MAX_X1);

  /* Gray code to binary */
  x0 = (x1 ? x0 : 1 - x0);

  /* to single two-bit-number */
  uint8_t x = x1 * 2 + x0;
  int8_t x_dir = 0;

  switch (old_x) {
    case 0b00: if (x == 0b01) x_dir = +1; else if (x == 0b11) x_dir = -1 ; break;
    case 0b01: if (x == 0b10) x_dir = +1; else if (x == 0b00) x_dir = -1 ; break;
    case 0b10: if (x == 0b11) x_dir = +1; else if (x == 0b01) x_dir = -1 ; break;
    case 0b11: if (x == 0b00) x_dir = +1; else if (x == 0b10) x_dir = -1 ; break;
    default: break;
  }
  x_pos = (x_pos + x_dir) % 64;
  old_x = x;
}


#ifdef CALIBRATE
// Quad Maximums for calibration
uint16_t max_x0 = 0;
uint16_t max_x1 = 0;
uint16_t max_y0 = 0;
uint16_t max_y1 = 0;

void calibrate()
{
  Serial.print("Calibrating...Move around both axes NOW for a good result...\n");
  for (uint16_t counter = 0; counter < CALIBRATION_ROUNDS; counter++)
  {
    update_pos_x();
    update_pos_y();

    if (raw_x0 > max_x0) max_x0 = raw_x0;
    if (raw_x1 > max_x1) max_x1 = raw_x1;
    if (raw_y0 > max_y0) max_y0 = raw_y0;
    if (raw_y1 > max_y1) max_y1 = raw_y1;
    delay(30);
  }
  char quad_nums[64];
  snprintf(quad_nums, 64, "Calibrated: Max: X0 %04u, X1 %04u, Y0 %04u, Y1 %04u\n", max_x0, max_x1, max_y0, max_y1);
  Serial.print(quad_nums);
  delay(5000);
}
#endif // CALIBRATE

uint8_t current_dir = 0;

/* Update one alternating direction, takes ca. 240uS */
void update_one_direction()
{
  if (current_dir == 0) {
    update_pos_x();
    current_dir = 1;
  } else {
    update_pos_y();
    current_dir = 0;
  }

}

/* start high-precision timer at 0 "jiffies" (0.5µS) */
inline void hpTimerStart()
{
  TCCR1A = 0;
  TCCR1B = 0;

  TCNT1 = 0; // reset counter
  TIMSK1 = 0; // no interrupts, no compare stuff.

  TCCR1B |=  (0 << CS12) | (1 << CS11) | (0 >> CS10); // Prescaler: 8 (1count == 0.5µS), start clock
}

/* wait for timer to pass us Microseconds */
inline void hpTimerWaituS(uint16_t us)
{
  while (TCNT1 < (us * 2)) {
    asm volatile ("NOP");
  }
}

/* Delay-encoded value in Host cycles */
uint16_t x_delay;
uint16_t y_delay;

#define triStatePin(PIN) pinMode(PIN, INPUT);
#define pullUpPin(PIN) digitalWrite(PIN, HIGH);pinMode(PIN, OUTPUT);

/* One cycle of

 *  * Measuring one axis
 *  * Transmitting both axes to C64
*/
void measureTransmitCycle() {
  uint16_t t = TCNT1;
  hpTimerStart();

  // pinMode(SYNC_INPUT, INPUT);
  // Diagnose: LED is on while the interrupt procedure runs
  digitalWrite(LED_BUILTIN, HIGH);

  // Toggle pin 12 if the interrupt pin missed a sync event
  if (t > 1060) {
    // we skipped a beat!
    digitalWrite(12, ! digitalRead(12) );
  }


  /* tristate pins */
 // triStatePin(POT_X);
 // triStatePin(POT_Y);

  update_one_direction(); // takes approx. 240us


  /* Position to delay (2uS per value) */
  x_delay = (x_pos * 2);
  y_delay = (y_pos * 2);

  /* wait 256µS (half of the SID measuring cycle) plus another 64 (ca. 65µS) */
#define TX_START_OFFSET 320

  /* wait until 480µS elapsed before tristating the POT pins again */
#define TX_END_OFFSET 480

  // Start 5µs "early", because switching from tristate to high takes some time
  uint16_t xtime = (TX_START_OFFSET + x_delay - 5 ) * 2;
  uint16_t ytime = (TX_START_OFFSET + y_delay - 5 ) * 2;
  uint8_t xset = 0;
  uint8_t yset = 0;
  while (TCNT1 <= TX_END_OFFSET * 2) {
    if (!xset && TCNT1 >= xtime) {
      pullUpPin(POT_X);
      xset = 1;
    }
    if (!yset && TCNT1 >= ytime) {
      pullUpPin(POT_Y);
      yset = 1;
    }
  }
  /* tristate pins */
  triStatePin(POT_X);
  triStatePin(POT_Y);

  digitalWrite(LED_BUILTIN, LOW);
}

void setup() {
  // Inputs
  pinMode(QUAD_X0, INPUT);
  pinMode(QUAD_X1, INPUT);
  pinMode(QUAD_Y0, INPUT);
  pinMode(QUAD_Y1, INPUT);
  pinMode(RIGHT_BUTTON, INPUT);

  pinMode(2, INPUT);
  pinMode(3, INPUT);

  pinMode(SYNC_INPUT, INPUT);

  /* Caution: they are pulled high inside CIA, and are intended to be shorted to ground to trigger.
   *  I guess it's a bad idea to pull them high here (see https://www.c64-wiki.de/wiki/Controlport)
   *  
   *  We leave them as input for now.
   */
  pinMode(JOY_UP, INPUT);
  pinMode(JOY_DOWN, INPUT);
  pinMode(JOY_RIGHT, INPUT);
  pinMode(JOY_LEFT, INPUT);

  // Outputs
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);

  triStatePin( POT_X);
  triStatePin( POT_Y);

#ifdef CALIBRATE
  Serial.begin(115200);
  calibrate();
#endif

  attachInterrupt(digitalPinToInterrupt(SYNC_INPUT), measureTransmitCycle, SYNC_MODE);
}


void loop()
{
  /* do nothing here, all work is done in the interrupt routine */
  while (true) {
    asm ("NOP");
    /* if (TCNT1 > 1040) {
       digitalWrite(12, ! digitalRead(12) );
       noInterrupts();
       measureTransmitCycle();
       interrupts();
      } */
  }
}
