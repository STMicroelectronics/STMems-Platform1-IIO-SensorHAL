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

#include <cerrno>

#include <IIOUtils.h>

int IIOUtils::getName(unsigned int index, std::string &name)
{
    const std::string filename {"name"};

    (void) index;
    (void) name;

    return -ENODEV;
}

int IIOUtils::getDeviceType(unsigned int index, IIODeviceType &type)
{
    (void) index;
    (void) type;

    return -ENODEV;
}

int IIOUtils::getDeviceTimestampClock(unsigned int index, IIOTimestampClock &clock)
{
    (void) index;
    (void) clock;

    return -ENODEV;
}

int IIOUtils::setDeviceTimestampClock(unsigned int index, IIOTimestampClock &clock)
{
    (void) index;
    (void) clock;

    return -ENODEV;
}

int IIOUtils::getDeviceSamplingFrequencyAvailable(unsigned int index,
                                                  std::vector<float> &frequencyList)
{
    const std::string filename {"sampling_frequency_available"};

    (void) index;
    (void) frequencyList;

    return -ENODEV;
}

int IIOUtils::getBufferSamplingFrequencyAvailable(unsigned int index,
                                                  std::vector<float> &frequencyList)
{
    (void) index;
    (void) frequencyList;

    return -ENODEV;
}

int IIOUtils::getTriggerSamplingFrequencyAvailable(unsigned int index,
                                                   std::vector<float> &frequencyList)
{
    (void) index;
    (void) frequencyList;

    return -ENODEV;
}
