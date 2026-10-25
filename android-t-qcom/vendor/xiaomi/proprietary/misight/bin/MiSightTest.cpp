//
// Created by ritter on 22-3-10.
//

#include <iostream>

#include "MiSight.h"
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

using namespace android;

void multiThreadWriteRead() {
    int pid;
    std::stringstream ss;
    std::string write_buf;

    int fd;
    fd = open("/dev/miev", O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(-2);
    }


    ss << "{PID_" << getpid() << "_VALUE_abcdefghijklmnopqrstuvwxyz123456789_END}";
    write_buf = ss.str();

    char read_buf[4096];

    for (int i = 0; i < 100; i++) {
        pid = fork();
        if (pid == 0 || pid == -1)
        {
            break;
        }
    }

    if (pid == -1) {
        std::cout << "fail to fork!" << std::endl;
        exit(1);
    } else if (pid == 0) {
        int count = 10;
        while (count> 0) {
            int ret = write(fd, write_buf.c_str(), write_buf.size());
        }
        exit(0);
    } else {
        while (1) {
            std::cout << "MiSightTest main process,pid:" << getpid() << ", beforce read" << std::endl;
            memset(read_buf, 0 , 4096);
            int ret = read(fd, (char *)read_buf, sizeof(read_buf));
            std::cout << "MiSightTest main process,pid:" << getpid() << ", after read ret:" << ret << ", read_buf:" << read_buf << std::endl;
            for (int i = 0; i < ret; i++) {
                std::cout << read_buf[i];
            }
            std::cout << "======MiSight read Done" << std::endl;
        }
        exit(0);
    }
}

void multiThreadWriteReadNew() {
    int pid;
    std::stringstream ss;
    std::string write_buf;

    int fd;
    fd = open("/dev/miev", O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(-2);
    }

    struct pollfd fds[1]; //定义1个pollfd结构体，nfds = 1
    int timeout_ms = 5000;

    fds[0].fd = fd;         //初始化fd
    fds[0].events = POLLIN; //初始化fd，有数据可读


    ss << "{PID_" << getpid() << "_VALUE_abcdefghijklmnopqrstuvwxyz123456789_END}";
    write_buf = ss.str();
    char read_buf[4096];
    for (int i = 0; i < 100; i++) {
        pid = fork();
        if (pid == 0 || pid == -1)
        {
            break;
        }
    }

    if (pid == -1) {
        std::cout << "fail to fork!" << std::endl;
        exit(1);
    } else if (pid == 0) {
        int count = 10;
        while (count> 0) {
            int ret = write(fd, write_buf.c_str(), write_buf.size());
        }
        exit(0);
    } else {
        while (1) {
            int ret = poll(fds, 1, timeout_ms);
            memset(read_buf,0,sizeof(read_buf));
            if ((ret == 1) && (fds[0].revents & POLLIN)) {
                ret = read(fd, (char *)read_buf, sizeof(read_buf));
                std::cout << "MiSightTest main process,pid:" << getpid() << ", after read ret:" << ret << ", read_buf:" << read_buf << std::endl;
            } else {
                printf("timeout\n");
            }
        }
        exit(0);
    }
}


void readFromKernel() {
    int fd;
    fd = open("/dev/miev", O_RDWR);
    if (fd < 0) {
        std::cout << "MiSight open /dev/miev fail" << std::endl;
        perror("open");
        exit(-2);
    }

    // 测试读接口
    char read_buf[4096];
    int ret = read(fd, (char *)read_buf, sizeof(read_buf));
    // // 读取'\0'被截断
    std::cout << "MiSight  part read_buf:" << read_buf << std::endl;
    // //完整读出来
    std::cout << "MiSight 完整的read_buf:"<< std::endl;
    for (int i = 0; i < ret; i++)
    {
        std::cout << "MiSight:" << read_buf[i] << std::endl;
    }
    close(fd);
}

void readFromKernelNew() {
    int fd;
    fd = open("/dev/miev", O_RDWR);
    if (fd < 0) {
        std::cout << "MiSight open /dev/miev fail" << std::endl;
        perror("open");
        exit(-2);
    }
    // 测试读接口
    char read_buf[4096];
    struct pollfd fds[1]; //定义1个pollfd结构体，nfds = 1
    int timeout_ms = 5000;

    fds[0].fd = fd;         //初始化fd
    fds[0].events = POLLIN; //初始化fd，有数据可读

    int ret = poll(fds, 1, timeout_ms);
    if ((ret == 1) && (fds[0].revents & POLLIN)) {
        ret = read(fd, (char *)read_buf, sizeof(read_buf));
        std::cout << "MiSight read_buf:" << read_buf << std::endl;
    } else {
        std::cout << "MiSight timeout end..." << std::endl;
    }
}

int parseArgs(int argc, char* argv[])
{
    MiEvent miEvent = MiEvent(900123123);
    for(int loop = 1; loop < argc; loop = loop + 1) {
        if (strcmp(argv[loop], "-id") == 0) {
            std::string value = argv[++loop];
            //std::cout<<"-id:" + value<<std::endl;
            miEvent.put("MiEvent_eventID", value);
            continue;
        }
        if (strcmp(argv[loop], "-put") == 0) {
            std::string key = argv[++loop];
            std::string value = argv[++loop];
            //std::cout<<"-put:" + key + " " + value<<std::endl;
            miEvent.put(key, value);
            continue;
        }
        if (strcmp(argv[loop], "-put_int") == 0) {
            std::string key = argv[++loop];
            int value = atoi(argv[++loop]);
            //std::cout<<"-put:" + key + " " + value<<std::endl;
            miEvent.put(key, value);
            continue;
        }
        if (strcmp(argv[loop], "-dump") == 0) {
            ++loop;
            std::cout<<"MiEvent:" + miEvent.flatten()<<std::endl;
            continue;
        }
        if (strcmp(argv[loop], "-send") == 0) {
            ++loop;
            std::cout<<"MiEvent send..."<<std::endl;
            MiSight::sendEvent(miEvent);
            continue;
        }
        if (strcmp(argv[loop], "-read") == 0) {
            ++loop;
            std::cout<<"MiEvent read..."<<std::endl;
            //readFromKernel();
            readFromKernelNew();
            continue;
        }
        if (strcmp(argv[loop], "-test") == 0) {
            ++loop;
            std::cout<<"MiEvent test..."<<std::endl;
            multiThreadWriteReadNew();
            continue;
        }
        std::cout<<"unsupport args: "<<loop<<" , "<< argv[loop]<<std::endl;
        return -1;
    };
    return 0;
}

int main(int argc, char* argv[]) {
    parseArgs(argc, argv);

/*
    //const std::string jsonStr("{{{\"key3\":\"val3\"\"");
    //std::cout<<"jsonStr:" + jsonStr<<std::endl;

    //MiEvent& miEvent = MiSight::constructEvent(123, jsonStr);
    MiEvent miEvent = MiEvent(900101101);
    std::cout<<"MiEvent:" + miEvent.flatten()<<std::endl;
    std::cout<<"MiEvent:" + miEvent.put("key2_int", 9000).put("key3_Str", "A\nB\tC").flatten()<<std::endl;

    std::cout<<"flatten MiEvent:" + miEvent.put("key2_int", 9000).put("key3_Str", "A\nB\tC").flatten()<<std::endl;

    MiEvent* miEvent2Ptr = new MiEvent(900101102);
    miEvent2Ptr->put("key", "value");
    std::cout<<"MiEvent2:" + miEvent2Ptr->jsonFormatString()<<std::endl;

    std::cout<<"MiEvent_putMiEvent:" + miEvent.putMiEvent("key4_MiEvent", *miEvent2Ptr).flatten()<<std::endl;

    std::vector<int> miIntVector = {1000,2000,3000};
    std::cout<<"MiEvent_putArray:" + miEvent.putArray("key5_intArray", miIntVector).flatten()<<std::endl;

    std::vector<MiEvent> miEventVector;
    miEventVector.emplace_back(1000);
    miEventVector.emplace_back(2000);
    miEventVector.emplace_back(3000);
    std::cout<<"MiEvent_putMiEventArray:" + miEvent.putMiEventArray("key6_MiEventArray", miEventVector).flatten()<<std::endl;

    return 0;
    */
}