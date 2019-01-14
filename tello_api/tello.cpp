//
// Created by zhouxi on 19-1-14.
//

#include "tello.h"
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <iosfwd>

using namespace std;

Task::Task() {
    this->isSent = false;
    this->have_extra = false;
}


sockaddr_in generate_sock_addr_in(string addr, uint16_t port);

void listen_command_thread(Tello *tello);
void task_send_thread(Tello * tello);
void listen_status_thread(Tello * tello);
void listen_video_thread(Tello * tello);
//command
void sendToTello(Tello * tello, string data);
void entrySDKMode(Tello * tello);


Tello::Tello() {
    this->is_init = false;
    this->is_connected = false;
    this->serial_number = "";
    this->video_stream_socket_fd = -1;
    this->status_socket_fd = -1;
    this->send_socket_fd = -1;
    this->is_sending_now = false;
    init(this);
}

Tello::~Tello() {
    pthread_exit(NULL);
}

string Tello::getSerialNumber() {
    return "test";
}

bool Tello::isInit() {
    return this->is_init;
}

void Tello::setConnect(bool isConnect) {
    this->is_connected = isConnect;
}

void Tello::setInit(bool isInit) {
    this->is_init = isInit;
}

bool Tello::isConnected() {
    return false;
}

void Tello::setSendSocketFD(int fd) {
    this->send_socket_fd = fd;
}

void Tello::setStatusSocketFD(int fd) {
    this->status_socket_fd = fd;
}

void Tello::setVideoStreamSocketFD(int fd) {
    this->video_stream_socket_fd = fd;
}

int Tello::getSendSocketFD() {
    return this->send_socket_fd;
}

int Tello::getStatusSocketFD() {
    return this->status_socket_fd;
}

int Tello::getVideoStreamSocketFD() {
    return this->video_stream_socket_fd;
}

void init(Tello *tello) {
    if(tello != NULL && !tello->isInit()) {
        cout << "Try to connect to the Tello" <<
             "\n send " << TELLO_IP_ADDR << ":" << TELLO_UDP_PORT_SEND_COMMMAND <<
             "\n status receive " << TELLO_LISTEN_IP_ADDR_ANY << ":" << TELLO_UDP_PORT_STATUS <<
             "\n video stream receive " << TELLO_LISTEN_IP_ADDR_ANY << ":" << TELLO_UDP_PORT_VIDEO_STREAM << endl;
        // get send fd
        int send_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (send_fd < 0) {
            perror("can not allocate socket fd for send");
            exit(1);
        }
        tello->setSendSocketFD(send_fd);
        cout << endl << "get socket fd for send successful, fd:" << send_fd << endl;
        // get receive fd
        int receive_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (receive_fd < 0) {
            perror("can not allocate socket fd for receive");
            exit(1);
        }
        tello->setStatusSocketFD(receive_fd);
        cout << endl << "get socket fd for receive successful, fd:" << receive_fd << endl;
        // get video stream fd
        int video_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (receive_fd < 0) {
            perror("can not allocate socket fd for video stream");
            exit(1);
        }
        tello->setVideoStreamSocketFD(video_fd);
        cout << endl << "get socket fd for video successful, fd:" << video_fd << endl;
        // bind address to fd
        sockaddr_in receive_sockaddr = generate_sock_addr_in(TELLO_LISTEN_IP_ADDR_ANY, TELLO_UDP_PORT_STATUS);
        int status = bind(tello->getStatusSocketFD(), (struct sockaddr *)&receive_sockaddr, sizeof(receive_sockaddr));
        if(status < 0) {
            perror("receive socket bind failed");
            exit(1);
        }
        sockaddr_in video_sockaddr = generate_sock_addr_in(TELLO_LISTEN_IP_ADDR_ANY, TELLO_UDP_PORT_VIDEO_STREAM);
        status = bind(tello->getVideoStreamSocketFD(), (struct sockaddr *)&video_sockaddr, sizeof(video_sockaddr));
        if(status < 0) {
            perror("video stream socket bind failed");
            exit(1);
        }
        tello->setInit(true);
        // using multi threads
        thread t1(listen_status_thread,ref(tello));
        thread t2(listen_video_thread,ref(tello));
        thread t_send_recv(listen_command_thread, ref(tello));
        thread t_send(task_send_thread, ref(tello));
        t1.join();
        t2.join();
        t_send_recv.join();
        t_send.join();
    }
}

void listen_command_thread(Tello *tello) {
    sockaddr_in receive_sockaddr = generate_sock_addr_in(TELLO_LISTEN_IP_ADDR_ANY, TELLO_UDP_PORT_SEND_COMMMAND);
    void * buff = malloc(TELLO_DATA_CUT_LENGTH);
    int receive_sockaddr_len = sizeof(sockaddr_in);
    while (true) {
        int len = recvfrom(tello->getSendSocketFD(), buff, TELLO_DATA_CUT_LENGTH, 0,
                           (struct sockaddr *) &receive_sockaddr, (socklen_t *) &receive_sockaddr_len);
        if(len < 0) {
            perror("cannot read datagram");
            continue;
        }else {
            char * return_char = new char(len);
            memcpy(return_char,buff,len);
            string s = string(return_char);
            cout << s << endl;
            Task * last_task = NULL;
            if(!tello->task_list.empty()) {
                Task * tmp_task = tello->task_list[0];
                if(tmp_task->result.empty() && tmp_task->isSent) {
                    last_task = tmp_task;
                }
            }
            if(last_task != NULL) {
                if(s.compare(TELLO_STATUS_FAILED) == 0) {
                    cout << last_task->message << " failed" << endl;
                    last_task->result = TELLO_STATUS_FAILED;
                }else {
                    cout << last_task->message << " successful" << endl;
                    if(last_task->have_extra) {
                        last_task->extra = s;
                        cout << "extra value:" << last_task->extra << endl;
                    }
                    last_task->result = TELLO_STATUS_SUCCESSFUL;
                }
            }
            delete(return_char);
        }
    }
}

void listen_status_thread(Tello *tello) {
    sockaddr_in receive_sockaddr = generate_sock_addr_in(TELLO_LISTEN_IP_ADDR_ANY, TELLO_UDP_PORT_STATUS);
    void * buff = malloc(TELLO_DATA_CUT_LENGTH);
    int receive_sockaddr_len = sizeof(sockaddr_in);
    while (true) {
        int len = recvfrom(tello->getStatusSocketFD(), buff, TELLO_DATA_CUT_LENGTH, 0,
                           (struct sockaddr *) &receive_sockaddr, (socklen_t *) &receive_sockaddr_len);
        if(len < 0) {
            perror("cannot read datagram");
            continue;
        }else {
            char * return_char = new char(len);
            memcpy(return_char,buff,len);
            string s = string(return_char);
            cout << s << endl;
        }
    }
}

void listen_video_thread(Tello * tello) {
    while (true) {
//        cout << "2" << endl;
        sleep(10);
    }
}

void advanced_handle(Tello * t, Task * task) {
    if(task->message.compare("command") == 0) {
        t->setConnect(true);
    }else if(task->message.compare("battery?") == 0) {
        int percent = stoi(task->extra);
        if(percent < 50) {
            cout << "battery percentage less than 50, now " + task->extra +", program must exit." << endl;
            exit(1);
        }
    }
}

void task_send_thread(Tello * t) {
    entrySDKMode(t);
    getBatteryPercentage(t);
    setSpeed(t, 30);
    autoTakeoff(t);
    flyUp(t,50);
    flyForward(t,80);
    flyBack(t,80);
    flyLeft(t,80);
    flyRight(t,80);
    autoLand(t);

    while (!t->task_list.empty()) {
        usleep(1000 * 500);

        Task * last_task = t->task_list[0];
        if(!last_task->isSent && last_task->result.empty() && !t->is_sending_now) {
            sendToTello(t, last_task ->message);
            last_task->isSent = true;
        }else if(last_task->isSent && !last_task->result.empty()){
            if(last_task->result.compare(TELLO_STATUS_FAILED) == 0) {
                last_task->result = "";
                last_task->isSent = false;
                t->task_list.erase(t->task_list.begin());
                t->task_list.push_back(last_task);
            }else {
                advanced_handle(t,last_task);
                t->task_list.erase(t->task_list.begin());
                last_task = NULL;
                delete last_task;
            }
        }
    }
    exit(0);
}


sockaddr_in generate_sock_addr_in(string addr, uint16_t port) {
    sockaddr_in sockaddrIn;
    memset(&sockaddrIn, 0, sizeof(sockaddrIn));
    sockaddrIn.sin_family = PF_INET;
    if(addr.compare(TELLO_LISTEN_IP_ADDR_ANY) == 0) {
        // any
        sockaddrIn.sin_addr.s_addr = htonl(INADDR_ANY);
    }else {
        sockaddrIn.sin_addr.s_addr = inet_addr(addr.c_str());
    }
    sockaddrIn.sin_port = htons(port);
    return sockaddrIn;
}


void sendToTello(Tello * tello, string data) {
    if(tello != NULL) {
        tello->is_sending_now = true;
        sockaddr_in send_sockaddr = generate_sock_addr_in(TELLO_IP_ADDR, TELLO_UDP_PORT_SEND_COMMMAND);
        int return_code = sendto(tello->getSendSocketFD(), data.c_str(), data.length(), 0,
                                 (struct sockaddr *) &send_sockaddr, sizeof(struct sockaddr_in));
        if(return_code < 0) {
            perror("cannot send message to Tello");
            exit(1);
        }
        tello->is_sending_now = false;
    }
}

// start command
void entrySDKMode(Tello * tello) {
//    cout<< endl<< "try to entry SDK Mode"<< endl;
    Task * task = new Task();
    task->message = "command";
    tello->task_list.push_back(task);
}

void getBatteryPercentage(Tello *tello) {
//    cout<< endl<< "try to get battery percentage"<< endl;
    Task * task = new Task();
    task->message = "battery?";
    task->have_extra = true;
    tello->task_list.push_back(task);
}

void setSpeed(Tello * tello , int cm_per_sec) {
    Task * task = new Task();
    task->message = "speed " + to_string(cm_per_sec);
    tello->task_list.push_back(task);
}

void autoTakeoff(Tello * tello) {
    Task * task = new Task();
    task->message = "takeoff";
    tello->task_list.push_back(task);
}

void autoLand(Tello * tello) {
    Task * task = new Task();
    task->message = "land";
    tello->task_list.push_back(task);
}

void flyForward(Tello * tello, int distance) {
    Task * task = new Task();
    task->message = "forward " + to_string(distance);
    tello->task_list.push_back(task);
}

void flyBack(Tello * tello, int back) {
    Task * task = new Task();
    task->message = "back " + to_string(back);
    tello->task_list.push_back(task);
}

void flyLeft(Tello * tello, int left) {
    Task * task = new Task();
    task->message = "left " + to_string(left);
    tello->task_list.push_back(task);
}

void flyRight(Tello * tello, int right) {
    Task * task = new Task();
    task->message = "right " + to_string(right);
    tello->task_list.push_back(task);
}

void flyUp(Tello * tello, int up) {
    Task * task = new Task();
    task->message = "up " + to_string(up);
    tello->task_list.push_back(task);
}

void flyDown(Tello * tello, int down) {
    Task * task = new Task();
    task->message = "down " + to_string(down);
    tello->task_list.push_back(task);
}