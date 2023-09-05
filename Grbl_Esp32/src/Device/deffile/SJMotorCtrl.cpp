// Copyright (c) 2022 -	Neil
#include "../../Grbl.h"
#include "../SJMotorCtrl.h"
#include "../Common.h"
#include "AccelStepper.h"
#include "MultiStepper.h"
#include "FS.h"
#include "SPIFFS.h"
#include "../../SettingsDefinitions.h"

// extern portMUX_TYPE myMutex;
extern parser_state_t gc_state;
extern parser_block_t gc_block;

extern FloatSetting* sj_cfg_l_chn_x_offset;
extern FloatSetting* sj_cfg_l_chn_Y_offset;
extern FloatSetting* sj_cfg_r_chn_X_offset;
extern FloatSetting* sj_cfg_r_chn_y_offset;
extern FloatSetting* sj_cfg_sps_x_offset;
extern FloatSetting* sj_cfg_sps_y_offset;

int   lastDirX       = 0;
int   lastDirY       = 0;
long  laserSteps     = 0;
float posY           = 0;
bool  lasering       = false;
float distance       = 0;
float loadMateLength = 50;  //加载物料的长度

float spacer         = 0.075;  //0.064;
float backDistance   = 0.3;
float bankDistance   = 2;
float accleDistance  = 12;
int   g_MaxSpeed     = 3000;  //90000;
int   g_MaxAccel     = 5000;  //120000;
int   g_SafeMode     = 0;
bool  g_PauseAndStop = 0;
int  z_homing_try_count = 0;

long toStepX(float mm) {
    float steps_per_mm = axis_settings[X_AXIS]->steps_per_mm->get();
    return steps_per_mm * mm;
}

float toMMX(long vSteps) {
    float steps_per_mm = axis_settings[X_AXIS]->steps_per_mm->get();
    return vSteps / steps_per_mm;
}

long toStepY(float mm) {
    float steps_per_mm = axis_settings[Y_AXIS]->steps_per_mm->get();
    return steps_per_mm * mm;
}

float toMMY(long vSteps) {
    float steps_per_mm = axis_settings[Y_AXIS]->steps_per_mm->get();
    return vSteps / steps_per_mm;
}

long toStepZ(float mm) {
    float steps_per_mm = axis_settings[Z_AXIS]->steps_per_mm->get();
    return steps_per_mm * mm;
}

float toMMZ(long vSteps) {
    float steps_per_mm = axis_settings[Z_AXIS]->steps_per_mm->get();
    return vSteps / steps_per_mm;
}

int dirs = 0;

void forwardX(void) {
    dirs &= ~(bit(X_AXIS));

    motors_direction(dirs);
    motors_step(bit(X_AXIS));
}

void backwardX(void) {
    dirs |= bit(X_AXIS);

    motors_direction(dirs);
    motors_step(bit(X_AXIS));
}

void forwardY(void) {
    dirs &= ~(bit(Y_AXIS));

    motors_direction(dirs);
    motors_step(bit(Y_AXIS));
}

void backwardY(void) {
    dirs |= bit(Y_AXIS);

    motors_direction(dirs);
    motors_step(bit(Y_AXIS));
}

void forwardZ(void) {
    dirs |= bit(Z_AXIS);

    motors_direction(dirs);
    motors_step(bit(Z_AXIS));
}

void backwardZ(void) {
    dirs &= ~(bit(Z_AXIS));

    motors_direction(dirs);
    motors_step(bit(Z_AXIS));
}
//////////////////////////////////////////////////////////////////
AccelStepper motorX(forwardX, backwardX);
AccelStepper motorY(forwardY, backwardY);
AccelStepper motorZ(forwardZ, backwardZ);

//////////////////////////////////////////////////////////////////

void SJMotorCtrl::init() {
    motorX.setAcceleration(5000);
    motorX.setSpeed(0);
    motorX.setCurrentPosition(0);
    motorX.setMaxSpeed(3000);

    motorZ.setAcceleration(toStepZ(500));
    motorZ.setSpeed(0);
    motorZ.setCurrentPosition(0);
    motorZ.setMaxSpeed(toStepZ(500 / 60));
}

void SJMotorCtrl::loop() {}

bool SJMotorCtrl::yAxisHoming(uint8_t client) {
    //初始Y轴
    motorY.setAcceleration(toStepY(3000));
    motorY.setSpeed(0);
    motorY.setMaxSpeed(toStepY(3000 / 60)); 
    motorY.runToNewPosition(-(motorY.currentPosition()));
 
}
bool SJMotorCtrl::zAxisHoming(uint8_t client) {
    if(z_homing_try_count>2){
        z_homing_try_count=0;
        return false;
    }
    while (true)
    {
        if((sys.state!=State::Homing&&sys.state!=State::Jog)){
            break;
        }
        yield();
    }
    
      sys.state = State::Homing;
#if defined(PEN1_END_STOP) && defined(PEN2_END_STOP)
    //假设电机负方向是往左转
    //下笔最深5mm

    //如果两个限位都没有碰到，属于异常情况
    if (digitalRead(PEN1_END_STOP) != PEN_TOUCH && digitalRead(PEN2_END_STOP) != PEN_TOUCH) {
        grbl_sendf(CLIENT_ALL, "err:pen check 0\r\n");
        sys.state = State::Idle;  //Alarm
        z_homing_try_count++;
        SJMotorCtrl::zAxisHoming(client);
        return false;
    }

    motors_unstep();
    motors_set_disable(false);

    //如果左边限位没有碰到，电机往右转，抬起左边压轮
    motorZ.moveTo(toStepZ(10));
    while (motorZ.distanceToGo()) {
        if (digitalRead(PEN1_END_STOP) == PEN_TOUCH)
            break;

        motorZ.run();
        yield();
    }

    //如果转动完，左边限位还没碰到，属于异常情况
    if (motorZ.distanceToGo() == 0) {
        motors_unstep();
        motors_set_disable(true);
        grbl_sendf(CLIENT_ALL, "err:pen check 1\r\n");
        sys.state = State::Idle;  //Alarm
        z_homing_try_count++;
        SJMotorCtrl::zAxisHoming(client);
        return false;
    }

    //如果右边限位没有碰到，电机往左转，抬起右边压轮
    motorZ.moveTo(toStepZ(-10));
    while (motorZ.distanceToGo()) {
        if (digitalRead(PEN2_END_STOP) == PEN_TOUCH)
            break;

        motorZ.run();
        yield();
    }

    //如果转动完，右边限位还没碰到，属于异常情况
    if (motorZ.distanceToGo() == 0) {
        motors_unstep();
        motors_set_disable(true);
        grbl_sendf(CLIENT_ALL, "err:pen check 2\r\n");
        sys.state = State::Idle;  //Alarm
        z_homing_try_count++;
        SJMotorCtrl::zAxisHoming(client);
        return false;
    }

    //这时，应该左右限位都碰到
    if (digitalRead(PEN1_END_STOP) != PEN_TOUCH || digitalRead(PEN2_END_STOP) != PEN_TOUCH) {
        motors_unstep();
        motors_set_disable(true);
        grbl_sendf(CLIENT_ALL, "err:pen check 3\r\n");
        sys.state = State::Idle;  //Alarm
        z_homing_try_count++;
        SJMotorCtrl::zAxisHoming(client);
        return false;
    }

    //粗略设置当前坐标为0
    motorZ.setCurrentPosition(0);

    //压左边轮子，使左边笔离开限位开关
    motorZ.moveTo(toStepZ(-10));
    while (motorZ.distanceToGo()) {
        if (digitalRead(PEN1_END_STOP) != PEN_TOUCH)
            break;

        motorZ.run();
        yield();
    }

    //测得需要下压的距离
    float tPosLeft = toMMZ(motorZ.currentPosition());

    //右转压右边轮子，使右边笔离开限位开关
    motorZ.moveTo(toStepZ(10));
    while (motorZ.distanceToGo()) {
        if (digitalRead(PEN2_END_STOP) != PEN_TOUCH)
            break;

        motorZ.run();
        yield();
    }

    //测得需要下压的距离
    float tPosRight = toMMZ(motorZ.currentPosition());

    //取得中间点
    float tPosMid = (tPosLeft + tPosRight) / 2;
    motorZ.moveTo(toStepZ(tPosMid));
    while (motorZ.distanceToGo()) {
        motorZ.run();
        yield();
    }
 
    motors_unstep();
    motors_set_disable(true);
    motorZ.setCurrentPosition(0); 

    //设置Z坐标为0
    // gc_block.values.xyz[Z_AXIS] = 0;
    // memcpy(gc_state.coord_offset, gc_block.values.xyz, sizeof(gc_block.values.xyz));
    // system_flag_wco_change();

    SJMotorCtrl::executeCommand(client,"G10 P0 L20 Z0");

    //额外计算左右下压的空行程（数值为正)
    float tBankDistance = (tPosRight - tPosLeft) / 2;
    grbl_sendf(CLIENT_ALL, "pen home ok bank %0.2f\r\n", tBankDistance);
#endif
    sys.state = State::Idle;  //Alarm
    return true;
}

bool SJMotorCtrl::xyAxisHoming(uint8_t client, bool vMiddle) {

    SJMotorCtrl::executeCommand(client,"$HX");

    while (true)
    {
        if((sys.state!=State::Homing&&sys.state!=State::Jog)){
            break;
        }
        yield();
    }


    sys.state = State::Homing;

    SJMotorCtrl::enableMotor();
    motorX.setAcceleration(toStepX(2000));
    motorX.setSpeed(0);
    motorX.runToNewPosition(toStepX(0));
    motorX.setMaxSpeed(toStepX(2000 / 60));

    

    //向右移，直到小车离开限位
    motorX.moveTo(toStepX(30));
    while (motorX.distanceToGo()) {
        if (digitalRead(X_LIMIT_PIN) != LIMIT_TOUCH)
            break;

        motorX.run();
        yield();
    }

    //如果转动完，限位还没离开，属于异常情况
    if (motorX.distanceToGo() == 0) {
        motors_unstep();
        motors_set_disable(true);
        grbl_sendf(CLIENT_ALL, "error:1000\r\n");

        sys.state = State::Idle;  //Alarm
        return false;
    }

    //停下
    motorX.stop();
    motorX.runToPosition();

    //往限位靠近
    motorX.setCurrentPosition(0);
    motorX.moveTo(toStepX(-350));

    //直到碰到限位
    while (motorX.distanceToGo()) {
        if (digitalRead(X_LIMIT_PIN) == LIMIT_TOUCH)
            break;

        motorX.run();
        yield();
    }

    //如果转动完，限位还没碰到，属于异常情况
    if (motorX.distanceToGo() == 0) {
        motors_unstep();
        motors_set_disable(true);
        grbl_sendf(CLIENT_ALL, "error:1001\r\n");
        sys.state = State::Idle;  //Alarm
        return false;
    }

    //远离一点限位
    motorX.setCurrentPosition(0);
    motorX.moveTo(toStepX(3));

    while (motorX.distanceToGo()) {
        motorX.run();
        yield();
    }

    //motors_unstep();
    //motors_set_disable(true);
    motorX.setCurrentPosition(0);
    motorY.setCurrentPosition(0);

    if (vMiddle) {
        // motorX.moveTo(toStepX(150));
        // while (motorX.distanceToGo()) {
        //     motorX.run();
        //     yield();
        // }


        //设置XY坐标为0
        // gc_block.values.xyz[X_AXIS] = -150;
        // gc_block.values.xyz[Y_AXIS] = 0;
    } else {
        // gc_block.values.xyz[X_AXIS] = 0;
        // gc_block.values.xyz[Y_AXIS] = 0;
    }
    // gc_block.values.xyz[X_AXIS] = 0;
    // gc_block.values.xyz[Y_AXIS] = 0;
    // memcpy(gc_state.coord_system, gc_block.values.xyz, sizeof(gc_block.values.xyz));


    // gc_state.position[X_AXIS] = 0;
    // gc_state.position[Y_AXIS] = 0;
    // SJMotorCtrl::executeCommand(client,"G10 P0 L20 X0Y0");
    // SJMotorCtrl::executeCommand(client,"G10 P1 L20 X0Y0");
    //gc_state.coord_system[idx] + gc_state.coord_offset[idx];
    //机械坐标清零
    // gc_state.coord_system[X_AXIS] = 0;
    // gc_state.coord_system[Y_AXIS] = 0;

    // SJMotorCtrl::executeCommand(client,"G10 P0 L20 X0Y0");
 
    system_flag_wco_change();

    grbl_sendf(CLIENT_ALL, "[MSG]HOME OK\r\n");

    sys.state = State::Idle;

    SJMotorCtrl::disableMotor();

    return true;
}

void SJMotorCtrl::enableMotor(void) {
    motors_unstep();
    motors_set_disable(false);
}

void SJMotorCtrl::disableMotor(void) {
    motors_unstep();
    motors_set_disable(true);
}

void SJMotorCtrl::runTest(uint8_t client) {
    // 任务运行状态操作无效
    if (SJCutterIns.taskRunStatus) {
        return;
    }

    SJCutterIns.testMode = true;
    SJMotorCtrl::zAxisHoming(client);

    SJMotorCtrl::xyAxisHoming(client, false);

    SJMotorCtrl::enableMotor();

    motorX.setAcceleration(toStepX(2000));
    motorX.setSpeed(0);
    motorX.setCurrentPosition(0);
    motorX.setMaxSpeed(toStepX(4000 / 60));

    motorY.setAcceleration(toStepY(2000));
    motorY.setSpeed(0);
    motorY.setCurrentPosition(0);
    motorY.setMaxSpeed(toStepY(4000 / 60));

    motorZ.setAcceleration(toStepZ(2000));
    motorZ.setSpeed(0);
    motorZ.setCurrentPosition(0);
    motorZ.setMaxSpeed(toStepZ(4000 / 60));

    while (SJCutterIns.testMode) {
        // 中止测试
        int tValue = analogRead(PIN_MENU_KEY);
        if (tValue < 3000) {
            break;
        }

        motorX.run();
        motorY.run();
        motorZ.run();

        if (motorX.distanceToGo() == 0)
            if (motorX.currentPosition() == 0)
                motorX.moveTo(toStepX(300));
            else
                motorX.moveTo(toStepX(0));

        if (motorY.distanceToGo() == 0)
            if (motorY.currentPosition() == 0)
                motorY.moveTo(toStepY(300));
            else
                motorY.moveTo(toStepY(0));

        if (motorZ.distanceToGo() == 0)
            if (motorZ.currentPosition() < 0)
                motorZ.moveTo(toStepZ(2.5));
            else
                motorZ.moveTo(toStepZ(-2.5));

        yield();
    }

    SJMotorCtrl::disableMotor();

     SJCutterIns.testMode = false;

    SJMotorCtrl::zAxisHoming(client);
    SJMotorCtrl::resetAxis(client,true);
}

bool SJMotorCtrl::loadMate(uint8_t client) {
    return SJMotorCtrl::absMoveAxis(client, Y_AXIS, 0);
};
bool SJMotorCtrl::unLoadMate(uint8_t client) {
    float mate_dist = sj_cfg_y_mate_dist->get();
    return SJMotorCtrl::absMoveAxis(client, Y_AXIS, mate_dist);
}
bool SJMotorCtrl::resetAxis(uint8_t client,bool reset) {
    char cmdLine[64];
    

    // X轴归位
    memset(cmdLine,0,64);
    sprintf(cmdLine, "$HX");
    execute_line(cmdLine, client, WebUI::AuthenticationLevel::LEVEL_GUEST);

    // X轴移动到中间
    float x_pulloff_dist = sj_cfg_x_pulloff_dist->get();  
    float homing_pulloff_dist = homing_pulloff->get();  

    if(reset){
        // 重置Y轴0位；
        memset(cmdLine,0,64);
        sprintf(cmdLine, "G10 P0 L20 X0 Y0 Z0");
        execute_line(cmdLine, client, WebUI::AuthenticationLevel::LEVEL_GUEST);
    }
 

    // 移动到中间
    memset(cmdLine,0,64);
    sprintf(cmdLine, "G0 X%3.2f Y0 F5000",x_pulloff_dist);
    // sprintf(cmdLine, "G0 X153 Y0 F5000");
    // grbl_sendf(client,"%s\n\0", cmdLine);
    execute_line(cmdLine, client, WebUI::AuthenticationLevel::LEVEL_GUEST);
}

bool SJMotorCtrl::moveAndResetYAxis(uint8_t client,float y_move_dist) {
    char cmdLine[64];

    memset(cmdLine,0,64);
    sprintf(cmdLine, "$J=G90G21Y%3.2fF5000",y_move_dist);
    execute_line(cmdLine, client, WebUI::AuthenticationLevel::LEVEL_GUEST);

      // 重置Y轴0位；
    memset(cmdLine,0,64);
    sprintf(cmdLine, "G10 P0 L20 Y0",0);
    execute_line(cmdLine, client, WebUI::AuthenticationLevel::LEVEL_GUEST);
}

bool SJMotorCtrl::moveYAxis(uint8_t client,float y_move_dist) {
    char cmdLine[64];

    sys.state = State::Homing;  //Alarm
    //电机准备
    motors_unstep();
    motors_set_disable(false);
    int speed = 6000;
    
    //初始Y轴
    motorY.setAcceleration(toStepY(speed));
    motorY.setSpeed(0);
    motorY.setMaxSpeed(toStepY(speed / 60));

    // 回到原位
    // motorY.runToNewPosition(toStepY(y_move_dist));
    motorY.moveTo(toStepY(y_move_dist));
    while (motorY.distanceToGo()) { 
        motorY.run();
        yield();
    }


    motorY.setCurrentPosition(0); 

        //设置Z坐标为0
    gc_block.values.xyz[Y_AXIS] = 0;
    memcpy(gc_state.coord_offset, gc_block.values.xyz, sizeof(gc_block.values.xyz));
    system_flag_wco_change();

    //释放电机
    motors_unstep();
    motors_set_disable(true);

    sys.state = State::Idle;  //Alarm
}



bool SJMotorCtrl::absMoveAxis(uint8_t client, uint axis, float moveDist) {
    char jogLine[LINE_BUFFER_SIZE];
    sprintf(jogLine, "$J=G90G21%s%3.2fF5000", axis == 0 ? "X" : "Y", moveDist);
    gc_execute_line(jogLine, client);
    // grbl_sendf(client, "mateLoadStatus:%s\r\n",jogLine);
    return true;
};
bool SJMotorCtrl::relMoveAxis(uint8_t client, uint axis, float moveDist) {
    char jogLine[LINE_BUFFER_SIZE];
    sprintf(jogLine, "$J=G91G21%s%3.2fF5000", axis == 0 ? "X" : "Y", moveDist);
    // grbl_send(client, jogLine);
    gc_execute_line(jogLine, client);
    return true;
};


void SJMotorCtrl::executeCommand(uint8_t client,const char *cmd) {
    char cmdLine[64];
    // X轴归位
    memset(cmdLine,0,64);
    sprintf(cmdLine, "%s",cmd);
    execute_line(cmdLine, client, WebUI::AuthenticationLevel::LEVEL_GUEST);     
}

void SJMotorCtrl::SeekActionStart(int speed) {
     //电机准备
    motors_unstep();
    motors_set_disable(false);
    //初始X轴
    motorX.setAcceleration(toStepX(speed));
    motorX.setSpeed(0);
    motorX.setMaxSpeed(toStepX(speed / 60));

    //初始Y轴
    motorY.setAcceleration(toStepY(speed));
    motorY.setSpeed(0);
    motorY.setMaxSpeed(toStepY(speed / 60));

    // 回到原位
    motorX.runToNewPosition(toStepX(0));
    motorY.runToNewPosition(toStepY(0));  

}

void SJMotorCtrl::SeekActionEnd() {
    // 回到原位
    motorX.runToNewPosition(toStepX(0));
    motorY.runToNewPosition(toStepY(0));  
     //释放电机
    motors_unstep();
    motors_set_disable(true);    
}


int SJMotorCtrl::SeekPointByXY(float x, float y) {  
    // x坐标最大值
    float maxXAxis = 345; 
#if defined(PIN_SEEK_BOX) 
    // 当超取识别范围直接判断为识别异常
    if (x <= maxXAxis) {
        // 移动电机到指定位置
        motorX.runToNewPosition(toStepX(x));
        motorY.runToNewPosition(toStepY(y));
        //获取寻边传感器值
        return  analogRead(PIN_SEEK_BOX); 
    } else {
        return 0;
    }
#else
    return 0;
#endif
}



 
int SJMotorCtrl::TestFindPointByRange(float maxval,int dirval,int speedval){
 
    //电机准备
    motors_unstep();
    motors_set_disable(false); 

    //初始X轴
    motorX.setAcceleration(toStepX(speedval));
    motorX.setSpeed(0);
    motorX.setMaxSpeed(toStepX(speedval / 60));

    //初始Y轴
    motorY.setAcceleration(toStepY(speedval));
    motorY.setSpeed(0);
    motorY.setMaxSpeed(toStepY(speedval / 60));

    // 取当前位置
    float currPos = (dirval==1)?toMMX(motorX.currentPosition()):toMMY(motorY.currentPosition()); 
    float step=0.05;
    float p= currPos;
    float pmax=currPos+maxval; 
    float *parray = new float[10];
    int pindex=0;
    while(p<=pmax){        
        // 移动到目标位置
        if(dirval==1){           
            motorX.runToNewPosition(toStepX(p));
        }else if(dirval==2){
            motorY.runToNewPosition(toStepY(p));
        }
        
        //获取寻边传感器值
        int psval  = analogRead(PIN_SEEK_BOX);
        //记录值
        parray[pindex]=psval;
        
        
        p=p+step;
        pindex++;
        if(pindex>=10){
            pindex=0;
            //输出
            grbl_sendf(CLIENT_SERIAL, "VAL:%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,\r\n", parray[0], parray[1], parray[2], parray[3], parray[4], parray[5], parray[6], parray[7], parray[8], parray[9]);
        }
         yield();
    } 
    
    //释放电机
    motors_unstep();
    motors_set_disable(true);

    return 0;
}
