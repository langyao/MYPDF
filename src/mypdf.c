#include <fitz.h>
#include <pdf.h>
#include "mypdf.h"

#include <ctype.h> /* for tolower() */

#define DEBUG(fmt,args...) printf("[%s %d]"fmt,__FILE__,__LINE__,##args)


static void pdf_showpage(mypdf_t *mypdf)
{
    //test
    int i,j;
    int error;

    int fw = open("OutputEnglish.txt",O_CREAT | O_WRONLY,0777);
    if(fw < 0)
        fz_throw("OutputEnglish open failed");

    for(i = 0; i < mypdf->xref->pagelen; i++)
    {

        error = pdf_loadpage(&mypdf->page,mypdf->xref,mypdf->xref->pageobjs[i]);
        if(error < 0)
            continue;

        mypdf->page->fw = fw;


        error = pdf_runpage(mypdf->xref,mypdf->page);
    }


}

void mypdf_init(mypdf_t *mypdf)
{
    memset(mypdf, 0, sizeof(mypdf_t));
}

void mypdf_open(mypdf_t *mypdf, char *filename, int fd)
{
    fz_error error;
    fz_obj *obj;
    fz_obj *info;
    fz_stream *file;


    /*
     * Open PDF and load xref table
     */

    file = fz_openfile(fd);
    error = pdf_openxrefwithstream(&mypdf->xref, file, nil);

    if (error)
        fz_rethrow(error, "cannot open document '%s'", filename);
    fz_close(file);

    mypdf->doctitle = filename;
    if (strrchr(mypdf->doctitle, '\\'))
        mypdf->doctitle = strrchr(mypdf->doctitle, '\\') + 1;
    if (strrchr(mypdf->doctitle, '/'))
        mypdf->doctitle = strrchr(mypdf->doctitle, '/') + 1;

    error = pdf_loadpagetree(mypdf->xref);
    if (error)
        fz_rethrow(error, "cannot load page tree");

    pdf_showpage(mypdf);

}


