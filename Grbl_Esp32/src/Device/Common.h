#pragma once
#ifndef DEBUG_SIMULATOR
    // #define DEBUG_SIMULATOR 1;
#endif

#ifdef DEBUG_SIMULATOR
#include <stdbool.h>
#include <stdio.h>
const float PI=3.1415926;
#else 
#include <cstdint> 
#endif


// 圆周率

const float UNIT_CONVERT_IN_TO_MM = 25.4;
// 点信息
struct SPointInfo {
	int x;
	int y;
	bool status;
} ;
// 两点信息
struct STwoPointInfo {
	int x;
	int y;
	int x2;
	int y2;
	bool status;
};

struct SAlignResult {
	float offset_x;
	float offset_y;
	float angle;
	float scale_x;
	float scale_y;
	bool status;
} ;

// 查找方向
enum FindDir { L2R = 1, R2L, T2B, B2T };
// 查找识别内容类型
enum FindRecType { NoThing = 0, Paper, Plate, Marker };
// 查找范围 单位为丝
struct SValRange {
    int min;
    int max;
    int safe;
    FindDir dir;
};




#ifdef DEBUG_SIMULATOR

int min(int, int);
int max(int, int);

#ifndef EM_PORT_API
#	if defined(__EMSCRIPTEN__)
#		include <emscripten.h>
#		if defined(__cplusplus)
#			define EM_PORT_API(rettype) extern "C" rettype EMSCRIPTEN_KEEPALIVE
#		else
#			define EM_PORT_API(rettype) rettype EMSCRIPTEN_KEEPALIVE
#		endif
#	else
#		if defined(__cplusplus)
#			define EM_PORT_API(rettype) extern "C" rettype
#		else
#			define EM_PORT_API(rettype) rettype
#		endif
#	endif
#endif



//JS注入方法
EM_PORT_API (void) js_print(char *str);
EM_PORT_API (int) js_seek_marker(int x,int y);
EM_PORT_API (int) js_show_point(int x,int y);
EM_PORT_API (int) js_show_overlayer(int x,int y,int w,int h,float a);

//CPP导出方法
EM_PORT_API(void) next_location();
#else
int js_show_point(int x,int y);
#endif