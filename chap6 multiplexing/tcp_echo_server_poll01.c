#include "unp.h"
#define OPEN_MAX                10240   /* max open files per process - todo, make a config option? */


/**
 * 功能： tcp echo server 负责把client发送过来的内容原样发送回去
 * 目标： 为了说明socket server通过 poll()函数，同时处理多个客户端socket请求的用法
 * 环境： playground(imac-home)，这个环境需要预先准备好unp.h
 * 用法： 
 *  1.配置Makefile
 *  2.执行：  make
 *  2.服务端启动tcp echo server
 *      ./tcp_echo_server_select01
 *  3.客户端启动tch echo client
 *    ./tcp_echo_client_select02 127.0.0.1
 *    然后在客户端随便输入一些内容，就能得到server echo back的内容
 *  4.按照同样的方式，启动多个客户端，都能够实现echo功能
 *  5.查看tcp echo server进程，只有一个。
 * 备注：
 *    tcp echo server启动的端口是9877，定义在unp.h中
*/

int main(){

    int listenfd; // server socket listening fd
    int sockfd;    // 保存在client数组中来自客户端的socketfd
    int connectfd; // 接收到来自客户端的socket连接
    struct pollfd client[OPEN_MAX]; // 这个数组存放的是poll检测到的socket poll fd
    int maxi;
    int i;
    ssize_t n;
    /**
     * nready是Select()的返回值，意思是有多少socket fd需要处理
     * 备注：这里的socket fd既可以是新客户端连接上来的socket fd
     *      也可是客户端通过之前已经连接的socket fd发送数据过来
    */
    int nready;
    char buf[MAXLINE];

    
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
    Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

    // 4.监听端口(listen)
    Listen(listenfd, LISTENQ);
    printf("start listening on port: %d", SERV_PORT);

    /**
     * 把server socket 登记到client数组第一个元素
     * 从这里我们也可以看到client数组元素： pollfd的组成
    */
    client[0].fd = listenfd;
    client[0].events = POLLRDNORM;
    // 初始化client数组各个元素： pollfd
    for(i = 1; i < OPEN_MAX; i++){
        client[i].fd = -1;
    }

    maxi = 0;

    /**
     * 5.循环，这个循环做两个事情
     * (1)代码块1 FD_ISSET(listenfd,..。) 
     *   这个代码块接收来自客户端的socket请求(accept)，
     * (2)代码块2 for(i = 0; ...)
     *   处理客户端通过原有socket连接发送过来的数据
    */
    for(;;){
        /**
         * Select()既可以处理新的socket连接请求，
         * 也可以处理通过原有socket请求管道发送过来的数据
         * 
        */
        nready = Poll(client, maxi + 1, INFTIM);
        // printf("current nready: %d" , nready);
        
        /**
         * 代码块1
         * 这个if()处理新socket连接
         * FD_ISSET(listenfd,..。)大于0的意思是，
         * 系统感受到listenfd(就是server socket对应的socket fd)有IO事件
         * 也就是意味着有新的socket连接捡来了
         * 
         * 具体步骤如下：
         * 1.检测到server socket有IO动作(FD_ISSET()干的活)
         * 2.通过Accept()获取到client socket fd
         * 3.client socket fd登记到数据client中
         * 
        */
        if(client[0].revents & POLLRDNORM ){
            clilen = sizeof(cliaddr);
            // connectfd是最新连接上来的socket fd
            connectfd = Accept(listenfd, (SA *) &cliaddr, &clilen);
            
            // 这个for循环是从client数组中找一个空位，存放新socket连接 fd
            for(i = 1; i < OPEN_MAX; i++){
                if(client[i].fd < 0){
                    client[i].fd = connectfd;
                    break;
                }
            }
            // 遍历client数组，已经没有空位了，没法再接收新的client socket连接请求了
            if(i == OPEN_MAX){
                err_quit("too many clients...");
            }
            client[i].events = POLLRDNORM;

            // 更新maxi，maxi保存的是最新的连接上来的client socket在client数组中的index
            if(i > maxi){
                maxi = i;
            }
            /**
             * nready的含义我们已经清楚了(参考nready定义处的注释)
             * 这里if()判断的意思是，
             * poll()的结果nready如果为1 ，就意味着这次poll()接收到一个新的socket请求
             * 那么，在代码块1 中处理完本次新socket请求之后nready就归0了，
             * 在这种情况下，后续代码块2 for循环就没必要继续处理了
            */
            if(--nready <= 0 ){
                continue;
            }
        }

        /**
         * 代码块2 主要处理原有socket连接，客户端通过这些socket连接发送数据过来
         * 处理模式
         * 1.遍历数组client，遍历各个socket fd
         * 2.如果某个socket fd有新数据进来(FD_ISSET()干的活)
         * 3.通过Read()从socket读取数据
         * 4.如果Read()不到数据，说明socket关闭，就清理这个socket fd
         * 如果Read()读取到了数据，就把数据echo back to the client
        */
        for(i = 1; i <= maxi; i++){
            // 先剔除掉fd array中空的元素，不处理
            if( (sockfd = client[i].fd) < 0){
                continue;
            }
            /**
             * 然后处理实际有可读内容的socket
             * 读取完成后关闭socket
            */
            if(client[i].revents & (POLLRDNORM | POLLERR)){
                if((n = read(sockfd, buf, MAXLINE)) < 0){
                    if(errno == ECONNRESET){ // connect reset by client
                        Close(sockfd);
                        client[i].fd = -1;
                    }else{
                        err_sys("read error");
                    }
                }else if(n == 0){
                    // connect closed by client
                    Close(sockfd);
                    client[i].fd = -1;
                    
                }else{
                    // 把接收到的数据立即通过socket返回给客户端
                    Write(sockfd, buf, n);
                }
                if(--nready <= 0 ){
                    break;
                }
            }
        }
    }
}


