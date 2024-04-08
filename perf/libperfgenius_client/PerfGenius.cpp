/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "libperfgenius_client"

#include <android-base/logging.h>
#include <vector>

#include "PerfGenius.h"

int perfRequest(uint32_t dur, [[maybe_unused]] const std::vector<ClientPerfRequestInfo>& requests,
                [[maybe_unused]] unsigned long& outParam) {
    LOG(INFO) << __func__ << ": dur: " << dur;
    return 0;
}

int perfRelease(unsigned long hdl) {
    LOG(INFO) << __func__ << ": hdl: " << hdl;
    return 0;
}
