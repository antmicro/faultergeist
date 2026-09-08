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

#include "vpi_user.h"

#include <cstdio>
#include <stdexcept>
#include <string>

#define UNEXPECTED_VPI_CALL() \
    throw std::runtime_error( \
        std::string{"Mock function '"} + __func__ + "' in " __FILE__ " is not to be called" \
    )

PLI_DLLISPEC PLI_INT32 vpi_release_handle(vpiHandle) {
    UNEXPECTED_VPI_CALL();
}

PLI_DLLISPEC vpiHandle vpi_iterate(PLI_INT32, vpiHandle) {
    UNEXPECTED_VPI_CALL();
}

PLI_DLLISPEC vpiHandle vpi_scan(vpiHandle) {
    UNEXPECTED_VPI_CALL();
}

PLI_DLLISPEC PLI_BYTE8* vpi_get_str(PLI_INT32, vpiHandle) {
    UNEXPECTED_VPI_CALL();
}

PLI_INT32 vpi_control(PLI_INT32, ...) {
    UNEXPECTED_VPI_CALL();
}

PLI_DLLISPEC PLI_INT32 vpi_get(PLI_INT32, vpiHandle) {
    UNEXPECTED_VPI_CALL();
}

PLI_DLLISPEC PLI_INT32 vpi_vprintf(PLI_BYTE8* format, va_list ap) {
    return vfprintf(stderr, format, ap);
}
