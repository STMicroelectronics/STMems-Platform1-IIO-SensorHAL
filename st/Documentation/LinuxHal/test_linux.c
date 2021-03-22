/*
 * STMicroelectronics SensorHAL simple test
 *
 * Copyright 2021 STMicroelectronics Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#undef ANDROID_LOG
#define LOG_FILE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <fcntl.h>
#include <string.h>
#include <dlfcn.h>
#include <pthread.h>
#include <getopt.h>
#include <stdbool.h>
#include <asm/types.h>
#include <linux/limits.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <hardware/sensors.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#define TEST_LINUX_VERSION	"1.1"

#ifndef LOG_TAG
#define LOG_TAG "test_linux"
#endif

#ifdef ANDROID_LOG
	#include <utils/Log.h>
	#include <android/log.h>
	#define tl_log(...) \
		__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else /* ANDROID_LOG */
	#define tl_debug(fmt, ...) printf("%s:%s:%s:%d > " fmt "\n", \
			LOG_TAG, __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)
	#ifdef LOG_FILE
		#define tl_log(fmt, ...) do { \
			fprintf(logfd, "%s:%s:%s:%d > " fmt "\n", \
				    LOG_TAG, __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__); \
			} while(0);
	#else /* LOG_FILE */
		#define tl_log(fmt, ...) printf("%s:%s:%s:%d > " fmt "\n", \
				    LOG_TAG, __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)
	#endif /* LOG_FILE */
#endif /* ANDROID_LOG */

/* Max event buffer for poll sensor */
#define BUFFER_EVENT		256

/* Defines to enable/disable sensors */
#define SENSOR_ENABLE		1
#define SENSOR_DISABLE		0

/* Translate sansor index from listo to handle */
#define HANDLE_FROM_INDEX(_i) (i + 1)
#define INVALID_HANDLE	-1

#define UCF_STR_LEN		256
#define PATH_MLC_BINARY		"/lib/firmware/st_asm330lhhx_mlc.bin"
#define SENSOR_TYPE_ST_CUSTOM_NO_SENSOR		(SENSOR_TYPE_DEVICE_PRIVATE_BASE + 20)

#define IIO_DEVICE_NAME		"/sys/bus/iio/devices/iio:device"

#define IIO_GYRO_IIO_DEVICE_NUMBER "0"
#define IIO_ACC_IIO_DEVICE_NUMBER  "1"

#define IIO_GYRO_DEVICE_NAME	IIO_DEVICE_NAME IIO_GYRO_IIO_DEVICE_NUMBER
#define IIO_ACC_DEVICE_NAME	IIO_DEVICE_NAME IIO_ACC_IIO_DEVICE_NUMBER

#define IIO_MLC_EVENT_NAME	"in_activity0_thresh_rising_en"
#define IIO_MLC_LOAD_FILE	"/load_mlc"
#define IIO_MLC_FLUSH_FILE	"/mlc_flush"
#define IIO_MLC_VERSION_FILE	"/mlc_version"
#define IIO_MLC_INFO_FILE	"/mlc_info"

#define IIO_GET_EVENT_FD_IOCTL _IOR('i', 0x90, int)

#define IIO_EVENT_CODE_EXTRACT_TYPE(mask) ((mask >> 56) & 0xFF)
#define IIO_EVENT_CODE_EXTRACT_DIR(mask) ((mask >> 48) & 0xCF)
#define IIO_EVENT_CODE_EXTRACT_CHAN_TYPE(mask) ((mask >> 32) & 0xFF)
#define IIO_EVENT_CODE_EXTRACT_CHAN(mask) ((__s16)(mask & 0xFFFF))
#define IIO_EVENT_CODE_EXTRACT_CHAN2(mask) ((__s16)(((mask) >> 16) & 0xFFFF))
#define IIO_EVENT_CODE_EXTRACT_MODIFIER(mask) ((mask >> 40) & 0xFF)
#define IIO_EVENT_CODE_EXTRACT_DIFF(mask) (((mask) >> 55) & 0x1)

#ifdef LOG_FILE
#define TL_FILE_DEFAULT_NAME	"log.txt"
static FILE *logfd;
#endif /* LOG_FILE */

/**
 * struct iio_event_data - The actual event being pushed to userspace
 * @id:		event identifier
 * @timestamp:	best estimate of time of event occurrence (often from
 *		the interrupt handler)
 */
struct iio_event_data {
	__u64	id;
	__s64	timestamp;
};

static const char *options = "a:g:b:fn:d:s:le:o:Nhv?";
static const int test_sensor_type[] = {
		SENSOR_TYPE_GYROSCOPE,
		SENSOR_TYPE_ACCELEROMETER,
		SENSOR_TYPE_TEMPERATURE,
		SENSOR_TYPE_MAGNETIC_FIELD,
		SENSOR_TYPE_PRESSURE,
		-1,
	};

static uint64_t start_systime, start_sensortime[SENSOR_TYPE_TEMPERATURE + 1];


static const struct option long_options[] = {
		{"accodr",    required_argument, 0,  'a' },
		{"gyroodr",   required_argument, 0,  'g' },
		{"list",      no_argument,       0,  'l' },
		{"nsample",   required_argument, 0,  'n' },
		{"flush",     no_argument,       0,  'f' },
		{"batch",     required_argument, 0,  'b' },
		{"delay",     required_argument, 0,  'd' },
		{"timeout",   required_argument, 0,  't' },
		{"sensor",    required_argument, 0,  's' },
		{"mlc",       required_argument, 0,  'm' },
		{"mlc_info",  no_argument,       0,  'i' },
		{"mlc_flush", no_argument,       0,  'u' },
		{"events",    required_argument, 0,  'e' },
#ifdef LOG_FILE
		{"logname",   required_argument, 0,  'o' },
#endif /* LOG_FILE */
		{"notemp",    no_argument,       0,  'N' },
		{"version",   no_argument,       0,  'v' },
		{"gyropm",    required_argument, 0,  'q' },
		{"accpm",     required_argument, 0,  'p' },
		{"help",      no_argument,       0,  '?' },
		{0,           0,                 0,   0  }
	};

/* Path to SensorHAL.so in target filesystem */
static const char *hal_paths[] = {
	"/tmp/mnt/SensorHAL.so",
	"/home/pi/SensorHAL.so",
	"/system/vendor/lib64/hw/SensorHAL.so",
	"/system/vendor/lib/hw/SensorHAL.so",
};

static const char* types[] = {
	"...",
	"Acc",
	"Mag",
	"...",
	"Gyro",
	"...",
	"Press",
	"Temp",
};

static int num_sample = 10;
static int acc_odr = 104;
static int gyro_odr = 104;
static int list_opt = 0;
static int flush_opt = 0;
static int64_t batch_ns = 0;
static int64_t delay_ns = 0;
static volatile int timeout = 0;
static int samples_timeout = 1;
static struct sensors_module_t *hmi;
static struct hw_device_t *dev;
struct sensors_poll_device_1 *poll_dev;
static struct sensor_t const* list;
static int sensor_num;
static int sensor_handle = -1;
static int test_events = 0;
static int mlc_iio_device_number = 3;
static int mlc_wait_events_device_number = 4;

/* Report sensor type in string format */
static const char *type_str(int type)
{
	int type_count = sizeof(types)/sizeof(char *);

	if (type < 0 || type >= type_count)
		return "unknown";

	return types[type];
}

static void dump_event(struct sensors_event_t *e)
{
	struct timeval tv;
	uint64_t sec_usec;

	gettimeofday(&tv, NULL);
	sec_usec = tv.tv_sec * 1000000 + tv.tv_usec;

	if (start_systime == 0LL)
		start_systime = sec_usec;

	switch (e->type) {
	case SENSOR_TYPE_ACCELEROMETER:
		if (start_sensortime[0] == 0LL)
			start_sensortime[0] = e->timestamp;
		tl_log("ACC event: x=%10.2f y=%10.2f z=%10.2f timestamp=%lld systime=%lld delta=%lld",
			e->acceleration.x, e->acceleration.y, e->acceleration.z,
			e->timestamp - start_sensortime[0],
			sec_usec - start_systime,
			((sec_usec - start_systime) * 1000) - (e->timestamp - start_sensortime[0]));
		break;
	case SENSOR_TYPE_GYROSCOPE:
		if (start_sensortime[1] == 0LL)
			start_sensortime[1] = e->timestamp;
		tl_log("GYRO event: x=%10.2f y=%10.2f z=%10.2f timestamp=%lld systime=%lld delta=%lld",
			e->gyro.x, e->gyro.y, e->gyro.z,
			e->timestamp - start_sensortime[1],
			sec_usec - start_systime,
			((sec_usec - start_systime) * 1000) - (e->timestamp - start_sensortime[0]));
		break;
	case SENSOR_TYPE_TEMPERATURE:
		if (start_sensortime[2] == 0LL)
			start_sensortime[2] = e->timestamp;
		tl_log("TEMP event: x=%10.2f timestamp=%lld systime=%lld delta=%lld",
			e->temperature, e->timestamp - start_sensortime[2],
			sec_usec - start_systime,
			((sec_usec - start_systime) * 1000) - (e->timestamp - start_sensortime[0]));
		break;
	case SENSOR_TYPE_MAGNETIC_FIELD:
		if (start_sensortime[2] == 0LL)
			start_sensortime[2] = e->timestamp;
		tl_log("MAG event: x=%10.2f y=%10.2f z=%10.2f timestamp=%lld systime=%lld delta=%lld",
			e->magnetic.x, e->magnetic.y, e->magnetic.z,
			e->timestamp - start_sensortime[1],
			sec_usec - start_systime,
			((sec_usec - start_systime) * 1000) - (e->timestamp - start_sensortime[0]));
		break;
	case SENSOR_TYPE_PRESSURE:
		if (start_sensortime[2] == 0LL)
			start_sensortime[2] = e->timestamp;
		tl_log("PRESS event: x=%10.2f timestamp=%lld systime=%lld delta=%lld",
			e->pressure, e->timestamp - start_sensortime[2],
			sec_usec - start_systime,
			((sec_usec - start_systime) * 1000) - (e->timestamp - start_sensortime[0]));
		break;
	default:
		tl_log("Unhandled events %d", e->type);
		break;
	}
}

static void dump_sensor(const struct sensor_t *s)
{
	int i;

	tl_debug("\nSensor List");
	for (i = 0; i < sensor_num; i++) {
		tl_debug("\n\tName %s", s[i].name);
		tl_debug("\tVendor %s", s[i].vendor);
		tl_debug("\tHandle %d", s[i].handle);
		tl_debug("\t\tType %s (%d)", type_str(s[i].type), s[i].type);
		tl_debug("\t\tVersion %d", s[i].version);
		tl_debug("\t\tMax Range %f", s[i].maxRange);
		tl_debug("\t\tResolution %f", s[i].resolution);
		tl_debug("\t\tPower %f", s[i].power);
		tl_debug("\t\tMin Delay %d", s[i].minDelay);
		tl_debug("\t\tMax Delay %d", s[i].maxDelay);
		tl_debug("\t\tFIFO Reserved Event %d",
		       s[i].fifoReservedEventCount);
		tl_debug("\t\tFIFO Max Event %d", s[i].fifoMaxEventCount);
	}
}

static int get_sensor(const struct sensor_t *s, int type,
		      struct sensor_t **sens)
{
	int i;

	for (i = 0; i < sensor_num; i++) {
		if (type == s[i].type) {
			*sens = (struct sensor_t *)&s[i];

			return HANDLE_FROM_INDEX(i);
		}
	}

	return INVALID_HANDLE;
}

static void sensors_poll(int maxcount, int timeout_s)
{
	int tot = 0;

	alarm(timeout_s);
	while (tot < maxcount && !timeout) {
		sensors_event_t events[BUFFER_EVENT];
		int i, count;

		count = poll_dev->poll(&poll_dev->v0,
				       events,
				       sizeof(events)/sizeof(sensors_event_t));

		for(i = 0; i < count; i++)
			dump_event(&events[i]);

		tot += count;
	}

	/* clear any pending alarms */
	alarm(0);

	tl_log("%s samples received %d\n", timeout ? "Timeout" : "Total", tot);
	timeout = 0;
}

static int sensor_activate(int handle, int enable)
{
	if (handle < 0)
		return -ENODEV;

	return poll_dev->activate(&poll_dev->v0, handle, enable);
}

static int sensor_setdelay(int type, int odr)
{
	int handle;
	int64_t delay;
	struct sensor_t *sensor = NULL;

	handle = get_sensor(list, type, &sensor);
	if (handle < 0 || !sensor)
		return -ENODEV;

	delay = 1000000000 / odr;
	tl_debug("Setting Acc ODR to %d Hz (%lld ns)\n", odr, delay);

	return poll_dev->setDelay(&poll_dev->v0, handle, delay);
}

static int sensor_flush(int handle)
{
	if (handle < 0)
		return -ENODEV;

	return poll_dev->flush(poll_dev, handle);
}

static int sensor_batch(int handle, int64_t sampling_period_ns,
			int64_t max_report_latency_ns)
{
	if (handle < 0)
		return -ENODEV;

	return poll_dev->batch(poll_dev, handle, 0,
			       sampling_period_ns,
			       max_report_latency_ns);
}

static int sensor_batching(int64_t delay_time, int64_t batch_time)
{
	int sindex = 0;
	int handle;
	struct sensor_t *sensor = NULL;

	while(test_sensor_type[sindex] != -1) {
		handle = get_sensor(list, test_sensor_type[sindex], &sensor);
		if (handle < 0)
			break;

		sensor_batch(handle, batch_time, delay_time);
		sindex++;
	}

	return 0;
}

static int open_hal(char *lcl_path)
{
	void *hal;
	int err;
	const char *lh_path = NULL;

	if (!lcl_path) {
		unsigned i;

		for (i = 0; i < sizeof(hal_paths)/sizeof(const char*); i++) {
			if (!access(hal_paths[i], R_OK)) {
				lh_path = hal_paths[i];
				break;
			}
		}

		if (!lh_path) {
			fprintf(stderr, "ERROR: unable to find HAL\n");
			exit(1);
		}
	} else
		lh_path = lcl_path;

	hal = dlopen(lh_path, RTLD_NOW);
	if (!hal) {
		fprintf(stderr, "ERROR: unable to load HAL %s: %s\n", lh_path,
			dlerror());
		return -1;
	}

	hmi = dlsym(hal, HAL_MODULE_INFO_SYM_AS_STR);
	if (!hmi) {
		fprintf(stderr, "ERROR: unable to find %s entry point in HAL\n",
			HAL_MODULE_INFO_SYM_AS_STR);
		return -1;
	}

	tl_log("HAL loaded: name %s vendor %s version %d.%d id %s",
	       hmi->common.name, hmi->common.author,
	       hmi->common.version_major, hmi->common.version_minor,
	       hmi->common.id);

	err = hmi->common.methods->open((struct hw_module_t *)hmi,
					SENSORS_HARDWARE_POLL, &dev);
	if (err) {
		fprintf(stderr, "ERROR: failed to initialize HAL: %d\n", err);
		exit(1);
	}

	poll_dev = (sensors_poll_device_1_t *)dev;

	return 0;
}

static void sensor_flush_all(void)
{
	int sindex = 0;
	int handle;
	struct sensor_t *sensor = NULL;

	while(test_sensor_type[sindex] != -1) {
		handle = get_sensor(list, test_sensor_type[sindex], &sensor);
		if (handle < 0)
			break;

		sensor_flush(handle);
		sindex++;
	}
}

static void sensor_disable_all(void)
{
	int sindex = 0;
	int handle;
	struct sensor_t *sensor = NULL;

	while(test_sensor_type[sindex] != -1 &&
	      test_sensor_type[sindex] != SENSOR_TYPE_ST_CUSTOM_NO_SENSOR) {
		handle = get_sensor(list, test_sensor_type[sindex], &sensor);
		if (handle < 0)
			break;

		tl_log("Deactivating %s sensor (handle %d)",
		       sensor->name, handle);
		sensor_activate(handle, SENSOR_DISABLE);

		sindex++;
	}
}

static void alarmHandler(int dummy)
{
	timeout = 1;
}

static void help(char *argv)
{
	int index = 0;

	printf("usage: %s [OPTIONS]\n\n", argv);
	printf("OPTIONS:\n");
	printf("\t--%s:\tSet Accelerometer ODR (default %d)\n",
	       long_options[index++].name, acc_odr);
	printf("\t--%s:\tSet Gyroscope ODR (default %d)\n",
	       long_options[index++].name, gyro_odr);
	printf("\t--%s:\t\tShow sensor list\n", long_options[index++].name);
	printf("\t--%s:\tNumber of samples (default %d)\n",
	       long_options[index++].name, num_sample);
	printf("\t--%s:\tFlush data before test\n", long_options[index++].name);
	printf("\t--%s:\tSet Batch Time in ns (default disabled)\n",
	       long_options[index++].name);
	printf("\t--%s:\tSet Delay time in ns\n", long_options[index++].name);
	printf("\t--%s:\tSet timeout (s) for receive samples (default %d s)\n",
	       long_options[index++].name, samples_timeout);
	printf("\t--%s:\tSelect sensor (default all)\n",
	       long_options[index++].name);
	printf("\t--%s:\t\tLoad a machine learning core ucf file\n",
	       long_options[index++].name);
	printf("\t--%s:\tGet MLC info\n",
	       long_options[index++].name);
	printf("\t--%s:\tFlush MLC configuration\n",
	       long_options[index++].name);
	printf("\t--%s:\tEvent monitoring (specify device as iio:device3)\n",
	       long_options[index++].name);

#ifdef LOG_FILE
	printf("\t--%s:\tOutput log file (default %s)\n",
	       long_options[index++].name, TL_FILE_DEFAULT_NAME);
#endif /* LOG_FILE */

	printf("\t--%s:\tDisable temperature sensor\n", long_options[index++].name);
	printf("\t--%s:\tPrint Version\n", long_options[index++].name);
	printf("\t--%s:\tSet Gyro Power Mode (0 = HP, 1 = LP)\n", long_options[index++].name);
	printf("\t--%s:\tSet Acc Power Mode (0 = HP, 1 = LP)\n", long_options[index++].name);
	printf("\t--%s:\t\tThis help\n", long_options[index++].name);

	exit(0);
}

static int single_sensor_test(int sindex)
{
	int handle;
	struct sensor_t *sensor = NULL;

	/* Activate sensor */
	handle = get_sensor(list, test_sensor_type[sindex], &sensor);
	if (handle != INVALID_HANDLE && sensor) {
		if (handle < 0)
			return -ENODEV;

		tl_log("Activating %s sensor (handle %d)",
			sensor->name, handle);
		sensor_activate(handle, SENSOR_ENABLE);

		/* Start polling data */
		tl_log("Polling %s sensor (handle %d) for %d samples in %d s",
		       sensor->name, handle, num_sample, samples_timeout);
		sensors_poll(num_sample, samples_timeout);

		/* Deactivate sensor */
		tl_log("Deactivating %s sensor (handle %d)",
		       sensor->name, handle);
		sensor_activate(handle, SENSOR_DISABLE);
	}
}

static int all_sensor_test(int notemp)
{
	int sindex = 0;
	int handle;
	struct sensor_t *sensor = NULL;

	while (test_sensor_type[sindex] != -1) {
		/* skip if requested (for acc/gyro log) */
		if (notemp && test_sensor_type[sindex] == SENSOR_TYPE_TEMPERATURE) {
			sindex++;
			continue;
		}

		/* Activate sensor */
		handle = get_sensor(list, test_sensor_type[sindex], &sensor);
		if (handle < 0) {
			sindex++;
			continue;
		}

		tl_log("Activating %s sensor (handle %d)",
			sensor->name, handle);
		sensor_activate(handle, SENSOR_ENABLE);
		sindex++;
	}

	/* Start polling data */
	tl_log("Polling all sensor for %d samples in %d s",
	       num_sample, samples_timeout);
	sensors_poll(num_sample, samples_timeout);

	sensor_disable_all();
}

/*
 * MLC
 */
static int find_mlc_iio_device_number(void)
{
	char iio_device_name[UCF_STR_LEN];
	struct stat sb;
	int i;

	for (i = 3; i <= 4; i++) {
		sprintf(iio_device_name,
			"/sys/bus/iio/devices/iio:device%d/mlc_version",
			i);

		if (stat(iio_device_name, &sb) == 0) {
			tl_debug("Found MLC IIO device number %d", i);

			return i;
		}
	}

	return -1;
}

/*
 * echo 1 > load_mlc
 */
static int load_mlc(int iio_device_number, char *ucf_file_name,
		    const char *mcl_fw_name)
{
	struct sensor_t *sensor = NULL;
	char mlc_file_name[UCF_STR_LEN];
	FILE *sysfs_load = NULL;
	FILE *ucf_file = NULL;
	char str[UCF_STR_LEN];
	unsigned char buff[2];
	FILE *mcl_fw = NULL;
	int ret;

	ucf_file = fopen(ucf_file_name, "r");
	if (ucf_file == 0) {
		perror("Unable to open file");

		goto out;
	}

	mcl_fw = fopen(mcl_fw_name, "wb");
	if (mcl_fw == 0) {
		perror("Unable to open file");

		goto out;
	}

	while (!feof(ucf_file)) {
		if (fgets(str, UCF_STR_LEN, ucf_file) != NULL) {
			if (strstr(str, "--"))
				continue;

			ret = sscanf(str, "Ac %x %x", &buff[0], &buff[1]);
			if (ret != 2) {
				continue;
			}

			fwrite(buff, ret, 1, mcl_fw);
		}
	}

	ret = sprintf(mlc_file_name, "%s%d/%s",
		      IIO_DEVICE_NAME,
		      iio_device_number,
		      IIO_MLC_LOAD_FILE);
	if (ret < 0) {
		perror("Failed to retrive device name");

		goto out;
	}

	sysfs_load = fopen(mlc_file_name, "w");
	if (sysfs_load == NULL) {
		perror("open");
		goto out;
	}

	tl_debug("Loading MLC.......");
	ret = fprintf(sysfs_load, "1");
	if (ret < 0) {
		tl_debug("[FAIL]");
	} else {
		tl_debug("[DONE]\n");
	}

out:
	if (ucf_file)
		fclose(ucf_file);

	if (mcl_fw)
		fclose(mcl_fw);

	if (sysfs_load)
		fclose(sysfs_load);

	return 0;
}

static int mlc_info(int iio_device_number)
{
	FILE *mlc_info_fd = NULL;
	FILE *mlc_version_fd = NULL;
	char mlc_info_file_name[256];
	char mlc_version_file_name[256];
	char str[256];
	int ret;

	ret = sprintf(mlc_info_file_name, "%s%d/%s",
		      IIO_DEVICE_NAME,
		      iio_device_number,
		      IIO_MLC_INFO_FILE);
	if (ret < 0) {
		perror("Failed to retrive mlc_info file name");

		return -1;
	}
	ret = sprintf(mlc_version_file_name, "%s%d/%s",
		      IIO_DEVICE_NAME,
		      iio_device_number,
		      IIO_MLC_VERSION_FILE);
	if (ret < 0) {
		perror("Failed to retrive mlc_version file name");

		return -1;
	}

	tl_debug("opening file %s\n", mlc_version_file_name);
	mlc_version_fd = fopen(mlc_version_file_name, "r");
	if (!mlc_version_fd) {
		perror("open");
		ret = -errno;

		return ret;
	}

	tl_debug("opening file %s\n", mlc_info_file_name);
	mlc_info_fd = fopen(mlc_info_file_name, "r");
	if (!mlc_info_fd) {
		perror("open");
		ret = -errno;

		goto out;
	}

	if (fgets(str, 256, mlc_info_fd) != NULL) {
		tl_debug("%s\n", str);
	}

	memset(str, 0, 256);
	if (fgets(str, 256, mlc_version_fd) != NULL) {
		tl_debug("%s\n", str);
	}

	fclose(mlc_info_fd);
out:
	fclose(mlc_version_fd);

	return ret;
}

static int mlc_flush(int iio_device_number)
{
	char mlc_file_name[UCF_STR_LEN];
	FILE *sysfs_flush = NULL;
	char str[UCF_STR_LEN];
	int ret;

	ret = sprintf(mlc_file_name, "%s%d/%s",
		      IIO_DEVICE_NAME,
		      iio_device_number,
		      IIO_MLC_FLUSH_FILE);
	if (ret < 0) {
		perror("Failed to retrive mlc device name");

		goto out;
	}

	tl_debug("opening file %s\n", mlc_file_name);
	sysfs_flush = fopen(mlc_file_name, "w");
	if (sysfs_flush == NULL) {
		perror("open");
		goto out;
	}

	ret = fprintf(sysfs_flush, "1");
	if (ret < 0) {
		tl_debug("[FAIL]");
	} else {
		tl_debug("[DONE]\n");
	}

out:
	if (sysfs_flush)
		fclose(sysfs_flush);

	return 0;
}

static void print_event(struct iio_event_data *event)
{
	unsigned char *pevent;

	pevent = (unsigned char *)event;

	tl_debug("Event: %x %x %x %x %x %x %x %x time: %lld\n",
		pevent[0], pevent[1], pevent[2], pevent[3],
		pevent[4], pevent[5], pevent[6], pevent[7],
		event->timestamp);
}

static int enable_events(int iio_device_number, int enable)
{
	FILE *event_file_enable = NULL;
	char event_file_enable_name[256];
	int ret;

	ret = sprintf(event_file_enable_name,
		      "/sys/bus/iio/devices/iio:device%d/events/%s",
		      iio_device_number, IIO_MLC_EVENT_NAME);
	if (ret < 0) {
		perror("Failed to retrive event file name");

		return -1;
	}

	event_file_enable = fopen(event_file_enable_name, "r+");
	if (!event_file_enable) {
		perror("open");
		ret = -errno;

		return ret;
	}

	if (enable)
		ret = fprintf(event_file_enable, "1");
	else
		ret = fprintf(event_file_enable, "0");

	fclose(event_file_enable);

	return ret;
}

static int poll_events(int iio_device_number)
{
	int fd = -1;
	int event_fd = -1;
	int ret = 0;
	struct iio_event_data event;
	char device_path[256];

	ret = sprintf(device_path,
		      "/dev/iio:device%d",
		      iio_device_number);
	if (ret < 0)
		return ret;

	tl_debug("opening device %s\n", device_path);
	fd = open(device_path, 0);
	if (fd == -1) {
		perror("open");
		ret = -errno;

		goto exit_poll;
	}

	ret = enable_events(iio_device_number, 1);
	if (ret < 0)
		return ret;

	ret = ioctl(fd, IIO_GET_EVENT_FD_IOCTL, &event_fd);
	close(fd);

	if (ret == -1 || event_fd == -1) {
		fprintf(stdout, "Failed to retrieve event fd\n");
		ret = -errno;

		goto exit_poll;
	}

	while (test_events) {
		ret = read(event_fd, &event, sizeof(event));
		if (ret == -1) {
			if (errno == EAGAIN) {
				tl_debug("nothing available\n");
				continue;
			} else {
				perror("Failed to read event from device");
				ret = -errno;
				break;
			}
		}

		print_event(&event);
	}

exit_poll:
	ret = enable_events(iio_device_number, 0);
	if (ret < 0)
		return ret;

	if (fd)
		close(fd);

	if (event_fd)
		close(event_fd);

	return ret;
}

static int set_power_mode(char *device, int mode)
{
	FILE *power_mode_fd = NULL;
	char power_mode_file_name[256];
	int ret;


	ret = sprintf(power_mode_file_name, "%s/power_mode", device);
	if (ret < 0) {
		perror("Failed to retrive power mode file name");

		return -1;
	}

	power_mode_fd = fopen(power_mode_file_name, "r+");
	if (!power_mode_fd) {
		perror("open");
		ret = -errno;

		return ret;
	}

	if (mode)
		ret = fprintf(power_mode_fd, "1");
	else
		ret = fprintf(power_mode_fd, "0");

	fclose(power_mode_fd);

	return ret;
}

static void ctrlzHandler(int events)
{
	sensor_disable_all();

	if (events)
		enable_events(mlc_iio_device_number, 0);

#ifdef LOG_FILE
	if (logfd)
		fclose(logfd);
#endif /* LOG_FILE */

	exit(0);
}

int main(int argc, char **argv)
{
	int ret;
	int c;
	int digit_optind = 0;
	char *ptr;
	char *ucf_file_name = NULL;
	int wait_events = 0;
	int flush_mlc = 0;
	int new_pm = 0;
	int set_pm = 0;
	int find_mlc = 0;
	int info_mlc = 0;

#ifdef LOG_FILE
	char *log_filename = NULL;
	int splitfile = 0;
#endif /* LOG_FILE */

	int notemp = 0;
	int i;

	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;

		c = getopt_long(argc, argv, options,
				long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'a':
			acc_odr = atoi(optarg);
			break;
		case 'g':
			gyro_odr = atoi(optarg);
			break;
		case 'l':
			list_opt = 1;
			break;
		case 'b':
			batch_ns = strtoul(optarg, &ptr, 10);
			break;
		case 'd':
			delay_ns = strtoul(optarg, &ptr, 10);
			break;
		case 'f':
			flush_opt = 1;
			break;
		case 't':
			samples_timeout = atoi(optarg);
			break;
		case 'n':
			num_sample = atoi(optarg);
			break;
		case 's':
			sensor_handle = atoi(optarg);
			break;
		case 'm':
			ucf_file_name = optarg;
			find_mlc = 1;
			break;
		case 'u':
			flush_mlc = 1;
			find_mlc = 1;
			break;
		case 'e':
			wait_events = 1;
			mlc_wait_events_device_number = atoi(optarg);
			break;
#ifdef LOG_FILE
		case 'o':
			log_filename = optarg;
			break;
#endif /* LOG_FILE */

		case 'N':
			notemp = 1;
			break;
		case 'q':
			new_pm = atoi(optarg);
			set_pm = 1;
			break;
		case 'p':
			new_pm = atoi(optarg);
			set_pm = 2;
			break;
		case 'v':
			printf("Version %s\n", TEST_LINUX_VERSION);
			exit(0);
		case 'i':
			find_mlc = 1;
			info_mlc = 1;
			break;
		default:
			help(argv[0]);
		}
	}

#ifdef LOG_FILE
	if (log_filename) {
		logfd = fopen(log_filename, "w+");
		if (!logfd) {
			perror("open");
			exit(-1);
		}
	} else {
		logfd = fopen(TL_FILE_DEFAULT_NAME, "w+");
		if (!logfd) {
			perror("open");
			exit(-1);
		}
	}
#endif /* LOG_FILE */

	if (find_mlc) {
		mlc_iio_device_number = find_mlc_iio_device_number();
		if (mlc_iio_device_number < 0) {
			fprintf(stderr, "ERROR: unable to find MLC device\n");
			exit(1);
		}
	}

	if (info_mlc) {
		mlc_info(mlc_iio_device_number);
		exit(0);
	}

	if (flush_mlc) {
		mlc_flush(mlc_iio_device_number);

		/* check if just a flush or not */
		if (!ucf_file_name) {
			exit(0);
		}
	}

	if (ucf_file_name) {
		ret = load_mlc(mlc_iio_device_number,
			       ucf_file_name,
			       PATH_MLC_BINARY);
		if (ret) {
			fprintf(stderr, "ERROR: unable to load MLC device\n");
			exit(1);
		}

		exit(0);
	}

	switch (set_pm) {
	case 0:
		break;
	case 1:
		set_power_mode(IIO_GYRO_DEVICE_NAME, new_pm);
		break;
	case 2:
		set_power_mode(IIO_ACC_DEVICE_NAME, new_pm);
		break;
	}

	if (wait_events) {
		/* set loop until CTRL^Z */
		test_events = 1;
		poll_events(mlc_wait_events_device_number);
	}

	ret = open_hal(NULL);
	if (ret) {
		fprintf(stderr, "ERROR: unable to open SensorHAL\n");
		exit(1);
	}

	sensor_num = hmi->get_sensors_list(hmi, &list);
	if (!sensor_num) {
		fprintf(stderr, "ERROR: no sensors available\n");
		exit(1);
	}

	/* Dump sensor list */
	if (list_opt) {
		dump_sensor(list);
		exit(0);
	}

	sensor_disable_all();

	if (sensor_flush)
		sensor_flush_all();

	/* Check for batching or not */
	if (batch_ns && delay_ns) {
		sensor_batching(batch_ns, delay_ns);
	} else {
		sensor_setdelay(SENSOR_TYPE_ACCELEROMETER, acc_odr);
		sensor_setdelay(SENSOR_TYPE_GYROSCOPE, gyro_odr);
	}

	signal(SIGINT, ctrlzHandler);
	signal(SIGALRM, alarmHandler);

	start_systime = 0LL;
	for(i = 0; i < 3; i++)
		start_sensortime[i] = 0LL;

	if (sensor_handle >= 0)
		single_sensor_test(sensor_handle);
	else
		all_sensor_test(notemp);

	ctrlzHandler(wait_events);

	return 0;
}
