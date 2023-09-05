
#include "../CPointQueue.h"



CPointQueue::CPointQueue() {
    minSize =5;
    maxSize = 50;
    allowNoise =2;
    paperCount = 0;
    markerCount = 0;
    noiseCount = 0;
    plateCount = 0;
}

void CPointQueue::push(SPointInfo point, FindRecType tag) {

    if (tag == FindRecType::Marker) {
        markerCount++;
        noiseCount=0;
        plateCount = 0;
         //小于队列最大长度时
        _queue.push(point);
    }else{
        // 增加噪音信号计数
        noiseCount++;
        if (tag == FindRecType::Paper) {
            // 出现白纸信号
            paperCount++;
        }else{

            if(tag == FindRecType::Plate)
            {
                // 出现垫板
                plateCount++;
            }
        }
        // 如果其它干扰标记大于阀值，则清空队列
        if (noiseCount >= allowNoise&&(_queue.size() < this->minSize)) {
            this->clear();
        }
    }
}

SPointInfo CPointQueue::front() {
    return _queue.front();
}

bool CPointQueue::isFulled() {
//    printf("paperCount:%d,%d\r\n",paperCount,_queue.size());
    return (paperCount>30&&_queue.size() >= this->minSize);
}

CPointQueue::~CPointQueue() {
    _queue.empty();
    markerCount = 0;
    noiseCount = 0;
    paperCount = 0;
}
void CPointQueue::clear() {
    _queue.empty();
    markerCount = 0;
    noiseCount = 0;
}



