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

#include <iostream> // TO REMOVE

#include <HWDevice.h>

HWDevice::HWDevice(int id)
        : deviceId(id)
{
    std::cout << "costruct " << std::to_string(id) << std::endl;
}

HWDevice::HWDevice(const HWDevice &device)
{
    std::cout << "copy costruct " << std::to_string(device.getId()) << std::endl;
}

HWDevice& HWDevice::operator=(const HWDevice &device)
{
    std::cout << "assignment operator " << std::to_string(device.getId()) << std::endl;

    return *this;
}

int HWDevice::getId(void) const
{
    return deviceId;
}
