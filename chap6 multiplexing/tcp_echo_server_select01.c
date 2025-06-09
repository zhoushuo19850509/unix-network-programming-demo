#include "unp.h"

/**
 * 功能： tcp echo server 负责把client发送过来的内容原样发送回去
 * 目标： 为了说明socket server通过select()函数，同时处理多个客户端socket请求的用法
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

    int listenfd; // 监听的socket
    int maxfd; 
    int sockfd;
    int client[FD_SETSIZE]; // 这个数组存放的是所有客户端连接socket fd
    int maxi;
    int i;
    int n;
    /**
     * nready是Select()的返回值，意思是有多少socket fd需要处理
     * 备注：这里的socket fd既可以是新客户端连接上来的socket fd
     *      也可是客户端通过之前已经连接的socket fd发送数据过来
    */
    int nready;
    fd_set allset; // 登记所有连接上来的client socket fd
    fd_set rset;
    char buf[MAXLINE];

    int connectfd; // 接收到来自客户端的socket连接
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
    printf("start listening on port: %d", SERV_PORT);

    maxfd = listenfd;
    maxi = -1;
    // 初始化fd client set
    for(i = 0; i < FD_SETSIZE; i++){
        client[i] = -1;
    }

    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    /**
     * 5.循环，这个循环做两个事情
     * (1)代码块1 FD_ISSET(listenfd,..。) 
     *   这个代码块接收来自客户端的socket请求(accept)，
     * (2)代码块2 for(i = 0; ...)
     *   处理客户端通过原有socket连接发送过来的数据
    */
    for(;;){
        rset = allset;

        /**
         * Select()既可以处理新的socket连接请求，
         * 也可以处理通过原有socket请求管道发送过来的数据
         * 
        */
        nready = Select(maxfd + 1, &rset, NULL, NULL, NULL);
        printf("current nready: %d" , nready);
        
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
        if(FD_ISSET(listenfd, &rset)){
            clilen = sizeof(cliaddr);
            // connectfd是最新连接上来的socket fd
            connectfd = Accept(listenfd, (SA *) &cliaddr, &clilen);
            
            // 这个for循环是从client数组中找一个空位，存放新socket连接 fd
            for(i = 0; i < FD_SETSIZE; i++){
                if(client[i] < 0){
                    client[i] = connectfd;
                    break;
                }
            }
            // 遍历client数组，已经没有空位了，没法再接收新的client socket连接请求了
            if(i == FD_SETSIZE){
                err_quit("too many clients...");
            }
            // 把新连接登记到allset中
            FD_SET(connectfd, &allset);
            // 更新maxfd，maxfd保存的是所有连接上来的client socket fd中最大的那个编号
            if(connectfd > maxfd){
                maxfd = connectfd;
            }
            // 更新maxi，maxi保存的是最新的连接上来的client socket在client数组中的index
            if(i > maxi){
                maxi = i;
            }
            /**
             * nready的含义我们已经清楚了(参考nready定义处的注释)
             * 这里if()判断的意思是，
             * Select()的结果nready如果为1 ，就意味着这次Select()接收到一个新的socket请求
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
        for(i = 0; i <= maxi; i++){
            // 先剔除掉fd array中空的元素，不处理
            if( (sockfd = client[i]) < 0){
                continue;
            }
            /**
             * 然后处理实际有可读内容的socket
             * 读取完成后关闭socket
            */
            if(FD_ISSET(sockfd, &rset)){
                if((n = Read(sockfd, buf, MAXLINE)) == 0){
                    Close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client[-1] = -1;
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


