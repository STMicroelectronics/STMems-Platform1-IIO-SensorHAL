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

#include "SensorsCallback.h"

namespace android {
namespace hardware {
namespace sensors {
namespace V2_0 {
namespace implementation {

Return<void>
SensorsCallback::onDynamicSensorsConnected(const hidl_vec<V1_0::SensorInfo> &sensorInfos)
{
    (void) sensorInfos;
    // TODO implement
    return Void();
}

Return<void>
SensorsCallback::onDynamicSensorsDisconnected(const hidl_vec<int32_t> &sensorHandles)
{
    (void) sensorHandles;
    // TODO implement
    return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android
