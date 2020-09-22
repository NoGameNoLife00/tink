#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
//#include <cygwin/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <cerrno>
#include <datapack.h>
#include <message.h>
#include <common.h>


#define MAX_BUF_SIZE 1024
int main() {
    printf("client Start...\n");
    setbuf(stdout, NULL); // debug
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
    BytePtr out = std::make_unique<byte[]>(MAX_BUF_SIZE);
    uint32_t out_len;
    int recv_size = 0;
    tink::DataPack dp;
    while (true) {
        BytePtr str_buf(new char[MAX_BUF_SIZE]);
        scanf("%s", str_buf.get());
//        strcpy(str_buf.get(), "hello world");
        // 发送msg给客户端
        tink::NetMessage msg;
        msg.Init(0,strlen(str_buf.get())+1, str_buf);

        dp.Pack(msg, out, out_len);
        if (send(fd, out.get(), out_len, 0) == -1) {
            printf("client Send msg error %s\n", strerror(errno));
            break;
        }

        BytePtr msg_head = std::make_unique<byte[]>(dp.GetHeadLen());
        tink::NetMessage recv_msg;

        if ((recv(fd, msg_head.get(), dp.GetHeadLen(), 0)) == -1) {
            printf("client recv msg head error %s\n", strerror(errno));
            return 0;
        }
        dp.Unpack(msg_head, recv_msg);
        if (recv_msg.GetDataLen() > 0) {
            BytePtr data(new char[recv_msg.GetDataLen()]);
            if ((recv(fd, data.get(), recv_msg.GetDataLen(), 0)) == -1) {
                printf("client recv msg data error %s\n", strerror(errno));
                return 0;
            }
            recv_msg.SetData(data);

            printf("client recv msg: id=%d, len=%d, data=%s",recv_msg.GetId(),
                    recv_msg.GetDataLen(), recv_msg.GetData().get());
        }

    }
    close(fd);
    getchar();
    return 0;
}