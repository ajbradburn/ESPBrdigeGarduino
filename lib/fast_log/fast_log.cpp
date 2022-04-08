#include "Arduino.h"
#include "string.h"
#include "fast_log.h"

// The log should be exported at a time interval in the event that the log isn't filled up prior to that point.

// There should be a flag that is set when the log is full.  Rather than dump it as the log is being expanded. We can afford to have the log dumped in the middle of a triggered event.
bool ledger_full = false;

struct fast_log {
    char ledger[512];
    int bookmark;
};

struct fast_log session_log;

void clear_ledger(){
    memset(session_log.ledger, 0x00, sizeof(session_log.ledger));
    session_log.bookmark = 0;
    ledger_full = false;
}

void log_force(){
	ledger_full = true;
}

void log_poll(){
	if(ledger_full){
		log_export();
	}
}

void log_init(){
    Serial.begin(115200);

    clear_ledger();
};

void log_append_char(char entry){
    if(session_log.bookmark + 2 >= sizeof(session_log.ledger)){
        // End of the ledger, we can't add anything else to the ledger.
        ledger_full = true;
    }

    session_log.ledger[session_log.bookmark++] = entry;

	session_log.ledger[session_log.bookmark++] = 0x0a;
};

void log_append_string(char *entry_long, int entry_length, bool newline){
    if(session_log.bookmark + 2 >= sizeof(session_log.ledger) || session_log.bookmark + entry_length + 1 >= sizeof(session_log.ledger)){
        ledger_full = true;
    }

    for(int i = 0; i < entry_length; i++){
        session_log.ledger[session_log.bookmark++] = entry_long[i];
    }   

	if(newline){
		session_log.ledger[session_log.bookmark++] = 0x0a;
	}
}

void log_append_int(int entry){
        char num[6]; // Accomodate Max Value 65535
	itoa(entry, num, 10);
        log_append_string(num, strlen(num), false); // The string object includes a newline automatically, so no need to add another.
}

void log_append_long(long entry){
	char num[11];
	ltoa(entry, num, 10);
	log_append_string(num, strlen(num), false);
}

void log_append_ulong(unsigned long entry){
	char num[11];
	ultoa(entry, num, 10);
	log_append_string(num, strlen(num), false);
}

void log_append_float(float entry){
	char num[8];
	dtostrf(entry, 6, 2, num);
	log_append_string(num, strlen(num));
}

void log_export(){
	if(session_log.bookmark > 0){
		Serial.print(session_log.ledger);
		Serial.println();
		clear_ledger();
	}
};
