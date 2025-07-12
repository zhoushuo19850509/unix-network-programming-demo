#include "web.h"

/**
 * 当某个下载链接文件的socket connect 连接完成之后，
 * 往服务器发送一个HTTP GET请求，用于下载某个链接文件，比如image1.gif
 * @param fptr 这是指向struct file的指针，
 *     包含了某个HTML页面中某个链接文件，
 *     比如我们启动程序的时候指定的gif文件：image1.gif 
*/
void write_get_cmd(struct file *fptr){

    int n;
    char line[MAXLINE];

    n = snprintf(line, sizeof(line), GET_CMD, fptr->f_name);
    Writen(fptr->f_fd, line, n);
    // 打印一下发送一个HTTP GET请求用于下载某个链接文件，GET请求包含了多少字节
    printf("wrote %d bytes for: %s \n", n , fptr->f_name);

    // set file socket fd status: reading response from server
    fptr->f_flags = F_READING; 

    FD_SET(fptr->f_fd, &rset);
    if(fptr->f_fd > maxfd){
        maxfd = fptr->f_fd;
    }

}



