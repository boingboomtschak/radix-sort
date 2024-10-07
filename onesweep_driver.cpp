#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include "easyvk.h"
#include "onesweep.h"

#define USE_VALIDATION_LAYERS true
#define DATA_SIZE (1llu << 24)
#define PRINT_ROWS 10
#define PRINT_COLS 10

using namespace easyvk;
using namespace onesweep;
using std::string;
using std::vector;

void print_data(vector<uint32_t> data) {
    printf("+");
    for (size_t i = 0; i < PRINT_COLS; i++)
        printf("-------+");
    printf("\n");
    for (size_t i = 0; i < PRINT_ROWS; i++) {
        printf("| ");
        for (size_t j = 0; j < PRINT_COLS; j++) {
            printf("%5u | ", data[i * PRINT_COLS + j]);
        }
        printf("\n");
    }
    printf("+");
    for (size_t i = 0; i < PRINT_COLS; i++)
        printf("-------+");
    printf("\n\n");
}

int main() {
    FILE *res_file;
    Instance instance = Instance(USE_VALIDATION_LAYERS);
    std::vector<VkPhysicalDevice> devices = instance.physicalDevices();
    if (devices.size() == 0) {
        fprintf(stderr, "No Vulkan devices available!\n");
        return 1;
    }
    printf("Testing Onesweep radix sort...\n");
    
    int d = 0;
    if (devices.size() > 1) {
        printf("Select a device: \n");
        for (int i = 0; i < devices.size(); i++) {
            Device device = Device(instance, devices[i]);
            printf("%d - '%s'\n", i, device.properties.deviceName);
            device.teardown();
        }
        printf("Enter device number: ");
        std::string d_s; 
        getline(std::cin, d_s);
        d = stoi(d_s);
        if (d < 0 || d >= devices.size()) {
            fprintf(stderr, "Incorrect device number '%d'!", d);
            exit(1);
        }
    }
   
    // Initialize device
    Device device = Device(instance, devices[d]);
    printf("Using '%s'...\n\n", device.properties.deviceName);

    std::vector<uint64_t> test_sizes;
    for (int i = 7; i < 32; i++) {
        test_sizes.push_back((1llu << i));
    }

    char fname[128];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(fname, 128, "%d-%02d-%02d_%02d_%02d_%s.sortperf", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, device.vendorName());
    printf("Opening '%s' for results...\n", fname);
    res_file = fopen(fname, "w");

    printf("Data Size   | Histogram (%% total) | Binning 1 (%% total) | Binning 2 (%% total) | Binning 3 (%% total) | Total Runtime\n");
    for (int i = 0; i < test_sizes.size(); i++) {
        // Set up CPU buffer
        uint64_t data_size = test_sizes[i];
        uint64_t len = data_size / sizeof(uint32_t);
        vector<uint32_t> data(len);

        // Fill buffer with random numbers
        srand(time(NULL));
        std::generate(data.begin(), data.end(), std::rand);

        OnesweepPerfStats perf = onesweep::onesweep(device, data.data(), data.size());

        bool sorted = true;
        for (size_t i = 0; i < len - 1; i++) {
            if (data[i] > data[i+1])
                sorted = false;
        }

        if (!sorted) {
            fprintf(stderr, "Error: Data not sorted! Dumping data and exiting...\n");
            print_data(data);
            fclose(res_file);
            device.teardown();
            instance.teardown();
            return 1;
        }
        
        char hist[32], bin1[32], bin2[32], bin3[32], bin4[32];
        snprintf(hist, 32, "%3.4fms (%2.2f%%)", perf.hist, perf.hist / perf.total * 100.0);
        snprintf(bin1, 32, "%3.4fms (%2.2f%%)", perf.bin1, perf.bin1 / perf.total * 100.0);
        snprintf(bin2, 32, "%3.4fms (%2.2f%%)", perf.bin2, perf.bin2 / perf.total * 100.0);
        snprintf(bin3, 32, "%3.4fms (%2.2f%%)", perf.bin3, perf.bin3 / perf.total * 100.0);
        snprintf(bin4, 32, "%3.4fms (%2.2f%%)", perf.bin4, perf.bin4 / perf.total * 100.0);

        printf("%-11lu | %-19s | %-19s | %-19s | %-19s | %-19s | %3.4fms\n",
            data_size, 
            hist,
            bin1,
            bin2,
            bin3,
            bin4,
            perf.total
        );
        fprintf(res_file, "%lu, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n",
            data_size,
            perf.hist, perf.hist / perf.total * 100.0,
            perf.bin1, perf.bin1 / perf.total * 100.0,
            perf.bin2, perf.bin2 / perf.total * 100.0,
            perf.bin3, perf.bin3 / perf.total * 100.0,
            perf.bin4, perf.bin4 / perf.total * 100.0,
            perf.total
        );
        fflush(res_file);
    }

    fclose(res_file);
    device.teardown();
    instance.teardown();
    return 0;
}
