#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <cygwin/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <cerrno>
#define MAX_BUF_SIZE 1024
int main() {
    printf("client start...");
    sleep(1);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8823);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        printf("client connect error %s\n", strerror(errno));
        return 0;
    }
    char buf[MAX_BUF_SIZE];
    char recv_buf[MAX_BUF_SIZE];
    int recv_size = 0;
    while (true) {
        memset(buf, 0, sizeof(buf));
        memset(recv_buf, 0, sizeof(recv_buf));
//        strcpy(buf, "hello tink!");
        scanf("%s", buf);
        if (send(fd, buf, strlen(buf), 0) == -1) {
            printf("client send msg error %s\n", strerror(errno));
            return 0;
        }
        if ((recv_size = recv(fd, recv_buf, MAX_BUF_SIZE, 0)) == -1) {
            printf("client recv msg error %s\n", strerror(errno));
            return 0;
        } else if ((recv_size >0)) {
            recv_buf[recv_size] = '\0';
            printf("client recv msg:%s\n", recv_buf);
        }


    }
    close(fd);
    getchar();
    return 0;
}