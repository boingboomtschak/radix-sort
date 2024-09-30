#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "easyvk.h"
#include "onesweep.h"

#define USE_VALIDATION_LAYERS true
#define DATA_SIZE 1llu << 31

using namespace easyvk;
using namespace onesweep;

int main() {
    Instance instance = Instance(USE_VALIDATION_LAYERS);
    std::vector<VkPhysicalDevice> devices = instance.physicalDevices();
    printf("Testing Onesweep radix sort...");

    printf("Select a device: \n");
    for (int i = 0; i < devices.size(); i++) {
        Device device = Device(instance, devices[i]);
        printf("%d - '%s'\n", i, device.properties.deviceName);
        device.teardown();
    }
    printf("Enter device number: ");

    std::string d_s; 
    getline(std::cin, d_s);
    printf("\n");
    int d = stoi(d_s);

    if (d < 0 || d >= devices.size()) {
        fprintf(stderr, "Incorrect device number '%d'!", d);
        exit(1);
    }

    // Initialize device
    Device device = Device(instance, devices[d]);
    printf("Using '%s'...\n\n", device.properties.deviceName);

    std::vector<uint32_t> data(DATA_SIZE / sizeof(uint32_t));
    srand(time(NULL));
    std::generate(data.begin(), data.end(), std::rand);

    printf("Unsorted data: ");
    for (size_t i = 0; i < 100; i++) {
        printf("%u ", data[i]);
    }
    printf("\n\n");

    onesweep::onesweep(device, data.data(), data.size());

    printf("Sorted data: ");
    for (size_t i = 0; i < 100; i++) {
        printf("%u ", data[i]);
    }
    printf("\n\n");

    device.teardown();
    instance.teardown();
    return 0;
}