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

#include <STMSensorStatus.h>

STMSensorStatus::STMSensorStatus(const STMSensorsList &sensorsList)
                : sensorsList(sensorsList)
{

}

bool STMSensorStatus::isActive(uint32_t handle)
{
    if (mapping.find(handle) == mapping.end()) {
        return false;
    }

    return mapping[handle].isActive();
}

bool STMSensorStatus::addToActive(uint32_t handle)
{
    if (mapping.find(handle) == mapping.end()) {
        return false;
    }

    mapping[handle].setActive();

    return true;
}

void STMSensorStatus::removeFromActive(uint32_t handle)
{
    mapping.erase(handle);
}

bool STMSensorStatus::addConfig(uint32_t handle,
                                int64_t samplingPeriodNanoSec,
                                int64_t maxReportLatencyNanoSec)
{
    const STMSensor *sensor;

    if (!sensorsList.getSensor(handle, &sensor)) {
        return false;
    }

    std::pair<std::map<uint32_t, SensorStatus>::iterator, bool> ret;
    ret = mapping.insert(std::pair<uint32_t, SensorStatus>
                         (handle, SensorStatus(*sensor, samplingPeriodNanoSec,
                                               maxReportLatencyNanoSec)));
    if (!ret.second) {
        //ret.first.
        return false;
    }

    return true;
}
