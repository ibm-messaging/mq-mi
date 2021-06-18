/*
 * (C) Copyright IBM Corporation 2021
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sys/types.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "tests.h"

char write_data_buffer[1024];

int write_lock_data(int fd) {
    int rc = -1;
    off_t file_offset;
    int len = -1;

    sprintf(write_data_buffer,
            "%s,%ld,%ld",
            write_data.host_name,
            write_data.lock_id,
            write_data.lock_time);
    len = strlen(write_data_buffer);
    file_offset = lseek(fd, 0, SEEK_SET);
    if (file_offset != (off_t)-1) {
        rc = write(fd, write_data_buffer, len);
        if (rc != -1) {
            rc = ftruncate(fd, len);
            if (rc != -1) {
                rc = fsync(fd);
                if (rc != -1) {
                    printf("Have written lock data \"%s\"\n", write_data_buffer);
                } else {
                    perror("fsync");
                }
            } else {
                perror("ftruncate");
            }
        } else {
            perror("write");
        }
    } else {
        perror("lseek");
    }

    return rc;
}