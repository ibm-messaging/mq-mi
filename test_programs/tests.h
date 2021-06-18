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

#include <limits.h>
#include <time.h>

typedef struct {
    char          host_name[HOST_NAME_MAX + 1];
    long int      lock_id;
    time_t        lock_time;
} lock_data;

lock_data read_data, write_data;

extern char *current_time();
extern int read_lock_data(int);
extern int write_lock_data(int);