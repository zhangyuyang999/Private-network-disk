#include"multi_process.h"

int trans_file(int new_fd)
{
    int ret;
    train_t t;
    //send the name of the file 
    t.data_len = strlen(FILENAME);
    strcpy(t.buf,FILENAME);
    ret = send(new_fd,&t,4+t.data_len,MSG_NOSIGNAL);
    ERROR_CHECK(ret,-1,"send");

    //open the file 
    int fd = open(FILENAME,O_RDONLY|O_CREAT,0600);
    ERROR_CHECK(fd,-1,"open");
    
    //send the size of the file 
    struct stat buf;
    ret = fstat(fd,&buf);
    ERROR_CHECK(ret,-1,"fstat");
    t.data_len = sizeof(buf.st_size);
    memcpy(t.buf,&buf.st_size,t.data_len);
    ret = send(new_fd,&t,4+t.data_len,MSG_NOSIGNAL);
    ERROR_CHECK(ret,-1,"send");

    //send the content of the file
    ret = sendfile(new_fd,fd,NULL,buf.st_size);
    ERROR_CHECK(ret,-1,"sendfile");
    return 0;
}
