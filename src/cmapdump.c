/* cmapdump.c -- parse a CMap file and dump it as a c-struct */

#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fitz.h"
#include "mypdf.h"
#include "pdf.h"

char *path  = "cmaps";
pdf_cmap *pdf_cmaptable[120]; //注意超过限制
int csize = 0;




static int parse_cmap_prefile(char *filename)
{

	pdf_cmap *cmap;
	fz_error error;
	fz_stream *fi;
	int fd;

	fd = open(filename, O_BINARY | O_RDONLY);
	if (fd < 0)
	{
		perror("open failed");
		return fz_throw("cmapdump: could not open input file '%s'\n", filename);
	}

	fi = fz_openfile(fd);

	error = pdf_parsecmap(&cmap, fi);
	if (error)
	{
		fz_catch(error, "cmapdump: could not parse input cmap '%s'\n", filename);
		return -1;
	}

	pdf_cmaptable[csize++] =  cmap; 

	return 1;
}

int precmap_init()
{

	struct stat statbuf;
	struct dirent *dirp;
	DIR *dp;
	char name[256];

	if(lstat(path,&statbuf) < 0)
	{
		printf("error path=%s\n",path);
		perror("lstat error");
		return fz_throw("lstat error");

	}

	if(S_ISDIR(statbuf.st_mode) == 0) //is not a directory
		fz_throw("expected a directory");

	if((dp = opendir(path)) == NULL) //cannot read directory
		return fz_throw("opendir error");

	int error;
	while((dirp = readdir(dp)) != NULL )
	{
		if(strcmp(dirp->d_name,".") == 0|| strcmp(dirp->d_name,"..") == 0)
			continue;
		strcpy(name,path);
		strcat(name,"/");
		strcat(name,dirp->d_name);

		if(lstat(name,&statbuf) < 0)
		{
			return fz_throw("lstat error");

		}

		if(S_ISDIR(statbuf.st_mode) == 0) //is not a directory
			parse_cmap_prefile(name);

	}

	closedir(dp);
	return 1;
}
