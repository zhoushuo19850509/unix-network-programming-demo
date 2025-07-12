
#include "unp.h"
#include "web.h"


/**
 * 主代码
 * 
 * @功能 
 * 模拟浏览器打开一个HTML页面，这个页面可能会下载N个链接，本程序就是负责下载主页面和N个链接
 * 
 * @目标
 * 为了说明non-blocking connect的用法
 * 
 * @程序用法
 * web 3 www.foobar.com / image1.gif image2.gif image3.gif \
 * image4.gif image5.gif image6.gif image7.gif
 * 
 * arg[0] 程序名
 * arg[1] 创建3个连接
 * arg[2] 要访问的网址的名称host 比如: www.foobar.com 
 * arg[3] 要访问的HTML页面, 比如 index.html 这里 / 意思是root page
 * arg[4] - arg[10] HTML页面中包含的下载链接,这里是gif文件
 * 
*/
int main(int argc, char **argv){

    int maxnconn; // 最多启动多少个连接
    int error;
    int i;
    fd_set rs, ws;
    int n;
    int flags; // 记录某个下载链接文件file的状态(待处理、连接中、读取中、完成)
    int fd;
    char buf[MAXLINE]; // buffer for read from socket connect

    //1.解析参数argc/argv

    // 判断参数合法性： 至少包含5个参数
    if(argc < 5){
        err_quit("usage: web <#conns> <hostname> <homepage> <file1> ...");
    }

    // 最大连接，从argv中解析
    maxnconn = atoi(argv[1]);

    // 2.处理多少个需要下载的链接文件，如果超过了MAXFILES，就以MAXFILES为准
    nfiles = min(argc - 4, MAXFILES);
    for( i = 0; i < nfiles; i++){
        file[i].f_name = argv[i + 4];
        file[i].f_host = argv[2];
        file[i].f_flags = 0;   // 各个需要下载的链接文件，初始状态为0
    }    
    printf("nfiles = %d\n ", nfiles);

    /**
     * 3.下载home_page
     * argv[2]: hostname 
     * argv[3]: homepage 如果是"/" 代表root page
    */
    printf("hostname: %s \n ",argv[2]);
    printf("homepage: %s \n ",argv[3]);
    home_page(argv[2], argv[3]);

    // 初始化各种数据
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    maxfd = -1;
    nconn = 0;
    nlefttoread = nlefttoconn = nfiles;

    while( nlefttoread > 0){
        printf("nlefttoread: %d \n", nlefttoread);

        /**
         * 4.创建连接
         * 这个while循环的功能，是利用有限的连接资源，为各个要下载的链接文件创建连接
         * (1)每次while循环最多只处理一个file的连接工作
         * (2)nconn < maxnconn 这个判断的意思：socket连接数最大不能超过maxnconn
         *  (就是程序启动时指定的那个参数argv[1])
        */
        while( nconn < maxnconn && nlefttoconn > 0){
            // 这个for循环的作用是从file数组中找到第一个状态为0(待处理)的文件
            for(i = 0; i < nfiles; i++){
                if( file[i].f_flags == 0){
                    break;
                }
            }
            if(i == nfiles){
                err_quit("nlefttoconn : %d but nothing found \n", nlefttoconn);
            }
            start_connect(&file[i]);
            // printf("finish start_connect() of file index : %d \n", i);
            nconn++;
            nlefttoconn--;            
        }

        /**
         * 5.调用select()开启监听
         * 等到nonblocking连接全部发起字后，
         * 就开始通过Select()监听各个要下载的链接文件fd的IO事件
        */
        rs = rset;
        ws = wset;
        // printf("start select ... maxfd:%d \n", maxfd);
        n = Select(maxfd +1, &rs, &ws, NULL, NULL);
        // printf("finish select ... \n");


        for( i = 0; i < nfiles; i++){
            flags = file[i].f_flags;
            // printf("flags: %d of file index: %d \n", flags, i);

            // 1.先剔除那些未开始连接，或者已经下载完毕的文件
            if(flags == 0 || flags & F_DONE){
                continue;
            }
            fd = file[i].f_fd;
            /**
             * 2.再处理那些状态为CONNECTING的文件，如果捕捉到这些文件的IO事件，
             * 那说明是连接成功了，那就调用write_get_cmd()开始下载
            */
           if(flags & F_CONNECTING 
                && (FD_ISSET(fd, &rs) || FD_ISSET(fd, &ws))){                
                // printf("deal connecting IO event\n");
                n = sizeof(error);
                // 处理连接失败的情况
                if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &n) < 0 || error !=0){
                    err_ret("non-blocking connect failed for %s", file[i].f_name);        
                }
                printf("non-blocking connect established for %s\n", file[i].f_name);
                /**
                 * 一旦连接成功，在本场景下只有read from socket，
                 * write动作没有了，所以可以从wset中把fd清理掉
                */
                FD_CLR(fd, &wset); 
                // 一旦连接成功，就可以调用write_get_cmd()下载链接文件了
                write_get_cmd(&file[i]);
           }
           /**
             * 3.最后处理那些状态为READING的文件，
             * 捕捉到这些文件的IO事件，那说明是可以下载了，那就调用Read()直接下载就行了
            */
           else if(flags & F_READING && FD_ISSET(fd, &rs)){
                // printf("deal reading IO event\n");
                if( (n = Read(fd, buf, sizeof(buf))) == 0){
                    printf("end-of-file on: %s\n", file[i].f_name);
                    Close(fd);  // 释放fd资源
                    file[i].f_flags = F_DONE; // file状态设置为已完成
                    FD_CLR(fd, &rset);
                    nconn--;  // 释放计数，后续新的下载链接请求对应的 socket connect可以发起了
                    nlefttoread--; 
                }else{
                    printf("read %d bytes from file: %s \n", n, file[i].f_name);
                }
           }
        }

    }


    

}





