// Copyright (c) 2022 -	Neil
#ifndef __SJMOTORCTRL_HPP__
#define __SJMOTORCTRL_HPP__

#include <../../grp.h>


struct _ValRange {		
	float min;
	float max;
    float safe;
};


class SJMotorCtrl {
public:
    static void init();
    static void loop();   
    static bool zAxisHoming(uint8_t client);
    static bool yAxisHoming(uint8_t client);
    static bool xyAxisHoming(uint8_t client, bool vMiddle);
    static void enableMotor(void);
    static void disableMotor(void);
    static void runTest(uint8_t client);
    static bool loadMate(uint8_t client);
    static bool unLoadMate(uint8_t client);
    static bool resetAxis(uint8_t client,bool reset);
    static bool absMoveAxis(uint8_t client, uint axis, float moveDist);
    static bool relMoveAxis(uint8_t client, uint axis, float moveDist);
    static bool moveAndResetYAxis(uint8_t client,float y_move_dist);
    static bool moveYAxis(uint8_t client,float y_move_dist);
    static void executeCommand(uint8_t client,const char *);

    static int SeekPointByXY(float x, float y);

    static void SeekActionStart(int speed);

    static void SeekActionEnd();

    static int TestFindPointByRange(float maxval,int dirval,int speedval);

};

#endif