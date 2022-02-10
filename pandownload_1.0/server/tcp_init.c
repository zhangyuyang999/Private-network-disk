#include "multi_process.h"

int tcp_init(char *ip, char *port,int *psfd)
{
    int sfd = socket(AF_INET,SOCK_STREAM,0);
    ERROR_CHECK(sfd,-1,"socket");
    
    struct sockaddr_in ser_addr;
    bzero(&ser_addr,sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(atoi(port));
    ser_addr.sin_addr.s_addr = inet_addr(ip);

    int reuse = 1;
    int ret = setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int));
    ERROR_CHECK(ret,-1,"setsockopt");

    ret = bind(sfd,(struct sockaddr*)&ser_addr,sizeof(ser_addr));
    ERROR_CHECK(ret,-1,"bind");

    listen(sfd,128);
    *psfd = sfd;
    return 0;
}
