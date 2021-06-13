/*
 * Copyright 2014-2021 Real Logic Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AERON_SPSC_RB_H
#define AERON_SPSC_RB_H

#include "concurrent/aeron_rb.h"

#if !defined(_MSC_VER)
#include <sys/uio.h>
#else
struct iovec
{
    void  *iov_base;
    size_t iov_len;
};
#endif

typedef struct aeron_spsc_rb_stct
{
    uint8_t *buffer;
    aeron_rb_descriptor_t *descriptor;
    size_t capacity;
    size_t max_message_length;
}
aeron_spsc_rb_t;

int aeron_spsc_rb_init(aeron_spsc_rb_t *ring_buffer, void *buffer, size_t length);

aeron_rb_write_result_t aeron_spsc_rb_write(
    aeron_spsc_rb_t *ring_buffer, int32_t msg_type_id, const void *msg, size_t length);

aeron_rb_write_result_t aeron_spsc_rb_writev(
    aeron_spsc_rb_t *ring_buffer, int32_t msg_type_id, const struct iovec* iov, int iovcnt);

int32_t aeron_spsc_rb_try_claim(aeron_spsc_rb_t *ring_buffer, int32_t msg_type_id, size_t length);

int aeron_spsc_rb_commit(aeron_spsc_rb_t *ring_buffer, int32_t offset);

int aeron_spsc_rb_abort(aeron_spsc_rb_t *ring_buffer, int32_t offset);

size_t aeron_spsc_rb_read(
    aeron_spsc_rb_t *ring_buffer, aeron_rb_handler_t handler, void *clientd, size_t message_count_limit);

int64_t aeron_spsc_rb_next_correlation_id(aeron_spsc_rb_t *ring_buffer);

void aeron_spsc_rb_consumer_heartbeat_time(aeron_spsc_rb_t *ring_buffer, int64_t time_ms);

#endif //AERON_SPSC_RB_H
