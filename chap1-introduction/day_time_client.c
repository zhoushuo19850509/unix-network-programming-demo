#include <stdio.h>
#include <stdarg.h> // 这个header file专门处理可变个数的参数
#include "unp.h"


void error(char *fmt, ...);

/**
 * 功能： daytime client ,read from the daytime server
 * 目标： 为了说明socket client的用法
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
        error("usage: a.out <ip adress>");
    }

    // 创建socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        error("socket fd create error ...");
    }

    // 初始化server address对象： servaddr
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(13);  // 指定访问哪个server port
    // 指定server address
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <=0){
        error("inet_pton error from server : %s", argv[1]);
    }

    // connect
    if(connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0){
        error("connect error ...");
    }

    // 从socket读取内容
    while( (n = read(sockfd, recvline, MAXLINE)) >0 ){
        recvline[n] = 0; // line的结尾
        if(fputs(recvline, stdout) == EOF){
            error("fputs error ...");
        }
    }
    if(n < 0){
        error("read error ...");
    }
}

/**
 * 功能： 打印异常信息到标准异常： stderr，并退出
   备注： 因为error()函数包含可变个数的参数，所以函数可以打印多个异常字段
 * 目标： 综合采用了可变数量参数(参考minprintf.c)、
 *      异常信息打印(参考concatenate-files-v2.c)
 */
void error(char *fmt, ...){
    /**
     * va_list是stdarg.h提供的语法糖，
     * 这厮一个指针，指向各个可变个数的参数
     */
     va_list args;
     va_start(args, fmt);  // va_list指向第一个参数： fmt
     fprintf(stderr, "error: ");
     /**
      * vfprintf()效果和printf()类似，
      * 说白了就是minprintf.c中处理%s/%d/%f的逻辑
      * 上面main()中使用error()打印异常信息的场景，可以明白vfprintf()的功能
      */
     vfprintf(stderr, fmt, args);
     fprintf(stderr, "\n");
     va_end(args);
     exit(1);
}