#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define BUFFLENTH 1024
#define POLL_SIZE 1024

int main(int argc, char **argv) {
    int listenfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in clientaddr;
    char buffer[BUFFLENTH];
    
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("The socket has failed, error %d is %s",errno,strerror(errno));
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(9999);
    if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_in)) == -1) {
        printf("The bind has failed, error %d is %s",errno, strerror(errno));
    }

    listen(listenfd, 10);

    int epfd = epoll_create(1);

    struct epoll_event ev, events[POLL_SIZE];
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;

    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);

    while(1) {
        int nready = epoll_wait(epfd, events, POLL_SIZE, 5);
        if (nready == -1) {
            printf("The epoll_wait has failed, error %d is %s",errno, strerror(errno));
            continue;
        }

        for (int i = 0; i < nready; i++) {
            int clientfd = events[i].data.fd;

            if (clientfd == listenfd) {
                int length = sizeof(struct sockaddr);
                int connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &length);
                ev.events = EPOLLIN;
                ev.data.fd = connfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
                printf("TCP has connected!\n");
            } else if (events[i].events & EPOLLIN) {
                int n = recv(clientfd, buffer, BUFFLENTH, 0);
                printf("n = %d\n", n);
                if (n > 0) {
                    buffer[n] = '\0';
                    printf("received data is '%s\n'", buffer);
                    send(clientfd, buffer, BUFFLENTH, 0);
                } else if (n == 0) {
                    ev.events = EPOLLIN;
                    ev.data.fd = clientfd;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, &ev);
                    close(clientfd);
                    printf("the tcp is closed!\n");
                }
            }
        }

    }


    return 0;
}

/*
epoll_event.data的用处
close关闭一个文件描述符，使他不指向任何文件
socket的缓冲区如何处理
用户的缓冲区如何处理
printf()函数为何没有被打印？只有断开连接时才打印;与\n的关系
*/