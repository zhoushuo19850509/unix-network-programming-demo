#include "web.h"

/**
 * 针对N个要下载的链接文件，启动non-blocking connet
 * 这是整个web程序的核心
 * 这个代码的套路和之前开发的day_time_client_conn_nonblocking.c类似
 * 1.把socket fd设置为non-blocking
 * 2.调用connect()尝试连接
 * 3.有可能直接就connect成功，那皆大欢喜；
 * 4.有可能没有connect成功，那就调用FD_SET设置socket fd ，
 *   由后续的程序监听socket fd connect IO，直到connect成功为止
 * @param fptr 这是第一个指向struct file的指针，
 *     包含了某个HTML页面中某个链接文件，
 *     比如我们启动程序的时候指定的gif文件：image1.gif 
*/
void start_connect(struct file *fptr){

    // 根据fptr中保存的host，组装addrinfo，
    struct addrinfo *ai;
    ai = Host_serv(fptr->f_host, SERV, 0, SOCK_STREAM);
    int flags; // 保存socket的属性
    int n;
    int fd;

    /**
     * 根据addrinfo，创建client和server之间的socket
     * (注意：此时还未调用connect()建立连接)
    */
    fd = Socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    fptr->f_fd = fd;
    printf("start connect for : %s , fd: %d \n" , fptr->f_name, fd);

    // 把这个socket fd设置为non-blocking
    flags = Fcntl(fd, F_GETFL, 0);  
    Fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    /**
     * start non-blocking connect
     * 在调用non-blocking模式下的 connect()方法，尝试丽娜姐
     * 
     * 如果connect()返回小于0，说明暂时还未连接成功
     * 1.处理异常
     * 2.设置file状态(设置为CONNECTING)
     * 3.设置socket fd的IO监听，由后续的程序监听socket fd connect IO，直到connect成功为止
    */
    if( (n = connect(fd, ai->ai_addr, ai->ai_addrlen)) < 0){
        printf("not connected now ... \n");
        /**
         * 在non-blocking模式下，connect()如果返回EINPROGRESS异常是正常，
         * 而如果是其他异常，那就是真正的异常，就直接返回了
        */
        if(errno != EINPROGRESS){
            err_sys("non-blocking error...");
        }
        // 设置file状态
        fptr->f_flags = F_CONNECTING; // socket non-blocking connecting 
        // 设置socket fd的IO监听
        FD_SET(fd, &rset);
        FD_SET(fd, &wset);

        if(fd > maxfd){
            maxfd = fd;
        }
    }else if( n >= 0){  // 如果n >=0 说明直接连接成功，可以发起HTT GET请求了。
        printf("connected immediately ,start GET now ...");
        write_get_cmd(fptr);
    }


}






