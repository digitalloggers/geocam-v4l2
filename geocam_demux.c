/*
 * GeoVision camera MUX format support.
 *
 * Copyright (C) 2019 Digital Loggers, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335  USA
 */

#include <string.h>
#include "geocam.h"

static uint32_t uint32be(uint8_t* buf)
{
    return ((uint32_t)(buf[3]) | ((uint32_t)(buf[2]) << 8) | ((uint32_t)(buf[1]) << 16) | ((uint32_t)(buf[0]) << 24));
}

ssize_t geocam_demux_process(struct geocam_demux_state* state, uint8_t* raw_bytes, ssize_t raw_byte_count)
{
    ssize_t ret = 0;
    uint8_t* input_bytes = raw_bytes;
    uint8_t* output_bytes = raw_bytes;
    while (raw_byte_count)
    {
        if (state->header_state < SKIP)
        {
            if (state->buf_pos + raw_byte_count < sizeof(uint32_t))
            {
                memcpy(state->buf + state->buf_pos, input_bytes, raw_byte_count);
                input_bytes += raw_byte_count;
                state->buf_pos += raw_byte_count;
                raw_byte_count = 0;
            }
            else
            {
                memcpy(state->buf + state->buf_pos, input_bytes, sizeof(uint32_t) - state->buf_pos);
                input_bytes += sizeof(uint32_t) - state->buf_pos;
                raw_byte_count -= sizeof(uint32_t) - state->buf_pos;
                switch (state->header_state)
                {
                case TOTAL_LENGTH:
                    state->payload_left = uint32be(state->buf) - GEOCAM_MUX_MIN_HEADER_SIZE;
                    break;
                case VER_FLAGS:
                    state->have_st6 = state->buf[0] == 1;
                    break;
                case SAMPLE_FLAGS:
                    state->have_skip_size = state->buf[2] & 0x2;
                    break;
                case ST_6:
                    state->payload_left -= sizeof(uint32_t);
                    break;
                case SKIP_SIZE:
                    state->payload_left -= sizeof(uint32_t);
                    state->skip_left = uint32be(state->buf) - sizeof(uint32_t);
                    state->payload_left -= state->skip_left;
                    break;
                default:
                    break;
                }
                state->header_state++;
                if (state->header_state == ST_6 && !state->have_st6)
                {
                    state->header_state++;
                }
                if (state->header_state == SKIP_SIZE && !state->have_skip_size)
                {
                    state->header_state++;
                }
                state->buf_pos = 0;
            }
        }
        switch (state->header_state)
        {
        case SKIP:
            if (state->skip_left >= raw_byte_count)
            {
                state->skip_left -= raw_byte_count;
                raw_byte_count = 0;
            }
            else
            {
                input_bytes += state->skip_left;
                raw_byte_count -= state->skip_left;
                state->skip_left = 0;
                state->header_state = PAYLOAD;
            }
            break;
        case PAYLOAD:
            if (input_bytes != output_bytes)
            {
                memmove(output_bytes, input_bytes, raw_byte_count);
                input_bytes = output_bytes;
            }
            if (state->payload_left >= raw_byte_count)
            {
                state->payload_left -= raw_byte_count;
                ret += raw_byte_count;
                raw_byte_count = 0;
            }
            else
            {
                ret += state->payload_left;
                raw_byte_count -= state->payload_left;
                input_bytes += state->payload_left;
                output_bytes += state->payload_left;
                state->payload_left = 0;
                state->header_state = TOTAL_LENGTH;
            }
            break;
        default:
            break;
        }
    }
    return ret;
}
