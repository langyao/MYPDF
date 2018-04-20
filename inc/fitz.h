#ifndef _FITZ_H_
#define _FITZ_H_

/*
 * Include the standard libc headers.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>	/* INT_MAX & co */
#include <float.h>	/* FLT_EPSILON */
#include <fcntl.h>	/* O_RDONLY & co */

#define nil ((void*)0)

#define nelem(x) (sizeof(x)/sizeof((x)[0]))

/*
 * Some differences in libc can be smoothed over
 */

#ifdef _MSC_VER /* Microsoft Visual C */

#pragma warning( disable: 4244 ) /* conversion from X to Y, possible loss of data */
#pragma warning( disable: 4996 ) /* The POSIX name for this item is deprecated */
#pragma warning( disable: 4996 ) /* This function or variable may be unsafe */

#include <io.h>

int gettimeofday(struct timeval *tv, struct timezone *tz);

#define snprintf _snprintf
#define strtoll _strtoi64

#else /* Unix or close enough */

#include <unistd.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

#endif



/*
 * Variadic macros, inline and restrict keywords
 */

#if __STDC_VERSION__ == 199901L /* C99 */

#define fz_throw(...) fz_throwimp(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define fz_rethrow(cause, ...) fz_rethrowimp(__FILE__, __LINE__, __func__, cause, __VA_ARGS__)
#define fz_catch(cause, ...) fz_catchimp(__FILE__, __LINE__, __func__, cause, __VA_ARGS__)

#elif _MSC_VER >= 1500 /* MSVC 9 or newer */

#define inline __inline
#define restrict __restrict
#define fz_throw(...) fz_throwimp(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define fz_rethrow(cause, ...) fz_rethrowimp(__FILE__, __LINE__, __FUNCTION__, cause, __VA_ARGS__)
#define fz_catch(cause, ...) fz_catchimp(__FILE__, __LINE__, __FUNCTION__, cause, __VA_ARGS__)

#elif __GNUC__ >= 3 /* GCC 3 or newer */

#define inline __inline
#define restrict __restrict
#define fz_throw(fmt...) fz_throwimp(__FILE__, __LINE__, __FUNCTION__, fmt)
#define fz_rethrow(cause, fmt...) fz_rethrowimp(__FILE__, __LINE__, __FUNCTION__, cause, fmt)
#define fz_catch(cause, fmt...) fz_catchimp(__FILE__, __LINE__, __FUNCTION__, cause, fmt)

#else /* Unknown or ancient */

#define inline
#define restrict
#define fz_throw fz_throwimpx
#define fz_rethrow fz_rethrowimpx
#define fz_catch fz_catchimpx

#endif

/*
 * GCC can do type checking of printf strings
 */

#ifndef __printflike
#if __GNUC__ > 2 || __GNUC__ == 2 && __GNUC_MINOR__ >= 7
#define __printflike(fmtarg, firstvararg) \
	__attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#else
#define __printflike(fmtarg, firstvararg)
#endif
#endif

/*
 * Error handling
 */

typedef int fz_error;

void fz_warn(char *fmt, ...) __printflike(1, 2);

fz_error fz_throwimp(const char *file, int line, const char *func, char *fmt, ...) __printflike(4, 5);
fz_error fz_rethrowimp(const char *file, int line, const char *func, fz_error cause, char *fmt, ...) __printflike(5, 6);
void fz_catchimp(const char *file, int line, const char *func, fz_error cause, char *fmt, ...) __printflike(5, 6);

fz_error fz_throwimpx(char *fmt, ...) __printflike(1, 2);
fz_error fz_rethrowimpx(fz_error cause, char *fmt, ...) __printflike(2, 3);
void fz_catchimpx(fz_error cause, char *fmt, ...) __printflike(2, 3);

#define fz_okay ((fz_error)0)
/*
 * Basic runtime and utility functions
 */

#define ABS(x) ( (x) < 0 ? -(x) : (x) )
#define MIN(a,b) ( (a) < (b) ? (a) : (b) )
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )
#define CLAMP(x,a,b) ( (x) > (b) ? (b) : ( (x) < (a) ? (a) : (x) ) )

/* memory allocation */
void *fz_malloc(int size);
void *fz_calloc(int count, int size);
void *fz_realloc(void *p, int count, int size);
void fz_free(void *p);
char *fz_strdup(char *s);

/* runtime (hah!) test for endian-ness */
int fz_isbigendian(void);

/* safe string functions */
char *fz_strsep(char **stringp, const char *delim);
int fz_strlcpy(char *dst, const char *src, int n);
int fz_strlcat(char *dst, const char *src, int n);

/* utf-8 encoding and decoding */
int chartorune(int *rune, char *str);
int runetochar(char *str, int *rune);
int runelen(int c);


/*
 * Generic hash-table with fixed-length keys.
 */

typedef struct fz_hashtable_s fz_hashtable;

fz_hashtable *fz_newhash(int initialsize, int keylen);
void fz_debughash(fz_hashtable *table);
void fz_emptyhash(fz_hashtable *table);
void fz_freehash(fz_hashtable *table);

void *fz_hashfind(fz_hashtable *table, void *key);
void fz_hashinsert(fz_hashtable *table, void *key, void *val);
void fz_hashremove(fz_hashtable *table, void *key);

int fz_hashlen(fz_hashtable *table);
void *fz_hashgetkey(fz_hashtable *table, int idx);
void *fz_hashgetval(fz_hashtable *table, int idx);

/*
 * Basic crypto functions.
 * Independent of the rest of fitz.
 * For further encapsulation in filters, or not.
 */

/* md5 digests */

typedef struct fz_md5_s fz_md5;

struct fz_md5_s
{
	unsigned int state[4];
	unsigned int count[2];
	unsigned char buffer[64];
};

void fz_md5init(fz_md5 *state);
void fz_md5update(fz_md5 *state, const unsigned char *input, const unsigned inlen);
void fz_md5final(fz_md5 *state, unsigned char digest[16]);

/* sha-256 digests */

typedef struct fz_sha256_s fz_sha256;

struct fz_sha256_s
{
	unsigned int state[8];
	unsigned int count[2];
	union {
		unsigned char u8[64];
		unsigned int u32[16];
	} buffer;
};

void fz_sha256init(fz_sha256 *state);
void fz_sha256update(fz_sha256 *state, const unsigned char *input, unsigned int inlen);
void fz_sha256final(fz_sha256 *state, unsigned char digest[32]);

/* arc4 crypto */

typedef struct fz_arc4_s fz_arc4;

struct fz_arc4_s
{
	unsigned x;
	unsigned y;
	unsigned char state[256];
};

void fz_arc4init(fz_arc4 *state, const unsigned char *key, const unsigned len);
void fz_arc4encrypt(fz_arc4 *state, unsigned char *dest, const unsigned char *src, const unsigned len);

/* AES block cipher implementation from XYSSL */

typedef struct fz_aes_s fz_aes;

#define AES_DECRYPT 0
#define AES_ENCRYPT 1

struct fz_aes_s
{
	int nr; /* number of rounds */
	unsigned long *rk; /* AES round keys */
	unsigned long buf[68]; /* unaligned data */
};

void aes_setkey_enc( fz_aes *ctx, const unsigned char *key, int keysize );
void aes_setkey_dec( fz_aes *ctx, const unsigned char *key, int keysize );
void aes_crypt_cbc( fz_aes *ctx, int mode, int length,
	unsigned char iv[16],
	const unsigned char *input,
	unsigned char *output );

/*
 * Dynamic objects.
 * The same type of objects as found in PDF and PostScript.
 * Used by the filters and the pdf parser.
 */

typedef struct fz_obj_s fz_obj;
typedef struct fz_keyval_s fz_keyval;

struct pdf_xref_s;

typedef enum fz_objkind_e
{
	FZ_NULL,
	FZ_BOOL,
	FZ_INT,
	FZ_REAL,
	FZ_STRING,
	FZ_NAME,
	FZ_ARRAY,
	FZ_DICT,
	FZ_INDIRECT
} fz_objkind;

struct fz_keyval_s
{
	fz_obj *k;
	fz_obj *v;
};

struct fz_obj_s
{
	int refs;
	fz_objkind kind;
	union
	{
		int b;
		int i;
		float f;
		struct {
			unsigned short len;
			char buf[1];
		} s;
		char n[1];
		struct {
			int len;
			int cap;
			fz_obj **items;
		} a;
		struct {
			char sorted;
			int len;
			int cap;
			fz_keyval *items;
		} d;
		struct {
			int num;
			int gen;
			struct pdf_xref_s *xref;
		} r;
	} u;
};

fz_obj *fz_newnull(void);
fz_obj *fz_newbool(int b);
fz_obj *fz_newint(int i);
fz_obj *fz_newreal(float f);
fz_obj *fz_newname(char *str);
fz_obj *fz_newstring(char *str, int len);
fz_obj *fz_newindirect(int num, int gen, struct pdf_xref_s *xref);

fz_obj *fz_newarray(int initialcap);
fz_obj *fz_newdict(int initialcap);
fz_obj *fz_copyarray(fz_obj *array);
fz_obj *fz_copydict(fz_obj *dict);

fz_obj *fz_keepobj(fz_obj *obj);
void fz_dropobj(fz_obj *obj);

/* type queries */
int fz_isnull(fz_obj *obj);
int fz_isbool(fz_obj *obj);
int fz_isint(fz_obj *obj);
int fz_isreal(fz_obj *obj);
int fz_isname(fz_obj *obj);
int fz_isstring(fz_obj *obj);
int fz_isarray(fz_obj *obj);
int fz_isdict(fz_obj *obj);
int fz_isindirect(fz_obj *obj);

int fz_objcmp(fz_obj *a, fz_obj *b);

fz_obj *fz_resolveindirect(fz_obj *obj);

/* silent failure, no error reporting */
int fz_tobool(fz_obj *obj);
int fz_toint(fz_obj *obj);
float fz_toreal(fz_obj *obj);
char *fz_toname(fz_obj *obj);
char *fz_tostrbuf(fz_obj *obj);
int fz_tostrlen(fz_obj *obj);
int fz_tonum(fz_obj *obj);
int fz_togen(fz_obj *obj);

int fz_arraylen(fz_obj *array);
fz_obj *fz_arrayget(fz_obj *array, int i);
void fz_arrayput(fz_obj *array, int i, fz_obj *obj);
void fz_arraypush(fz_obj *array, fz_obj *obj);
void fz_arraydrop(fz_obj *array);
void fz_arrayinsert(fz_obj *array, fz_obj *obj);

int fz_dictlen(fz_obj *dict);
fz_obj *fz_dictgetkey(fz_obj *dict, int idx);
fz_obj *fz_dictgetval(fz_obj *dict, int idx);
fz_obj *fz_dictget(fz_obj *dict, fz_obj *key);
fz_obj *fz_dictgets(fz_obj *dict, char *key);
fz_obj *fz_dictgetsa(fz_obj *dict, char *key, char *abbrev);
void fz_dictput(fz_obj *dict, fz_obj *key, fz_obj *val);
void fz_dictputs(fz_obj *dict, char *key, fz_obj *val);
void fz_dictdel(fz_obj *dict, fz_obj *key);
void fz_dictdels(fz_obj *dict, char *key);
void fz_sortdict(fz_obj *dict);

int fz_fprintobj(FILE *fp, fz_obj *obj, int tight);
void fz_debugobj(fz_obj *obj);
void fz_debugref(fz_obj *obj);

char *fz_objkindstr(fz_obj *obj);

/*
 * Data buffers.
 */

typedef struct fz_buffer_s fz_buffer;

struct fz_buffer_s
{
	int refs;
	unsigned char *data;
	int cap, len;
};

fz_buffer *fz_newbuffer(int size);
fz_buffer *fz_keepbuffer(fz_buffer *buf);
void fz_dropbuffer(fz_buffer *buf);

void fz_resizebuffer(fz_buffer *buf, int size);
void fz_growbuffer(fz_buffer *buf);

/*
 * Buffered reader.
 * Only the data between rp and wp is valid data.
 */

typedef struct fz_stream_s fz_stream;

struct fz_stream_s
{
	int refs;
	int dead;
	int pos;
	int avail;
	int bits;
	unsigned char *bp, *rp, *wp, *ep;
	void *state;
	int (*read)(fz_stream *stm, unsigned char *buf, int len);
	void (*close)(fz_stream *stm);
	void (*seek)(fz_stream *stm, int offset, int whence);
	unsigned char buf[4096];
};

fz_stream *fz_openfile(int file);
fz_stream *fz_openbuffer(fz_buffer *buf);
void fz_close(fz_stream *stm);

fz_stream *fz_newstream(void*, int(*)(fz_stream*, unsigned char*, int), void(*)(fz_stream *));
fz_stream *fz_keepstream(fz_stream *stm);
void fz_fillbuffer(fz_stream *stm);

int fz_tell(fz_stream *stm);
void fz_seek(fz_stream *stm, int offset, int whence);

int fz_read(fz_stream *stm, unsigned char *buf, int len);
void fz_readline(fz_stream *stm, char *buf, int max);
fz_error fz_readall(fz_buffer **bufp, fz_stream *stm, int initial);

static inline int fz_readbyte(fz_stream *stm)
{
	if (stm->rp == stm->wp)
	{
		fz_fillbuffer(stm);
		return stm->rp < stm->wp ? *stm->rp++ : EOF;
	}
	return *stm->rp++;
}

static inline int fz_peekbyte(fz_stream *stm)
{
	if (stm->rp == stm->wp)
	{
		fz_fillbuffer(stm);
		return stm->rp < stm->wp ? *stm->rp : EOF;
	}
	return *stm->rp;
}

static inline void fz_unreadbyte(fz_stream *stm)
{
	if (stm->rp > stm->bp)
		stm->rp--;
}

static inline unsigned int fz_readbits(fz_stream *stm, int n)
{
	unsigned int x;

	if (n <= stm->avail)
	{
		stm->avail -= n;
		x = (stm->bits >> stm->avail) & ((1 << n) - 1);
	}
	else
	{
		x = stm->bits & ((1 << stm->avail) - 1);
		n -= stm->avail;
		stm->avail = 0;

		while (n > 8)
		{
			x = (x << 8) | fz_readbyte(stm);
			n -= 8;
		}

		if (n > 0)
		{
			stm->bits = fz_readbyte(stm);
			stm->avail = 8 - n;
			x = (x << n) | (stm->bits >> stm->avail);
		}
	}

	return x;
}

/*
 * Data filters.
 */

fz_stream *fz_opencopy(fz_stream *chain);
fz_stream *fz_opennull(fz_stream *chain, int len);
fz_stream *fz_openarc4(fz_stream *chain, unsigned char *key, unsigned keylen);
fz_stream *fz_openaesd(fz_stream *chain, unsigned char *key, unsigned keylen);
fz_stream *fz_opena85d(fz_stream *chain);
fz_stream *fz_openahxd(fz_stream *chain);
fz_stream *fz_openrld(fz_stream *chain);
fz_stream *fz_opendctd(fz_stream *chain, fz_obj *param);
fz_stream *fz_openfaxd(fz_stream *chain, fz_obj *param);
fz_stream *fz_openflated(fz_stream *chain);
fz_stream *fz_openlzwd(fz_stream *chain, fz_obj *param);
fz_stream *fz_openpredict(fz_stream *chain, fz_obj *param);
fz_stream *fz_openjbig2d(fz_stream *chain, fz_buffer *global);

/*
 * Resources and other graphics related objects.
 */

enum { FZ_MAXCOLORS = 32 };

extern const char *fz_blendnames[];


/*
 * Fonts come in two variants:
 *	Regular fonts are handled by FreeType.
 *	Type 3 fonts have callbacks to the interpreter.
 */


typedef struct fz_font_s fz_font;
char *ft_errorstring(int err);

struct fz_font_s
{
	int refs;
	char name[32];

	void *ftface; /* has an FT_Face if used */
	int ftsubstitute; /* ... substitute metrics */
	int fthint; /* ... force hinting for DynaLab fonts */

	/* origin of font data */
	char *ftfile;
	unsigned char *ftdata;
	int ftsize;

	fz_obj *t3resources;
	fz_buffer **t3procs; /* has 256 entries if used */
	float *t3widths; /* has 256 entries if used */
	void *t3xref; /* a pdf_xref for the callback */
	int widthcount;
	int *widthtable;
};

fz_font *fz_newtype3font(char *name);

fz_error fz_newfontfrombuffer(fz_font **fontp, unsigned char *data, int len, int index);
fz_error fz_newfontfromfile(fz_font **fontp, char *path, int index);

fz_font *fz_keepfont(fz_font *font);
void fz_dropfont(fz_font *font);

void fz_debugfont(fz_font *font);





#endif
