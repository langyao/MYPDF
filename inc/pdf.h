#ifndef _MUPDF_H_
#define _MUPDF_H_


#include "fitz.h"

typedef struct pdf_xref_s pdf_xref;

void pdf_logxref(char *fmt, ...);
void pdf_logrsrc(char *fmt, ...);
void pdf_logfont(char *fmt, ...);
void pdf_logpage(char *fmt, ...);


enum
{
	PDF_TERROR, PDF_TEOF,
	PDF_TOARRAY, PDF_TCARRAY,
	PDF_TODICT, PDF_TCDICT,
	PDF_TOBRACE, PDF_TCBRACE,
	PDF_TNAME, PDF_TINT, PDF_TREAL, PDF_TSTRING, PDF_TKEYWORD,
	PDF_TR, PDF_TTRUE, PDF_TFALSE, PDF_TNULL,
	PDF_TOBJ, PDF_TENDOBJ,
	PDF_TSTREAM, PDF_TENDSTREAM,
	PDF_TXREF, PDF_TTRAILER, PDF_TSTARTXREF,
	PDF_NTOKENS
};


/* lex.c */
fz_error pdf_lex(int *tok, fz_stream *f, char *buf, int n, int *len);

/* parse.c */
fz_error pdf_parsearray(fz_obj **op, pdf_xref *xref, fz_stream *f, char *buf, int cap);
fz_error pdf_parsedict(fz_obj **op, pdf_xref *xref, fz_stream *f, char *buf, int cap);
fz_error pdf_parsestmobj(fz_obj **op, pdf_xref *xref, fz_stream *f, char *buf, int cap);
fz_error pdf_parseindobj(fz_obj **op, pdf_xref *xref, fz_stream *f, char *buf, int cap, int *num, int *gen, int *stmofs);

char *pdf_toutf8(fz_obj *src);
unsigned short *pdf_toucs2(fz_obj *src);
fz_obj *pdf_toutf8name(fz_obj *src);






/*
 * xref and object / stream api
 */

typedef struct pdf_xrefentry_s pdf_xrefentry;

struct pdf_xrefentry_s
{
	int ofs;	/* file offset / objstm object number */
	int gen;	/* generation / objstm index */
	int stmofs;	/* on-disk stream */
	fz_obj *obj;	/* stored/cached object */
	int type;	/* 0=unset (f)ree i(n)use (o)bjstm */
};

struct pdf_xref_s
{
	fz_stream *file;
	int version;
	int startxref;
	int filesize;
	fz_obj *trailer;

	int len;
	pdf_xrefentry *table;

	int pagelen;
	int pagecap;
	fz_obj **pageobjs;
	fz_obj **pagerefs;

	struct pdf_store_s *store;

	char scratch[65536];
};

fz_error pdf_cacheobject(pdf_xref *, int num, int gen);
fz_error pdf_loadobject(fz_obj **objp, pdf_xref *, int num, int gen);
void pdf_updateobject( pdf_xref *xref, int num, int gen, fz_obj *newobj);

int pdf_isstream(pdf_xref *xref, int num, int gen);
fz_stream *pdf_openinlinestream(fz_stream *chain, pdf_xref *xref, fz_obj *stmobj, int length);
fz_error pdf_loadrawstream(fz_buffer **bufp, pdf_xref *xref, int num, int gen);
fz_error pdf_loadstream(fz_buffer **bufp, pdf_xref *xref, int num, int gen);
fz_error pdf_openrawstream(fz_stream **stmp, pdf_xref *, int num, int gen);
fz_error pdf_openstream(fz_stream **stmp, pdf_xref *, int num, int gen);
fz_error pdf_openstreamat(fz_stream **stmp, pdf_xref *xref, int num, int gen, fz_obj *dict, int stmofs);

fz_error pdf_openxrefwithstream(pdf_xref **xrefp, fz_stream *file, char *password);
fz_error pdf_openxref(pdf_xref **xrefp, char *filename, char *password);
void pdf_freexref(pdf_xref *);

/* private */
fz_error pdf_repairxref(pdf_xref *xref, char *buf, int bufsize);
fz_error pdf_repairobjstms(pdf_xref *xref);
void pdf_debugxref(pdf_xref *);
void pdf_resizexref(pdf_xref *xref, int newcap);

/*
 * Resource store
 */

typedef struct pdf_store_s pdf_store;

pdf_store *pdf_newstore(void);
void pdf_freestore(pdf_store *store);

void pdf_storeitem(pdf_store *store, void *keepfn, void *dropfn, fz_obj *key, void *val);
void *pdf_finditem(pdf_store *store, void *dropfn, fz_obj *key);
void pdf_removeitem(pdf_store *store, void *dropfn, fz_obj *key);
void pdf_agestore(pdf_store *store, int maxage);


/*
 * CMap
 */

typedef struct pdf_cmap_s pdf_cmap;
typedef struct pdf_range_s pdf_range;

enum { PDF_CMAP_SINGLE, PDF_CMAP_RANGE, PDF_CMAP_TABLE, PDF_CMAP_MULTI };

struct pdf_range_s
{
	unsigned short low;
	unsigned short extentflags;
	unsigned short offset;	/* range-delta or table-index */
};
		
struct pdf_cmap_s
{
	int refs;
	char cmapname[32];

	char usecmapname[32];
	pdf_cmap *usecmap;

	int wmode;

	int ncspace;
	struct
	{
		unsigned short n;
		unsigned short low;
		unsigned short high;
	} cspace[40];

	int rlen, rcap;
	pdf_range *ranges;

	int tlen, tcap;
	unsigned short *table;
};

/*cmapdump.c*/
extern int precmap_init();
extern pdf_cmap *pdf_cmaptable[120]; /* list of builtin system cmaps */

pdf_cmap *pdf_newcmap(void);
pdf_cmap *pdf_keepcmap(pdf_cmap *cmap);
void pdf_dropcmap(pdf_cmap *cmap);

int pdf_getwmode(pdf_cmap *cmap);
void pdf_setwmode(pdf_cmap *cmap, int wmode);
void pdf_setusecmap(pdf_cmap *cmap, pdf_cmap *usecmap);

void pdf_addcodespace(pdf_cmap *cmap, int low, int high, int n);
void pdf_maprangetotable(pdf_cmap *cmap, int low, int *map, int len);
void pdf_maprangetorange(pdf_cmap *cmap, int srclo, int srchi, int dstlo);
void pdf_maponetomany(pdf_cmap *cmap, int one, int *many, int len);
void pdf_sortcmap(pdf_cmap *cmap);

int pdf_lookupcmap(pdf_cmap *cmap, int cpt);
int pdf_lookupcmapfull(pdf_cmap *cmap, int cpt, int *out);
unsigned char *pdf_decodecmap(pdf_cmap *cmap, unsigned char *s, int *cpt);

pdf_cmap *pdf_newidentitycmap(int wmode, int bytes);
fz_error pdf_parsecmap(pdf_cmap **cmapp, fz_stream *file);
fz_error pdf_loadembeddedcmap(pdf_cmap **cmapp, pdf_xref *xref, fz_obj *ref);
fz_error pdf_loadsystemcmap(pdf_cmap **cmapp, char *name);

/*
 * Font
 */

void pdf_loadencoding(char **estrings, char *encoding);
int pdf_lookupagl(char *name);
char **pdf_lookupaglnames(int ucs);

extern const unsigned short pdf_docencoding[256];
extern const char * const pdf_macroman[256];
extern const char * const pdf_macexpert[256];
extern const char * const pdf_winansi[256];
extern const char * const pdf_standard[256];
extern const char * const pdf_expert[256];
extern const char * const pdf_symbol[256];
extern const char * const pdf_zapfdingbats[256];

typedef struct pdf_fontdesc_s pdf_fontdesc;

struct pdf_fontdesc_s
{
	int refs;

	fz_font *font;


	/* Encoding (CMap) */
	pdf_cmap *encoding;
	pdf_cmap *tottfcmap;
	int ncidtogid;
	unsigned short *cidtogid;

	/* ToUnicode */
	pdf_cmap *tounicode;
	int ncidtoucs;
	unsigned short *cidtoucs;

	/* Metrics (given in the PDF file) */
	int wmode;

	int isembedded;
};

/* fontmtx.c */
void pdf_setfontwmode(pdf_fontdesc *font, int wmode);

/* unicode.c */
fz_error pdf_loadtounicode(pdf_fontdesc *font, pdf_xref *xref, char **strings, char *collection, fz_obj *cmapstm);


/* type3.c */
fz_error pdf_loadtype3font(pdf_fontdesc **fontp, pdf_xref *xref, fz_obj *rdb, fz_obj *obj);

/* font.c */
fz_error pdf_loadfont(pdf_fontdesc **fontp, pdf_xref *xref, fz_obj *rdb, fz_obj *obj);
pdf_fontdesc *pdf_newfontdesc(void);
pdf_fontdesc *pdf_keepfont(pdf_fontdesc *fontdesc);
void pdf_dropfont(pdf_fontdesc *font);
void pdf_debugfont(pdf_fontdesc *fontdesc);


/*
 * Page tree, pages and related objects
 */

typedef struct pdf_page_s pdf_page;

struct pdf_page_s
{
    int fw;
    int fw_code;
	fz_obj *resources;
	fz_buffer *contents;
};

/* pagetree.c */
fz_error pdf_loadpagetree(pdf_xref *xref);
int pdf_getpagecount(pdf_xref *xref);
fz_obj *pdf_getpageobject(pdf_xref *xref, int p);
fz_obj *pdf_getpageref(pdf_xref *xref, int p);
int pdf_findpageobject(pdf_xref *xref, fz_obj *pageobj);

/* page.c */
fz_error pdf_loadpage(pdf_page **pagep, pdf_xref *xref, fz_obj *ref);
void pdf_freepage(pdf_page *page);

/*
 * content stream parsing
 */

typedef struct pdf_gstate_s pdf_gstate;
typedef struct pdf_csi_s pdf_csi;

struct pdf_gstate_s
{

	pdf_fontdesc *font;
	float size;

};

struct pdf_csi_s
{
	pdf_xref *xref;

	fz_obj *stack[32];
	int top;
	fz_obj *array;

	pdf_gstate gstate[64];
	int gtop;

    int fw;
    int fw_code;

	int BTflag; 
};

/* build.c */
void pdf_initgstate(pdf_gstate *gs);
void pdf_showtext(pdf_csi*, fz_obj *text);

/* interpret.c */
void pdf_gsave(pdf_csi *csi);
void pdf_grestore(pdf_csi *csi);
fz_error pdf_runcsibuffer(pdf_csi *csi, fz_obj *rdb, fz_buffer *contents);
fz_error pdf_runpage(pdf_xref *xref, pdf_page *page);


#endif
