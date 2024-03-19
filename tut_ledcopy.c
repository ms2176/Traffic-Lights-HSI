// Optimized Code - Compile: gcc  -o  t1 tut_led.c - Run: sudo ./t1

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

void setPinOn(int pin)
{
  if ((pin & 0xFFFFFFC0 /* PI_GPIO_MASK */) == 0) // bottom 64 pins belong to the Pi
  {
    *(gpio + 7) = 1 << (pin & 31); // 7 is the offset for GPSET0 register sets GPIO pins 0-31 to HIGH
  }
  else
  {
    fprintf(stderr, "only supporting on-board pins\n");
  }
}

void setPinOff(int pin)
{
  if ((pin & 0xFFFFFFC0 /* PI_GPIO_MASK */) == 0) // bottom 64 pins belong to the Pi
  {
    *(gpio + 10) = 1 << (pin & 31); // 10 is the offset for GPCLR0 register sets GPIO pins 0-31 to LOW
  }
  else
  {
    fprintf(stderr, "only supporting on-board pins\n");
  }
}

void delay(int howLong)
{
  struct timespec sleeper, dummy;
  sleeper.tv_sec = (time_t)(howLong / 1000);
  sleeper.tv_nsec = (long)(howLong % 1000) * 1000000;
  nanosleep(&sleeper, &dummy);
}

int main(void)
{
  int pinRED = RED, pinYELLOW = YELLOW, pinGREEN = GREEN; //
  int fSel, shift, pin, clrOff, setOff;
  int fd;
  int j;
  int theValue, thePin;
  unsigned int howLong = DELAY;
  uint32_t res; /* testing only */

  printf("Raspberry Pi blinking LED %d %d %d\n", pinRED, pinYELLOW, pinGREEN);

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
  *(gpio + 1) = (*(gpio + 1) & ~(0b111)) | 0b1; // binary literals

  fprintf(stderr, "setting pin %d to %d ...\n", pinYELLOW, OUTPUT); // GPIO 11 sits in slot 1 of register 1, thus shift by 1*3 (3 bits per pin)
  //*(gpio + 1) = (*(gpio + 1) & ~(7 << 3)) | (1 << 3) ;  // Sets bits to one = output
  *(gpio + 1) = (*(gpio + 1) & ~(0b111000)) | 0b1000; // binary literals

  fprintf(stderr, "setting pin %d to %d ...\n", pinGREEN, OUTPUT); // GPIO 13 sits in slot 3 of register 1, thus shift by 3*3 (3 bits per pin)
  //*(gpio + 1) = (*(gpio + 1) & ~(7 << 9)) | (1 << 9) ;  // binary literals
  *(gpio + 1) = (*(gpio + 1) & ~(0b111000000000)) | 0b1000000000; // binary literals

  // -----------------------------------------------------------------------------
  setPinOff(pinRED);
  setPinOff(pinYELLOW);
  setPinOff(pinGREEN);

  // now, start a loop
  fprintf(stderr, "starting loop ...\n");

  for (j = 0; j < 1000; j++)
  {
    // Turn on RED
    setPinOn(pinRED);
    delay(howLong);

    // Turn on YELLOW
    setPinOn(pinYELLOW);
    delay(howLong);

    // Turn off RED and YELLOW
    setPinOff(pinRED);
    setPinOff(pinYELLOW);

    // Turn on GREEN
    setPinOn(pinGREEN);
    delay(howLong);

    // Turn off GREEN
    setPinOff(pinGREEN);

    // Blink YELLOW 3 times
    for (int i = 0; i < 6; i++)
    {
      if (i % 2 == 0)
      {
        setPinOn(pinYELLOW);
      }
      else
      {
        setPinOff(pinYELLOW);
      }
      delay(howLong);
    }

    // Turn off YELLOW
    setPinOff(pinYELLOW);

    // Turn on RED
    setPinOn(pinRED);
    delay(howLong);

    // Turn off RED
    setPinOff(pinRED);
  }

  fprintf(stderr, "end main.\n");
}