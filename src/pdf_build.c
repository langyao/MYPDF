
#include "fitz.h"
#include "pdf.h"

extern int write_map_file(int *ucsbuf,int ucslen,int fw);

void
pdf_initgstate(pdf_gstate *gs)
{
	gs->font = nil;
	gs->size = -1;
}


static void
pdf_showcontents(pdf_csi *csi, int cid)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_fontdesc *fontdesc = gstate->font;
	int ucsbuf[8];
	int ucslen;
	int i;


	ucslen = 0;
	if (fontdesc->tounicode)
		ucslen = pdf_lookupcmapfull(fontdesc->tounicode, cid, ucsbuf);
	if (ucslen == 0 && cid < fontdesc->ncidtoucs)
	{
		ucsbuf[0] = fontdesc->cidtoucs[cid];
		ucslen = 1;

	}
	if (ucslen == 0 || (ucslen == 1 && ucsbuf[0] == 0))
	{
		ucsbuf[0] = '?';
		ucslen = 1;
	}

	int error = write(csi->fw,ucsbuf,ucslen);
	if(error < 0)
	{
			perror("write fw error");
		fz_throw("write error");
	}
	write_map_file(ucsbuf,ucslen,csi->fw_code);

	return;

}

	void
pdf_showtext(pdf_csi *csi, fz_obj *text)
{
	pdf_gstate *gstate = csi->gstate + csi->gtop;
	pdf_fontdesc *fontdesc = gstate->font;
	unsigned char *buf;
	unsigned char *end;
	int i, len;
	int cpt, cid;

	if (!fontdesc)
	{
		fz_warn("cannot draw text since font and size not set");
		return;
	}

	if (fz_isarray(text))
	{
		for (i = 0; i < fz_arraylen(text); i++)
		{
			fz_obj *item = fz_arrayget(text, i);
			if (fz_isstring(item))
				pdf_showtext(csi, item);
			else
				if(abs(fz_toreal(item)) > 100)
				{
					int error = write(csi->fw," ",1);
					if(error < 0)
						fz_throw("write error");
					error = write(csi->fw_code," ",1);
					if(error < 0)
						fz_throw("write error");
					
				}
		}
	}

	if (fz_isstring(text))
	{
		buf = (unsigned char *)fz_tostrbuf(text);
		len = fz_tostrlen(text);
		end = buf + len;

		while (buf < end)
		{
			buf = pdf_decodecmap(fontdesc->encoding, buf, &cpt);
			cid = pdf_lookupcmap(fontdesc->encoding, cpt);
			if (cid >= 0)
				pdf_showcontents(csi,cid);
			else
				fz_warn("cannot encode character with code point %#x", cpt);

			if (cpt == 32)
			{
//				int error = write(csi->fw," ",1);
//				if(error < 0)
//					fz_throw("write error");
//				error = write(csi->fw_code," ",1);
//				if(error < 0)
//					fz_throw("write error");

			}

		}
	}
}
