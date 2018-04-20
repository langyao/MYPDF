#include "fitz.h"
#include "pdf.h"

fz_error
pdf_loadtype3font(pdf_fontdesc **fontdescp, pdf_xref *xref, fz_obj *rdb, fz_obj *dict)
{
	fz_error error;
	char buf[256];
	char *estrings[256];
	pdf_fontdesc *fontdesc;
	fz_obj *encoding;
	fz_obj *widths;
	fz_obj *charprocs;
	fz_obj *obj;
	int i,n,k;

	obj = fz_dictgets(dict, "Name");
	if (fz_isname(obj))
		fz_strlcpy(buf, fz_toname(obj), sizeof buf);
	else
		sprintf(buf, "Unnamed-T3");

	fontdesc = pdf_newfontdesc();

	pdf_logfont("load type3 font (%d %d R) ptr=%p {\n", fz_tonum(dict), fz_togen(dict), fontdesc);
	pdf_logfont("name %s\n", buf);


	fontdesc->font = fz_newtype3font(buf);


	/* Encoding */

	for (i = 0; i < 256; i++)
		estrings[i] = nil;

	encoding = fz_dictgets(dict, "Encoding");
	if (!encoding)
	{
		error = fz_throw("syntaxerror: Type3 font missing Encoding");
		goto cleanup;
	}

	if (fz_isname(encoding))
		pdf_loadencoding(estrings, fz_toname(encoding));

	if (fz_isdict(encoding))
	{
		fz_obj *base, *diff, *item;

		base = fz_dictgets(encoding, "BaseEncoding");
		if (fz_isname(base))
			pdf_loadencoding(estrings, fz_toname(base));

		diff = fz_dictgets(encoding, "Differences");
		if (fz_isarray(diff))
		{
			n = fz_arraylen(diff);
			k = 0;
			for (i = 0; i < n; i++)
			{
				item = fz_arrayget(diff, i);
				if (fz_isint(item))
					k = fz_toint(item);
				if (fz_isname(item))
					estrings[k++] = fz_toname(item);
				if (k < 0) k = 0;
				if (k > 255) k = 255;
			}
		}
	}

	fontdesc->encoding = pdf_newidentitycmap(0, 1);

	error = pdf_loadtounicode(fontdesc, xref, estrings, nil, fz_dictgets(dict, "ToUnicode"));
	if (error)
		goto cleanup;

	/* Resources -- inherit page resources if the font doesn't have its own */

	fontdesc->font->t3resources = fz_dictgets(dict, "Resources");
	if (!fontdesc->font->t3resources)
		fontdesc->font->t3resources = rdb;
	if (fontdesc->font->t3resources)
		fz_keepobj(fontdesc->font->t3resources);
	if (!fontdesc->font->t3resources)
		fz_warn("no resource dictionary for type 3 font!");

	fontdesc->font->t3xref = xref;

	pdf_logfont("}\n");

	*fontdescp = fontdesc;
	return fz_okay;

cleanup:
	fz_dropfont(fontdesc->font);
	fz_free(fontdesc);
	return fz_rethrow(error, "cannot load type3 font (%d %d R)", fz_tonum(dict), fz_togen(dict));
}
