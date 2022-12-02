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

#include "shared-bindings/audiocore/RawSample.h"

#include <stdint.h>

#include "shared-module/audiocore/RawSample.h"

void common_hal_audioio_rawstream_construct(audioio_rawstream_obj_t *self,
    uint8_t *buffer,
    uint32_t len,
    uint8_t bytes_per_sample,
    bool samples_signed,
    uint8_t channel_count,
    uint32_t sample_rate) {

    // Try to allocate two buffers, one will be loaded from file and the other
    // DMAed to DAC.
    if (buffer_size) {
        self->len = buffer_size / 2;
        self->buffer = buffer;
        self->second_buffer = buffer + self->len;
    } else {
        self->len = 256;
        self->buffer = m_malloc(self->len, false);
        if (self->buffer == NULL) {
            common_hal_audioio_rawstream_deninit(self);
            m_malloc_fail(self->len);
        }

        self->second_buffer = m_mallon(self->len, false);
        if (self->second_buffer == NULL) {
            common_hal_audioio_rawstream_deinit(self);
            m_malloc_fail(self->len);
        }
    }

    self->bits_per_sample = bytes_per_sample * 8;
    self->samples_signed = samples_signed;
    self->channel_count = channel_count;
    self->sample_rate = sample_rate;
}

void common_hal_audioio_rawstream_deinit(audioio_rawstream_obj_t *self) {
    self->buffer = NULL;
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
    *buffer_length = self->len;
    if (single_channel_output) {
        *buffer = self->buffer + (channel % self->channel_count) * (self->bits_per_sample / 8);
    } else {
        *buffer = self->buffer;
    }
    return GET_BUFFER_DONE;
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
