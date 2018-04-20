#include "fitz.h"
#include "pdf.h"





/*
 * Create and destroy
 */

pdf_fontdesc *
pdf_keepfont(pdf_fontdesc *fontdesc)
{
	fontdesc->refs ++;
	return fontdesc;
}

void
pdf_dropfont(pdf_fontdesc *fontdesc)
{
	if (fontdesc && --fontdesc->refs == 0)
	{
	//	if (fontdesc->font)
	//		fz_dropfont(fontdesc->font);
		if (fontdesc->encoding)
			pdf_dropcmap(fontdesc->encoding);
//		if (fontdesc->tottfcmap)
//			pdf_dropcmap(fontdesc->tottfcmap);
		if (fontdesc->tounicode)
			pdf_dropcmap(fontdesc->tounicode);
		fz_free(fontdesc->cidtogid);
		fz_free(fontdesc->cidtoucs);
		fz_free(fontdesc);
	}
}

pdf_fontdesc *
pdf_newfontdesc(void)
{
	pdf_fontdesc *fontdesc;

	fontdesc = fz_malloc(sizeof(pdf_fontdesc));
	fontdesc->refs = 1;

	fontdesc->font = nil;


	fontdesc->encoding = nil;
	fontdesc->tottfcmap = nil;
	fontdesc->ncidtogid = 0;
	fontdesc->cidtogid = nil;

	fontdesc->tounicode = nil;
	fontdesc->ncidtoucs = 0;
	fontdesc->cidtoucs = nil;

	fontdesc->wmode = 0;


	fontdesc->isembedded = 0;

	return fontdesc;
}

/*
 * Simple fonts (Type1 and TrueType)
 */

static fz_error
loadsimplefont(pdf_fontdesc **fontdescp, pdf_xref *xref, fz_obj *dict)
{
	fz_error error;
	fz_obj *encoding;
	pdf_fontdesc *fontdesc;

	char *estrings[256];
	int i, k, n;


	/* Load font file */

	fontdesc = pdf_newfontdesc();

	pdf_logfont("load simple font (%d %d R) ptr=%p {\n", fz_tonum(dict), fz_togen(dict), fontdesc);


	for (i = 0; i < 256; i++)
	{
		estrings[i] = nil;
	}

	encoding = fz_dictgets(dict, "Encoding");
	if (encoding)
	{
		if (fz_isname(encoding))
			pdf_loadencoding(estrings, fz_toname(encoding));

		if (fz_isdict(encoding))
		{
			fz_obj *base, *diff, *item;

			base = fz_dictgets(encoding, "BaseEncoding");
			if (fz_isname(base))
				pdf_loadencoding(estrings, fz_toname(base));
			else if (!fontdesc->isembedded)
				pdf_loadencoding(estrings, "StandardEncoding");

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
	}

	fontdesc->encoding = pdf_newidentitycmap(0, 1);

	error = pdf_loadtounicode(fontdesc, xref, estrings, nil, fz_dictgets(dict, "ToUnicode"));
	if (error)
		fz_catch(error, "cannot load tounicode");

	pdf_logfont("}\n");

	*fontdescp = fontdesc;
	return fz_okay;

	pdf_dropfont(fontdesc);
	return fz_rethrow(error, "cannot load simple font (%d %d R)", fz_tonum(dict), fz_togen(dict));
}

/*
 * CID Fonts
 */

static fz_error
loadcidfont(pdf_fontdesc **fontdescp, pdf_xref *xref, fz_obj *dict, fz_obj *encoding, fz_obj *tounicode)
{
	fz_error error;
	fz_obj *widths;
	pdf_fontdesc *fontdesc;
	int kind;
	char collection[256];
	fz_obj *obj;
	int dw;

	/* Get font name and CID collection */


		fz_obj *cidinfo;
		char tmpstr[64];
		int tmplen;

		cidinfo = fz_dictgets(dict, "CIDSystemInfo");
		if (!cidinfo)
			return fz_throw("cid font is missing info");

		obj = fz_dictgets(cidinfo, "Registry");
		tmplen = MIN(sizeof tmpstr - 1, fz_tostrlen(obj));
		memcpy(tmpstr, fz_tostrbuf(obj), tmplen);
		tmpstr[tmplen] = '\0';
		fz_strlcpy(collection, tmpstr, sizeof collection);

		fz_strlcat(collection, "-", sizeof collection);

		obj = fz_dictgets(cidinfo, "Ordering");
		tmplen = MIN(sizeof tmpstr - 1, fz_tostrlen(obj));
		memcpy(tmpstr, fz_tostrbuf(obj), tmplen);
		tmpstr[tmplen] = '\0';
		fz_strlcat(collection, tmpstr, sizeof collection);

	/* Load font file */

	fontdesc = pdf_newfontdesc();

	pdf_logfont("load cid font (%d %d R) ptr=%p {\n", fz_tonum(dict), fz_togen(dict), fontdesc);
	pdf_logfont("collection %s\n", collection);

	/* Encoding */

	error = fz_okay;
	if (fz_isname(encoding))
	{
		pdf_logfont("encoding /%s\n", fz_toname(encoding));
		if (!strcmp(fz_toname(encoding), "Identity-H"))
			fontdesc->encoding = pdf_newidentitycmap(0, 2);
		else if (!strcmp(fz_toname(encoding), "Identity-V"))
		{
			fz_warn("warning not dealing identity-V");
			fontdesc->encoding = pdf_newidentitycmap(1, 2);
		}
		else
			error = pdf_loadsystemcmap(&fontdesc->encoding, fz_toname(encoding));
	}
	else if (fz_isindirect(encoding))
	{
		pdf_logfont("encoding %d %d R\n", fz_tonum(encoding), fz_togen(encoding));
		error = pdf_loadembeddedcmap(&fontdesc->encoding, xref, encoding);
	}
	else
	{
		error = fz_throw("syntaxerror: font missing encoding");
	}
	if (error)
		goto cleanup;

	pdf_setfontwmode(fontdesc, pdf_getwmode(fontdesc->encoding));
	pdf_logfont("wmode %d\n", pdf_getwmode(fontdesc->encoding));

	error = pdf_loadtounicode(fontdesc, xref, nil, collection, tounicode);
	if (error)
		fz_catch(error, "cannot load tounicode");

	pdf_logfont("}\n");

	*fontdescp = fontdesc;
	return fz_okay;

cleanup:
	pdf_dropfont(fontdesc);
	return fz_rethrow(error, "cannot load cid font (%d %d R)", fz_tonum(dict), fz_togen(dict));
}



static fz_error
loadtype0(pdf_fontdesc **fontdescp, pdf_xref *xref, fz_obj *dict)
{
	fz_error error;
	fz_obj *dfonts;
	fz_obj *dfont;
	fz_obj *subtype;
	fz_obj *encoding;
	fz_obj *tounicode;


	dfonts = fz_dictgets(dict, "DescendantFonts");
	if (!dfonts)
		return fz_throw("cid font is missing descendant fonts");

	dfont = fz_arrayget(dfonts, 0);

	subtype = fz_dictgets(dfont, "Subtype");
	encoding = fz_dictgets(dict, "Encoding");
	tounicode = fz_dictgets(dict, "ToUnicode");

	if (fz_isname(subtype) && !strcmp(fz_toname(subtype), "CIDFontType0"))
		error = loadcidfont(fontdescp, xref, dfont, encoding, tounicode);
	else if (fz_isname(subtype) && !strcmp(fz_toname(subtype), "CIDFontType2"))
		error = loadcidfont(fontdescp, xref, dfont, encoding, tounicode);
	else
		error = fz_throw("syntaxerror: unknown cid font type");
	if (error)
		return fz_rethrow(error, "cannot load descendant font (%d %d R)", fz_tonum(dfont), fz_togen(dfont));

	return fz_okay;
}

fz_error
pdf_loadfont(pdf_fontdesc **fontdescp, pdf_xref *xref, fz_obj *rdb, fz_obj *dict)
{
	fz_error error;
	char *subtype;
	fz_obj *dfonts;
	fz_obj *charprocs;

	if ((*fontdescp = pdf_finditem(xref->store, pdf_dropfont, dict)))
	{
		pdf_keepfont(*fontdescp);
		return fz_okay;
    }

    subtype = fz_toname(fz_dictgets(dict, "Subtype"));
    dfonts = fz_dictgets(dict, "DescendantFonts");
    charprocs = fz_dictgets(dict, "CharProcs");

    if (subtype && !strcmp(subtype, "Type0"))
        error = loadtype0(fontdescp, xref, dict);
    else if (subtype && !strcmp(subtype, "Type1"))
        error = loadsimplefont(fontdescp, xref, dict);
    else if (subtype && !strcmp(subtype, "MMType1"))
        error = loadsimplefont(fontdescp, xref, dict);
    else if (subtype && !strcmp(subtype, "TrueType"))
        error = loadsimplefont(fontdescp, xref, dict);
    else if (subtype && !strcmp(subtype, "Type3"))
        error = pdf_loadtype3font(fontdescp, xref, rdb, dict);
    else if (charprocs)
    {
        fz_warn("unknown font format, guessing type3.");
        error = pdf_loadtype3font(fontdescp, xref, rdb, dict);
    }
    else if (dfonts)
    {
        fz_warn("unknown font format, guessing type0.");
        error = loadtype0(fontdescp, xref, dict);
    }
    else
    {
        fz_warn("unknown font format, guessing type1 or truetype.");
        //	error = loadsimplefont(fontdescp, xref, dict);
    }
    if (error)
        return fz_rethrow(error, "cannot load font (%d %d R)", fz_tonum(dict), fz_togen(dict));


    pdf_storeitem(xref->store, pdf_keepfont, pdf_dropfont, dict, *fontdescp);

    return fz_okay;
}

    void
pdf_debugfont(pdf_fontdesc *fontdesc)
{
    int i;

    printf("fontdesc {\n");

    if (fontdesc->font->ftface)
        printf("\tfreetype font\n");
    if (fontdesc->font->t3procs)
        printf("\ttype3 font\n");

    printf("\twmode %d\n", fontdesc->wmode);


}


void
pdf_setfontwmode(pdf_fontdesc *font, int wmode)
{
	font->wmode = wmode;
}

