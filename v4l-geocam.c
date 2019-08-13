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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>

#include <linux/videodev2.h>
#include <libv4l-plugin.h>
#include <linux/uvcvideo.h>

#include "geocam.h"
#include "geocam_priv.h"

#define SYS_IOCTL(fd, cmd, arg) syscall(SYS_ioctl, (int)(fd), (unsigned long)(cmd), (void*)(arg))
#define SYS_READ(fd, buf, len)  syscall(SYS_read,  (int)(fd), (void*)(buf),         (size_t)(len))
#define SYS_WRITE(fd, buf, len) syscall(SYS_write, (int)(fd), (const void*)(buf),   (size_t)(len))

#if HAVE_VISIBILITY
#define PLUGIN_PUBLIC __attribute__ ((visibility("default")))
#else
#define PLUGIN_PUBLIC
#endif

typedef struct uvc_xu_control_mapping control_mapping;

static control_mapping extra_controls[] ={
    { .id = V4L2_CID_MUX_XU_START_CHANNEL, .name = "Channel start", .entity = MUX1_XU_GUID, .selector = MUX_XU_SEL_START_CHANNEL, .size = 32, .offset = 0, .v4l2_type = V4L2_CTRL_TYPE_INTEGER, .data_type = UVC_CTRL_DATA_TYPE_SIGNED },
    { .id = V4L2_CID_MUX_XU_STOP_CHANNEL,  .name = "Channel stop",  .entity = MUX1_XU_GUID, .selector = MUX_XU_SEL_STOP_CHANNEL,  .size = 32, .offset = 0, .v4l2_type = V4L2_CTRL_TYPE_INTEGER, .data_type = UVC_CTRL_DATA_TYPE_SIGNED }
};

struct v4l_mmapped_buf
{
    void* data;
    size_t size;
};

struct v4l_geocam_demux_state
{
    struct geocam_demux_state state;
    struct v4l_mmapped_buf* mmapped_bufs;
    unsigned mmapped_buf_count;
};

static void* plugin_init(int fd)
{
    struct v4l2_fmtdesc fmt;
    int rc;
    struct v4l_geocam_demux_state* ret = NULL;

    fmt.index = 0;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rc = SYS_IOCTL(fd, VIDIOC_ENUM_FMT, &fmt);
    if ((rc == 0) && (fmt.pixelformat == MUX_FOURCC))
    {
        ret = (struct v4l_geocam_demux_state*)calloc (1, sizeof (struct v4l_geocam_demux_state));
        if (ret)
        {
            int i;
            for (i=0; i < sizeof(extra_controls)/sizeof(extra_controls[0]); i++)
            {
                SYS_IOCTL(fd, UVCIOC_CTRL_MAP, &extra_controls[i]);
            }
        }
    }
    return (void*)ret;
}

static void plugin_close(void* dev_ops_priv)
{
    if (dev_ops_priv)
    {
        struct v4l_geocam_demux_state* state = (struct v4l_geocam_demux_state*)dev_ops_priv;
        if (state->mmapped_bufs)
        {
            size_t i;
            for (i=0; i<state->mmapped_buf_count; i++)
            {
                if (state->mmapped_bufs[i].data)
                {
                    munmap(state->mmapped_bufs[i].data, state->mmapped_bufs[i].size);
                }
            }
            free(state->mmapped_bufs);
        }
        free(state);
    }
}

static int plugin_ioctl(void* dev_ops_priv, int fd, unsigned long int cmd, void* arg)
{
    struct v4l_geocam_demux_state* state = (struct v4l_geocam_demux_state*)dev_ops_priv;

#define IOCTL_WITH_TRANSFORMED_PIXEL_FORMAT(TYPE,FIELD) \
    do {                                                \
        TYPE* typed_arg = (TYPE*)arg;                   \
        if (                                            \
            (typed_arg->FIELD == PAYLOAD_FOURCC)        \
        )                                               \
        {                                               \
            typed_arg->FIELD = MUX_FOURCC;              \
        }                                               \
        ret = SYS_IOCTL(fd, cmd, arg);                  \
        if (                                            \
            (typed_arg->FIELD == MUX_FOURCC)            \
        )                                               \
        {                                               \
            typed_arg->FIELD = PAYLOAD_FOURCC;          \
        }                                               \
    }                                                   \
    while(0)

    int ret=-1;
    switch (cmd)
    {
    case VIDIOC_ENUM_FMT:
        {
            struct v4l2_fmtdesc* fmt = (struct v4l2_fmtdesc*)arg;
            if (
                (fmt->index == 0) &&
                (fmt->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
            )
            {
                fmt->flags = V4L2_FMT_FLAG_COMPRESSED;
                fmt->pixelformat = PAYLOAD_FOURCC;
                snprintf((char*)fmt->description, sizeof(fmt->description), PAYLOAD_DESCRIPTION);
                ret = 0;
            }
            else
            {
                ret = SYS_IOCTL(fd, cmd, arg);
            }
        }
        break;
    case VIDIOC_G_FMT:
    case VIDIOC_S_FMT:
    case VIDIOC_TRY_FMT:
        IOCTL_WITH_TRANSFORMED_PIXEL_FORMAT(struct v4l2_format, fmt.pix.pixelformat);
        break;
    case VIDIOC_ENUM_FRAMEINTERVALS:
        IOCTL_WITH_TRANSFORMED_PIXEL_FORMAT(struct v4l2_frmivalenum, pixel_format);
        break;
    case VIDIOC_ENUM_FRAMESIZES:
        IOCTL_WITH_TRANSFORMED_PIXEL_FORMAT(struct v4l2_frmsizeenum, pixel_format);
        break;
    case VIDIOC_QUERYCAP:
        ret = SYS_IOCTL(fd, cmd, arg);
        /*
         * TODO: Implement demultiplexing with async I/O interfaces
         * instead of disabling it and drop this ioctl handling.
         */
        if (ret == 0)
        {
            struct v4l2_capability* cap = (struct v4l2_capability*)arg;
            cap->capabilities &= ~V4L2_CAP_ASYNCIO;
            cap->device_caps &= ~V4L2_CAP_ASYNCIO;
        }
        break;
    case VIDIOC_REQBUFS:
        {
            struct v4l2_requestbuffers* bufs = (struct v4l2_requestbuffers*) arg;
            if (bufs->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
            {
                switch (bufs->memory)
                {
                case V4L2_MEMORY_MMAP:
                case V4L2_MEMORY_USERPTR:
                    ret = SYS_IOCTL(fd, cmd, arg);
                    break;
                default:
                    ret = -EINVAL;
                }
            }
            else
            {
                ret = SYS_IOCTL(fd, cmd, arg);
            }
        }
        break;
    case VIDIOC_CREATE_BUFS:
        /* TODO: Implement creation processing instead of disabling it and drop this ioctl handling. */
        ret = -ENOTSUP;
        break;
    case VIDIOC_DQBUF:
        ret = SYS_IOCTL(fd, cmd, arg);
        if (ret == 0)
        {
            struct v4l2_buffer* buf = (struct v4l2_buffer*)arg;
            if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
            {
                uint8_t* data = NULL;
                switch (buf->memory)
                {
                case V4L2_MEMORY_MMAP:
                    if (buf->index < state->mmapped_buf_count)
                    {
                        data = state->mmapped_bufs[buf->index].data;
                    }
                    if (!data)
                    {
                        if (buf->index >= state->mmapped_buf_count)
                        {
                            state->mmapped_bufs = (struct v4l_mmapped_buf*)realloc(state->mmapped_bufs, (buf->index+1)*sizeof(struct v4l_mmapped_buf));
                        }
                        data = mmap(NULL, buf->length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, buf->m.offset);
                        state->mmapped_bufs[buf->index].data = data;
                        state->mmapped_bufs[buf->index].size = buf->length;
                    }
		    break;
                case V4L2_MEMORY_USERPTR:
                    data = (uint8_t*)buf->m.userptr;
                    break;
                default:
                    /* SHOULDNT-HAPPEN */
                    break;
                }
                if (data)
                {
                    buf->bytesused = geocam_demux_process(&state->state, data, buf->bytesused);
                }
            }
        }
        break;
    case VIDIOC_STREAMON:
        {
            struct v4l2_control control;
            control.id = V4L2_CID_MUX_XU_START_CHANNEL;
            control.value = 0;
            ret = SYS_IOCTL(fd, VIDIOC_S_CTRL, &control);
            if (ret == 0)
            {
                ret = SYS_IOCTL(fd, cmd, arg);
            }
        }
        break;
    case VIDIOC_STREAMOFF:
        {
            struct v4l2_control control;
            control.id = V4L2_CID_MUX_XU_STOP_CHANNEL;
            control.value = 0;
            ret = SYS_IOCTL(fd, VIDIOC_S_CTRL, &control);
            if (ret == 0)
            {
                ret = SYS_IOCTL(fd, cmd, arg);
            }
        }
        break;
    default:
        ret = SYS_IOCTL(fd, cmd, arg);
    }

#undef IOCTL_WITH_TRANSFORMED_PIXEL_FORMAT

    return ret;
}

static ssize_t plugin_read(void* dev_ops_priv, int fd, void* buf, size_t len)
{
    struct v4l_geocam_demux_state* state = (struct v4l_geocam_demux_state*)dev_ops_priv;
    ssize_t read_count = SYS_READ(fd, buf, len);
    read_count = geocam_demux_process(&state->state, buf, read_count);
    return read_count;
}

PLUGIN_PUBLIC const struct libv4l_dev_ops libv4l2_plugin = {
    .init = &plugin_init,
    .close = &plugin_close,
    .ioctl = &plugin_ioctl,
    .read = &plugin_read
};
