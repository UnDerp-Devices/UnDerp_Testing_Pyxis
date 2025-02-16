/*
 * Copyright (C) 2019 The LineageOS Project
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

#define LOG_TAG "FingerprintInscreenService"

#include "FingerprintInscreen.h"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <hardware_legacy/power.h>

#include <fstream>
#include <cmath>

#define COMMAND_NIT 10
#define PARAM_NIT_630_FOD 1
#define PARAM_NIT_NONE 0

#define DISPPARAM_PATH "/sys/devices/platform/soc/ae00000.qcom,mdss_mdp/drm/card0/card0-DSI-1/disp_param"
#define DISPPARAM_HBM_FOD_ON "0x20000"
#define DISPPARAM_HBM_FOD_OFF "0xE0000"

#define FOD_STATUS_PATH "/sys/devices/virtual/touch/tp_dev/fod_status"
#define FOD_STATUS_ON 1
#define FOD_STATUS_OFF 0

#define FOD_SENSOR_X 455
#define FOD_SENSOR_Y 1910
#define FOD_SENSOR_SIZE 190

#define FOD_ERROR 8
#define FOD_ERROR_VENDOR 6

namespace vendor {
namespace lineage {
namespace biometrics {
namespace fingerprint {
namespace inscreen {
namespace V1_1 {
namespace implementation {

template <typename T>
static T get(const std::string& path, const T& def) {
    std::ifstream file(path);
    T result;
    file >> result;
    return file.fail() ? def : result;
}

template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);
    file << value;
}

using ::android::base::GetProperty;

FingerprintInscreen::FingerprintInscreen() {
    xiaomiFingerprintService = IXiaomiFingerprint::getService();
}

Return<int32_t> FingerprintInscreen::getPositionX() {
    int result;
    std::string vOff = GetProperty(propFODOffset, "");

    if (vOff.length() < 3) {
        return FOD_SENSOR_X;
    }

    result = std::stoi(vOff.substr(0, vOff.find(",")));
    if (result < 1) {
        return FOD_SENSOR_X;
    }

    return result;
}

Return<int32_t> FingerprintInscreen::getPositionY() {
    int result;
    std::string vOff = GetProperty(propFODOffset, "");

    if (vOff.length() < 3) {
        return FOD_SENSOR_Y;
    }

    result = std::stoi(vOff.substr(vOff.find(",") + 1));
    if (result < 1) {
        return FOD_SENSOR_Y;
    }

    return result;
}

Return<int32_t> FingerprintInscreen::getSize() {
    int result, propW, propH;
    std::string vSize = GetProperty(propFODSize, "");

    if (vSize.length() < 7) {
        return FOD_SENSOR_SIZE;
    }

    propW = std::stoi(vSize.substr(0, vSize.find(",")));
    propH = std::stoi(vSize.substr(vSize.find(",") +1));

    result = fmax(propW, propH);
    if (result < 1) {
        return FOD_SENSOR_SIZE;
    }
    
    return result;
}

Return<void> FingerprintInscreen::onStartEnroll() {
    return Void();
}

Return<void> FingerprintInscreen::onFinishEnroll() {
    return Void();
}

Return<void> FingerprintInscreen::switchHbm(bool enabled) {
    if (enabled) {
        set(DISPPARAM_PATH, DISPPARAM_HBM_FOD_ON);
    } else {
        set(DISPPARAM_PATH, DISPPARAM_HBM_FOD_OFF);
    }
    return Void();
}

Return<void> FingerprintInscreen::onPress() {
    acquire_wake_lock(PARTIAL_WAKE_LOCK, LOG_TAG);
    xiaomiFingerprintService->extCmd(COMMAND_NIT, PARAM_NIT_630_FOD);
    return Void();
}

Return<void> FingerprintInscreen::onRelease() {
    release_wake_lock(LOG_TAG);
    xiaomiFingerprintService->extCmd(COMMAND_NIT, PARAM_NIT_NONE);
    return Void();
}

Return<void> FingerprintInscreen::onShowFODView() {
    set(FOD_STATUS_PATH, FOD_STATUS_ON);
    return Void();
}

Return<void> FingerprintInscreen::onHideFODView() {
    set(FOD_STATUS_PATH, FOD_STATUS_OFF);
    return Void();
}

Return<bool> FingerprintInscreen::handleAcquired(int32_t acquiredInfo, int32_t vendorCode) {
    LOG(ERROR) << "acquiredInfo: " << acquiredInfo << ", vendorCode: " << vendorCode << "\n";
    return false;
}

Return<bool> FingerprintInscreen::handleError(int32_t error, int32_t vendorCode) {
    LOG(ERROR) << "error: " << error << ", vendorCode: " << vendorCode << "\n";
    return error == FOD_ERROR && vendorCode == FOD_ERROR_VENDOR;
}

Return<void> FingerprintInscreen::setLongPressEnabled(bool) {
    return Void();
}

Return<int32_t> FingerprintInscreen::getDimAmount(int32_t brightness) {
    float alpha;
    int realBrightness = brightness * 2047 / 255;

    if (realBrightness > 500) {
        alpha = 1.0 - pow(realBrightness / 2047.0 * 430.0 / 600.0, 0.455);
    } else {
        alpha = 1.0 - pow(realBrightness / 1680.0, 0.455);
    }

    return 255 * alpha;
}

Return<bool> FingerprintInscreen::shouldBoostBrightness() {
    return false;
}

Return<void> FingerprintInscreen::setCallback(const sp<IFingerprintInscreenCallback>&) {
    return Void();
}

}  // namespace implementation
}  // namespace V1_1
}  // namespace inscreen
}  // namespace fingerprint
}  // namespace biometrics
}  // namespace lineage
}  // namespace vendor
