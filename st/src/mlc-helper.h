/*
 * main.c
 *
 *  Created on: Sep 29, 2021
 *      Author: jainm1
 */

#pragma once

#include <stdint.h>

#define STODR (13)
#define STSTATICVAR (0.0001F)
#define STSTATICMEAN (0.1F)
#define FSMTHR (0.025f)

typedef struct {
    int64_t tLast; //in ms
    float data[2][3];
    int64_t t[2];
    int N;      // init with zero with lib
    float dt;  //at which rate we want to interpolate
} st_sensorData;

typedef struct {
	int16_t n;
	float sum[3];
	float sumSq[3];
	float sum5Sec[3];
	int16_t tTotal;
	int8_t isValid;
	st_sensorData xlData;
} stFSMSensor;

enum stFSMState {
	RESET = 0,
	INITIALIZED,
	RUNNING
};

void stFSMInit(stFSMSensor *state);

/*input current state, acc[3] data in g,
 * output : gVec[3] if return 1*/
int8_t computeGravityVector(stFSMSensor *state, float *accData, int64_t time_ns, float *gVec);

/*compute thresholds and provide float16 output*/
void computeThreshold(float *gvec, uint16_t thresh[][2]);
