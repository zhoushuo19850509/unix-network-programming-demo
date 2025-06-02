#include <stdio.h>
#include "unp.h"
#include <time.h>

/**
 * 功能： daytime server , send date into back to daytime cliient
 * 目标： 为了说明socket server的用法
 * 环境： playground(imac-home)，这个环境需要预先准备好unp.h
 * 用法： 
 *  1.配置Makefile
 *  2.执行：  make
 *  3.启动day time server
 *      sudo ./day_time_server
 *  4.启动day time client访问
 *    ./day_time_client 127.0.0.1
 *    可以得到时间信息
 * 
 * 备注：
 *    经过实践，在ubuntu下，必须用sudo启动，可能是因为13端口是系统端口，
 *    如果用普通用户启动，13端口不允许启动
*/
int main(){

    int listenfd; // 监听的socket
    int connectfd; // 接收到来自客户端的socket连接
    struct sockaddr_in servaddr;
    time_t ticks;
    char buff[MAXLINE];

    // 1.创建socket server
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    // 2.创建server address
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(13);  // 指定server port

    // 3.绑定端口(bind)
    bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

    // 4.监听端口(listen)
    listen(listenfd, LISTENQ);

    // 5.循环接收来自客户端的socket请求(accept)
    for(;;){
        connectfd = accept(listenfd, (SA *) NULL, NULL);

        // 6.接收到来自客户端的socket请求之后，把日期信息发送给客户端(write())
        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        write(connectfd, buff, strlen(buff));

        // 7.发送完日期信息之后，关闭客户端的socket请求(close)
        close(connectfd);
    }

}



