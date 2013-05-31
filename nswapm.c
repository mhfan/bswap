/****************************************************************
 * $ID: nswapm.c       Sun, 25 Sep 2005 22:05:23 +0800  mhfan $ *
 *                                                              *
 * Description:                                                 *
 *                                                              *
 * Maintainer:  ∑∂√¿ª‘(Meihui Fan)  <mhfan@ustc.edu>            *
 *                                                              *
 * CopyRight (c)  2005  M.H.Fan                                 *
 *   All rights reserved.                                       *
 *                                                              *
 * This file is free software;                                  *
 *   you are free to modify and/or redistribute it   	        *
 *   under the terms of the GNU General Public Licence (GPL).   *
 *                                                              *
 * Last modified: Thu, 23 Mar 2006 16:21:17 +0800      by mhfan #
 ****************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <libgen.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define	COMPILED_STANDALONE		1

#if 1   /* defined by mhfan */
#include <stdio.h>

#define dtrace do { fprintf(stderr, "\033[36mTRACE"       \
			"\033[1;34m==>\033[33m%16s"       \
			"\033[36m: \033[32m%4d\033[36m: " \
			"\033[35m%-24s \033[34m"          \
			"[\033[0;37m%s\033[1;34m,"        \
			" \033[0;36m%s\033[1;34m]"        \
			"\033[0m\n", __FILE__, __LINE__,  \
			__FUNCTION__ /* __func__ */,      \
			__TIME__, __DATE__);              \
		} while (0)

#define dprintp(a,n) do { int i, m=sizeof((a)[0]);        \
	fprintf(stderr, "\033[33m" #a ": \033[36m"    \
			"%p\033[0m ==>\n", a);		  \
	    m = (m<2 ? 24 : (m<4 ? 16 : 8));	  \
	    for (i=0; i < n; ) {	      \
			    int j=(i+m > n ? n-i : m);	      \
			    for (; j--; ++i)		  \
			    if (m > 16) fprintf(stderr,	  \
						    "%02x ", (a)[i]); else    \
			    if (m > 8) fprintf(stderr,	  \
						    "%04x ", (a)[i]); else    \
			    fprintf(stderr, "%08x ", (a)[i]); \
			    fprintf(stderr, "\n"); }	      \
	} while (0)

#define dprintn(a) do { fprintf(stderr, "\033[33m" #a     \
			": \033[36m%#x, %d, %g\033[0m\n"  \
			, a, a, a);                       \
		} while (0)

#define dprints(a) do { fprintf(stderr, "\033[33m" #a     \
			": \033[36m%s\033[0m\n", a);      \
		} while (0)
#endif  /* defined by mhfan */
 
#if 0
void bswapm(uint8_t* p, size_t n, int m)
{
    int a, b, s;
    if (n < m) m = n;
    for (a=n/m, s=m; a--; m=s) {
	for (b=(m>>1); b--; ++p) {
	    unsigned char t=*p;
	    *p = *(p+(--m));
	    *(p+m) = t;
	}   p += m;
    }
    if ((n%=m)) swapm(p, n, n);
}

inline void bswapw(uint16_t* p, size_t n)
{
    for (n>>=1; n--; ++p) {
	uint16_t t=*p;
	*p = ((t >> 8) | ((t & 0x00ff) << 8));	    // XXX
    }
}

inline void bswapl(uint32_t* p, size_t n)
{
    for (n>>=2; n--; ++p) {
	uint32_t t=*p;
	*p = ((t >> 24) | ((t & 0x000000ff) << 24)  // XXX
		| ((t & 0x00ff0000) >> 8) | ((t & 0x0000ff00) << 8));
    }
}
#endif

int swap_mem(int unit, int many, ssize_t size, unsigned char* iptr,	// XXX
	unsigned char* optr)
{
    int n, half;

    many *= unit, half = (many >> 1);

    if (iptr == optr) {
	unsigned char* sbuf;

	if (!(sbuf = malloc(unit))) {
	    fprintf(stderr, "Fail to alocate memory of %d bytes\n", unit);
	    return -1;
	}

	while (!((size -= many) < 0)) {
	    optr += many - unit;

	    for (n=0; n < half; n += unit) {
		memcpy(sbuf, iptr, unit);
		memcpy(iptr, optr, unit);
		memcpy(optr, sbuf, unit);
		iptr += unit, optr -= unit;
	    }

	    iptr += many - half, optr += half + unit;
	}

	free(sbuf);
    } else {
	while (!((size -= many) < 0)) {
	    optr += many - unit;

	    for (n=0; n < half; n += unit) {
		memcpy(optr, iptr, unit);
		iptr += unit, optr -= unit;
	    }

	    iptr += many - half, optr += half + unit;
	}
    }

    if ((size += many) > 0) {	// XXX
	fprintf(stderr, "Memory buffer is not aligned by %d bytes: %d\n",
		many, size);	return 1;
    }

    return 0;
}

int swap_fs(int unit, int many, ssize_t size, FILE* ifs, FILE* ofs)	// XXX
{
    int m=0;
    unsigned char* buf;

    if (many == INT_MAX) {
	long len;			buf = calloc(unit, 1);

	if (fseek(ifs, 0, SEEK_END) < 0 || (len = ftell(ifs)) < 0 ||
		((m = len % unit) && (fread (buf, 1, m, ifs) < m ||
				      fwrite(buf, 1, m, ofs) < m))) goto IOE;

	for (len -= m; len > 0; len -= unit)
	    if (fread (buf, 1, unit, ifs) < unit ||
		fwrite(buf, 1, unit, ofs) < unit)		    goto IOE;

	goto OUT;

IOE:	fprintf(stderr, "File IO error: %s\n", strerror(errno));    goto ERR;
    }

    if (!(buf = malloc((m = unit * many)))) {
	fprintf(stderr, "Fail to alocate memory of %d bytes\n", m);
	return -1;
    }

    for (size = (size / m) + ((size % m) > 0); size > 0; --size) {
	int c, e;

	for (c=0; c < many; c += e) {
	    if ((e = fread((buf + c * unit), unit, (many - c), ifs)) < 0) {
		fprintf(stderr, "Fail to read from file: %s\n",
			strerror(errno));   goto ERR;
	    } else if (e > 0) { ; } else {
		if (c == 0) { free(buf);    return 0; }
		if (c < m) { memset((buf + c), 0, (m - c)); size = 1; }	break;
	    }
	}

	if ((e = swap_mem(unit, many, m, buf, buf)) < 0) {
	    fprintf(stderr, "Fail to swap by %d units in %d bytes\n",
		    many, unit);	    goto ERR;
	} else
	if (e > 0) fprintf(stderr, "Something isn't right while swapping.\n");

	for (c=0; c < many; c += e) {
	    if ((e = fwrite((buf + c * unit), unit, (many - c), ofs)) < 0) {
		fprintf(stderr, "Fail to write to file: %s\n",
			strerror(errno));   goto ERR;
	    } else if (e > 0) { ; } else    break;	// XXX
	}
    }

OUT:
    free(buf);

    return 0;

ERR:
    free(buf);

    return 1;
}

int swap_fn(int unit, int many, ssize_t size, char* ifn, char* ofn)	// XXX
{
    FILE *ifs, *ofs;

    if (!strcmp(ifn, "-")  || !strcmp(ifn, "stdin")) ifs = stdin;
    else if ((ifs = fopen(ifn, "r")) < 0) {
	fprintf(stderr, "Fail to open %s: %s\n", ifn, strerror(errno));
					return -1;
    }

    if (!strcmp(ofn, "-") || !strcmp(ofn, "stdout")) ofs = stdout;
    else if ((ofs = fopen(ofn, "w")) < 0) {
	fprintf(stderr, "Fail to open %s: %s\n", ofn, strerror(errno));
	fclose(ifs);			return -1;
    }

    return swap_fs(unit, many, size, ifs, ofs);
}

#ifdef	COMPILED_STANDALONE
int main(int argc, char* argv[])
{
    int i=0;
    int unit=-1, many=-1;
    ssize_t size=INT_MAX;	//basename(argv[0]);
    char *ifn="-", *ofn="-", *self=strrchr(argv[0], '/')+1;

    if (self) ++self; else self=argv[0];

    switch (self[0]) {
    case 'n': unit = 0; break;
    case 'L': unit = 8; break;
    case 'l': unit = 4; break;
    case 'w': unit = 2; break;
    case 'b': unit = 1; break;
    }

    if (!strncmp(self, "bswap", 5)) {
	if (self[5] == '.') ++self;
	switch (self[5]) {
	case 'm': many = 0; break;
	case 'w': many = 2; break;
	case 'l': many = 4; break;
	case 'L': many = 8; break;
	case 'a': many = INT_MAX ; break;
	}
    }

    if ((many < 0 || unit < 0) && argc < 2) goto USG;
    while (++i < argc) {		// TODO: parse arguments here
        if (argv[i][0] != '-' || !argv[i][1]) {
	    ifn = argv[i];	continue;	}
	switch (argv[i][1]) {
	case 'n': if (++i < argc) switch (argv[i][0]) {
		    case 'b': unit = 1; break;
		    case 'w': unit = 2; break;
		    case 'l': unit = 4; break;
		    case 'L': unit = 8; break;
		    default : unit = atoi(argv[i]);
		  }			break;
	case 'm': if (++i < argc) switch (argv[i][0]) {
		    case 'w': many = 2; break;
		    case 'l': many = 4; break;
		    case 'L': many = 8; break;
		    case 'a': many = INT_MAX ; break;
		    default : many = atoi(argv[i]);
		  }			break;
	case 'o': if (++i < argc) ofn  = argv[i]; else goto USG; break;
	case 's': if (++i < argc) size = atoi(argv[i]); else goto USG; break;
	case 'h': default :		goto USG;
	}
    }

    if (unit < 1 || many < 1) {
	fprintf(stderr, "I don't know how to swap: unit=%d, many=%d\n",
		unit, many);		goto USG;
    } else
    if (many == 1) fprintf(stderr, "Nothing to do with one unit swapping\n");

    fprintf(stderr, "Swapping by %d units in %d bytes ...\n", many, unit);

    if (swap_fn(unit, many, size, ifn, ofn) < 0)
	fprintf(stderr, "Swapping failed\n");
    else
#if 0
	fprintf(stderr, "Swapped %d bytes by %d units in %d bytes\n",
		size, many, unit);
#else
	fprintf(stderr, "Swapped done(corrupted)\n");
#endif

    return 0;

USG:
    fprintf(stderr, "Usage:  [b|w|l|L|n]swap[[.]w|l|L|a|m] [-h] "
	    "[-n b|w|l|L|<n>] [-m w|l|L|a|<m>]\n\t[-s <size>] "
	    "[-o <output-file>] [input-file]\n");

    return 1;
}
#endif//COMPILED_STANDALONE

/********************* End Of File: nswapm.c ********************/
/* vim: set sts=4:                                              */
