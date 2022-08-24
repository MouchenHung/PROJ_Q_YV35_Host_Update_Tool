#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "util_freeipmi.h"

uint8_t iana_lst[IANA_MAX][3] = { // LS byte first
    {BYTE(CONFIG_IANA_0, 0), BYTE(CONFIG_IANA_0, 1), BYTE(CONFIG_IANA_0, 2)}, // 15,A0,00
    {BYTE(CONFIG_IANA_1, 0), BYTE(CONFIG_IANA_1, 1), BYTE(CONFIG_IANA_1, 2)}, // 9C,9C,00
};

uint8_t GLOBAL_IANA[3] = { // LS byte first
    BYTE(CONFIG_IANA_DEFAULT, 0),
    BYTE(CONFIG_IANA_DEFAULT, 1),
    BYTE(CONFIG_IANA_DEFAULT, 2)
};

/*
  - Name: switch_global_iana
  - Description: Switch global iana for oem command
  - Input:
      * idx: Type index of iana list
  - Return:
      * 0, if no error
      * 1, if error
*/
int switch_global_iana(iana_type_t idx)
{
    if (idx < 0 || idx >= IANA_MAX) {
        log_print(LOG_ERR, "%s: Invalid iana type index\n", __func__);
        return 1;
    }

    GLOBAL_IANA[0] = iana_lst[idx][0];
    GLOBAL_IANA[1] = iana_lst[idx][1];
    GLOBAL_IANA[2] = iana_lst[idx][2];

    return 0;
}

/*
  - Name: send_recv_command
  - Description: Send and receive message of ipmi-raw
  - Input:
      * ipmi_ctx: Pointer to save ipmi-raw session
      * msg: IPMI package
  - Return:
      * Completion code, if no error
      * -1, if error
  - Note: Support OEM command 0x38 with auto-fill default IANA
*/
int send_recv_command(ipmi_ctx_t ipmi_ctx, ipmi_cmd_t *msg)
{
    PARM_CHECK(ipmi_ctx, -1, __func__)
    PARM_CHECK(msg, -1, __func__)

    int ret = -1;
    int ipmi_data_len = msg->data_len;

    int oem_flag = 0;
    if ((msg->netfn >> 2) == CONFIG_OEM_38) {
        ipmi_data_len += 3;
        if (ipmi_data_len > CONFIG_MAX_IPMB_SIZE)
            return -1;
        oem_flag = 1;
    }

    uint8_t *ipmi_data;
    int init_idx = 0;
    ipmi_data = (uint8_t*)malloc(++ipmi_data_len); // Insert one byte from the head.
    if (!ipmi_data) {
        log_print(LOG_ERR, "%s: ipmi_data malloc failed!\n", __func__);
        return -1;
    }
    ipmi_data[0] = msg->cmd; // The byte #0 is cmd.
    init_idx++;
    if (oem_flag) {
        memcpy(&ipmi_data[1], GLOBAL_IANA, sizeof(GLOBAL_IANA));
        init_idx += 3;
    }
    memcpy(&ipmi_data[init_idx], msg->data, msg->data_len);

    int rs_len = 0;
    uint8_t *bytes_rs = NULL;
    if (!(bytes_rs = calloc (IPMI_RAW_MAX_ARGS, sizeof (uint8_t)))) {
        log_print(LOG_ERR, "%s: bytes_rs calloc failed!\n", __func__);
        goto ending;
    }

    if (g_log_level >= 2) {
        log_print(LOG_NON, "         ipmi-raw:\n");
        log_print(LOG_NON, "         * ipmi command     : 0x%x/0x%x/0x%x\n", msg->netfn>>2, msg->netfn & 0x03, ipmi_data[0]);
        log_print(LOG_NON, "         * ipmi data length : %d\n", ipmi_data_len-1);
        log_print(LOG_NON, "         * ipmi data        : ");

        int max_data_print = ipmi_data_len;

        if (g_log_level == 2) {
            if (msg->data_len > CONFIG_MAX_DATA_DBG_PRINT)
                max_data_print = CONFIG_MAX_DATA_DBG_PRINT;
        }

        // print from iana or first data
        for (int i=1; i<max_data_print; i++)
            log_print(LOG_NON, "0x%x ", ipmi_data[i]);
        if (g_log_level == 2) {
            if (msg->data_len > CONFIG_MAX_DATA_DBG_PRINT)
                log_print(LOG_NON, "...");
        }
        log_print(LOG_NON, "\n\n");
    }

    rs_len = ipmi_cmd_raw (
        ipmi_ctx,
        msg->netfn & 0x03,
        msg->netfn >> 2,
        ipmi_data, //byte #0 = cmd
        ipmi_data_len, // Add 1 because the cmd is combined with the data buf.
        bytes_rs,
        IPMI_RAW_MAX_ARGS
    );

    ret = bytes_rs[1];

    /* Check for ipmi-raw command response */
    if (bytes_rs[0] != msg->cmd || bytes_rs[1] != CC_SUCCESS) {
        if (bytes_rs[1] == CC_INVALID_CMD)
            log_print(LOG_WRN, "%s: Target device not support command NetFn: 0x%x Cmd: 0x%x\n",
                __func__, msg->netfn >> 2, msg->cmd);
        else if (bytes_rs[1] == CC_INVALID_IANA)
            log_print(LOG_WRN, "%s: Target device IANA not mach with given IANA 0x%x%x%x\n",
                __func__, GLOBAL_IANA[2], GLOBAL_IANA[1], GLOBAL_IANA[0]);
        else
            log_print(LOG_ERR, "%s: ipmi-raw received bad cc 0x%x\n", __func__, bytes_rs[1]);
        goto ending;
    }

    /* Check for oem iana */
    if (oem_flag) {
        if (bytes_rs[2]!=GLOBAL_IANA[0] || bytes_rs[3]!=GLOBAL_IANA[1] || bytes_rs[4]!=GLOBAL_IANA[2]) {
            log_print(LOG_ERR, "%s: ipmi-raw received invalid IANA\n", __func__);
            ret = -1;
            goto ending;
        }
    }

    /* return back response data */
    msg->netfn += 1;
    msg->data_len = rs_len - 4 - 1; //minus command code
    memcpy(msg->data, &bytes_rs[5], msg->data_len);

ending:
    if (ipmi_data)
        free(ipmi_data);
    if (bytes_rs)
        free(bytes_rs);

    return ret;
}
