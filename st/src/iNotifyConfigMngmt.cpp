/*
 * STMicroelectronics iNotify HAL configuration management
 *
 * Copyright 2021 STMicroelectronics Inc.
 * Author: Mario Tesi <mario.tesi@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <mutex>

#include "utils.h"

#include "iNotifyConfigMngmt.h"

static const char *parsing_strings[] = {
	"imu_sensor_placement = ",
	"imu_sensor_euler_angles = ",
	"algo_towing_jack_delta_th = ",
	"algo_towing_jack_min_duration = ",
	"algo_crash_impact_th = ",
	"algo_crash_min_duration = ",
	"ignition_off = ",
};

static volatile int run = 1;
static struct hal_config_t hal_config;
static std::mutex configMutex;

static void sig_callback(int sig)
{
	switch(sig) {
	case SIGINT:
		run = 0;
		break;
	default:
		break;
	}
}

static int show_sensor_placement(struct hal_config_t *config)
{
	ALOGD("Rotation Matrix: \t%5.2f %5.2f %5.2f %6.2f\n\t\t\t%5.2f %5.2f %5.2f %6.2f\n\t\t\t%5.2f %5.2f %5.2f %6.2f\n",
		config->sensor_placement.rot[0][0],
		config->sensor_placement.rot[0][1],
		config->sensor_placement.rot[0][2],
		config->sensor_placement.location[0],
		config->sensor_placement.rot[1][0],
		config->sensor_placement.rot[1][1],
		config->sensor_placement.rot[1][2],
		config->sensor_placement.location[1],
		config->sensor_placement.rot[2][0],
		config->sensor_placement.rot[2][1],
		config->sensor_placement.rot[2][2],
		config->sensor_placement.location[2]);

	return 0;
}

static void init_hal_config(struct hal_config_t *config)
{
	std::lock_guard<std::mutex> lock(configMutex);

	config->sensor_placement.rot[0][0] = 1;
	config->sensor_placement.rot[0][1] = 0;
	config->sensor_placement.rot[0][2] = 0;

	config->sensor_placement.rot[1][0] = 0;
	config->sensor_placement.rot[1][1] = 1;
	config->sensor_placement.rot[1][2] = 0;

	config->sensor_placement.rot[2][0] = 0;
	config->sensor_placement.rot[2][1] = 0;
	config->sensor_placement.rot[2][2] = 1;

	config->sensor_placement.location[0] = 0;
	config->sensor_placement.location[1] = 0;
	config->sensor_placement.location[2] = 0;

	config->algo_towing_jack_delta_th = 25; //25mg
	config->algo_towing_jack_min_duration = 0;
	config->algo_crash_impact_th = 0;
	config->algo_crash_min_duration = 0;
}

static void update_rotation_matrix(struct hal_config_t *config, float yawd, float pitchd, float rolld)
{
	float yaw = (yawd / 10.0f) * M_PI / 180.0f;
	float pitch = (pitchd / 10.0f) * M_PI / 180.0f;
	float roll = (rolld / 10.0f) * M_PI / 180.0f;

	config->sensor_placement.rot[0][0] = cos(yaw) * cos(roll) - sin(yaw) * sin(pitch) * sin(roll);
	config->sensor_placement.rot[0][1] = -sin(yaw) * cos(pitch);
	config->sensor_placement.rot[0][2] = cos(yaw) * sin(roll) + sin(yaw) * sin(pitch) * cos(roll);

	config->sensor_placement.rot[1][0] = sin(yaw) * cos(roll) + cos(yaw) * sin(pitch) * sin(roll);
	config->sensor_placement.rot[1][1] = cos(yaw) * cos(pitch);
	config->sensor_placement.rot[1][2] = sin(yaw) * sin(roll) - cos(yaw) * sin(pitch) * cos(roll);

	config->sensor_placement.rot[2][0] = -cos(pitch) * sin(roll);
	config->sensor_placement.rot[2][1] = sin(pitch);
	config->sensor_placement.rot[2][2] = cos(pitch) * cos(roll);
}

static int update_sensor_placement(struct hal_config_t *config,
				   enum PARSING_STRING_INDEX index,
				   char *data, int len)
{
	std::lock_guard<std::mutex> lock(configMutex);
	int roll = 0, pitch = 0, yaw = 0;
	int x = 0, y = 0, z = 0;

	switch (index) {
	case IMU_SENSOR_PLACEMENT_INDEX:
		if (sscanf(data, "[%d,%d,%d]", &x, &y, &z) != 3) {
			return -EINVAL;
		}

		config->sensor_placement.location[0] = x;
		config->sensor_placement.location[1] = y;
		config->sensor_placement.location[2] = z;
		break;
	case IMU_SENSOR_EULER_ANGLES_INDEX:
		if (sscanf(data, "[%d,%d,%d]", &roll, &pitch, &yaw) != 3) {
			return -EINVAL;
		}

		update_rotation_matrix(config, yaw, pitch, roll);
		break;
	default:
		return 0;
	}

	return 0;
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

static int write_algos_parameters_to_driver(struct hal_config_t *config)
{
	int ret;

	if (config->algo_towing_jack_min_duration != 0) {
		uint16_t value = (config->algo_towing_jack_min_duration * 12.5f) / 17;
		char fsm_str[3 * sizeof(value) * strlen("00,")];

		ret = sprintf(fsm_str,
			      "%2x,%2x,%2x,%2x,%2x,%2x",
			      (uint8_t)(value >> 8),
			      (uint8_t)(value & 0x00FF),
			      (uint8_t)(value >> 8),
			      (uint8_t)(value & 0x00FF),
			      (uint8_t)(value >> 8),
			      (uint8_t)(value & 0x00FF));
		if (ret < 0) {
			ALOGE("\"%s\": Failed to allocate min duration",
			      __FUNCTION__);

			return ret;
		}

		ALOGD("\"%s\": Updating towing/jack min duration to %s",
		      __FUNCTION__, fsm_str);
		ret = device_iio_utils::update_fsm_jack_min_duration(fsm_str);
		if (ret < 0) {
			ALOGE("\"%s\": Failed to update towing/jack min duration",
			      __FUNCTION__);

			return ret;
		}
	}
	if (config->algo_crash_impact_th != 0) {
		uint16_t valueth2 = float16((config->algo_crash_impact_th / 1000.0f) / 2);
		uint16_t valueth1 = float16(-(config->algo_crash_impact_th / 1000.0f) / 2);
		char fsm_str[2 * sizeof(uint16_t) * strlen("00,")];

		ret = sprintf(fsm_str,
			      "%2x,%2x,%2x,%2x",
			      (uint8_t)(valueth1 >> 8),
			      (uint8_t)(valueth1 & 0x00FF),
			      (uint8_t)(valueth2 >> 8),
			      (uint8_t)(valueth2 & 0x00FF));
		if (ret < 0) {
			ALOGE("\"%s\": Failed to allocate crash impact threshold",
			      __FUNCTION__);

			return ret;
		}

		ALOGD("\"%s\": Updating crash impact threshold to %s",
		      __FUNCTION__, fsm_str);
		ret = device_iio_utils::update_crash_impact_th(fsm_str);
		if (ret < 0) {
			ALOGE("\"%s\": Failed to update crash impact threshold",
			      __FUNCTION__);

			return ret;
		}
	}
	if (config->algo_crash_min_duration != 0) {
		uint16_t value = (config->algo_crash_min_duration * 12.5f) / 17;

		char fsm_str[sizeof(value) * strlen("00,")];
		ret = sprintf(fsm_str,
			      "%2x,%2x",
			      (uint8_t)(value >> 8),
			      (uint8_t)(value & 0x00FF));
		if (ret < 0) {
			ALOGE("\"%s\": Failed to allocate crash min duration",
			      __FUNCTION__);

			return ret;
		}

		ALOGD("\"%s\": Updating crash min duration to %s",
		      __FUNCTION__, fsm_str);
		ret = device_iio_utils::update_crash_min_duration(fsm_str);
		if (ret < 0) {
			ALOGE("\"%s\": Failed to update crash min duration",
			      __FUNCTION__);

			return ret;
		}
	}

	return 0;
}

static int update_algo_towing_delta_th(struct hal_config_t *config,
				       enum PARSING_STRING_INDEX index,
				       char *data, int len)
{
	std::lock_guard<std::mutex> lock(configMutex);
	uint16_t algo_towing_jack_delta_th;

	if (sscanf(data, "%hu", &algo_towing_jack_delta_th) != 1) {
		return -EINVAL;
	}

	/* check algo towing jack delta th interval [10-1000] mg */
	if (algo_towing_jack_delta_th < 10)
		algo_towing_jack_delta_th = 10;
	if (algo_towing_jack_delta_th > 1000)
		algo_towing_jack_delta_th = 1000;

	config->algo_towing_jack_delta_th = algo_towing_jack_delta_th;

	return 0;
}

static int update_algo_min_duration(struct hal_config_t *config,
				    enum PARSING_STRING_INDEX index,
				    char *data, int len)
{
	std::lock_guard<std::mutex> lock(configMutex);
	uint32_t duration = 0;

	if (sscanf(data, "%u", &duration) != 1) {
		return -EINVAL;
	}

	/*
	 * check if towing jack min duration and crash min duration
	 * interval in [2-89000] s
	 */
	if (duration < 2)
		duration = 2;
	if (duration > 89000)
		duration = 89000;

	switch (index) {
	case ALGO_TOWING_JACK_MIN_DURATION_INDEX:
		config->algo_towing_jack_min_duration = duration;
		break;
	case ALGO_CRASH_MIN_DURATION_INDEX:
		config->algo_crash_min_duration = duration;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int update_algo_crash_impact_th(struct hal_config_t *config,
				       enum PARSING_STRING_INDEX index,
				       char *data, int len)
{
	std::lock_guard<std::mutex> lock(configMutex);
	uint16_t algo_crash_impact_th;

	if (sscanf(data, "%hu", &algo_crash_impact_th) != 1) {
		return -EINVAL;
	}

	/* check crash impact th interval [100-2000] mg */
	if (algo_crash_impact_th < 100)
		algo_crash_impact_th = 100;
	if (algo_crash_impact_th > 2000)
		algo_crash_impact_th = 2000;

	config->algo_crash_impact_th = algo_crash_impact_th;

	return 0;
}

static int update_ignition_off(struct hal_config_t *config,
			       enum PARSING_STRING_INDEX index,
			       char *data, int len)
{
	std::lock_guard<std::mutex> lock(configMutex);
	uint32_t ignition_off;

	if (sscanf(data, "%u", &ignition_off) != 1) {
		return -EINVAL;
	}

	config->ignition_off = ignition_off;

	return 0;
}

static int ignition_off_check_and_run(struct hal_config_t *config,
				      STSensorHAL_data *hal_data)
{
	std::lock_guard<std::mutex> lock(configMutex);
	int ret = 0;

	if (config->ignition_off && hal_data) {
		ret = hal_data->sensor_classes[hal_data->sensor_t_list[0].handle]->Ignition(config->ignition_off);
		config->ignition_off = 0;
	}

	return ret;
}

static int update_file_data(const char *file, char *path)
{
	char *file_path_name = NULL;
	char *buffer_string = NULL;
	FILE *fd_config = NULL;
	struct stat st;
	int err = 0;
	int len = 0;
	char *ptr;
	int size;

	file_path_name = (char *)calloc(strlen(path) + strlen(file) + 2, 1);
	if (!file_path_name) {
		return -ENOMEM;
	}

	sprintf(file_path_name, "%s/%s", path, file);
	stat(file_path_name, &st);
	size = st.st_size;
	if (size == 0) {
		ALOGE("Zero len file %s\n", file_path_name);

		goto err_out;
	}

	fd_config = fopen(file_path_name, "r");
	if (!fd_config) {
		err = -errno;
		ALOGE("Filed to open %s (errno %d)\n", file_path_name, err);

		goto err_out;
	}

	buffer_string = (char *)calloc(size + 1, 1);
	if (!buffer_string) {
		goto err_out;
	}

	len = fread(buffer_string, 1, size, fd_config);
	if (!len) {
		goto err_out;
	}

	ptr = strstr(buffer_string, parsing_strings[IMU_SENSOR_PLACEMENT_INDEX]);
	if (ptr) {
		ptr += strlen(parsing_strings[IMU_SENSOR_PLACEMENT_INDEX]);

		update_sensor_placement(&hal_config, IMU_SENSOR_PLACEMENT_INDEX, ptr, ptr - buffer_string);
	}

	ptr = strstr(buffer_string, parsing_strings[IMU_SENSOR_EULER_ANGLES_INDEX]);
	if (ptr) {
		ptr += strlen(parsing_strings[IMU_SENSOR_EULER_ANGLES_INDEX]);

		update_sensor_placement(&hal_config, IMU_SENSOR_EULER_ANGLES_INDEX, ptr, ptr - buffer_string);
	}

	ptr = strstr(buffer_string, parsing_strings[ALGO_TOWING_JACK_DELTA_TH_INDEX]);
	if (ptr) {
		ptr += strlen(parsing_strings[ALGO_TOWING_JACK_DELTA_TH_INDEX]);

		update_algo_towing_delta_th(&hal_config, ALGO_TOWING_JACK_DELTA_TH_INDEX, ptr, ptr - buffer_string);
	}

	ptr = strstr(buffer_string, parsing_strings[ALGO_TOWING_JACK_MIN_DURATION_INDEX]);
	if (ptr) {
		ptr += strlen(parsing_strings[ALGO_TOWING_JACK_MIN_DURATION_INDEX]);

		update_algo_min_duration(&hal_config, ALGO_TOWING_JACK_MIN_DURATION_INDEX, ptr, ptr - buffer_string);
	}

	ptr = strstr(buffer_string, parsing_strings[ALGO_CRASH_IMPACT_TH_INDEX]);
	if (ptr) {
		ptr += strlen(parsing_strings[ALGO_CRASH_IMPACT_TH_INDEX]);

		update_algo_crash_impact_th(&hal_config, ALGO_CRASH_IMPACT_TH_INDEX, ptr, ptr - buffer_string);
	}

	ptr = strstr(buffer_string, parsing_strings[ALGO_CRASH_MIN_DURATION_INDEX]);
	if (ptr) {
		ptr += strlen(parsing_strings[ALGO_CRASH_MIN_DURATION_INDEX]);

		update_algo_min_duration(&hal_config, ALGO_CRASH_MIN_DURATION_INDEX, ptr, ptr - buffer_string);
	}

	ptr = strstr(buffer_string, parsing_strings[IGNITION_OFF_INDEX]);
	if (ptr) {
		ptr += strlen(parsing_strings[IGNITION_OFF_INDEX]);

		update_ignition_off(&hal_config, IGNITION_OFF_INDEX, ptr, ptr - buffer_string);
	}

err_out:
	if (fd_config) {
		fclose(fd_config);
	}

	if (file_path_name) {
		free(file_path_name);
	}

	if (buffer_string) {
		free(buffer_string);
	}

	return err;
}

static int is_directory(const char *path)
{
	struct stat path_stat;
	stat(path, &path_stat);

	return S_ISDIR(path_stat.st_mode);
}

static void *hal_configuration_thread(void *parm)
{
	static size_t bufsiz = sizeof(struct inotify_event) + PATH_MAX + 1;
	struct thread_params_t *thread_params;
	char *buffer;
	int fd;
	int wd;

	fd = inotify_init();
	if (fd < 0) {
		ALOGE("inotify_init");
		return NULL;
	}

	buffer = (char *)malloc(bufsiz);
	if (!buffer) {
		ALOGE("no mem");
		return NULL;
	}

	thread_params = (struct thread_params_t *)parm;
	if (!thread_params->pathname ||
	    !is_directory(thread_params->pathname)) {
		ALOGE("pathname is not a valid directory %s\n", thread_params->pathname);
		exit(-1);
	}

	init_hal_config(&hal_config);

	update_file_data(HAL_CONFIGURATION_FILE, thread_params->pathname);
	write_algos_parameters_to_driver(&hal_config);
	show_sensor_placement(&hal_config);
	ignition_off_check_and_run(&hal_config, thread_params->hal_data);

	wd = inotify_add_watch(fd, thread_params->pathname, IN_CLOSE);
	if (wd < 0) {
		if (buffer) {
			free(buffer);
			buffer = NULL;
		}

		ALOGE("unable to add watch");
		exit(-1);
	}

	while (run) {
		int length;
		int i;

		length = read(fd, buffer, bufsiz);
		if (length < 0) {
			continue;
		}

		i = 0;
		while (i < length) {
			struct inotify_event *event = (struct inotify_event *)&buffer[i];

			if (event->mask & IN_Q_OVERFLOW) {
				ALOGE( "Overflow" );
			}

			if (event->len) {
				if (event->mask & IN_CLOSE) {
					if (event->mask & IN_CLOSE_WRITE) {
						ALOGD("Configuration file %s closed for write", event->name);
						if (strcmp(event->name, HAL_CONFIGURATION_FILE) == 0) {
							update_file_data(HAL_CONFIGURATION_FILE, thread_params->pathname);
							write_algos_parameters_to_driver(&hal_config);
							show_sensor_placement(&hal_config);
						}
					}
				}
			}

			i += sizeof(struct inotify_event) + event->len;
		}

		ignition_off_check_and_run(&hal_config,
					   thread_params->hal_data);
	}

	if (fd >= 0) {
		/* removing the directory from the watch list */
		inotify_rm_watch(fd, wd);

		/* closing the INOTIFY instance */
		close(fd);
	}

	if (buffer) {
		free(buffer);
		buffer = NULL;
	}

	pthread_exit((void *)0);
}

int init_notify_loop(char *pathname, STSensorHAL_data *hal_data)
{
	static struct thread_params_t thread_params;
	static struct sigaction sa;
	pthread_t threadid;
	int status;

	if (!pathname) {
		ALOGE("Invalid path name\n");

		return -1;
	}

	sa.sa_handler = sig_callback;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags &= ~SA_RESTART;

	if (sigaction(SIGINT, &sa, NULL) == -1) {
		ALOGE("unable to install sigint handler");

		return -1;
	}

	thread_params.pathname = pathname;
	thread_params.hal_data = hal_data;
	status = pthread_create(&threadid, NULL,
				hal_configuration_thread,
				(void *)&thread_params);
	if ( status <  0) {
		ALOGE("pthread_create failed");

		return -1;
	}

	return 0;
}

const struct hal_config_t get_config(void)
{
	configMutex.lock();
	struct hal_config_t tmp = hal_config;
	configMutex.unlock();

	return tmp;
}
