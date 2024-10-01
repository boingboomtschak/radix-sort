#include "onesweep.h"

namespace onesweep {
    std::vector<uint32_t> read_spirv(const char *filename) {
        auto fin = std::ifstream(filename, std::ios::binary | std::ios::ate);
        if (!fin.is_open())
            throw std::runtime_error(std::string("failed opening file ") + filename + " for reading");
        const auto stream_size = unsigned(fin.tellg());
        fin.seekg(0);
        auto ret = std::vector<std::uint32_t>((stream_size + 3) / 4, 0);
        std::copy(std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>(), reinterpret_cast<char *>(ret.data()));
        return ret;
    }

    void onesweep(easyvk::Device device, uint32_t* data, uint64_t len) {
        // copy data to GPU
        easyvk::Buffer d_data = easyvk::Buffer(device, len * sizeof(uint32_t), true);
        uint64_t data_size = len * sizeof(uint32_t);

        // If data > 1GB, copy it over in 1GB intervals so as to not overflow memory with staging buffer size
        if (data_size > (1llu << 30)) {
            for (uint64_t i = 0; i < data_size; i += (1llu << 30)) {
                uint64_t chunk_size = (1llu << 30);
                if ((data_size - i) < chunk_size)
                    chunk_size = data_size - i;
                d_data.store(data, chunk_size, i, i);
            }
        } else {
            d_data.store(data, data_size);
        }

        const auto binnig_thread_blocks = (len + 7680 - 1) / 7680;

        easyvk::Buffer d_sorted = easyvk::Buffer(device, len * sizeof(uint32_t), true);
        easyvk::Buffer d_hist = easyvk::Buffer(device, 1024 * sizeof(uint32_t), true);
        d_hist.clear();
        easyvk::Buffer d_meta = easyvk::Buffer(device, 3 * sizeof(uint32_t), true);
        d_meta.clear();
        d_meta.store(&len, sizeof(uint32_t), 0, 2 * sizeof(uint32_t));
        easyvk::Buffer d_index = easyvk::Buffer(device, 4 * sizeof(uint32_t), true);
        d_index.clear();
        easyvk::Buffer d_pass_hist = easyvk::Buffer(device, (uint64_t)(binnig_thread_blocks * 256.0 * 4) * sizeof(uint32_t), true); // pass hist size = len / BIN_PART_SIZE * local_hist_bins
        d_pass_hist.clear();

        // read/set up shaders
        std::vector<uint32_t> histogramSpv = read_spirv("shaders/histogram.spv");
        std::vector<easyvk::Buffer> histogramBufs = {d_data, d_hist, d_meta};
        easyvk::Program histogramProgram = easyvk::Program(device, histogramSpv, histogramBufs);
        histogramProgram.setWorkgroups((len +65536 - 1)/ 65536); // # workgroups = (len / G_HIST_PART_SIZE)
        histogramProgram.initialize("main");

        std::vector<uint32_t> onesweepSpv = read_spirv("shaders/onesweep.spv");
        std::vector<easyvk::Buffer> onesweepBufs = {d_data, d_sorted, d_hist, d_index, d_pass_hist, d_meta};
        easyvk::Program onesweepProgram = easyvk::Program(device, onesweepSpv, onesweepBufs);
        onesweepProgram.setWorkgroups((len + 7680 -1) / 7680); // # workgroups = (len / BIN_PART_SIZE)
        onesweepProgram.initialize("main");

        // execute histogram pass
        float hist_runtime = histogramProgram.runWithDispatchTiming();

        // execute binning pass 1
        int pass_num = 0;
        int radix_shift = 0;
        d_meta.store(&pass_num, sizeof(uint32_t));
        d_meta.store(&radix_shift, sizeof(uint32_t), 0, sizeof(uint32_t));
        float bin1_runtime = onesweepProgram.runWithDispatchTiming();

        // execute binning pass 2
        pass_num = 1;
        radix_shift = 8;
        d_meta.store(&pass_num, sizeof(uint32_t));
        d_meta.store(&radix_shift, sizeof(uint32_t), 0, sizeof(uint32_t));
        float bin2_runtime = onesweepProgram.runWithDispatchTiming();

        // execute binning pass 3
        pass_num = 2;
        radix_shift = 16;
        d_meta.store(&pass_num, sizeof(uint32_t));
        d_meta.store(&radix_shift, sizeof(uint32_t), 0, sizeof(uint32_t));
        float bin3_runtime = onesweepProgram.runWithDispatchTiming();

        // execute binning pass 4
        pass_num = 3;
        radix_shift = 24;
        d_meta.store(&pass_num, sizeof(uint32_t));
        d_meta.store(&radix_shift, sizeof(uint32_t), 0, sizeof(uint32_t));
        float bin4_runtime = onesweepProgram.runWithDispatchTiming();

        // copy sorted buffer back to CPU buffer
        d_data.load(data, data_size);

        float total_runtime = hist_runtime + bin1_runtime + bin2_runtime + bin3_runtime + bin4_runtime;
        printf("Shader           | Runtime (%% total)\n");
        printf("-------------------------------------\n");
        printf("Global histogram | %fms (%.2f%%)\n", hist_runtime / 1000000.0, hist_runtime / total_runtime * 100.0);
        printf("Binning pass 1   | %fms (%.2f%%)\n", bin1_runtime / 1000000.0, bin1_runtime / total_runtime * 100.0);
        printf("Binning pass 2   | %fms (%.2f%%)\n", bin2_runtime / 1000000.0, bin2_runtime / total_runtime * 100.0);
        printf("Binning pass 3   | %fms (%.2f%%)\n", bin3_runtime / 1000000.0, bin3_runtime / total_runtime * 100.0);
        printf("Binning pass 4   | %fms (%.2f%%)\n", bin4_runtime / 1000000.0, bin4_runtime / total_runtime * 100.0);
        printf("\n");

        // deallocate
        histogramProgram.teardown();
        onesweepProgram.teardown();
        d_data.teardown();
        d_sorted.teardown();
        d_hist.teardown();
        d_meta.teardown();
        d_index.teardown();
        d_pass_hist.teardown();
    }
}
