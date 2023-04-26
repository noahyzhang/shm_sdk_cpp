#include <ctime>
#include <iostream>
#include <thread>
#include <chrono>
#include "zy_array_shm.h"
#include "zy_semaphore.h"
#include "rw_process.h"

struct DataNode {
    uint32_t tid;
    uint32_t arena_id;
    uint32_t allocated_kb;
    uint32_t deallocated_kb;
};

const std::vector<DataNode>& gen_random_arr(size_t arr_len) {
    static std::vector<DataNode> arr;
    arr.clear();
    arr.resize(arr_len);
    unsigned int local_seed = time(nullptr);
    for (size_t i = 0; i < arr_len; ++i) {
        arr[i].tid = rand_r(&local_seed);
        arr[i].arena_id = rand_r(&local_seed);
        arr[i].allocated_kb = rand_r(&local_seed);
        arr[i].deallocated_kb = rand_r(&local_seed);
    }
    return arr;
}

int main() {
    using thread_mem_shm_sdk::CArrayShm;
    using thread_mem_shm_sdk::CSemaphore;

    CArrayShm<DataNode> array_shm;
    bool res = array_shm.init(SHM_KEY, MAX_SHM_ARR_COUNT, true);
    if (!res) {
        std::cout << "init shm failed, err: " << array_shm.get_err_msg() << std::endl;
        return -1;
    }

    CSemaphore sem;
    res = sem.create(SEM_KEY);
    if (!res) {
        std::cout << "init sem failed, err: " << sem.get_err_msg() << std::endl;
        return -2;
    }

    for (;;) {
        std::vector<DataNode> arr = gen_random_arr(20);

        sem.lock();
        auto count = array_shm.insert(arr);

        std::cout << "insert node count: " << count << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        sem.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
