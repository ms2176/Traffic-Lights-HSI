// Blinking LED, now really standalone; LED controlled from C level
// Compile: gcc  -o  t1 tut_led.c
// Run:     sudo ./t1

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

// =======================================================
// Tunables
// The OK/Act LED is connected to BCM_GPIO pin 47 (RPi 2)
#define RED 10
#define YELLOW 11
#define GREEN 13
#define BUTTON 26
// delay for blinking
#define DELAY 700
// =======================================================

#ifndef TRUE
#define TRUE (1 == 1)
#define FALSE (1 == 2)
#endif

#define PAGE_SIZE (4 * 1024)
#define BLOCK_SIZE (4 * 1024)

#define INPUT 0
#define OUTPUT 1

#define LOW 0
#define HIGH 1

static volatile unsigned int gpiobase;
static volatile uint32_t *gpio;

// -----------------------------------------------------------------------------

int failure(int fatal, const char *message, ...)
{
  va_list argp;
  char buffer[1024];

  if (!fatal) //  && wiringPiReturnCodes)
    return -1;

  va_start(argp, message);
  vsnprintf(buffer, 1023, message, argp);
  va_end(argp);

  fprintf(stderr, "%s", buffer);
  exit(EXIT_FAILURE);

  return 0;
}

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* HaWo: tinkering starts here */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

int main(void)
{
  int pinRED = RED, pinYELLOW = YELLOW, pinGREEN = GREEN, pinBUTTON = BUTTON; //
  int fSel, shift, pin, clrOff, setOff;
  int fd;
  int j;
  int theValue, thePin;
  unsigned int howLong = DELAY;
  uint32_t res; /* testing only */

  printf("Raspberry Pi blinking LED %d %d %d and button %d\n", pinRED, pinYELLOW, pinGREEN, pinBUTTON);

  if (geteuid() != 0)
    fprintf(stderr, "setup: Must be root. (Did you forget sudo?)\n");

  // -----------------------------------------------------------------------------
  // constants for RPi2
  gpiobase = 0x3F200000;

  // -----------------------------------------------------------------------------
  // memory mapping
  // Open the master /dev/memory device

  if ((fd = open("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC)) < 0)
    return failure(FALSE, "setup: Unable to open /dev/mem: %s\n", strerror(errno));

  // GPIO:
  gpio = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, gpiobase);
  if ((int32_t)gpio == -1)
    return failure(FALSE, "setup: mmap (GPIO) failed: %s\n", strerror(errno));
  else
    fprintf(stderr, "NB: gpio = %x for gpiobase %x\n", gpio, gpiobase);

  // -----------------------------------------------------------------------------
  // setting the mode

  fprintf(stderr, "setting pin %d to %d ...\n", pinRED, OUTPUT); // GPIO 10 sits in slot 0 of register 1, thus shift by 0*3 (3 bits per pin)
  //*(gpio + 1) = (*(gpio + 1) & ~(7 << 0)) | (1 << 0) ;  // Sets bits to one = output
  *(gpio + 1) = (*(gpio + 1) & ~(0b111)) | 0b1; // Sets bits to one = output

  // fprintf(stderr, "Red: %u\n", (*(gpio + 1) & ~(7 << 0)) | (1 << 0));

  fprintf(stderr, "setting pin %d to %d ...\n", pinYELLOW, OUTPUT); // GPIO 11 sits in slot 1 of register 1, thus shift by 1*3 (3 bits per pin)
  //*(gpio + 1) = (*(gpio + 1) & ~(7 << 3)) | (1 << 3) ;  // Sets bits to one = output
  *(gpio + 1) = (*(gpio + 1) & ~(0b111000)) | 0b1000; // Sets bits to one = output

  // fprintf(stderr, "Yellow: %u\n", (*(gpio + 1) & ~(7 << 3)) | (1 << 3));

  fprintf(stderr, "setting pin %d to %d ...\n", pinGREEN, OUTPUT); // GPIO 13 sits in slot 3 of register 1, thus shift by 3*3 (3 bits per pin)
  //*(gpio + 1) = (*(gpio + 1) & ~(7 << 9)) | (1 << 9) ;  // Sets bits to one = output

  *(gpio + 1) = (*(gpio + 1) & ~(0b111000000000)) | 0b1000000000; // Sets bits to one = output
  // fprintf(stderr, "Green: %u\n", (*(gpio + 1) & ~(7 << 9)) | (1 << 9));

  fprintf(stderr, "setting pin %d to %d ...\n", pinBUTTON, INPUT); // GPIO 26 sits in slot 6 of register 2, thus shift by 6*3 (3 bits per pin)
  //*(gpio + 2) = (*(gpio + 2) & ~(7 << 18)) | (1 << 18) ;  // Sets bits to one = output
  *(gpio + 2) = 0b1000000000000000000; // Sets bits to one = input
  // fprintf(stderr, "Button: %u\n", (*(gpio + 2) & ~(7 << 18)) | (1 << 18));

  // -----------------------------------------------------------------------------

  // now, start a loop, listening to pinButton and if set pressed, set pinLED
  fprintf(stderr, "starting loop ...\n");

  for (j = 0; j < 1000; j++)
  {

    // Turn on RED
    //*(gpio + 7) = 1 << (pinRED & 31);

    if ((pinRED & 0xFFFFFFC0 /* PI_GPIO_MASK */) == 0) // bottom 64 pins belong to the Pi
    {
      int off = (theValue == LOW) ? 11 : 7; // ACT/LED 47; register number for GPSET/GPCLR
      *(gpio + off) = 1 << (pinRED & 31);
    }
    else
    {
      fprintf(stderr, "only supporting on-board pins\n");
    }

    // INLINED delay
    {
      struct timespec sleeper, dummy;
      sleeper.tv_sec = (time_t)(howLong / 1000);
      sleeper.tv_nsec = (long)(howLong % 1000) * 1000000;
      nanosleep(&sleeper, &dummy);
    }

    // Turn on RED and YELLOW
    //*(gpio + 7) = 1 << (pinYELLOW & 31);

    if ((pinYELLOW & 0xFFFFFFC0 /* PI_GPIO_MASK */) == 0) // bottom 64 pins belong to the Pi
    {
      int off = (theValue == LOW) ? 11 : 7; // ACT/LED 47; register number for GPSET/GPCLR
      *(gpio + 7) = 1 << (pinYELLOW & 31);
    }
    else
    {
      fprintf(stderr, "only supporting on-board pins\n");
    }

    // INLINED delay
    {
      struct timespec sleeper, dummy;
      sleeper.tv_sec = (time_t)(howLong / 1000);
      sleeper.tv_nsec = (long)(howLong % 1000) * 1000000;
      nanosleep(&sleeper, &dummy);
    }

    // Turn off RED and YELLOW
    *(gpio + 10) = 1 << (pinRED & 31);
    *(gpio + 10) = 1 << (pinYELLOW & 31);

    // Turn on GREEN
    //*(gpio + 7) = 1 << (pinGREEN & 31);

    if ((pinGREEN & 0xFFFFFFC0 /* PI_GPIO_MASK */) == 0) // bottom 64 pins belong to the Pi
    {
      int off = (theValue == LOW) ? 11 : 7; // ACT/LED 47; register number for GPSET/GPCLR
      *(gpio + 7) = 1 << (pinGREEN & 31);
    }
    else
    {
      fprintf(stderr, "only supporting on-board pins\n");
    }

    // INLINED delay
    {
      struct timespec sleeper, dummy;
      sleeper.tv_sec = (time_t)(howLong / 1000);
      sleeper.tv_nsec = (long)(howLong % 1000) * 1000000;
      nanosleep(&sleeper, &dummy);
    }

    // Turn off GREEN
    *(gpio + 10) = 1 << (pinGREEN & 31);

    // Blink YELLOW 3 times
    for (j = 0; j < 6; j++)
    {
      if (j % 2 == 0)
      {
        *(gpio + 7) = 1 << (pinYELLOW & 31);
      }
      else
      {
        *(gpio + 10) = 1 << (pinYELLOW & 31);
      }
      // INLINED delay
      {
        struct timespec sleeper, dummy;
        sleeper.tv_sec = (time_t)(howLong / 1000);
        sleeper.tv_nsec = (long)(howLong % 1000) * 1000000;
        nanosleep(&sleeper, &dummy);
      }
    }

    // Turn off YELLOW
    *(gpio + 10) = 1 << (pinYELLOW & 31);

    // Turn on RED
    //*(gpio + 7) = 1 << (pinRED & 31);
    if ((pinRED & 0xFFFFFFC0 /* PI_GPIO_MASK */) == 0) // bottom 64 pins belong to the Pi
    {
      int off = (theValue == LOW) ? 11 : 7; // ACT/LED 47; register number for GPSET/GPCLR
      *(gpio + off) = 1 << (pinRED & 31);
    }
    else
    {
      fprintf(stderr, "only supporting on-board pins\n");
    }
    // INLINED delay
    {
      struct timespec sleeper, dummy;
      sleeper.tv_sec = (time_t)(howLong / 1000);
      sleeper.tv_nsec = (long)(howLong % 1000) * 1000000;
      nanosleep(&sleeper, &dummy);
    }

    // Turn off RED
    *(gpio + 10) = 1 << (pinRED & 31);

    // Clean up: write LOW
    *(gpio + 10) = 1 << (23 & 31);

    fprintf(stderr, "end main.\n");
  }
}