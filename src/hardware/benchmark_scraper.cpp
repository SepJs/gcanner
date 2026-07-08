#include <game_req_analyzer/hardware/hardware_database.hpp>
#include <game_req_analyzer/core/logger.hpp>

using namespace game_req;

Result<std::vector<CPUEntry>> scrape_passmark_cpu() {
    return std::vector<CPUEntry>{};
}

Result<std::vector<GPUEntry>> scrape_passmark_gpu() {
    return std::vector<GPUEntry>{};
}

Result<std::vector<CPUEntry>> scrape_geekbench_cpu() {
    return std::vector<CPUEntry>{};
}

Result<std::vector<GPUEntry>> scrape_geekbench_gpu() {
    return std::vector<GPUEntry>{};
}
