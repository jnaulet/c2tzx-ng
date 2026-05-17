# c2tzx-ng

## Presentation

This is a better version of c2tzx for Linux & other UNIX-based OSes, re-written
from scratch.

c2tzx-ng converts a z80 binary code into a a Sinclair Spectrum TZX tape image.
This means you can develop you own software using SDCC, for example.

You can find the original inspiration software here: https://github.com/verfum/c2tzx   
And here: https://github.com/vext01/c2tzx4u

The code is full of ```(void)```s, ```TODO```s and ```FIXME```s, so fell free to
improve on that if you need to.

## Requirements

To build c2tzx-ng:

  - gcc/clang
  - make

To cross compile C code into a tzx using c2tzx-ng:

  - SDCC: http://sdcc.sourceforge.net
  - objcopy (should come with your binutils install)

To test:

  - An emulator. Try fuse: http://fuse-emulator.sourceforge.net

## License

The code is derived from c2tzx4u, which is derived from c2tzx for windows.   
This project can be found here: https://code.google.com/p/c2tzx/

Apparently this is GPLv2 code, despite the fact it's a complete rewrite,
we will roll with that, too.
