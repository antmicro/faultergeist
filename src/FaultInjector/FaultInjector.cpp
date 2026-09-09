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

#include "Event.h"
#include "EventParser.h"
#include "ManagedVpiHandle.h"
#include "Utils.h"

#include "vpi_user.h"

#include <bitset>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <queue>
#include <sstream>
#include <string>

namespace fin {

class FaultInjector {
    ManagedVpiHandle vh_value_cb;
    EventParser eventParser;

    std::priority_queue<Event> transient_events;

    std::optional<Event> leftover_event;

   public:
    FaultInjector(const std::string& input_file)
        : vh_value_cb{registerCb()}, eventParser{input_file} {
        if (eventParser.ok()) {
            (void)simulateSingleEventEffects();
        } else {
            fin_fatal("%%Error: Couldn't parse '%s'\n", input_file.c_str());
        }
    }

    ~FaultInjector() = default;

    FaultInjector(const FaultInjector&) = delete;
    FaultInjector& operator=(const FaultInjector&) = delete;

   private:
    ManagedVpiHandle registerCb() {
        fin_printf("- Registering cbNextSimTime callback\n");

        s_cb_data cb_data{};
        s_vpi_time time = {vpiSimTime, 0, 1, 0};
        cb_data.time = &time;
        cb_data.reason = cbAfterDelay;
        cb_data.cb_rtn = AtNextSimTimeCallback;
        cb_data.user_data = reinterpret_cast<PLI_BYTE8*>(this);
        cb_data.value = nullptr;
        ManagedVpiHandle handle = vpi_register_cb(&cb_data);
        assert(handle.valid());
        return handle;
    }

    static PLI_INT32 AtNextSimTimeCallback(t_cb_data* data) {
        auto* self = reinterpret_cast<FaultInjector*>(data->user_data);
        auto t = getVpiTime();
        const std::uint64_t current_time = timeValue(t);
        fin_printf(
            const_cast<char*>("- [@%llu] %s iterating\n"),
            static_cast<unsigned long long>(current_time),
            cbReasonToString(data->reason)
        );

        const std::uint64_t next_event_time = self->simulateSingleEventEffects();
        if (next_event_time == 0) {
            fin_printf(
                const_cast<char*>("- [@%llu] Last fault emitted\n"),
                static_cast<unsigned long long>(current_time)
            );
            return 0;
        }
        const std::uint64_t next_time = next_event_time - current_time;

        s_cb_data cb_data{};
        s_vpi_time time = toVpiTime(next_time);
        cb_data.cb_rtn = AtNextSimTimeCallback;
        cb_data.obj = nullptr;
        cb_data.reason = data->reason;
        cb_data.time = &time;
        cb_data.user_data = reinterpret_cast<PLI_BYTE8*>(self);

        fin_printf(
            const_cast<char*>("- [@%llu] Registering next %s Callback, next event at @%llu\n"),
            static_cast<unsigned long long>(current_time),
            cbReasonToString(cb_data.reason),
            static_cast<unsigned long long>(next_event_time)
        );

        ManagedVpiHandle handle = vpi_register_cb(&cb_data);
        assert(handle.handle());
        return 0;
    }

    void simulateSingleEventEffect(const s_vpi_time& time, const Event& event) {
        switch (event.type) {
            case Event::Type::SingleEventTransientUpset:
                simulateSingleEventTransient(time, event);
                break;
            case Event::Type::SingleEventTransientRollback:
                simulateSingleEventTransientRollback(time, event);
                break;
            case Event::Type::SingleEventUpset:
                simulateSingleEventUpset(time, event);
                break;
        }
    }

    std::uint64_t simulateSingleEventEffects() {
        auto t = getVpiTime();
        const std::uint64_t current_time = timeValue(t);

        while (!eventParser.eof()) {
            if (!leftover_event) {
                leftover_event = eventParser.parse();
                if (!leftover_event) {
                    continue;
                }
            }

            if (transient_events.empty() || transient_events.top().time > leftover_event->time) {
                if (leftover_event->time > current_time) {
                    return leftover_event->time;
                }
                simulateSingleEventEffect(t, *leftover_event);
                leftover_event.reset();
            } else {
                if (transient_events.top().time > current_time) {
                    return transient_events.top().time;
                }
                simulateSingleEventEffect(t, transient_events.top());
                transient_events.pop();
            }
        }
        while (!transient_events.empty()) {
            if (transient_events.top().time > current_time) {
                return transient_events.top().time;
            }
            simulateSingleEventEffect(t, transient_events.top());
            transient_events.pop();
        }
        return 0;
    }

    void simulateSingleEventTransient(const s_vpi_time& time, const Event& event) {
        fin_printf(const_cast<char*>("- [@%d] Simulating single-event transient\n"), time.low);

        s_vpi_value vpi_value{};
        vpi_value.format = vpiIntVal;
        vpi_get_value(event.handle(), &vpi_value);
        auto transient_event = Event{
            .signal = event.signal,
            .time = event.time + 1 /*duration of transient effect*/,
            .bit_idx = event.bit_idx,
            .type = Event::Type::SingleEventTransientRollback,
            .vpi_value = vpi_value,
        };
        fin_printf(
            const_cast<char*>("- [@%d] SET: before flipping %d bit of %.*s: %d\n"),
            time.low,
            event.bit_idx,
            (int)event.sig_path().size(),
            event.sig_path().data(),
            vpi_value.value.integer
        );
        vpi_value.value.integer ^= 1 << event.bit_idx;
        fin_printf(
            const_cast<char*>("- [@%d] SET: after flipping %d bit of %.*s: %d\n"),
            time.low,
            event.bit_idx,
            (int)event.sig_path().size(),
            event.sig_path().data(),
            vpi_value.value.integer
        );
        vpi_put_value(event.handle(), &vpi_value, nullptr, vpiForceFlag);

        // Insert after all ops on event as insert invalidates it.
        transient_events.push(std::move(transient_event));
    }

    void simulateSingleEventTransientRollback(const s_vpi_time& time, const Event& event) {
        fin_printf(const_cast<char*>("- [@%d] Rollback single-event transient\n"), time.low);
        assert(event.vpi_value.has_value());

        s_vpi_value vpi_value{};
        vpi_value.format = vpiIntVal;
        vpi_get_value(event.handle(), &vpi_value);

        fin_printf(
            const_cast<char*>("- [@%d] SET: before rollback %d bit of %.*s: %d\n"),
            time.low,
            event.bit_idx,
            (int)event.sig_path().size(),
            event.sig_path().data(),
            vpi_value.value.integer
        );
        vpi_value = event.vpi_value.value();
        fin_printf(
            const_cast<char*>("- [@%d] SET: after rollback %d bit of %.*s: %d\n"),
            time.low,
            event.bit_idx,
            (int)event.sig_path().size(),
            event.sig_path().data(),
            vpi_value.value.integer
        );
        vpi_put_value(event.handle(), &vpi_value, nullptr, vpiReleaseFlag);
    }

    std::string vpiVectorToString(s_vpi_value vpi_value, int vpi_size) {
        std::stringstream ss;
        constexpr int word_size =
            std::numeric_limits<decltype(vpi_value.value.vector->aval)>::digits;
        for (int i = std::max(vpi_size / word_size - 1, 0); i >= 0; --i) {
            std::bitset<word_size> word(vpi_value.value.vector[i].aval);
            ss << word.to_string();
        }
        return ss.str();
    }

    void simulateSingleEventUpset(const s_vpi_time& time, const Event& event) {
        fin_printf(const_cast<char*>("- [@%d] Simulating single-event upset\n"), time.low);

        s_vpi_value vpi_value{};
        vpi_value.format = vpiVectorVal;
        vpi_get_value(event.handle(), &vpi_value);

        fin_printf(
            const_cast<char*>("- [@%d] SEU: before flipping %d bit of %.*s: %s\n"),
            time.low,
            event.bit_idx,
            (int)event.sig_path().size(),
            event.sig_path().data(),
            vpiVectorToString(vpi_value, event.signal->vpi_width).data()
        );
        vpi_value.value.vector[event.bit_idx / 32].aval ^= 1 << (event.bit_idx % 32);
        fin_printf(
            const_cast<char*>("- [@%d] SEU: after flipping %d bit of %.*s: %s\n"),
            time.low,
            event.bit_idx,
            (int)event.sig_path().size(),
            event.sig_path().data(),
            vpiVectorToString(vpi_value, event.signal->vpi_width).data()
        );
        vpi_put_value(event.handle(), &vpi_value, nullptr, vpiNoDelay);
    }

    static s_vpi_time getVpiTime() {
        s_vpi_time t;
        t.type = vpiSimTime;
        vpi_get_time(0, &t);
        return t;
    }

    static std::uint64_t timeValue(const s_vpi_time& time) {
        return (static_cast<std::uint64_t>(time.high) << 32) | time.low;
    }

    static s_vpi_time toVpiTime(std::uint64_t time) {
        return {vpiSimTime, static_cast<PLI_UINT32>(time >> 32), static_cast<PLI_UINT32>(time), 0};
    }

#define STRINGIFY_CB_CASE(_cb) \
    case _cb: \
        return #_cb

    static const char* cbReasonToString(int cb_name) {
        switch (cb_name) {
            STRINGIFY_CB_CASE(cbAtStartOfSimTime);
            STRINGIFY_CB_CASE(cbReadWriteSynch);
            STRINGIFY_CB_CASE(cbReadOnlySynch);
            STRINGIFY_CB_CASE(cbNextSimTime);
            STRINGIFY_CB_CASE(cbStartOfSimulation);
            STRINGIFY_CB_CASE(cbEndOfSimulation);
            STRINGIFY_CB_CASE(cbAtEndOfSimTime);
            STRINGIFY_CB_CASE(cbAfterDelay);
            default:
                return "Unsupported callback";
        }
    }

#undef STRINGIFY_CB_CASE
};

}  // namespace fin

// Store pointer to FaultInjector here to exclude it from traces
static std::unique_ptr<fin::FaultInjector> fi;

extern "C" {

void FaultergeistCreate(const char* input_file) {
    std::printf("Faultergeist Init\n");
    assert(!fi);
    fi = std::make_unique<fin::FaultInjector>(input_file);
}
void FaultergeistDestroy(void) {
    std::printf("Faultergeist Teardown\n");
    assert(fi);
    fi.reset();
}
}
