#include <iostream>
#include <chrono>
#include <thread>
#include "zy_array_shm.h"
#include "zy_semaphore.h"
#include "rw_process.h"

struct DataNode {
    uint32_t tid;
    uint32_t arena_id;
    uint32_t allocated_kb;
    uint32_t deallocated_kb;
};

int main() {
    using thread_mem_shm_sdk::CArrayShm;
    using thread_mem_shm_sdk::ARRAY_SHM_HEADER;
    using thread_mem_shm_sdk::CSemaphore;

    CArrayShm<DataNode> array_shm;
    bool res = array_shm.init(SHM_KEY, MAX_SHM_ARR_COUNT, true);
    if (!res) {
        std::cout << "init array_shm failed, err: " << array_shm.get_err_msg() << std::endl;
        return -1;
    }
    CSemaphore sem;
    res = sem.create(SEM_KEY);
    if (!res) {
        std::cout << "init sem failed, err: " << sem.get_err_msg() << std::endl;
        return -2;
    }

    for (;;) {
        ARRAY_SHM_HEADER header;

        sem.lock();
        array_shm.get_header(&header);
        std::cout << "header info, version: " << header.version << ", cur_node_count: " << header.cur_node_count
            << ", max_node_count: " << header.max_node_count << ", time_ns: " << header.time_ns
            << ", crc: " << header.header_crc_val << std::endl;
        bool ret = array_shm.traverse([](DataNode* node) ->bool {
            std::cout << "tid: " << node->tid << ", arena_id: " << node->arena_id
                << ", allocated_kb: " << node->allocated_kb << ", deallocated_kb: " << node->deallocated_kb << " ";
            return true;
        });
        sem.unlock();

        if (!ret) {
            std::cout << "traverse failed, err: " << array_shm.get_err_msg() << std::endl;
            return -2;
        }
        std::cout << std::endl << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
