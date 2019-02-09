5717emu
=======

Emulates the mouse controller chip inside a Commodore 1351 mouse for the Commodore 64.

I made this because the chip inside my mouse was broken. There are several other projects like this out there, but I did not find any source code... (and doing it by myself is more fun).

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

Calibration
-----------

When you first run it with your mouse, you should "calibrate" it: The four photosensitive devices inside the mouse have different maximum voltages, I read somewhere that this is from ageing.
to calibrate, uncomment the `#define CALIBRATE` and run it while having the serial console open.
Right after starting, swirl around both axes for several (>10) seconds, to make sure the code sees the maximum value for each photodiode.

After a while, the four values are printed to the serial connection. Insert these into the code, comment out the calibration again and upload again. That should do it.

Open Issues
-----------

* No Joystick mode. I see no need to implement it at present.
* Right mouse button not supported. Should be easy to implement.
* SYNC detection is still jittery, probably the 5717 did it differently from the ATmega. This results in the mouse values jerk around a bit.
* transition from tristate to high leaves a signal artifact.
* Work around the calibration. The original 5717 did without it, so we should, too.
* Y signal looks different from X signal, surely due to the connection to SYNC. Original 5717 apparently did this differently.

Links & Credits
---------------

Read this for details about the protocol used. There's Assembler source code for another MCU, too.
ftp://www.zimmers.net/pub/cbm/documents/projects/interfaces/mouse/Mouse.html

A schematic of the mouse circuit.
https://www.commodore.ca/manuals/funet/cbm/schematics/misc/index.html

SID data sheet:
http://archive.6502.org/datasheets/mos_6581_sid.pdf

A interesting document is attached here.
http://cbm-hackers.2304266.n4.nabble.com/MOS-5717-td4660680.html


