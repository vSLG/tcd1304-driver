/*
 * TCD1304.h
 *
 *  Created on: May 20, 2023
 *      Author: slg
 */

#ifndef INC_TCD1304_H_
#define INC_TCD1304_H_

#include <stdint.h>

#define MAGIC 0x1304

enum CCDMsgType {
	CCDMSG_CFG,
	CCDMSG_READ,
};

typedef struct CCDConfig {
	uint32_t sh_period;
	uint32_t icg_period;
} CCDConfig_t;

typedef struct CCDCmd {
	uint16_t magic;
	uint8_t type;
	uint32_t len;
	uint8_t data[];
} __attribute__((__packed__)) CCDCmd_t;

void tcd1304_set_config(CCDConfig_t *cfg);
void tcd1304_setup();
void tcd1304_loop();

extern CCDConfig_t ccd_config;


#endif /* INC_TCD1304_H_ */
