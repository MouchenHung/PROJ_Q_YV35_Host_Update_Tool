#ifndef DEV_FW_UPDATE_H
#define DEV_FW_UPDATE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "proj_config.h"
#include "util_common.h"
#include "fw_update.h"

/* Parsing from wiwynn fw sign tool */
typedef struct __attribute__((packed)) {
    uint8_t platform_name[16];
    uint8_t version[13];
    union {
		uint8_t value[3];
        struct __attribute__((packed)) {
            uint8_t board_id : 5;
            uint8_t board_stage : 3;
            uint8_t comp_id : 3;
            uint16_t inst : 12;
			uint8_t rsv : 1;
		} fields;
	} board_info;
} board_info_t;

int check_bic_info(ipmi_ctx_t ipmi_ctx, uint8_t *buff, uint32_t buff_len);
int do_bic_update(ipmi_ctx_t ipmi_ctx, uint8_t *buff, uint32_t buff_len);

#endif
