/*
 * Minimal GeoVision camera MUX format definitions.
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

#ifndef _GEOCAM_H_
#define _GEOCAM_H_

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#define GEOCAM_MUX_MIN_HEADER_SIZE 24

/*
 * Note that we don't have generic support for the multiplexing format here.
 * We expect a single stream with a hard-coded fourcc.
 */

struct geocam_demux_state
{
    enum {
        TOTAL_LENGTH,
        ST_1,
        VER_FLAGS,
        ST_3,
        SAMPLE_FLAGS,
        ST_5,
        ST_6,
        SKIP_SIZE,
        SKIP,
        PAYLOAD
    } header_state;
    size_t skip_left;
    size_t payload_left;
    uint8_t buf[sizeof(uint32_t)];
    uint8_t buf_pos;
    uint8_t have_st6;
    uint8_t have_skip_size;
};

ssize_t geocam_demux_process(struct geocam_demux_state* state, uint8_t* raw_bytes, ssize_t raw_byte_count);

#endif
