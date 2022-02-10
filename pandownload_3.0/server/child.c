
#include "multi_process.h"

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
        else {
            close(fds[1]);
            parr[i].pid = pid;
            parr[i].fd = fds[0];
            parr[i].flag = 0;
        }
    }      
    return 0;
}
//子进程处理函数：用来完成客户端发送过来的任务

char username[16] = {0};

int child_handle(int pipefd)
{
    int new_fd;
    int cmd_len = 0;//
    char cmd[1024] = {0};//
    int recv_ret = 0;//
    int ret = 0;
    char password[128] = {0};
    char salt[128] = {0};
    char msg[1] = {0};//message buf to communicate with client
    char query[1024] = {0};//mysql statement buf
    char retval[1024] = {0};//mysql return value buf

    while(1)
    {
        recv_fd(pipefd,&new_fd);
        printf("receive task:new_fd = %d,mission start!\n",new_fd);


        ret = recv(new_fd,msg,1,0);

        if (msg[0] == '1') {
            printf("begain register\n");

            ret = recv(new_fd,username,sizeof(username),0);//<register:1>

            sprintf(query, "insert into user(username) values('%s')", username);
            insert(query);

            generate_salt(salt);
            sprintf(query,"%s%s%s%s%s","update user set salt = '",salt,"' where username = '",username,"'");
            update(query);
            send(new_fd,salt,strlen(salt),0);//<register:2>

            recv(new_fd,password,sizeof(password),0);//<register:3>

            sprintf(query,"%s%s%s%s%s","update user set password= '",password,"' where username = '",username,"'");
            update(query);

            //create a home directory
            sprintf(query,"%s%s%s", "insert into VirtualFileTable(parent_id,filename,filetype,owner) values(0,'home','d','", username, "')");
            insert(query);

            sprintf(query,"%s%s%s%s%s","update user set pwd = ","'/home'"," where username = '",username,"'");
            update(query);
        }

        while(1)
        {
            bzero(username,sizeof(username));
            ret = recv(new_fd,username,sizeof(username),0);//<login:1>

            bzero(query,sizeof(query));
            bzero(retval,sizeof(retval));
            sprintf(query,"%s%s%s","select salt,password from user where username = '",username,"'");
            ret =Query(query,retval);

            if(-1 == ret) {
                memcpy(msg,"-1",1);
                send(new_fd,msg,1,0);//<login:2>
                continue;
            }
            else {
                send(new_fd,retval,11,0);//<login:2>
            }

            recv(new_fd,password,sizeof(password),0);//<login:3>

            if (strncmp(retval+12,password,strlen(password)) == 0) {
                send(new_fd,"1",1,0);//<login:4>send 1 on behalf of succuss
                break;
            } else {
                send(new_fd,"0",1,0);//<login:4>send 0 on behalf of failure
            }
        }

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
            else if(strncmp("pwd", cmd, 3) == 0) 
            {
                do_pwd(new_fd,username);
            }
            else if (strncmp("mkdir", cmd, 5) == 0 )
            {
                do_mkdir(new_fd,cmd);
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
    char query[1024] = {0};
    char retval[1024] = {0};
    int file_id;
    int i,len;
    char cur_dir[16] = {0};
    char dir_name[16] = {0};
    char pwd[128] = {0};
    int ret;

    //get the target directory name from command
    sscanf(cmd+3,"%s",dir_name);

    //fetch "pwd" from Vitual File Table
    sprintf(query,"%s%s%s","select pwd from user where username = '",username,"'");
    Query(query,pwd);

    if(strcmp(dir_name,"..") == 0) {//the target directory is superior directory

        if(strcmp(pwd,"/home") == 0) {//reach the user's home directory
            memcpy(retval,"reach home directory~",21);
            send(new_fd,retval,strlen(retval),0);//<cd:case1>

        } else {//didn't reach the user's home directory
            len = strlen(pwd);
            for(i=len-1 ;pwd[i]!='/';i--);
            //strncpy(pwd,pwd,i);//check out what will happen:strncpy will not truncate the remaining data
            //memcpy(pwd,pwd,i);//memcpy also doesn't work
            bzero(pwd+i,sizeof(pwd)-i);

            sprintf(query,"%s%s%s%s'","update user set pwd = '",pwd,"' where username = '",username);
            update(query);
            send(new_fd,pwd,strlen(pwd),0);//<cd:case2>
        }

    } else {
        //get current working directory from pwd
        len = strlen(pwd);
        for(i=len-1 ;pwd[i]!='/';i--);
        memcpy(cur_dir,pwd+i+1,len-i+1);

        //fetch cwd's id from Virtual File Table which is a string in retval and transfer it to an integer
        bzero(retval,sizeof(retval));
        sprintf(query,"%s%s%s%s'","select id from VirtualFileTable where owner = '",username,"' and filename = '",cur_dir);
        Query(query,retval);
        file_id = atoi(retval);

        //Query whether the target file exists
        bzero(retval,sizeof(retval));
        sprintf(query,"select id from VirtualFileTable where owner = '%s' and filename = '%s' and parent_id = %d and filetype = 'd'",username,dir_name,file_id);
        printf("%s\n",query);
        ret = Query(query,retval);

        if(-1 == ret) {//the directory doesn's exist
            memcpy(retval,"no such directory",17);
            send(new_fd,retval,strlen(retval),0);//<cd:case3>

        } else {//the directory exist
            sprintf(pwd,"%s/%s",pwd,dir_name);

            sprintf(query,"%s%s%s%s'","update user set pwd = '",pwd,"' where username = '",username);
            update(query);
            send(new_fd,pwd,strlen(pwd),0);//<cd:case4>
        }
    }
    return 0;
}
int do_ls(int new_fd) 
{
    char query[1024] = {0};
    char retval[1024] = {0};
    int file_id;
    int i,len;
    char cur_dir[16] = {0};
    char pwd[128] = {0};
    int ret;

    //fetch "pwd" from Vitual File Table
    sprintf(query,"%s%s%s","select pwd from user where username = '",username,"'");
    Query(query,pwd);

    //get current working directory from pwd
    len = strlen(pwd);
    for(i=len-1 ;pwd[i]!='/';i--);
    memcpy(cur_dir,pwd+i+1,len-i+1);

    //fetch cwd's id from Virtual File Table which is a string in retval and transfer it to an integer
    bzero(retval,sizeof(retval));
    sprintf(query,"%s%s%s%s'","select id from VirtualFileTable where owner = '",username,"' and filename = '",cur_dir);
    Query(query,retval);
    file_id = atoi(retval);

    //query Virtual File Table according to file_id(parent_id) and username
    bzero(retval,sizeof(retval));
    sprintf(query,"select filename, filetype, owner,filesize from VirtualFileTable where owner = '%s' and parent_id = %d",username,file_id);
    ret = Query(query,retval);

    if(-1 == ret) {
        memcpy(retval,"the directory is empty",22);
    }
    send(new_fd,retval,strlen(retval),0);
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

int do_pwd(int new_fd,char *username)
{
    char query[1024] = {0};
    char retval[1024] = {0};

    sprintf(query,"%s%s%s","select pwd from user where username = '",username,"'");
    Query(query,retval);

    send(new_fd,retval,strlen(retval),0);

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
    off_t file_size = 0;//the downloaded size of the file
    int ret = 0;
    char query[1024] = {0};
    char retval[1024] = {0};
    int file_id;
    int i,len;
    char cur_dir[16] = {0};
    char pwd[128] = {0};
    char md5[128] = {0};
    char msg[1] = {0};

    chdir("filepool");

    //fetch "pwd" from Vitual File Table
    sprintf(query,"%s%s%s","select pwd from user where username = '",username,"'");
    Query(query,pwd);

    //get current working directory from pwd
    len = strlen(pwd);
    for(i=len-1 ;pwd[i]!='/';i--);
    memcpy(cur_dir,pwd+i+1,len-i+1);

    //fetch cwd's id from Virtual File Table which is a string in retval and transfer it to an integer
    bzero(retval,sizeof(retval));
    sprintf(query,"%s%s%s%s'","select id from VirtualFileTable where owner = '",username,"' and filename = '",cur_dir);
    Query(query,retval);
    file_id = atoi(retval);

    sscanf(cmd+4,"%s",file_name);

    sprintf(query,"select md5sum from VirtualFileTable where owner = '%s' and parent_id = %d and filename = '%s'",username,file_id,file_name);
    ret = Query(query,md5);

    if(-1 == ret) {
        msg[0] = '0';
        ret = send(new_fd,msg,1,0);//<gets:1:case1>:命令不合法（如文件错误）,回执0
        return 0;

    } else {
        msg[0] = '1';
        ret = send(new_fd,msg,1,0);//<gets:1:case2>:命令合法回执1

        int fd = open(md5,O_RDONLY);

        //receive the qualification command to testify wheather there is a identical file
        recv(new_fd,&file_size,8,0);//<gets:2>
        if(0 != file_size)
        {
            lseek(fd,+file_size,SEEK_SET);
        }

        //send the content of the file
        while((t.data_len = read(fd,t.buf,sizeof(t.buf))))//once reaching the end of the file ,t.data_len will be set zero
        {
            ret = send(new_fd,&t,t.data_len+4,0);
            ERROR_CHECK(ret,-1,"send");
        }
        //send a zero on be half of the end of the file
        t.data_len = 0;
        ret = send(new_fd,&t,4,0);
        ERROR_CHECK(ret,-1,"send");
    }
    return 0;
}
int do_puts(int new_fd,char *cmd)
{
    int data_len;
    char buf[1024]={0};
    char file_name[64] = {0};
    long file_len;
    char md5sum[64] = {0};
    char retval[1024] = {0};
    char query[1024] = {0};
    int ret = 0;
    struct stat statbuf;
    int i, len, file_id;
    char cur_dir[16] = {0};
    char pwd[1024] = {0};
    char msg[1] = {0};

    chdir("filepool");

    sscanf(cmd+4,"%s",file_name);

    recv(new_fd,&data_len,4,0);//<puts:1>接收命令回执消息，如果命令合法，受到1，否则受到0
    if(data_len == 0)//命令不合法，接收到客户端发过来的0，直接返回
    {
        return 0;
    }
    recv(new_fd,md5sum,sizeof(md5sum),0);//<puts:2>

    //fetch "pwd" from Vitual File Table
    sprintf(query,"%s%s%s","select pwd from user where username = '",username,"'");
    Query(query,pwd);

    //get current working directory from pwd
    len = strlen(pwd);
    for(i=len-1 ;pwd[i]!='/';i--);
    memcpy(cur_dir,pwd+i+1,len-i+1);

    //fetch cwd's id from Virtual File Table which is a string in retval and transfer it to an integer
    bzero(retval,sizeof(retval));
    sprintf(query,"%s%s%s%s'","select id from VirtualFileTable where owner = '",username,"' and filename = '",cur_dir);
    Query(query,retval);
    file_id = atoi(retval);

    bzero(retval,sizeof(retval));
    sprintf(query,"select owner from VirtualFileTable where md5sum = '%s'",md5sum);
    ret =Query(query,retval);

    if(-1 == ret) {//the file doesn't exist in the file pool
        memcpy(msg,"1",1);
        send(new_fd,msg,1,0);//<puts:3:case1>
        int fd=open(md5sum,O_WRONLY|O_CREAT,0666);
        ERROR_CHECK(fd,-1,"open");

        //receive the content of the file
        while(1) 
        {
            recvn(new_fd,&data_len,4);
            if (data_len>0) {
                recvn(new_fd,buf,data_len);
                write(fd,buf,data_len);
            } else {
                break;
            }
        }

        //alter the Virtual file table
        fstat(fd,&statbuf);

        sprintf(query,"insert into VirtualFileTable(parent_id,filetype,filename,owner,filesize,md5sum) values(%d, 'f', '%s', '%s',%ld,'%s')", file_id, file_name, username, statbuf.st_size,md5sum);
        insert(query);

    } else {//the file is already exist in the file pool

        bzero(retval,sizeof(retval));
        sprintf(query,"select owner from VirtualFileTable where md5sum = '%s' and parent_id = %d",md5sum,file_id);
        ret =Query(query,retval);

        if(strcmp(retval,username) != 0) {
            memcpy(retval,"2",1);
            send(new_fd,retval,1,0);//<puts:3:case2>

            bzero(retval,sizeof(retval));
            sprintf(query,"select filesize from VirtualFileTable where md5sum = '%s'", md5sum);
            Query(query,retval);
            file_len = atoi(retval);

            sprintf(query,"insert into VirtualFileTable(parent_id,filetype,filename,owner,filesize,md5sum) values(%d, 'f', '%s', '%s',%ld,'%s')", file_id, file_name, username, file_len,md5sum);
            insert(query);

        } else {//the user has already uploaded the file
            memcpy(retval,"3",1);
            send(new_fd,retval,1,0);//<puts:3:case3>
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
void get_salt(char *salt,char *passwd) 
{ 
    int i,j; 
    for(i=0,j=0;passwd[i] && j != 3;++i) 
    { 
        if(passwd[i] == '$') 
            ++j; 
    }
    strncpy(salt,passwd,i-1); 
}

int generate_salt(char *salt) {
    char str[8] = {0}; 
    int i,flag;
    srand(time(NULL));//通过时间函数设置随机数种子，使得每次运行结果随机。
    for(i = 0; i < 8; i ++) {
        flag = rand()%3;
        switch(flag) {
        case 0:
            str[i] = rand()%26 + 'a';
            break;
        case 1: 
            str[i] = rand()%26 + 'A'; 
            break;
        case 2:
            str[i] = rand()%10 + '0';
            break;
        } 
    }
    sprintf(salt,"%s%s","$6$",str);
    return 0;
}

int do_mkdir(int new_fd,char *cmd)
{
    char query[1024] = {0};

    char retval[1024] = {0};
    int file_id;
    int i,len;
    char cur_dir[16] = {0};
    char dir_name[16] = {0};

    //get the directory name from command
    sscanf(cmd+6,"%s",dir_name);

    //fetch "pwd" from Vitual File Table
    sprintf(query,"%s%s%s","select pwd from user where username = '",username,"'");
    Query(query,retval);

    //get current working directory from pwd
    len = strlen(retval);
    for(i=len-1 ;retval[i]!='/';i--);
    memcpy(cur_dir,retval+i+1,len-i+1);

    //fetch cwd's id from Virtual File Table which is a string in retval and transfer it to an integer
    bzero(retval,sizeof(retval));
    sprintf(query,"%s%s%s%s'","select id from VirtualFileTable where owner = '",username,"' and filename = '",cur_dir);
    Query(query,retval);
    file_id = atoi(retval);

    //insert a new directory information into Virtual File Table
    sprintf(query,"%s%d, '%s', 'd','%s')", "insert into VirtualFileTable(parent_id,filename,filetype,owner) values(", file_id, dir_name,username);
    insert(query);
    memcpy(retval,"mkdir success~",13);
    send(new_fd,retval,strlen(retval),0);

    return 0;

}
