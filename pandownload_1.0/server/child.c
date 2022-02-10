#include "multi_process.h"
//该函数的作用：创建进程池
int creat_child(proc_data_t *parr,int proc_num)
{
    int i;
    pid_t pid;
    int ret;
    int fds[2];

    for(i=0;i<proc_num;i++)
    {
        ret = socketpair(AF_LOCAL,SOCK_STREAM,0,fds);
        ERROR_CHECK(ret,-1,"socketpair");

        pid = fork();
        if(0 == pid)//child thread
        {
            close(fds[0]);
            child_handle(fds[1]);//embark task
        }
        //parent thread
        close(fds[1]);
        parr[i].pid = pid;
        parr[i].fd = fds[0];
        parr[i].flag = 0;
    }      
    return 0;
}
//子进程处理函数：用来完成客户端发送过来的任务
int child_handle(int pipefd)
{
    int new_fd;
    int cmd_len = 0;//命令长度
    char cmd[1024] = {0};//命令buf
    int recv_ret = 0;//返回值
    while(1)
    {
        recv_fd(pipefd,&new_fd);
        printf("receive task:new_fd = %d,mission start!\n",new_fd);
        while(1)
        {
            bzero(cmd,sizeof(cmd));
            recv_ret=recv(new_fd,&cmd_len,4,0);//接收命令长度
            if(cmd_len == 0 || recv_ret == 0)
            {
                break;
            }
            recv_ret=recv(new_fd,cmd,cmd_len,0);//接收命令本身
            ERROR_CHECK(recv_ret,-1,"recv");
            if(strncmp("cd", cmd, 2) == 0)
            {
                do_cd(new_fd,cmd);
            }
            else if(strncmp("ls", cmd, 2) == 0)
            {
                do_ls(new_fd);
            }
            else if( strncmp("puts", cmd, 4)== 0 )
            {
                do_puts(new_fd,cmd);

            }
            else if( strncmp("gets", cmd, 4)== 0 )
            {
                do_gets(new_fd,cmd);

            }
            else if( strncmp("remove", cmd, 6)== 0 )
            {
                do_remove(new_fd,cmd);

            }
            else if(strncmp("pwd", cmd, 3) == 0) 
            {
                do_pwd(new_fd);
            }
            else 
            {
                continue ;
            }
        }
        printf("mission conplete\n");
        close(new_fd);//disconnecting with client 
        write(pipefd,"o",1);
    }
    return 0;
}
int do_cd(int new_fd,char *cmd) 
{
    int ret;
    int len;
    char dir[128]={0};
    sscanf(cmd +3, "%s", dir);
    ret = chdir(dir);
    if(ret == -1)//出错的三种情况：①没有此目录②目标是一个文件③没有参数
    {
        strcpy(dir,"no such file or directory!");//此处挖一个坑：如果客户端输入错误（如:没有该目录、打开一个普通文件等）打印对应的错误
    }
    else
    {

        getcwd(dir, 128);
    }
    len = strlen(dir);
    send(new_fd, &len, sizeof(int), 0);//先发长度
    send(new_fd, dir, len,0);//send the messege
    return 0;
}
int do_ls(int new_fd) 
{
    DIR* pdir = opendir("./");//打开当前文件目录
    struct dirent* pdirent ;
    int len ;
    char buf[1024] = {0};
    while((pdirent = readdir(pdir)))//读到最后一个文件时返回NULL
    {
        if(strncmp(pdirent->d_name, ".", 1) == 0 || strncmp(pdirent->d_name,"..", 2) == 0)
        {
            continue ;//不打印隐藏文件（以.开头的文件）
        }
        struct stat mystat;
        bzero(&mystat, sizeof(mystat));
        stat(pdirent->d_name, &mystat);
        bzero(buf, 1024);
        sprintf(buf, "%-20s %10ldB %-10s", pdirent->d_name, mystat.st_size,getpwuid(mystat.st_uid)->pw_name);
        len = strlen(buf);
        send(new_fd, &len, sizeof(int), 0);
        sendn(new_fd, buf, len);
    }
    //如果文件为空文件，必须发送一条回执消息，否则客户端会卡死
    len = 0;
    send(new_fd, &len, sizeof(int), 0);
    return 0;
}

int sendn(int fd_send, char* send_buf, int len)
{
    int sum = 0 ;
    int nsend ;
    while(sum < len)
    {
        nsend = send(fd_send, send_buf + sum, len - sum, 0);
        sum += nsend ;

    }
    return sum ;
}

int do_pwd(int new_fd)
{
    char buf[1024] ={0} ;   
    int len;
    getcwd(buf,sizeof(buf));
    len = strlen(buf);
    send(new_fd,&len,4,0);
    send(new_fd,buf,len,0);
    return 0;
}

int do_remove(int new_fd,char *cmd) 
{
    int ret;
    int len;
    char file_name[128]={0};
    sscanf(cmd +7, "%s", file_name);
    printf("%s\n",file_name);
    ret = unlink(file_name);
    if(ret == -1)//出错的三种情况：①没有此文件②目标是一个目录③没有参数
    {
        strcpy(file_name,"no such file! ");//此处挖一个坑：如果客户端输入错误,打印对应的错误
    }
    else
    {
        strcpy(file_name,"remove success!");
    }
    len = strlen(file_name);
    send(new_fd, &len, sizeof(int), 0);//先发长度
    send(new_fd,file_name, len,0);//send the messege
    return 0;
}
int do_gets(int new_fd,char *cmd)
{
    train_t t;
    char file_name[64] = {0};
    sscanf(cmd+4,"%s",file_name);
    int fd = open(file_name,O_RDONLY);
    if(fd == -1)
    {
        t.data_len = 0;//命令不合法（如文件错误）,回执0
        send(new_fd,&t,4,0);
    }
    else
    {
        t.data_len = 1;//命令合法，回执1
        send(new_fd,&t,4,0);

        //send the content of the file
        while((t.data_len = read(fd,t.buf,sizeof(t.buf))))//once reaching the end of the file ,t.data_len will be set zero
        {
            send(new_fd,&t,t.data_len+4,0);
        }
    }
    //send a zero on be half of the end of the file
    t.data_len = 0;
    send(new_fd,&t,4,0);
    return 0;
}
int do_puts(int new_fd,char *cmd)
{
    int data_len;
    recv(new_fd,&data_len,4,0);//接收命令回执消息，如果命令合法，受到1，否则受到0
    if(data_len == 0)//命令不合法，接收到客户端发过来的0，直接返回
    {
        return 0;
    }

    char buf[1024]={0};
    char file_name[64] = {0};
    sscanf(cmd+4,"%s",file_name);
    int fd=open(file_name,O_WRONLY|O_CREAT,0666);
    ERROR_CHECK(fd,-1,"open");
    //接文件内容
    while(1)
    {
        recvn(new_fd,&data_len,4);
        if(data_len>0)
        {
            recvn(new_fd,buf,data_len);
            write(fd,buf,data_len);
        }
        else 
        {
            break;
        }
    }
    return 0;
}
int recvn(int sfd,void* pstart,int len)
{
    int total=0,ret;
    char *p=(char*)pstart;
    while(total<len)
    {
        ret=recv(sfd,p+total,len-total,0);
        total+=ret;//每次接收到的字节数加到total上
    }
    return 0;
}
