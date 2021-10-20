/*
 * main.c
 *
 *  Created on: Sep 29, 2021
 *      Author: jainm1
 */

#include <cmath>
#include <cstring>
#include "mlc-helper.h"

//inputs : input data, 3d vector for output and time in ms
static char st_getDataAtFixedRate(st_sensorData *state, float inputData[3], float *outData, int64_t t)
{
	char valid = 0;
	int n;
	int64_t tCurrent, dt;
	float alpha;
	if(state->N==0)
	{
		state->N++;
		valid = 1;
		state->data[0][0] = outData[0] = inputData[0];
		state->data[0][1] = outData[1] = inputData[1];
		state->data[0][2] = outData[2] = inputData[2];
		state->tLast = state->t[0] = t;
		return valid;
	}
	//%push data back
	for (n=0;n<3;n++)
	{
		state->data[1][n] = state->data[0][n];
		state->data[0][n] = inputData[n];
	}
	state->t[1] = state->t[0];
	state->t[0] = t;
	state->N++;
	if(state->N>5)
	  state->N = 5;

	tCurrent = (int64_t)(state->tLast+(int64_t)state->dt);
	if(tCurrent>state->t[0])
	    return valid;

	//%check time difference between last and current data is not large
	dt = state->t[0] -state->t[1];
	if(dt<=0 || dt > 100) //%ms
	    return valid;


	//%do linear interpolation
	alpha = (tCurrent - state->t[1])/(float)dt;

	if(alpha<0)
	{
		alpha=0;
		tCurrent = state->t[1];
	}

	for (n=0;n<3;n++)
		outData[n] = state->data[0][n]*(alpha) + (1-alpha)*state->data[1][n];

	valid = 1;
	state->tLast = tCurrent;
	return valid;
}

static uint16_t float16(float in)
{
  uint32_t inu = *((uint32_t*)&in);
  uint32_t t1;
  uint32_t t2;
  uint32_t t3;

  t1 = inu & 0x7FFFFFFF; // Non-sign bits
  t2 = inu & 0x80000000; // Sign bit
  t3 = inu & 0x7F800000; // Exponent

  t1 >>= 13; // Align mantissa on MSB
  t2 >>= 16; // Shift sign bit into position

  t1 -= 0x1C000; // Adjust bias

  t1 = (t3 < 0x38800000) ? 0 : t1; // Flush-to-zero
  t1 = (t3 > 0x47000000) ? 0x7BFF : t1; // Clamp-to-max
  t1 = (t3 == 0 ? 0 : t1); // Denormals-as-zero

  t1 |= t2; // Re-insert sign bit

  return (uint16_t) t1;
}

void stFSMInit(stFSMSensor *state)
{
	memset(state, 0, sizeof(stFSMSensor));
	state->xlData.dt = 76.9230f; //13 Hz
}

/*input current state, acc[3] data in g,
 * output : gVec[3] if return 1*/
int8_t computeGravityVector(stFSMSensor *state, float *accData, int64_t time_ns, float *gVec){
	int16_t nLoop;

	if(state->isValid)
	{
		for (nLoop = 0; nLoop<3; nLoop++)
			gVec[nLoop] = state->sum5Sec[nLoop];

		return state->isValid;
	}
	if(state->tTotal>=STODR*5)
	{
		for (nLoop = 0; nLoop<3; nLoop++)
		{
			state->sum5Sec[nLoop] = state->sum5Sec[nLoop]/(STODR*5);
			gVec[nLoop] = state->sum5Sec[nLoop];
		}

		state->isValid = 1;
		return state->isValid;
	}
	if(state->n<STODR){
		float acc_LINT[3];
		int64_t t= (int64_t)(time_ns/1e6);
	    int8_t v  = st_getDataAtFixedRate(&state->xlData, accData, acc_LINT, t);
	    if(v != 1) {
	    	return state->isValid;
	    }
		for (nLoop = 0; nLoop<3; nLoop++){
			state->sum[nLoop] += acc_LINT[nLoop];
			state->sumSq[nLoop] += acc_LINT[nLoop]*acc_LINT[nLoop];
		}
		state->n++;
	}
	else{
		float varAcc = 0, meanV=0;
		for (nLoop = 0; nLoop<3; nLoop++){
			varAcc +=  (state->sumSq[nLoop] - state->sum[nLoop]*state->sum[nLoop]/STODR);
			meanV += state->sumSq[nLoop];
		}
		varAcc = varAcc/(STODR-1);
		meanV = sqrtf(meanV/STODR);

		if(varAcc < STSTATICVAR && fabs(meanV-1.0f) < STSTATICMEAN)
		{
			state->isValid = 1;
			for (nLoop = 0; nLoop<3; nLoop++)
			{
				state->sum[nLoop] = state->sum[nLoop]/(STODR);
				state->sum5Sec[nLoop] = state->sum[nLoop];
				gVec[nLoop] = state->sum[nLoop];
			}
		}
		else{
			for (nLoop = 0; nLoop<3; nLoop++){
				state->sum5Sec[nLoop] += state->sum[nLoop];
			}
		}
		for (nLoop = 0; nLoop<3; nLoop++){
			state->sum[nLoop] = 0;
			state->sumSq[nLoop] = 0;
		}
		state->tTotal = state->n;
		state->n = 0;
	}
	return state->isValid;
}

/*compute thresholds and provide float16 output*/
void computeThreshold(float *gvec, uint16_t thresh[][2])
{
	int16_t nLoop;
	for (nLoop = 0; nLoop<3; nLoop++)
	{
		thresh[nLoop][0] =  float16(gvec[nLoop]+ FSMTHR);
		thresh[nLoop][1] =  float16(gvec[nLoop]- FSMTHR);
	}
}
