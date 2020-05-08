/*
 * STMicroelectronics SW Gravity Sensor Class
 *
 * Copyright 2015-2016 STMicroelectronics Inc.
 * Author: Denis Ciocca - <denis.ciocca@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include <fcntl.h>
#include <assert.h>
#include <signal.h>

#include "SWGravity.h"

SWGravity::SWGravity(const char *name, int handle) :
	SWSensorBaseWithPollrate(name, handle, STMSensorType::GRAVITY,
			true, false, true, false)
{
#if (CONFIG_ST_HAL_ANDROID_VERSION > ST_HAL_KITKAT_VERSION)
	//sensor_t_data.stringType = SENSOR_STRING_TYPE_GRAVITY;
	info.flags |= SENSOR_FLAG_CONTINUOUS_MODE;
#endif /* CONFIG_ST_HAL_ANDROID_VERSION */

	info.maxRange = ST_HAL_GRAVITY_MAX_ON_EARTH;

#ifdef CONFIG_ST_HAL_GRAVITY_AP_ENABLED_9X
	dependencies_type_list[SENSOR_DEPENDENCY_ID_0] = SENSOR_TYPE_ST_ACCEL_MAGN_GYRO_FUSION9X;
#endif /* CONFIG_ST_HAL_GRAVITY_AP_ENABLED_9X */

#ifdef CONFIG_ST_HAL_GRAVITY_AP_ENABLED_6X
	dependencies_type_list[SENSOR_DEPENDENCY_ID_0] = SENSOR_TYPE_ST_ACCEL_GYRO_FUSION6X;
#endif /* CONFIG_ST_HAL_GRAVITY_AP_ENABLED_6X */

	id_sensor_trigger = SENSOR_DEPENDENCY_ID_0;
}

SWGravity::~SWGravity()
{

}

void SWGravity::ProcessData(SensorBaseData *data)
{
	memcpy(sensor_event.data, data->processed, SENSOR_DATA_3AXIS * sizeof(float));
	sensor_event.timestamp = data->timestamp;

	SWSensorBaseWithPollrate::WriteDataToPipe(data->pollrate_ns);
	SWSensorBaseWithPollrate::ProcessData(data);
}
