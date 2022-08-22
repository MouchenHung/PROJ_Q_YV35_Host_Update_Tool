#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "util_freeipmi.h"

/*
  - Name: send_recv_command
  - Description: Send and receive message of ipmi-raw
  - Input:
      * ipmi_ctx: Pointer to save ipmi-raw session
      * msg: IPMI package
  - Return:
      * Completion code, if no error
      * -1, if error
  - Note: Support OEM command 0x38 with auto-fill IANA
*/
int send_recv_command(ipmi_ctx_t ipmi_ctx, ipmi_cmd_t *msg)
{
    if (!ipmi_ctx || !msg) {
        log_print(LOG_ERR, "%s: Get empty inputs!\n", __func__);
        return -1;
    }

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
        ipmi_data[1] = CONFIG_IANA_1;
        ipmi_data[2] = CONFIG_IANA_2;
        ipmi_data[3] = CONFIG_IANA_3;
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
        log_print(LOG_NON, "         * ipmi command     : 0x%x/0x%x\n", msg->netfn, ipmi_data[0]);
        log_print(LOG_NON, "         * ipmi data length : %d\n", ipmi_data_len-1);
        log_print(LOG_NON, "         * ipmi data        : ");

        int max_data_print = ipmi_data_len;

        if (g_log_level == 2) {
            /* IPMI data max print limit is 10 */
            if (msg->data_len > 10)
                max_data_print = 10;
        }

        // print from iana or first data
        for (int i=1; i<max_data_print; i++)
            log_print(LOG_NON, "0x%x ", ipmi_data[i]);
        if (g_log_level == 2)
            log_print(LOG_NON, "...");
        log_print(LOG_NON, "\n");
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
        log_print(LOG_ERR, "%s: ipmi-raw received bad cc 0x%x\n", __func__, bytes_rs[1]);
        goto ending;
    }

    /* Check for oem iana */
    if (oem_flag) {
        if (bytes_rs[2]!=CONFIG_IANA_1 || bytes_rs[3]!=CONFIG_IANA_2 || bytes_rs[4]!=CONFIG_IANA_3) {
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
