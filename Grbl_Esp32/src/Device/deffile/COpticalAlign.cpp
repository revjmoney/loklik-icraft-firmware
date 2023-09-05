#include "../COpticalAlign.h"
#include "../CPointQueue.h"

#ifdef DEBUG_SIMULATOR
#   include <cmath>
#   include <chrono>
#   include <thread>
#else
#   include "../../Grbl.h"
#   include "../SJMotorCtrl.h"
#endif


//LT=(A1,A2,An)∩(B1,B2,Bn)
SPointInfo COpticalAlign::cacleP1( ) {
    SPointInfo result={0,0, false};
    /////////////////////////////////////////////////////////////////////////
    //计算A1点
    int A1_x_min=find_start_x_min_val;
    int A1_x_max=find_start_x_max_val;
    int A1_y= find_start_y_min_val + find_start_offset_y;//从2MM位置开始
    SValRange A1_range={A1_x_min,A1_x_max,A1_y,FindDir::L2R};
    SPointInfo A1=this->findMarkByRange(A1_range);
    js_show_point(A1.x,A1.y);
    if(!A1.status)return result;
    //计算A2点
    int A2_x_min=A1.x-range_min_val;
    int A2_x_max=A1.x+range_min_val;
    int A2_y=A1.y+next_range_offset;
    SValRange A2_range={A2_x_min,A2_x_max,A2_y,FindDir::L2R};
    SPointInfo A2=this->findMarkByRange(A2_range);
    js_show_point(A2.x,A2.y);
    if(!A2.status)return result;
    ////////////////////////////////////////////////////////////////////////////
    //计算B1点
    int B1_y_min= find_start_offset_y;
    int B1_y_max=B1_y_min+range_max_val;
    int B1_x=A1.x+range_min_val;//从2MM位置开始
    SValRange B1_range={B1_y_min,B1_y_max,B1_x,FindDir::T2B};
    SPointInfo BP1=this->findMarkByRange(B1_range);
    js_show_point(BP1.x,BP1.y);
    if(!BP1.status)return result;
    //计算B2点
    int B2_y_min=BP1.y-range_min_val;
    int B2_y_max=BP1.y+range_min_val;
    int B2_x=BP1.x+next_range_offset;//从2MM位置开始
    SValRange B2_range={B2_y_min,B2_y_max,B2_x,FindDir::T2B};
    SPointInfo B2=this->findMarkByRange(B2_range);
    js_show_point(B2.x,B2.y);
    if(!B2.status)return result;
    //计算两线交点
    SPointInfo AB=this->cacleIntersect(A1,A2,BP1,B2);
    js_show_point(AB.x,AB.y);
    return AB;
}

//RT=(C1,C2,Cn)∩(D1,D2,Dn)
SPointInfo COpticalAlign::cacleP2( ) {
    SPointInfo result={0,0, false};

    /////////////////////////////////////////////////////////////////////////
    //计算D1点
    int D1_x_min=this->p1.x+marker_width-range_min_val;
    int D1_x_max=D1_x_min+(range_min_val*2);
    int D1_y= this->p1.y +find_start_offset_y+range_min_val;//从2MM位置开始
    SValRange D1_range={D1_x_min,D1_x_max,D1_y,FindDir::R2L};
    SPointInfo D1=this->findMarkByRange(D1_range);
    js_show_point(D1.x,D1.y);
    if(!D1.status)return result;
    ////////////////////////////////////////////////////////////////////////////
    //计算D2点
    int D2_x_min=D1.x-range_min_val;
    int D2_x_max=D1.x+range_min_val;
    int D2_y=D1.y+next_range_offset;
    SValRange D2_range={D2_x_min,D2_x_max,D2_y,FindDir::R2L};
    SPointInfo D2=this->findMarkByRange(D2_range);
    js_show_point(D2.x,D2.y);
    if(!D2.status)return result;

    ////////////////////////////////////////////////////////////////////////////
    //计算C1点
    int C1_y_min= find_start_offset_y + range_min_val;
    int C1_y_max=  C1_y_min+ range_max_val;
    int C1_x=this->p1.x+marker_width-range_min_val;//从2MM位置开始
    SValRange C1_range={C1_y_min,C1_y_max,C1_x,FindDir::T2B};
    SPointInfo C1=this->findMarkByRange(C1_range);
    js_show_point(C1.x,C1.y);
    if(!C1.status)return result;
    ////////////////////////////////////////////////////////////////////////////
    //计算C2点
    int C2_y_min=C1.y-range_min_val;
    int C2_y_max=C1.y+range_min_val;
    int C2_x=C1.x-next_range_offset;//从2MM位置开始
    SValRange C2_range={C2_y_min,C2_y_max,C2_x,FindDir::T2B};
    SPointInfo C2=this->findMarkByRange(C2_range);
    js_show_point(C2.x,C2.y);
    if(!C2.status)return result;
    ////////////////////////////////////////////////////////////////////////////
    //计算两线交点
    SPointInfo CD=this->cacleIntersect(D1,D2,C1,C2);
    js_show_point(CD.x,CD.y);
    return CD;
}

//RB=(E1,E2,En)∩(F1,F2,Fn)
SPointInfo COpticalAlign::cacleP3( ) {
    SPointInfo result={0,0, false};
    /////////////////////////////////////////////////////////////////////////
    //计算E1点
    int E1_x_min=this->p2.x-range_max_val;
    int E1_x_max=this->p2.x+(range_min_val*2);
    int E1_y=p2.y+this->marker_height-range_min_val;//从2MM位置开始
    SValRange E1_range={E1_x_min,E1_x_max,E1_y,FindDir::R2L};
    SPointInfo E1=this->findMarkByRange(E1_range);
    js_show_point(E1.x,E1.y);
    if(!E1.status)return result;
    ////////////////////////////////////////////////////////////////////////////
    //计算E2点
    int E2_x_min=E1.x-range_min_val;
    int E2_x_max=E1.x+range_min_val;
    int E2_y=E1.y-next_range_offset;
    SValRange E2_range={E2_x_min,E2_x_max,E2_y,FindDir::R2L};
    SPointInfo E2=this->findMarkByRange(E2_range);
    js_show_point(E2.x,E2.y);
    if(!E2.status)return result;
    ////////////////////////////////////////////////////////////////////////////
    //计算F1点
    int F1_y_min= E1.y;
    int F1_y_max=  F1_y_min+ (range_min_val*2);
    int F1_x=E2.x-range_min_val;//从2MM位置开始
    SValRange F1_range={F1_y_min,F1_y_max,F1_x,FindDir::B2T};
    SPointInfo F1=this->findMarkByRange(F1_range);
    js_show_point(F1.x,F1.y);
    if(!F1.status)return result;
    ////////////////////////////////////////////////////////////////////////////
    //计算F2点
    int F2_y_min=F1.y-range_min_val;
    int F2_y_max=F1.y+range_min_val;
    int F2_x=F1.x-next_range_offset;//从2MM位置开始
    SValRange F2_range={F2_y_min,F2_y_max,F2_x,FindDir::B2T};
    SPointInfo F2=this->findMarkByRange(F2_range);
    js_show_point(F2.x,F2.y);
    if(!F2.status)return result;

    ////////////////////////////////////////////////////////////////////////////
    //计算两线交点
    SPointInfo EF=this->cacleIntersect(E1,E2,F1,F2);
    js_show_point(EF.x,EF.y);

    return EF;

}
//LB=(G1,G2,Gn)∩(H1,H2,Gn)
SPointInfo COpticalAlign::cacleP4( ) {
    SPointInfo result={0,0, false};
    return result;
}

SPointInfo COpticalAlign::cacleIntersect(SPointInfo p1, SPointInfo p2, SPointInfo p3, SPointInfo p4) {
    int x1_1 = p1.x,x1_2 = p2.x,x2_1 = p3.x,x2_2 = p4.x;
    int y1_1 = p1.y,y1_2 = p2.y,y2_1 = p3.y,y2_2 = p4.y;
    if ((x1_1 - x1_2) == 0 || (x2_1 - x2_2) == 0){
        return { (int)x1_1, (int)y2_1 , true };
    }
    else{
        float k1 = (y1_1 - y1_2)/(x1_1 - x1_2);
        float k2 = (y2_1 - y2_2)/(x2_1 - x2_2);
        float b1 = y1_1 - x1_1*k1;
        float b2 = y2_1 - x2_1*k2;
        int pointx = (b2 - b1)/(k1 - k2);
        int pointy = k1*pointx + b1;
        return { (int)pointx, (int)pointy , true };
    }
}

float COpticalAlign::cacleAngle(SPointInfo p1, SPointInfo p2, SPointInfo p3) {
    int x1 = (p1.x - p3.x);
    int y1 = (p1.y - p3.y);
    int x2 = (p2.x - p3.x);
    int y2 = (p2.y - p3.y);
    float dot = (x1 * x2 + y1 * y2);
    float det = (x1 * y2 - y1 * x2);
    float angle =atan2(det, dot) / PI*180;
    float ang = fmod(angle, 360.0);
    if (ang < 0.0)
        ang = 360.0 + ang;
    return ang;
}


SPointInfo COpticalAlign::findMarkByRange(SValRange range){
    SPointInfo result={0,0,false};
    int p=-1;
    float *parray = new float[10];
    int pindex=0;
    // 光传感器容差范围,分为四个区间0:传感器异常，1:白纸，2:垫板，3:标记物
    int tolerance_range[4][2] = { { 0, 1 }, { 1, 2500 }, { 2500, 3000 }, { 3200, 5500 } };
    // 步长
    int step =5;
    // 点队列
    CPointQueue pointQueue;
    while(true){
        SPointInfo point={0,0,true};
        // 初始化和递进
        if(range.dir==FindDir::L2R||range.dir==FindDir::T2B){
            if(p==-1)p=range.min;
            p=p+step;
            if(p>range.max) break;
        }else if(range.dir==FindDir::R2L||range.dir==FindDir::B2T){
            if(p==-1)p=range.max;
            p=p-step;
            if(p<range.min||p<0) break;
        }

        // 判断变化量是X或Y
        if(range.dir==FindDir::L2R||range.dir==FindDir::R2L){
            point={p,range.safe,true};
        }else if(range.dir==FindDir::T2B||range.dir==FindDir::B2T){
            point={range.safe,p,true};
        }

        int psval  =0;
#ifdef DEBUG_SIMULATOR
        //获取寻边传感器值
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        psval  = js_seek_marker(point.x,point.y);
#else
        float asix_x=((sensor_offset_x+point.x)*0.01)+0.5;
        float asix_y=((sensor_offset_y+point.y)*0.01)+0.5;
        psval  = SJMotorCtrl::SeekPointByXY(asix_x,asix_y);
        //psval
        // grbl_sendf(CLIENT_SERIAL, "SeekPointByXY:%f,%f,%d\r\n", asix_x,asix_y,psval); 

        //记录值
        parray[pindex]=psval; 
        pindex++;
        if(pindex>=10){
            pindex=0;
            //输出
            grbl_sendf(CLIENT_SERIAL, "VAL:%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,\r\n", parray[0], parray[1], parray[2], parray[3], parray[4], parray[5], parray[6], parray[7], parray[8], parray[9]);
        }

#endif

        //据传感器获取的灰度值转换为识别内容标记
        FindRecType rec_tag = FindRecType::NoThing;
        if (psval >= tolerance_range[0][0] && psval < tolerance_range[0][1]) {
            rec_tag = FindRecType::NoThing;
        } else if (psval >= tolerance_range[1][0] && psval < tolerance_range[1][1]) {
            rec_tag = FindRecType::Paper;
        } else if (psval >= tolerance_range[2][0] && psval < tolerance_range[2][1]) {
            rec_tag = FindRecType::Plate;
        } else if (psval >= tolerance_range[3][0] && psval < tolerance_range[3][1]) {
            rec_tag = FindRecType::Marker;
        }
        //将识别内容位置记录到队列
        pointQueue.push(point,rec_tag);
        //判断队列状态提前中止
        if(pointQueue.isFulled()){
            break;
        }
    }
    if(pointQueue.isFulled()){
        result=pointQueue.front();
        result.status=true;
    }
    pointQueue.clear();
    return result;
}


void COpticalAlign::reviseOffset(int taskindex) { 
    SAlignResult result={0,0,0,0,0,false};
    #ifndef DEBUG_SIMULATOR
    SJMotorCtrl::SeekActionStart(5000);
    #endif
    find_start_offset_y=0;
    SPointInfo p1=this->cacleP1();
    if(!p1.status){
        // 向下偏移
        find_start_offset_y+=1000;
        p1=this->cacleP1();
    }
    if(p1.status){
        this->p1=p1;
        find_start_offset_y=0;
        SPointInfo p2=this->cacleP2();
        if(!p2.status){
            // 向下偏移
            find_start_offset_y+=1000;
            p2=this->cacleP2();
        }
        if(p2.status){
            this->p2=p2;
            SPointInfo p3=this->cacleP3();
            if(p3.status){
                this->p3=p3;
                ////////////////////////////////////////////////////////////////////////////
                SPointInfo vp2;
                SPointInfo vp3;
                float multiplying_x = sqrt((p1.x -p2.x)*(p1.x -p2.x)+(p1.y -p2.y)*(p1.y -p2.y)) / marker_width;
                float multiplying_y = sqrt((p2.x -p3.x)*(p2.x -p3.x)+(p2.y -p3.y)*(p2.y -p3.y)) / marker_height;
                vp2.x = p1.x + marker_width * multiplying_x;
                vp2.y = p1.y;
                vp3.x = p1.x + marker_width * multiplying_x;
                vp3.y = p1.y + marker_height * multiplying_y;
                float angle1 = cacleAngle(vp2, p2, p1);
                float angle2 = cacleAngle(vp3, p3, p1);
                float offset_x=(p1.x-marker_offset_x)*0.01;//s to mm
                float offset_y=(p1.y-marker_offset_y)*0.01;
                
                result={offset_x,offset_y,angle2, multiplying_x,multiplying_y,true};
                             
#ifdef DEBUG_SIMULATOR
                // printf("offset:%d,%d,%f\n",offset_x,offset_y,angle2);   
                js_show_point(offset_x,offset_y);
                js_show_overlayer(p1.x,p1.y,(marker_width*multiplying_x ),(marker_height*multiplying_y),angle2); 
#endif
            }

        }
    }
#ifndef DEBUG_SIMULATOR
    SJMotorCtrl::SeekActionEnd();
    SJMotorCtrl::executeCommand(CLIENT_ALL,"$J=G90G21X150Y0F5000");
    if(!result.status){
        grbl_sendf(CLIENT_ALL,"SEEK: %d,%f,%f,%f,1,1\r\n",-1,0,0,0);
    }else{
        grbl_sendf(CLIENT_ALL,"SEEK: %d,%0.5f,%0.5f,%0.5f,%0.5f,%0.5f\r\n", taskindex,result.offset_x,result.offset_y,result.angle, result.scale_x,result.scale_y);
    }    
#endif
}
