# P1 drivers: what's left to write

## Things to check on the board

None of these is visible in the schematic, and all are a minute's work with the board powered up:

* **Which digit is CC_1?** I've assumed PA5 (CC_1, pin 9) drives the left digit and PA6 (CC_2,
  pin 6) the right one. If it's the other way round, `display_write(12)` shows `21`.
* **Is the joystick's button really active low?** Assumed yes, and the driver now enables the
  internal pull-up on PD7 and reads a press as low. If the module turns out to drive the pin high
  instead, `joystick_pressed()` and the pull-up in `joystick_init()` both flip.
* **Which way is up?** The driver reads a rising voltage on PD5 as up and on PD6 as right. Both
  are one sign change in `joystick_read()` if the module is oriented the other way.

---

## Conventions I'm following

* No interrupts, no structs in the API, enums for IDs.
* Every function tolerates an invalid ID: it returns false or does nothing. The tests check it.
* Register work stays in the `.c` files, commented for anyone who opens them.
* The headers carry the Doxygen comments, since they're what a student reads.

---

## Pin map, traced from the schematic

| Peripheral | Pins | Notes |
| ---------- | ---- | ----- |
| LEDs, red / green / blue | PC0, PC1, PC2 | Active high. The RGB LED shares these pins; DIP switch SW4 selects which one is grounded |
| SW1 | PC3 | Pressed reads **high**. External pull-down, plus a capacitor for debouncing |
| SW2, SW3 | PF0, PF1 | Pressed reads **high**. External pull-downs, no debouncing |
| 7-segment, BCD input | PA0 - PA3 | PA0 is the least significant bit. Digits 0 - 9 only; 10 - 15 blank the digit |
| 7-segment, decimal point | PA4 | One shared point, lit alongside whichever digit is on |
| 7-segment, digit select | PA5 (CC_1), PA6 (CC_2) | Active **low**, one digit at a time |
| Relay | PA7 | Active high. Also lights the yellow indicator LED |
| Potentiometer 1, 2 | PD1, PD3 | ADC0 channels AIN1 and AIN3 |
| Temperature sensor | PD4 | ADC0 channel AIN4. TMP36: 10 mV per degree, 500 mV at 0 °C |
| Joystick, vertical / horizontal | PD5, PD6 | ADC0 channels AIN5 and AIN6 |
| Joystick, button | PD7 | Digital input, internal pull-up, pressed reads **low** |
