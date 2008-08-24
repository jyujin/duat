/*
 *  9p.c
 *  libduat
 *
 *  Created by Magnus Deininger on 21/08/2008.
 *  Copyright 2008 Magnus Deininger. All rights reserved.
 *
 */

/*
 * Copyright (c) 2008, Magnus Deininger All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution. *
 * Neither the name of the project nor the names of its contributors may
 * be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include <duat/9p.h>
#include <curie/memory.h>
#include <curie/multiplex.h>

struct io_element {
    struct duat_9p_io *io;
    void *data;
};

/* read these codes in the plan9 sourcecode... guess there's not much of an
   alternative to look these codes up. */
enum request_code {
    Tversion = 100,
    Rversion = 101,
    Tauth    = 102,
    Rauth    = 103,
    Tattach  = 104,
    Rattach  = 105,
/*    Terror   = 106, */
    Rerror   = 107,
    Tflush   = 108,
    Rflush   = 109,
    Twalk    = 110,
    Rwalk    = 111,
    Topen    = 112,
    Ropen    = 113,
    Tcreate  = 114,
    Rcreate  = 115,
    Tread    = 116,
    Rread    = 117,
    Twrite   = 118,
    Rwrite   = 119,
    Tclunk   = 120,
    Rclunk   = 121,
    Tremove  = 122,
    Rremove  = 123,
    Tstat    = 124,
    Rstat    = 125,
    Twstat   = 126,
    Rwstat   = 127,
};

static struct memory_pool list_pool = MEMORY_POOL_INITIALISER(sizeof (struct io_element));
static struct memory_pool duat_io_pool = MEMORY_POOL_INITIALISER(sizeof (struct duat_9p_io));

struct duat_9p_io *duat_open_io (struct io *in, struct io *out) {
    struct duat_9p_io *rv;

    rv = get_pool_mem (&duat_io_pool);

    rv->in = in;
    rv->out = out;

    rv->Tauth   = (void *)0;
    rv->Tattach = (void *)0;
    rv->Tflush  = (void *)0;
    rv->Twalk   = (void *)0;
    rv->Topen   = (void *)0;
    rv->Tcreate = (void *)0;
    rv->Tread   = (void *)0;
    rv->Twrite  = (void *)0;
    rv->Tclunk  = (void *)0;
    rv->Tremove = (void *)0;
    rv->Tstat   = (void *)0;
    rv->Twstat  = (void *)0;

    rv->Rauth   = (void *)0;
    rv->Rattach = (void *)0;
    rv->Rerror  = (void *)0;
    rv->Rflush  = (void *)0;
    rv->Rwalk   = (void *)0;
    rv->Ropen   = (void *)0;
    rv->Rcreate = (void *)0;
    rv->Rread   = (void *)0;
    rv->Rwrite  = (void *)0;
    rv->Rclunk  = (void *)0;
    rv->Rremove = (void *)0;
    rv->Rstat   = (void *)0;
    rv->Rwstat  = (void *)0;

    rv->arbitrary = (void *)0;

    in->type = iot_read;
    out->type = iot_write;

    return rv;
}

void duat_close_io (struct duat_9p_io *io) {
    io_close (io->in);
    io_close (io->out);

    free_pool_mem (io);
}

void multiplex_duat () {
    static char installed = (char)0;

    if (installed == (char)0) {
        multiplex_io();
        installed = (char)1;
    }
}

static int_64 popq (unsigned char *p) {
    return (((int_64)(p[7])) << 56) +
           (((int_64)(p[6])) << 48) +
           (((int_64)(p[5])) << 40) +
           (((int_64)(p[4])) << 32) +
           (((int_64)(p[3])) << 24) +
           (((int_64)(p[2])) << 16) +
           (((int_64)(p[1])) << 8)  +
           (((int_64)(p[0])));
}

static int_32 popl (unsigned char *p) {
    return (((int_32)(p[3])) << 24) +
           (((int_32)(p[2])) << 16) +
           (((int_32)(p[1])) << 8)  +
           (((int_32)(p[0])));
}

static int_16 popw (unsigned char *p) {
    return (((int_16)(p[1])) << 8) +
           (((int_16)(p[0])));
}

static int_64 toleq (unsigned char *p, int_64 n) {
    union {
        unsigned char c[8];
        int_64 i;
    } res = { .c = { (unsigned char)((n >> 56) & 0xff),
                     (unsigned char)((n >> 48) & 0xff),
                     (unsigned char)((n >> 40) & 0xff),
                     (unsigned char)((n >> 32) & 0xff),
                     (unsigned char)((n >> 24) & 0xff),
                     (unsigned char)((n >> 16) & 0xff),
                     (unsigned char)((n >> 8)  & 0xff),
                     (unsigned char)(n         & 0xff) } };

    return res.i;
}

static int_32 tolel (unsigned char *p, int_32 n) {
    union {
      unsigned char c[4];
      int_32 i;
    } res = { .c = { (unsigned char)((n >> 24) & 0xff),
                     (unsigned char)((n >> 16) & 0xff),
                     (unsigned char)((n >> 8)  & 0xff),
                     (unsigned char)(n         & 0xff) } };

    return res.i;
}

static int_16 tolew (unsigned char *p, int_16 n) {
    union {
        unsigned char c[2];
        int_16 i;
    } res = { .c = { (unsigned char)((n >> 8)  & 0xff),
                     (unsigned char)(n         & 0xff) } };

    return res.i;
}

static char *pop_string (unsigned char *b, int_32 *i, int_32 length) {
}

static unsigned int pop_message (unsigned char *b, int_32 length,
                                 struct duat_9p_io *io, void *d) {
    enum request_code code = (enum request_code)(b[4]);
    int_16 tag = popw (b + 5);
    int_32 i = 7;

    switch (code) {
        case Tversion:
            if (length > 13)
            {
                int_32 msize = popl (b + 7);
                int_16 slen  = popd (b + 11);

                if ((slen + 13) < length)
                {
                    return length;
                }

                duat_9p_reply_error (io, tag, "Message too short.");
                return 0;
            }

            duat_9p_reply_error (io, tag, "Message too short.");
            return 0;
        case Rversion:
        case Tauth:
        case Rauth:
        case Tattach:
        case Rattach:
        case Rerror:
        case Tflush:
        case Rflush:
        case Twalk:
        case Rwalk:
        case Topen:
        case Ropen:
        case Tcreate:
        case Rcreate:
        case Tread:
        case Rread:
        case Twrite:
        case Rwrite:
        case Tclunk:
        case Rclunk:
        case Tremove:
        case Rremove:
        case Tstat:
        case Rstat:
        case Twstat:
        case Rwstat:
        default:
            /* bad/unrecognised message */
            duat_9p_reply_error (io, tag, "Function not implemented.");
            io_close (io->in);
    }

    return 0;
}

static void mx_on_read (struct io *in, void *d) {
    struct io_element *element = (struct io_element *)d;
    unsigned int p = in->position;
    int_32 cl = (in->length - p);

    if (cl > 6) { /* enough data to parse a message... */
        int_32 length = popl ((unsigned char *)(in->buffer + p));

        if (cl < length) return;

        in->position += pop_message ((unsigned char *)(in->buffer + p), length,
                                     element->io, element->data);
    }
}

void multiplex_add_duat (struct duat_9p_io *io, void *data) {
    struct io_element *element = get_pool_mem (&list_pool);

    element->io = io;
    element->data = data;

    multiplex_add_io (io->in, mx_on_read, (void *)element);
    multiplex_add_io_no_callback(io->out);
}
