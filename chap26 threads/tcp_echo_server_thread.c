#include "unp.h"

/**
 * 功能： tcp echo server 负责把client发送过来的内容原样发送回去
 * 目标： 为了说明socket server创建一个新的进程，处理客户端socket请求的用法
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
void str_echo(int sockfd);
static void *doit(void *arg);

int main(){

    int listenfd; // 监听的socket
    // 一旦接收到来自客户端的socket连接，就创建一个新的线程,tid就这个子线程的id
    pthread_t tid;
    // 这个int指针，指向的int值，是socket server接收到来自客户端的socket连接的fd
    int *iptr;
    socklen_t clilen; // length of client address length
    struct sockaddr_in servaddr; // socket server address
    struct sockaddr_in cliaddr;  // socket client address
    
    // 1.创建socket server
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    // 2.创建server address
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // 指定server port，定义在unp.h，为9827
    servaddr.sin_port = htons(SERV_PORT);  

    // 3.绑定端口(bind)
    bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

    // 4.监听端口(listen)
    listen(listenfd, LISTENQ);

    // 5.循环接收来自客户端的socket请求(accept)
    for(;;){
        clilen = sizeof(cliaddr);
        iptr = Malloc(sizeof(int));
        *iptr = accept(listenfd, (SA *) &cliaddr, &clilen);

        // 6.接收到来自客户端的socket请求之后，启动一个新的线程予以处理
        Pthread_create(&tid, NULL, &doit, iptr);
        printf("server accept new connect and create new thread to handle it ... tid: %d \n", tid); 
    }

}

/**
 * 子线程要做的事情，很简单，就是接收参数(connected fd)，
 * 然后调用str_echo() echo back就行了
*/
static void *doit(void *arg){
    int connfd;

    connfd = *((int *) arg);  // 参数传递给connfd
    free(arg);
    // 子线程像是daemon process那样，在背景执行就行，不用等待它结束
    Pthread_detach(pthread_self()); 
    str_echo(connfd);
    Close(connfd);
    return (NULL);
}

// 从socket中读取内容，并把内容重新写入到socket，返回给client
void str_echo(int sockfd){
    printf("server enter str_echo ...\n");
    ssize_t n;
    char buf[MAXLINE];

    again:
        while( (n = read(sockfd, buf, MAXLINE)) > 0){
            Write(sockfd, buf, n);
        }

        if( n < 0 && errno == EINTR){
            // 如果是因为回车，就跳转到"again"，继续从socket读取
            goto again;
        }else if( n < 0){
            // 如果是其他异常，就打印error日志
            err_sys("str_echo error , read error!");
        }
}
