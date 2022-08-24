#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "util_plock.h"

/*
  - Name: init_process_lock_file
  - Description: Create the plock, a file for checking BIC FW updating.
  - Input:
      * file_path: Plock file
  - Return:
      * a file descriptor of the checking file, if no error
      * -1, if error
*/
int init_process_lock_file(char *file_path)
{
    PARM_CHECK(file_path, -1, __func__)
    return open(file_path, O_RDONLY | O_CREAT);
}

/*
  - Name: remove_process_lock_file
  - Description: Remove the plock, a file for checking BIC FW updating.
  - Input:
      * file_path: Plock file
  - Return:
      * 0, if the plock file is removed by this process.
      * -1, if error
*/
int remove_process_lock_file(char *file_path)
{
    PARM_CHECK(file_path, -1, __func__)
    return remove(file_path);
}

/*
  - Name: close_process_lock_file
  - Description: Close the plock.
  - Input:
      * fd: The file descriptor of plock.
  - Return:
      * 0, if the plock file is closed by this process.
      * -1, if error
*/
int close_process_lock_file(int fd)
{
    return close(fd);
}

/*
  - Name: lock_plock_file
  - Description: Lock the plock_file.
  - Input:
      * fd: The file descriptor of plock.
  - Return:
      * 0, if the plock file is locked by this process.
      * -1, if the plock file is locked by the other process.
*/
int lock_plock_file(int fd)
{
    if (fd <= 0)
        return -1;

    return flock(fd, LOCK_EX | LOCK_NB);
}

/*
  - Name: unlock_plock_file
  - Description: Unlock the plock_file.
  - Input:
      * fd: The file descriptor of plock.
  - Return:
      * 0, if the plock file is unlocked by this process.
      * -1, if error.
*/
int unlock_plock_file(int fd)
{
    if (fd <= 0)
        return -1;

    return flock(fd, LOCK_UN);
}
