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
#include <regex>

#include <FileUtils.h>
#include <IIODevicesInterface.h>

IIODevicesInterface::IIODevicesInterface(void)
{
    deviceList.clear();

    scanDevices();

    for (auto &device : deviceList) {
        IIODevice *iioDevice = dynamic_cast<IIODevice *>(device.get());
        iioDevice->enable(false);
        iioDevice->setCurrentTimestampClock(IIOTimestampClock::BOOTTIME);
    }
}

IIODevicesInterface::~IIODevicesInterface(void)
{
    deviceList.clear();
}

IIODevicesInterface& IIODevicesInterface::getInstance(void)
{
    static IIODevicesInterface instance;

    return instance;
}

int IIODevicesInterface::scanDevices(void)
{
    std::vector<int> devicesAvailableIdList = getDevicesAvailableIdList();

    for (const auto &id : devicesAvailableIdList) {
        deviceList.push_back(std::move(std::make_unique<IIODevice>(id)));
    }
    if (devicesAvailableIdList.size() == 0) {
        return -ENODEV;
    }

    return 0;
}

const std::vector<int> IIODevicesInterface::getDevicesAvailableIdList(void)
{
    const std::regex iioDeviceRegex("(iio:device)([[:digit:]]+)");
    std::vector<int> idList;

    const std::vector<std::string> directoryList = FileUtils::listDirectories(IIODevice::iioDevicesDirPath);

    for (const auto &dirName : directoryList) {
        if (std::regex_match(dirName, iioDeviceRegex)) {
            idList.push_back(std::stoi(std::regex_replace(dirName, iioDeviceRegex,
                                                          std::string("$2")), nullptr));
        }
    }

    return idList;
}
