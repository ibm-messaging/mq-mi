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

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tests.h"

int main(int argc, char *argv[]) {
    int rc = 0;
    char *directory = NULL;
    char directory_last_char = '\0';
    char *lock_file_name = "test1_master";
    size_t lock_file_path_len = 0;
    char *lock_file_path = NULL;
    int flags = O_CLOEXEC | O_CREAT | O_RDWR;
    int mode = S_IRWXU | S_IRWXG;
    int fd = -1;
    struct flock lockdetails;
    int status = -1;
    unsigned int sleep_time_remaining = 0;
    int continue_sleeping = 1;

    if ((argc < 2) || (argc > 3)) {
        fprintf(stderr, "usage: test1 <path to shared directory>[ 1|2|3]\n");
    } else {
        if (argc == 3) {
            switch (atoi(argv[2])) {
                case 1: flags |= O_SYNC;
                         break;
                case 2: flags |= O_DIRECT;
                         break;
                case 3: flags |= O_DIRECT | O_SYNC;
                         break;
                default: fprintf(stderr, "usage: test1 <path to shared directory>[ 1|2|3]\n");
                         flags = -1;
                         break;
            }
        }
        if (flags != -1) {
            rc = gethostname(write_data.host_name, HOST_NAME_MAX);
            if (rc == 0) {
                directory = argv[1];
                directory_last_char = directory[strlen(directory) - 1];
                lock_file_path_len = strlen(directory) + strlen(lock_file_name) + 2; // In case directory does not end in '/'
                lock_file_path = (char *)malloc(lock_file_path_len);
                if (lock_file_path == NULL) {
                    perror("malloc");
                } else {
                    strcpy(lock_file_path, directory);
                    if (directory_last_char == '/') {
                        strcat(lock_file_path, lock_file_name);
                    } else {
                        strcat(lock_file_path, "/");
                        strcat(lock_file_path, lock_file_name);
                    }
                    fd = open(lock_file_path, flags, mode);
                    if (fd == -1) {
                        perror("open");
                    } else {
                        lockdetails.l_type = F_WRLCK; // Exclusive lock
                        lockdetails.l_whence = SEEK_SET;
                        lockdetails.l_start = 0;
                        lockdetails.l_len = 0;
                        printf("About to attempt to get lock %s on host %s\n",
                               lock_file_path,
                               write_data.host_name);
                        rc = fcntl(fd, F_SETLKW, &lockdetails);
                        if (rc == 0) {
                            status = 0;
                            printf("Have got the lock at %s\n", current_time());
                            write_data.lock_time = time(NULL);
                            srandom(write_data.lock_time);
                            write_data.lock_id = random();
                            rc = write_lock_data(fd);
                            while (continue_sleeping == 1) {
                                sleep_time_remaining = sleep(10);
                                if (sleep_time_remaining == 0) {
                                    rc = read_lock_data(fd);
                                    if (rc == 0) {
                                        if (strcmp(write_data.host_name, read_data.host_name) != 0) {
                                            fprintf(stderr,
                                                    "Read different host name \"%s\"\n",
                                                    read_data.host_name);
                                            continue_sleeping = 0;
                                        } else if (write_data.lock_id != read_data.lock_id) {
                                            fprintf(stderr,
                                                    "Read different lock id \"%ld\"\n",
                                                    read_data.lock_id);
                                            continue_sleeping = 0;
                                        } else if (write_data.lock_time != read_data.lock_time) {
                                            fprintf(stderr,
                                                    "Read different lock time \"%ld\"\n",
                                                    read_data.lock_time);
                                            continue_sleeping = 0;
                                        }
                                    } else {
                                        continue_sleeping = 0;
                                    }
                                } else {
                                    printf("sleep interrupted\n");
                                    continue_sleeping = 0;
                                }
                            }
                        } else {
                            perror("fcntl");
                        }
                        rc = close(fd);
                        if (rc == -1) {
                            perror("close");
                        }
                    }
                    free(lock_file_path);
                }
            } else {
                perror("gethostname");
            }
        }
    }
    exit(status);
}