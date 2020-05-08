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

#pragma once

#include <string>
#include <vector>

#include <HWDevice.h>
#include <IIODeviceType.h>

enum class IIOTimestampClock {
    UNKNOWN,
    REALTIME,
    MONOTONIC,
    MONOTONIC_RAW,
    REALTIME_COARSE,
    MONOTONIC_COARSE,
    BOOTTIME,
    TAI,
};

enum class IIOType {
};

class IIODevice : public HWDevice {
public:
    IIODevice(unsigned int index);

    static const std::string iioDevicesDirPath;
    static std::string iioDeviceDirPath(unsigned int id);

    //ok
    std::string getName(void) const override;
    bool hasFifo(void) const override;
    bool hasHwTrigger(void) const override;
    int enable(bool enable) const override;
    int setSamplingFrequency(float frequency) const override;

    IIODeviceType getType(void) const;

    IIOTimestampClock getCurrentTimestampClock(void) const;
    int setCurrentTimestampClock(IIOTimestampClock clock) const;

    const std::vector<float>& getSamplingFrequencyAvailable(void) const;
    float getSamplingFrequency(void) const;

    int setBufferWatermark(int numSamples) const;

    int getDataAvailable(void) const;

private:
    static const std::string iioDeviceNameFilename;
    static const std::string timestampClockFilename;
    static const std::string samplingFrequencyFilename;
    static const std::string samplingFrequencyAvlFilename;
    static const std::string bufferEnableFilename;
    static const std::string bufferWatermarkFilename;
    static const std::string bufferDataAvailableFilename;

    std::string iioDeviceBasePath;
    IIODeviceType type;

    std::vector<float> samplingFrequencyList;
};
