/* misc - miscellaneous flex routines */

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
#include "tables.h"

/* Append "new_text" to the running buffer. */
void add_action (const char *new_text)
{
    /*字符串长度*/
	int     len = (int) strlen (new_text);

	while (len + action_index >= action_size - 10 /* slop */ ) {

		if (action_size > INT_MAX / 2)
			/* Increase just a little, to try to avoid overflow
			 * on 16-bit machines.
			 */
			action_size += action_size / 8;
		else
			action_size = action_size * 2;

		/*扩大action array*/
		action_array =
			reallocate_character_array (action_array,
						    action_size);
	}

	/*将new_text输出到action数组*/
	strcpy (&action_array[action_index], new_text);

	/*action数组待填充起始地址*/
	action_index += len;
}


/* allocate_array - allocate memory for an integer array of the given size */

void   *allocate_array (int size, size_t element_size)
{
	void *new_array;
#if HAVE_REALLOCARR
	new_array = NULL;
	if (reallocarr(&new_array, (size_t) size, element_size))
		flexfatal (_("memory allocation failed in allocate_array()"));
#else
# if HAVE_REALLOCARRAY
	new_array = reallocarray(NULL, (size_t) size, element_size);
# else
	/* Do manual overflow detection */
	/*需要申请size个element,每个element大小为element_size，则内存大小为num_bytes*/
	size_t num_bytes = (size_t) size * element_size;
	new_array = (size && SIZE_MAX / (size_t) size < element_size) ? NULL /*所需内存过大时返回NULL*/:
		malloc(num_bytes);
# endif
	if (!new_array)
	    /*申请内存失败后，直接退出*/
		flexfatal (_("memory allocation failed in allocate_array()"));
#endif
	return new_array;
}


/* all_lower - true if a string is all lower-case */

int all_lower (char *str)
{
    /*检查str是否全为小写字每（如果非小写或者不是ascii，则返回0）*/
	while (*str) {
		if (!isascii ((unsigned char) * str) || !islower ((unsigned char) * str))
			return 0;
		++str;
	}

	return 1;
}


/* all_upper - true if a string is all upper-case */

int all_upper (char *str)
{
    /*检查str是否全为大写字每（如果非大写或者不是ascii，则返回0）*/
	while (*str) {
		if (!isascii ((unsigned char) * str) || !isupper ((unsigned char) * str))
			return 0;
		++str;
	}

	return 1;
}


/* intcmp - compares two integers for use by qsort. */

int intcmp (const void *a, const void *b)
{
    /*a,b是两个int*指针，比对两者指向的int值是否相等*/
  return *(const int *) a - *(const int *) b;
}


/* check_char - checks a character to make sure it's within the range
 *		we're expecting.  If not, generates fatal error message
 *		and exits.
 */

void check_char (int c)
{
	if (c >= CSIZE)
	    /*字符c超过CSIZE*/
		lerr (_("bad character '%s' detected in check_char()"),
			readable_form (c));

	if (c >= ctrl.csize)
	    /*c大于等于ctrl.csize*/
		lerr (_
			("scanner requires -8 flag to use the character %s"),
			readable_form (c));
}



/* clower - replace upper-case letter to lower-case */

unsigned char clower (int c)
{
    /*如果c为大写字每，则将其转换为小写*/
	return (unsigned char) ((isascii (c) && isupper (c)) ? tolower (c) : c);
}


char *xstrdup(const char *s)
{
	char *s2;

	/*制做s的副本，并返回*/
	if ((s2 = strdup(s)) == NULL)
		flexfatal (_("memory allocation failure in xstrdup()"));

	return s2;
}


/* cclcmp - compares two characters for use by qsort with '\0' sorting last. */

int cclcmp (const void *a, const void *b)
{
	if (!*(const unsigned char *) a)
		return 1;
	else
		if (!*(const unsigned char *) b)
			return - 1;
		else
			return *(const unsigned char *) a - *(const unsigned char *) b;
}


/* dataend - finish up a block of data declarations */

void dataend (const char *endit)
{
	/* short circuit any output */
	if (gentables) {

		if (datapos > 0)
			dataflush ();

		/* add terminator for initialization; { for vi */
		if (endit)
		    outn (endit);
	}
	dataline = 0;
	datapos = 0;
}


/* dataflush - flush generated data statements */

void dataflush (void)
{
	/* short circuit any output */
	if (!gentables)
		return;

	if (datapos > 0)
		outc ('\n');

	if (++dataline >= NUMDATALINES) {
		/* Put out a blank line so that the table is grouped into
		 * large blocks that enable the user to find elements easily.
		 */
		outc ('\n');
		dataline = 0;
	}

	/* Reset the number of characters written on the current line. */
	datapos = 0;
}


/* flexerror - report an error message and terminate */

void flexerror (const char *msg)
{
	fprintf (stderr, "%s: %s\n", program_name, msg);
	flexend (1);
}


/* flexfatal - report a fatal error message and terminate */

void flexfatal (const char *msg)
{
	fprintf (stderr, _("%s: fatal internal error, %s\n"),
		 program_name, msg);
	FLEX_EXIT (1);
}


/* lerr - report an error message */

void lerr (const char *msg, ...)
{
	char    errmsg[MAXLINE];
	va_list args;

	va_start(args, msg);
	vsnprintf (errmsg, sizeof(errmsg), msg, args);
	va_end(args);
	flexerror (errmsg);
}


/* lerr_fatal - as lerr, but call flexfatal */

void lerr_fatal (const char *msg, ...)
{
	char    errmsg[MAXLINE];
	va_list args;
	va_start(args, msg);

	vsnprintf (errmsg, sizeof(errmsg), msg, args);
	va_end(args);
	flexfatal (errmsg);
}


/* line_directive_out - spit out a "#line" statement or equivalent */
void line_directive_out (FILE *output_file, char *path, int linenum)
{
	char	*trace_fmt = "m4_ifdef([[M4_HOOK_TRACE_LINE_FORMAT]], [[M4_HOOK_TRACE_LINE_FORMAT([[%d]], [[%s]])]])";
	char    directive[MAXLINE*2], filename[MAXLINE];
	char   *s1, *s2, *s3;

	if (!ctrl.gen_line_dirs)
		/*如果不输出行指令，则直接退出*/
		return;

	s1 = (path != NULL) ? path : "M4_YY_OUTFILE_NAME";

	if ((path != NULL) && !s1)
		s1 = "<stdin>";
    
	s2 = filename;
	s3 = &filename[sizeof (filename) - 2];

	while (s2 < s3 && *s1) {
		if (*s1 == '\\' || *s1 == '"')
			/* Escape the '\' or '"' */
			*s2++ = '\\';

		*s2++ = *s1++;
	}

	*s2 = '\0';

	if (path != NULL)
		snprintf (directive, sizeof(directive), trace_fmt, linenum, filename);
	else {
		snprintf (directive, sizeof(directive), trace_fmt, 0, filename);
	}

	/* If output_file is nil then we should put the directive in
	 * the accumulated actions.
	 */
	if (output_file) {
		fputs (directive, output_file);
	}
	else
		add_action (directive);
}


/* mark_defs1 - mark the current position in the action array as
 *               representing where the user's section 1 definitions end
 *		 and the prolog begins
 */
void mark_defs1 (void)
{
	defs1_offset = 0;
	action_array[action_index++] = '\0';
	action_offset = prolog_offset = action_index;
	action_array[action_index] = '\0';
}


/* mark_prolog - mark the current position in the action array as
 *               representing the end of the action prolog
 */
void mark_prolog (void)
{
	action_array[action_index++] = '\0';
	action_offset = action_index;
	action_array[action_index] = '\0';
}


/* mk2data - generate a data statement for a two-dimensional array
 *
 * Generates a data statement initializing the current 2-D array to "value".
 */
void mk2data (int value)
{
	/* short circuit any output */
	if (!gentables)
		return;

	if (datapos >= NUMDATAITEMS) {
		outc (',');
		dataflush ();
	}

	if (datapos == 0)
		/* Indent. */
		out ("    ");

	else
		outc (',');

	++datapos;

	out_dec ("%5d", value);
}


/* mkdata - generate a data statement
 *
 * Generates a data statement initializing the current array element to
 * "value".
 */
void mkdata (int value)
{
	/* short circuit any output */
	if (!gentables)
		return;

	if (datapos >= NUMDATAITEMS) {
		outc (',');
		dataflush ();
	}

	if (datapos == 0)
		/* Indent. */
		out ("     ");
	else
		outc (',');

	++datapos;

	out_dec ("%5d", value);
}


/* myctoi - return the integer represented by a string of digits */

int myctoi (const char *array)
{
	int     val = 0;

	(void) sscanf (array, "%d", &val);

	return val;
}


/* myesc - return character corresponding to escape sequence */

unsigned char myesc (unsigned char array[])
{
	unsigned char    c, esc_char;

	/*返回转义后字符*/
	switch (array[1]) {
	case 'b':
		return '\b';
	case 'f':
		return '\f';
	case 'n':
		return '\n';
	case 'r':
		return '\r';
	case 't':
		return '\t';
	case 'a':
		return '\a';
	case 'v':
		return '\v';
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
		{		/* \<octal> */
			int     sptr = 1;

			/*查找到数字结尾*/
			while (sptr <= 3 &&
                               array[sptr] >= '0' && array[sptr] <= '7') {
				++sptr;
			}

			/*取出非数字的字符*/
			c = array[sptr];
			array[sptr] = '\0';/*将已保存的字符置为'\0',断开字符串*/

			/*将数字按8进制进行转换*/
			esc_char = (unsigned char) strtoul (array + 1, NULL, 8);

			/*还原刚才为转换而设置为'\0'的字符*/
			array[sptr] = c;

			/*返回转换后的数字*/
			return esc_char;
		}

	case 'x':
		{		/* \x<hex> */
			int     sptr = 2;

			while (sptr <= 3 && isxdigit (array[sptr])) {
				/* Don't increment inside loop control
				 * because if isxdigit() is a macro it might
				 * expand into multiple increments ...
				 */
				++sptr;
			}

			c = array[sptr];
			array[sptr] = '\0';

			esc_char = (unsigned char) strtoul (array + 2, NULL, 16);

			array[sptr] = c;

			/*转换16进制，并返回转换后的value*/
			return esc_char;
		}

	default:
	    /*返回其它非转义字符*/
		return array[1];
	}
}


/* out - various flavors of outputing a (possibly formatted) string for the
 *	 generated scanner, keeping track of the line count.
 */

/*输出字符串到stdout*/
void out (const char *str)
{
	fputs (str, stdout);
}

void out_dec (const char *fmt, int n)
{
	fprintf (stdout, fmt, n);
}

void out_dec2 (const char *fmt, int n1, int n2)
{
	fprintf (stdout, fmt, n1, n2);
}

void out_hex (const char *fmt, unsigned int x)
{
	fprintf (stdout, fmt, x);
}

void out_str (const char *fmt, const char str[])
{
	fprintf (stdout,fmt, str);
}

void out_str_dec (const char *fmt, const char str[], int n)
{
    /*格式化fmt,输出到stdout*/
	fprintf (stdout,fmt, str, n);
}

/*输出一个字符到stdout*/
void outc (int c)
{
	fputc (c, stdout);
}

/*输出一行字符串至stdout*/
void outn (const char *str)
{
	fputs (str,stdout);
    fputc('\n',stdout);
}

/** Print "m4_define( [[def]], [[val]])m4_dnl\n".
 * @param def The m4 symbol to define.
 * @param val The definition; may be NULL.
 */
void out_m4_define (const char* def, const char* val)
{
    /*通过m4定义宏def,其对应的内容为val*/
    const char * fmt = "m4_define( [[%s]], [[%s]])m4_dnl\n";
    fprintf(stdout, fmt, def, val?val:"");
}


/* readable_form - return the the human-readable form of a character
 *
 * The returned string is in static storage.
 */

char   *readable_form (int c)
{
	static char rform[20];

	if ((c >= 0 && c < 32) || c >= 127) {
		switch (c) {
		case '\b':
			return "\\b";
		case '\f':
			return "\\f";
		case '\n':
			return "\\n";
		case '\r':
			return "\\r";
		case '\t':
			return "\\t";
		case '\a':
			return "\\a";
		case '\v':
			return "\\v";
		default:
			if(env.trace_hex)
				snprintf (rform, sizeof(rform), "\\x%.2x", (unsigned int) c);
			else
				snprintf (rform, sizeof(rform), "\\%.3o", (unsigned int) c);
			return rform;
		}
	}

	else if (c == ' ')
		return "' '";

	else {
		rform[0] = (char) c;
		rform[1] = '\0';

		return rform;
	}
}


/* reallocate_array - increase the size of a dynamic array */

void   *reallocate_array (void *array, int size, size_t element_size)
{
	void *new_array;
#if HAVE_REALLOCARR
	new_array = array;
	if (reallocarr(&new_array, (size_t) size, element_size))
		flexfatal (_("attempt to increase array size failed"));
#else
# if HAVE_REALLOCARRAY
	new_array = reallocarray(array, (size_t) size, element_size);
# else
	/* Do manual overflow detection */
	/*需要申请size个element,每个element大小为element_size，则内存大小为num_bytes*/
	size_t num_bytes = (size_t) size * element_size;
	new_array = (size && SIZE_MAX / (size_t) size < element_size) ? NULL /*所需内存过大时返回NULL*/:
		realloc(array, num_bytes);
# endif
	if (!new_array)
		flexfatal (_("attempt to increase array size failed"));
#endif
	return new_array;
}


/* transition_struct_out - output a yy_trans_info structure
 *
 * outputs the yy_trans_info structure with the two elements, element_v and
 * element_n.  Formats the output with spaces and carriage returns.
 */

void transition_struct_out (int element_v, int element_n)
{

	/* short circuit any output */
	if (!gentables)
		return;

	out_dec2 ("M4_HOOK_TABLE_OPENER[[%4d]],[[%4d]]M4_HOOK_TABLE_CONTINUE", element_v, element_n);
	outc ('\n');

	datapos += TRANS_STRUCT_PRINT_LENGTH;

	if (datapos >= 79 - TRANS_STRUCT_PRINT_LENGTH) {
		outc ('\n');

		if (++dataline % 10 == 0)
			outc ('\n');

		datapos = 0;
	}
}


/* The following is only needed when building flex's parser using certain
 * broken versions of bison.
 *
 * XXX: this is should go soon
 */
void   *yy_flex_xmalloc (int size)
{
	void   *result;

	result = malloc((size_t) size);
	if (!result)
		flexfatal (_
			   ("memory allocation failed in yy_flex_xmalloc()"));

	return result;
}


/* Remove all '\n' and '\r' characters, if any, from the end of str.
 * str can be any null-terminated string, or NULL.
 * returns str. */
char   *chomp (char *str)
{
	char   *p = str;

	if (!str || !*str)	/* s is null or empty string */
		return str;

	/* find end of string minus one */
	while (*p)
		++p;
	--p;

	/* eat newlines */
	while (p >= str && (*p == '\r' || *p == '\n'))
		*p-- = 0;
	return str;
}

/*输出注释行*/
void comment(const char *txt)
{
	char buf[MAXLINE];
	bool eol;

	/*注释内容*/
	strncpy(buf, txt, MAXLINE-1);
	eol = buf[strlen(buf)-1] == '\n';/*检查行后是否为'\n'*/

	if (eol)
	    /*移除最后的end of line标记*/
		buf[strlen(buf)-1] = '\0';
	/*采用comment_open,comment_close包裹注释内容*/
	out_str("M4_HOOK_COMMENT_OPEN [[%s]] M4_HOOK_COMMENT_CLOSE", buf);
	if (eol)
		outc ('\n');
}


