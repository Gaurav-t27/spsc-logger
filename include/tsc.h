#pragma once
#include <cstdint>
#include <ctime>

namespace logger {

class Tsc {
public:

    static uint64_t now() noexcept {
        return __builtin_ia32_rdtsc();
    }


    struct Calibration {

        uint64_t tsc_base;    // TSC reading at calibration time
        uint64_t ns_base;     // wall clock ns-since-epoch at calibration time
        double   ns_per_tsc;  // conversion factor: nanoseconds per TSC tick

        // Measures ns_per_tsc empirically by sampling RDTSC and clock_gettime
        // over ~1ms. Call once from the consumer thread at startup.
        static Calibration measure() noexcept {
            struct timespec ts1, ts2;
            uint64_t tsc1, tsc2;

            // Sample wall clock and TSC as close together as possible.
            // The few nanoseconds between the two reads become a fixed offset
            // error — same for every record, so relative ordering is preserved.
            clock_gettime(CLOCK_REALTIME, &ts1);
            tsc1 = Tsc::now();

            // Busy-wait ~1ms so we get a stable frequency measurement.
            // Longer wait = more accurate ns_per_tsc.
            struct timespec deadline = ts1;
            deadline.tv_nsec += 1'000'000;
            if (deadline.tv_nsec >= 1'000'000'000) {
                deadline.tv_sec  += 1;
                deadline.tv_nsec -= 1'000'000'000;
            }
            do { clock_gettime(CLOCK_REALTIME, &ts2); }
            while (ts2.tv_sec  <  deadline.tv_sec  ||
                  (ts2.tv_sec  == deadline.tv_sec   &&
                   ts2.tv_nsec <  deadline.tv_nsec));
            tsc2 = Tsc::now();

            // Convert timespec → nanoseconds since epoch
            uint64_t ns1 = static_cast<uint64_t>(ts1.tv_sec)  * 1'000'000'000ULL
                         + static_cast<uint64_t>(ts1.tv_nsec);
            uint64_t ns2 = static_cast<uint64_t>(ts2.tv_sec)  * 1'000'000'000ULL
                         + static_cast<uint64_t>(ts2.tv_nsec);

            double ns_per_tsc = static_cast<double>(ns2 - ns1)
                              / static_cast<double>(tsc2 - tsc1);

            return { tsc1, ns1, ns_per_tsc };
        }

        uint64_t toNs(uint64_t tsc) const noexcept {
            return ns_base +
                   static_cast<uint64_t>((tsc - tsc_base) * ns_per_tsc);
        }
    };
};

} // namespace logger
