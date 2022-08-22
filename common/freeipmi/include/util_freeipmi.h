#ifndef UTIL_FREEIPMI_H
#define UTIL_FREEIPMI_H

#include <freeipmi/api/ipmi-api.h>
#include <freeipmi/driver/ipmi-openipmi-driver.h>
#include "ipmi-api-defs.h"
#include "proj_config.h"
#include "util_common.h"

#define IPMI_RAW_MAX_ARGS 512

/* IPMI CC */
enum {
    CC_SUCCESS = 0x00,
    CC_INVALID_PARAM = 0x80,
    CC_FRU_DEV_BUSY = 0x81,
    CC_BRIDGE_MSG_ERR = 0x82,
    CC_I2C_BUS_ERROR = 0x83,
    CC_INVALID_IANA = 0x84,
    CC_NODE_BUSY = 0xC0,
    CC_INVALID_CMD = 0xC1,
    CC_INVALID_LUN = 0xC2,
    CC_TIMEOUT = 0xC3,
    CC_OUT_OF_SPACE = 0xC4,
    CC_INVALID_RESERVATION = 0xC5,
    CC_DATA_TRUNCATED = 0xC6,
    CC_INVALID_LENGTH = 0xC7,
    CC_LENGTH_EXCEEDED = 0xC8,
    CC_PARAM_OUT_OF_RANGE = 0xC9,
    CC_SENSOR_NOT_PRESENT = 0xCB,
    CC_INVALID_DATA_FIELD = 0xCC,
    CC_CAN_NOT_RESPOND = 0xCE,
    CC_NOT_SUPP_IN_CURR_STATE = 0xD5,
    CC_UNSPECIFIED_ERROR = 0xFF,
};

typedef enum {
    IANA_15A000 = 0,
    IANA_9C9C00,
    IANA_MAX
} iana_type_t;

typedef struct fw_update_data {
    uint8_t target;
    uint8_t offset[4];
    uint8_t length[2];
    uint8_t data[CONFIG_MAX_IPMB_DATA_SIZE];
} fw_update_data_t;

typedef struct ipmi_cmd {
    uint8_t netfn; /* include LUN */
    uint8_t cmd;
    uint8_t data[CONFIG_MAX_IPMB_SIZE];
    uint32_t data_len;
} ipmi_cmd_t;

extern uint8_t GLOBAL_IANA[3];

ipmi_ctx_t freeipmi_session_create();
int freeipmi_session_abort(ipmi_ctx_t ipmi_ctx);
int switch_global_iana(iana_type_t idx);
int send_recv_command(ipmi_ctx_t ipmi_ctx, ipmi_cmd_t *msg);

#endif
