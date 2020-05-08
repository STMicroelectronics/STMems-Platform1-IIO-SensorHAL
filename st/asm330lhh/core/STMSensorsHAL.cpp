/*
 * Copyright (C) 2018 The Android Open Source Project
 * Copyright (C) 2019 STMicroelectronics
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

#include <functional>

#include <STMSensorsCallbackData.h>
#include <STMSensorsHAL.h>
#include <IUtils.h>
#include "sensors_legacy.h"

STMSensorsHAL::STMSensorsHAL(void)
              : sensorsCallback(&emptySTMSensorCallback),
                console(IConsole::getInstance())
{
    st_hal_open_sensors(&hal_data);

    t = new std::thread(mypoll, this);
}

void STMSensorsHAL::mypoll(STMSensorsHAL *hal)
{
    IConsole &console { IConsole::getInstance() };
    sensors_event_t sdata[10];

    while (1) {
        auto n = st_hal_dev_poll(hal->hal_data, sdata, 10);

        {
            std::vector<ISTMSensorsCallbackData> sensorsData;

            for (auto i = 0; i < n; ++i) {
                std::vector<float> data;
                data.push_back(sdata[i].data[0]);
                data.push_back(sdata[i].data[1]);
                data.push_back(sdata[i].data[2]);

                switch (sdata[i].type) {
                case SENSOR_TYPE_ACCELEROMETER:
                    sensorsData.push_back(STMSensorsCallbackData(sdata[i].sensor,
                                                                 STMSensorType::ACCELEROMETER,
                                                                 sdata[i].timestamp,
                                                                 data));
                    break;
                case SENSOR_TYPE_GYROSCOPE:
                    sensorsData.push_back(STMSensorsCallbackData(sdata[i].sensor,
                                                                 STMSensorType::GYROSCOPE,
                                                                 sdata[i].timestamp,
                                                                 data));
                    break;
                case SENSOR_TYPE_META_DATA:
                    console.debug("new flush data " + std::to_string(sdata[i].timestamp));
                    sensorsData.push_back(STMSensorsCallbackData(sdata[i].meta_data.sensor,
                                                                 STMSensorType::META_DATA,
                                                                 sdata[i].timestamp,
                                                                 data));
                    break;
                default:
                    console.error("sensors data received not supported");
                    break;
                }
            }

            if (sensorsData.size() > 0) {
                hal->sensorsCallback->onNewSensorsData(sensorsData);
            }
        }
    }
}

/**
 * initialize: implementation of an interface,
 *             reference: ISTMSensorsHAL.h
 */
void STMSensorsHAL::initialize(const ISTMSensorsCallback &sensorsCallback)
{
    this->sensorsCallback = (ISTMSensorsCallback *)&sensorsCallback;

    const std::vector<STMSensor> &sensorList = st_hal_get_sensors_list(hal_data).getList();

    for (auto sensor : sensorList) {
        activate(sensor.getHandle(), false);
    }
}

/**
 * getSensorslist: implementation of an interface,
 *                 reference: ISTMSensorsHAL.h
 */
const STMSensorsList& STMSensorsHAL::getSensorsList(void)
{
    return st_hal_get_sensors_list(hal_data);
}

/**
 * activate: implementation of an interface,
 *           reference: ISTMSensorsHAL.h
 */
int STMSensorsHAL::activate(uint32_t handle, bool enable)
{
    return st_hal_dev_activate(hal_data, handle, enable);
}

/**
 * setRate: implementation of an interface,
 *          reference: ISTMSensorsHAL.h
 */
int STMSensorsHAL::setRate(uint32_t handle,
                           int64_t samplingPeriodNanoSec,
                           int64_t maxReportLatencyNanoSec)
{
    return st_hal_dev_batch(hal_data, handle, 0, samplingPeriodNanoSec, maxReportLatencyNanoSec);
}

/**
 * flushData: implementation of an interface,
 *            reference: ISTMSensorsHAL.h
 */
int STMSensorsHAL::flushData(uint32_t handle)
{
    return st_hal_dev_flush(hal_data, handle);
}
