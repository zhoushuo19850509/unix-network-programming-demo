#include <stdio.h>
#include "unp.h"


/**
 * 功能： tcp echo  在console(stdin)发送任何字符给server，
 *       然后接收来自server echo back的内容，展示在console(stdout)中
 * 目标： 为了说明socket client和socket server交互的功能
 * 环境： playground(imac-home)，这个环境需要预先准备好unp.h
 * 用法： 
 *  1.配置Makefile
 *  2.执行：  make
 *  2.服务端启动tcp echo server
 *      ./tcp_echo_server
 *  3.客户端启动tch echo client
 *    ./tcp_echo_client  127.0.0.1
 *    然后在客户端随便输入一些内容，就能得到server echo back的内容
 * 
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
void str_cli(FILE *fp, int sockfd){
    printf("client enter str_cli() ...\n");
    char sendline[MAXLINE];  // buffer1 保存client发送给server的内容
    char recvline[MAXLINE];  // buffer2 保存client接收来自server的内容
    /**
     * 1.通过Fgets()函数，把文件内容(比如标准输入stdin)放到buffer:  sendline
    */
    while(Fgets(sendline, MAXLINE, fp) != NULL){ 
        /**
         * 2.通过Writen()函数，把buffer中的内容写入 socket，
         * 然后通过socket把内容发送给server
        */
        Writen(sockfd, sendline, strlen(sendline));
        /**
         * 3.通过 Readline()函数，读取socket中(server发送过来)的内容，放到buffer
        */
        if(Readline(sockfd, recvline, MAXLINE) == 0){
            err_quit("str_cli: ");
        }
        /**
         * 4.通过Fputs()函数，把buffer中的内容放到标准输出
        */
        Fputs(recvline, stdout);
    }
}
