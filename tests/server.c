/*
 *  server.c
 *  libcurie
 *
 *  Created by Magnus Deininger on 24/08/2008.
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

#include <curie/multiplex.h>
#include <curie/memory.h>
#include <curie/main.h>

#include <duat/9p-server.h>
#include <duat/filesystem.h>

static void *rm_recover(unsigned long int s, void *c, unsigned long int l)
{
    a_exit(22);
    return (void *)0;
}

static void *gm_recover(unsigned long int s)
{
    a_exit(23);
    return (void *)0;
}

int a_main(void) {
    static char *dirn[1] = { "nyoron" };
    struct dfs *fs;

    set_resize_mem_recovery_function(rm_recover);
    set_get_mem_recovery_function(gm_recover);

    dfs_update_user  ("nyu",     1000);
    dfs_update_group ("kittens", 100);

    multiplex_d9s();

    fs = dfs_create ();

    dfs_mk_file (dfs_mk_directory_r (fs, 1, (char **)dirn),
                 "nyu", (char *)0, (int_8 *)"meow!\n", (int_64)6,
                 (void *)0, (void *)0, (void *)0);
    dfs_mk_file (fs->root,
                 "blah", (char *)0, (int_8 *)"meow!\n", (int_64)6,
                 (void *)0, (void *)0, (void *)0);
    dfs_mk_file (fs->root,
                 "blubb", (char *)0, (int_8 *)"meow!\n", (int_64)6,
                 (void *)0, (void *)0, (void *)0);

    multiplex_add_d9s_socket ("./test-socket-9p", fs);

    while (multiplex() != mx_nothing_to_do);

    return 0;
}
