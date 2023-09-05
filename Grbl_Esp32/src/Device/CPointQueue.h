#pragma once
#ifndef POINTQUEUE_H
#define POINTQUEUE_H
#include <queue> 
#include <cstdint> 
#include "Common.h"

class CPointQueue {
private:
    int minSize;
    int maxSize;
    int allowNoise;
    std::queue<SPointInfo> _queue;
    int paperCount;
    int markerCount;
    int noiseCount;
    int plateCount;
public:
    CPointQueue();
    ~CPointQueue();

    void push(SPointInfo point, FindRecType tag);

    SPointInfo front();

    bool isFulled();

    void clear();

};


#endif