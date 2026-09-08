// Copyright 2026 Antmicro <antmicro.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <absl/base/log_severity.h>
#include <absl/flags/internal/private_handle_accessor.h>
#include <absl/flags/reflection.h>
#include <absl/log/check.h>
#include <absl/log/globals.h>
#include <absl/log/initialize.h>
#include <absl/log/internal/globals.h>
#include <absl/log/log.h>

#ifndef NDEBUG
#define SEE_CHECK(condition) CHECK(condition)
#define SEE_PCHECK(condition) PCHECK(condition)
#define SEE_INTERNAL_CHECK(condition) CHECK(condition)
#else
#define SEE_CHECK(condition) \
    if (condition) \
        ; \
    else \
        LOG(QFATAL)

#define SEE_PCHECK(condition) \
    if (condition) \
        ; \
    else \
        LOG(QFATAL).WithPerror()
#define SEE_INTERNAL_CHECK(condition) \
    if (condition) \
        ; \
    else \
        LOG(QFATAL).WithPerror() << "Internal error"; \
    VLOG(10000)
#endif

inline void LogInitialize() {
    static bool initialized = false;
    if (initialized) {
        return;
    }
    const auto* stderrthreshold = absl::FindCommandLineFlag("stderrthreshold");
    if (stderrthreshold == nullptr ||
        !absl::flags_internal::PrivateHandleAccessor::IsSpecifiedOnCommandLine(*stderrthreshold)) {
#ifndef NDEBUG
        absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
#else
        absl::SetStderrThreshold(absl::LogSeverityAtLeast::kWarning);
#endif
    }

#ifdef NDEBUG
    absl::EnableLogPrefix(false);
#endif
    absl::InitializeLog();
    initialized = true;
}
