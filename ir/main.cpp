/*
 * Copyright (C) 2021 The Android Open Source Project
 *               2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "audio.h"

#include <aidl/android/hardware/ir/BnConsumerIr.h>
#include <aidl/android/hardware/ir/ConsumerIrFreqRange.h>
#include <android-base/logging.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <hardware/consumerir.h>
#include <numeric>

#include <log/log.h>

using ::aidl::android::hardware::ir::ConsumerIrFreqRange;

namespace aidl::android::hardware::ir {

class ConsumerIr : public BnConsumerIr {
  public:
    ConsumerIr();
  private:
    ::ndk::ScopedAStatus getCarrierFreqs(std::vector<ConsumerIrFreqRange>* _aidl_return) override;
    ::ndk::ScopedAStatus transmit(int32_t in_carrierFreqHz,
                                  const std::vector<int32_t>& in_pattern) override;
    consumerir_device_t *mDevice = nullptr;
    AudioService mAudioService;
};

ConsumerIr::ConsumerIr() {
    const hw_module_t *hw_module = NULL;

    int ret = hw_get_module(CONSUMERIR_HARDWARE_MODULE_ID, &hw_module);
    if (ret != 0) {
        ALOGE("hw_get_module %s failed: %d", CONSUMERIR_HARDWARE_MODULE_ID, ret);
        return;
    }
    ret = hw_module->methods->open(hw_module, CONSUMERIR_TRANSMITTER, (hw_device_t **) &mDevice);
    if (ret < 0) {
        // note - may want to make this a fatal error - otherwise the service will crash when it's used
        ALOGE("Can't open consumer IR transmitter, error: %d", ret);
        // in case it's modified
        mDevice = nullptr;
    }

    // Initialize the audio service
    mAudioService.initializeAudioService();

    // Initialize mParameter
    mAudioService.getIrdaSupportParameter();
}

::ndk::ScopedAStatus ConsumerIr::getCarrierFreqs(std::vector<ConsumerIrFreqRange>* _aidl_return) {
    int32_t len = mDevice->get_num_carrier_freqs(mDevice);
    if (len < 0) {
        (*_aidl_return).clear();
        return ::ndk::ScopedAStatus::ok();
    }

    consumerir_freq_range_t *rangeAr = new consumerir_freq_range_t[len];
    bool success = (mDevice->get_carrier_freqs(mDevice, len, rangeAr) >= 0);
    if (!success) {
        (*_aidl_return).clear();
        return ::ndk::ScopedAStatus::ok();
    }

    (*_aidl_return).resize(len);
    for (int32_t i = 0; i < len; i++) {
        (*_aidl_return)[i].minHz = static_cast<uint32_t>(rangeAr[i].min);
        (*_aidl_return)[i].maxHz = static_cast<uint32_t>(rangeAr[i].max);
    }
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ConsumerIr::transmit(int32_t in_carrierFreqHz,
                                          const std::vector<int32_t>& in_pattern) {
    if (in_carrierFreqHz > 0) {
        mAudioService.setTransmitParameter(true);
        mDevice->transmit(mDevice, in_carrierFreqHz, in_pattern.data(), in_pattern.size());
        mAudioService.setTransmitParameter(false);
        return ::ndk::ScopedAStatus::ok();
    } else {
        return ::ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
}

}  // namespace aidl::android::hardware::ir

using aidl::android::hardware::ir::ConsumerIr;

int main() {
    auto binder = ::ndk::SharedRefBase::make<ConsumerIr>();
    const std::string name = std::string() + ConsumerIr::descriptor + "/default";
    CHECK_EQ(STATUS_OK, AServiceManager_addService(binder->asBinder().get(), name.c_str()))
            << "Failed to register " << name;

    ABinderProcess_setThreadPoolMaxThreadCount(0);
    ABinderProcess_joinThreadPool();

    return EXIT_FAILURE;  // should not reached
}
