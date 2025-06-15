
#include "unp.h"

/**
 * 功能： tcp echo  在console(stdin)发送任何字符给server，
 *       然后接收来自server echo back的内容，展示在console(stdout)中
 * 目标： 本代码主要是为了说明non-blocking io的用法。有两块内容需要说明：
 *      1.把本代码涉及的file fd(sockfd /stdin fd/ stdout fd)属性设置为non-blocking
 *      2.设计一套缓存体系，支持多次反复读写
 *   
 * 环境： playground(imac-home)，这个环境需要预先准备好unp.h
 * 用法： 
 *  1.配置Makefile
 *  2.执行：  make
 *  2.服务端启动tcp echo server（随便挑一个之前开发的server）
 *      ./tcp_echo_server_poll01
 *  3.客户端启动tch echo client
 *    ./tcp_echo_client_nonblocking  127.0.0.1
 *    然后在客户端随便输入一些内容，就能得到server echo back的内容
 * 备注：
 *    tcp echo server启动的端口是9877，定义在unp.h中
*/
void str_cli(FILE *fp, int sockfd);
char *gf_time(void);

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
    int val;
    fd_set rset;  // 这个fd set保存了read相关的fd， 包括： stdin fd 和 socket fd
    fd_set wset;  // 这个fd set保存了write相关的fd， 包括： stdout fd 和 socket fd
    char to[MAXLINE];
    char fr[MAXLINE];
    int stdineof;  // 标志stdin是否结束 为1就说明结束了
    /**
     * 以下这4个char * 指针，是buffer pointer，和读取缓存内容有关
     * 具体参考《主题4 non-blocking read/write主题解读》
    */
    char *toiptr; 
    char *tooptr; 
    char *friptr; 
    char *froptr; 
    ssize_t n;    // n bytes read from buffer
    ssize_t nwriten;  // n bytes write to buffer

    /**
     * set fd to non-blocking
     * 由Fcntl()实现，Fcntl()应该是linux system call interface
     * 功能是设置fd的属性
    */
    val =Fcntl(sockfd, F_GETFL, 0);
    Fcntl(sockfd, F_SETFL, val | O_NONBLOCK);

    val = Fcntl(STDIN_FILENO, F_GETFL, 0);
    Fcntl(STDIN_FILENO, F_SETFL, val | O_NONBLOCK);

    val = Fcntl(STDOUT_FILENO, F_GETFL, 0);
    Fcntl(STDOUT_FILENO, F_SETFL, val | O_NONBLOCK);

    // 初始化buffer pointers
    toiptr = tooptr = to;
    friptr = froptr= fr;
    stdineof = 0;

    maxfdp1 = max(max(STDIN_FILENO, STDOUT_FILENO),sockfd) + 1;

    // for循环不断执行select()，读取来自socket/stdin的数据
    for(;;){

        // 指定我们感兴趣的fd，和read相关的fd纳入rset/和write相关的fd纳入wset
        FD_ZERO(&rset);
        FD_ZERO(&wset);

        /**
         * FD_SET我们之前已经在select() API中已经实践过了，
         * 比如我们想要select()捕获来自stdin的IO事件，就要把stdin对应的fd(fileno)注册到rset
         * 具体参考tcp_echo_client_select02.c
        */
        if(stdineof == 0 && toiptr < &to[MAXLINE]){
            FD_SET(STDIN_FILENO, &rset); // read from stdin
        }
        if(friptr < &fr[MAXLINE]){
            FD_SET(sockfd, &rset);  // read from socket
        }
        if(tooptr != toiptr){
            FD_SET(sockfd, &wset);  // write to socket
        }
        if(froptr != friptr){
            FD_SET(STDOUT_FILENO, &wset);  // write to stdout
        }

        // 这里开始调用Select()监听各种IO事件
        Select(maxfdp1, &rset, &wset, NULL, NULL);

        // 处理stdin，从stdin读取用户输入
        if(FD_ISSET(STDIN_FILENO, &rset)){
            n = read(STDIN_FILENO, toiptr, &to[MAXLINE] - toiptr);
            if( n < 0){
                // error from stdin
                fprintf(stderr, "%s: read error from stdin, print n: \n", 
                    gf_time(), n);
                if(errno != EWOULDBLOCK){
                    err_sys("read error on stdin");
                }
            }else if( n == 0){
                // finish stdio
                fprintf(stderr, "EOF on stdin \n");
                stdineof = 1; // stdio结束了
                if(tooptr == toiptr){
                    Shutdown(sockfd, SHUT_WR); // send FIN and close the socket
                }
            }else{
                // n > 0 说明从stdin读取到数据
                fprintf(stderr, "%s: read %d bytes from stdin \n",gf_time(), n);
                toiptr += n;
                FD_SET(sockfd, &wset);
            }
        }

        // 处理sockfd，从socket读取数据
        if(FD_ISSET(sockfd, &rset)){
            if( (n = read(sockfd, friptr, &fr[MAXLINE] - friptr)) < 0){
                // error from socket
                if(errno != EWOULDBLOCK){
                    err_sys("read error on socket");
                }
            }else if( n == 0){
                // finish stdio
                fprintf(stderr, "EOF on socket \n");
                stdineof = 1; // stdio结束了
                if(stdineof){
                    return; // stdin结束了，normal termination
                }else{
                    // 如果socket 结束，而stdin还没有结束，说明server主动断开了socket连接
                    err_sys("str_cli: server terminated prematurely");
                }

            }else{
                // n > 0 说明从socket读取到数据
                fprintf(stderr, "%s: read %d bytes from socket \n", 
                gf_time(), n);
                friptr += n;
                FD_SET(STDOUT_FILENO, &wset);
            }
        }

        // 处理标准输出stdout : STDOUT_FILENO，输入信息到stdout
        if(FD_ISSET(STDOUT_FILENO, &wset) && ( (n = friptr - froptr) > 0)){
            if((nwriten = write(STDOUT_FILENO, froptr, n)) < 0 ){
                // error write to stdout
                if(errno != EWOULDBLOCK){
                    err_sys("write error to stdout");
                }
            }else{
                fprintf(stderr, "%s: write %d bytes to stdout \n", 
                   gf_time(), n);
                froptr += nwriten;
                if(froptr == friptr){
                    froptr = friptr = fr; // back to beginning of buffer
                }
            }
        }

        // 处理sockfd，把数据发送到socket管道
        if(FD_ISSET(sockfd, &wset) && ( (n = toiptr - tooptr) > 0)){

            if((nwriten = write(sockfd, tooptr, n)) < 0 ){
                // error write to socket
                if(errno != EWOULDBLOCK){
                    err_sys("write error to socket");
                }
            }else{
                fprintf(stderr, "%s: write %d bytes to socket \n", 
                    gf_time(), n);
                tooptr += nwriten;
                if(tooptr == toiptr){
                    toiptr = tooptr = to;  // back to beginning of buffer
                    if(stdineof){
                        Shutdown(sockfd, SHUT_WR); // send FIN
                    }
                }
            }
        }
    }
}

char *gf_time(void){
    struct timeval tv;
    static char str[30];
    char *ptr;

    if(gettimeofday(&tv, NULL) < 0){
        err_sys("gettimeofday error");
    }

    ptr = ctime(&tv.tv_sec);
    strcpy(str, &ptr[11]);

    snprintf(str + 8, sizeof(str) - 8, ".%06ld", tv.tv_usec);

    return str;
}