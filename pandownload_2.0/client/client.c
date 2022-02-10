//为什么wh
#include "multi_process.h"
//循环接收
int main(int argc,char *argv[])
{
    ARGS_CHECK(argc,3);
    //初始化一个socket描述符，用于tcp通信
    int sfd=socket(AF_INET,SOCK_STREAM,0);
    ERROR_CHECK(sfd,-1,"socket");
    struct sockaddr_in ser_addr;
    bzero(&ser_addr,sizeof(ser_addr));
    ser_addr.sin_family=AF_INET;
    ser_addr.sin_port=htons(atoi(argv[2]));//端口转为网络字节序
    ser_addr.sin_addr.s_addr=inet_addr(argv[1]);
    int ret=connect(sfd,(struct sockaddr*)&ser_addr,sizeof(ser_addr));
    ERROR_CHECK(ret,-1,"connect");

    char username[16] = {0};
    char * password;
    char msg[1] = {0};
    while(1)
    {
        //printf("please input username:");//why this sentence didn't printf
        printf("please input username:");
        scanf("%s",username);//before scanf,system will flush stdout antomatically
        send(sfd,username,strlen(username),0);//

        password=getpass("please input password:");
        send(sfd,password,strlen(password),0);

        recv(sfd,msg,1,0);
        if(msg[0] == '1')
        {
            printf("login success!\n");
            break;
        }
        printf("incorrect username or password,please try again!\n");
    }

    int read_len;
    int send_len;
    int recv_len;
    char cmd[128];
    char recv_buf[1024];

    while(bzero(cmd, 128),(read_len = read(0, cmd, 128)) > 0 )
    {
        //trim_space(cmd);
        bzero(recv_buf,1024);
        send_len = strlen(cmd);
        send(sfd, &send_len, sizeof(int), 0);//发送命令长度
        send(sfd,cmd,send_len,0);//发送命令
        if(strncmp("cd", cmd, 2) == 0)
        {
            system("clear");
            recv(sfd, &recv_len, sizeof(int), 0);//先接消息长度
            recv(sfd,recv_buf,recv_len,0);
            printf("%s\n", recv_buf);
        }
        else if(strncmp("ls", cmd, 2) == 0)
        {
            system("clear");
            while(1)
            {
                recv(sfd, &recv_len, sizeof(int), 0);
                if(recv_len == 0)//接收到零表明目录已读取完毕或者文件本身就是一个空文件
                {
                    break ;
                }
                recvn(sfd, recv_buf, recv_len);
                printf("%s\n", recv_buf);
            }
        }
        else if(strncmp("puts", cmd, 4) == 0)
        {
            train_t t;
            char file_name[64] = {0};
            sscanf(cmd+4,"%s",file_name);
            int fd = open(file_name,O_RDONLY);
            if(fd == -1)
            {
                printf("invalid command!\n");
                t.data_len = 0;
                send(sfd,&t,4,0);//命令不合法，给服务器发送0
                continue;
            }
            else
            {
                t.data_len = 1;//命令合法,给服务器发送1
                send(sfd,&t,4,0);

                //send the content of the file
                while((t.data_len = read(fd,t.buf,sizeof(t.buf))))//once reaching the end of the file ,t.data_len will be set zero
                {
                    send(sfd,&t,t.data_len+4,0);
                }
            }
            //send a zero on be half of the end of the file
            t.data_len = 0;
            send(sfd,&t,4,0);
            printf("upload success!\n");
        }
        else if( strncmp("gets", cmd, 4)== 0 )
        {
            int data_len;
            char buf[1024]={0};
            char file_name[64] = {0};
            off_t file_len = 0;//file's total size
            off_t file_size = 0;//the size that have been downloaed
            int fd;

            recv(sfd,&data_len,4,0);//接收命令回执消息，如果命令合法，收到1，否则收到0
            if(data_len == 0)
            {
                printf("invalid command!\n");
                continue;
            }

            //choice the different transmission methed from there
            recvn(sfd,&file_len,8);
            sscanf(cmd+4,"%s",file_name);

            if(file_len < 100*1024)//less than 100M
            {
                fd = open(file_name,O_RDWR|O_APPEND);//try to open the file,if the file doesn't exist,this function will return -1,
                if(fd == -1)//file does not exist
                {
                    file_size= 0;
                    send(sfd,&file_size,8,0);

                    fd=open(file_name,O_WRONLY|O_CREAT,0666);//create a new file
                    ERROR_CHECK(fd,-1,"open");
                }
                else//the file exist
                {
                    struct stat statbuf;
                    stat(file_name,&statbuf);//get the size of the file
                    file_size= statbuf.st_size;
                    send(sfd,&file_size,8,0);
                }
                //receive the content of the file
                while(1)
                {
                    recvn(sfd,&data_len,4);
                    if(data_len>0)
                    {
                        recvn(sfd,buf,data_len);
                        write(fd,buf,data_len);
                    }else{
                        printf("download success!\n");
                        break;
                    }
                }

            }
            else//more than 100M
            {
                fd = open(file_name,O_RDWR|O_APPEND);//try to open the file,if the file doesn't exist,this function will return -1,
                if(fd == -1)//file does not exist
                {
                    file_size= 0;
                    send(sfd,&file_size,8,0);

                    fd=open(file_name,O_RDWR|O_CREAT,0666);//create a new file
                    ERROR_CHECK(fd,-1,"open");
                    ftruncate(fd,file_len);
                    char * p =(char*)mmap(NULL,file_len,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
                    ERROR_CHECK(p,(char *)-1,"mmap");

                    ret = map_recv(sfd,p,file_len);
                    if(ret == -1)
                    {
                        printf("recv failed!\n");

                    }else{
                        printf("recv success!\n");
                    }
                    ret = munmap(p,file_len);
                    ERROR_CHECK(ret,-1,"munmap");
                }
                else
                {
                    int fd1 = open("log",O_RDWR);
                    read(fd1,&file_size,sizeof(file_size));
                    printf("%ld\n",file_size);
                    send(sfd,&file_size,8,0);

                    char * p =(char*)mmap(NULL,file_len-file_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
                    ERROR_CHECK(p,(char *)-1,"mmap");

                    ret = map_recv(sfd,p,file_len-file_size);
                    if (ret == -1) {
                        printf("recv failed!\n");
                    } else {
                        printf("recv success!\n");
                    }
                    ret = munmap(p,file_len-file_size);
                    msync(p,file_len-file_size,MS_SYNC);
                    ERROR_CHECK(ret,-1,"munmap");
                }
            }
        }
        else if( strncmp("remove", cmd, 6) == 0 )
        {
            system("clear");
            recv(sfd, &recv_len, sizeof(int), 0);
            recvn(sfd, recv_buf, recv_len );
            printf("%s\n", recv_buf);
        }
        else if(strncmp("pwd", cmd, 3) == 0)
        {
            system("clear");
            recv(sfd, &recv_len, sizeof(int), 0);
            recvn(sfd, recv_buf, recv_len );
            printf("%s\n", recv_buf);
        }
        else 
        {
            printf("can't find the command!\n");
            continue ;
        }
    }
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

int map_recv(int sfd,void* pstart,int len)
{
    int total=0,ret;
    char *p=(char*)pstart;
    int fd = open("log",O_RDWR);
    while(total<len)
    {
        ret=recv(sfd,p+total,len-total,0);
        total+=ret;//每次接收到的字节数加到total上
        write(fd,&total,sizeof(total));
        lseek(fd,0,SEEK_SET);
    }
    return 0;
}
