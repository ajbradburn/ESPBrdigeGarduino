
union short_float{
	float f;
	uint8_t i8[4];
	uint16_t i16[2];
	uint32_t ul;
};

  // Variables and settings for our custom, addressable SPI interface.
#define PACKET_START 0x28
#define PACKET_END 0x29
#define PACKET_DELIM 0x3A
#define PACKET_SIZE 21

extern uint8_t packet_template[PACKET_SIZE];

#define P_ADDRESS_I 1
#define P_COMMAND_I 6
#define P_PARAMETER_I 11
#define P_PAYLOAD_I 16

  // Output buffer.
extern uint8_t ob[PACKET_SIZE];
extern volatile int ob_index;

  // Input buffer.
extern uint8_t ib[PACKET_SIZE];
extern volatile int ib_index;

  // Input values.
extern volatile unsigned long i_address;
extern volatile unsigned long i_command;
extern volatile unsigned long i_parameter;
extern volatile unsigned long i_payload;

  // Time based reset variables.
extern volatile unsigned long t_start;
extern volatile bool active_transmission;
#define T_TIMEOUT 150 // Milliseconds

  // Flag that a complete packet has been received.
extern volatile bool request;
  // Flag that we are in the process of sending data.
extern volatile bool response;

void debug_setup(void);
void as_reset(void);
bool as_packet_integrity_check(void);
bool as_packet_unpack(void);
void as_prepare_ob(void);
bool as_packet_pack(unsigned long response_payload);
void as_loop(void);

extern void command_execute(void);

