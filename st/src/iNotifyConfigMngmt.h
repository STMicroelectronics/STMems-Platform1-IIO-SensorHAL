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

#define HAL_CONFIGURATION_FILE	"hal_config"
#define HAL_CONFIGURATION_PATH	"/etc/sensorhal/"

enum PARSING_STRING_INDEX {
	IMU_SENSOR_PLACEMENT_INDEX = 0,
	IMU_SENSOR_EULER_ANGLES_INDEX,
};

struct thread_params_t {
	char *pathname;
};

struct sensor_placement_t {
	float rot[3][3];
	float location[3];
};

struct hal_config_t {
	struct sensor_placement_t sensor_placement;
	int loglevel;
};

int init_notify_loop(char *pathname);

struct hal_config_t* get_sensor_placement(void);

#endif /* __HAL_INOTIFY_CONFIGURATION */
