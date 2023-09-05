#include "../Common.h"
#include "../COpticalAlign.h"


 
#ifdef DEBUG_SIMULATOR
int min(int a, int b) {
	return a < b ? a : b;
}
int max(int a, int b) {
	return a > b ? a : b;
}

COpticalAlign opticalAlign;

//CPP导出方法
EM_PORT_API(void) next_location() {
    opticalAlign.reviseOffset(1);
}
#else
#include "../../Grbl.h"
int js_show_point(int x,int y){
     grbl_sendf(CLIENT_SERIAL, "js_show_point:%d,%d\r\n", x,y); 
    return 0;
}
#endif