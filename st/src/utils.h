/*
 * STMicroelectronics device iio utils header for SensorHAL
 *
 * Copyright 2015-2018 STMicroelectronics Inc.
 * Author: Denis Ciocca - <denis.ciocca@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __DEVICE_IIO_UTILS
#define __DEVICE_IIO_UTILS

#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <stdint.h>

#include "SensorHAL.h"

#define DEVICE_IIO_MAX_FILENAME_LEN		256
#define DEVICE_IIO_MAX_NAME_LENGTH		16
#define DEVICE_IIO_MAX_SAMPLINGFREQ_LENGTH	32
#define DEVICE_IIO_MAX_SAMP_FREQ_AVAILABLE	10
#define DEVICE_IIO_SCALE_AVAILABLE		10

/*
 * To fill values for following define please refer to ASM330LHH
 * driver implementation and kernel version
 */
typedef enum {
	DEVICE_IIO_ACC = 3,
	DEVICE_IIO_GYRO,
	DEVICE_IIO_TEMP = 9,
	DEVICE_IIO_TIMESTAMP = 13,
} device_iio_chan_type_t;

#define DEVICE_IIO_EV_DIR_FIFO_DATA		0x05
#define DEVICE_IIO_EV_TYPE_FIFO_FLUSH		0x06

struct device_iio_events {
	uint64_t event_id;
	int64_t event_timestamp;
};

struct device_iio_scales {
	float scales[DEVICE_IIO_SCALE_AVAILABLE];
	unsigned int length;
};

struct device_iio_sampling_freqs {
	float freq[DEVICE_IIO_MAX_SAMP_FREQ_AVAILABLE];
	unsigned int length;
};

struct device_iio_info_channel {
	char *name;
	char *type_name;
	unsigned int index;
	unsigned int enabled;
	float scale;
	float offset;
	unsigned int bytes;
	unsigned int bits_used;
	unsigned int shift;
	unsigned long long int mask;
	unsigned int be;
	unsigned int sign;
	unsigned int location;
};

class device_iio_utils {
	private:
		static int sysfs_write_int(char *file, int val);
		static int sysfs_write_str(char *file, char *str);
		static int sysfs_read_str(char *file, char *str, int len);
		static int sysfs_read_int(char *file, int *val);
		static int sysfs_write_scale(char *file, float val);
		static int sysfs_read_scale(char *file, float *val);
		static int enable_channels(const char *device_dir, bool enable);
		static int check_file(char *filename);

	public:
		static int get_device_by_name(const char *name);
		static int enable_sensor(char *device_dir, bool enable);
		static int get_sampling_frequency_available(char *device_dir,
				struct device_iio_sampling_freqs *sfa);
		static int get_fifo_length(const char *device_dir);
		static int set_sampling_frequency(char *device_dir,
						  unsigned int frequency);
		static int set_hw_fifo_watermark(char *device_dir,
						 unsigned int watermark);
		static int hw_fifo_flush(char *device_dir);
		static int set_scale(const char *device_dir, float value,
				     device_iio_chan_type_t device_type);
		static int get_scale(const char *device_dir, float *value,
				     device_iio_chan_type_t device_type);
		static int get_type(struct device_iio_info_channel *channel,
				    const char *device_dir, const char *name,
				    const char *post);

		static int get_scale_available(const char *device_dir,
					       struct device_iio_scales *sa,
					       device_iio_chan_type_t device_type);
		static int support_injection_mode(const char *device_dir);
		static int set_injection_mode(const char *device_dir, bool enable);
		static int inject_data(const char *device_dir, unsigned char *data,
				       int len, device_iio_chan_type_t device_type);
};

#endif /* __DEVICE_IIO_UTILS */
