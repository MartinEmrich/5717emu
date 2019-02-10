5717emu
=======

Emulates the mouse controller chip inside a Commodore 1351 mouse for the Commodore 64.

I made this because the chip inside my mouse was broken. There are several other projects like this out there, but I did not find any source code... (and doing it by myself was more fun).

At present, it does NOT work as smoothly as the original chip, but better than a dead mouse.


Connection
----------

| Arduino nano | original 5717 pin|
|--------------|------------------|
| A0  | 1| 
| A1  | 2| 
| A2  | 3| 
| A3  | 4| 
| D4  | 5| 
| D3  | 6| 
| --  | 7| 
| --  | 8| 
| GND | 9| 
| --  | 10| 
| --  | 11| 
| D10 | 12| 
| D9  | 13| 
| D8  | 14| 
| D7  | 15| 
| D6  | 16| 
| D5  | 17| 
| 5V  | 18| 

Theory of operation
-------------------
(For the full story, read the links below, especially Bo Zimmermans page and the original patent)

The C64 has no "real" mouse port, so the engineers had to "retrofit" it later. They used the two paddle potentiometer pins
to transmit the mouse position.

The mouse has an internal 6-bit counter for each axis, increasing or decreasing in relation to the movement. On over- or underflow, the counters wrap around.

Meanwhile, every 512 clock cycles, the SID starts a measurement cycle to read the paddle pots, by counting the time it takes to charge capacitors over the potentiometers. For the first 256 cycles, the pins are grounded to discarge the caps. Then the pins are pulled high, and the SID counts the cycles it takes to reach a threshold voltage.

The mouse makes use of this: When POT_Y goes low, it starts counting, until it reaches 256 cycles (or µS, as the mouse clock runs at 1MHz). Then it waits another 64 cycles. Now the magic happens. For each value in the counter, the mouse waits 2 µS, so after counter x 2µS, the pin is pulled high, instantly making the SID think "the capacitor is charged". After a total of 480µs
the POT pins are released again, ready for the next cycle.

(Fun fact: as the mouse clock (1MHz) slightly differs from the host clock (on both PAL and NTSC), the lowest bit of the paddle ports is just useless noise)

Arduino caveats
---------------

Two issues I see on the Arduino:

1. Measuring two analog values on the Arduino Nano (ATmega 328p) takes already ca. 240µS, so each axis is only updated every two trasmission cycles. This could be circumvented by programming the ATmega directly instead of using `analogRead()`, but the current solution works ok.
2. The interrupt trigger on the POT_Y pin seems to be a bit flaky, the mouse pointer jerks around ca. 1 pixel. But it is still usable to control GEOS. (Maybe the original mouse did this, too...? I forgot over all those years....)


Calibration
-----------

When you first run it with your mouse, you should "calibrate" it: The four photosensitive devices inside the mouse have different maximum voltages, I read somewhere that this is from ageing.
to calibrate, uncomment the `#define CALIBRATE` and run it while having the serial console open.
Right after starting, swirl around both axes for several (>10) seconds, to make sure the code sees the maximum value for each photodiode.

After a while, the four values are printed to the serial connection. Insert these into the code, comment out the calibration again and upload again. That should do it.

Open Issues
-----------

* No Joystick mode. I see no need to implement it at present. Also, the joystick direction pins have 5V and need to be pulled down to Ground.
* Right mouse button not supported.
* SYNC detection is still jittery, probably the 5717 did it differently from the ATmega. This results in the mouse values jerk around a bit.
* transition from tristate to high leaves a signal artifact.
* Work around the calibration. The original 5717 did without it, so we should, too.

Links & Credits
---------------

Read this for details about the protocol used. There's Assembler source code for another MCU, too.
ftp://www.zimmers.net/pub/cbm/documents/projects/interfaces/mouse/Mouse.html

The original patent for the mouse.
https://patents.google.com/patent/US4886941

A schematic of the mouse circuit.
https://www.commodore.ca/manuals/funet/cbm/schematics/misc/index.html

SID data sheet:
http://archive.6502.org/datasheets/mos_6581_sid.pdf

A interesting document is attached here.
http://cbm-hackers.2304266.n4.nabble.com/MOS-5717-td4660680.html

Another adapter:
http://sensi.org/~svo/[m]ouse/

