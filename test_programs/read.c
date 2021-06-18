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

char read_data_buffer[1024];


int read_lock_data(int fd) {
    int rc = -1;
    off_t file_offset;
    ssize_t bytes_read;
    char *first_comma = NULL;
    char *second_comma = NULL;
    time_t rawtime;
    struct tm *timeinfo;
    char time_buffer[26];

    file_offset = lseek(fd, 0, SEEK_SET);
    if (file_offset != (off_t)-1) {
        bytes_read = read(fd, read_data_buffer, 1024);
        if (bytes_read != -1) {
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", timeinfo);
            printf("%s - data read: \"%s\"\n", time_buffer, read_data_buffer);
            first_comma = strchr(read_data_buffer, ',');
            if (first_comma != NULL) {
                *first_comma = '\0';
                strcpy(read_data.host_name, read_data_buffer);
                second_comma = strchr(first_comma + 1, ',');
                if (second_comma != NULL) {
                    *second_comma = '\0';
                    sscanf(first_comma + 1, "%ld\n", &(read_data.lock_id));
                    sscanf(second_comma + 1, "%ld\n", &(read_data.lock_time));
                    rc = 0;
                } else {
                    fprintf(stderr, "Data read did not contain second comma\n");
                }
            } else {
                fprintf(stderr, "Data read did not contain a comma\n");
            }
        } else {
            perror("read");
        }
    } else {
        perror("lseek");
    }

    return rc;
}