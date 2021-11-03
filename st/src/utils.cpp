/*
 * STMicroelectronics device iio utils core for SensorHAL
 *
 * Copyright 2015-2018 STMicroelectronics Inc.
 * Author: Denis Ciocca - <denis.ciocca@st.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 */

#include <iostream>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>

#include "utils.h"

/* IIO Device SYSFS interface */
static const char *device_iio_dir = "/sys/bus/iio/devices/";
static const char *device_iio_sfa_filename = "sampling_frequency_available";
static const char *device_iio_sf_filename = "sampling_frequency";
static const char *device_iio_hw_fifo_enabled = "hwfifo_enabled";
static const char *device_iio_hw_fifo_length = "hwfifo_watermark_max";
static const char *device_iio_hw_fifo_watermark = "hwfifo_watermark";
static const char *device_iio_hw_fifo_flush = "hwfifo_flush";
static const char *device_iio_buffer_enable = "buffer/enable";
static const char *device_iio_buffer_length = "buffer/length";
static const char *device_iio_device_name = "iio:device";
static const char *device_iio_injection_mode_enable = "injection_mode";
static const char *device_iio_injection_sensors_filename = "injection_sensors";
static const char *device_iio_fsm_threshold_filename = "fsm_threshold";
static const char *device_iio_mlc_device_type = "mlc";

int device_iio_utils::sysfs_write_int(char *file, int val)
{
	FILE *fp;

	fp = fopen(file, "w");
	if (NULL == fp)
		return -errno;

	fprintf(fp, "%d", val);
	fclose(fp);

	return 0;
}

int device_iio_utils::sysfs_write_scale(char *file, float val)
{
	FILE *fp;

	fp = fopen(file, "w");
	if (NULL == fp)
		return -errno;

	fprintf(fp, "%f", val);
	fclose(fp);

	return 0;
}

int device_iio_utils::sysfs_read_scale(char *file, float *val)
{
	FILE *fp;

	fp = fopen(file, "r");
	if (NULL == fp)
		return -errno;

	fscanf(fp, "%f", val);
	fclose(fp);

	return 0;
}

int device_iio_utils::sysfs_write_str(char *file, char *str)
{
	FILE *fp;

ALOGD("sysfs_write_str: write to file ret %s data %s", file, str);

	fp = fopen(file, "w");
	if (NULL == fp)
		return -errno;

ALOGD("sysfs_write_str: 111 write to file ret %s data %s", file, str);

	fprintf(fp, "%s", str);
	fclose(fp);

	return 0;
}

int device_iio_utils::sysfs_read_str(char *file, char *str, int len)
{
	FILE *fp;
	int err = 0;

	fp = fopen(file, "r");
	if (fp == NULL)
		return -errno;

	if (fgets(str, len, fp) == NULL)
		err = -errno;

	fclose(fp);

	return err;
}

int device_iio_utils::sysfs_read_int(char *file, int *val)
{
	FILE *fp;
	int ret;

	fp = fopen(file, "r");
	if (NULL == fp)
		return -errno;

	ret = fscanf(fp, "%d\n", val);
	fclose(fp);

	return ret;
}

int device_iio_utils::check_file(char *filename)
{
	struct stat info;

	return stat(filename, &info);
}

int device_iio_utils::enable_channels(const char *device_dir, bool enable)
{
	char dir[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	char filename[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	const struct dirent *ent;
	FILE *sysfsfp;
	DIR *dp;

	if (strlen(device_dir) +
		strlen("scan_elements") + 1 > DEVICE_IIO_MAX_FILENAME_LEN)
		return -1;

	sprintf(dir, "%s/scan_elements", device_dir);
	dp = opendir(dir);
	if (!dp)
		return -errno;

	while (ent = readdir(dp), ent != NULL) {
	if (strlen(dir) +
		strlen(ent->d_name) > DEVICE_IIO_MAX_FILENAME_LEN)
		continue;

		if (!strcmp(ent->d_name + strlen(ent->d_name) - strlen("_en"),
			    "_en")) {
			snprintf(filename, DEVICE_IIO_MAX_FILENAME_LEN,
					 "%s/%s", dir, ent->d_name);
			sysfsfp = fopen(filename, "r+");
			if (!sysfsfp) {
				closedir(dp);
				return -errno;
			}

			fprintf(sysfsfp, "%d", enable);
			fclose(sysfsfp);
		}
	}

	closedir(dp);

	return 0;
}

int device_iio_utils::get_device_by_name(const char *name)
{
	struct dirent *ent;
	int number, numstrlen;
	FILE *devilceFile;
	DIR *dp;
	char dname[DEVICE_IIO_MAX_NAME_LENGTH];
	char dfilename[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	int ret;
	int fnamelen;

	dp = opendir(device_iio_dir);
	if (NULL == dp)
		return -ENODEV;

	for (ent = readdir(dp); ent; ent = readdir(dp)) {
		if (strlen(ent->d_name) <= strlen(device_iio_device_name) ||
		    !strcmp(ent->d_name, ".") ||
		    !strcmp(ent->d_name, ".."))
			continue;

		if (strncmp(ent->d_name, device_iio_device_name,
			    strlen(device_iio_device_name)) == 0) {
			numstrlen = sscanf(ent->d_name +
					   strlen(device_iio_device_name),
					   "%d", &number);
			fnamelen = numstrlen + strlen(device_iio_dir) +
				   strlen(device_iio_device_name);
			if (fnamelen > DEVICE_IIO_MAX_FILENAME_LEN)
				continue;

			sprintf(dfilename,
				"%s%s%d/name",
				device_iio_dir,
				device_iio_device_name,
				number);
			devilceFile = fopen(dfilename, "r");
			if (!devilceFile)
				continue;

			ret = fscanf(devilceFile, "%s", dname);
			if (ret <= 0) {
				fclose(devilceFile);

				break;
			}

			if (strncmp(name, dname, strlen(dname)) == 0 &&
			    strlen(name) == strlen(dname)) {
				fclose(devilceFile);
				closedir(dp);

				return number;
			}

		fclose(devilceFile);
		}
	}

	closedir(dp);

	return -ENODEV;
}

int device_iio_utils::get_device_by_type(const char *type)
{
	struct dirent *ent;
	int number, numstrlen;
	FILE *devilceFile;
	DIR *dp;
	char dname[DEVICE_IIO_MAX_NAME_LENGTH];
	char dfilename[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	int ret;
	int fnamelen;

	dp = opendir(device_iio_dir);
	if (NULL == dp)
		return -ENODEV;

	for (ent = readdir(dp); ent; ent = readdir(dp)) {
		if (strlen(ent->d_name) <= strlen(device_iio_device_name) ||
		    !strcmp(ent->d_name, ".") ||
		    !strcmp(ent->d_name, ".."))
			continue;

		if (strncmp(ent->d_name, device_iio_device_name,
			    strlen(device_iio_device_name)) == 0) {
			numstrlen = sscanf(ent->d_name +
					   strlen(device_iio_device_name),
					   "%d", &number);
			fnamelen = numstrlen + strlen(device_iio_dir) +
				   strlen(device_iio_device_name);
			if (fnamelen > DEVICE_IIO_MAX_FILENAME_LEN)
				continue;

			sprintf(dfilename,
				"%s%s%d/name",
				device_iio_dir,
				device_iio_device_name,
				number);
			devilceFile = fopen(dfilename, "r");
			if (!devilceFile)
				continue;

			ret = fscanf(devilceFile, "%s", dname);
			if (ret <= 0) {
				fclose(devilceFile);

				break;
			}

			if (strncmp(type,
                        dname + (strlen(dname) - strlen(type)),
                        strlen(type)) == 0) {
				fclose(devilceFile);
				closedir(dp);

				return number;
			}

		fclose(devilceFile);
		}
	}

	closedir(dp);

	return -ENODEV;
}

int device_iio_utils::enable_sensor(char *device_dir, bool enable)
{
	char enable_file[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	int err;

	err = enable_channels(device_dir, enable);
	if (err < 0)
		return err;

	sprintf(enable_file, "%s/%s", device_dir, device_iio_buffer_enable);

	return sysfs_write_int(enable_file, enable);
}

int device_iio_utils::get_sampling_frequency_available(char *device_dir,
				struct device_iio_sampling_freqs *sfa)
{
	int err = 0;
	char sf_filaname[DEVICE_IIO_MAX_FILENAME_LEN];
	char *pch;
	char line[100];

	sfa->length = 0;

	err = sprintf(sf_filaname,
		      "%s/%s",
		      device_dir,
		      device_iio_sfa_filename);
	if (err < 0)
		return err;

	err = sysfs_read_str(sf_filaname, line, sizeof(line));
	if (err < 0)
		return err;

	pch = strtok(line," ,");
	while (pch != NULL) {
		sfa->freq[sfa->length] = atof(pch);
		pch = strtok(NULL, " ,");
		sfa->length++;
		if (sfa->length >= DEVICE_IIO_MAX_SAMP_FREQ_AVAILABLE)
			break;
	}

	return err < 0 ? err : 0;
}

int device_iio_utils::get_fifo_length(const char *device_dir)
{
	int ret;
	int len;
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN];

	/* read <iio:devicex>/hwfifo_watermark_max */
	ret = snprintf(tmp_filaname, DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s", device_dir, device_iio_hw_fifo_length);
	if (ret < 0)
		return 0;

	ret = sysfs_read_int(tmp_filaname, &len);
	if (ret < 0 || len <= 0)
		return 0;

	/* write "len * 2" -> <iio:devicex>/buffer/length */
	ret = snprintf(tmp_filaname, DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s", device_dir, device_iio_buffer_length);
	if (ret < 0)
		goto out;

	ret = sysfs_write_int(tmp_filaname, 2 * len);
	if (ret < 0)
		goto out;;

	/* write "1" -> <iio:devicex>/hwfifo_enabled */
	ret = snprintf(tmp_filaname, DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s", device_dir, device_iio_hw_fifo_enabled);
	if (ret < 0)
		goto out;

	/* used for compatibility with old iio API */
	ret = check_file(tmp_filaname);
	if (ret < 0 && errno == ENOENT)
		goto out;

	ret = sysfs_write_int(tmp_filaname, 1);
	if (ret < 0) {
		ALOGE("Failed to enable hw fifo: %s.", tmp_filaname);
	}

out:
	return len;
}

int device_iio_utils::set_sampling_frequency(char *device_dir,
					     unsigned int frequency)
{
	int ret;
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN];

	/* write "frequency" -> <iio:devicex>/sampling_frequency */
	ret = snprintf(tmp_filaname, DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s", device_dir, device_iio_sf_filename);

	return ret < 0 ? -ENOMEM : sysfs_write_int(tmp_filaname, frequency);
}

int device_iio_utils::set_hw_fifo_watermark(char *device_dir,
					    unsigned int watermark)
{
	int ret;
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN];

	/* write "watermark" -> <iio:devicex>/hwfifo_watermark */
	ret = snprintf(tmp_filaname, DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s", device_dir, device_iio_hw_fifo_watermark);

	return ret < 0 ? -ENOMEM : sysfs_write_int(tmp_filaname, watermark);
}

int device_iio_utils::hw_fifo_flush(char *device_dir)
{
	int ret;
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN];

	/* write "1" -> <iio:devicex>/hwfifo_flush */
	ret = snprintf(tmp_filaname, DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s", device_dir, device_iio_hw_fifo_flush);
	if (ret < 0)
		return -ENOMEM;

	return ret < 0 ? -ENOMEM : sysfs_write_int(tmp_filaname, 1);
}

int device_iio_utils::set_scale(const char *device_dir,
				float value,
				device_iio_chan_type_t device_type)
{
	int ret;
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN];
	char *scale_filename;

	switch (device_type) {
		case DEVICE_IIO_ACC:
			scale_filename = (char *)"in_accel_x_scale";
			break;
		case DEVICE_IIO_GYRO:
			scale_filename = (char *)"in_anglvel_x_scale";
			break;
		default:
			return -EINVAL;
	}

	/* write "1" -> <iio:devicex>/in_<device_type>_x_scale */
	ret = snprintf(tmp_filaname, DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s", device_dir, scale_filename);

	return ret < 0 ? -ENOMEM : sysfs_write_scale(tmp_filaname, value);
}

int device_iio_utils::get_scale(const char *device_dir, float *value,
				device_iio_chan_type_t device_type)
{
	int ret;
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN];
	char *scale_filename;

	switch (device_type) {
		case DEVICE_IIO_ACC:
			scale_filename = (char *)"in_accel_x_scale";
			break;
		case DEVICE_IIO_GYRO:
			scale_filename = (char *)"in_anglvel_x_scale";
			break;
		default:
			return -EINVAL;
	}

	/* read <iio:devicex>/in_<device_type>_x_scale */
	ret = snprintf(tmp_filaname, DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s", device_dir, scale_filename);

	return ret < 0 ? -ENOMEM : sysfs_read_scale(tmp_filaname, value);
}

int device_iio_utils::get_type(struct device_iio_info_channel *channel,
			       const char *device_dir, const char *name,
			       const char *post)
{
	DIR *dp;
	int ret;
	FILE *sysfsfp;
	unsigned padint;
	const struct dirent *ent;
	char signchar, endianchar;
	char dir[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	char type_name[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	char name_post[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	char filename[DEVICE_IIO_MAX_FILENAME_LEN + 1];

	/* Check string len */
	if (strlen(device_dir) +
	    strlen("scan_elements") + 1 > DEVICE_IIO_MAX_FILENAME_LEN)
		return -1;

	if (strlen(name) +
	    strlen("_type") + 1 > DEVICE_IIO_MAX_FILENAME_LEN)
		return -1;

	if (strlen(post) +
	    strlen("_type") + 1 > DEVICE_IIO_MAX_FILENAME_LEN)
		return -1;

	sprintf(dir, "%s/scan_elements", device_dir);
	sprintf(type_name, "%s_type", name);
	sprintf(name_post, "%s_type", post);

	dp = opendir(dir);
	if (dp == NULL)
		return -errno;

	while (ent = readdir(dp), ent != NULL) {
		if ((strcmp(type_name, ent->d_name) == 0) ||
		    (strcmp(name_post, ent->d_name) == 0)) {
			snprintf(filename, DEVICE_IIO_MAX_FILENAME_LEN,
					 "%s/%s", dir, ent->d_name);
			sysfsfp = fopen(filename, "r");
			if (sysfsfp == NULL)
				continue;

			/* scan format like "le:s16/16>>0" */
			ret = fscanf(sysfsfp, "%ce:%c%u/%u>>%u",
				     &endianchar,
				     &signchar,
				     &channel->bits_used,
				     &padint,
				     &channel->shift);
			if (ret < 0)
				continue;

			channel->be = (endianchar == 'b');
			channel->sign = (signchar == 's');
			channel->bytes = (padint >> 3);

			if (channel->bits_used == 64)
				channel->mask = ~0;
			else
				channel->mask = (1 << channel->bits_used) - 1;

			fclose(sysfsfp);
		}
	}

	closedir(dp);

	return 0;
}

int device_iio_utils::get_scale_available(const char *device_dir,
				   struct device_iio_scales *sa,
				   device_iio_chan_type_t device_type)
{
	int err = 0;
	FILE *fp;
	char tmp_name[DEVICE_IIO_MAX_FILENAME_LEN + 1];
	char *avl_name;
	char *pch;
	char line[DEVICE_IIO_MAX_FILENAME_LEN + 1];

	sa->length = 0;

	switch (device_type) {
		case DEVICE_IIO_ACC:
			avl_name = (char *)"in_accel_scale_available";
			break;
		case DEVICE_IIO_GYRO:
			avl_name = (char *)"in_anglvel_scale_available";
			break;
		case DEVICE_IIO_TEMP:
			avl_name = (char *)"in_temp_scale_available";
			break;
		default:
			return -EINVAL;
	}

	/* Check string len */
	if (strlen(device_dir) +
		strlen(avl_name) + 1 > DEVICE_IIO_MAX_FILENAME_LEN)
		return -1;

	sprintf(tmp_name, "%s/%s", device_dir, avl_name);

	fp = fopen(tmp_name, "r");
	if (fp == NULL)
		return -1;

	if (!fgets(line, sizeof(line), fp)) {
		err = -EINVAL;

		goto fpclose;
	}

	pch = strtok(line," ");
	while (pch != NULL) {
		sa->scales[sa->length] = atof(pch);
		pch = strtok(NULL, " ");
		sa->length++;

		if (sa->length >= DEVICE_IIO_SCALE_AVAILABLE)
			break;
	}

fpclose:
	fclose(fp);

	return err;
}

int device_iio_utils::support_injection_mode(const char *device_dir)
{
	int err;
	char injectors[30];
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN + 1];

	/* Check string len */
	if (strlen(device_dir) +
	    strlen(device_iio_injection_mode_enable) +
	    1 > DEVICE_IIO_MAX_FILENAME_LEN)
		return -1;

	sprintf(tmp_filaname,
		"%s/%s",
		device_dir,
		device_iio_injection_mode_enable);
	err = sysfs_read_int(tmp_filaname, &err);
	if (err < 0) {
		sprintf(tmp_filaname,
			"%s/%s",
			device_dir,
			device_iio_injection_sensors_filename);
		err = sysfs_read_str(tmp_filaname,
				     injectors,
				     sizeof(injectors));
		if (err < 0)
			return err;

		return 1;
	}

	return 0;
}

int device_iio_utils::set_injection_mode(const char *device_dir, bool enable)
{
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN];
	int ret = 0;

	/* write "enable" -> <iio:devicex>/injection_mode */
	ret = snprintf(tmp_filaname,
		       DEVICE_IIO_MAX_FILENAME_LEN,
		       "%s/%s",
		       device_dir,
		       device_iio_injection_mode_enable);

	return ret < 0 ? -ENOMEM : sysfs_write_int(tmp_filaname, enable);
}

int device_iio_utils::inject_data(const char *device_dir, unsigned char *data,
				  int len, device_iio_chan_type_t device_type)
{
	char *injection_filename;
	char tmp_filaname[DEVICE_IIO_MAX_FILENAME_LEN];
	FILE  *sysfsfp;
	int ret = 0;

	switch (device_type) {
		case DEVICE_IIO_ACC:
			injection_filename = (char *)"in_accel_injection_raw";
			break;
		case DEVICE_IIO_GYRO:
			injection_filename =
				(char *)"in_anglvel_injection_raw";
			break;
		default:
			return -EINVAL;
	}

	/* Check string len */
	if (strlen(device_dir) +
		strlen(injection_filename) + 1 > DEVICE_IIO_MAX_FILENAME_LEN)
		return -1;

	sprintf(tmp_filaname, "%s/%s", device_dir, injection_filename);

	sysfsfp = fopen(tmp_filaname, "w");
	if (sysfsfp == NULL)
		return -errno;

	ret = fwrite(data, len, 1, sysfsfp);
	fclose(sysfsfp);

	return ret;
}

int device_iio_utils::update_fsm_thresholds(char *threshold_data)
{
	char fsm_thresholds_filename[DEVICE_IIO_MAX_FILENAME_LEN];
	int ret, number;

	ret = get_device_by_type(device_iio_mlc_device_type);
	if (ret < 0) {
        ALOGE("%s: unable to detect device type %s",
              __FUNCTION__,
              device_iio_mlc_device_type);

		return ret;
    }

	number = ret;
	ret = snprintf(fsm_thresholds_filename,
				   DEVICE_IIO_MAX_FILENAME_LEN,
				   "%s%s%d/%s",
				   device_iio_dir,
				   device_iio_device_name,
				   number,
				   device_iio_fsm_threshold_filename);

	return ret < 0 ? -ENOMEM : sysfs_write_str(fsm_thresholds_filename,
                                               threshold_data);
}
