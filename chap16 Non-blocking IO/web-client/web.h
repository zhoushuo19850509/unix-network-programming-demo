#include "unp.h"

#define MAXFILES 20
#define SERV "80"  // server port

/**
 * 定义file结构体
 * web.c程序的参数包括HTML页面中各个需要单独下载的链接文件，比如image1.gif
 * 那么file结构体就是代表了这个链接文件的
*/
struct file{
    char *f_name; // 链接文件的file name 比如”image1.gif“
    char *f_host; // 链接文件所在的服务器的hostname 比如”www.foobar.com“
    int f_fd; // 下载该链接文件对应的socket fd
    // socket non-blocking connect status，
    // 具体来说，这些状态就是后续的那组宏定义： F_CONNECTING啥的
    int f_flags; 
} file[MAXFILES];

// 这组宏定义，定义了文件处理状态(socket connect状态)
// 状态0 待连接
// 状态1 发起non-blocking connect之后，等待连接结果中... 用于start_connect.c
#define F_CONNECTING 1  // connect() 连接中
// 状态2 connect() 连接完成之后，发起HTTP GET请求以后，读取response数据中，用于start_connect.c
#define F_READING 2     
// 状态3 
#define F_DONE 4        // all done

#define GET_CMD "GET %s HTTP/1.0\r\n\r\n"   // 通过HTTP GET请求访问web服务的命令

// 定义一些全局变量
int maxfd;
int nconn;    // 经过start_connect.c处理之后(发起non-blocking connect)，这个值计数加1
int nfiles;   // 程序参数指定了有多少个需要下载的链接文件
int nlefttoread; // 待read的文件数量
int nlefttoconn; // 待connect成功的文件数量
// 需要全局共享的fd_set 
// 比如start_connect()中通过non-blocking方式连接后的file fd就要登记到这里，用于后续处理连接结果
// 比如write_get_cmd()发起HTTP GET请求之后，file fd也要登记到这里，用于获取后续的response
fd_set rset, wset;  

// 声明各个funtion
void home_page(const char *host, const char *fname);
void start_connect(struct file *fptr);
void write_get_cmd(struct file *fptr);