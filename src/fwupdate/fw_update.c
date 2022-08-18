#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include "fw_update.h"

uint8_t force_update_flag = 0;

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
      * ipmi_ctx: ipmi-raw session
      * buff: Buffer to store image bytes
      * buff_len: Buffer length
  - Return:
      * 0, if no error
      * 1, if error
*/
int img_parsing_and_validate(ipmi_ctx_t ipmi_ctx, uint8_t *buff, uint32_t buff_len)
{
    int ret = 1;

    if (!buff || !ipmi_ctx) {
        log_print(LOG_ERR, "%s: Empty space for buff/img_info\n", __func__);
        goto exit;
    }

    if (buff_len <= CONFIG_BIC_SIGN_AREA_SIZE) {
        log_print(LOG_ERR, "%s: given buffer size %d lower than sign area size %d\n", __func__, buff_len, CONFIG_BIC_SIGN_AREA_SIZE);
        goto exit;
    }

    uint8_t key_byte[CONFIG_BIC_SIGN_AREA_SIZE];

    memcpy(&key_byte, &buff[buff_len-CONFIG_BIC_SIGN_AREA_SIZE], CONFIG_BIC_SIGN_AREA_SIZE);
    sign_info_t sign_info, tar_sign_info;
    board_info_t img_info, tar_img_info;

    memcpy(&sign_info.platform_name, &key_byte[16], sizeof(sign_info.platform_name));
    memcpy(&sign_info.version, &key_byte[32], sizeof(sign_info.platform_name));
    memcpy(&sign_info.board_info, &key_byte[45], sizeof(sign_info.platform_name));
    img_info.component = sign_info.board_info[1] & 0x7;
    img_info.stage = sign_info.board_info[0] & 0x0D;
    img_info.board_id = sign_info.board_info[0] & 0x1F;

    if (g_log_level >= 1) {
        log_print(LOG_NON, "         ------------- fw info ------------\n");
        log_print(LOG_NON, "         * plarform name  : ");
        for (int i=0; i<sizeof(sign_info.platform_name); i++) {
            log_print(LOG_NON, "%c", sign_info.platform_name[i]);
        }
        log_print(LOG_NON, "\n");
        log_print(LOG_NON, "         * fw version     : ");
        for (int i=0; i<sizeof(sign_info.version); i++) {
            log_print(LOG_NON, "%c", sign_info.version[i]);
        }
        log_print(LOG_NON, "\n");
        if (img_info.component >= MAX_IMG_COMP || !img_info.component)
            log_print(LOG_NON, "         * board component: unknown %d\n", img_info.component);
        else
            log_print(LOG_NON, "         * board component: %s\n", prj_comp_name[img_info.component]);
        log_print(LOG_NON, "         * board id       : %d\n", img_info.board_id);
        if (img_info.component >= MAX_STAGE)
            log_print(LOG_NON, "         * board stage    : unknown %d\n", img_info.stage);
        else
            log_print(LOG_NON, "         * board stage    : %s\n", prj_stage_name[img_info.stage]);
        log_print(LOG_NON, "         ------------- fw info ------------\n");
    }

    ipmi_cmd_t msg_out;
    int resp_cc;
    int validate_fail = 0;

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
            log_print(LOG_ERR, "Can't get firmware info %d from target device!\n", BIC_PROJ_STAGE);
            goto exit;
        }

        tar_img_info.stage = msg_out.data[0];
        if (tar_img_info.stage != img_info.stage) {
            log_print(LOG_ERR, "Fw info stage not mach!\n");
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
            log_print(LOG_ERR, "Can't get firmware info %d from target device!\n", BIC_PLAT_NAME);
            goto exit;
        }

        memcpy(&tar_sign_info.platform_name, &msg_out.data, msg_out.data_len);
        for (int i=0; i<msg_out.data_len; i++) {
            if (tar_sign_info.platform_name[i] != sign_info.platform_name[i]) {
                log_print(LOG_ERR, "Fw info platform name not mach!\n");
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
            log_print(LOG_ERR, "Can't get firmware info %d from target device!\n", BIC_PLAT_BOARD_ID);
            goto exit;
        }

        tar_img_info.board_id = msg_out.data[0];
        if (tar_img_info.board_id != img_info.board_id) {
            log_print(LOG_ERR, "Fw info board id not mach!\n");
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
      * buff: Buffer to store image bytes
      * buff_len: Buffer length
  - Return:
      * 0, if no error
      * 1, if error
*/
int do_bic_update(uint8_t *buff, uint32_t buff_len)
{
    int ret = 1;

    if (!buff) {
        log_print(LOG_ERR, "%s: Get empty inputs!\n", __func__);
        return 1;
    }

    ipmi_ctx_t ipmi_ctx = ipmi_ctx_create();
    if (ipmi_ctx == NULL) {
        log_print(LOG_ERR, "%s: ipmi_ctx_create error\n", __func__);
        return 1;
    }

    ipmi_ctx->type = IPMI_DEVICE_OPENIPMI;
    if (!(ipmi_ctx->io.inband.openipmi_ctx = ipmi_openipmi_ctx_create ())) {
        log_print(LOG_ERR, "%s: !(ipmi_ctx->io.inband.openipmi_ctx = ipmi_openipmi_ctx_create ())\n", __func__);
        goto clean;
    }

    if (ipmi_openipmi_ctx_io_init (ipmi_ctx->io.inband.openipmi_ctx) < 0) {
        log_print(LOG_ERR, "%s: ipmi_openipmi_ctx_io_init (ctx->io.inband.openipmi_ctx) < 0\n", __func__);
        goto clean;
    }

    if (!force_update_flag) {
        log_print(LOG_INF, "Parsing & Validate image sign key\n");
        if (img_parsing_and_validate(ipmi_ctx, buff, buff_len)) {
            log_print(LOG_ERR, "%s: There's an error while parsing fw info\n", __func__);
            goto clean;
        }
        log_print(LOG_INF, "PASS!\n\n");
    }

    uint32_t cur_msg_offset = 0;
    uint8_t *cur_buff = buff;
    uint8_t last_cmd_flag = 0;
    uint32_t section_offset = 0;
    uint16_t section_idx = 0;
    uint8_t percent;

    uint16_t msg_len;
    if (buff_len > CONFIG_MAX_IPMB_DATA_SIZE) {
        msg_len = CONFIG_MAX_IPMB_DATA_SIZE;
    } else {
        msg_len = buff_len;
        last_cmd_flag = 1;
    }

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
            goto clean;
        }

        cur_msg_offset += msg_len;
        cur_buff += msg_len;
        section_offset += msg_len;
    }
    ret = 0;

clean:
    ipmi_ctx_close (ipmi_ctx);
    ipmi_ctx_destroy (ipmi_ctx);
    return ret;
}

/*
  - Name: fw_update
  - Description: Firmware update controller
  - Input:
      * flag: Image type flag
      * buff: Buffer to store image bytes
      * buff_len: Buffer length
      * max_retry: Maximum retry time
  - Return:
      * 0, if no error
      * 1, if error
*/
int fw_update(fw_type_t flag, uint8_t *buff, uint32_t buff_len, int max_retry)
{
    if (!buff) {
        log_print(LOG_ERR, "%s: Get empty inputs!\n", __func__);
        return 1;
    }

    int ret = 1;
    int retry = 0;

    while (retry <= max_retry) {
        if (retry) {
            log_print(LOG_NON, "\n");
            log_print(LOG_INF, "FW update retry %d/%d ...\n", retry, max_retry);
        }

        switch(flag)
        {
        case FW_T_BIC:
            if (do_bic_update(buff, buff_len) == 0)
                ret = 0;
            break;

        default:
            log_print(LOG_ERR, "%s: No such flag!\n", __func__);
            break;
        }

        if (!ret)
            break;

        retry++;
    }

    return ret;
}
