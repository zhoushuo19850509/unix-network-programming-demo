#include <stdio.h>
#include "unp.h"

/**
 * 功能： tcp echo  在console(stdin)发送任何字符给server，
 *       然后接收来自server echo back的内容，展示在console(stdout)中
 * 目标： 为了说明thread在socket 通讯中的作用
 * 环境： playground(imac-home)，这个环境需要预先准备好unp.h
 * 用法： 
 *  1.配置Makefile
 *  2.执行：  make
 *  2.服务端启动tcp echo server
 *     备注：现在有很多版本的tcp echo server，我们随便挑一个就行了，比如：
 *      ././tcp_echo_server_poll01
 *  3.客户端启动tch echo client
 *    ./tcp_echo_client_thread 127.0.0.1
 *    然后在客户端随便输入一些内容，就能得到server echo back的内容
*/
void str_cli(FILE *fp, int sockfd);
void copyto(void *arg);

int main(int argc, char* argv[]){

    int sockfd; // socket file descriptor
    struct sockaddr_in servaddr;

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
    servaddr.sin_port = htons(SERV_PORT);  // 指定访问哪个server port
    // 指定server address
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <=0){
        err_sys("inet_pton error from server : %s", argv[1]);
    }

    // connect
    if(connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0){
        err_sys("connect error ...");
    }

    // client从stdin读取用户输入，通过socket发送给server
    str_cli(stdin, sockfd);

    exit(0);
}

// 把sockfd/ fp都设置为static全局变量，方便主线程、子线程共享
static int sockfd_shared;
static FILE *fp_shared;

/**
 * 这个函数供tcp_echo_client使用
 * 1.作用是把标准输入stdin的内容写入socket，
 * 然后再通过socket把内容发送给server
 * 2.读取socket中(server发送过来)的内容，
 * 然后输出到标准输出stdout
 * 
 * @param fp file pointer to the input file ，比如标准输入stdio
 * @param sockfd file descriptor of the socket
 * 
*/
void str_cli(FILE *fp_arg, int sockfd_arg){
    
    pthread_t tid;
    char recvline[MAXLINE];

    sockfd_shared = sockfd_arg;
    fp_shared = fp_arg;

    /**
     * 一旦和socket server建立了连接，就创建子线程
     * 子线程用于处理socket client stdin -> socket server这条链路
    */
    Pthread_create(&tid, NULL, copyto, NULL);
    printf("child thread created ... tid: %d \n", tid); 

    // 主线程还是负责处理socket server -> socket client stdout这条链路
    while(Readline(sockfd_shared, recvline, MAXLINE) > 0){
        Fputs(recvline, stdout);
    }
}

void copyto(void *arg){
    char sendline[MAXLINE];

    while(Fgets(sendline, MAXLINE, fp_shared) != NULL){
        Writen(sockfd_shared, sendline, strlen(sendline));
    }

    // socket连接结束之后，关闭连接
    Shutdown(sockfd_shared, SHUT_WR); 
    return (NULL);
}
