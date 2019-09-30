/*
 * Minimal GEO Semiconductor camera control definitions.
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

#ifndef _GEOCAM_PRIV_H_
#define _GEOCAM_PRIV_H_

#include <linux/videodev2.h>

#define V4L2_CID_PRIV_EXT_BASE        (0x4000000)
#define V4L2_CID_XU_BASE              (V4L2_CID_PRIVATE_BASE|V4L2_CID_PRIV_EXT_BASE)
#define V4L2_CID_MUX_XU_START_CHANNEL (V4L2_CID_XU_BASE + 107)
#define V4L2_CID_MUX_XU_STOP_CHANNEL  (V4L2_CID_XU_BASE + 108)

#define MUX1_XU_GUID {0xd9, 0x92, 0x2b, 0xba, 0xf2, 0x26, 0x94, 0x42, 0x42, 0xae, 0xe4, 0xeb, 0x4d, 0x68, 0xdd, 0x06}
#define MUX_XU_SEL_START_CHANNEL 16
#define MUX_XU_SEL_STOP_CHANNEL  17

/*
 * We expect a single stream with either the following hard-coded fourcc ...
 */

#define MUX_FOURCC          v4l2_fourcc_be('M','U','X',' ')

/* ... or the zero fourcc for an unpatched kernel. */

#define MUX_FOURCC_UNKNOWN  0

#define PAYLOAD_FOURCC      V4L2_PIX_FMT_H264
#define PAYLOAD_DESCRIPTION "H.264"

#endif
