/*
 * STMicroelectronics iNotify HAL configuration management header file
 *
 * Copyright 2021 STMicroelectronics Inc.
 * Author: Mario Tesi <mario.tesi@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#ifndef __HAL_INOTIFY_CONFIGURATION
#define __HAL_INOTIFY_CONFIGURATION

#include "SensorHAL.h"

#define HAL_CONFIGURATION_FILE	"hal_config"
#define HAL_CONFIGURATION_PATH	"/etc/sensorhal/"

enum PARSING_STRING_INDEX {
	IMU_SENSOR_PLACEMENT_INDEX = 0,
	IMU_SENSOR_EULER_ANGLES_INDEX,
	ALGO_TOWING_JACK_DELTA_TH_INDEX,
	ALGO_TOWING_JACK_MIN_DURATION_INDEX,
	ALGO_CRASH_IMPACT_TH_INDEX,
	ALGO_CRASH_MIN_DURATION_INDEX,
	IGNITION_OFF_INDEX,
};

struct thread_params_t {
	char *pathname;
	STSensorHAL_data *hal_data;
};

struct sensor_placement_t {
	float rot[3][3];
	float location[3];
};

struct hal_config_t {
	struct sensor_placement_t sensor_placement;
	uint16_t algo_towing_jack_delta_th;
	uint32_t algo_towing_jack_min_duration;
	uint16_t algo_crash_impact_th;
	uint32_t algo_crash_min_duration;
	uint32_t ignition_off;
	int loglevel;
};

int init_notify_loop(char *pathname, STSensorHAL_data *hal_data);

const struct hal_config_t get_config(void);

#endif /* __HAL_INOTIFY_CONFIGURATION */
