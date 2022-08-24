#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "proj_config.h"
#include "fw_update.h"
#include "util_common.h"
#include "util_freeipmi.h"
#include "util_plock.h"

static uint8_t force_update_flag;

static void HELP()
{
    log_print(LOG_NON, "\n");
    log_print(LOG_NON, "Try: ./host_update -i <img_path> [-t <fw_type>] [-I <iana_type>] [-f] [-v]\n");
    log_print(LOG_NON, "     -h               Help\n");
    log_print(LOG_NON, "     -t <fw_type>     Firmware type [0]BIC(default)\n");
    log_print(LOG_NON, "     -I <iana_type>   IANA [0]0x00a015(default) [1]0x009c9c\n");
    log_print(LOG_NON, "     -i <img_path>    Image path\n");
    log_print(LOG_NON, "     -f               (optional) Force update flag [-f]without validate\n");
    log_print(LOG_NON, "     -v               (optional) Log level [-v]L1 [-vv]L2 [-vvv]L3\n\n");
    log_print(LOG_NON, "     Example: Update BIC with ./host_update -i bic_img.bin\n\n");
}

static int HEADER_PRINT()
{
    if ( check_version_info(CONFIG_PROJ_VERSION, CONFIG_PROJ_DATE) )
        return 1;
    log_print(LOG_NON, "===============================================================================\n");
    log_print(LOG_NON, "* Name         : %s\n", CONFIG_PROJ_NAME);
    log_print(LOG_NON, "* Description  : %s\n", CONFIG_PROJ_DESCRIPTION);
    log_print(LOG_NON, "* Ver/Date     : %s/%s\n", CONFIG_PROJ_VERSION, CONFIG_PROJ_DATE);
    log_print(LOG_NON, "* Note         : %s\n", "none");
    log_print(LOG_NON, "===============================================================================\n");
    return 0;
}

int main(int argc, char * const argv[])
{
    ipmi_ctx_t ipmi_ctx = NULL;
    uint8_t *img_buff = NULL;
    g_log_level = 0;

    int plock_fd = -1;
    if ((plock_fd = init_process_lock_file(CONFIG_PLOCK_FILE)) == -1) {
        log_print(LOG_ERR, "Failed to create %s: %s\n", CONFIG_PLOCK_FILE, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (lock_plock_file(plock_fd)) {
        log_print(LOG_ERR, "BIC update tool is processing.\n");
        if (close_process_lock_file(plock_fd))
            log_print(LOG_WRN, "Can't close %s: %s\n", CONFIG_PLOCK_FILE, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if ( HEADER_PRINT() )
        log_print(LOG_WRN, "Skip HEADER due to some reason...\n");

    int img_idx = 0;
    int iana_idx = 0;
    char *img_path = NULL;

    int key;
    while ((key = getopt (argc, argv, "hvft:i:I:")) != -1) {
        switch (key)
        {
        case 'h':
            HELP();
            goto ending;

        case 'v':
            g_log_level++;
            break;

        case 'f':
            force_update_flag = 1;
            break;

        case 't':
            if (str_is_key(optarg)) {
                log_print(LOG_ERR, "Lost -t argument!\n");
                HELP();
                goto ending;
            }
            if (!str_is_digit(optarg)) {
                log_print(LOG_ERR, "Invalid -t argument!\n");
                HELP();
                goto ending;
            }
            img_idx = atoi(optarg);
            break;

        case 'i':
            if (str_is_key(optarg)) {
                log_print(LOG_ERR, "Lost -i argument!\n");
                HELP();
                goto ending;
            }
            img_path = optarg;
            break;

        case 'I':
            if (str_is_key(optarg)) {
                log_print(LOG_ERR, "Lost -I argument!\n");
                HELP();
                goto ending;
            }
            if (!str_is_digit(optarg)) {
                log_print(LOG_ERR, "Invalid -I argument!\n");
                HELP();
                goto ending;
            }
            iana_idx = atoi(optarg);
            break;

        case '?':
            HELP();
            goto ending;

        default:
            break;
        }
    }

    if ( (img_idx >= FW_T_MAX_IDX) || (img_idx < 0) ) {
        log_print(LOG_ERR, "Invalid <fw_type>!\n");
        HELP();
        goto ending;
    }

    if (switch_global_iana(iana_idx)) {
        log_print(LOG_ERR, "Invalid <iana_type>!\n");
        HELP();
        goto ending;
    }

    if (!img_path) {
        log_print(LOG_ERR, "Lost <img_path>!\n");
        HELP();
        goto ending;
    }

    if (force_update_flag)
        log_print(LOG_WRN, "Force update without validate sign-key.\n");

    if (g_log_level > CONFIG_MAX_LOG_LEVEL)
        g_log_level = CONFIG_MAX_LOG_LEVEL;
    if (g_log_level)
        log_print(LOG_INF, "Log level %d...\n\n", g_log_level);

    log_print(LOG_INF, "Start [%s] update task with image [%s]\n", IMG_TYPE_LST[img_idx], img_path);
    img_buff = malloc(sizeof(uint8_t) * CONFIG_MAX_IMG_LENGTH);
    if (!img_buff) {
        log_print(LOG_ERR, "img_buff malloc failed!\n");
        goto ending;
    }

    /* STEP1 - Read image */
    log_print(LOG_NON, "\n");
    log_print(LOG_INF, "STEP1. Read image\n");
    uint32_t img_size = read_binary(img_path, img_buff, CONFIG_MAX_IMG_LENGTH);
    if (!img_size) {
        log_print(LOG_NON, "\n");
        log_print(LOG_INF, "Update failed!\n");
        goto ending;
    }
    log_print(LOG_INF, "PASS!\n");

    /* STEP2 - Create session */
    log_print(LOG_NON, "\n");
    log_print(LOG_INF, "STEP2. Create free-ipmi session\n");
    ipmi_ctx = freeipmi_session_create(ipmi_ctx);
    if (!ipmi_ctx) {
        log_print(LOG_ERR, "There's an error while Creating free ipmi-session!\n");
        goto ending;
    }
    log_print(LOG_INF, "PASS!\n");

    /* STEP3 - Verify image */
    log_print(LOG_NON, "\n");
    log_print(LOG_INF, "STEP3. Verify image\n");
    if (!force_update_flag) {
        if ( img_parsing_and_validate(ipmi_ctx, img_buff, img_size, img_idx) ) {
            log_print(LOG_NON, "\n");
            log_print(LOG_INF, "Update failed!\n");
            goto ending;
        }
        log_print(LOG_INF, "PASS!\n");
    } else {
        log_print(LOG_INF, "skip!\n");
    }

    /* STEP4 - Upload image */
    log_print(LOG_NON, "\n");
    log_print(LOG_INF, "STEP4. Upload image\n");
    if ( fw_update(ipmi_ctx, img_buff, img_size, img_idx, IPMI_RAW_RETRY) ) {
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

    if (ipmi_ctx) {
        if (freeipmi_session_abort(ipmi_ctx))
            log_print(LOG_ERR, "Can't abort free-ipmi session\n");
    }

    if (unlock_plock_file(plock_fd))
        log_print(LOG_WRN, "Can't unlock %s: %s\n", CONFIG_PLOCK_FILE, strerror(errno));

    if (close_process_lock_file(plock_fd))
        log_print(LOG_WRN, "Can't close %s: %s\n", CONFIG_PLOCK_FILE, strerror(errno));

    if (remove_process_lock_file(CONFIG_PLOCK_FILE)) {
        log_print(LOG_ERR,
        "Can't remove %s: %s\n"
        "Please execute this command: \'rm %s\'\n",
        CONFIG_PLOCK_FILE, strerror(errno), CONFIG_PLOCK_FILE);
    }

    return 0;
}
