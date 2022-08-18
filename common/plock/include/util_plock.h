#ifndef UTIL_PLOCK_H
#define UTIL_PLOCK_H

#include <stdint.h>
#include <sys/file.h>
#include <sys/stat.h>
#include "proj_config.h"
#include "util_common.h"

int init_process_lock_file(char *file_path);
int remove_process_lock_file(char *file_path);
int close_process_lock_file(int fd);
int lock_plock_file(int fd);
int unlock_plock_file(int fd);

#endif
