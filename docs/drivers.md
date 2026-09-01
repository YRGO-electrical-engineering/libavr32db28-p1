# Driver reference
Every function the P1 drivers expose, what the hardware does when you call it, and the few places
where one driver's behaviour depends on another's. The headers in `include/driver/` carry a short
summary of each function; this document is the long version.

---

## Conventions
The drivers all follow the same shape, so learning one teaches the rest:
* **Initialize once, then use.** Every driver has an `x_init()` that takes no arguments and prepares
  the whole peripheral. Calling it twice is harmless.
* **IDs are enumerations.** `LED_RED`, `SW1`, `POT2`, `TIMER_ID_0`. The compiler catches a wrong
  name, and there is nothing to look up.
* **An unknown ID does nothing.** Functions that return a value return zero or false for one;
  functions that act do nothing at all. A program never crashes because it passed the wrong ID.
* **Reads report what the driver last did**, not what a voltmeter would measure on the pin. This
  matters only for `led_read()` and `relay_read()`.
* **Nothing uses interrupts.** Every driver is polled, so a program does exactly what its statements say, in order.

---

## The hardware

| Peripheral                        | Pins                     | Notes                                                            |
| --------------------------------- | ------------------------ | ---------------------------------------------------------------- |
| LEDs, red, green and blue         | PC0, PC1, PC2            | Active high. The RGB LED shares these pins; DIP switch SW4 picks which of the two is grounded |
| Pushbutton SW1                    | PC3                      | Pressed reads **high**. External pull-down, plus a 100 nF capacitor for debouncing |
| Pushbuttons SW2 and SW3           | PF0, PF1                 | Pressed reads **high**. External pull-downs, no debouncing        |
| 7-segment display, BCD input      | PA0 - PA3                | Into a 74HC4511 decoder. PA0 is the least significant bit         |
| 7-segment display, decimal point  | PA4                      | One point shared by both digits                                   |
| 7-segment display, digit select   | PA5, PA6                 | Active **low**, one digit at a time                               |
| Relay                             | PA7                      | Active high, through a 2N7000. Also lights the yellow LED         |
| Potentiometers                    | PD1, PD3                 | 10 kohm, analog inputs AIN1 and AIN3                              |
| Temperature sensor                | PD4                      | TMP36, analog input AIN4                                          |
| Joystick, vertical and horizontal | PD5, PD6                 | Analog inputs AIN5 and AIN6                                       |
| Joystick, button                  | PD7                      | Digital input, pull-up enabled, pressed reads **low**             |

The device runs at 5 V on its internal 4 MHz oscillator. No external crystal is fitted, and none is
needed for anything the drivers do.

---

## Where the drivers meet
Four things are worth knowing before writing a program that uses more than one driver.

**The display needs calling.** Its two digits share the same seven segment lines, so only one can
be lit at a time. `display_update()` lights whichever digit's turn it is, and the digits alternate
every 5 ms. Forget to call it and the display simply stays dark, with no other symptom.

**There are three timers, and the display takes one.** `timer_init()` hands out one of the
AVR32DB28's three TCB circuits and returns `TIMER_ID_NONE` when they are gone. `display_init()`
reserves one for itself, so a program that also uses the display has two left. This is why
`display_init()` returns a `bool`.

**The LEDs own TCA0.** Dimming is done by the timer rather than in software, which is what lets all
three LEDs be dimmed at once. It costs nothing from the pool above, since TCA0 is a different timer
from the three TCBs, but it does mean TCA0 is unavailable for anything else.

**Blocking delays hold up the display.** `delay_ms()`, declared in
`include/arch/avr/hw_platform.h`, busy-waits, and nothing else happens meanwhile. 
A `delay_ms(500U)` in a loop that also refreshes the display gives half a second of dark digits. `timer_elapsed()` is the non-blocking way to wait.

---

## LED
Three LEDs on PC0 - PC2, red, green and blue, each through a series resistor to ground. The RGB LED
D4 sits on the same three pins, and DIP switch SW4 selects which of the two banks has its cathode
grounded, so a program drives both identically.

Dimming is done by timer TCA0, whose outputs are routed to port C. The counter tops out at 99 and
runs at 2.5 kHz, which is far above what the eye can see, and it means the compare value is the
percentage itself.

```c
void led_init(void);
```
Configures all three LEDs as outputs and starts the PWM timer. The LEDs are left off, and driven by
the output register rather than the timer until `led_pwm()` is called.

```c
bool led_read(led_id_t led);
```
Returns whether the LED is lit. A dimmed LED counts as lit whenever its duty cycle is above zero,
so an LED at 1 % reads as on. An unknown ID reads as false.

```c
void led_write(led_id_t led, bool state);
```

Switches the LED fully on or fully off. If the LED was dimmed, this takes its pin back from the
timer, so the brightness set earlier is forgotten.

```c
void led_toggle(led_id_t led);
```

Inverts the LED. Like `led_write()`, this takes the pin back from the timer if the LED was dimmed.

```c
void led_pwm(led_id_t led, uint8_t percent);
```

Dims the LED to the given percentage, from 0 (off) to 100 (fully on). The timer holds that
brightness until the LED is dimmed or switched again, so one call is enough, and all three LEDs can
be dimmed at the same time — which is how the RGB LED mixes a colour. A percentage above 100 is
ignored, leaving the LED as it was.

---

## Switch
Three pushbuttons: SW1 on PC3, SW2 on PF0 and SW3 on PF1. Each one connects its pin to +5 V when
pressed, with a 10 kohm resistor to ground holding the pin low while it is released.

**Pressed reads high**, which is the opposite of the more common arrangement, and the reason the
internal pull-ups are deliberately switched off: one would fight the external resistor and leave the pin stuck between high and low.

SW1 has a 100 nF capacitor across its resistor, which absorbs the contact bounce in hardware. SW2
and SW3 do not, so a program that counts presses on those will occasionally count one press twice.

```c
void sw_init(void);
```

Configures all three switches as inputs and makes sure their internal pull-ups are off.

```c
bool sw_read(switch_id_t sw);
```

Returns whether the switch is pressed at this instant. Nothing is remembered between calls, so
detecting the moment of a press means comparing against what the previous lap read. An unknown ID
reads as false.

---

## Relay
A normally open relay on PA7, driven through a 2N7000 transistor. Closing it also lights the yellow
LED next to it, so the state is visible without a meter. What the contact switches is wired to the
screw terminal J6.

A 100 kohm resistor at the transistor's gate holds the relay open while PA7 is still an input, so
nothing switches during the moment between reset and `relay_init()`.

```c
void relay_init(void);
```

Configures the relay pin as an output, leaving the relay open. The display shares this I/O port, so
the other pins are left untouched.

```c
bool relay_read(void);
```

Returns whether the relay is closed, i.e. what the driver was last asked for.

```c
void relay_write(bool state);
```

Closes the relay when true and opens it when false.

```c
void relay_toggle(void);
```

Inverts the relay.

---

## Display
Two digits, driven through a 74HC4511 decoder. A program hands the decoder a number from 0 to 9 and
it lights the right segments, which is why **only digits can be shown** — there are no letters, and
no way to light a single segment. Feeding it anything above 9 blanks the digit, which is how
blanking and the suppressed leading zero work.

Both digits share the same seven segment lines and are selected one at a time, so the driver lights
them alternately, 5 ms each. The pair is redrawn 100 times a second, fast enough to look steady.

```c
bool display_init(void);
```

Configures the display's pins, blanks it and reserves a timer to alternate the digits with.
Returns false if all three timers were already taken, in which case the display stays dark; see
[Where the drivers meet](#where-the-drivers-meet).

```c
void display_write(uint8_t value);
```

Shows a value from 0 to 99. A value above 99 is ignored and the display keeps showing what it had.
A value below 10 is shown without a leading zero, unless the decimal point is on, where the zero is
kept so that 0.7 doesn't read as .7.

```c
void display_clear(void);
```

Blanks both digits. The value is remembered, so writing a new one is not necessary to show
something again. Takes effect on the next update, within 5 ms.

```c
void display_show_dp(bool show);
```

Shows or hides the decimal point, which sits between the two digits. With it on, 37 reads as 3.7.

```c
void display_update(void);
```

Lights whichever digit's turn it is. **Call this every lap of the main loop.** It returns
immediately when the current digit's 5 ms are not yet up, so calling it often costs almost nothing,
and calling it rarely makes the display flicker or stay dark.

---

## Potentiometer
Two 10 kohm knobs, RV1 on PD1 and RV2 on PD3, each wired across the supply with its wiper to the
pin. Turned fully one way the pin sits at ground, fully the other at 5 V.

```c
void pot_init(void);
```

Prepares the ADC. A program reading a knob needs nothing else.

```c
uint16_t pot_read(pot_id_t pot);
```

Returns the knob's position as a 12-bit value, from 0 at 0 V to 4095 at 5 V.

```c
uint16_t pot_read_mv(pot_id_t pot);
```

Returns the voltage at the wiper in millivolts. A fully turned knob gives 4998 rather than 5000,
since the conversion divides the range into 4096 steps and counts from zero.

```c
uint8_t pot_read_percent(pot_id_t pot);
```

Returns the position as a percentage, from 0 to 100. This is the one to reach for when driving
`led_pwm()`, which takes the same scale.

---

## Temperature sensor
A TMP36 on PD4. It outputs 500 mV at 0 degrees and 10 mV more for every degree above that, so the
whole range the sensor covers, -40 to +125 degrees, fits between 100 mV and 1750 mV.

```c
void temp_init(void);
```

Prepares the ADC. A program reading the temperature needs nothing else.

```c
int16_t temp_read(void);
```

Returns the temperature in whole degrees Celsius, rounded to the nearest, and negative below zero.

A conversion that fails reads as 0 mV, which on this sensor's scale is **-50 degrees**. That is
outside what the part can measure, so it can be told apart from a real reading, but it does not look like an error at a glance.

---

## Joystick
An analog thumb joystick on the J5 connector: the vertical axis on PD5, the horizontal on PD6, and
a button on PD7. Each axis is a potentiometer that rests near the middle of its range, so the
driver treats a reading near the centre as no movement at all.

The button grounds its pin, so **pressed reads low** — the opposite of the pushbuttons on the
board — and the driver enables the internal pull-up to hold the pin high while it is released.

```c
void joystick_init(void);
```

Prepares the ADC for both axes and the button's pin as a pulled-up input.

```c
joystick_dir_t joystick_read(void);
```

Returns the direction the joystick is being held: `JOYSTICK_CENTER`, `JOYSTICK_UP`,
`JOYSTICK_DOWN`, `JOYSTICK_LEFT` or `JOYSTICK_RIGHT`.

An axis has to be pushed about half way to its limit before it counts, which keeps a joystick
resting slightly off centre from reporting a direction constantly. Held diagonally, whichever axis has moved furthest wins, so there are four directions rather than eight.

```c
bool joystick_pressed(void);
```

Returns whether the joystick's button is pressed at this instant.

---

## Timer
Three timers, counting in milliseconds. A program reserves one, starts it, and then asks whether it
has elapsed. They exist so that a program can wait for something without `delay_ms()` stopping
everything else, which matters most for the display.

The timers are polled rather than interrupt-driven, which has one consequence worth understanding:
**a timer only advances while the program is asking about one**. `timer_elapsed()` advances every
reserved timer, not only the one being asked about, so calling it on any timer every lap keeps them
all honest. Slow code makes every timer run slow.

```c
timer_id_t timer_init(uint16_t timeout_ms);
```

Reserves a timer with the given timeout and returns its ID, or `TIMER_ID_NONE` if all three are
taken or the timeout is zero. The timer is stopped; call `timer_start()` to start it.

```c
void timer_deinit(timer_id_t id);
```

Stops the timer and releases it, so that another part of the program can reserve it.

```c
void timer_start(timer_id_t id);
```

Starts the timer, discarding whatever time it had counted.

```c
void timer_stop(timer_id_t id);
```

Stops the timer. A stopped timer never elapses.

```c
void timer_toggle(timer_id_t id);
```

Starts the timer if it is stopped, stops it if it is running.

```c
bool timer_running(timer_id_t id);
```

Returns whether the timer is running.

```c
bool timer_elapsed(timer_id_t id);
```

Returns true once the timeout has passed, and starts counting again from zero, so a running timer
elapses once per timeout. A stopped or unreserved timer never elapses.

---

## ADC
The analog to digital converter underneath the potentiometers, the temperature sensor and the
joystick. A program can use it directly to read any of those pins, though the drivers above give
the same readings in units that mean something.

It measures against the supply, so 4095 means 5 V, and it samples for longer than the device's
default because the 10 kohm potentiometers charge its sample capacitor slowly.

```c
void adc_init(void);
```

Enables the converter and switches off the digital input buffers on the five analog pins, which
stops a voltage half way between high and low from leaving a buffer switching back and forth.

```c
uint16_t adc_read(adc_channel_t channel);
```

Converts the channel and returns a 12-bit value, from 0 at 0 V to 4095 at the top of the range.
The range is divided into 4096 steps counting from zero, so the largest reading is one step short
of the supply rather than exactly at it. The channels are `ADC_POT1`, `ADC_POT2`, `ADC_TEMP`,
`ADC_JOYSTICK_Y` and `ADC_JOYSTICK_X`.

The call blocks until the conversion finishes, which takes a few tens of microseconds: the
converter is clocked at 1 MHz and samples for longer than its default. If the conversion never
finishes, the driver gives up and returns 0 rather than waiting forever — so a reading of zero is
either 0 V or a converter that isn't answering.

```c
uint16_t adc_read_mv(adc_channel_t channel);
```

The same reading, scaled to millivolts.

---
