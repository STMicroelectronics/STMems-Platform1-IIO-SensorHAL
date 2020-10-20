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

int Accelerometer::Enable(int handle, bool enable, bool lock_en_mutex)
{
	return HWSensorBaseWithPollrate::Enable(handle, enable, lock_en_mutex);
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
size_t Accelerometer::getSensorAdditionalInfoPayLoadFramesArray(additional_info_event_t **array_sensorAdditionalInfoPLFrames)
{
	additional_info_event_t XL_SAI_Placement_event, *p;
	additional_info_event_t *p_custom_XL_SAI_Placement_event = NULL;

	// place for ODM/OEM to fill custom_XL_SAI_Placement_event

	if (!p_custom_XL_SAI_Placement_event) {
		XL_SAI_Placement_event = defaultSensorPlacement_additional_info_event;
		ALOGD("%s: using Sensor Additional Info Placement default", GetName());
	} else {
		XL_SAI_Placement_event = *p_custom_XL_SAI_Placement_event;
	}

	size_t frames = 1;

	p = (additional_info_event_t *)calloc(frames , sizeof(additional_info_event_t));
	if (!p) {
		ALOGE("%s: Failed to allocate memory.", GetName());
		return (size_t)-ENOMEM;
	}
	for (size_t i = 0; i < frames; i++)
		memcpy(&p[i], &XL_SAI_Placement_event, sizeof(additional_info_event_t));

	*array_sensorAdditionalInfoPLFrames = p;
	return sizeof(**array_sensorAdditionalInfoPLFrames)/sizeof(*array_sensorAdditionalInfoPLFrames[0]);
}
#endif /* CONFIG_ST_HAL_ADDITIONAL_INFO_ENABLED */
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */
