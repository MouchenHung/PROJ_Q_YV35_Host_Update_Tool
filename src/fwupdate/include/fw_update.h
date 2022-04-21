#ifndef FW_UPDATE_H
#define FW_UPDATE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "proj_config.h"
#include "util_common.h"
#include "util_ipmiraw.h"

/* Firware update command relative config(using ipmi-raw) */
#define FW_UPDATE_NETFN 0x38
#define FW_UPDATE_CMD 0x09
#define FW_UPDATE_LUN 0x00

#define IPMI_RAW_RETRY 2

typedef enum fw_type {
    FW_T_BIC = 0,
    FW_T_MAX_IDX
} fw_type_t;

char *IMG_TYPE_LST[FW_T_MAX_IDX];

int do_bic_update(uint8_t *buff, uint32_t buff_len);
int fw_update(fw_type_t flag, uint8_t *buff, uint32_t buff_len, int max_retry);

#endif
