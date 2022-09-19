/* flex - tool to generate fast lexical analyzers */

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

/* Take note: The buffer object is sometimes used as a String buffer (one
 * continuous string), and sometimes used as a list of strings, usually line by
 * line.
 * 
 * The type is specified in buf_init by the elt_size. If the elt_size is
 * sizeof(char), then the buffer should be treated as string buffer. If the
 * elt_size is sizeof(char*), then the buffer should be treated as a list of
 * strings.
 *
 * Certain functions are only appropriate for one type or the other. 
 */

/* global buffers. */
struct Buf userdef_buf;		/**< for user #definitions triggered by cmd-line. */
struct Buf top_buf;             /**< contains %top code. String buffer. */

/* Append a "%s" formatted string to a string buffer */
struct Buf *buf_prints (struct Buf *buf, const char *fmt, const char *s)
{
	char   *t;
        size_t tsz;

	tsz = strlen(fmt) + strlen(s) + 1;
	t = malloc(tsz);
	if (!t)
	    flexfatal (_("Allocation of buffer to print string failed"));
	/*按fmt格式化s,产生t*/
	snprintf (t, tsz, fmt, s);
	/*将t添加进buf*/
	buf = buf_strappend (buf, t);
	free(t);
	return buf;
}

/* Appends n characters in str to buf. */
struct Buf *buf_strnappend (struct Buf *buf, const char *str, int n)
{
    /*将str中前n+1个元素(包含了'\0')填充进buf*/
	buf_append (buf, str, n + 1);

	/* "undo" the '\0' character that buf_append() already copied. */
	buf->nelts--;/*回退'\0'*/

	return buf;
}

/* Appends characters in str to buf. */
struct Buf *buf_strappend (struct Buf *buf, const char *str)
{
    /*将字符串str填充进buf中*/
	return buf_strnappend (buf, str, (int) strlen (str));
}

/* create buf with 0 elements, each of size elem_size. */
void buf_init (struct Buf *buf, size_t elem_size)
{
    /*初始化buffer*/
	buf->elts = NULL;
	buf->nelts = 0;
	buf->elt_size = elem_size;
	buf->nmax = 0;
}

/* frees memory */
void buf_destroy (struct Buf *buf)
{
	if (buf) {
		free(buf->elts);
		buf->elts = NULL;
	}
}


/* appends ptr[] to buf, grow if necessary.
 * n_elem is number of elements in ptr[], NOT bytes.
 * returns buf.
 * We grow by mod(512) boundaries.
 */

struct Buf *buf_append (struct Buf *buf, const void *ptr, int n_elem)
{
	int     n_alloc = 0;

	if (!ptr || n_elem == 0)
		return buf;

	/* May need to alloc more. */
	if (n_elem + buf->nelts > buf->nmax) {

		/* exact count needed... */
		n_alloc = n_elem + buf->nelts;

		/* ...plus some extra */
		if ((((size_t) n_alloc * buf->elt_size) % 512) != 0
		    && buf->elt_size < 512)
			n_alloc += (int)
				((512 -
				 (((size_t) n_alloc * buf->elt_size) % 512)) /
				buf->elt_size);

		if (!buf->elts)
		    /*elts为空，直接分配*/
			buf->elts =
				allocate_array ((int) n_alloc, buf->elt_size);
		else
		    /*elts不为空，则执行realloc*/
			buf->elts =
				reallocate_array (buf->elts, (int) n_alloc,
						  buf->elt_size);

		buf->nmax = n_alloc;
	}

	/*将ptr指向的内容填充进去，元素数增加*/
	memcpy ((char *) buf->elts + (size_t) buf->nelts * buf->elt_size/*当前元素结尾*/, ptr,
		(size_t) n_elem * buf->elt_size);
	buf->nelts += n_elem;

	return buf;
}

/* vim:set tabstop=8 softtabstop=4 shiftwidth=4: */
