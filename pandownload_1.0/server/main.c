//main函数的主要作用为：监控是否有新用户连接,接收连接，并将新任务分配给子进程
//监控子进程是否完成任务,将完成任务的子进程，重新设为空闲状态
#include "multi_process.h"

int main(int argc, char *argv[])//input: ./main 192.168.29.128 8000 5
{
    ARGS_CHECK(argc,4);

    int proc_num = atoi(argv[3]);
    proc_data_t *parr = (proc_data_t *)calloc(proc_num,sizeof(proc_data_t));
    creat_child(parr,proc_num);
    
    int sfd;
    tcp_init(argv[1],argv[2],&sfd);

    int epfd;
    epfd = epoll_create(1);

    struct epoll_event evs[proc_num+1];

    epoll_add(epfd,sfd);
    for(int i=0;i<proc_num;i++)
    {
        epoll_add(epfd,parr[i].fd);
    }

    int epoll_ready_num;
    char finish_flag;
    int i,j;

    int cur_link = 0;//当前连接人数
    while(1)
    {
        epoll_ready_num = epoll_wait(epfd,evs,proc_num+1,-1);
        for(i=0;i<epoll_ready_num;i++)
        {
            if(evs[i].data.fd == sfd)
            {
                int new_fd = accept(sfd,NULL,NULL);
                for(j=0;j<proc_num;j++)
                {
                    if(0 == parr[j].flag)
                    {
                        send_fd(parr[j].fd,new_fd);
                        parr[j].flag = 1;
                        printf("the number of online person:%d\n",++cur_link);
                        break;
                    }
                }
            }
            for(j=0;j<proc_num;j++)
            {
                if(evs[i].data.fd == parr[j].fd)
                {
                    read(parr[j].fd,&finish_flag,1);
                    parr[j].flag = 0;
                    cur_link--;
                    printf("set %d is not busy\n",parr[j].pid);
                    break;
                }
            }
        }
    }
    return 0;
}
