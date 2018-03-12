#include "fitz.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H

static void fz_finalizefreetype(void);

static fz_font *
fz_newfont(void)
{
	fz_font *font;

	font = fz_malloc(sizeof(fz_font));
	font->refs = 1;
	strcpy(font->name, "<unknown>");

	font->ftface = nil;
	font->ftsubstitute = 0;
	font->fthint = 0;

	font->ftfile = nil;
	font->ftdata = nil;
	font->ftsize = 0;

	font->t3resources = nil;
	font->t3procs = nil;
	font->t3xref = nil;
	font->t3run = nil;


	font->widthcount = 0;
	font->widthtable = nil;

	return font;
}

fz_font *
fz_keepfont(fz_font *font)
{
	font->refs ++;
	return font;
}

void
fz_dropfont(fz_font *font)
{
	int fterr;
	int i;

	if (font && --font->refs == 0)
	{
		if (font->t3procs)
		{
			if (font->t3resources)
				fz_dropobj(font->t3resources);
			for (i = 0; i < 256; i++)
				if (font->t3procs[i])
					fz_dropbuffer(font->t3procs[i]);
			fz_free(font->t3procs);
			fz_free(font->t3widths);
		}

		if (font->ftface)
		{
			fterr = FT_Done_Face((FT_Face)font->ftface);
			if (fterr)
				fz_warn("freetype finalizing face: %s", ft_errorstring(fterr));
			fz_finalizefreetype();
		}

		if (font->ftfile)
			fz_free(font->ftfile);
		if (font->ftdata)
			fz_free(font->ftdata);

		if (font->widthtable)
			fz_free(font->widthtable);

		fz_free(font);
	}
}

void
fz_setfontbbox(fz_font *font, float xmin, float ymin, float xmax, float ymax)
{
	font->bbox.x0 = xmin;
	font->bbox.y0 = ymin;
	font->bbox.x1 = xmax;
	font->bbox.y1 = ymax;
}

/*
 * Freetype hooks
 */

static FT_Library fz_ftlib = nil;
static int fz_ftlib_refs = 0;

#undef __FTERRORS_H__
#define FT_ERRORDEF(e, v, s)	{ (e), (s) },
#define FT_ERROR_START_LIST
#define FT_ERROR_END_LIST	{ 0, NULL }

struct ft_error
{
	int err;
	char *str;
};

static const struct ft_error ft_errors[] =
{
#include FT_ERRORS_H
};

char *ft_errorstring(int err)
{
	const struct ft_error *e;

	for (e = ft_errors; e->str != NULL; e++)
		if (e->err == err)
			return e->str;

	return "Unknown error";
}

static fz_error
fz_initfreetype(void)
{
	int fterr;
	int maj, min, pat;

	if (fz_ftlib)
	{
		fz_ftlib_refs++;
		return fz_okay;
	}

	fterr = FT_Init_FreeType(&fz_ftlib);
	if (fterr)
		return fz_throw("cannot init freetype: %s", ft_errorstring(fterr));

	FT_Library_Version(fz_ftlib, &maj, &min, &pat);
	if (maj == 2 && min == 1 && pat < 7)
	{
		fterr = FT_Done_FreeType(fz_ftlib);
		if (fterr)
			fz_warn("freetype finalizing: %s", ft_errorstring(fterr));
		return fz_throw("freetype version too old: %d.%d.%d", maj, min, pat);
	}

	fz_ftlib_refs++;
	return fz_okay;
}

static void
fz_finalizefreetype(void)
{
	int fterr;

	if (--fz_ftlib_refs == 0)
	{
		fterr = FT_Done_FreeType(fz_ftlib);
		if (fterr)
			fz_warn("freetype finalizing: %s", ft_errorstring(fterr));
		fz_ftlib = nil;
	}
}

fz_error
fz_newfontfromfile(fz_font **fontp, char *path, int index)
{
	fz_error error;
	fz_font *font;
	int fterr;

	error = fz_initfreetype();
	if (error)
		return fz_rethrow(error, "cannot init freetype library");

	font = fz_newfont();

	fterr = FT_New_Face(fz_ftlib, path, index, (FT_Face*)&font->ftface);
	if (fterr)
	{
		fz_free(font);
		return fz_throw("freetype: cannot load font: %s", ft_errorstring(fterr));
	}

	*fontp = font;
	return fz_okay;
}

fz_error
fz_newfontfrombuffer(fz_font **fontp, unsigned char *data, int len, int index)
{
	fz_error error;
	fz_font *font;
	int fterr;

	error = fz_initfreetype();
	if (error)
		return fz_rethrow(error, "cannot init freetype library");

	font = fz_newfont();

	fterr = FT_New_Memory_Face(fz_ftlib, data, len, index, (FT_Face*)&font->ftface);
	if (fterr)
	{
		fz_free(font);
		return fz_throw("freetype: cannot load font: %s", ft_errorstring(fterr));
	}

	*fontp = font;
	return fz_okay;
}


/*
 * Type 3 fonts...
 */

fz_font *
fz_newtype3font(char *name, fz_matrix matrix)
{
	fz_font *font;
	int i;

	font = fz_newfont();
	font->t3procs = fz_calloc(256, sizeof(fz_buffer*));
	font->t3widths = fz_calloc(256, sizeof(float));

	fz_strlcpy(font->name, name, sizeof(font->name));
	font->t3matrix = matrix;
	for (i = 0; i < 256; i++)
	{
		font->t3procs[i] = nil;
		font->t3widths[i] = 0;
	}

	return font;
}

void
fz_debugfont(fz_font *font)
{
	printf("font '%s' {\n", font->name);

	if (font->ftface)
	{
		printf("\tfreetype face %p\n", font->ftface);
		if (font->ftsubstitute)
			printf("\tsubstitute font\n");
	}

	if (font->t3procs)
	{
		printf("\ttype3 matrix [%g %g %g %g]\n",
			font->t3matrix.a, font->t3matrix.b,
			font->t3matrix.c, font->t3matrix.d);
	}

	printf("\tbbox [%g %g %g %g]\n",
		font->bbox.x0, font->bbox.y0,
		font->bbox.x1, font->bbox.y1);

	printf("}\n");
}
