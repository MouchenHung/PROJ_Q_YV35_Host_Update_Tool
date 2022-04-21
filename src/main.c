#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "proj_config.h"
#include "fw_update.h"
#include "util_common.h"
#include "util_ipmiraw.h"
#include "util_plock.h"

static void HELP()
{
    log_print(LOG_NON, "Try: ./host_fw_update <fw_type> <img_path> <log_level>\n");
    log_print(LOG_NON, "     <fw_type>   Firmware type [0]BIC\n");
    log_print(LOG_NON, "     <img_path>  Image path\n");
    log_print(LOG_NON, "     <log_level> (optional) Log level [-v]L1 [-vv]L2 [-vvv]L3\n\n");
}

static int HEADER_PRINT()
{
    if ( check_version_info(PROJ_VERSION, PROJ_DATE) )
        return 1;
    log_print(LOG_NON, "===============================================================================\n");
    log_print(LOG_NON, "* Name         : %s\n", PROJ_NAME);
    log_print(LOG_NON, "* Description  : %s\n", PROJ_DESCRIPTION);
    log_print(LOG_NON, "* Ver/Date     : %s/%s\n", PROJ_VERSION, PROJ_DATE);
    log_print(LOG_NON, "* Note         : %s\n", "none");
    log_print(LOG_NON, "===============================================================================\n");
    return 0;
}

int main(int argc, char * const argv[])
{
    uint8_t *img_buff = NULL;
    g_log_level = 0;

    int plock_fd = -1;
    if ((plock_fd = init_process_lock_file(PLOCK_FILE)) == -1) {
        log_print(LOG_ERR, "Failed to create %s: %s\n", PLOCK_FILE, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (lock_plock_file(plock_fd)) {
        log_print(LOG_ERR, "BIC update tool is processing.\n");
        exit(EXIT_FAILURE);
    }

    if ( HEADER_PRINT() )
        log_print(LOG_WRN, "Skip HEADER due to some reason...\n");

    if (argc!=3 && argc!=4) {
        HELP();
        goto ending;
    }

    int img_idx = atoi(argv[1]);
    char *img_path = argv[2];

    if ( (img_idx >= FW_T_MAX_IDX) || (img_idx < 0) ) {
        log_print(LOG_ERR, "Invalid <fw_type>!\n");
        HELP();
        goto ending;
    }

    int key;
    while ((key = getopt (argc, argv, "v")) != -1) {
        switch (key)
        {
        case 'v':
            g_log_level++;
            if (g_log_level > CONFIG_MAX_LOG_LEVEL) {
                log_print(LOG_WRN, "Log level over limit, use L3 instead!\n");
                g_log_level = CONFIG_MAX_LOG_LEVEL;
            }
            break;

        case '?':
            log_print(LOG_WRN, "Unknown option `-%c'.\n", optopt);
            HELP();
            goto ending;

        default:
            break;
        }
    }

    if (g_log_level)
        log_print(LOG_INF, "Log level %d...\n\n", g_log_level);

    log_print(LOG_INF, "Start [%s] update task with image [%s]\n", IMG_TYPE_LST[img_idx], img_path);

    img_buff = malloc(sizeof(uint8_t) * MAX_IMG_LENGTH);
    if (!img_buff) {
        log_print(LOG_ERR, "img_buff malloc failed!\n");
        goto ending;
    }

    /* STEP1 - Read image */
    log_print(LOG_NON, "\n");
    log_print(LOG_INF, "STEP1. Read image\n");
    uint32_t img_size = read_binary(img_path, img_buff, MAX_IMG_LENGTH);
    if (!img_size) {
        log_print(LOG_NON, "\n");
        log_print(LOG_INF, "Update failed!\n");
        goto ending;
    }
    log_print(LOG_INF, "PASS!\n");

    /* STEP2 - Upload image */
    log_print(LOG_NON, "\n");
    log_print(LOG_INF, "STEP2. Upload image\n");
    if ( fw_update(img_idx, img_buff, img_size, IPMI_RAW_RETRY) ) {
        log_print(LOG_NON, "\n");
        log_print(LOG_INF, "Update failed!\n");
        goto ending;
    }
    log_print(LOG_INF, "PASS!\n");

    log_print(LOG_NON, "\n");
    log_print(LOG_INF, "Update complete!\n");

ending:
    if (img_buff)
        free(img_buff);

    if (unlock_plock_file(plock_fd))
        log_print(LOG_WRN, "Can't unlock %s: %s\n", PLOCK_FILE, strerror(errno));

    close(plock_fd);

    if (remove(PLOCK_FILE)) {
        log_print(LOG_ERR,
        "Can't remove %s: %s\n"
        "Please execute this command: \'rm %s\'\n",
        PLOCK_FILE, strerror(errno), PLOCK_FILE);
    }

    return 0;
}
