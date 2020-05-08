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

#include <FileUtils.h>
#include <IIODevice.h>

const std::string IIODevice::iioDevicesDirPath = "/mnt/development/linux/fake_sysfs/devices";
const std::string IIODevice::iioDeviceNameFilename = "name";
const std::string IIODevice::timestampClockFilename = "current_timestamp_clock";
const std::string IIODevice::samplingFrequencyFilename = "sampling_frequency";
const std::string IIODevice::samplingFrequencyAvlFilename = "sampling_frequency_available";
const std::string IIODevice::bufferEnableFilename = "buffer/enable";
const std::string IIODevice::bufferWatermarkFilename = "buffer/watermark";
const std::string IIODevice::bufferDataAvailableFilename = "buffer/data_available";

std::string IIODevice::iioDeviceDirPath(unsigned int id)
{
    static const std::string iioDeviceDirPrefixPath(IIODevice::iioDevicesDirPath + "/iio:device");

    return std::string(iioDeviceDirPrefixPath + std::to_string(id) + std::string("/"));
}

IIODevice::IIODevice(unsigned int index)
         : HWDevice(index),
           iioDeviceBasePath(iioDeviceDirPath(index)),
           type(IIODeviceType::UNKNOWN)
{
    FileUtils::readFloatList(iioDeviceBasePath + IIODevice::samplingFrequencyAvlFilename,
                             samplingFrequencyList);
    //    if (IIOUtils::getDeviceType(index, type) < 0) {
    //    valid = false;
    //}

    //TODO: scan samplingfrequency list
}

std::string IIODevice::getName(void) const
{
    std::string deviceName;

    FileUtils::readString(iioDeviceBasePath + IIODevice::iioDeviceNameFilename, deviceName);

    return deviceName;
}

bool IIODevice::hasFifo(void) const
{
    return false;
}

bool IIODevice::hasHwTrigger(void) const
{
    return false;
}

int IIODevice::enable(bool enable) const
{
    return FileUtils::writeInt(iioDeviceBasePath + IIODevice::bufferEnableFilename, enable);
}

IIODeviceType IIODevice::getType(void) const
{
    return type;
}

IIOTimestampClock IIODevice::getCurrentTimestampClock(void) const
{
    std::string value;

    auto err = FileUtils::readString(iioDeviceBasePath + IIODevice::timestampClockFilename, value);
    if (err < 0) {
        return IIOTimestampClock::UNKNOWN;
    }

    // parse string

    return IIOTimestampClock::UNKNOWN;
}

int IIODevice::setCurrentTimestampClock(IIOTimestampClock clock) const
{
    std::string clockString;

    switch (clock) {
    case IIOTimestampClock::REALTIME:
        clockString = "realtime";
        break;
    case IIOTimestampClock::MONOTONIC:
        clockString = "monotonic";
        break;
    case IIOTimestampClock::MONOTONIC_RAW:
        clockString = "monotonic_raw";
        break;
    case IIOTimestampClock::REALTIME_COARSE:
        clockString = "realtime_coarse";
        break;
    case IIOTimestampClock::MONOTONIC_COARSE:
        clockString = "monotonic_coarse";
        break;
    case IIOTimestampClock::BOOTTIME:
        clockString = "boottime";
        break;
    case IIOTimestampClock::TAI:
        clockString = "tai";
        break;
    default:
        return -EINVAL;
    }

    return FileUtils::writeString(iioDeviceBasePath + IIODevice::timestampClockFilename,
                                  clockString);
}

const std::vector<float>& IIODevice::getSamplingFrequencyAvailable(void) const
{
    return samplingFrequencyList;
}

float IIODevice::getSamplingFrequency(void) const
{
    return FileUtils::readFloat(iioDeviceBasePath + IIODevice::samplingFrequencyFilename);
}

int IIODevice::setSamplingFrequency(float frequency) const
{
    for (auto &freq : samplingFrequencyList) {
        if (frequency == freq) {
            return FileUtils::writeFloat(iioDeviceBasePath + IIODevice::samplingFrequencyFilename,
                                         frequency);
        }
    }

    return -EINVAL;
}

int IIODevice::setBufferWatermark(int numSamples) const
{
    return FileUtils::writeInt(iioDeviceBasePath + IIODevice::bufferWatermarkFilename, numSamples);
}

int IIODevice::getDataAvailable(void) const
{
    return FileUtils::readInt(iioDeviceBasePath + IIODevice::bufferDataAvailableFilename);
}
