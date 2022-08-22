#ifndef DEV_FW_UPDATE_H
#define DEV_FW_UPDATE_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "proj_config.h"
#include "util_common.h"
#include "fw_update.h"

int check_bic_info(ipmi_ctx_t ipmi_ctx, uint8_t *buff, uint32_t buff_len);
int do_bic_update(ipmi_ctx_t ipmi_ctx, uint8_t *buff, uint32_t buff_len);

#endif
