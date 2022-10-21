#include <8051.h>

#include "common.h"

#define BUZZ P1_6

unsigned long INTERRUPT_COUNT = 0;
unsigned char INTERRUPT_FLAG = 0;
unsigned short VALID_BUZZER_INTERVAL = 1000;
unsigned short DEAD_BUZZER_INTERVAL = 5;
unsigned long DIGIT = 30;

unsigned char LED_CHAR[] = {
    0xff ^ 0b111111,  0xff ^ 0b110,     0xff ^ 0b1011011, 0xff ^ 0b1001111,
    0xff ^ 0b1100110, 0xff ^ 0b1101101, 0xff ^ 0b1111101, 0xff ^ 0b111,
    0xff ^ 0b1111111, 0xff ^ 0b1101111,
};

unsigned char LED_BUFF[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

void enable_buzzer();

void switch_buzzer();

void interrupt_time0() __interrupt(1);

void turn_off_all_segs();

void enable_tube(unsigned char i);

void show_digit(unsigned char i);

void update_led_buffer(unsigned long digit);

void flush_led_buffer();

int main() {
  enable_u3_74hc138();
  enable_buzzer();

  update_led_buffer(DIGIT);
  flush_led_buffer();

  // enable Timer0
  EA = 1;   // enable global interrupt
  ET0 = 1;  // enable Timer0 interrupt
  TMOD = 0x01;
  TH0 = 0xFC;
  TL0 = 0x67;
  TR0 = 1;  // start Timer1

  while (1) {
    if (INTERRUPT_FLAG) {
      switch_buzzer();
      update_led_buffer(DIGIT);
      INTERRUPT_FLAG = 0;
    }
  }
}

void enable_buzzer() { BUZZ = 0; }

void switch_buzzer() { BUZZ ^= 0x01; }

void interrupt_time0() __interrupt(1) {
  // reset Timer0
  TH0 = 0xFC;
  TL0 = 0x67;

  INTERRUPT_COUNT++;

  if (INTERRUPT_COUNT >= VALID_BUZZER_INTERVAL) {  // 1 second
    INTERRUPT_COUNT = 0;
    INTERRUPT_FLAG = 1;
    if (DIGIT > 0) {
      DIGIT--;
    } else if (VALID_BUZZER_INTERVAL != DEAD_BUZZER_INTERVAL) {
      VALID_BUZZER_INTERVAL = DEAD_BUZZER_INTERVAL;
    }
  }
  flush_led_buffer();
}

void turn_off_all_segs() { P0 = 0xff; }

// i: 0 - (TUBE_SIZE-1)
void enable_tube(unsigned char i) {
  // P1_2 P1_1 P1_0
  // TUBE 0 000
  // TUBE 1 001
  // TUBE 2 010
  // TUBE 3 011
  // TUBE 4 100
  // TUBE 5 101
  P1 &= 1 << 3;
  P1 |= i;
}

void show_digit(unsigned char i) {
  // P0 = 0xff ^ digit_seg(i);
  // use array buffer to accelerate since the value is not changed in run-time.
  P0 = LED_CHAR[i];
}

unsigned int pow(unsigned int x, unsigned int y) {
  unsigned int res = 1;
  while (y--) {
    res *= x;
  }
  return res;
}

void update_led_buffer(unsigned long digit) {
  signed char i = 0;
  unsigned char buf[6];
  for (i = 0; i < 6; ++i) {
    buf[i] = digit % 10;
    digit /= 10;
  }

  for (i = 5; i >= 1; i--) {
    if (buf[i] == 0) {
      LED_BUFF[i] = 0xFF;
    } else {
      break;
    }
  }
  for (; i >= 0; i--) {
    LED_BUFF[i] = LED_CHAR[buf[i]];
  }
}

void flush_led_buffer() {
  turn_off_all_segs();
  static unsigned char TUBE_IDX = 0;
  switch (TUBE_IDX) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
      enable_tube(TUBE_IDX);
      P0 = LED_BUFF[TUBE_IDX];
      TUBE_IDX++;
      break;
    case 5:
      enable_tube(TUBE_IDX);
      P0 = LED_BUFF[TUBE_IDX];
      TUBE_IDX = 0;
      break;
    default:
      break;
  }
}
