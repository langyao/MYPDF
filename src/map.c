#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <iconv.h>
#include <zlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>


#define LOG(fmt,args...) printf("[-%s-%d]"fmt,__FILE__,__LINE__,##args)


char* unic2utf(char *inbuf, size_t inbuf_len);



/*
 *功能：将提取出来的内容 写文件
 * */
int write_map_file(int *buf,int size,int fw)
{
    char *ret;

    ret = unic2utf((char *)buf,sizeof(buf));  //转码
	write(fw,ret,strlen(ret));
    return 0;

}


/*********************************************
 *function: 转换编码 unicode->utf-8
 *argument: inbuf: 原始数据
 *          inbuf_len: 原始数据长度
 *return: -1   失败
 *        0    成功
 *tips: no
 * ********************************************/
char* unic2utf(char *inbuf, size_t inbuf_len)
{
    char encTo[] = "UTF-8";
    char encFrom[] = "UNICODE";

    /* 获得转换句柄
     *@param encTo 目标编码方式
     *@param encFrom 源编码方式
     *
     * */
    iconv_t cd = iconv_open (encTo, encFrom);
    if (cd == (iconv_t)-1)
    {
        LOG("Here is return -1 \n");
        perror ("iconv_open");
        return NULL;
    }

    size_t srclen = inbuf_len;//strlen (inbuf);

    /* 存放转换后的字符串 */
    //size_t outlen = SEC_SIZE;  //暂定
    //size_t outlen = SEC_SIZE*10;  //暂定
    size_t outlen = inbuf_len*2;  //2倍的长度
    char *outbuf = (char*)malloc(outlen*sizeof(char));
    memset (outbuf, 0, outlen);

    /* 由于iconv()函数会修改指针，所以要保存源指针 */
    char *srcstart = inbuf;
    char *tempoutbuf = outbuf;

    /* 进行转换
     *@param cd iconv_open()产生的句柄
     *@param srcstart 需要转换的字符串
     *@param srclen 存放还有多少字符没有转换
     *@param tempoutbuf 存放转换后的字符串
     *@param outlen 存放转换后,tempoutbuf剩余的空间
     *
     * */
    int ret = iconv(cd, &srcstart, &srclen, &tempoutbuf, &outlen);
#if 1
    if (ret == -1)
    {
        LOG("errno = %d \n", errno);
        LOG("Here is return -1 \n");
        perror ("iconv");
        LOG("%s \n", strerror(errno));
        return NULL;
    }
#endif
    //LOG("inbuf=%s, srclen=%d, outbuf=%s, outlen=%d\n", inbuf, srclen, outbuf, outlen);
   // LOG("Final the rgb_string = %s \n", outbuf);
    /* 关闭句柄 */
    iconv_close (cd);
    //return 0;
    return outbuf;
}


