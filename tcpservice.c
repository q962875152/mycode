#include <sys/socket.h>
#include <stdio.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

#define BUFFLENGTH 1024

int main(int argc, char **argv) {
    int listenfd, connfd;
    struct sockaddr_in servaddr, cliendaddr;
    char buff[BUFFLENGTH];

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        printf("The socket has failed, error %d is %s",errno, strerror(errno));
    }

    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons((uint16_t)9999);
    servaddr.sin_family = AF_INET;
    if(bind(listenfd, ((struct sockaddr *)&servaddr), sizeof(struct sockaddr))) {
        printf("The bind has failed, error %d is %s", errno, strerror(errno));
    }

    listen(listenfd, 10);
    socklen_t socklen = sizeof(struct sockaddr);
    connfd = accept(listenfd, (struct sockaddr *)&cliendaddr, &socklen);
    if (connfd < 0) {
        printf("The accept has failed, error %d is %s", errno, strerror(errno));
    } 
    else {
        printf("TCP has connected!\n");
    }

    while (1) {
        int length = recv(connfd, buff, BUFFLENGTH, 0);//返回实际读取到的字节数量；3参数是二参数的长度；
        if (length > 0) {
            buff[length] = '\0';
            printf("%s\n", buff);
            send(connfd, buff, length, 0);
        } else if (length == 0){
            close(connfd);
            break;
        }
    }

    return 0;
}

/*
socket函数：第一个参数代表协议族；第二个参数代表套接字类型；第三个参数代表协议类型常值，如果为0，则选择第一、二个参数所组合的系统默认常量。它返回一个非0整数来代表生成的套接字描述符，
            或者返回-1表示此函数调用失败。AF_INET与SOCK_STREAM的组合代表TCP。
bind函数：它把本地协议地址赋给了一个套接字。第二个参数是指向特定协议的地址结构的指针。ip地址是通配地址，代表监听操作系统里所有的IP地址；端口为0，代表由内核随机分配一个端口。
            sockaddr和sockaddr_in的大小相等，可以互相转化。前者是通用套接字地址，后者是internet环境下套接字的地址。
listen函数：fd被创建时，默认是主动套接字。这个函数将fd转换为被动套接字。第二个参数规定了内核应该为相应套接字排队的最大队列（？？模糊？？）。
accept函数：用于从已连接完成队列中返回第一个连接。参数二是已连接的对端进程的协议地址。第三个参数是已连接的对端进程的协议地址的长度。
recv函数：第三个参数是第二个指针指向的缓冲区的大小。此函数会将内核已经准备好的缓冲区的数据copy到用户缓冲区中，如果内核中的数据大于用户缓冲区的长度，则需要调用几次recv函数才能读取完成。如果网络连接中断，
            此函数返回0。
send函数：第三个参数指明要发送的实际的字节数。如果函数要发送的字节数大于套接字的缓冲区，则返回SOCKET_ERROR;如果套接字缓冲区没有开始发送数据且剩余长度大于要发送的内容，则将用户缓冲区的内容copy到
            套接字缓冲区。
*/