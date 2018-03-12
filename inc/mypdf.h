#ifndef MYPDF_H
#define MYPDF_H


#include "pdf.h"
#include "fitz.h"
#define MINRES 54
#define MAXRES 300

typedef struct mypdf_s mypdf_t;

enum { ARROW, HAND, WAIT };


struct mypdf_s
{
	/* current document params */
	char *doctitle;
	pdf_xref *xref;
	pdf_outline *outline;
	int pagecount;
	fz_glyphcache *cache;


	/* current page params */
	int pageno;
	pdf_page *page;


	/* client context storage */
	void *userdata;
};

void mypdf_init(mypdf_t *app);
void mypdf_open(mypdf_t *app, char *filename, int fd);
void mypdf_close(mypdf_t *app);

#endif
