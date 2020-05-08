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

#include <IIODeviceType.h>

class IIOUtils {
public:
    static int getName(unsigned int index, std::string &name);
    static int getDeviceType(unsigned int index, IIODeviceType &type);

    static int getDeviceTimestampClock(unsigned int index, IIOTimestampClock &clock);
    static int setDeviceTimestampClock(unsigned int index, IIOTimestampClock &clock);

    static int getDeviceSamplingFrequencyAvailable(unsigned int index,
                                                   std::vector<float> &frequencyList);
    static int getBufferSamplingFrequencyAvailable(unsigned int index,
                                                   std::vector<float> &frequencyList);
    static int getTriggerSamplingFrequencyAvailable(unsigned int index,
                                                    std::vector<float> &frequencyList);
};
