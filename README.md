5717emu
=======

Emulates the mouse controller chip inside a Commodore 1351 mouse for the Commodore 64.

I made this because the chip inside my mouse was broken. There are several other projects like this out there, but I did not find any source code... (and doing it by myself is more fun).


Connection
----------

Arduino nano | original 5717 pin
A0  | 1
A1  | 2
A2  | 3
A3  | 4
D4  | 5
D3  | 6
--  | 7
--  | 8
GND | 9
--  | 10
--  | 11
D10 | 12
D9  | 13
D8  | 14
D7  | 15
D6  | 16
D5  | 17
5V  | 18

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


