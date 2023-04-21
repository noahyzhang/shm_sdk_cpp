#include <ctime>
#include <iostream>
#include <thread>
#include <chrono>
#include "zy_array_shm.h"
#include "zy_semaphore.h"
#include "rw_process.h"

const std::vector<int>& gen_random_arr(size_t arr_len) {
    static std::vector<int> arr;
    arr.clear();
    arr.resize(arr_len);
    unsigned int local_seed = time(nullptr);
    for (size_t i = 0; i < arr_len; ++i) {
        arr[i] = rand_r(&local_seed) % 10000;
    }
    return arr;
}

int main() {
    using thread_mem_shm_sdk::CArrayShm;
    using thread_mem_shm_sdk::CSemaphore;

    CArrayShm<int> array_shm;
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
        std::vector<int> arr = gen_random_arr(20);

        sem.lock();
        auto count = array_shm.insert(arr);
        sem.unlock();

        std::cout << "insert node count: " << count << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
