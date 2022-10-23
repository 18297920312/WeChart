#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <strings.h>
int fd;
#define PRINT_ERR(msg) \
    do                 \
    {                  \
        perror(msg);   \
        return -1;     \
    } while (0)
typedef struct client
{
    struct sockaddr_in cli_data;
    char name[20];
    struct client *next;
} * cli_node, client;

cli_node data_init() // 数据初始化
{
    cli_node L = (cli_node)malloc(sizeof(client));
    if (NULL == L)
        return NULL;
    L->next = NULL;
    return L;
}
int login(int sfd, cli_node L, struct sockaddr_in cli_addr, char *buff);
int chart(int sfd, cli_node L, struct sockaddr_in cli_addr, char *buff);
int unlogin(cli_node L, struct sockaddr_in cli_addr, char *buff);

// 绑定 sockaddr
int bind_sockaddr(int sfd, struct sockaddr_in *addr, char *ip, int port)
{
    if (NULL == addr)
    {
        printf("get_sockaddr error\n");
        return -1;
    }
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = inet_addr(ip);
    if (bind(sfd, (struct sockaddr *)addr, sizeof(*addr)) < 0)
        PRINT_ERR("bind");
    return 0;
}

int node_insert(cli_node L, struct sockaddr_in cli_addr, char *name) // 数据链表插入
{
    cli_node p = (cli_node)malloc(sizeof(client));
    if (NULL == L || NULL == p)
    {
        printf("insert error\n");
        return -1;
    }
    p->cli_data = cli_addr;
    strcpy(p->name, name);
    // 插入
    p->next = L->next;
    L->next = p;
    return 0;
}

void *task(void *arg)
{
    cli_node L = (cli_node)arg;
    cli_node p = L;
    if (NULL == L)
    {
        printf("发送线程发生错误\n");
        pthread_exit(NULL);
    }
    printf("多线程已开启\n");
    //  键盘输入数据
    char name[] = "服务器消息:";
    char data[50] = "";
    char buff[100] = "";
    // 循环发送数据
    while (1)
    {
        p = L;
        // bzero(buff,sizeof(buff));
        scanf("%s", data);
        getchar();
        sprintf(buff,"%s%s",name,data);
        while (p = p->next)
        {
            if (sendto(fd, buff, sizeof(buff), 0, (struct sockaddr *)&(p->cli_data), sizeof(p->cli_data)) < 0)
            {
                perror("多线程发送失败");
                pthread_exit(NULL);
            }
        }
    }
    printf("发送完毕\n");
    pthread_exit(NULL);
}

int main(int argc, const char *argv[])
{
    cli_node L = data_init(); // 数据初始化
    if (NULL == L)
    {
        printf("data init error\n");
        return -1;
    }

    // socket初始化
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        PRINT_ERR("socket");

    // 绑定 IP 端口号
    struct sockaddr_in host_addr;
    bind_sockaddr(fd, &host_addr, "192.168.126.140", 8000);

    pthread_t tid;
    if (pthread_create(&tid, NULL, task, L) < 0)
        PRINT_ERR("多线程开启失败\n");

    // 接受客户的数据初始化
    struct sockaddr_in cli_addr;
    int cli_len = sizeof(cli_addr);
    char buff[100] = ""; // 接受的数据
    int recv_size = 0;
    while (1)
    {
        recv_size = recvfrom(fd, buff, 100, 0, (struct sockaddr *)&cli_addr, &cli_len);
        if (-1 == recv_size)
            PRINT_ERR("recv");
        printf("%s\n", buff+1);
        if (0 == buff[0])
        {   
            printf("%s已登录\n",buff+1);
            login(fd, L, cli_addr, buff + 1);
        }
        else if (1 == buff[0])
        {
            chart(fd, L, cli_addr, buff + 1);
        }
        else if (2 == buff[0])
        {
            unlogin(L, cli_addr, buff + 1);
        }
        else
        {
            printf("数据错误\n");
            break;
        }
    }
    close(fd);
    return 0;
}

int login(int sfd, cli_node L, struct sockaddr_in cli_addr, char *buff)
{
    if (NULL == L || NULL == buff)
    {
        printf("login error\n");
        return -1;
    }
    char name[20] = "";
    strcpy(name,buff);
    sprintf(buff, "----大家欢迎新同学%s上线----\n", name);
    cli_node p = L;
    while (p = p->next) // 循环发送
    {
        if (-1 == sendto(sfd, buff, strlen(buff) + 1, 0, (struct sockaddr *)&(p->cli_data), sizeof(p->cli_data)))
            PRINT_ERR("sendto 上线"); // 发送上线信息
    }
    node_insert(L, cli_addr, buff);
    return 0;
}

int chart(int sfd, cli_node L, struct sockaddr_in cli_addr, char *buff)
{
    if (NULL == L || NULL == buff)
    {
        printf("chart error");
        return -1;
    }
    while (L = L->next)
    {
        if (!memcmp(&cli_addr, &(L->cli_data), sizeof(cli_addr)))
            continue;
        if (-1 == sendto(sfd, buff, strlen(buff) + 1, 0, (struct sockaddr *)&(L->cli_data), sizeof(L->cli_data)))
            PRINT_ERR("sendto 上线"); // 循环发送消息
    }
    return 0;
}

int unlogin(cli_node L, struct sockaddr_in cli_addr, char *buff)
{
    if (NULL == L || NULL == buff)
    {
        printf("unlogin error\n");
        return -1;
    }
    cli_node p = L;
    while (p = p->next)
    {
        if (!memcmp(&cli_addr, &(p->cli_data), sizeof(cli_addr)))
        {
            L->next = L->next->next;
            free(p);
            break;
        }
        L = p;
    }
    return 0;
}