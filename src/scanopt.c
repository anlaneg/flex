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
#include "scanopt.h"


/* Internal structures */

#define ARG_NONE 0x01
#define ARG_REQ  0x02
#define ARG_OPT  0x04
#define IS_LONG  0x08

struct _aux {
	int     flags;		/* The above hex flags. */
	int     namelen;	/* Length of the actual option word, e.g., "--file[=foo]" is 4 */
	int     printlen;	/* Length of entire string, e.g., "--file[=foo]" is 12 */
};


struct _scanopt_t {
	/*支持的所有选项*/
	const optspec_t *options;	/* List of options. */
	/*每一个选项对应一个_aux结构*/
	struct _aux *aux;	/* Auxiliary data about options. */
	/*指明参数options的数目*/
	int     optc;		/* Number of options. */
	/*argv的数目*/
	int     argc;		/* Number of args. */
	/*传入的参数*/
	char  **argv;		/* Array of strings. */
	/*当前分析位置*/
	int     index;		/* Used as: argv[index][subscript]. */
	int     subscript;
	char    no_err_msg;	/* If true, do not print errors. */
	char    has_long;/*是否有长选项*/
	char    has_short;
};

/* Accessor functions. These WOULD be one-liners, but portability calls. */
static const char *NAME(struct _scanopt_t *, int);
static int PRINTLEN(struct _scanopt_t *, int);
static int RVAL(struct _scanopt_t *, int);
static int FLAGS(struct _scanopt_t *, int);
static const char *DESC(struct _scanopt_t *, int);
static void scanopt_err(struct _scanopt_t *, int, int);
static int matchlongopt(char *, char **, int *, char **, int *);
static int find_opt(struct _scanopt_t *, int, char *, int, int *, int *opt_offset);

static const char *NAME (struct _scanopt_t *s, int i)
{
	return s->options[i].opt_fmt +
		((s->aux[i].flags & IS_LONG) ? 2 : 1);
}

static int PRINTLEN (struct _scanopt_t *s, int i)
{
	return s->aux[i].printlen;
}

static int RVAL (struct _scanopt_t *s, int i)
{
	return s->options[i].r_val;
}

static int FLAGS (struct _scanopt_t *s, int i)
{
	return s->aux[i].flags;
}

static const char *DESC (struct _scanopt_t *s, int i)
{
	return s->options[i].desc ? s->options[i].desc : "";
}

#ifndef NO_SCANOPT_USAGE
static int get_cols (void);

static int get_cols (void)
{
	char   *env;
	int     cols = 80;	/* default */

#ifdef HAVE_NCURSES_H
	initscr ();
	endwin ();
	if (COLS > 0)
		return COLS;
#endif

	if ((env = getenv ("COLUMNS")) != NULL)
		cols = atoi (env);

	return cols;
}
#endif

/* Macro to check for NULL before assigning a value. */
#define SAFE_ASSIGN(ptr,val) \
    do{                      \
        if((ptr)!=NULL)      \
            *(ptr) = val;    \
    }while(0)

/* Macro to assure we reset subscript whenever we adjust s->index.*/
#define INC_INDEX(s,n)     \
    do{                    \
       (s)->index += (n);  \
       (s)->subscript= 0;  \
    }while(0)

scanopt_t *scanopt_init (const optspec_t *options/*选项指针*/, int argc, char **argv, int flags)
{
	int     i;
	struct _scanopt_t *s;
	s = malloc(sizeof (struct _scanopt_t));

	s->options = options;/*指定选项*/
	s->optc = 0;
	s->argc = argc;/*参数数目*/
	s->argv = (char **) argv;/*参数*/
	s->index = 1;
	s->subscript = 0;
	s->no_err_msg = (flags & SCANOPT_NO_ERR_MSG);
	s->has_long = 0;
	s->has_short = 0;

	/* Determine option count. (Find entry with all zeros). */
	s->optc = 0;
	while (options[s->optc].opt_fmt
	       || options[s->optc].r_val || options[s->optc].desc)
		s->optc++;

	/* Build auxiliary data */
	s->aux = malloc((size_t) s->optc * sizeof (struct _aux));

	for (i = 0; i < s->optc; i++) {
		const unsigned char *p, *pname;
		const struct optspec_t *opt;
		struct _aux *aux;

		opt = s->options + i;/*取i号选项*/
		aux = s->aux + i;/*取i号选项对应的aux*/

		aux->flags = ARG_NONE;

		if (opt->opt_fmt[0] == '-' && opt->opt_fmt[1] == '-') {
		    /*此选项为长选项，打LONG标记*/
			aux->flags |= IS_LONG;
			/*获得pname(跳过'--')*/
			pname = (const unsigned char *)(opt->opt_fmt + 2);
			s->has_long = 1;
		}
		else {
		    /*此选项为短选项，跳过'-'*/
			pname = (const unsigned char *)(opt->opt_fmt + 1);
			s->has_short = 1;
		}
		/*此选项长可打印长度*/
		aux->printlen = (int) strlen (opt->opt_fmt);

		aux->namelen = 0;
		for (p = pname + 1; *p; p++) {
			/* detect required arg */
			if (*p == '=' || isspace ((unsigned char)*p)
			    || !(aux->flags & IS_LONG)) {
				if (aux->namelen == 0)
					aux->namelen = (int) (p - pname);
				aux->flags |= ARG_REQ;/*必选参数*/
				aux->flags &= ~ARG_NONE;
			}
			/* detect optional arg. This overrides required arg. */
			if (*p == '[') {
				if (aux->namelen == 0)
					aux->namelen = (int) (p - pname);
				aux->flags &= ~(ARG_REQ | ARG_NONE);
				aux->flags |= ARG_OPT;/*可选参数*/
				break;
			}
		}
		if (aux->namelen == 0)
			aux->namelen = (int) (p - pname);/*无参数*/
	}
	return (scanopt_t *) s;
}

#ifndef NO_SCANOPT_USAGE
/* these structs are for scanopt_usage(). */
struct usg_elem {
	int     idx;
	struct usg_elem *next;
	struct usg_elem *alias;
};
typedef struct usg_elem usg_elem;


/* Prints a usage message based on contents of optlist.
 * Parameters:
 *   scanner  - The scanner, already initialized with scanopt_init().
 *   fp       - The file stream to write to.
 *   usage    - Text to be prepended to option list.
 * Return:  Always returns 0 (zero).
 * The output looks something like this:

[indent][option, alias1, alias2...][indent][description line1
                                            description line2...]
 */
int     scanopt_usage (scanopt_t *scanner, FILE *fp, const char *usage)
{
	struct _scanopt_t *s;
	int     i, columns;
	const int indent = 2;
	usg_elem *byr_val = NULL;	/* option indices sorted by r_val */
	usg_elem *store;	/* array of preallocated elements. */
	int     store_idx = 0;
	usg_elem *ue;
	int     opt_col_width = 0, desc_col_width = 0;
	int     desccol;
	int     print_run = 0;

	s = (struct _scanopt_t *) scanner;

	if (usage) {
		fprintf (fp, "%s\n", usage);
	}
	else {
		fprintf (fp, _("Usage: %s [OPTIONS]...\n"), s->argv[0]);
	}
	fprintf (fp, "\n");

	/* Sort by r_val and string. Yes, this is O(n*n), but n is small. */
	store = malloc((size_t) s->optc * sizeof (usg_elem));
	for (i = 0; i < s->optc; i++) {

		/* grab the next preallocate node. */
		ue = store + store_idx++;
		ue->idx = i;
		ue->next = ue->alias = NULL;

		/* insert into list. */
		if (!byr_val)
			byr_val = ue;
		else {
			int     found_alias = 0;
			usg_elem **ue_curr, **ptr_if_no_alias = NULL;

			ue_curr = &byr_val;
			while (*ue_curr) {
				if (RVAL (s, (*ue_curr)->idx) ==
				    RVAL (s, ue->idx)) {
					/* push onto the alias list. */
					ue_curr = &((*ue_curr)->alias);
					found_alias = 1;
					break;
				}
				if (!ptr_if_no_alias
				    &&
				    strcasecmp (NAME (s, (*ue_curr)->idx),
						NAME (s, ue->idx)) > 0) {
					ptr_if_no_alias = ue_curr;
				}
				ue_curr = &((*ue_curr)->next);
			}
			if (!found_alias && ptr_if_no_alias)
				ue_curr = ptr_if_no_alias;
			ue->next = *ue_curr;
			*ue_curr = ue;
		}
	}

#if 0
	if (1) {
		printf ("ORIGINAL:\n");
		for (i = 0; i < s->optc; i++)
			printf ("%2d: %s\n", i, NAME (s, i));
		printf ("SORTED:\n");
		ue = byr_val;
		while (ue) {
			usg_elem *ue2;

			printf ("%2d: %s\n", ue->idx, NAME (s, ue->idx));
			for (ue2 = ue->alias; ue2; ue2 = ue2->next)
				printf ("  +---> %2d: %s\n", ue2->idx,
					NAME (s, ue2->idx));
			ue = ue->next;
		}
	}
#endif

	/* Now build each row of output. */

	/* first pass calculate how much room we need. */
	for (ue = byr_val; ue; ue = ue->next) {
		usg_elem *ap;
		int     len;

		len = PRINTLEN(s, ue->idx);

		for (ap = ue->alias; ap; ap = ap->next) {
			len += PRINTLEN(s, ap->idx) + (int) strlen(", ");
		}

		if (len > opt_col_width)
			opt_col_width = len;

		/* It's much easier to calculate length for description column! */
		len = (int) strlen (DESC (s, ue->idx));
		if (len > desc_col_width)
			desc_col_width = len;
	}

	/* Determine how much room we have, and how much we will allocate to each col.
	 * Do not address pathological cases. Output will just be ugly. */
	columns = get_cols () - 1;
	if (opt_col_width + desc_col_width + indent * 2 > columns) {
		/* opt col gets whatever it wants. we'll wrap the desc col. */
		desc_col_width = columns - (opt_col_width + indent * 2);
		if (desc_col_width < 14)	/* 14 is arbitrary lower limit on desc width. */
			desc_col_width = INT_MAX;
	}
	desccol = opt_col_width + indent * 2;

#define PRINT_SPACES(fp,n) \
	fprintf((fp), "%*s", (n), "")

	/* Second pass (same as above loop), this time we print. */
	/* Sloppy hack: We iterate twice. The first time we print short and long options.
	   The second time we print those lines that have ONLY long options. */
	while (print_run++ < 2) {
		for (ue = byr_val; ue; ue = ue->next) {
			usg_elem *ap;
			int     nwords = 0, nchars = 0, has_short = 0;

/* TODO: get has_short schtick to work */
			has_short = !(FLAGS (s, ue->idx) & IS_LONG);
			for (ap = ue->alias; ap; ap = ap->next) {
				if (!(FLAGS (s, ap->idx) & IS_LONG)) {
					has_short = 1;
					break;
				}
			}
			if ((print_run == 1 && !has_short) ||
			    (print_run == 2 && has_short))
				continue;

			PRINT_SPACES (fp, indent);
			nchars += indent;

/* Print, adding a ", " between aliases. */
#define PRINT_IT(i) do{\
                  if(nwords++)\
                      nchars+=fprintf(fp,", ");\
                  nchars+=fprintf(fp,"%s",s->options[i].opt_fmt);\
            }while(0)

			if (!(FLAGS (s, ue->idx) & IS_LONG))
				PRINT_IT (ue->idx);

			/* print short aliases first. */
			for (ap = ue->alias; ap; ap = ap->next) {
				if (!(FLAGS (s, ap->idx) & IS_LONG))
					PRINT_IT (ap->idx);
			}


			if (FLAGS (s, ue->idx) & IS_LONG)
				PRINT_IT (ue->idx);

			/* repeat the above loop, this time for long aliases. */
			for (ap = ue->alias; ap; ap = ap->next) {
				if (FLAGS (s, ap->idx) & IS_LONG)
					PRINT_IT (ap->idx);
			}

			/* pad to desccol */
			PRINT_SPACES (fp, desccol - nchars);

			/* Print description, wrapped to desc_col_width columns. */
			if (1) {
				const char *pstart;

				pstart = DESC (s, ue->idx);
				while (1) {
					int     n = 0;
					const char *lastws = NULL, *p;

					p = pstart;

					while (*p && n < desc_col_width
					       && *p != '\n') {
						if (isspace ((unsigned char)(*p))
						    || *p == '-') lastws =
								p;
						n++;
						p++;
					}

					if (!*p) {	/* hit end of desc. done. */
						fprintf (fp, "%s\n",
							 pstart);
						break;
					}
					else if (*p == '\n') {	/* print everything up to here then wrap. */
						fprintf (fp, "%.*s\n", n,
							 pstart);
						PRINT_SPACES (fp, desccol);
						pstart = p + 1;
						continue;
					}
					else {	/* we hit the edge of the screen. wrap at space if possible. */
						if (lastws) {
							fprintf (fp,
								 "%.*s\n",
								 (int)(lastws - pstart),
								 pstart);
							pstart =
								lastws + 1;
						}
						else {
							fprintf (fp,
								 "%.*s\n",
								 n,
								 pstart);
							pstart = p + 1;
						}
						PRINT_SPACES (fp, desccol);
						continue;
					}
				}
			}
		}
	}			/* end while */
	free (store);
	return 0;
}
#endif /* no scanopt_usage */


static void scanopt_err(struct _scanopt_t *s, int is_short, int err)
{
	const char *optname = "";
	char    optchar[2];

	if (!s->no_err_msg) {

		if (s->index > 0 && s->index < s->argc) {
			if (is_short) {
				optchar[0] =
					s->argv[s->index][s->subscript];
				optchar[1] = '\0';
				optname = optchar;
			}
			else {
				optname = s->argv[s->index];
			}
		}

		fprintf (stderr, "%s: ", s->argv[0]);
		switch (err) {
		case SCANOPT_ERR_ARG_NOT_ALLOWED:
			fprintf (stderr,
				 _
				 ("option `%s' doesn't allow an argument\n"),
				 optname);
			break;
		case SCANOPT_ERR_ARG_NOT_FOUND:
			fprintf (stderr,
				 _("option `%s' requires an argument\n"),
				 optname);
			break;
		case SCANOPT_ERR_OPT_AMBIGUOUS:
			fprintf (stderr, _("option `%s' is ambiguous\n"),
				 optname);
			break;
		case SCANOPT_ERR_OPT_UNRECOGNIZED:
			fprintf (stderr, _("Unrecognized option `%s'\n"),
				 optname);
			break;
		default:
			fprintf (stderr, _("Unknown error=(%d)\n"), err);
			break;
		}
	}
}


/* Internal. Match str against the regex  ^--([^=]+)(=(.*))?
 * return 1 if *looks* like a long option.
 * 'str' is the only input argument, the rest of the arguments are output only.
 * optname will point to str + 2
 *
 */
static int matchlongopt (char *str/*待分析的长选项*/, char **optname/*出参，选项名称*/, int *optlen/*出参，选项名称长度*/, char **arg/*出参，选项参数*/, int *arglen/*出参，选项参数长度*/)
{
	char   *p;

	*optname = *arg = NULL;
	*optlen = *arglen = 0;

	/* Match regex /--./   */
	p = str;
	if (p[0] != '-' || p[1] != '-' || !p[2])
	    /*遇到本函数不处理的情况，返回0*/
		return 0;

	/*跳过'--'*/
	p += 2;
	*optname = p;/*指定选项名*/

	/* find the end of optname */
	while (*p && *p != '=')
		++p;

	/*确定选项名称长度*/
	*optlen = (int) (p - *optname);

	/*此选项没有'='号部分*/
	if (!*p)
		/* an option with no '=...' part. */
		return 1;


	/* We saw an '=' char. The rest of p is the arg. */
	p++;/*跳过'='符*/
	*arg = p;/*标记选项参数*/
	while (*p)
		++p;
	*arglen = (int) (p - *arg);/*指明选项参数长度*/

	return 1;
}


/* Internal. Look up long or short option by name.
 * Long options must match a non-ambiguous prefix, or exact match.
 * Short options must be exact.
 * Return boolean true if found and no error.
 * Error stored in err_code or zero if no error. */
static int find_opt (struct _scanopt_t *s, int lookup_long, char *optstart, int
	len/*待查找选项长度*/, int *err_code, int *opt_offset/*出参，匹配的选项索引*/)
{
	int     nmatch = 0, lastr_val = 0, i;

	*err_code = 0;
	*opt_offset = -1;

	if (!optstart)
	    /*指定为空，直接返回*/
		return 0;

	for (i = 0; i < s->optc; i++) {
		const char   *optname;

		/*指向选项名称*/
		optname = s->options[i].opt_fmt + (lookup_long ? 2 : 1);

		if (lookup_long && (s->aux[i].flags & IS_LONG)) {
			if (len > s->aux[i].namelen)
			    /*待查找的长度与此选项不符，忽略*/
				continue;

			if (strncmp (optname, optstart, (size_t) len) == 0) {
			    /*与当前选项匹配*/
				nmatch++;
				*opt_offset = i;

				/* exact match overrides all. */
				if (len == s->aux[i].namelen) {
				    /*严格匹配，跳出*/
					nmatch = 1;
					break;
				}

				/* ambiguity is ok between aliases. */
				if (lastr_val
				    && lastr_val ==
				    s->options[i].r_val) nmatch--;
				lastr_val = s->options[i].r_val;
			}
		}
		else if (!lookup_long && !(s->aux[i].flags & IS_LONG)) {
			if (optname[0] == optstart[0]) {
			    /*短名称匹配*/
				nmatch++;
				*opt_offset = i;
			}
		}
	}

	if (nmatch == 0) {
	    /*匹配失败，置error code*/
		*err_code = SCANOPT_ERR_OPT_UNRECOGNIZED;
		*opt_offset = -1;
	}
	else if (nmatch > 1) {
	    /*匹配失败，有多个匹配情况*/
		*err_code = SCANOPT_ERR_OPT_AMBIGUOUS;
		*opt_offset = -1;
	}

	return *err_code ? 0 : 1;/*匹配成功返回1*/
}


/*实现参数解析*/
int     scanopt (scanopt_t *svoid/*支持的选项*/, char **arg, int *optindex)
{
	char   *optname = NULL, *optarg = NULL, *pstart;
	int     namelen = 0, arglen = 0;
	int     errcode = 0, has_next;
	const optspec_t *optp;
	struct _scanopt_t *s;
	struct _aux *auxp;
	int     is_short;
	int     opt_offset = -1;

	s = (struct _scanopt_t *) svoid;

	/* Normalize return-parameters. */
	SAFE_ASSIGN (arg, NULL);
	SAFE_ASSIGN (optindex, s->index);

	if (s->index >= s->argc)
	    /*当前分析位置已超过参数上限，退出*/
		return 0;

	/* pstart always points to the start of our current scan. */
	pstart = s->argv[s->index] + s->subscript;
	if (!pstart)
	    /*此位置参数为空，退出*/
		return 0;

	if (s->subscript == 0) {

		/* test for exact match of "--" */
		if (pstart[0] == '-' && pstart[1] == '-' && !pstart[2]) {
		    /*遇到'--' 参数*/
			SAFE_ASSIGN (optindex, s->index + 1);
			INC_INDEX (s, 1);
			return 0;
		}

		/* Match an opt. */
		if (matchlongopt
		    (pstart/*参数起始位置*/, &optname, &namelen, &optarg, &arglen)) {

			/* it LOOKS like an opt, but is it one?! */
			if (!find_opt
			    (s, 1/*指明查长选项*/, optname, namelen, &errcode,
			     &opt_offset)) {
			    /*匹配失败，返回error code*/
				scanopt_err (s, 0, errcode);
				return errcode;
			}
			/* We handle this below. */
			is_short = 0;

			/* Check for short opt.  */
		}
		else if (pstart[0] == '-' && pstart[1]) {
			/* Pass through to below. */
			is_short = 1;
			s->subscript++;
			pstart++;
		}

		else {
			/* It's not an option. We're done. */
			return 0;/*不是一个选项*/
		}
	}

	/* We have to re-check the subscript status because it
	 * may have changed above. */

	if (s->subscript != 0) {

		/* we are somewhere in a run of short opts,
		 * e.g., at the 'z' in `tar -xzf` */

		optname = pstart;
		namelen = 1;
		is_short = 1;

		if (!find_opt
		    (s, 0/*短选项*/, pstart, namelen, &errcode, &opt_offset)) {
		    /*匹配短选项失败*/
			scanopt_err(s, 1, errcode);
			return errcode;
		}

		optarg = pstart + 1;
		if (!*optarg) {
			optarg = NULL;
			arglen = 0;
		}
		else
			arglen = (int) strlen (optarg);
	}

	/* At this point, we have a long or short option matched at opt_offset into
	 * the s->options array (and corresponding aux array).
	 * A trailing argument is in {optarg,arglen}, if any.
	 */

	/* Look ahead in argv[] to see if there is something
	 * that we can use as an argument (if needed). */
	has_next = s->index + 1 < s->argc;

	optp = s->options + opt_offset;/*命中的选项*/
	auxp = s->aux + opt_offset;/*此选项对应的aux*/

	/* case: no args allowed */
	if (auxp->flags & ARG_NONE) {
		if (optarg && !is_short) {
			scanopt_err(s, is_short, SCANOPT_ERR_ARG_NOT_ALLOWED);
			INC_INDEX (s, 1);
			return SCANOPT_ERR_ARG_NOT_ALLOWED;
		}
		else if (!optarg)
			INC_INDEX (s, 1);
		else
			s->subscript++;
		return optp->r_val;
	}

	/* case: required */
	if (auxp->flags & ARG_REQ) {
		if (!optarg && !has_next) {
			scanopt_err(s, is_short, SCANOPT_ERR_ARG_NOT_FOUND);
			return SCANOPT_ERR_ARG_NOT_FOUND;
		}

		if (!optarg) {
			/* Let the next argv element become the argument. */
			SAFE_ASSIGN (arg, s->argv[s->index + 1]);/*设置参数值*/
			INC_INDEX (s, 2);
		}
		else {
			SAFE_ASSIGN (arg, (char *) optarg);
			INC_INDEX (s, 1);
		}
		return optp->r_val;
	}

	/* case: optional */
	if (auxp->flags & ARG_OPT) {
		SAFE_ASSIGN (arg, optarg);
		INC_INDEX (s, 1);
		return optp->r_val;
	}


	/* Should not reach here. */
	return 0;
}


int     scanopt_destroy (scanopt_t *svoid)
{
	struct _scanopt_t *s;

	s = (struct _scanopt_t *) svoid;
	if (s != NULL) {
		free(s->aux);
		free(s);
	}
	return 0;
}


/* vim:set tabstop=8 softtabstop=4 shiftwidth=4: */
