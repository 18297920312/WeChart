#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#define PRINT_ERR(msg) \
    do                 \
    {                  \
        perror(msg);   \
        return -1;     \
    } while (0)

void *task(void *arg)
{
    int sfd = *(int *)arg;
    printf("多线程已开启\n");
    //  键盘输入数据
    char buff[100] = "";
    // 数据来源地址
    struct sockaddr_in ser_addr;
    int ser_len;
    int recv_size;
    // 循环发送数据
    while (1)
    {
        recv_size = recvfrom(sfd, buff, 100, 0, (struct sockaddr *)&ser_addr, &ser_len);
        if (-1 == recv_size)
        {
            printf("recv error\n");
            pthread_exit(NULL);
        }
        printf("%s\n", buff);
    }
    printf("多线程关闭\n");
    pthread_exit(NULL);
}

int main(int argc, const char *argv[])
{
    printf("////////// 欢迎登录 ///////////\n");
    printf("请输入登录姓名：");
    char buff[100] = {0};
    char name[20] = "";
    scanf("%s", name); // 输入姓名
    getchar();
    sprintf(buff, "%c%s", 0, name);

    int sfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sfd < 0)
        PRINT_ERR("socket");

    // 多线程接受
    pthread_t tid;
    if (pthread_create(&tid, NULL, task, &sfd) < 0)
        PRINT_ERR("多线程开启失败\n");

    // 绑定 IP 端口号
    struct sockaddr_in host_addr;
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(8000);
    host_addr.sin_addr.s_addr = inet_addr("192.168.126.140");

    // 发送登录
    if (sendto(sfd, buff, 100, 0, (struct sockaddr *)&host_addr, sizeof(host_addr)) < 0)
        PRINT_ERR("登录失败");
    printf("////////// 登录成功 ////////\n");
    char data[79] = "";
    while (1)
    {
        scanf("%s", data);
        getchar();
        sprintf(buff, "%c%s%c%s", 1, name, ':', data);
        if (!strcmp(data, "quit"))
        {
            printf("您已退出退出登录\n");
            buff[0] = 2;
            sendto(sfd, buff, 100, 0, (struct sockaddr *)&host_addr, sizeof(host_addr));
            break;
        }
        if (sendto(sfd, buff, 100, 0, (struct sockaddr *)&host_addr, sizeof(host_addr)) < 0)
            PRINT_ERR("发送失败");
    }
    close(sfd);
    return 0;
}