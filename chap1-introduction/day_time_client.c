#include <stdio.h>
#include "unp.h"


/**
 * 功能： daytime client ,read from the daytime server
 * 目标： 为了说明socket client的用法
 * 环境： playground(imac-home)，这个环境需要预先准备好unp.h
 * 用法： 
 *  1.编译 gcc -o day_time_client day_time_client.c
 *  2.访问day time server
 *      ./day_time_client 127.0.0.1
*/
int main(int argc, char* argv[]){

    int sockfd; // socket file descriptor
    struct sockaddr_in servaddr;
    int n; // while循环中从daytime server读取到的字节数
    char recvline[MAXLINE + 1];

    // 判断参数数量是否合法
    if(argc != 2){
        err_sys("usage: a.out <ip adress>");
    }

    // 创建socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        err_sys("socket fd create error ...");
    }

    // 初始化server address对象： servaddr
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(13);  // 指定访问哪个server port
    // 指定server address
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <=0){
        err_sys("inet_pton error from server : %s", argv[1]);
    }

    // connect
    if(connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0){
        err_sys("connect error ...");
    }

    // 从socket读取内容
    while( (n = read(sockfd, recvline, MAXLINE)) >0 ){
        recvline[n] = 0; // line的结尾
        if(fputs(recvline, stdout) == EOF){
            err_sys("fputs error ...");
        }
    }
    if(n < 0){
        err_sys("read error ...");
    }
}
