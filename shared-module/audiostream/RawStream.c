/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Scott Shawcroft for Adafruit Industries
 * Copyright (c) 2022 Ben Williams (modified from RawSample.c)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "shared-bindings/audiostream/RawStream.h"

#include <stdint.h>

#include "shared-module/audiostream/RawStream.h"


#define SILENCE_LEN 256

void common_hal_audioio_rawstream_construct(audioio_rawstream_obj_t *self,
    uint8_t bytes_per_sample,
    bool samples_signed,
    uint8_t channel_count,
    uint32_t sample_rate,
    bool persistent) {

    self->buffer = NULL;
    self->next_buffer = NULL;

    self->len = 0;
    self->next_len = 0;

    self->bits_per_sample = bytes_per_sample * 8;
    self->samples_signed = samples_signed;
    self->channel_count = channel_count;
    self->sample_rate = sample_rate;

    self->persistent = persistent;

    if (persistent) {
        self->silence = m_malloc(SILENCE_LEN, false);
        if (self->silence == NULL) {
            common_hal_audioio_rawstream_deinit(self);
            m_malloc_fail(SILENCE_LEN);
        }
        for (uint16_t i = 0; i < SILENCE_LEN; i++) {
            self->silence[i] = 0;
        }
    } else {
        // We still need a small silence buffer to return
        // when persistence is off and the buffer runs out.
        self->silence = m_malloc(8, false);
        if (self->silence == NULL) {
            common_hal_audioio_rawstream_deinit(self);
            m_malloc_fail(8);
        }
        for (uint16_t i = 0; i < 8; i++) {
            self->silence[i] = 0;
        }
    }

}

void common_hal_audioio_rawstream_deinit(audioio_rawstream_obj_t *self) {
    self->buffer = NULL;
    self->next_buffer = NULL;
}
bool common_hal_audioio_rawstream_deinited(audioio_rawstream_obj_t *self) {
    return self->buffer == NULL;
}

uint32_t common_hal_audioio_rawstream_get_sample_rate(audioio_rawstream_obj_t *self) {
    return self->sample_rate;
}
void common_hal_audioio_rawstream_set_sample_rate(audioio_rawstream_obj_t *self,
    uint32_t sample_rate) {
    self->sample_rate = sample_rate;
}
uint8_t common_hal_audioio_rawstream_get_bits_per_sample(audioio_rawstream_obj_t *self) {
    return self->bits_per_sample;
}
uint8_t common_hal_audioio_rawstream_get_channel_count(audioio_rawstream_obj_t *self) {
    return self->channel_count;
}

void audioio_rawstream_reset_buffer(audioio_rawstream_obj_t *self,
    bool single_channel_output,
    uint8_t channel) {
}

audioio_get_buffer_result_t audioio_rawstream_get_buffer(audioio_rawstream_obj_t *self,
    bool single_channel_output,
    uint8_t channel,
    uint8_t **buffer,
    uint32_t *buffer_length) {

    audioio_get_buffer_result_t return_value = GET_BUFFER_MORE_DATA;

    if (self->next_buffer != NULL) {
        self->buffer = self->next_buffer;
        self->len = self->next_len;

        self->next_buffer = NULL;
        self->next_len = 0;
    } else {
        self->buffer = self->silence;
        if (self->persistent) {
            self->len = SILENCE_LEN;
        } else {
            self->len = 8;
            return_value = GET_BUFFER_DONE;
        }
    }

    *buffer_length = self->len;
    if (single_channel_output) {
        *buffer = self->buffer + (channel % self->channel_count) * (self->bits_per_sample / 8);
    } else {
        *buffer = self->buffer;
    }

    return return_value;
}

void audioio_rawstream_get_buffer_structure(audioio_rawstream_obj_t *self, bool single_channel_output,
    bool *single_buffer, bool *samples_signed,
    uint32_t *max_buffer_length, uint8_t *spacing) {
    *single_buffer = true;
    *samples_signed = self->samples_signed;
    *max_buffer_length = self->len;
    if (single_channel_output) {
        *spacing = self->channel_count;
    } else {
        *spacing = 1;
    }
}


bool audioio_rawstream_is_ready(audioio_rawstream_obj_t *self) {
    return self->next_buffer == NULL;
}


bool audioio_rawstream_queue_sample(audioio_rawstream_obj_t *self,
    uint8_t *buffer,
    uint32_t buffer_length) {
    if (self->next_buffer) {
        // Maybe raise an exception here instead?
        // I'm not sure how to do that, and it would
        // probably need a new exception type.
        // Maybe something like "buffer full"?
        // What would use less CPU time, handling
        // an exception raised here or waiting for
        // the next iteration of the buffer write
        // code?  Ideally, we want to minimize time,
        // to avoid a situation where a failure to
        // queue  allows enough time to go
        // by for the buffer and the queue to finish
        // before the next attempt.  I suspect
        // returning a bool is the faster option.
        return false;
    }

    self->next_buffer = buffer;
    self->next_len = buffer_length;

    return true;
}
