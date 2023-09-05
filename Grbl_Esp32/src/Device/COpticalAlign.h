#pragma once
#include "./Common.h"
#include <cstdint> 

class COpticalAlign {
private:

    // 统一单位为丝，1cm=10mm=1000s
    const int line_width= 100;//丝
    // 开始查找位置
    const int find_start_x_min_val= 0;//丝
    const int find_start_x_max_val= 6000;// x-500:x+3000
    const int find_start_y_min_val= 2000;//丝
    int find_start_offset_y= 0;//丝
    // 搜索两点延长到第三点的长度
    const int extend_line_length= 2000;//丝
    // 同边两点搜索相隔间距
    const int next_range_offset= 500;//
    // 搜索开始起始位置
    const int range_min_val= 500;//x-500
    // 最大搜索范围
    const int range_max_val= 3000;// x-500:x+3000
    //标记范围宽度
    const int marker_width=18800; // 单位丝
    //标记范围高度
    const int marker_height=24700; // 单位丝
    //标记P1.X偏移
    const int marker_offset_x=1150; // 单位丝
    //标记P1.Y偏移
    const int marker_offset_y=1250; // 单位丝
    //传感器X偏移
    const int sensor_offset_x=1380; // 单位丝35.26
    //传感器Y偏移
    const int sensor_offset_y=3480; // 单位丝
    //搜索速度
    const int find_speed=5000; // 查找速度
    //重试时的偏移量
    const int try_again_offset=1000;  


    SPointInfo p1;//LT=(A1,A2,An)∩(B1,B2,Bn)
    SPointInfo p2;//RT=(C1,C2,Cn)∩(D1,D2,Dn)
    SPointInfo p3;//RB=(E1,E2,En)∩(F1,F2,Fn)
    SPointInfo p4;//LB=(G1,G2,Gn)∩(H1,H2,Gn)
public:
    SPointInfo cacleP1();
    SPointInfo cacleP2();
    SPointInfo cacleP3();
    SPointInfo cacleP4();
    void reviseOffset(int taskindex);
    SPointInfo findMarkByRange(SValRange range);
    static SPointInfo cacleIntersect(SPointInfo p1, SPointInfo p2, SPointInfo p3, SPointInfo p4);
    static float cacleAngle(SPointInfo p1, SPointInfo p2, SPointInfo p3);
    int TestSeekPointByXY(float x, float y);
};