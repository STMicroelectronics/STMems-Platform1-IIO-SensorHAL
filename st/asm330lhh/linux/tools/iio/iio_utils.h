/*
 * STMicroelectronics iio_utils header for SensorHAL
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
#ifndef __IIO_UTILS
#define __IIO_UTILS

#include <sys/ioctl.h>
#include <stdint.h>
#include "SensorHAL.h"

#define DEVICE_IIO_MAX_FILENAME_LEN				256
#define DEVICE_IIO_MAX_NAME_LENGTH				16
#define DEVICE_IIO_MAX_SAMPLINGFREQ_LENGTH		32
#define DEVICE_IIO_MAX_SAMP_FREQ_AVAILABLE		10

#define DEVICE_IIO_SCALE_AVAILABLE				10

#define IIO_EVENT_CODE_EXTRACT_CHAN(mask)		((__s16)(mask & 0xFFFF))
#define IIO_EVENT_CODE_EXTRACT_CHAN2(mask)		((__s16)(((mask) >> 16) & 0xFFFF))
#define IIO_EVENT_CODE_EXTRACT_MODIFIER(mask)	((mask >> 40) & 0xFF)
#define IIO_EVENT_CODE_EXTRACT_DIFF(mask)		(((mask) >> 55) & 0x1)

#include <linux/ioctl.h>
#include <linux/types.h>

struct device_iio_event_data {
	__u64	id;
	__s64	timestamp;
};

#define IIO_GET_EVENT_FD_IOCTL _IOR('i', 0x90, int)
#define IIO_EVENT_CODE_EXTRACT_TYPE(mask) ((mask >> 56) & 0xFF)
#define IIO_EVENT_CODE_EXTRACT_DIR(mask) ((mask >> 48) & 0x7F)
#define IIO_EVENT_CODE_EXTRACT_CHAN_TYPE(mask) ((mask >> 32) & 0xFF)

/* Event code number extraction depends on which type of event we have.
 * Perhaps review this function in the future*/
#define IIO_EVENT_CODE_EXTRACT_CHAN(mask) ((__s16)(mask & 0xFFFF))
#define IIO_EVENT_CODE_EXTRACT_CHAN2(mask) ((__s16)(((mask) >> 16) & 0xFFFF))

#define IIO_EVENT_CODE_EXTRACT_MODIFIER(mask) ((mask >> 40) & 0xFF)
#define IIO_EVENT_CODE_EXTRACT_DIFF(mask) (((mask) >> 55) & 0x1)

typedef enum {
	/* Refer to ASM330LHH driver implementation */
	DEVICE_IIO_ACC = 3,
	DEVICE_IIO_GYRO,
	DEVICE_IIO_TEMP = 9,
	DEVICE_IIO_TIMESTAMP = 13,
} device_iio_chan_type_t;

#define IIO_EV_DIR_FIFO_DATA	0x05
#define IIO_EV_TYPE_FIFO_FLUSH	0x06

struct device_iio_scale_available {
	float scales[DEVICE_IIO_SCALE_AVAILABLE];
	unsigned int length;
};

struct device_iio_sampling_freq_avail {
	unsigned int freq[DEVICE_IIO_MAX_SAMP_FREQ_AVAILABLE];
	unsigned int length;
};

struct device_iio_channel_info {
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

class iio_utils {
	private:
		static int sysfs_write_int(char *file, int val);
		static int sysfs_write_str(char *file, char *str);
		static int sysfs_read_str(char *file, char *str, int len);
		static int sysfs_read_int(char *file, int *val);
		static int sysfs_write_scale(char *file, float val);
		static int sysfs_read_scale(char *file, float *val);
		static int enable_channels(const char *device_dir, bool enable);

	public:
		static int get_device_by_name(const char *name);
		static int enable_sensor(char *device_dir, bool enable);
		static int get_sampling_frequency_available(char *device_dir,
				struct device_iio_sampling_freq_avail *sfa);
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
		static int get_type(struct device_iio_channel_info *channel,
							const char *device_dir, const char *name,
							const char *post);
		
		static int get_scale_available(const char *device_dir,
									   struct device_iio_scale_available *sa,
									   device_iio_chan_type_t device_type);
		static int support_injection_mode(const char *device_dir);
		static int set_injection_mode(const char *device_dir, bool enable);
		static int inject_data(const char *device_dir, unsigned char *data, 
							   int len, device_iio_chan_type_t device_type);
};

#endif /* __IIO_UTILS */
