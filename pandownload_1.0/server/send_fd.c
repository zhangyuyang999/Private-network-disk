#include "multi_process.h"

int send_fd(int pipefd,int new_fd)//parent thread send client's new_fd to child
{
    struct msghdr msg;
    bzero(&msg,sizeof(msg));

    //initialize msg_iov and msg_iovlen member
    struct iovec iov[1];
    char buf[10] = "hello";
    iov[0].iov_base = buf;
    iov[0].iov_len = 5;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    
    //initialize msg_control and msg_controllen member
    struct cmsghdr * cmsg;
    int cmsg_len =  CMSG_LEN(sizeof(int));
    cmsg =(struct cmsghdr*) calloc(1,cmsg_len);
    cmsg->cmsg_len = cmsg_len;
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    *(int *)CMSG_DATA(cmsg) = new_fd;
    msg.msg_control = cmsg;
    msg.msg_controllen = cmsg_len;

    int ret = sendmsg(pipefd,&msg,0);//sending new_fd kernel information to child thread through pipefd;
    ERROR_CHECK(ret,-1,"sendmsg");
    return 0;
}

int recv_fd(int pipefd,int *fd)//receive client's new_fd from parent thread
{
    struct msghdr msg;
    bzero(&msg,sizeof(msg));//must be bzero 

    //initialize msg_iov and msg_iovlen member
    struct iovec iov[1];
    char buf[10] = "hello";
    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    
    //initialize msg_control and msg_controllen member
    struct cmsghdr * cmsg;
    int cmsg_len =  CMSG_LEN(sizeof(int));
    cmsg =(struct cmsghdr*) calloc(1,cmsg_len);
    cmsg->cmsg_len = cmsg_len;
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    msg.msg_control = cmsg;
    msg.msg_controllen = cmsg_len;

    int ret = recvmsg(pipefd,&msg,0);//sending new_fd kernel information to child thread through pipefd;
    ERROR_CHECK(ret,-1,"recvmsg");
    *fd = *(int *)CMSG_DATA(cmsg);
    return 0;
}
