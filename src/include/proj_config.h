#ifndef PROJ_CONFIG_H
#define PROJ_CONFIG_H

/* ===============================================
    PROJECT CONFIGS
   =============================================== */
#define PROJ_NAME "FW UPDATE TOOL"
#define PROJ_DESCRIPTION "Firmware update tool from host, including [BIC]."
#ifndef PROJ_VERSION
    #define PROJ_VERSION "none"
#endif
#ifndef PROJ_DATE
    #define PROJ_DATE "none"
#endif
#define PROJ_LOG_FILE "./log.txt"

/* ===============================================
    OTHER CONFIGS
   =============================================== */
/* FW update rule config */
#define MAX_IMG_LENGTH 0x80000
#define SECTOR_SZ_64K 0x10000

/* Plock file config */
#define PLOCK_FILE "/var/run/HOST_FW_updating"

/* ===============================================
    GLOBAL CONFIGS AND VARIABLES
   =============================================== */
/* Log config */
#define CONFIG_MAX_LOG_LEVEL 3
int g_log_level;

/* IPMB relative config */
#define CONFIG_MAX_IPMB_SIZE 244
#define CONFIG_MAX_IPMB_DATA_SIZE 224

/* QUANTA oem command relative config */
#define CONFIG_OEM_38 0x38
#define CONFIG_OEM_36 0x36
#define CONFIG_IANA_1 0x9C
#define CONFIG_IANA_2 0x9C
#define CONFIG_IANA_3 0x00

#endif