#include "fitz.h"

static fz_font *
fz_newfont(void)
{
	fz_font *font;

	font = fz_malloc(sizeof(fz_font));
	font->refs = 1;
	strcpy(font->name, "<unknown>");

	font->t3resources = nil;
	font->t3procs = nil;
	font->t3xref = nil;
	font->t3widths = nil;

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


		fz_free(font);
	}
}




/*
 * Type 3 fonts...
 */

fz_font *
fz_newtype3font(char *name)
{
	fz_font *font;
	int i;

	font = fz_newfont();
	font->t3procs = fz_calloc(256, sizeof(fz_buffer*));
	font->t3widths = fz_calloc(256, sizeof(float));

	fz_strlcpy(font->name, name, sizeof(font->name));
	for (i = 0; i < 256; i++)
	{
		font->t3procs[i] = nil;
		font->t3widths[i] = 0;
	}

	return font;
}

