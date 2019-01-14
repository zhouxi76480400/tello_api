//
// Created by zhouxi on 19-1-14.
//

#ifndef TELLO_API_TELLO_H
#define TELLO_API_TELLO_H

#endif //TELLO_API_TELLO_H

#define TELLO_IP_ADDR "192.168.10.1"
#define TELLO_LISTEN_IP_ADDR_ANY "0.0.0.0"
#define TELLO_UDP_PORT_SEND_COMMMAND 8889
#define TELLO_UDP_PORT_STATUS 8890
#define TELLO_UDP_PORT_VIDEO_STREAM 11111

#define TELLO_DATA_CUT_LENGTH 1460

#define TELLO_STATUS_SUCCESSFUL "ok"

#define TELLO_STATUS_FAILED "error"

#define TELLO_SUB_THREAD 2


#include <string>
#include <thread>
#include <vector>

using namespace std;

class Task {
public:string message;
    string result;
    bool have_extra;
    string extra;
    bool isSent;
    Task();
};

class Tello {

private:
    bool is_init;
    bool is_connected;
    string serial_number;
    // define socket fd
    int send_socket_fd;
    int status_socket_fd;
    int video_stream_socket_fd;

public:
    Tello();
    ~Tello();
    bool isInit();
    bool isConnected();
    void setConnect(bool isConnect);
    string getSerialNumber();
    void setInit(bool isInit);
    void setSendSocketFD(int fd);
    void setStatusSocketFD(int fd);
    void setVideoStreamSocketFD(int fd);
    int getSendSocketFD();
    int getStatusSocketFD();
    int getVideoStreamSocketFD();
    vector<Task *> task_list;
    bool is_sending_now;
};

void init(Tello * tello);

#pragma mark controll command

/**
 * Command Name command
 * @param tello
 * @return
 */
void entrySDKMode(Tello * tello);

/**
 * Command Name battery?
 * @param tello
 */
void getBatteryPercentage(Tello * tello);

void setSpeed(Tello * tello , int cm_per_sec);

/**
 * Command Name
 * @param tello takeoff
 */
void autoTakeoff(Tello * tello);

/**
 * Command Name land
 * @param tello
 */
void autoLand(Tello * tello);

void flyForward(Tello * tello, int distance);

void flyBack(Tello * tello, int back);

void flyLeft(Tello * tello, int left);

void flyRight(Tello * tello, int right);

void flyUp(Tello * tello, int up);

void flyDown(Tello * tello, int down);