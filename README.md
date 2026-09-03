# libavr32db28-p1
AVR32DB28 drivers for the P1 hardware development hat.

## About
A small, deliberately plain set of drivers for the AVR32DB28 microcontroller. The set covers every
peripheral on the P1 hardware development hat: the LEDs, the pushbuttons, the relay, the two digit
7-segment display, the two potentiometers, the temperature sensor and the joystick, along with the
timers a program needs to pace itself.

Students design the hat themselves in *Tillämpad elektronik och hållbar utveckling*, and then
learn to code on it in *Programmeringsmetodik*, where these drivers are what they build on.

The drivers therefore assume no previous experience with microcontrollers, and nothing more of C
than the course itself teaches. Everything a student needs is in the header files: each one
exposes a handful of ordinary C functions and no hardware detail at all. There are no registers,
no bit masks and no pointers to learn before switching on an LED. Registers and the rest of the
hardware detail come in the programming course after this one; until then the register work lives
in the `.c` files, commented throughout for anyone curious enough to open them.

That simplicity is a deliberate constraint rather than a limitation of the hardware:
* **No interrupts.** Everything is polled, so the flow of a program is the order of its statements.
* **No structs in the API.** The board is fixed, so there is exactly one of each peripheral and
  nothing to pass around. Where several instances genuinely exist, as with the timers, they are
  handed out as enumerated IDs rather than pointers.
* **Blocking where blocking is simpler.** Non-blocking variants are provided where they are
  genuinely useful, not everywhere.
* **No configuration a beginner doesn't need.** Alternate pin mappings and clock tuning are left out
  until a course exercise calls for them.

Experienced readers will find features missing on purpose. The
[AVR32DB28 data sheet](https://www.microchip.com/en-us/product/AVR32DB28) documents what the
hardware can do beyond what the drivers expose.

---

## Drivers

| Peripheral         | Header                                                     | Reads                        | Writes                          |
| ------------------ | ---------------------------------------------------------- | ---------------------------- | ------------------------------- |
| LED                | [include/driver/led.h](./include/driver/led.h)             | Whether an LED is lit        | On, off, inverted or dimmed     |
| Switch             | [include/driver/sw.h](./include/driver/sw.h)               | Whether a button is pressed  | -                               |
| Relay              | [include/driver/relay.h](./include/driver/relay.h)         | Whether the relay is closed  | Closed, open or inverted        |
| Display            | [include/driver/display.h](./include/driver/display.h)     | The number being shown       | A number from 0 to 99           |
| Potentiometer      | [include/driver/pot.h](./include/driver/pot.h)             | A knob, raw, in mV or in %   | -                               |
| Temperature sensor | [include/driver/temp.h](./include/driver/temp.h)           | Degrees Celsius              | -                               |
| Joystick           | [include/driver/joystick.h](./include/driver/joystick.h)   | A direction and its button   | -                               |
| Timer              | [include/driver/timer.h](./include/driver/timer.h)         | Whether a timeout has passed | Started, stopped or inverted    |
| ADC                | [include/driver/adc.h](./include/driver/adc.h)             | Any analog pin, raw or in mV | -                               |

The ADC driver sits underneath the potentiometers, the temperature sensor and the joystick. A
program can use it directly, but rarely needs to.

**[docs/drivers.md](./docs/drivers.md) describes every function in these headers**, together with
the hardware behind it and the handful of places where two drivers interact.

---

## Usage
Each driver is initialized once, and then used from the main loop. Lighting every LED while its
switch is held down is a complete program:

```c
#include "driver/led.h"
#include "driver/sw.h"

int main(void)
{
    led_init();
    sw_init();

    while (1)
    {
        led_write(LED_RED, sw_read(SW1));
        led_write(LED_GREEN, sw_read(SW2));
        led_write(LED_BLUE, sw_read(SW3));
    }
}
```

The LEDs are called `LED_RED`, `LED_GREEN` and `LED_BLUE`, and the switches `SW1` - `SW3`. Those
names are the only hardware a program needs to know about: `sw_read()` reports whether a switch is
pressed, `led_write()` switches an LED on or off, `led_toggle()` inverts one, and `led_read()`
reports whether it is lit.

`led_pwm()` dims a LED rather than switching it fully on. A timer holds the brightness, so one call
is enough and the LED stays dimmed until it is dimmed or switched again:

```c
// Run the red LED at 30 %.
led_pwm(LED_RED, 30U);

while (1)
{
    // Full brightness while SW1 is held down, 30 % otherwise.
    if (sw_read(SW1)) { led_pwm(LED_RED, 100U); }
    else { led_pwm(LED_RED, 30U); }
}
```

All three LEDs can be dimmed at the same time, which is how the RGB LED mixes a colour:

```c
led_pwm(LED_RED, 80U);
led_pwm(LED_GREEN, 40U);
led_pwm(LED_BLUE, 0U);
```

Reading a knob and dimming an LED with it is two lines, since both speak percent:

```c
pot_init();
led_init();

while (1)
{
    led_pwm(LED_RED, pot_read_percent(POT1));
}
```

The display is the one peripheral that needs the program's help. Its two digits share their
segments and are lit alternately, so `display_update()` has to be called every lap of the loop or
the digits never light up:

```c
display_init();
temp_init();

while (1)
{
    display_write((uint8_t)temp_read());
    display_update();
}
```

A program like this goes in `main.c`, which is built with `make build` and flashed as described
below. The `main.c` in this repository counts up and down on the display as the joystick is held,
and starts over when the joystick is pressed.

---

## Getting started in Microchip Studio
The quickest route to a running board is Microchip Studio, which needs no toolchain setup and no
command line at all. The `make` targets described further down are an alternative to this, not a
prerequisite.

### 1. Get the code
Either download it as a ZIP, which needs no Git:

1. Open the repository on GitHub and press `Code > Download ZIP`.
2. Right-click the downloaded file and choose `Extract All`.
3. Extract somewhere short and simple, such as `C:\avr`. Do not work inside the ZIP file itself;
   Windows shows its contents readily enough, but nothing can be built from in there.

GitHub names the extracted folder `libavr32db28-p1-main`, which is the same thing as
`libavr32db28-p1` and can be renamed if the `-main` bothers you.

Or clone it, if Git is installed:

```bash
git clone https://github.com/YRGO-electrical-engineering/libavr32db28-p1.git
```

Cloning is worth the trouble if you expect to pick up later fixes, since `git pull` then replaces
downloading the ZIP again. Either way is fine for building in Microchip Studio. The `libs/`
submodule is only needed for the [tests](#tests), which a ZIP download leaves out and Studio does
not build.

### 2. Open the project
Double-click `libavr32db28-p1.atsln` in the folder you just unpacked or cloned. Microchip Studio
opens with the whole project in the `Solution Explorer` on the right: `main.c` at the top, the
drivers under `include/` and `source/`, and the tests under `test/`.

Opening `main.c` on its own also shows the code, but Studio then has no project around it and the
build commands stay greyed out. Always start from the `.atsln`.

### 3. Build
Press `F7`, or `Build > Build Solution`. The `Output` window should end with `Build succeeded` and
report a program of a few kilobytes.

If it complains about an unknown device instead, the AVR-Dx device family pack is missing. Install
it under `Tools > Device Pack Manager`, search for `AVR-Dx_DFP`, then build again. The project is
set up for pack version 1.10.114; another version works just as well, it only means correcting the
two pack paths under `Project > Properties > Toolchain`.

### 4. Program the board
With an Atmel-ICE, or a Curiosity board with a programmer built in:

1. Connect the board and open `Tools > Device Programming`.
2. Pick your tool, set `Device` to `AVR32DB28` and `Interface` to `UPDI`, then press `Apply`.
3. Press `Read` under `Device signature` to confirm the board is answering.
4. Go to `Memories`, check that the `Flash` box points at `Debug\libavr32db28-p1.hex`, and press
   `Program`.

Selecting the same tool under `Project > Properties > Tool` lets you skip all of that afterwards and
simply press `Ctrl+Alt+F5` to build and program in one go.

With a plain serial UPDI cable, i.e. a USB-to-serial adapter wired to the UPDI pin, Microchip Studio
is of no use: it only talks to Microchip's own programmers. Use `make flash` instead, as described
under [Compilation](#compilation). It picks up the `Debug/` output Studio just produced, so nothing
has to be built twice.

### 5. What you should see
`main.c` counts on the 7-segment display: hold the joystick up to count towards 99, down to count
back towards 0, and press it to start over from zero. Once that works, the file is yours to change.
[Usage](#usage) shows the shape of a program and
[docs/drivers.md](./docs/drivers.md) describes every function available.

The `test/` folder is in the project so it can be read, but Studio does not build it. Those tests
run on the computer rather than on the board, and are covered under [Tests](#tests). Ignore them
entirely if you like; nothing on the board depends on them.

---

## Structure

```text
ci/                    Scripts for compilation, flashing, testing and code formatting
docs/                  Driver reference and toolchain setup, i.e. the long form of this README
include/               Driver headers, i.e. the API students use
libs/                  The yrgo::test framework, checked out as a git submodule
source/                Driver implementations and the mocked hardware platform
test/                  Unit tests, run on the host against the mocked hardware
main.c                 Application entry point
Makefile               Targets for building, flashing, testing and formatting
libavr32db28-p1.atsln  Microchip Studio solution, opening the project below
libavr32db28-p1.cproj  Microchip Studio project, building the firmware
```

`include/arch/` and `source/arch/` hold the hardware platform. It selects the real AVR registers
when building firmware, and a mocked set of registers in RAM when building the tests, so the same
driver code compiles for both. It also provides `delay_ms`, which busy-waits on the target.

---

## Toolchain
Building the firmware needs `avr-gcc` and the AVR-Dx device family pack (DFP). `avr-libc` does not
ship the device specs or the `<avr/io.h>` header for this part, so the pack supplies them.
Flashing adds `avrdude`, and the formatting check adds `clang-format`.

**[docs/toolchain.md](./docs/toolchain.md) installs all of it step by step**, on Windows in Git
Bash as well as on Linux, and lists what each `make` target actually uses. The short version, on
Linux or WSL:

```bash
sudo apt -y update
sudo apt -y install gcc-avr binutils-avr avr-libc avrdude
```

The device family pack is downloaded and unpacked once:

```bash
wget http://packs.download.atmel.com/Atmel.AVR-Dx_DFP.1.10.114.atpack
unzip -q -d dfp Atmel.AVR-Dx_DFP.1.10.114.atpack
```

The build looks for the pack in `dfp/` first, then in a local Microchip Studio installation, which
it finds by itself if Studio is installed. Set `DFP_DIR` to point at a pack somewhere else.

---

## Compilation
The root `Makefile` cross-compiles the firmware for the AVR32DB28:

```bash
make build
```

The result is written to `build/main.elf`, along with a flashable `build/main.hex`. Building and
flashing in one step:

```bash
make flash
```

This expects a serial UPDI adapter, i.e. a USB-to-serial cable wired to the UPDI pin. The port
defaults to `COM3` on Windows and `/dev/ttyUSB0` elsewhere, so pass another one if the board turns
up somewhere else:

```bash
make flash PORT=COM4
```

Which amounts to running avrdude directly:

```bash
avrdude -c serialupdi -p avr32db28 -P COM3 -b 115200 -U flash:w:build/main.hex:i
```

Programming through an Atmel-ICE or a Curiosity board is done from Microchip Studio instead, as
described under [Getting started in Microchip Studio](#getting-started-in-microchip-studio).

---

## Tests
The drivers are unit tested against a mocked AVR32DB28, so **the tests run on the host and no board
is needed**:

```bash
make test
```

The mock provides the peripheral registers as ordinary variables in RAM. A test calls a driver
function and then checks which register it wrote and with what value, which is how the tests catch a
driver reaching for the wrong register without anyone plugging in hardware. The platform records
delays rather than sleeping through them, so a driver that busy-waits would cost the suite no time
either.

The suite is built against the [yrgo::test](https://github.com/yrgo-libs/yrgo-test) framework, which
lives in `libs/test` as a git submodule. Check it out once before running the tests:

```bash
git submodule update --init
```

A C++17-capable compiler (e.g. `g++`) needs to be installed and available on `PATH`. The drivers
themselves are compiled as C; only the tests are C++.

---

## Code Formatting
The root `Makefile` formats all C/C++ code with `clang-format`:

```bash
make format        # Format all files.
make format-check  # Check formatting without modifying files.
```

`clang-format` needs to be installed and available on `PATH`. On Linux or WSL:

```bash
sudo apt -y update
sudo apt -y install clang-format
```

On Windows, and for why the version matters to the check, see
[docs/toolchain.md](./docs/toolchain.md#clang-format).

---

## Continuous Integration
Every push and pull request to `main` runs three jobs: a firmware cross-compile, the test suite, and
the formatting check. See [.github/workflows/ci.yml](./.github/workflows/ci.yml).

---
