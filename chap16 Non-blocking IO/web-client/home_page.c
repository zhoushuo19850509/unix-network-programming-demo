#include "web.h"

/**
 * 本函数功能： 处理homepage
 * 本函数处理原理： 向wbe server发起HTTP GET请求,下载home page内容
 * 本函数不是web系列代码的重点，演示了通过blocking connect的方式发起HTTP GET请求
 * @param host 要访问的网址的名称host 比如: www.foobar.com 
 * @param fname 要访问的HTML页面, 比如 index.html 
 *   如果是 / ，意思就是root page
*/
void home_page(const char *host, const char *fname){
    int fd;
    int n;
    char line[MAXLINE];

    // blocking connect
    fd = Tcp_connect(host, SERV);
    // 通过HTTP GET请求访问web服务
    n = snprintf(line, sizeof(line), GET_CMD, fname);
    Writen(fd, line, n);

    for(;;){
        if( (n = Read(fd, line, MAXLINE)) == 0){
            break; // server close the socket connection
        }
        printf("read %d bytes of home page from server ... \n", n);
    }
    printf("end of file on home page ... \n");
    Close(fd);

}
