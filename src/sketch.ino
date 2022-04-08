#if DEBUG
#include "fast_log.h"
#include "MemoryFree.h"
#endif

  // The device address, along with other device specific values are set here.
#include "device_specifics.h"

#if IS_SLAVE_MASTER
#define ESP_RESET 3
#define ESP_RESET_DELAY 4000
volatile bool esp_reset;
volatile unsigned long esp_reset_time;
#endif

/* ---------- HTU21D OPTION */
// The HTU21D is a temperature/humidity sensor.
#define HT_SEN 1

#if HT_SEN
#include "Wire.h"
#include "SparkFunHTU21D.h"
HTU21D HTSen;

volatile union short_float HT_temp;
volatile union short_float HT_hum;

bool temp_read = false;
bool hum_read = false;
#endif
// END HTU21D

/* ---------- SI1145 OPTION */
// The SI1145 is a light sensor, which reads infrared, visibile, and UV levels.
#define SI_SEN 1

#if SI_SEN
#include "Wire.h"
#include "Adafruit_SI1145.h"

Adafruit_SI1145 SISen = Adafruit_SI1145();

volatile union short_float SI_IR;
volatile union short_float SI_VL;
volatile union short_float SI_UV;

bool IR_read = false;
bool VL_read = false;
bool UV_read = false;
#endif
//END SI1145

/* ---------- SOIL MOISTURE SENSORS */
#define SM_SEN 1
#if SM_SEN

#define SM_ANALOG 1
#define SEN1_A A7

#define SM_DIGITAL 0
#define SEN1_D 8

volatile int sen1_analog;
volatile int sen1_digital;
#endif
// END SOIL MOISTURE SENSORS

/* ---------- WATER PUMP */

	/* We need to control the water pump, but limit it's functionality.  We don't want it left running in a way that will flood the plant.
	Ideally, when called, the function would start a timer, and, when done, stop.
	I don't know if we should include some protections to keep it from running when the resistance is too high (disconnected), or too low (flooded).
	*/
#define WATER_PUMP 1

#if WATER_PUMP
#define WATER_PIN A2
#define WATER_RUNTIME 2 // SECONDS
volatile unsigned long water_start;
bool water_on = true;
#endif
// END WATER PUMP

/* ---------- LIGHT */
#define LIGHT_CONTROL 1

#if LIGHT_CONTROL
#define LIGHT_PIN A1
#endif

// END LIGHT

#include "SSPI.h"

#if IS_SLAVE_MASTER
  /* ---------- SPI Slave Alert Interface */
#include "ssa.h"
#endif

  /* ---------- Addressable SPI Interface */
#include "as.h"

void(* resetFunc) (void) = 0;

void setup (void){
	#if DEBUG
	log_init();
	#endif

	#if IS_SLAVE_MASTER
	pinMode(ESP_RESET, OUTPUT);
	reset_master();
	#else
	SSPI.slave_begin();
	#endif

	SSPI.attachInterrupt();

	  // Set all relevant variables to their default variables.
	as_reset();

	  // Alert interrupt setup.
	  // This is a feature that is only needed by the slave master.
	#if IS_SLAVE_MASTER
	slave_alert_init();
	#endif

	#if HT_SEN
	HTSen.begin();
	#endif

	#if SI_SEN
	SISen.begin();
	#endif

	#if SM_SEN
	pinMode(SEN1_A, INPUT);
	pinMode(SEN1_D, INPUT);
	#endif

	#if WATER_PUMP
	#endif

	#if DEBUG
	log_append_string("SC", 2);
	#endif

	/* Timer Based Interrupt - Application Maintenance */

	// These settings control the frequency that the ISR(TIMER1_COMPA_vect) function is called.

	  // Disable interrupts
	cli();
	TCCR1A = 0;
	TCCR1B = 0;

	  // Set compare register to desired interrupt interval.
	  // Clock speed of 16,000,000 divided by the prescalar of 1024 = 15625 ticks a second, or time between ticks of [1/15625].  If you want the interrupt to happen evern "t" seconds, you need to find the number of ticks by dividing "t"/[1/15625].
	  // Subtract 1 from the final value to account for the fact that resetting the clock to zero eats a cycle befor the interrupt is called.
	OCR1A = 15624;  // [number] = [target_time] / [timer resolution] - 1| [1s] / [6.4^-5] - 1
	  // Turn on interrupt on compare mode (CTC Mode).
	TCCR1B |= (1 << WGM12);
	  // Configure prescalar for 1024 resolution.
	TCCR1B |= (1 << CS10);
	TCCR1B |= (1 << CS12);
	  // Enable timer compare interrupt.
	TIMSK1 |= (1 << OCIE1A); // Also enabled in the addressable spi reset function.
	sei();
	// END TIMER BASED INTERRUPT

}

#if IS_SLAVE_MASTER
void reset_master(void){
	  // Pin 10 on Arduino connects to D8 on the Esp8266, This needs to be kept low on boot of the Esp8266.
	  // Pin 10 is also known as SS (Slave Select).
	  // Reset pin on the Esp8266 is connected to pin D3 on the Arduino.

	  // SPI interface requires that this be INPUT. But, to reboot the ESP8266, we need to bring it low.
	pinMode(SS, OUTPUT);
	digitalWrite(SS, LOW);

	digitalWrite(MISO, LOW);

	delay(10);

	  // End SPI interface.
	SSPI.end();

	delay(10);

	  // Turn off the ESP8266
	digitalWrite(ESP_RESET, LOW);
	delay(500);

	  // The pint used for alert must be high for the ESP8266 to boot properly.  Also will trigger an alert if the ESP8266 is on when brought high.
	  //pinMode(ALERT_BRIDGE_PIN, OUTPUT);
	digitalWrite(ALERT_BRIDGE_PIN, HIGH);

	delay(500);

	digitalWrite(ESP_RESET, HIGH);
	delay(100);

	digitalWrite(ALERT_BRIDGE_PIN, LOW);

	  // The SPI interface requires that this be INPUT, and should have a default of HIGH (pullup resistor).
	pinMode(SS, INPUT);
	digitalWrite(SS, HIGH);

	  // Begin the SPI interface.
	SSPI.slave_begin();
}
#endif

// Applications maintenance function.
ISR(TIMER1_COMPA_vect){
	/* -- User code may be included here for operations that need to be executed on a regular interval.  Care should be taken to prevent excessive excessive length inside an interrupt. */
	#if DEBUG
	#if DEBUG_MEM
	log_append_string("Free Mem: ", 10, false);
	log_append_int(getFreeMemory());
	#endif
	log_append_ulong(millis());
	log_force();
	#endif
}

void loop(void){
	  // If an interrupt has occured, this will signal to the master that we need to perform some action.
	  // Slave Alert maintenance operations.
	slave_alert_loop();

	  // Addressable SPI maintenance operations.
	as_loop();

	#if IS_SLAVE_MASTER
	if(esp_reset && millis() >= esp_reset_time){
		reset_master();
		esp_reset = false;
	}
	#endif

	  // Intensive operations shouldn't be executed while there is an active SPI transaction.
	if(!active_transmission){
		// Place intensive operations here.
		#if DEBUG
		log_poll();
		#endif

		#if HT_SEN
		if(hum_read){
			HT_hum.f = HTSen.readHumidity();
			#if DEBUG
			log_append_float(HT_hum.f);
			#endif
			hum_read = false;
		}

		if(temp_read){
			HT_temp.f = HTSen.readTemperature();
			#if DEBUG
			log_append_float(HT_temp.f);
			#endif
			temp_read = false;
		}
		#endif

		#if SI_SEN
		if(IR_read){
			SI_IR.i[0] = SISen.readIR();
			IR_read = false;
		}

		if(VL_read){
			SI_VL.i[0] = SISen.readVisible();
			VL_read = false;
		}

		if(UV_read){
			SI_UV.i[0] = SISen.readUV();
			UV_read = false;
		}
		#endif
	}
	  // Place non-intensive operations here.
	#if WATER_PUMP
	if(water_on){
		if(abs(millis() - water_start) >= WATER_RUNTIME * 1000){
			digitalWrite(WATER_PIN, LOW);
			water_on = false;
		}
	}
	#endif
}  // end of loop

void command_execute(){
	switch(i_command){
		  // Commands 1 - 100 are reserved for universal commands.
		  // Custom commands should be in the range of 101+.
		case 1:
			  // Included command, will be used as a simple test command to verify that the device is present on the buss.
			as_packet_pack(1);
			break;
		#if IS_SLAVE_MASTER
		case 2:
			  // Return the address of the alert.
			/*
				With this design, an event on a slave can trigger a pin to high, which will trigger an interrupt on the slave master.
				The slave master will read in the address which triggered the interrupt, and then alert the ESP bridge.
				The ESP bridge will request this address, and then place an HTTP request to the application server, and inclue this address.
				The application server will need to keep track of which device is at which alert address.
			*/
			as_packet_pack(slave_alert_address);
			slave_alert_reset();
			break;
		case 10:
			as_packet_pack(1);
			esp_reset = true;
			esp_reset_time = millis() + ESP_RESET_DELAY;
			break;
		case 20:
			resetFunc();
			break;
		#endif
		case 101:
			  // Start your custom command responses here.
			  /* Parameters passed through the REST API will be available here.
			  i_command, i_parameter, i_payload
				These parameters are 32 bits long and unsigned long.  If you're expecting a signed number, you're going to have to do some conversion, possibly with the short_float structure that is used to return values.
				Similarly, if you're reading a signed long value from a sesor, and you need to return that via the SPI interface, you will have to convert it into unsigned long via the short_float structure provided, and then decode it on the receiving end to make it useable.
				I don't have the energy to explain how all of this works at this time. If you've got questions, ask.
				*/
			break;
		case 201:
			break;
		#if HT_SEN
		case 311:
			// Read humidity
			as_packet_pack(1);
			hum_read = true;
			break;
		case 312:
			// Return humidity
			as_packet_pack(HT_hum.ul);
			break;
		case 321:
			// Read temperature
			as_packet_pack(1);
			temp_read = true;
			break;
		case 322:
			// Return temperature
			as_packet_pack(HT_temp.ul);
			break;
		#endif
		#if SI_SEN
		case 411:
			// Read IR
			as_packet_pack(1);
			IR_read = true;
			break;
		case 412:
			// Return IR
			as_packet_pack(SI_IR.ul);
			break;
		case 421:
			as_packet_pack(1);
			VL_read = true;
			// Read VL
			break;
		case 422:
			// Return VL
			as_packet_pack(SI_VL.ul);
			break;
		case 431:
			as_packet_pack(1);
			UV_read = true;
			// Read UV
			break;
		case 432:
			// Return UV
			as_packet_pack(SI_UV.ul);
			break;
		#endif
		#if SM_SEN
		#if SM_ANALOG
		case 511:
			sen1_analog = analogRead(SEN1_A);
			as_packet_pack(sen1_analog);
			break;
		#endif
		#if SM_DIGITAL
		case 512:
			sen1_digital = digitalRead(SEN1_D);
			as_packet_pack(sen1_digital);
			break;
		#endif
		#endif
		#if WATER_PUMP
		case 701:
			as_packet_pack(1);
			water_start = millis();
			water_on = true;
			digitalWrite(WATER_PIN, HIGH);
			break;
		#endif
		#if LIGHT_CONTROL
		case 801:
			as_packet_pack(1);
			digitalWrite(LIGHT_PIN, HIGH);
			break;
		case 802:
			as_packet_pack(1);
			digitalWrite(LIGHT_PIN, LOW);
			break;
		#endif
		default:
			break;
	}
}
