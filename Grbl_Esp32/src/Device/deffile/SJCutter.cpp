// Copyright (c) 2022 -	Neil
// Use of this source code is governed by a GPLv3 license that can be found in the LICENSE file.
#include "../../Grbl.h" 
#include "../SJCutter.h"
#include "../SJUtils.hpp"
#include "../SJMotorCtrl.h"
#include "../COpticalAlign.h"
#include <OneButton.h>
#include "FS.h"
#include "SPIFFS.h"
#include "../../Uart.h"
// #include "base64.hpp"
// #include <Base64.h>

//  extern Base64Class Base64;

SJCutter SJCutterIns;
OneButton btn_power = OneButton(PIN_ONE_KEY, false, false);
OneButton btn_paper;
OneButton btn_start;
OneButton btn_pause;
u_int last_btn_val =0;
// 进纸键模式
u_int paper_btn_mode =PAPER_BTN_MODE::PAPER_BTN_MODE_DEF;
// 记录启动时间
unsigned long start_up_time =millis();

u_int startup_status_tag=0;
u_int startup_status_tags[4]={1000,3000,5000,8000};  

void ctrlPanelInit(){
  pinMode(PIN_POWER, OUTPUT);
  pinMode(PIN_MENU_KEY, INPUT);
  pinMode(PIN_LED_POWER, OUTPUT);
  pinMode(PIN_LED_PAUSE, OUTPUT);
  pinMode(PIN_LED_PAPER, OUTPUT);
  pinMode(PIN_LED_START, OUTPUT);

  pinMode(PEN1_END_STOP, INPUT);
  pinMode(PEN2_END_STOP, INPUT);    
  pinMode(PIN_PAPER, INPUT_PULLUP);
  pinMode(X_LIMIT_PIN, INPUT);    

 
  
  digitalWrite(PIN_POWER, HIGH);
  digitalWrite(PIN_LED_POWER, HIGH);
  digitalWrite(PIN_LED_PAUSE, HIGH);
  digitalWrite(PIN_LED_PAPER, HIGH);
  digitalWrite(PIN_LED_START, HIGH);
  delay_ms(800);  


  SJCutterIns.setCtrlLedAction(BUTTON_POWER,LEDActions::LED_ON);
  SJCutterIns.setCtrlLedAction(BUTTON_PAUSE,LEDActions::LED_ON);
  SJCutterIns.setCtrlLedAction(BUTTON_PAPER,LEDActions::LED_ON);
  SJCutterIns.setCtrlLedAction(BUTTON_START,LEDActions::LED_ON);  


  btn_power.setClickTicks(400);
  btn_power.setPressTicks(800);
  btn_power.attachLongPressStart([](){
    if((millis()-SJCutterIns.powerOnTime)>5000){
      SJCutterIns.powerStatus=0;
      digitalWrite(PIN_POWER, LOW);
      SJCutterIns.setCtrlLedAction(BUTTON_POWER,LEDActions::LED_OFF);
    }
  });

  btn_pause.setDebounceTicks(80);
  btn_pause.attachClick([](){
    if(paper_btn_mode==PAPER_BTN_MODE::PAPER_BTN_MODE_ADJ){
      SJMotorCtrl::moveAndResetYAxis(CLIENT_ALL,10);
    }else{
      SJCutterIns.exchangeTaskPausedStatus(CLIENT_ALL);
    } 
  });

  // btn_paper.setDebounceTicks(80);
  btn_paper.setPressTicks(800);
  btn_paper.attachClick([](){
    if(paper_btn_mode==PAPER_BTN_MODE::PAPER_BTN_MODE_ADJ){
      // 切换为进纸模式
      paper_btn_mode=PAPER_BTN_MODE::PAPER_BTN_MODE_DEF;
      SJCutterIns.setCtrlLedAction(BUTTON_PAPER,LEDActions::LED_ON);
      SJCutterIns.setButtonST(BUTTON_PAPER,false);
    } else {
      // 切换进退料
      SJCutterIns.exchangeMateLoadStatus(CLIENT_ALL);  
    }    
  });
  
  btn_paper.attachLongPressStart([](){
    if(SJCutterIns.taskRunStatus)return;
    // 切换进纸键模式为微调模式
    paper_btn_mode=PAPER_BTN_MODE::PAPER_BTN_MODE_ADJ;
    SJCutterIns.setCtrlLedAction(BUTTON_PAPER,LEDActions::LED_FAST);
  });
  // btn_start.setDebounceTicks(80);
  btn_start.attachClick([](){
    if(paper_btn_mode==PAPER_BTN_MODE::PAPER_BTN_MODE_ADJ){
      SJMotorCtrl::moveAndResetYAxis(CLIENT_ALL,-10);
    }else {
      if(SJCutterIns.testMode){
        // 结束测试模式
        SJCutterIns.setCtrlLedAction(BUTTON_START, LEDActions::LED_OFF);
        SJCutterIns.testMode=false;
      }else{
        if(!SJCutterIns.taskRunStatus&&!SJCutterIns.taskPausedStatus){
          SJCutterIns.setButtonST(BUTTON_START,false);
          SJCutterIns.setCtrlLedAction(BUTTON_START, LEDActions::LED_OFF);
        }  
        // COpticalAlign optalign;
        // SJMotorCtrl::xyAxisHoming(CLIENT_ALL,false);
        // optalign.reviseOffset(1);
        // SJMotorCtrl::executeCommand(CLIENT_ALL,"$J=G90G21X150Y0F5000");

      } 
    } 
  });

  btn_start.attachLongPressStart([](){
      if(SJCutterIns.taskRunStatus)return;
      paper_btn_mode=PAPER_BTN_MODE::PAPER_BTN_MODE_DEF;
      if(!SJCutterIns.testMode) {
        SJCutterIns.setCtrlLedAction(BUTTON_START, LEDActions::LED_FAST);
        SJMotorCtrl::runTest(CLIENT_ALL);
      }        
      SJCutterIns.setCtrlLedAction(BUTTON_START, LEDActions::LED_OFF); 
  });

}
 
void ctrlPanelLoop(){
  unsigned long now = millis();
  unsigned long now_mic = micros();

   // 功能按钮
  int tValue = analogRead(PIN_MENU_KEY);
  // 按钮状态检测 
  btn_power.tick();
 
    if(tValue>=0&&tValue<2200){
      last_btn_val=tValue;      
    }

    btn_pause.tick((tValue > 2000 && tValue < 3000));
    btn_paper.tick((tValue > 1000 && tValue < 2000));
    btn_start.tick((tValue > 300 && tValue < 1000));

    // btn_paper.tick((tValue > 1500 && tValue < 2200));
    // btn_start.tick((tValue > 500 && tValue < 1200));
    // btn_pause.tick((tValue >= 0 && tValue <20));     
  



  // 启动初化动作
  // startup_status_tag=0  start_up_time= millis()
  if(startup_status_tag<4){
    // 判断系统状态是否允许执行
    bool able_run_c1=!(sys.state == State::Alarm || sys.state == State::Jog ||  sys.state == State::Homing);
    // 判断在时间范围内
    bool able_run_c2=((now-start_up_time)>startup_status_tags[startup_status_tag]);

    if(startup_status_tag==0&&able_run_c1&&able_run_c2){          
      SJMotorCtrl::zAxisHoming(CLIENT_ALL); 
      startup_status_tag++;
    }else if(startup_status_tag==1&&able_run_c1&&able_run_c2){
      SJMotorCtrl::moveYAxis(CLIENT_ALL,-320);
      startup_status_tag++;      
    }else if(startup_status_tag==2&&able_run_c1&&able_run_c2){     
      SJMotorCtrl::resetAxis(CLIENT_ALL,true);     
      startup_status_tag++;
    }else if(startup_status_tag==3&&able_run_c1&&able_run_c2){
      SJCutterIns.setCtrlLedAction(BUTTON_PAUSE,LEDActions::LED_OFF);
      SJCutterIns.setCtrlLedAction(BUTTON_PAPER,LEDActions::LED_ON);
      SJCutterIns.setCtrlLedAction(BUTTON_START,LEDActions::LED_OFF);  
      SJCutterIns.setButtonST(BUTTON_POWER,true);
      SJCutterIns.setButtonST(BUTTON_PAUSE,false);    
      SJCutterIns.setButtonST(BUTTON_PAPER,true);
      SJCutterIns.setButtonST(BUTTON_START,false);
      SJCutterIns.taskRunStatus=false;
      SJCutterIns.taskPausedStatus=false;
      startup_status_tag++;
    }
  }
 


  // 灯光控制
  for (int i = 0; i < 4; i++){
    int key = SJCutterIns.ledSet[i][0];
    int action = SJCutterIns.ledSet[i][1];
    if (action == LEDActions::LED_OFF){
      digitalWrite(key, LOW);
    }else if (action == LEDActions::LED_ON){
      digitalWrite(key, HIGH);
    }else if (action == LEDActions::LED_SLOW){
      if (now % 1000 < 500){
        digitalWrite(key, LOW);
      }else{
        digitalWrite(key, HIGH);
      }
    }else if (action == LEDActions::LED_FAST){
      if (now % 200 < 100){
        digitalWrite(key, LOW);
      }else{
        digitalWrite(key, HIGH);
      }
    }
  }
}



SJCutter::SJCutter() {}
SJCutter::~SJCutter() {}
 
void SJCutter::init() {    
  if (!SPIFFS.exists("/")){
      SPIFFS.begin(true);
  }  
  SJMotorCtrl::init();    
}
 
 // 事务循环
void SJCutter::loop() {    

    SJMotorCtrl::loop();

    ctrlPanelLoop();
}

// 设备按钮状态
void SJCutter::setButtonST(u_int key,bool val){
  u_int oldval=this->buttonSet[key][1]; 
  u_int newval=val?1:0;
  if(oldval!=newval){
    this->buttonSet[key][1]=newval;
    // this->ledSet[key][1]=newval;
  }
}
// 获得取按钮状态
bool SJCutter::getButtonST(u_int key){
  return this->buttonSet[key][1]>0;
}

// 设置LED
void SJCutter::setCtrlLedAction(int key,int action){
  // 改变LED活动状态
  this->ledSet[key][1]=action;
  // 立即响应开关机动作，其它动作需要等待轮询
  if(this->ledSet[key][1]==LEDActions::LED_OFF){
    digitalWrite(key, LOW);
  }else if(this->ledSet[key][1]==LEDActions::LED_ON){
    digitalWrite(key, HIGH);
  }
}
 

// 数据解码
bool SJCutter::decodeData(uint8_t client,char* line){
  int line_char_max=255;
  // 有效数据偏移
  int vdata_offset=4;
  // 编码数据长度
  int data_len=strlen(line)-1;   
  // 临时存放BASE64编码数据 
  char base64_data[data_len];
  // 存放解码后的数据
  char row_data[line_char_max];
  // 设置初始数据
  memset(base64_data,0,data_len);
  memset(row_data,0,line_char_max);
  // 复制有效数据到新数组
  memcpy(base64_data,(char *)(line+1),data_len);    
  // BASE64解码
  SJUtils::base64_decode(base64_data,data_len,row_data);  

  // 从解码数据中取行号
  u_int line_num=(u_int)((unsigned int)row_data[0]|((unsigned int)row_data[1])<<8|((unsigned int)row_data[2])<<16);
  // 原始数据长度
  u_int rdata_len=(u_int)(row_data[3]);
  for(u_int i=vdata_offset;i<(rdata_len+4);i++){
    row_data[i]=SJUtils::bitencrypt(row_data[i],this->taskSecurityCode); 
  }
   
  // 实际数据长度
  u_int vdata_len=strlen(row_data+vdata_offset);
  // 置换命令数据
  memset(line,0,vdata_len);
  memcpy(line,row_data+vdata_offset,vdata_len);
  // 在有效数据区外插入字符串结束符
  line[vdata_len]=0; 
  // 判断接收数据长度是否与原数据长度一致
  bool check_status= (rdata_len==vdata_len)&&((this->taskRecvLine+1)==line_num); 
  // 替换尾部空格
  for(int i=vdata_len-2;i<vdata_len;i++){
    u_int fc1=(u_int)line[i]; 
    if(fc1==13||fc1==10){
      line[i]=0;
    }
  } 
  //判断是否丢包
  if(check_status){     
    //grbl_sendf(CLIENT_BTSSP,"--------------: %d\n\0", line_num);
  }
  this->taskRecvLine=line_num;

  int tl=strlen(line);
  //grbl_sendf(CLIENT_BTSSP,"【%d:%d:%d】:%d:%s[%d,%d]:%d:%d:%d\n\0",row_data[0],row_data[1],row_data[2],line_num, line,line[tl-2],line[tl-1],this->taskRecvLine,rdata_len,vdata_len);
  return true;
}




// 数据编码
bool SJCutter::encodeData(uint8_t client,char* line){
   return true;
}

// 指令处理器
CMDResult SJCutter::handleMessage(uint8_t client,char* line) {   
  // uint slen=20;
  // 处理加密码响应
  if (!strncmp(line, ">",1)) {    
    //加密传输请求处理
    uint8_t mark_bit=(uint8_t)line[0];    
    // 判断首字节标记为加密传输请求
    if(mark_bit==IO_SECURITY_REQUEST_MARK){  
      // 计算安全和值
      u_int sval1_bit=(u_int)line[6];
      u_int sval2_bit=(u_int)line[16];

      if(IO_SECURITY_SUMVAL!=(sval1_bit+sval2_bit)){
        // 安全和值不符合预期将中止消息流
        return CMDResult::RESULT_ABANDON;
      } 
      // 准备响应数据包
      uint8_t rsppkg[IO_SECURITY_PKG_SIZE];
      memset(rsppkg,0,IO_SECURITY_PKG_SIZE);
      // 填充随机值
      for(int i=1;i<IO_SECURITY_PKG_SIZE;i++){
        rsppkg[i]=random(65,90);//随机产生可见字符
      }
      rsppkg[0]=IO_SECURITY_RESPONSE_MARK;
      // 产生安全密钥
      rsppkg[6]=random(65,90);
      rsppkg[16]=(127-rsppkg[6]);
      rsppkg[19]=10;
      rsppkg[20]=13;      
      rsppkg[21]=0;      
      // 安全码，用于解码加密包
      this->taskSecurityCode=(rsppkg[6]+rsppkg[16]);
      this->taskRecvLine=0;   
      // 发送响应数据包
      grbl_send(client,(char*)rsppkg);   
      return CMDResult::RESULT_ABANDON;
    }    
  }
  // 处理数据解码
  if (!strncmp(line, "#",1)) {
    // 消息收到进入加密传输模式
    if(this->taskSecurityCode>0){
      // 消息数据解码
      if(!this->decodeData(client,line)){ 
        // 行号不连续，发起丢包警告
        char lost_alert_pkg[IO_SECURITY_PKG_SIZE];
        memset(lost_alert_pkg,0,IO_SECURITY_PKG_SIZE);
        lost_alert_pkg[0]=IO_SECURITY_ALERT_LOST_PKG_MARK;
        lost_alert_pkg[1]='T'; //丢包标记
        lost_alert_pkg[2]='L'; 
        lost_alert_pkg[3]=(char)(this->taskRecvLine); 
        lost_alert_pkg[4]=(char)(this->taskRecvLine>>8); 
        lost_alert_pkg[5]=(char)(this->taskRecvLine>>16); 
        int edlen=SJUtils::base64_encode((unsigned char *)(lost_alert_pkg+3),3,(char *)(lost_alert_pkg+3));
        // 发送警告数据包
        grbl_sendf(client,"%s\n\0",(char*)lost_alert_pkg);
        return CMDResult::RESULT_ABANDON;
      }else{
        grbl_sendf(0,"%s\n",(char*)line);
      }
    }else{
      return CMDResult::RESULT_ABANDON;
    }
  }

  

  // 处理状态
  CMDResult proc_status=CMDResult::RESULT_IGNORE; 
  // 任务执行状态
  bool task_status=this->taskRunStatus;   
  
  if (!strcmp(line, "HP")) {  
    // Z轴归位
    SJMotorCtrl::zAxisHoming(client); 
    proc_status=CMDResult::RESULT_OK; 
  }else if (!strcmp(line, "HA")) { 
    // 归位
    SJMotorCtrl::resetAxis(CLIENT_ALL,false);
    proc_status=CMDResult::RESULT_OK; 
  }else if (!strcmp(line, "HPA")) { 
    // 归位，对刀
    SJMotorCtrl::zAxisHoming(client);
    SJMotorCtrl::resetAxis(CLIENT_ALL,false);
    proc_status=CMDResult::RESULT_OK; 
  }else if (!strncmp(line, "MA Z",4)) { 
    int movedist=0;
    sscanf(line + 4, "%d", &movedist);
    SJMotorCtrl::absMoveAxis(client,Z_AXIS,movedist);
    proc_status=CMDResult::RESULT_OK; 
  }else if (!strncmp(line, "MA X",4)) { 
    int movedist=0;
    sscanf(line + 4, "%d", &movedist);
    SJMotorCtrl::absMoveAxis(client,X_AXIS,movedist);
    proc_status=CMDResult::RESULT_OK; 
  }else if (!strncmp(line, "MA Y",4)) { 
    int movedist=0;
    sscanf(line + 4, "%d", &movedist);
    SJMotorCtrl::absMoveAxis(client,Y_AXIS,movedist);
    proc_status=CMDResult::RESULT_OK; 
  }else if (!strncmp(line, "MR Y",4)) { 
    int movedist=0;
    sscanf(line + 4, "%d", &movedist);
    SJMotorCtrl::relMoveAxis(client,Y_AXIS,movedist);
    proc_status=CMDResult::RESULT_OK; 
  }else if (!strcmp(line, "SV")) {
    char status_info_pkg[1024];
    memset(status_info_pkg,0,1024);
    // 获取传感器     
    this->readStatus((char*)status_info_pkg);
    grbl_sendf(client, "STATUS:%s",status_info_pkg);
    proc_status=CMDResult::RESULT_OK; 
  }else if (!strncmp(line, "TS",2)) {
    // 任务开始    
    this->taskRunStatus=true;  
    this->taskPausedStatus=false;  
    SJCutterIns.setCtrlLedAction(BUTTON_PAUSE,LEDActions::LED_OFF);  
    SJCutterIns.setButtonST(BUTTON_PAUSE,false);
    paper_btn_mode=PAPER_BTN_MODE::PAPER_BTN_MODE_DEF;
    proc_status=CMDResult::RESULT_OK; 
  }else if (!strcmp(line, "TE")) {
    // 任务终止  
    this->taskRunStatus=false;  
    this->taskSecurityCode=0;
    this->taskRecvLine=0;
    proc_status=CMDResult::RESULT_OK; 
  }else if (!strcmp(line, "TP")) {
    // 任务暂停  
    this->taskPausedStatus=true;  
    SJCutterIns.setCtrlLedAction(BUTTON_PAUSE,LEDActions::LED_ON);  
    SJCutterIns.setButtonST(BUTTON_PAUSE,true);
    SJCutterIns.setCtrlLedAction(BUTTON_START,LEDActions::LED_OFF);  
    SJCutterIns.setButtonST(BUTTON_START,false);
    proc_status=CMDResult::RESULT_OK; 
  }else if (!strcmp(line, "TU")) {
    // 任务恢复
    this->taskPausedStatus=false;  
    SJCutterIns.setCtrlLedAction(BUTTON_PAUSE,LEDActions::LED_OFF);  
    SJCutterIns.setButtonST(BUTTON_PAUSE,false);
    proc_status=CMDResult::RESULT_OK; 
    
  }else if (!strcmp(line, "TC")) {
    // 任务清除
    this->taskPausedStatus=false;  
    this->taskRunStatus=false;  
    this->taskSecurityCode=0;
    this->taskRecvLine=0;

    // 中止程序
    SJMotorCtrl::executeCommand(client,"G0 Z0 F5000");
    SJMotorCtrl::executeCommand(client,"M2");

    // 重置状态
    startup_status_tag=0;  
    start_up_time= millis(); 
    proc_status=CMDResult::RESULT_OK; 

    proc_status=CMDResult::RESULT_OK; 
  }else if (!strcmp(line, "LM")) {
    // 加载物料
    this->mateLoadStatus=false;  
    this->exchangeMateLoadStatus(client);
    proc_status=CMDResult::RESULT_OK; 
  }else if (!strcmp(line, "UM")) {
    // 退出物料
    this->mateLoadStatus=true;  
    this->exchangeMateLoadStatus(client);
    proc_status=CMDResult::RESULT_OK; 
  }else if (!strcmp(line, "TESTA")) {
    sys.state = State::Alarm;  // Set system alarm state
    // report_alarm_message(ExecAlarm::SoftLimit); 
    grbl_sendf(CLIENT_ALL, "ALARM:%d\r\n", 1); 
    proc_status=CMDResult::RESULT_OK;
  }else if (!strcmp(line, "TESTE")) { 
    proc_status=CMDResult::RESULT_OK;  
  }else if (!strncmp(line, "TESTOA", 6)) {
    // 机器状态控制 
    float max_val=6.0;
    int axio_dir=0;
    int axio_speed=9000;
    sscanf(line + 7, "%f,%d,%d", &max_val,&axio_dir,&axio_speed); 
    grbl_sendf(CLIENT_SERIAL, "PARAMS: %0.2f,%d,%d\r\n", max_val,axio_dir,axio_speed);   
   SJMotorCtrl::TestFindPointByRange(max_val,axio_dir,axio_speed); 
    proc_status=CMDResult::RESULT_OK; 
  }else if (!strcmp(line, "$BC=AT+BRATE")) { 
    char blueconfig[20];
    memset(blueconfig,0, 20);   
    sprintf(blueconfig, "%s", "AT+BRATE=115200");
    Uart2.close();
    Uart2.setPins(GPIO_NUM_16,GPIO_NUM_17);
    Uart2.begin(BLE_BAUD_RATE, Uart::Data::Bits8, Uart::Stop::Bits1, Uart::Parity::None);
    grbl_send(CLIENT_BTSSP,blueconfig);
    proc_status=CMDResult::RESULT_OK; 
  }else if (!strcmp(line, "$BC=AT+BTNAME")) { 
    grbl_send(CLIENT_BTSSP,"AT+BTNAME?");
    proc_status=CMDResult::RESULT_OK; 
  }else if (!strncmp(line, "$BC=", 4)) {
    char blueconfig[20];
    memset(blueconfig,0, 20);    
    // 配置蓝牙 
    sscanf(line + 4, "%s", &blueconfig);
    grbl_send(CLIENT_BTSSP,blueconfig);
    proc_status=CMDResult::RESULT_ABANDON; 
  }else if (!strncmp(line, "$SN_RESET=", 10)) {
    // 重写入序列号
    sj_serial_number->setStringValue(line + 10);    
    proc_status=CMDResult::RESULT_OK; 
  }else if (!strncmp(line, "$SN=", 4)) {

    auto sn=sj_serial_number->get();
    // 仅当初始编码允许修改
    if(sn==DEFAULT_SERIAL_NUMBER){
      // 写入序列号
      sj_serial_number->setStringValue(line + 4);
      proc_status=CMDResult::RESULT_OK; 
    }else{
      proc_status=CMDResult::RESULT_FAIL; 
    } 
    
  }else if (!strcmp(line, "$SN")) {
    // 设置序列号
    this->readSerialNumber(client);
    //TESTE
    // grbl_send(0,line);
    proc_status=CMDResult::RESULT_OK;  
    
  }else if (!strcmp(line, "$FV")) {
    // 读取固件版本
    this->readFirmwareVersion(client);
    proc_status=CMDResult::RESULT_OK; 
  
  }else if (!strncmp(line, "$MSC",4)) {
    // 机器状态控制
     int key_val=-1;
     int state_val=-1;
    sscanf(line + 5, "%d,%d", &key_val,&state_val);
    if(key_val>=0&&key_val<=3&&(state_val>=0&&state_val<=3)){      
      this->setButtonST(key_val,state_val>0);
      this->setCtrlLedAction(key_val,state_val);
      proc_status=CMDResult::RESULT_OK;       
    }else{
      proc_status=CMDResult::RESULT_FAIL;  
    }
  }else if (!strncmp(line, "SEEKMARK", 8)&&!task_status) {
    //设置序列号     
    int   tTaskIndex = 0;
    float tAreaW     = 0;
    float tAreaH     = 0;
    sscanf(line + 9, "%d,%f,%f", &tTaskIndex, &tAreaW, &tAreaH);  
     COpticalAlign optalign;
    SJMotorCtrl::xyAxisHoming(client,false);
    optalign.reviseOffset(tTaskIndex); 
    proc_status=CMDResult::RESULT_OK; 
  }  
  
  if(proc_status==CMDResult::RESULT_IGNORE){

    // 判断来源为蓝牙
    if(client==CLIENT_BTSSP){
       //grbl_sendf(CLIENT_SERIAL,"BR: %s\r\n",line);    
        // 判断为配置返回
        if (!strncmp(line, "+",1)) {
          // 直接转发至串口
          grbl_sendf(CLIENT_SERIAL,"BR: %s\r\n",line);            
          proc_status=CMDResult::RESULT_ABANDON; 
        }
    }
    // 修改输入指令
    this->modifyCommendLine(line);
    
  }
  return proc_status; 
}

int SJCutter::startTestMode() {

    return 0;
}

void SJCutter::readSerialNumber(uint8_t client) {
  //读取序列号
  String product_name = sj_product_name->get();  
  String serial_number = sj_serial_number->get();
  grbl_sendf(client, "SN:%s-%s\r\n",product_name.c_str(),serial_number.c_str());    
}

void SJCutter::readFirmwareVersion(uint8_t client) {
  //读取固件版本 
  String firmware_version = FIRMWARE_VERSION;
  grbl_sendf(client, "FV:%s\r\n", firmware_version);    
}
void SJCutter::modifyCommendLine(char* line) {
    //PA->Z-; PB->Z+
    char* ptr = strrchr(line, 'P');
    if (ptr) {
        if (ptr[1] == 'A') {
            ptr[0] = 'Z';
            ptr[1] = '-';
        } else if (ptr[1] == 'B') {
            ptr[0] = ' ';
            ptr[1] = 'Z';
        }
    }
}

void SJCutter::readStatus(char* status_info) { 
  u_int button_1=this->getButtonST(BUTTON_POWER)?1:0;
  u_int button_2=this->getButtonST(BUTTON_PAUSE)?1:0;
  u_int button_3=this->getButtonST(BUTTON_PAPER)?1:0;
  u_int button_4=this->getButtonST(BUTTON_START)?1:0;
  int sensor_1=digitalRead(X_LIMIT_PIN);
  int sensor_2=digitalRead(PEN1_END_STOP);
  int sensor_3=digitalRead(PEN2_END_STOP);
  int sensor_4=digitalRead(PIN_LED_PAPER);
  int sensor_5=analogRead(PIN_SEEK_BOX);
  // float* axis_position = system_get_mpos();
  // mpos_to_wpos(axis_position);
  // float unit_conv = 1.0;      // unit conversion multiplier..default is mm
  // if (report_inches->get()) {
  //     unit_conv = 1.0 / MM_PER_INCH;
  // }
  // float x_axis=axis_position[X_AXIS];
  // float y_axis=axis_position[Y_AXIS];
  // float z_axis=axis_position[Z_AXIS];
  // char * state_info=report_state_text();
  // sprintf(status_info, "SI %s|X %4.3f|Y %4.3f|Z %4.3f|P %u%u%u%u|SV %u%u%u%u|LV %u\r\n",state_info, x_axis*unit_conv,y_axis*unit_conv,z_axis*unit_conv,button_1,button_2,button_3,button_4,sensor_1,sensor_2,sensor_3,sensor_4,sensor_5);
  // int tValue = analogRead(PIN_MENU_KEY);
  sprintf(status_info, "BV:%u%u%u%u|SV:%u%u%u%u|LV:%u|MKV:%u",button_1,button_2,button_3,button_4,sensor_1,sensor_2,sensor_3,sensor_4,sensor_5,last_btn_val);
  last_btn_val=5000;
  

}

void SJCutter::exchangeMateLoadStatus(uint8_t client) {
  // 任务运行状态操作无效  
  if(this->taskRunStatus){
    return;
  }
  // 变更加载状态  
  if(this->mateLoadStatus){    
    this->mateLoadStatus=false;    
    SJMotorCtrl::unLoadMate(client);
  }else{
    this->mateLoadStatus=true;
    SJMotorCtrl::loadMate(client);   
  }
  // 变更LED状态 
  this->setCtrlLedAction(BUTTON_PAPER,this->mateLoadStatus?LEDActions::LED_ON:LEDActions::LED_OFF);
  // 变更按钮状态 
  this->setButtonST(BUTTON_PAPER,this->mateLoadStatus);
}


void SJCutter::exchangeTaskPausedStatus(uint8_t client) {   
   // 非任务运行状态操作无效  
  if(!this->taskRunStatus){
    return;
  }
  // 变更暂停状态  
  if(this->taskPausedStatus){
    this->taskPausedStatus=false; 
  }else{
    this->taskPausedStatus=true; 
  }
  // 变更LED状态 
  this->setCtrlLedAction(BUTTON_PAUSE,this->taskPausedStatus?LEDActions::LED_ON:LEDActions::LED_OFF);
  // 变更按钮状态 
  this->setButtonST(BUTTON_PAUSE,this->taskPausedStatus);   
}
