/*
 * STMicroelectronics Accelerometer Sensor Class
 *
 * Copyright 2015-2018 STMicroelectronics Inc.
 * Author: Denis Ciocca - <denis.ciocca@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include <fcntl.h>
#include <assert.h>
#include <signal.h>

#include "Accelerometer.h"
#include "iNotifyConfigMngmt.h"

static int toggle_ignition;

Accelerometer::Accelerometer(HWSensorBaseCommonData *data, const char *name,
		struct device_iio_sampling_freqs *sfa, int handle,
		unsigned int hw_fifo_len, float power_consumption, bool wakeup) :
			HWSensorBaseWithPollrate(data, name, sfa, handle,
			SENSOR_TYPE_ACCELEROMETER, hw_fifo_len, power_consumption)
{
#if (CONFIG_ST_HAL_ANDROID_VERSION > ST_HAL_KITKAT_VERSION)
	sensor_t_data.stringType = SENSOR_STRING_TYPE_ACCELEROMETER;
	sensor_t_data.flags |= SENSOR_FLAG_CONTINUOUS_MODE;

	if (wakeup)
		sensor_t_data.flags |= SENSOR_FLAG_WAKE_UP;
#else /* CONFIG_ST_HAL_ANDROID_VERSION */
	(void)wakeup;
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

	sensor_t_data.resolution = data->channels[0].scale;
	sensor_t_data.maxRange =
		sensor_t_data.resolution * (pow(2, data->channels[0].bits_used - 1) - 1);

#if (CONFIG_ST_HAL_ANDROID_VERSION >= ST_HAL_PIE_VERSION)
#if (CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED)
	supportsSensorAdditionalInfo = true;
#endif /* CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED */
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */
}

Accelerometer::~Accelerometer()
{

}

#ifdef PLTF_LINUX_ENABLED
int Accelerometer::Ignition(int status)
{
	if (status) {
		Enable(10, true, true);
	}

	ALOGD("\"%s\": Ignition mode %d", sensor_t_data.name, status);
	toggle_ignition = status;

	return 0;
}
#endif /* PLTF_LINUX_ENABLED */

int Accelerometer::Enable(int handle, bool enable, bool lock_en_mutex)
{
	return HWSensorBaseWithPollrate::Enable(handle, enable, lock_en_mutex);
}

void Accelerometer::calculateThresholdMLC(SensorBaseData &data)
{
	static int8_t isStatic = 0;
 
	switch (fsmNextState) {
	case RESET:
		isStatic = 0;
		stFSMInit(&state);
		fsmNextState = INITIALIZED;
		break;
	case INITIALIZED:
		// move to running when ignition is off
		// waiting for a command ignition off, then move to running state
		if (toggle_ignition) {
			toggle_ignition = 0;
			fsmNextState = RUNNING;
		}
		break;
	case RUNNING:
		float acc[3], gVec[3];

		acc[0] = data.raw[0] / GRAVITY_EARTH;
		acc[1] = data.raw[1] / GRAVITY_EARTH;
		acc[2] = data.raw[2] / GRAVITY_EARTH;

		if (isStatic == 0){
			isStatic = computeGravityVector(&state, acc, data.timestamp, gVec);
			if (isStatic) {
				int ret;
				int16_t nLoop;
				float threshold;
				uint16_t thresh[3][2];
				uint8_t thresh_hex[3][4];
				char fsm_th_str[sizeof(thresh_hex) * strlen("00,")];
				const struct hal_config_t& config = get_config();

				threshold = config.algo_towing_jack_delta_th / 1000.0f;

				computeThreshold(gVec, thresh, threshold);

				for (nLoop = 0; nLoop < 3; nLoop++) {
					thresh_hex[nLoop][0] = (uint8_t)(thresh[nLoop][0] & 0x00FF);
					thresh_hex[nLoop][1] = (uint8_t)(thresh[nLoop][0] >> 8);
					thresh_hex[nLoop][2] = (uint8_t)(thresh[nLoop][1] & 0x00FF);
					thresh_hex[nLoop][3] = (uint8_t)(thresh[nLoop][1] >> 8);
				}

				// store thresholds into sensors fsm registers
				ret = sprintf(fsm_th_str,
					      "%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x,%2x",
					      thresh_hex[0][1], thresh_hex[0][0],
					      thresh_hex[0][3], thresh_hex[0][2],
					      thresh_hex[1][1], thresh_hex[1][0],
					      thresh_hex[1][3], thresh_hex[1][2],
					      thresh_hex[2][1], thresh_hex[2][0],
					      thresh_hex[2][3], thresh_hex[2][2]);
				if (ret < 0) {
					ALOGE("\"%s\": Failed to allocate FSM threshold",
					      sensor_t_data.name);

					return;
				}

				ALOGD("\"%s\": Updating FSM thresholds %s",
				      sensor_t_data.name, fsm_th_str);
				ret = device_iio_utils::update_fsm_thresholds(fsm_th_str);
				if (ret < 0) {
					ALOGE("\"%s\": Failed to update FSM threshold",
					      sensor_t_data.name);

					return;
				}

				Enable(10, false, true);
			}
		}
		break;
	default:
		return;
	}
}

void Accelerometer::ProcessData(SensorBaseData *data)
{
	float tmp_raw_data[SENSOR_DATA_3AXIS];

	memcpy(tmp_raw_data, data->raw, SENSOR_DATA_3AXIS * sizeof(float));

	data->raw[0] = SENSOR_X_DATA(tmp_raw_data[0],
				     tmp_raw_data[1],
				     tmp_raw_data[2],
				     CONFIG_ST_HAL_ACCEL_ROT_MATRIX);
	data->raw[1] = SENSOR_Y_DATA(tmp_raw_data[0],
				     tmp_raw_data[1],
				     tmp_raw_data[2],
				     CONFIG_ST_HAL_ACCEL_ROT_MATRIX);
	data->raw[2] = SENSOR_Z_DATA(tmp_raw_data[0],
				     tmp_raw_data[1],
				     tmp_raw_data[2],
				     CONFIG_ST_HAL_ACCEL_ROT_MATRIX);

	calculateThresholdMLC(*data);

	applyRotationMatrix(*data);

#if (CONFIG_ST_HAL_DEBUG_LEVEL >= ST_HAL_DEBUG_EXTRA_VERBOSE)
	ALOGD("\"%s\": received new sensor data: x=%f y=%f z=%f, timestamp=%" PRIu64 "ns, deltatime=%" PRIu64 "ns (sensor type: %d).",
	      sensor_t_data.name, data->raw[0], data->raw[1], data->raw[2],
	      data->timestamp, data->timestamp - sensor_event.timestamp, sensor_t_data.type);
#endif /* CONFIG_ST_HAL_DEBUG_LEVEL */

#ifdef CONFIG_ST_HAL_FACTORY_CALIBRATION
	data->raw[0] = (data->raw[0] - factory_offset[0]) * factory_scale[0];
	data->raw[1] = (data->raw[1] - factory_offset[1]) * factory_scale[1];
	data->raw[2] = (data->raw[2] - factory_offset[2]) * factory_scale[2];
	data->accuracy = SENSOR_STATUS_ACCURACY_HIGH;
#else /* CONFIG_ST_HAL_FACTORY_CALIBRATION */
	data->accuracy = SENSOR_STATUS_UNRELIABLE;
#endif /* CONFIG_ST_HAL_FACTORY_CALIBRATION */

	data->processed[0] = data->raw[0];
	data->processed[1] = data->raw[1];
	data->processed[2] = data->raw[2];

	data->accuracy = SENSOR_STATUS_UNRELIABLE;

	sensor_event.acceleration.x = data->processed[0];
	sensor_event.acceleration.y = data->processed[1];
	sensor_event.acceleration.z = data->processed[2];
	sensor_event.acceleration.status = data->accuracy;
	sensor_event.timestamp = data->timestamp;

	HWSensorBaseWithPollrate::WriteDataToPipe(data->pollrate_ns);
	HWSensorBaseWithPollrate::ProcessData(data);
}

#if (CONFIG_ST_HAL_ANDROID_VERSION >= ST_HAL_PIE_VERSION)
#if (CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED)
int Accelerometer::getSensorAdditionalInfoPayLoadFramesArray(additional_info_event_t **array_sensorAdditionalInfoPLFrames)
{
	additional_info_event_t* p_custom_SAI_Placement_event = nullptr;

	// place for ODM/OEM to fill custom_SAI_Placement_event
	// p_custom_SAI_Placement_event = &custom_SAI_Placement_event

/*  //Custom Placement example
	additional_info_event_t custom_SAI_Placement_event;
	custom_SAI_Placement_event = {
		.type = AINFO_SENSOR_PLACEMENT,
		.serial = 0,
		.data_float = {-1, 0, 0, 1, 0, -1, 0, 2, 0, 0, -1, 3},
	};
	p_custom_SAI_Placement_event = &custom_SAI_Placement_event;
*/

	return UseCustomAINFOSensorPlacementPLFramesArray(array_sensorAdditionalInfoPLFrames, p_custom_SAI_Placement_event);

}
#endif /* CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED */
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */
