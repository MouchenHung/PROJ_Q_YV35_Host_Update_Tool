#ifndef PROJ_CONFIG_H
#define PROJ_CONFIG_H

/* ===============================================
    PROJECT CONFIGS
   =============================================== */
#define CONFIG_PROJ_NAME "FW UPDATE TOOL"
#define CONFIG_PROJ_DESCRIPTION "Firmware update tool from host, including [BIC]."
#ifndef CONFIG_PROJ_VERSION
    #define CONFIG_PROJ_VERSION "none"
#endif
#ifndef CONFIG_PROJ_DATE
    #define CONFIG_PROJ_DATE "none"
#endif
#define CONFIG_PROJ_LOG_FILE "./log.txt"

/* ===============================================
    OTHER CONFIGS
   =============================================== */
/* FW update rule config */
#define CONFIG_MAX_IMG_LENGTH 0x80000
#define CONFIG_SECTOR_SZ_64K 0x10000

/* Plock file config */
#define CONFIG_PLOCK_FILE "/var/run/HOST_FW_updating"

/* ===============================================
    GLOBAL CONFIGS AND VARIABLES
   =============================================== */
/* Log config */
#define CONFIG_MAX_LOG_LEVEL 3
extern int g_log_level;

/* ipmi-raw config */
#define CONFIG_MAX_DATA_DBG_PRINT 10 //how much ipmi data bytes could display if using -vvv

/* IPMB relative config */
#define CONFIG_MAX_IPMB_SIZE 244
#define CONFIG_MAX_IPMB_DATA_SIZE 224

/* QUANTA oem command relative config */
#define CONFIG_OEM_36 0x36
#define CONFIG_OEM_38 0x38
#define CONFIG_IANA_0 0x00A015
#define CONFIG_IANA_1 0x009C9C
#define CONFIG_IANA_DEFAULT CONFIG_IANA_0

/* Sign image check config */
#define CONFIG_BIC_SIGN_AREA_SIZE 64 //(byte)
#define CONFIG_PROJ_STAGE_CHECK 0
#define CONFIG_PLAT_NAME_CHECK 1
#define CONFIG_BOARD_ID_CHECK 1
#define CONFIG_FW_COMPO_CHECK 1

#endif
