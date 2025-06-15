#include <stdio.h>
#include "unp.h"


/**
 * 功能： tcp echo  在console(stdin)发送任何字符给server，
 *       然后接收来自server echo back的内容，展示在console(stdout)中
 * 目标： 本代码基于tcp_echo_client_select01.c 有如下的升级：
 *      1.引入buffer的概念，用read()函数代替
 *        Readline(from socket) and Fget(from stdin)
 *      2.引入Shutdown的概念，当stdin关闭的时候，正常断开socket连接
 *        这意思就是client主动关闭连接
 *   
 * 环境： playground(imac-home)，这个环境需要预先准备好unp.h
 * 用法： 
 *  1.配置Makefile
 *  2.执行：  make
 *  2.服务端启动tcp echo server
 *      ./tcp_echo_server
 *  3.客户端启动tch echo client
 *    ./tcp_echo_client_select02  127.0.0.1
 *    然后在客户端随便输入一些内容，就能得到server echo back的内容
 *  4.尝试退出tcp echo server
 *  5.此时，按照预期，tcp echo client能够立即接收到来自tcp echo server socket信息： 
 *    以为socket server程序退出导致的REST请求
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
    int maxfdp1;   // max fd正如其名，取stdin fd 和socket fd中比较大的那个
    fd_set rset;  // 这个fd set保存了stdin fd 和 socket fd这两个fd
    
     = 0; // 标志stdin是否关闭 0 ： 未关闭； 1 ： 关闭
    int n;
    /**
     * buffer作为缓存， 既保存client 从stdin读取的内容
     * 又保存client接收来自server socket的内容
    */
    char buff[MAXLINE];  

    FD_ZERO(&rset);

    // for循环不断执行select()，读取来自socket/stdin的数据
    for(;;){
        if(stdineof == 0){
            FD_SET(fileno(fp), &rset);
        }
        FD_SET(sockfd, &rset);

        // max fd 取stdin 和socket中比较大的那个
        maxfdp1 = max(fileno(fp), sockfd ) + 1;

        // select()函数监听rset中登记的fd
        Select(maxfdp1, &rset, NULL, NULL, NULL);

        /**
         * 如果从socket可以读取到数据,那就处理从socket读取的数据
         * 处理方式和之前chap5/tcp_echo_client类似
         * 读取socket，并输出到stdout
        */
        if(FD_ISSET(sockfd, &rset)){
            // 通过 Read()函数，读取socket中(server发送过来)的内容，放到buffer
            if( (n = Read(sockfd, buff, MAXLINE)) == 0){
                if(stdineof == 1){
                    // 读取到stdineof，说明socket是正常关闭的，这里只需要return即可
                    return;    
                }else{
                    // socket异常关闭，报错
                    err_quit("str_cli: server terminated prematurely");
                }
            }
            // 通过Fputs()函数，把buffer中的内容放到标准输出
            Writen(fileno(stdout), buff, n);    
        }

        /**
         * 如果从stdin可以读取到数据，那就处理从socket读取的数据
         * 处理方式和之前chap5/tcp_echo_client类似
         * 读取socket，并输出到stdout
        */
        if(FD_ISSET(fileno(fp), &rset)){
            /**
             * 通过Read()函数，把文件内容(比如标准输入stdin)放到buffer
             * 如果Read()函数返回0，说明stdin关闭
            */
            if( (n = Read(fileno(fp), buff, MAXLINE)) == 0){   
                printf("clinet stdin closed ...");
                stdineof = 1;   // 先置stdin的标志位
                Shutdown(sockfd, SHUT_WR); // 正常关闭socket连接
                FD_CLR(fileno(fp), &rset); // 把stdin fd 从rset集合清除
                continue; // 这里继续for循环，用于client socket正常关闭               
            }
            /**
             * 调用Writen()函数，把buffer中的内容写入 socket，
             * 然后通过socket把内容发送给server
            */
            Writen(sockfd, buff, n);    
        }
    }
}
