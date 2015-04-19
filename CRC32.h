#ifndef CRC32_H
#define CRC32_H

#include "Arduino.h"

class CRC32 {
	public:
		CRC32();
		/*~CRC32();*/
		unsigned long crc_string(const char *s);
	private:
		unsigned long crc_update(unsigned long crc, byte data);
};

#endif