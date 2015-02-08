#ifndef _ELANBSD_ELANBSD_H_
#define _ELANBSD_ELANBSD_H_
#include <iostream>
#include <errno.h>

/* aux device commands (sent to KBD_DATA_PORT) */
#define PSMC_RESET_DEV	     	0x00ff
#define PSMC_ENABLE_DEV      	0x00f4
#define PSMC_DISABLE_DEV     	0x00f5
#define PSMC_SET_DEFAULTS	0x00f6
#define PSMC_SEND_DEV_ID     	0x00f2
#define PSMC_SEND_DEV_STATUS 	0x00e9
#define PSMC_SEND_DEV_DATA	0x00eb
#define PSMC_SET_SCALING11	0x00e6
#define PSMC_SET_SCALING21	0x00e7
#define PSMC_SET_RESOLUTION	0x00e8
#define PSMC_SET_STREAM_MODE	0x00ea
#define PSMC_SET_REMOTE_MODE	0x00f0
#define PSMC_SET_SAMPLING_RATE	0x00f3

const int ETP_XMAX_V2 = 1152;
const int ETP_YMAX_V2 = 768;
const int ETP_WIDTH_V2 = 80;

const unsigned char ETP_FW_ID_QUERY = 0x00;
const unsigned char ETP_FW_VERSION_QUERY = 0x01;
const unsigned char ETP_CAPABILITIES_QUERY = 0x02;
const unsigned char ETP_SAMPLE_QUERY = 0x03;
const unsigned char ETP_RESOLUTION_QUERY = 0x04;

const unsigned char ETP_PS2_CUSTOM_COMMAND = 0xf8;

const unsigned char ETP_REGISTER_READWRITE = 0x00;
const unsigned char ETP_REGISTER_READ= 0x10;
const unsigned char ETP_REGISTER_WRITE = 0x11;

const int ETP_MAX_FINGERS = 5;
const int ETP_MAX_BUTTONS = 20;

const int HEAD = 1;
const int MOTION = 2;
const int STATUS = 3;
const int UNKNOWN = 0;

inline void errexit(const std::string& err) {
    std::cerr << strerror(errno) << std::endl;
	std::cerr << err << std::endl;
	exit(errno);
}

void create_timeout(void (*timeout_func)(void* arg), void *arg, int ms);

#endif // _ELANBSD_ELANBSD_H_
