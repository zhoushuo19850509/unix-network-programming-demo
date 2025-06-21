

#include "unp.h"

/**
 * 功能： tcp echo  在console(stdin)发送任何字符给server，
 *       然后接收来自server echo back的内容，展示在console(stdout)中
 * 目标： 本代码主要是为了说明fork的用法。
 *   
 * 环境： playground(imac-home)，这个环境需要预先准备好unp.h
 * 用法： 
 *  1.配置Makefile
 *  2.执行：  make
 *  2.服务端启动tcp echo server（随便挑一个之前开发的server）
 *      ./tcp_echo_server_poll01
 *  3.客户端启动tch echo client
 *    ./tcp_echo_client_fork  127.0.0.1
 *    然后在客户端随便输入一些内容，就能得到server echo back的内容
 * 备注：
 *    tcp echo server启动的端口是9877，定义在unp.h中
*/
void str_cli(FILE *fp, int sockfd);

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

void str_cli(FILE *fp, int sockfd){
    
    pid_t pid;
    char recvline[MAXLINE]; // buffer for read from socket
    char sendline[MAXLINE]; // buffer for write to socket

    if( (pid = Fork())  == 0){  // 启动一个新的进程处理
        while(Readline(sockfd, recvline, MAXLINE) > 0){
            Fputs(recvline, stdout);
        }
        kill(getpid(), SIGTERM);
        exit(0);
    }

    while(Fgets(sendline, MAXLINE, fp)){
        Writen(sockfd, sendline, strlen(sendline));
    }

    Shutdown(sockfd, SHUT_WR);  // socket通信结束后，关闭socket连接
    pause();
    return;

}
