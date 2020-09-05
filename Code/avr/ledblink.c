// https://tldp.org/HOWTO/Avr-Microcontrollers-in-Linux-Howto/x207.html

/* ledblink.c, an LED blinking program */
#include<avr/io.h>
// #include<util/delay.h>
void sleep(uint8_t millisec) {
  // TODO: implement this.
}

void main() {
  DDRC |=1<<PC2;  /* PC2 will now be the output pin */
  while(1) {
    PORTC &= ~(1<<PC2);/* PC2 LOW */
    sleep(100);/* 100 ms delay */
    
    PORTC |=(1<<PC2); /* PC2 HIGH */
    sleep(100);/* 100 ms delay */
  }
}
