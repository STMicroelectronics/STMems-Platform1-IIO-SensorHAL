Index
=====
	* Introduction
	* Configure and Build test_linux
	* Copyright


Introduction
=========

SensorHal Linux Documentation

The **test_linux** application is an example that demonstrates how to reuse the **SensorHAL** library code even in a Linux environment

This test_linux code examples searches the SensorHAL shared library(*) in the following paths of the target board:

>    "/tmp/mnt/SensorHAL.so"
>    "/system/vendor/lib64/hw/SensorHAL.so"
>    "/system/vendor/lib/hw/SensorHAL.so"

Once found the SensorHAL shared library the test_linux example application executes the dlopen (open_hal routine) and looks for the symbol library HAL_MODULE_INFO_SYM_AS_STR (defined as "HMI"); if library is found on the target, test_linux launches the hmi-> common.methods-> open method of the SensorHAL

The SensorHAL open method will search for all IIO configured and supported MEMS sensor devices and add them to the list of managed sensors

At the end of the scanning procedure it will be possible to use the **poll_dev** and **hmi** structure methods to perform all common sensor operations, for examples:

#####  list of scanned IIO ST MEMS sensors:
    sensor_num = hmi->get_sensors_list(hmi, &list);

#####  getting reference to a sensor:
    handle = get_sensor(list, <sensor_type>, &sensor);

#####  acrivate a sensor:
    sensor_activate(handle, SENSOR_ENABLE);

#####  deactivate a sensor:
    sensor_activate(handle, SENSOR_DISABLE);

#####  setting ODR frequency (in Hz):
    static int sensor_setdelay(int type, int odr)
    {
        int handle;
        int64_t delay;
        struct sensor_t *sensor = NULL;

        delay = 1000000000 / odr;
        handle = get_sensor(list, type, &sensor);
        if (handle != INVALID_HANDLE && sensor) {
            return poll_dev->setDelay(&poll_dev->v0, handle, delay);
        }

        return -1;
    }

#####  flushing sensor data from hw FIFO:
    static int sensor_flush(int handle)
    {
        return poll_dev->flush(poll_dev, handle);
    }

#####  set batching configuration:
    static int sensor_batching(int64_t delay_time, int64_t batch_time)
    {
        int sindex = 0;
        int handle;
        struct sensor_t *sensor = NULL;

        while(test_sensor_type[sindex] != -1) {
            handle = get_sensor(list, test_sensor_type[sindex], &sensor);
            sensor_batch(handle, batch_time, delay_time);
            sindex++;
        }

        return 0;
    }

All the native SensorHAL methods have been implemented in the test_linux application

Furthermore it will be possible to use the mlc loader to load sensor (only for ASM330LHHX) .ucf files containing the binaries of the machine learning core and verify the operation by reading the events generated by the configured MLC
Some examples of ucf file can be taken from the local mlc_asm330lhhx folder or at the following link: [MLC UCF samples](https://github.com/STMicroelectronics/STMems_Machine_Learning_Core)

The test_linux example application has the following menu options:

    usage: ./test_linux [OPTIONS]

    OPTIONS:
        --accodr:       Set Accelerometer ODR (default 104)
        --gyroodr:      Set Gyroscope ODR (default 104)
        --list:         Show sensor list
        --nsample:      Number of samples (default 10)
        --flush:        Flush data before test
        --batch:        Set Batch Time in ns (default disabled)
        --delay:        Set Delay time in ns
        --timeout:      Set timeout (s) for receive samples (default 1 s)
        --sensor:       Select sensor (default all)
        --mlc:          Load a machine learning core ucf file
        --mlc_info:     Get MLC info
        --mlc_flush:    Flush MLC configuration
        --events:       Event monitoring (specify device as iio:device3)
        --logname:      Output log file (default log.txt)
        --notemp:       Disable temperature sensor
        --version:      Print Version
        --gyropm:       Set Gyro Power Mode (0 = HP, 1 = LP)
        --accpm:        Set Acc Power Mode (0 = HP, 1 = LP)
        --rotmat:       Update HAL rotation matrix (yaw,pitch,roll), data expressed
                        in tenth of a degree (i.e. 900 means 90 degree)
        --position:     Update HAL sensor position (x,y,z)
	--ign_cmd:      Run Ignition command on SensorHAL (data 0/1)
        --help:         This help

NOTE: (*) SensorHAL library must becompiled for linux by using the Makefile provided

Configure and Build test_linux
========

Is possible to use the SensorHAL configuration menu by setting reference to aosp:
>	ANDROID_BUILD_TOP=/<aosp path>/ PLATFORM_VERSION=<aosp version> make -f Makefile_config sensors-menuconfig

To build the test_linux sample application set CROSS_COMPILE environment accordingly to you target board, follow an examples for raspberry pi zero target:

>   PATH=$PATH:/local/home/raspy/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin
>   make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-


Copyright
========

Copyright (C) 2021 STMicroelectronics

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
