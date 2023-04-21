#include <iostream>
#include <thread>
#include <chrono>
#include "zy_array_shm.h"

/**
 * 主要比较封装的共享内存的插入的性能和正常堆内存的插入性能
 * 
 * 1. TEST_COUNT = 1000000
 *    shm performance cost time(ns): 2.32808e+08
 *    heap performance cost time(ns): 1.3623e+08
 * 
 * 2. TEST_COUNT = 10000000
 *    shm performance cost time(ns): 2.28475e+09
 *    heap performance cost time(ns): 1.54185e+09
 * 
 * 3. TEST_COUNT = 100000000
 *    shm performance cost time(ns): 2.24757e+10
 *    heap performance cost time(ns): 1.5801e+10
 */

static const size_t TEST_COUNT = 100000000;

using thread_mem_shm_sdk::CArrayShm;
CArrayShm<int> array_shm;
void shm_performance() {
    auto start_tm = std::chrono::steady_clock().now();

    for (size_t i = 0; i < TEST_COUNT; ++i) {
        std::vector<int> arr{100, 10};
        auto count = array_shm.insert(arr);
    }

    auto end_tm = std::chrono::steady_clock().now();
    auto ts = std::chrono::duration<double, std::nano>(end_tm - start_tm).count();
    std::cout << "shm performance cost time(ns): " << ts << std::endl;
}

std::vector<int> heap_arr(100, 0);
void heap_performance() {
    auto start_tm = std::chrono::steady_clock().now();

    for (size_t i = 0; i < TEST_COUNT; ++i) {
        for (size_t j = 0; j < 100; ++j) {
            heap_arr[j] = 10;
        }
    }

    auto end_tm = std::chrono::steady_clock().now();
    auto ts = std::chrono::duration<double, std::nano>(end_tm - start_tm).count();
    std::cout << "heap performance cost time(ns): " << ts << std::endl;
}

int main() {
    bool res = array_shm.init(0x5c7f, 100, true);
    if (!res) {
        std::cout << "init shm failed, err: " << array_shm.get_err_msg() << std::endl;
        return -1;
    }

    shm_performance();
    heap_performance();
    return 0;
}
