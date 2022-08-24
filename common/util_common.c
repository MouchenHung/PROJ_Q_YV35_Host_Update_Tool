#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include "util_common.h"

/*
  - Name: log_print
  - Description: Print message with header
  - Input:
      * level: Level of message
      * va_alist: Format of message
      * ...: Add args if needed in format
  - Return:
      * none
*/
void log_print(LOG_TAG level, const char *va_alist, ...)
{
    PARM_CHECK(va_alist, , __func__)

    va_list ap;
    switch (level)
    {
    case LOG_INF:
        printf("<system> ");
        break;
    case LOG_DBG:
        printf("<debug>  ");
        break;
    case LOG_WRN:
        printf("<warn>   ");
        break;
    case LOG_ERR:
        printf("<error>  ");
        break;
    default:
        break;
    }
    va_start(ap, va_alist);
    vfprintf(stdout, va_alist, ap);
    va_end(ap);
    return;
}

/*
  - Name: datetime_get
  - Description: Get current timestamp
  - Input:
      * psDateTime: Buffer to read back time string
      * flag: Format of time
        * [TIME_FORMAT_FULL] YYYY.MM.DD - hh:mm:ss
        * [TIME_FORMAT_DATE] YYYY.MM.DD
        * [TIME_FORMAT_TIME] hh:mm:ss
  - Return:
      * none
*/
void datetime_get(char *psDateTime, tf_type_t flag)
{
    PARM_CHECK(psDateTime, , __func__)

    time_t nSeconds;
    struct tm *pTM = NULL;

    time(&nSeconds);
    pTM = localtime(&nSeconds);

    switch (flag)
    {
    case TIME_FORMAT_FULL:
        sprintf(psDateTime, "%04d.%02d.%02d - %02d:%02d:%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday,
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);
        break;
    
    case TIME_FORMAT_DATE:
        sprintf(psDateTime, "%04d.%02d.%02d",
            pTM->tm_year + 1900, pTM->tm_mon + 1, pTM->tm_mday);
        break;

    case TIME_FORMAT_TIME:
        sprintf(psDateTime, "%02d:%02d:%02d",
            pTM->tm_hour, pTM->tm_min, pTM->tm_sec);
        break;
    
    default:
        log_print(LOG_ERR, "%s: No such flag!\n", __func__);
        break;
    }

    return;
}

/*
  - Name: log_record
  - Description: Record log to file
  - Input:
      * file_path: ipmi-raw session
      * content: IPMI package
      * init_flag: 0 if append, 1 if create/rewrite
  - Return:
      * none
*/
void log_record(char *file_path, char *content, int init_flag)
{
    PARM_CHECK(file_path, , __func__)
    PARM_CHECK(content, , __func__)

    uint32_t content_size = 0;
    char *tmp = content;
    while(*tmp) {
        content_size++;
        tmp++;
    }

    FILE *ptr;
    if (init_flag)
        ptr = fopen(file_path, "w");
    else
        ptr = fopen(file_path, "a");

    if (!ptr) {
        log_print(LOG_ERR, "%s: Invalid log file path [%s]\n", __func__, file_path);
        return;
    }

    log_print(LOG_NON, "%s\n", content);

    char cur_time[22];
    datetime_get(cur_time, TIME_FORMAT_FULL);

    char output[content_size+22];
    sprintf(output, "[%s] %s", cur_time, content);

    fwrite(output, 1, sizeof(output), ptr);

    fclose(ptr);

    return;
}

/*
  - Name: check_version_info
  - Description: Check release version and date format from CMakeList.
  - Input:
      * ver: Version buffer
      * date: Date buffer
  - Return:
      * 1, if error
      * 0, if no error
  - Note:
      * Version format:
        ------------
        vx.x.x
        YYYY.MM.DD
        ------------
        * 'V' and 'v' are both accept for version
        * '/' and '.' are both accept for date
*/
int check_version_info(char *ver, char *date) {
    PARM_CHECK(ver, 1, __func__)
    PARM_CHECK(date, 1, __func__)

    int i = 0;
    while (ver[i]!='\0') {
        i++;
    }
    if (i != 6) {
        log_print(LOG_WRN, "%s: Version string over length!\n", __func__);
        goto error;
    }
    
    i = 0;
    while (date[i]!='\0') {
        i++;
    }
    if (i != 10) {
        log_print(LOG_WRN, "%s: Date string over length!\n", __func__);
        goto error;
    }

    if (ver[0] != 'v' && ver[0] != 'V')
        goto error;
    if (ver[2] != '.' || ver[4] != '.')
        goto error;
    if ( (date[4] != date[7]) ||
            (date[4] != '.' && date[7] != '/') )
        goto error;

    return 0;
error:
    log_print(LOG_WRN, "%s: Invalid version format!\n", __func__);
    log_print(LOG_NON, "         <version format>:\n");
    log_print(LOG_NON, "         vx.x.x\n");
    log_print(LOG_NON, "         YYYY.MM.DD\n\n");

    return 1;
}

/*
  - Name: read_binary
  - Description: Read binary file to buffer
  - Input:
      * file_path: Binary file path
      * buff: Buffer to read back image bytes
      * buff_len: Buffer length
  - Return:
      * Binary file size, if no error
      * 0, if error
  - Note: buff_len should >= given image length
*/
uint32_t read_binary(const char *file_path, uint8_t *buff, uint32_t buff_len)
{
    PARM_CHECK(file_path, 0, __func__)
    PARM_CHECK(buff, 0, __func__)

    FILE *ptr;
    uint32_t bin_size = 0;

    ptr = fopen(file_path,"rb");
    if (!ptr) {
        log_print(LOG_ERR, "%s: Invalid bin file [%s]\n", __func__, file_path);
        return 0;
    }

    fseek(ptr, 0, SEEK_END);
    bin_size = ftell(ptr);
    fseek(ptr, 0, SEEK_SET);

    if (bin_size > buff_len) {
        log_print(LOG_ERR, "%s: Given buffer length (0x%x) smaller than Image length (0x%x)\n",
            __func__, buff_len, bin_size);
        bin_size = 0;
        goto ending;
    }

    int read_size = fread(buff, sizeof(char), bin_size, ptr);
    if (read_size != bin_size) {
        log_print(LOG_ERR, "%s: There's an error occur while reading image\n", __func__);
        bin_size = 0;
        goto ending;
    }

    if (g_log_level >= 1)
        log_print(LOG_INF, "Image size: 0x%x\n", bin_size);

ending:
    fclose(ptr);
    return bin_size;
}

/*
  - Name: str_is_digit
  - Description: Whether string is a number
  - Input:
      * str: string pointer
  - Return:
      * 1: is digit
      * 0: not digit
*/
int str_is_digit(char *str)
{
    PARM_CHECK(str, 0, __func__)

    int i=0;
    while (str[i] != '\0')
    {
        if (!isdigit(str[i]))
            return 0;
        i++;
    }
    return 1;
}

/*
  - Name: str_is_key
  - Description: Whether first char of string is '-'
  - Input:
      * str: string pointer
  - Return:
      * 1: is key
      * 0: not key
*/
int str_is_key(char *str)
{
    PARM_CHECK(str, 0, __func__)

    if (str[0] == '-')
        return 1;
    return 0;
}
