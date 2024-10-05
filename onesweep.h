#include <vector>
#include <cmath>
#include <ctime>
#include "easyvk.h"

namespace onesweep {
    struct OnesweepPerfStats {
        float hist;
        float bin1;
        float bin2;
        float bin3;
        float total;
    };

    OnesweepPerfStats onesweep(easyvk::Device device, uint32_t* data, uint64_t len);
}