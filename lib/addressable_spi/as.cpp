#include "device_specifics.h"
#include "SSPI.h"
#include "as.h"

#if DEBUG
#include "fast_log.h"
#endif

uint8_t packet_template[PACKET_SIZE] = {PACKET_START, 0, 0, 0, 0, PACKET_DELIM, 0, 0, 0, 0, PACKET_DELIM, 0, 0, 0, 0, PACKET_DELIM, 0, 0, 0, 0, PACKET_END};
  // Packet Position:			   0  1  2  3  4	     5  6  7  8  9	    10 11 12 13 14	    15 16 17 18 19	  20

  // Output buffer.
uint8_t ob[PACKET_SIZE];
volatile int ob_index;

  // Input buffer.
uint8_t ib[PACKET_SIZE];
volatile int ib_index;

  // Input values.
volatile unsigned long i_address;
volatile unsigned long i_command;
volatile unsigned long i_parameter;
volatile unsigned long i_payload;

  // Time based reset variables.
volatile unsigned long t_start;
volatile bool active_transmission;

  // Flag that a complete packet has been received.
volatile bool request;
  // Flag that we are in the process of sending data.
volatile bool response;

  // Reset all relevant variables to their default values.
void as_reset(void){
	#if DEBUG
	log_append_string("SPI Res", 7);
	#endif

	pinMode(MISO, INPUT);

	request = false;
	response = false;

	t_start = 0;
	active_transmission = false;

	memset(ib, 0, sizeof(ib));
	ib_index = 0;

	i_address = 0;
	i_command = 0;
	i_parameter = 0;
	i_payload = 0;

	memset(ob, 0, sizeof(ob));
	ob_index = 0;

	  // Re-enable the timer based interrupt for logging.
	TIMSK1 |= (1 << OCIE1A);
}

  // Confirm that the packet has delimiters in the places we think they should be.
  // This is just a way for us to verify that the data we have received is intentional.
bool as_packet_integrity_check(void){
	#if DEBUG_INTERRUPT
	log_append_string("A", 1);
	#endif

	for(int i = 0; i < PACKET_SIZE; i++){
		if(packet_template[i] != 0 && packet_template[i] != ib[i]){
			#if DEBUG_INTERRUPT
			log_append_string("B", 1);
			#endif

			return false;
		}
	}

	return true;
}

  // Take the data that was transmitted in a packet, and load it into the 4 byte integer variables for address, command, parameter, and payload.
  // Because the Arduino SPI buffer works in single byte segments, we have to take our 4, 1 byte values, and load them into our 4 byte variables.
bool as_packet_unpack(void){
	#if DEBUG_INTERRUPT
	log_append_string("PUP", 3);
	#endif

	int i;

	  // Address
	for(i = P_ADDRESS_I; i < P_ADDRESS_I + sizeof(i_address); i++){
		i_address = (i_address << 8) | ib[i];
		ib[i] = 0;
	}
	  // Command
	for(i = P_COMMAND_I; i < P_COMMAND_I + sizeof(i_command); i++){
		i_command = (i_command << 8) | ib[i];

		ib[i] = 0;
	}
	  // Parameter
	for(i = P_PARAMETER_I; i < P_PARAMETER_I + sizeof(i_parameter); i++){
		i_parameter = (i_parameter << 8) | ib[i];
		ib[i] = 0;
	}
	  // Data
	for(i = P_PAYLOAD_I; i < P_PAYLOAD_I + sizeof(i_payload); i++){
		i_payload = (i_payload << 8) | ib[i];
		ib[i] = 0;
	}

	memset(ib, 0, sizeof(ib));
	ib_index = 0;

	return true;
}

  // Load a byte into the SPI buffer.
void as_prepare_ob(void){
	pinMode(MISO, OUTPUT);

	//SPDR = ob[ob_index++];
	SPDR = 0;

	response = true;

	#if DEBUG_INTERRUPT
	log_append_string("POB", 3);
	#endif
}

  // Take the data that is meant to be transmitted, and load it into a packet.
  // Arduino SPI buffer works in single byte segments, so we have to take our 4 byte in variables, and break it up into 4, 1 byte values.
bool as_packet_pack(unsigned long response_payload){
	byte i;

	#if DEBUG
	//log_append_string("pack", 4);
	#endif
	#if DEBUG_INTERRUPT
	log_append_string("C", 1);
	#endif

	memcpy(ob, packet_template, PACKET_SIZE);

	  // Address
	for(i = 0; i < sizeof(i_address); i++){
		ob[P_ADDRESS_I + i] = 0 | i_address >> 8 * (sizeof(i_address) - 1 - i);
	}

	  // Command
	for(i = 0; i < sizeof(i_command); i++){
		ob[P_COMMAND_I + i] = 0 | i_command >> 8 * (sizeof(i_command) - 1 - i);
	}

	  // Parameter
	for(i = 0; i < sizeof(i_parameter); i++){
		ob[P_PARAMETER_I + i] = 0 | i_parameter >> 8 * (sizeof(i_parameter) - 1 - i);
	}

	  // Data
	for(i = 0; i < sizeof(response_payload); i++){
		ob[P_PAYLOAD_I + i] = 0 | response_payload >> 8 * (sizeof(response_payload) - 1 - i);
	}

	as_prepare_ob();
}

void as_loop(void){
	  // Time based reset.
	if(active_transmission){
		if(millis() - t_start > T_TIMEOUT){
			#if DEBUG
			log_append_string("SPI TOut", 8);
			#endif
			as_reset();
			return;
		}
	}

	  // The input buffer (ib) is full, we need to check if we have a request.
/*
	if(request){
		if(!as_packet_integrity_check()){
			as_reset();
			return;
		}

		as_packet_unpack();

		if(i_address == SPI_ADDRESS){
			#if DEBUG
			log_append_string("SPI Req", 7);
			#endif
			command_execute();

			request = false;
			return;
		}else{
			as_reset();
			return;
		}
	}
*/
}

void as_process_request(){
		if(!as_packet_integrity_check()){
			#if DEBUG
			/*
			log_append_string("Rc1", 3);
			for(int i = 0; i < PACKET_SIZE; i++){
				log_append_int(ib[i]);
			}
			*/
			#endif
			as_reset();
			return;
		}

		as_packet_unpack();

		if(i_address == SPI_ADDRESS){
			#if DEBUG
			log_append_string("SPI Req", 7);
			#endif
			command_execute();

			request = false;
			return;
		}else{
			#if DEBUG
			log_append_string("Rc2", 3);
			#endif
			as_reset();
			return;
		}

		#if DEBUG
		log_append_string("End Req", 7);
		#endif
}

  // SPI Interrupt Routine
  // This function will be called once each time the SPI buffer recieves a full byte.
ISR(SPI_STC_vect){
	#if DEBUG_INTERRUPT
	log_append_string("I", 1);
	#endif

	if(!active_transmission){
		#if DEBUG_INTERRUPT
		log_append_string("D", 1);
		#endif
		t_start = millis();

		active_transmission = true;

		  // Disable the timer based interrupt for logging.
		TIMSK1 &= ~(1 << OCIE1A);
	}

	// add to buffer if room
	if(!request && !response){
		#if DEBUG_INTERRUPT
		log_append_string("E", 1);
		#endif

		if(ib_index < sizeof(ib)){
			#if DEBUG_INTERRUPT
			log_append_string("F", 1);
			#endif

			int c = SPDR;  // grab byte from SPI Data Register

			#if DEBUG
			log_append_int(c);
			#endif

			ib[ib_index++] = c;
		}

		if(ib_index == sizeof(ib)){
			#if DEBUG_INTERRUPT
			log_append_string("G", 1);
			#endif

			active_transmission = false;
			request = true;

			as_process_request();
		}
	}

	if(response){
		#if DEBUG_INTERRUPT
		log_append_string("H", 1);
		#endif

		if(ob_index < sizeof(ob)){
			#if DEBUG_INTERRUPT
			log_append_string("J", 1);
			#endif

			SSPI.transfer_slave(ob[ob_index++]);
		}else{
			#if DEBUG_INTERRUPT
			log_append_string("K", 1);
			#endif

			response = false;
			#if DEBUG
			log_append_string("Rc3", 3);
			#endif
			as_reset();
			return;
		}
	}

	#if DEBUG_INTERRUPT
	log_append_string("L", 1);
	#endif
}
  // SPI Interrupt Routine - END
