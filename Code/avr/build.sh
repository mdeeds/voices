#!/bin/sh
set -e

avr-gcc -mmcu=atmega8 ledblink.c

avr-objcopy -j .text -j .data -O ihex  a.out ledblink.hex
