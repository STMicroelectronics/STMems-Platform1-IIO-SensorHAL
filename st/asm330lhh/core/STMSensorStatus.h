/*
 * Copyright (C) 2018 The Android Open Source Project
 * Copyright (C) 2019 STMicroelectronics
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
n *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <map>
#include <STMSensor.h>
#include <STMSensorsList.h>

class STMSensorStatus {
public:
    STMSensorStatus(const STMSensorsList &sensorsList);
    ~STMSensorStatus(void) {};

    bool isActive(uint32_t handle);
    bool addToActive(uint32_t handle);
    void removeFromActive(uint32_t handle);

    bool addConfig(uint32_t handle,
                   int64_t samplingPeriodNanoSec,
                   int64_t maxReportLatencyNanoSec);

private:
    class SensorStatus {
    public:
        SensorStatus(void) {};
        SensorStatus(const STMSensor &sensor,
                     int64_t samplingPeriodNanoSec,
                     int64_t maxReportLatencyNanoSec)
            : active(false),
              samplingPeriodNanoSec(samplingPeriodNanoSec),
              maxReportLatencyNanoSec(maxReportLatencyNanoSec)
        {
            this->sensor = &sensor;
        }

        bool isActive(void) { return  active; }
        void setActive(void) { active = true; }
        int64_t getSamplingPeriod(void) { return samplingPeriodNanoSec; }
        int64_t getMaxReportLatency(void) { return  maxReportLatencyNanoSec; }
    private:
        const STMSensor *sensor;
        bool active;
        int64_t samplingPeriodNanoSec;
        int64_t maxReportLatencyNanoSec;
    };

    const STMSensorsList &sensorsList;
    std::map<uint32_t, SensorStatus> mapping;

};
