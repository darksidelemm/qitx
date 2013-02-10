# QITX - A HF Data Transmitter

This repo contains software to control an Arduino Leonardo + AD9834 DDS based HF data transmitter. It implements the following modulation schemes:
- BPSK 31/63/125/250
- 8N1 RTTY, with custom baud rate and shift
- Morse Code
- CCIR 493-4 HF SELCALL (compatible with Barrett and Codan commercial HF radios)

All code is subject to modification, and I'll be making lots of those! I'll also eventually upload the hardware design files.

## Other Notes
- The AD9834 lib I've included talks to a port directly (PORTB, pin 7), so I can do the SPI signalling as fast as possible. This will have to be changed to use it with a different pin.

- I'm using the square wave output of the AD9834 as an input to a Class-D amplifier, for power efficiency reasons. The DAC is enabled too, so that can still be used.

- The BPSK modulator doesn't do pulse shaping, as the DDS doesn't support it. Instead, I'm using phase shaping. Since this doesn't require a linear amp, we can use highly efficient Class-D or E amplifiers. Currently the spectral occupancy is nowhere near as good as pulse-shaped BPSK, but I'm working on optimizing the bandwidth using a carefully shaped phase transition.
