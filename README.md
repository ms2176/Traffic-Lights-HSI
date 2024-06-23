# Traffic Lights: Raspberry Pi LED Control

This project demonstrates control of multiple LEDs and a button using a Raspberry Pi's GPIO pins. It's written in C and provides direct hardware access for GPIO manipulation.

## Features

- Controls three LEDs (Red, Yellow, Green) connected to GPIO pins 10, 11, and 13 respectively
- Reads input from a button connected to GPIO pin 26
- Implements a blinking pattern for the LEDs

## Prerequisites

- Raspberry Pi (tested on RPi 2)
- GCC compiler
- Root access (sudo)

## Hardware Setup

Connect LEDs and a button to your Raspberry Pi as follows:
- Red LED: GPIO 10
- Yellow LED: GPIO 11
- Green LED: GPIO 13
- Button: GPIO 26

Ensure proper resistors are used with the LEDs.

## Compilation

Compile the program using GCC:

```
gcc -o led_control led_control.c
```

## Usage

Run the compiled program with sudo:

```
sudo ./led_control
```

The program will execute a blinking pattern on the LEDs.

## Notes

- This program uses direct memory access to control GPIO pins, which requires root privileges.
- The code includes detailed comments explaining the GPIO manipulation process.
- The main loop runs 1000 times before terminating.

## Caution

Be careful when using direct memory access. Incorrect usage can potentially damage your Raspberry Pi.

## Acknowledgements

This code was developed as part of the 2nd year (2023-2024) Hardware Software Interface course in the BSc Computer Science program at Heriot-Watt University. It was completed as a pair coursework assignment. 
