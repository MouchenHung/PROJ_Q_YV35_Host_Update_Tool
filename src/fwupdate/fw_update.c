#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include "fw_update.h"

char *IMG_TYPE_LST[FW_T_MAX_IDX] = {
    "BIC"
};

const char *const prj_comp_name[] = {
	[IMG_COMP_CPLD] = log_num_to_name(IMG_COMP_CPLD),
	[IMG_COMP_BIC] = log_num_to_name(IMG_COMP_BIC),
	[IMG_COMP_BIOS] = log_num_to_name(IMG_COMP_BIOS),
};

const char *const prj_stage_name[] = {
	[STAGE_POC] = log_num_to_name(STAGE_POC),
	[STAGE_EVT] = log_num_to_name(STAGE_EVT),
    [STAGE_DVT] = log_num_to_name(STAGE_PVT),
	[STAGE_PVT] = log_num_to_name(STAGE_PVT),
    [STAGE_MP] = log_num_to_name(STAGE_MP),
};

/*
  - Name: img_parsing_and_validate
  - Description: Parsing image sign key and validate image which should mach with target device
  - Input:
      * ipmi_ctx: Pointer to save ipmi-raw session
      * buff: Buffer to store image bytes
      * buff_len: Buffer length
      * dev_type: Target device type
  - Return:
      * 0, if no error
      * 1, if error
*/
int img_parsing_and_validate(ipmi_ctx_t ipmi_ctx, uint8_t *buff, uint32_t buff_len, fw_type_t dev_type)
{
    PARM_CHECK(ipmi_ctx, 1, __func__)
    PARM_CHECK(buff, 1, __func__)

    int ret = 1;
    switch (dev_type)
    {
    case FW_T_BIC:
        if (check_bic_info(ipmi_ctx, buff, buff_len) == 0)
            ret = 0;
        break;

    default:
        log_print(LOG_ERR, "%s: Invalid device type %d\n", __func__, dev_type);
        break;
    }

    return ret;
}

/*
  - Name: fw_update
  - Description: Firmware update controller
  - Input:
      * ipmi_ctx: Pointer to save ipmi-raw session
      * buff: Buffer to store image bytes
      * buff_len: Buffer length
      * dev_type: Target device type
      * max_retry: Maximum retry time
  - Return:
      * 0, if no error
      * 1, if error
*/
int fw_update(ipmi_ctx_t ipmi_ctx, uint8_t *buff, uint32_t buff_len, fw_type_t dev_type, int max_retry)
{
    PARM_CHECK(ipmi_ctx, 1, __func__)
    PARM_CHECK(buff, 1, __func__)

    int ret = 1;
    int retry = 0;
    while (retry <= max_retry) {
        if (retry) {
            log_print(LOG_NON, "\n");
            log_print(LOG_INF, "FW update retry %d/%d ...\n", retry, max_retry);
        }

        switch(dev_type)
        {
        case FW_T_BIC:
            if (do_bic_update(ipmi_ctx, buff, buff_len) == 0)
                ret = 0;
            break;

        default:
            log_print(LOG_ERR, "%s: No such dev_type!\n", __func__);
            break;
        }

        if (!ret)
            break;

        retry++;
    }

    return ret;
}
