#ifndef UTIL_COMMON_H
#define UTIL_COMMON_H

#include <stdint.h>
#include "proj_config.h"

typedef enum {
    LOG_INF = 0x01,
    LOG_DBG = 0x02,
    LOG_WRN = 0x04,
    LOG_ERR = 0x08,
    LOG_NON = 0xff
} LOG_TAG;

typedef enum time_format_type {
    TIME_FORMAT_FULL = 0,
    TIME_FORMAT_DATE,
    TIME_FORMAT_TIME,
    TIME_FORMAT_MAX
} tf_type_t;

void log_print(LOG_TAG level, const char *va_alist, ...);
void datetime_get(char *psDateTime, tf_type_t flag);
void log_record(char *file_path, char *content, int init_flag);
int check_version_info(char *ver, char *date);
uint32_t read_binary(const char *file_path, uint8_t *buff, uint32_t buff_len);

#endif
