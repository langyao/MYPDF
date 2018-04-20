#include "fitz.h"
#include "pdf.h"

static fz_error
pdf_loadpagecontentsarray(fz_buffer **bigbufp, pdf_xref *xref, fz_obj *list)
{
	fz_error error;
	fz_buffer *big;
	fz_buffer *one;
	int i;

	pdf_logpage("multiple content streams: %d\n", fz_arraylen(list));

	/* TODO: openstream, read, close into big buffer at once */

	big = fz_newbuffer(32 * 1024);

	for (i = 0; i < fz_arraylen(list); i++)
	{
		fz_obj *stm = fz_arrayget(list, i);
		error = pdf_loadstream(&one, xref, fz_tonum(stm), fz_togen(stm));
		if (error)
		{
			fz_dropbuffer(big);
			return fz_rethrow(error, "cannot load content stream part %d/%d (%d %d R)", i + 1, fz_arraylen(list), fz_tonum(stm), fz_togen(stm));
		}

		if (big->len + one->len + 1 > big->cap)
			fz_resizebuffer(big, big->len + one->len + 1);
		memcpy(big->data + big->len, one->data, one->len);
		big->data[big->len + one->len] = ' ';
		big->len += one->len + 1;

		fz_dropbuffer(one);
	}

	*bigbufp = big;
	return fz_okay;
}

static fz_error
pdf_loadpagecontents(fz_buffer **bufp, pdf_xref *xref, fz_obj *obj)
{
	fz_error error;

	if (fz_isarray(obj))
	{
		error = pdf_loadpagecontentsarray(bufp, xref, obj);
		if (error)
			return fz_rethrow(error, "cannot load content stream array (%d 0 R)", fz_tonum(obj));
	}
	else if (pdf_isstream(xref, fz_tonum(obj), fz_togen(obj)))
	{
		error = pdf_loadstream(bufp, xref, fz_tonum(obj), fz_togen(obj));
		if (error)
			return fz_rethrow(error, "cannot load content stream (%d 0 R)", fz_tonum(obj));
	}
	else
	{
		fz_warn("page contents missing, leaving page blank");
		*bufp = fz_newbuffer(0);
	}

#ifdef DEBUG
	printf("----fz_buffer:%s\n",(*bufp)->data);
#endif
	return fz_okay;
}

void
pdf_freepage(pdf_page *page)
{
	pdf_logpage("drop page %p\n", page);
	if (page->resources)
		fz_dropobj(page->resources);
	if (page->contents)
		fz_dropbuffer(page->contents);
	fz_free(page);
}

fz_error
pdf_loadpage(pdf_page **pagep, pdf_xref *xref, fz_obj *dict)
{
	fz_error error;
	pdf_page *page;
	fz_obj *obj;

	pdf_logpage("load page {\n");

	/* Ensure that we have a store for resource objects */
	if (!xref->store)
		xref->store = pdf_newstore();

	page = fz_malloc(sizeof(pdf_page));
	page->resources = nil;
	page->contents = nil;

	page->resources = fz_dictgets(dict, "Resources");

    if(fz_isindirect(page->resources))
    {
        page->resources = fz_resolveindirect(page->resources);
    }

    if(!fz_isdict(page->resources))
        return fz_throw("cannot open parse rescources");

//    //进行resource分析，如果含有字段procset并且无/text值，则该页无需分析
//    fz_obj *objProcSet = fz_dictgets(page->resources,"ProcSet");
//    if(!fz_isarray(objProcSet))
//    {
//        return fz_throw("expected array ProcSet type");
//    }
//	else if(!fz_isname)
//
//    if(!objProcSet)
//    {
//        return fz_throw("expected ProcSet type");
//    }
//
//    int j;
//    for(j = 0; j < objProcSet->u.a.len; ++j)
//    {
//   //     DEBUG("ProcSet name:%s\n",objProcSet->u.a.items[j]->u.n);
//        if(strcmp(objProcSet->u.a.items[j]->u.n,"Text") == 0)
//            break;
//    }
//    if(j == objProcSet->u.a.len)
//    {
//        return fz_throw("this page need not parse");
//    }

	if (page->resources)
        fz_keepobj(page->resources);

    obj = fz_dictgets(dict, "Contents");
    error = pdf_loadpagecontents(&page->contents, xref, obj);
    if (error)
    {
        pdf_freepage(page);
        return fz_rethrow(error, "cannot load page contents (%d %d R)", fz_tonum(obj), fz_togen(obj));
    }

    pdf_logpage("} %p\n", page);

    *pagep = page;
    return fz_okay;
}
