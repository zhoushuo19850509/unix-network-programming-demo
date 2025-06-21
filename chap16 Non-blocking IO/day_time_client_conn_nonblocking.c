#include <stdio.h>
#include "unp.h"


/**
 * 功能： daytime client ,read from the daytime server
 * 目标： 本代码基于chap1/day_time_client，对connect()予以改造，
 *       就是为了说明non-blocking connect的用法
 * 环境： playground(imac-home)，这个环境需要预先准备好unp.h
 * 用法： 
 *  用法： 
 *  1.配置Makefile
 *  2.执行：  make
 *  3.访问day time server
 *      ./day_time_client_conn_nonblocking 127.0.0.1
*/
int connect_nonb(int sockfd, const SA *aaptr, socklen_t salen, int nsec);


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
    if(connect_nonb(sockfd, (SA *) &servaddr, sizeof(servaddr), 0) < 0){
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

/**
 * non-blocking 版本的socket client connect()
 * 替代chap1/day_time_client.c中的connect() API
*/
int connect_nonb(int sockfd, const SA *aaptr, socklen_t salen, int nsec){
    
    int flags; // 用来保存sockfd原来的属性
    int error; 
    // connect()结果保存在这里
    int n; 
    fd_set rset, wset; // 这个我们之前已经了解了：select()捕获的IO读写事件保存在这里
    struct timeval tval;
    int len;


    // 先暂时把socket fd 文件属性设置为non-blocking
    // 在设置为non-blocking之前，先保存一下sockfd原来的属性，connect()完成后还要改回去的
    flags = Fcntl(sockfd, F_GETFL, 0);  
    Fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    error = 0;
    /**
     * 处理connect()失败的情况
     * connect()调用结果 
     * <0 说明connect失败 
     * ==0说明connect成功
    */
    if( n = connect(sockfd,aaptr, salen) < 0) {
        /**
         * 在non-blocking模式下，connect()如果返回EINPROGRESS异常是正常，
         * 而如果是其他异常，那就是真正的异常，就直接返回了
        */
        if(errno != EINPROGRESS){
            return (-1);
        }
    }

    // 如果直接connect成功，那就完成任务了
    if( n == 0 ){
        goto done;
    }

    /**
     *  如果connect暂时还没成功，可以先做其他事
     * (因为connect是non-blocking的，不阻塞其他任务)
    */
    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);
    wset = rset;
    tval.tv_sec = nsec;
    tval.tv_usec = 0;

    /**
     * 调用Select监听socket事件，
     * 如果Select返回0那就是connect超时了
    */
    if( (n = Select(sockfd + 1, &rset, &wset, NULL, nsec ? &tval: NULL)) == 0){
        close(sockfd);
        errno = ETIMEDOUT;
        return (-1);
    }

    // 如果Select监听到了IO事件
    if( FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)){
        len = sizeof(error);
        // Select监听到了IO事件，通过getsockopt()找一下socket是否有异常
        if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0){
            return (-1);
        }
    }else{
        err_quit("select error : sockfd not set!");
    }


   // connect成功后该做的事：把socket fd的文件属性改回原来的
  done:
    Fcntl(sockfd, F_SETFL, flags);
    if(error){
        close(sockfd);
        errno = error;
        return (-1);
    }
    return 0;

}
