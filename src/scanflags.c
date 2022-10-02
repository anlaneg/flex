/* scanflags - flags used by scanning. */

/*  Copyright (c) 1990 The Regents of the University of California. */
/*  All rights reserved. */

/*  This code is derived from software contributed to Berkeley by */
/*  Vern Paxson. */

/*  The United States Government has rights in this work pursuant */
/*  to contract no. DE-AC03-76SF00098 between the United States */
/*  Department of Energy and the University of California. */

/*  This file is part of flex. */

/*  Redistribution and use in source and binary forms, with or without */
/*  modification, are permitted provided that the following conditions */
/*  are met: */

/*  1. Redistributions of source code must retain the above copyright */
/*     notice, this list of conditions and the following disclaimer. */
/*  2. Redistributions in binary form must reproduce the above copyright */
/*     notice, this list of conditions and the following disclaimer in the */
/*     documentation and/or other materials provided with the distribution. */

/*  Neither the name of the University nor the names of its contributors */
/*  may be used to endorse or promote products derived from this software */
/*  without specific prior written permission. */

/*  THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR */
/*  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR */
/*  PURPOSE. */

#include "flexdef.h"

scanflags_t* _sf_stk = NULL;
size_t _sf_top_ix=0, _sf_max=0;

void
sf_push (void)
{
    if (_sf_top_ix + 1 >= _sf_max) {
        /*_sf_stk空间不足，增长其空间*/
        _sf_max += 32;
        _sf_stk = realloc(_sf_stk, sizeof(scanflags_t) * _sf_max);
    }

    // copy the top element
    /*复制栈顶元素到_sf_stk,增加栈指针*/
    _sf_stk[_sf_top_ix + 1] = _sf_stk[_sf_top_ix];
    ++_sf_top_ix;
}

void
sf_pop (void)
{
    /*弹栈*/
    assert(_sf_top_ix > 0);
    --_sf_top_ix;
}

/* one-time initialization. Should be called before any sf_ functions. */
void
sf_init (void)
{
    /*初始化stack,栈顶指针为0*/
    assert(_sf_stk == NULL);
    _sf_max = 32;
    _sf_stk = malloc(sizeof(scanflags_t) * _sf_max);
    if (!_sf_stk)
        lerr_fatal(_("Unable to allocate %zu of stack"), sizeof(scanflags_t));
    _sf_stk[_sf_top_ix] = 0;
}

/* vim:set expandtab cindent tabstop=4 softtabstop=4 shiftwidth=4 textwidth=0: */
