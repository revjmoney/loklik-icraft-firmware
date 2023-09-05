// Copyright (c) 2022 -	Neil 

#pragma once
#ifndef SJCUTTER_H
#define SJCUTTER_H


#include "../Grbl.h" 
#include <cstdint> 


#define IO_SECURITY_PKG_SIZE            22//加密响应包大小
#define IO_SECURITY_SUMVAL              113//安全和值
#define IO_SECURITY_REQUEST_MARK        62//请求包标识
#define IO_SECURITY_RESPONSE_MARK       60//响应包标识
#define IO_SECURITY_ALERT_LOST_PKG_MARK 33//丢包警告标识
#define IO_SECURITY_LINE_PKG_MARK 35// 行标识
#define FIRMWARE_VERSION               "A11"//固件版本

#define BUTTON_POWER		0
#define BUTTON_PAUSE		1
#define BUTTON_PAPER		2
#define BUTTON_START		3

/// 指示灯状态
typedef enum{
  LED_OFF = 0,
  LED_ON = 1,
  LED_SLOW = 2,
  LED_FAST = 3
} LEDActions;

typedef enum{
  PAPER_BTN_MODE_DEF = 0,
  PAPER_BTN_MODE_ADJ = 1,
} PAPER_BTN_MODE;
/// 指令执行状态
typedef enum{
  RESULT_FAIL = 0,
  RESULT_OK = 1,
  RESULT_IGNORE = 2,
  RESULT_ABANDON = 3,
} CMDResult;

/// 切割机实现
class SJCutter {
public:
    // 行数据最大字节数
    static const int maxLine = 255;
    // 测试模式
    bool testMode = false;
    // 开关状态
    u_int powerStatus = false;
    // 物料加载状态
    bool mateLoadStatus = true;
    // 任务运行状态
    bool taskRunStatus = false;    
    // 任务暂停状态
    bool taskPausedStatus = false;
    // 安全传输码
    uint8_t   taskSecurityCode =0;
    // 最后接收行号
    u_int   taskRecvLine =0;

    // 开启时长
    unsigned long powerOnTime = millis();
    // LED集合
    u_int ledSet[4][2]={
        {PIN_LED_POWER, LEDActions::LED_OFF},
        {PIN_LED_PAUSE, LEDActions::LED_OFF},
        {PIN_LED_PAPER, LEDActions::LED_OFF},
        {PIN_LED_START, LEDActions::LED_OFF},
    };  
    
    // 按钮集合
    u_int buttonSet[4][2]={
        {PIN_LED_POWER, LOW},
        {PIN_LED_PAUSE, LOW},
        {PIN_LED_PAPER, LOW},
        {PIN_LED_START, LOW},
    }; 
 

protected:
    const char* _name;             

public:
    SJCutter();
    ~SJCutter(); 
    // 初始化
    void init();
    // 事务循环
    void loop();
    // 指令消息处理
    CMDResult handleMessage(uint8_t client,char* line);
    // 启动测试模式
    int startTestMode();
    // 修改控制端发送的指令
    void modifyCommendLine(char* line);
    // 设备按钮状态
    void setButtonST(u_int key,bool val);
    // 获得取按钮状态
    bool getButtonST(u_int key);
    // 设置LED
    void setCtrlLedAction(int key,int action);
    // 变更物料加载状态
    void exchangeMateLoadStatus(uint8_t client);
    // 变更任务暂停状态
    void exchangeTaskPausedStatus(uint8_t client);
     // 读取序列数据
    void readSerialNumber(uint8_t client);
    // 读取固件版本
    void readFirmwareVersion(uint8_t client);
    // 读取状态数据
    void readStatus(char* status_info);    
    // 数据解码
    bool decodeData(uint8_t client,char* line);
    // 数据编码
    bool encodeData(uint8_t client,char* line);
};

// 
void ctrlPanelInit();
void ctrlPanelLoop();


extern SJCutter SJCutterIns;

#endif