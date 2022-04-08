/*
	The library was designed to take input from slave devices on an SPI bus.

	The design hinges on a parallel-in/serial-out shift register like the 74HC165N.
	All lines should be connected to a single pin with the use of diodes.
	When a slave has registered an event, it can signal the master, which will read a value from the registers.
	Once an address has been read, it will signal the bridge device (ESP8266), which will request the register address of the slave, and pass that along via a HTTP request to the application server.

	The number of slave devices supported in this matter is limited to the number/value of the parallel-in/serial-out registers in use.
	The 74HC165N is an 8 bit register.
*/

#include <Arduino.h>

  // Pin definitions.
#define ALERT_CLOCK 6
#define ALERT_LATCH 7
#define ALERT_DATA 5

#define ALERT_DELAY 10
#define ALERT_BRIDGE_PIN 4

  // n * 8 = ALERT_ADDRESS_WIDTH, where n is the number of devices in series.
#define ALERT_ADDRESS_WIDTH 8

extern volatile unsigned long slave_alert_address;

void slave_alert_init(void);
void slave_alert_loop(void);
void slave_alert_ISR(void);
void slave_alert_read_address(void);
void slave_alert_reset(void);
