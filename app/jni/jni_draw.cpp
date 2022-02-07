#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Memory.h"
#include "AOBScan.h"

#define APP_PACKAGE_NAME "com.popcap.pvz"

struct zombie_data
{
    int x;
    int y;
};

unsigned char pattern[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};

struct zombie_data zd[25];

void *checkApp(void *)
{
    while (true)
    {
        if (find_pid(APP_PACKAGE_NAME) == -1)
        {
            exit(0);
        }
        sleep(5);
        // printf("heart beat\n");
    }
}

int main(int argc, char *argv[])
{
    auto pid = find_pid(APP_PACKAGE_NAME); //获取pid
    if (pid == 0)
    {
        printf("Can't find pid\n");
        return -1;
    }
    else
    {
        printf("pid:%d\n", pid);
    }

    //创建线程检测app是否退出
    pthread_t tidp;
    if (pthread_create(&tidp, NULL, checkApp, NULL) == -1)
    {
        printf("pthread_create error");
    }

    int port = 53316;
    int udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket_fd == -1)
    {
        perror("socket failed!\n");
        return -1;
    }
    //设置目的IP地址
    struct sockaddr_in dest_addr = {0};
    dest_addr.sin_family = AF_INET;                     //使用IPv4协议
    dest_addr.sin_port = port;                          //设置接收方端口号
    dest_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //设置接收方IP

    PModuleListEntry ml = get_process_map(pid); //读取进程的模块信息,PModuleListEntry是一个链表
    PModuleListEntry ml_ = ml;

    while (true)
    {
        memset(zd,0,sizeof(zd));
        int i = 0;
        ml = ml_;
        while (ml->next_ptr != nullptr) //循环所有的模块
        {
            if (!ml->moduleName[0]) //判断是否有模块名,注意,这个例子是没有模块名的.
            {
                int size = ml->moduleSize;
                unsigned char *target = (unsigned char *)malloc(size);
                unsigned char *target_ = target;
                int r = ReadProcessMemory(pid, (void *)ml->baseAddress, target, size); //读取内存
                if (r == 0)
                {
                    goto lable; //unreadable
                }
                while (true) //循环搜索所有匹配到的特征地址
                {
                    int offset = AOBScan(target, size, pattern, sizeof(pattern));
                    if (offset != -1)
                    {
                        target += offset + sizeof(pattern);
                        size -= offset + sizeof(pattern);
                        unsigned long long offset_ = target - target_ - sizeof(pattern); //这里是拿到偏移
                        unsigned long long address = ml->baseAddress + offset_;          //搜索到的目标进程的内存地址
                        // printf("offset_:%llx\n", offset_);
                        // printf("%llx\n", address);
                        unsigned long long y = address + 0x4c;//僵尸的y坐标
                        unsigned long long x = address + 0x38;//僵尸的x坐标
                        // unsigned int data[] = {0x0};
                        // WriteProcessMemory(pid, (void *)y, (void *)data, sizeof(data) / sizeof(unsigned int)); //写内存

                        int buffer;
                        ReadProcessMemory(pid, (void *)x, &buffer, sizeof(buffer));
                        zd[i].x = buffer;
                        ReadProcessMemory(pid, (void *)y, &buffer, sizeof(buffer));
                        zd[i].y = buffer;
                        i++;
                        continue;
                    }
                    break;
                }
                free(target_);
            }
        lable:
            ml = (PModuleListEntry)ml->next_ptr;
        }
        sendto(udp_socket_fd, zd, sizeof(zd), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    }
    close(udp_socket_fd);
    return 0;
}