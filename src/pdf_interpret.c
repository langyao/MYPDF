#include "fitz.h"
#include "pdf.h"

static pdf_csi *
pdf_newcsi(pdf_xref *xref)
{
    pdf_csi *csi;

    csi = fz_malloc(sizeof(pdf_csi));
    csi->xref = xref;

    csi->top = 0;
    csi->array = nil;


	pdf_initgstate(&csi->gstate[0]);
    csi->gtop = 0;

	csi->BTflag = 0;

    return csi;
}


static void
pdf_clearstack(pdf_csi *csi)
{
	int i;
	for (i = 0; i < csi->top; i++)
		fz_dropobj(csi->stack[i]);
	csi->top = 0;
}


static fz_error
pdf_runkeyword(pdf_csi *csi, fz_obj *rdb, char *buf)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	fz_error error;
	int what;
	int i;
	int m;

	switch (buf[0])
	{
	case 'B':
		switch(buf[1])
		{
		case 'T': /* "BT" */
			if (buf[2] != 0)
				goto defaultcase;
			if (csi->top < 0)
				goto syntaxerror;
			csi->BTflag = 1;
			break;
		default:
			goto defaultcase;
		}
		break;

	case 'E':
		switch (buf[1])
		{
		case 'T': /* "ET" */
			if (buf[2] != 0)
				goto defaultcase;
			if (csi->top < 0)
				goto syntaxerror;
		//	pdf_flushtext(csi);

			csi->BTflag = 0;
			break;
		default:
			goto defaultcase;
		}
		break;

	case 'T':
		switch (buf[1])
		{
		case 'f': /* "Tf" */
		{
			fz_obj *dict;
			fz_obj *obj;

			if (buf[2] != 0)
				goto defaultcase;
			if (csi->top < 2)
				goto syntaxerror;

			dict = fz_dictgets(rdb, "Font");
			if (!dict)
				return fz_throw("cannot find Font dictionary");

			obj = fz_dictget(dict, csi->stack[0]);
			if (!obj)
				return fz_throw("cannot find font resource: %s", fz_toname(csi->stack[0]));

			if (gstate->font)
			{
    			pdf_dropfont(gstate->font);
    			gstate->font = nil;
			}

    		error = pdf_loadfont(&gstate->font, csi->xref, rdb, obj);
			if (error)
				return fz_rethrow(error, "cannot load font (%d %d R)", fz_tonum(obj), fz_togen(obj));

			gstate->size = fz_toreal(csi->stack[1]);

			break;
		}
		case 'j': /* "Tj" */
			if (buf[2] != 0)
				goto defaultcase;
			if (csi->top < 1)
				goto syntaxerror;
			if(csi->BTflag)
			{
				pdf_showtext(csi, csi->stack[0]);
			}
			break;
		case 'J': /* "TJ" */
			if (buf[2] != 0)
				goto defaultcase;
			if (csi->top < 1)
				goto syntaxerror;
			if(csi->BTflag)
				pdf_showtext(csi, csi->stack[0]);
			break;
		case 'd': /* "Td" */
			/*TODO:tm6 = ltm6 +td2*ltm4;
			 * |tm6 - ltms| >=1 newline
			 */
			if (buf[2] != 0)
				goto defaultcase;
			if (csi->top < 2)
				goto syntaxerror;
			m = abs(fz_toreal(csi->stack[1]));
			if(m > 0.001)
			{
				error = write(csi->fw_code,"\n",1);
				if(error < 0)
					fz_throw("write error");
			}
			break;
		case 'D': /* "TD" */
			if (buf[2] != 0)
				goto defaultcase;
			if (csi->top < 2)
				goto syntaxerror;
			m = abs(fz_toreal(csi->stack[1]));
			if(m > 0.001)
			{
				error = write(csi->fw_code,"\n",1);
				if(error < 0)
					fz_throw("write error");
			}
			break;
		default:
			goto defaultcase;
		}
		break;
	case '\'': /* "'" */
		if (buf[1] != 0)
			goto defaultcase;
		if (csi->top < 1)
			goto syntaxerror;

		break;

	case '"': /* """ */
		if (buf[1] != 0)
			goto defaultcase;
		if (csi->top < 3)
			goto syntaxerror;

		break;
	default:
defaultcase:
		break;
	}

	return fz_okay;

syntaxerror:
	return fz_throw("syntax error near '%s' with %d items on the stack", buf, csi->top);
}
	static fz_error
pdf_runcsifile(pdf_csi *csi, fz_obj *rdb, fz_stream *file, char *buf, int buflen)
{
	fz_error error;
	int tok;
	int len;
	fz_obj *obj;

	pdf_clearstack(csi);

	while (1)
	{
		if (csi->top == nelem(csi->stack) - 1)
			return fz_throw("stack overflow");

		error = pdf_lex(&tok, file, buf, buflen, &len);
		if (error)
			return fz_rethrow(error, "lexical error in content stream");

		if (csi->array)
		{
			if (tok == PDF_TCARRAY)
			{
				csi->stack[csi->top] = csi->array;
				csi->array = nil;
				csi->top ++;
			}
			else if (tok == PDF_TINT || tok == PDF_TREAL)
			{
				obj = fz_newreal(atof(buf));
				fz_arraypush(csi->array, obj);
				fz_dropobj(obj);
			}
			else if (tok == PDF_TSTRING)
			{
				obj = fz_newstring(buf, len);
				fz_arraypush(csi->array, obj);
				fz_dropobj(obj);
			}
			else if (tok == PDF_TKEYWORD)
			{
				/* some producers try to put Tw and Tc commands in the TJ array */
				fz_warn("ignoring keyword '%s' inside array", buf);
				if (!strcmp(buf, "Tw") || !strcmp(buf, "Tc"))
				{
					if (fz_arraylen(csi->array) > 0)
						fz_arraydrop(csi->array);
				}
			}
			else if (tok == PDF_TEOF)
			{
				return fz_okay;
			}
			else
			{
				pdf_clearstack(csi);
				return fz_throw("syntaxerror in array");
			}
		}

		else switch (tok)
		{
			case PDF_TENDSTREAM:
			case PDF_TEOF:
				return fz_okay;

				/* optimize text-object array parsing */
			case PDF_TOARRAY:
				csi->array = fz_newarray(8);
				break;

			case PDF_TODICT:
				error = pdf_parsedict(&csi->stack[csi->top], csi->xref, file, buf, buflen);
				if (error)
					return fz_rethrow(error, "cannot parse dictionary");
				csi->top ++;
				break;

			case PDF_TNAME:
				csi->stack[csi->top] = fz_newname(buf);
				csi->top ++;
				break;

			case PDF_TINT:
				csi->stack[csi->top] = fz_newint(atoi(buf));
				csi->top ++;
				break;

			case PDF_TREAL:
				csi->stack[csi->top] = fz_newreal(atof(buf));
				csi->top ++;
				break;

			case PDF_TSTRING:
				csi->stack[csi->top] = fz_newstring(buf, len);
				csi->top ++;
				break;

			case PDF_TTRUE:
				csi->stack[csi->top] = fz_newbool(1);
				csi->top ++;
				break;

			case PDF_TFALSE:
				csi->stack[csi->top] = fz_newbool(0);
				csi->top ++;
				break;

			case PDF_TNULL:
				csi->stack[csi->top] = fz_newnull();
				csi->top ++;
				break;

			case PDF_TKEYWORD:
				error = pdf_runkeyword(csi, rdb, buf);
				if (error)
					fz_catch(error, "cannot run keyword '%s'", buf);
				pdf_clearstack(csi);
				break;

			default:
				pdf_clearstack(csi);
				return fz_throw("syntaxerror in content stream");
		}
	}
}

	static void
pdf_freecsi(pdf_csi *csi)
{
	while (csi->gtop)
		//		pdf_grestore(csi);

		//	pdf_dropmaterial(&csi->gstate[0].fill);
		//	pdf_dropmaterial(&csi->gstate[0].stroke);
		if (csi->gstate[0].font)
			//		pdf_dropfont(csi->gstate[0].font);

			if (csi->array) fz_dropobj(csi->array);

	pdf_clearstack(csi);

	fz_free(csi);
}

	fz_error
pdf_runcsibuffer(pdf_csi *csi, fz_obj *rdb, fz_buffer *contents)
{
	fz_stream *file;
	fz_error error;
	file = fz_openbuffer(contents);
	error = pdf_runcsifile(csi, rdb, file, csi->xref->scratch, sizeof csi->xref->scratch);
	fz_close(file);
	if (error)
		return fz_rethrow(error, "cannot parse content stream");
	return fz_okay;
}

	fz_error
pdf_runpage(pdf_xref *xref,pdf_page *page)
{

	pdf_csi *csi;
	fz_error error;
	int flags;

	csi = pdf_newcsi(xref);

	csi->fw = page->fw;
	csi->fw_code = page->fw_code;

	error = pdf_runcsibuffer(csi, page->resources, page->contents);
	pdf_freecsi(csi);
	if (error)
		return fz_rethrow(error, "cannot parse page content stream");
	return fz_okay;

}
