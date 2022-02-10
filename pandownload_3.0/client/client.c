//TODO:keep the server nomal once client quit abruptly
#include "multi_process.h"
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
    char salt[16] = {0};
    char *encryptedpd = NULL;
    int read_len;
    int send_len;
    char cmd[128];
    char recv_buf[1024];

    printf("choice log in(0) or register(1):");
    scanf("%s",msg);
    send(sfd,msg,1,0);
    if (msg[0] == '1') {
        printf("begain register\n");
        printf("please input username:");
        scanf("%s",username);//before scanf,system will flush stdout antomatically
        send(sfd,username,strlen(username),0);//<register:1>

        recv(sfd,salt,sizeof(salt),0);//<register:2>

        password=getpass("please input password:");
        encryptedpd = crypt(password,salt);
        send(sfd,encryptedpd,strlen(encryptedpd),0);//<register:3>
        printf("register success!\n");
    }
    

    while(1)
    {
        //printf("please input username:");//why this sentence didn't printf
        printf("please input username:");
        scanf("%s",username);//before scanf,system will flush stdout antomatically
        send(sfd,username,strlen(username),0);//<login:1>
        
        recv(sfd,salt,sizeof(salt),0);//<login:2>
        if (strncmp(salt,"-1",1) == 0) {
            printf("incorrect username,please try again\n");
            continue;
        }

        password=getpass("please input password:");
        encryptedpd = crypt(password,salt);
        
        send(sfd,encryptedpd,strlen(encryptedpd),0);//<login:3>

        recv(sfd,msg,1,0);//<login:4>
        if(msg[0] == '1')
        {
            printf("login success!\n");
            break;
        }
        printf("incorrect password,please try again!\n");
    }


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
            recv(sfd,recv_buf,sizeof(recv_buf),0);
            printf("%s\n", recv_buf);
        }
        else if(strncmp("ls", cmd, 2) == 0)
        {
            system("clear");
            recv(sfd, recv_buf, sizeof(recv_buf), 0);
            printf("%s\n",recv_buf);
        }
        else if (strncmp("mkdir", cmd, 5) == 0)
        {
            recv(sfd,recv_buf,sizeof(recv_buf),0);
            printf("%s\n",recv_buf);
        }
        else if(strncmp("puts", cmd, 4) == 0)
        {
            train_t t;
            char file_name[64] = {0};
            char md5sum[64] = {0};

            sscanf(cmd+4,"%s",file_name);
            int fd = open(file_name,O_RDONLY);
            if (fd == -1) {
                printf("invalid command!type 'help' for help\n");
                t.data_len = 0;
                send(sfd,&t,4,0);//<puts:1:case1>命令不合法，给服务器发送0
                continue;

            } else {
                t.data_len = 1;
                send(sfd,&t,4,0);//<puts:1:case2>命令合法,给服务器发送1

                //caculate md5sum of the file
                cal_md5(file_name,md5sum);
                
                //send the md5sum of the file
                send(sfd,md5sum,strlen(md5sum),0);//<puts:2>

                recv(sfd,msg,1,0);//<puts:3>
                
                if (msg[0] == '1') {
                    //send the content of the file
                    while((t.data_len = read(fd,t.buf,sizeof(t.buf))))//once reaching the end of the file ,t.data_len will be set zero
                    {
                        send(sfd,&t,t.data_len+4,0);
                    }

                    //send a zero on be half of the end of the file
                    t.data_len = 0;
                    send(sfd,&t,4,0);
                    printf("upload success!\n");

                } else if (msg[0] == '2') {
                    printf("秒传成功！\n");

                } else {
                    printf("the file has already been uploaded,do not repeat operation\n");
                }
            }
        }
        else if( strncmp("gets", cmd, 4)== 0 )
        {
            int data_len;
            char buf[1024]={0};
            char file_name[64] = {0};
            off_t file_size = 0;//the size that have been downloaed
            int fd;
            char msg[1] = {0};


            recv(sfd,msg,1,0);//<gets:1> 接收命令回执消息，如果命令合法，收到1，否则收到0
            if(msg[0] == '0')
            {
                printf("no such file!\n");
                continue;
            }

            sscanf(cmd+4,"%s",file_name);

            fd = open(file_name,O_RDWR|O_APPEND);//try to open the file,if the file doesn't exist,this function will return -1,
            if(fd == -1)//file does not exist
            {
                file_size= 0;
                send(sfd,&file_size,8,0);//<puts:2:case1>

                fd=open(file_name,O_WRONLY|O_CREAT,0666);//create a new file
                ERROR_CHECK(fd,-1,"open");
            }
            else//the file exist
            {
                struct stat statbuf;
                stat(file_name,&statbuf);//get the size of the file
                file_size= statbuf.st_size;
                send(sfd,&file_size,8,0);//<puts:2:case1>
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
        else if( strncmp("help", cmd, 4) == 0 )
        {
            printf("-----------------------------------------------------------------\n");
            printf("1.cd dir/..   enter specified directory or superior directory\n");
            printf("2.ls          show files and directories on current directory\n");
            printf("5.pwd         print current working path\n");
            printf("3.puts file   upload a file ,not allow upload a directory\n");
            printf("4.gets file   download a file ,not allow download a directory\n");
            printf("6.mkdir dir   creat a new directory\n");
            printf("7.other command will not respond\n");
            printf("-----------------------------------------------------------------\n");
        }
        else if(strncmp("pwd", cmd, 3) == 0)
        {
            system("clear");
            bzero(recv_buf,sizeof(recv_buf));
            recv(sfd,recv_buf,sizeof(recv_buf),0);
            printf("%s\n", recv_buf);
        }
        else 
        {
            printf("can't find the command!\n");
            printf("please type 'help' for help\n");
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
