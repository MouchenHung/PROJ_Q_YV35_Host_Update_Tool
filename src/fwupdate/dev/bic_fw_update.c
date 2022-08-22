#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include "dev_fw_update.h"

/*
  - Name: check_bic_info
  - Description: Validate BIC target device info with given image info
  - Input:
      * ipmi_ctx: Pointer to save ipmi-raw session
      * buff: Buffer to store image bytes
      * buff_len: Buffer length
  - Return:
      * 0, if no error
      * 1, if error
*/
int check_bic_info(ipmi_ctx_t ipmi_ctx, uint8_t *buff, uint32_t buff_len)
{
    if (!buff || !ipmi_ctx) {
        log_print(LOG_ERR, "%s: Empty space for buff/ipmi_ctx\n", __func__);
        return 1;
    }

    if (buff_len <= CONFIG_BIC_SIGN_AREA_SIZE) {
        log_print(LOG_ERR, "%s: given buffer size %d lower than sign area size %d\n", __func__, buff_len, CONFIG_BIC_SIGN_AREA_SIZE);
        goto exit;
    }

    uint8_t key_byte[CONFIG_BIC_SIGN_AREA_SIZE];
    memcpy(&key_byte, &buff[buff_len-CONFIG_BIC_SIGN_AREA_SIZE], CONFIG_BIC_SIGN_AREA_SIZE);

    board_info_t img_info;
    memcpy(&img_info, &key_byte[16], sizeof(board_info_t));

    if (g_log_level >= 1) {
        log_print(LOG_NON, "         ------------- fw info ------------\n");
        log_print(LOG_NON, "         * plarform name  : ");
        for (int i=0; i<sizeof(img_info.platform_name); i++) {
            log_print(LOG_NON, "%c", img_info.platform_name[i]);
        }
        log_print(LOG_NON, "\n");
        log_print(LOG_NON, "         * fw version     : ");
        for (int i=0; i<sizeof(img_info.version); i++) {
            log_print(LOG_NON, "%c", img_info.version[i]);
        }
        log_print(LOG_NON, "\n");
        if (img_info.board_info.fields.comp_id >= MAX_IMG_COMP || !img_info.board_info.fields.comp_id)
            log_print(LOG_NON, "         * board component: unknown %d\n", img_info.board_info.fields.comp_id);
        else
            log_print(LOG_NON, "         * board component: %s\n", prj_comp_name[img_info.board_info.fields.comp_id]);
        log_print(LOG_NON, "         * board id       : %d\n", img_info.board_info.fields.board_id);
        if (img_info.board_info.fields.board_stage >= MAX_STAGE)
            log_print(LOG_NON, "         * board stage    : unknown %d\n", img_info.board_info.fields.board_stage);
        else
            log_print(LOG_NON, "         * board stage    : %s\n", prj_stage_name[img_info.board_info.fields.board_stage]);
        log_print(LOG_NON, "         ------------- fw info ------------\n");
    }

    ipmi_cmd_t msg_out;
    int resp_cc;
    int validate_fail = 0;
    int ret = 1;

    if (CONFIG_FW_COMPO_CHECK) {
        // TODO
    }

    if (CONFIG_PROJ_STAGE_CHECK) {
        memset(&msg_out, 0, sizeof(msg_out));
        msg_out.netfn = FW_UPDATE_NETFN << 2;
        msg_out.cmd = OEM_CMD_GET_BIC_FW_INFO;
        msg_out.data_len = 1;
        msg_out.data[0] = BIC_PROJ_STAGE;
        resp_cc = send_recv_command(ipmi_ctx, &msg_out);
        if (resp_cc) {
            log_print(LOG_ERR, "Can't get PROJECT STAGE from target device!\n", BIC_PROJ_STAGE);
            goto exit;
        }

        if (msg_out.data[0] != img_info.board_info.fields.board_stage) {
            log_print(LOG_ERR, "Fw info PROJECT STAGE not mach!\n");
            validate_fail = 1;
        }
    }

    if (CONFIG_PLAT_NAME_CHECK) {
        memset(&msg_out, 0, sizeof(msg_out));
        msg_out.netfn = FW_UPDATE_NETFN << 2;
        msg_out.cmd = OEM_CMD_GET_BIC_FW_INFO;
        msg_out.data_len = 1;
        msg_out.data[0] = BIC_PLAT_NAME;
        resp_cc = send_recv_command(ipmi_ctx, &msg_out);
        if (resp_cc) {
            log_print(LOG_ERR, "Can't get PLATFORM NAME from target device!\n");
            goto exit;
        }

        for (int i=0; i<msg_out.data_len; i++) {
            if (msg_out.data[i] != img_info.platform_name[i]) {
                log_print(LOG_ERR, "Fw info PLATFORM NAME not mach!\n");
                validate_fail = 1;
                break;
            }
        }
    }

    if (CONFIG_BOARD_ID_CHECK) {
        memset(&msg_out, 0, sizeof(msg_out));
        msg_out.netfn = FW_UPDATE_NETFN << 2;
        msg_out.cmd = OEM_CMD_GET_BIC_FW_INFO;
        msg_out.data_len = 1;
        msg_out.data[0] = BIC_PLAT_BOARD_ID;
        resp_cc = send_recv_command(ipmi_ctx, &msg_out);
        if (resp_cc) {
            log_print(LOG_ERR, "Can't get BOARD ID from target device!\n", BIC_PLAT_BOARD_ID);
            goto exit;
        }

        if (msg_out.data[0] != img_info.board_info.fields.board_id) {
            log_print(LOG_ERR, "Fw info BOARD ID not mach!\n");
            validate_fail = 1;
        }
    }

    ret = validate_fail;

exit:
    return ret;
}

/*
  - Name: do_bic_update
  - Description: BIC update process
  - Input:
      * ipmi_ctx: Pointer to save ipmi-raw session
      * buff: Buffer to store image bytes
      * buff_len: Buffer length
  - Return:
      * 0, if no error
      * 1, if error
*/
int do_bic_update(ipmi_ctx_t ipmi_ctx, uint8_t *buff, uint32_t buff_len)
{
    if (!buff || !ipmi_ctx) {
        log_print(LOG_ERR, "%s: Empty space for buff/ipmi_ctx\n", __func__);
        return 1;
    }

    uint32_t cur_msg_offset = 0;
    uint8_t *cur_buff = buff;
    uint8_t last_cmd_flag = 0;
    uint32_t section_offset = 0;
    uint16_t section_idx = 0;
    uint8_t percent;
    uint16_t msg_len;

    while(cur_msg_offset < buff_len) {
        if (section_offset == CONFIG_SECTOR_SZ_64K) {
            section_offset = 0;
            section_idx++;
        }

        /* If current size over 64K */
        if ( (section_offset + CONFIG_MAX_IPMB_DATA_SIZE) / CONFIG_SECTOR_SZ_64K )
            msg_len = (CONFIG_SECTOR_SZ_64K - section_offset);
        else
            msg_len = CONFIG_MAX_IPMB_DATA_SIZE;

        /* If next msg offset over given img length */
        if ( (cur_msg_offset + msg_len) >= buff_len) {
            msg_len = (buff_len - cur_msg_offset);
            last_cmd_flag = 1;
        }

        fw_update_data_t cmd_data;
        memset(&cmd_data, 0, sizeof(cmd_data));
        if (last_cmd_flag)
            cmd_data.target = 0x82;
        else
            cmd_data.target = 0x02;
        cmd_data.offset[0] = (cur_msg_offset & 0xFF);
        cmd_data.offset[1] = (cur_msg_offset >> 8) & 0xFF;
        cmd_data.offset[2] = (cur_msg_offset >> 16) & 0xFF;
        cmd_data.offset[3] = (cur_msg_offset >> 24) & 0xFF;
        cmd_data.length[0] = msg_len & 0xFF;
        cmd_data.length[1] = (msg_len >> 8) & 0xFF;
        memcpy(cmd_data.data, cur_buff, msg_len);

        if ( percent != (cur_msg_offset+msg_len)*100/buff_len ) {
            percent = (cur_msg_offset+msg_len)*100/buff_len;
            if (!(percent % 5))
                log_print(LOG_NON, "         update status %d%%\n", percent);
        }

        ipmi_cmd_t msg_out;
        memset(&msg_out, 0, sizeof(msg_out));
        msg_out.netfn = FW_UPDATE_NETFN << 2;
        msg_out.cmd = OEM_CMD_FW_UPDATE;
        msg_out.data_len = msg_len+7;
        memcpy(msg_out.data, &cmd_data, msg_len+7);

        if (g_log_level >= 1) {
            log_print(LOG_DBG, "section_idx[%d] section_offset[0x%x/0x%x] image_offset[0x%x]\n",
                section_idx, section_offset, CONFIG_SECTOR_SZ_64K, cur_msg_offset);
            log_print(LOG_NON, "         target[0x%x] offset[0x%x] size[%d]\n",
                msg_out.data[0],
                msg_out.data[1]|(msg_out.data[2] << 8)|(msg_out.data[3] << 16)|(msg_out.data[4] << 24),
                msg_out.data[5]|(msg_out.data[6] << 8));
        }

        int resp_cc = send_recv_command(ipmi_ctx, &msg_out);
        if (resp_cc) {
            /* to handle unexpected user interrupt-behavior last time */
            if (resp_cc == CC_INVALID_DATA_FIELD) {
                log_print(LOG_WRN, "Given update offset not mach with previous record!\n");
                log_print(LOG_NON, "         Retry in few seconds...\n");
            }
            return 1;
        }

        cur_msg_offset += msg_len;
        cur_buff += msg_len;
        section_offset += msg_len;
    }

    return 0;
}
