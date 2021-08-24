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

#include "iNotifyConfigMngmt.h"
#include "SensorHAL.h"

static const char *parsing_strings[] = {
	"imu_sensor_placement = ",
};

static volatile int run = 1;
static struct hal_config_t hal_config;

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
	ALOGD("Rotation Matrix: \t%5.2f %5.2f %5.2f %6.2f\n\t\t\t%5.2f %5.2f %5.2f %6.2f\n\t\t\t%5.2f %5.2f %5.2f %6.2f",
		config->sensor_placement.x[0],
		config->sensor_placement.x[1],
		config->sensor_placement.x[2],
		config->sensor_placement.location[0],
		config->sensor_placement.y[0],
		config->sensor_placement.y[1],
		config->sensor_placement.y[2],
		config->sensor_placement.location[1],
		config->sensor_placement.z[0],
		config->sensor_placement.z[1],
		config->sensor_placement.z[2],
		config->sensor_placement.location[2]);

	return 0;
}

static int update_sensor_placement(struct hal_config_t *config,
				   char *rotms, int len)
{
	if (len > 0) {
		sscanf(rotms, "[%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f]",
		       &config->sensor_placement.x[0],
		       &config->sensor_placement.x[1],
		       &config->sensor_placement.x[2],
		       &config->sensor_placement.y[0],
		       &config->sensor_placement.y[1],
		       &config->sensor_placement.y[2],
		       &config->sensor_placement.z[0],
		       &config->sensor_placement.z[1],
		       &config->sensor_placement.z[2],
		       &config->sensor_placement.location[0],
		       &config->sensor_placement.location[1],
		       &config->sensor_placement.location[2]);
	}

	return 0;
}

static int update_file_data(const char *file, char *path, struct inotify_event *event)
{
	char *file_path_name = NULL;
	char *buffer_string = NULL;
	FILE *fd_config = NULL;
	struct stat st;
	int err = 0;
	int len = 0;
	char *ptr;
	int size;

	if (!event->name) {
		return -EINVAL;
	}

	file_path_name = (char *)calloc(strlen(path) + strlen(file) + 2, 1);
	if (!file_path_name) {
		return -ENOMEM;
	}

	sprintf(file_path_name, "%s/%s", path, event->name);
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
	if (!ptr) {
		goto err_out;
	}

	ptr += strlen(parsing_strings[IMU_SENSOR_PLACEMENT_INDEX]);

	update_sensor_placement(&hal_config, ptr, ptr - buffer_string);

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
		ALOGE("pathname is not a valid directory");
		exit(-1);
	}

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
							update_file_data(HAL_CONFIGURATION_FILE, thread_params->pathname, event);
							show_sensor_placement(&hal_config);
						}
					}
				}
			}

		i += sizeof(struct inotify_event) + event->len;
		}
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

int init_notify_loop(char *pathname)
{
	struct thread_params_t thread_params;
	struct sigaction sa;
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
	status = pthread_create(&threadid, NULL,
				hal_configuration_thread,
				(void *)&thread_params);
	if ( status <  0) {
		ALOGE("pthread_create failed");

		return -1;
	}

	return 0;
}
