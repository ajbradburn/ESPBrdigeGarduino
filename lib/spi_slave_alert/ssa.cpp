#include "ssa.h"
#include "device_specifics.h"

#if DEBUG
#include "fast_log.h"
#endif

volatile unsigned long slave_alert_address;

volatile uint8_t event;

void slave_alert_init(void){

	slave_alert_address = 0;

	  // Configure the hardware interrupt.
	pinMode(2, INPUT);
	digitalWrite(2, LOW);
	  // Digital pin 2 is actually mapped to int.0
	  // https://www.arduino.cc/en/Reference/attachInterrupt
	attachInterrupt(0, slave_alert_ISR, HIGH);

	  // Configure the pins for reading from the parallel in/serial out register.
	pinMode(ALERT_LATCH, OUTPUT);
	digitalWrite(ALERT_LATCH, HIGH);

	pinMode(ALERT_CLOCK, OUTPUT);
	digitalWrite(ALERT_CLOCK, HIGH);

	pinMode(ALERT_DATA, INPUT);

	pinMode(ALERT_BRIDGE_PIN, OUTPUT);
	digitalWrite(ALERT_BRIDGE_PIN, LOW);
}

void slave_alert_loop(void){
	if(event){
		slave_alert_read_address();

		event = false;
		digitalWrite(ALERT_BRIDGE_PIN, HIGH);
	}
}

void slave_alert_ISR(void){
	event++;
	#if DEBUG
	log_append_string("SA", 2);
	#endif
}

void slave_alert_read_address(void){
	  // Set latch LOW to lock in the input.  Return to HIGH.
	  // Bring clock LOW.
	  // Read input.
	  // Bring clock HIGH.

	digitalWrite(ALERT_LATCH, LOW);
	delayMicroseconds(ALERT_DELAY);
	digitalWrite(ALERT_LATCH, HIGH);

	for(int i = 0; i < ALERT_ADDRESS_WIDTH; i++){
		digitalWrite(ALERT_CLOCK, LOW);
		slave_alert_address = (slave_alert_address << 1) | digitalRead(ALERT_DATA);
		delayMicroseconds(ALERT_DELAY/2);
		digitalWrite(ALERT_CLOCK, HIGH);
		delayMicroseconds(ALERT_DELAY/2);
	}
}

void slave_alert_reset(void){
	digitalWrite(ALERT_BRIDGE_PIN, LOW);
	slave_alert_address = 0;
}
