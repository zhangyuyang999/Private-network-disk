#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <unistd.h>
#include <dirent.h>
#include<grp.h>
#include<pwd.h>
#include<time.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<errno.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <signal.h>
#include<pthread.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<ctype.h>
#include<sys/epoll.h>
#include <sys/sendfile.h>
 #include <sys/mman.h>

#define ARGS_CHECK(argc,num){if(argc!=num){\
    fprintf(stderr,"参数错误！\n");\
    return -1;\
    }}
#define ERROR_CHECK(ret,num,msg){if(ret==num){\
    perror(msg);\
    return -1;\
    }}
#define TERROR_CHECK(ret,msg){if(ret != 0){\
    printf("%s is failed,%s\n",msg,strerror(ret));\
    return -1;\
    }}

#define FILENAME "file2"

//父进程管理子进程的结构体
typedef struct{
    pid_t pid;//子进程的pid
    int fd;//子进程管道的对端，用来与子进程通信
    short flag;//标记子进程是否忙碌，0表示非忙碌，1表示忙碌
}proc_data_t;

//the data structure for transmitting file
typedef struct{
    int data_len;
    char buf[1000];
}train_t;

int creat_child(proc_data_t *pArr,int proc_num);//creat child thread circularly
int child_handle(int pfd);//child thread embark mission 
int tcp_init(char *ip,char *port,int *psfd);//creat a socket and bind ip and prot,then start listening  
int epoll_add(int epfd,int fd);//add object that parent thread want to monitor
int send_fd(int pipefd,int new_fd);//parent thread send client's new_fd to child thread
int recv_fd(int pipefd,int *new_fd);//child thread receive client's new_fd from parent thread 
int trans_file(int new_fd);//child thread embark transmitting file 
void do_cd(int new_fd,char *buf);//
int sendn(int fd_send, char* send_buf, int len);
int recvn(int sfd,void *buf,int len);
void do_ls(int new_fd) ;
int map_recv(int sfd,void* pstart,int len);


























