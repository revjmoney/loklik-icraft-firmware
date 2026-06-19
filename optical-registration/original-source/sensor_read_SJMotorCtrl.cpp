// EXTRACT from Grbl_Esp32/src/Device/deffile/SJMotorCtrl.cpp (LOKLiK GPL release).
// These two functions are the hardware read for the optical registration eye.
// The "blue light" sensor is PIN_SEEK_BOX = GPIO35, read via analogRead().
// SeekPointByXY() moves the head so the sensor sits over (x,y) then samples reflectance.
//
// Sensor sits offset from the tool: +13.8mm X, +34.8mm Y (see COpticalAlign sensor_offset_*).
// Analog value is classified into Paper / Plate / Marker bins by COpticalAlign::findMarkByRange.

int SJMotorCtrl::SeekPointByXY(float x, float y) {
    // x coordinate max
    float maxXAxis = 345;
#if defined(PIN_SEEK_BOX)
    // out of recognizable range -> recognition error
    if (x <= maxXAxis) {
        // move motors to the target position
        motorX.runToNewPosition(toStepX(x));
        motorY.runToNewPosition(toStepY(y));
        // read the edge-seek sensor
        return  analogRead(PIN_SEEK_BOX);
    } else {
        return 0;
    }
#else
    return 0;
#endif
}

// Raw line-scan helper used to dump 10-sample reflectance batches over serial (VAL: ...).
// Useful for calibrating the tolerance bins to your actual paper/mat/ink.
int SJMotorCtrl::TestFindPointByRange(float maxval, int dirval, int speedval) {
    motors_unstep();
    motors_set_disable(false);

    motorX.setAcceleration(toStepX(speedval));
    motorX.setSpeed(0);
    motorX.setMaxSpeed(toStepX(speedval / 60));

    motorY.setAcceleration(toStepY(speedval));
    motorY.setSpeed(0);
    motorY.setMaxSpeed(toStepY(speedval / 60));

    float currPos = (dirval == 1) ? toMMX(motorX.currentPosition()) : toMMY(motorY.currentPosition());
    float step = 0.05;            // 0.05mm scan step
    float p = currPos;
    float pmax = currPos + maxval;
    float* parray = new float[10];
    int pindex = 0;
    while (p <= pmax) {
        if (dirval == 1) {
            motorX.runToNewPosition(toStepX(p));
        } else if (dirval == 2) {
            motorY.runToNewPosition(toStepY(p));
        }
        int psval = analogRead(PIN_SEEK_BOX);
        parray[pindex] = psval;
        p = p + step;
        pindex++;
        if (pindex >= 10) {
            pindex = 0;
            grbl_sendf(CLIENT_SERIAL, "VAL:%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,%0.2f,\r\n",
                       parray[0], parray[1], parray[2], parray[3], parray[4],
                       parray[5], parray[6], parray[7], parray[8], parray[9]);
        }
        yield();
    }
    motors_unstep();
    motors_set_disable(true);
    return 0;
}
