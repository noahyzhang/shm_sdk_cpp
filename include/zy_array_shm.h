/**
 * @file zy_array_shm.h
 * @author noahyzhang
 * @version 0.1
 * @date 2023-04-20
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <stdint.h>
#include <vector>
#include "zy_base_shm.h"
#include "zy_utils.h"

namespace thread_mem_shm_sdk {

// 全局的内存格式版本
const uint32_t g_shm_version = 0xFFFFFF01;

// 内存头数组
struct ARRAY_SHM_HEADER {
    uint32_t version;
    uint32_t cur_node_count;
    uint32_t max_node_count;
    uint32_t header_crc_val;
    uint64_t time_ns;
};

/**
 * @brief 数组格式的共享内存
 * 
 * @tparam T 
 */
template <class T>
class CArrayShm : public CShm<T, ARRAY_SHM_HEADER> {
public:
    using TRAVERSE_METHOD_FUNC = typename CShm<T, ARRAY_SHM_HEADER>::TRAVERSE_METHOD_FUNC;

public:
    CArrayShm() {
        memset(&array_header_, 0, sizeof(ARRAY_SHM_HEADER));
    }
    ~CArrayShm() = default;
    CArrayShm(const CArrayShm&) = delete;
    CArrayShm& operator=(const CArrayShm&) = delete;
    CArrayShm(CArrayShm&&) = delete;
    CArrayShm& operator=(CArrayShm&&) = delete;


public:
    /**
     * @brief 初始化，默认是挂载，创建的时候设置 is_create=true
     * 1. 创建时，init(shm_key, node_count, true) ==> 如果存在该内存会优先选择 attach，没有才会 create
     * 2. 挂载时，init(shm_key)
     * 
     * @param shm_key 
     * @param node_count 
     * @param is_create 
     * @return true 
     * @return false 
     */
    bool init(size_t shm_key, size_t node_count = 0, bool is_create = false);

    /**
     * @brief 顺序插入节点
     * 
     * @param node_vec 
     * @return true 
     * @return false 
     */
    int insert(const std::vector<T>& node_vec);

    /**
     * @brief 遍历共享内存，对每个节点调用回调函数处理
     * 
     * @param node_func 
     * @param user_info 
     * @return true 
     * @return false 
     */
    bool traverse(TRAVERSE_METHOD_FUNC node_func) override;

    /**
     * @brief 获取头部数据
     * 
     * @param header 
     * @return true 
     * @return false 
     */
    bool get_header(ARRAY_SHM_HEADER* header);

private:
    /**
     * @brief 设置头部
     * 
     * @return true 
     * @return false 
     */
    bool set_header() override;

    /**
     * @brief 解析头部
     * 
     * @param header 
     * @return uint32_t 
     */
    uint32_t parse_header(const ARRAY_SHM_HEADER& header) override;

private:
    bool is_init_{false};
    ARRAY_SHM_HEADER array_header_;
};

template <class T>
bool CArrayShm<T>::init(size_t shm_key, size_t max_node_count, bool is_create) {
    if (is_init_) {
        this->set_err_msg("[CArrayShm::init] Already initialized, can't reinitialized");
        return false;
    }
    array_header_.version = g_shm_version;
    array_header_.max_node_count = max_node_count;
    array_header_.cur_node_count = 0;

    bool res = CShm<T, ARRAY_SHM_HEADER>::init(shm_key, max_node_count * this->get_node_size(), is_create);
    if (!res) {
        return false;
    }
    is_init_ = true;
    return true;
}

template <class T>
int CArrayShm<T>::insert(const std::vector<T>& node_vec) {
    if (!is_init_) {
        this->set_err_msg("[CArrayShm:insert] init might be mistaken");
        return -1;
    }
    size_t cur_node_count = 0;
    for (size_t i = 0; i < node_vec.size() && i < array_header_.max_node_count; ++i) {
        T* p_node = this->get_node_by_pos(i);
        if (p_node == nullptr) {
            continue;
        }
        memcpy(p_node, &node_vec[i], sizeof(T));
        ++cur_node_count;
    }
    array_header_.cur_node_count = cur_node_count;
    this->set_header();
    return cur_node_count;
}

template <class T>
bool CArrayShm<T>::set_header() {
    if (array_header_.max_node_count == 0) {
        this->set_err_msg("[CArrayShm::set_header] input max_node_count invalid");
        return false;
    }
    array_header_.header_crc_val = 0;
    array_header_.time_ns = get_now_system_time_ns();
    uint32_t crc = calc_crc_val((unsigned char*)&array_header_, sizeof(ARRAY_SHM_HEADER));
    array_header_.header_crc_val = crc;
    // 设置 header
    this->do_set_header(array_header_);
    return true;
}

template <class T>
uint32_t CArrayShm<T>::parse_header(const ARRAY_SHM_HEADER& p_header) {
    uint32_t version = p_header.version;
    if (version != g_shm_version) {
        char buf[1024] = {0};
        snprintf(buf, sizeof(buf), "[CArrayShm::parse_header] version check error, head info,"
            "version: %d, curNodeCount: %d, maxNodeCount: %d, headerCRCVal: %d, timeNs: %ld",
            p_header.version, p_header.cur_node_count, p_header.max_node_count,
            p_header.header_crc_val, p_header.time_ns);
        this->set_err_msg(buf);
        return 0;
    }
    // CRC 校验
    memcpy(&array_header_, &p_header, sizeof(ARRAY_SHM_HEADER));
    array_header_.header_crc_val = 0;
    uint32_t crc = calc_crc_val((unsigned char*)&array_header_, sizeof(ARRAY_SHM_HEADER));
    array_header_.header_crc_val = p_header.header_crc_val;
    if (crc != p_header.header_crc_val) {
        this->set_err_msg("[CArrayShm::parse_header] CRC calibration error");
        return 0;
    }
    // 整个共享内存占用的长度
    return (array_header_.max_node_count * sizeof(T) + sizeof(ARRAY_SHM_HEADER));
}

template <class T>
bool CArrayShm<T>::traverse(TRAVERSE_METHOD_FUNC node_func) {
    if (!is_init_) {
        this->set_err_msg("[CArrayShm::traverse] init might be mistaken");
        return false;
    }
    ARRAY_SHM_HEADER header;
    if (!get_header(&header)) {
        char buf[1024] = {0};
        snprintf(buf, sizeof(buf), "[CArrayShm::traverse] get_header err: %s", this->get_err_msg().c_str());
        this->set_err_msg(buf);
        return false;
    }
    if (parse_header(header) == 0) {
        char buf[1024] = {0};
        snprintf(buf, sizeof(buf), "[CArrayShm::traverse] parse_header err: %s", this->get_err_msg().c_str());
        this->set_err_msg(buf);
        return false;
    }
    T* p_node = nullptr;
    for (size_t i = 0; i < array_header_.cur_node_count; i++) {
        p_node = this->get_node_by_pos(i);
        if (p_node == nullptr) {
            this->set_err_msg("[CArrayShm::traverse] Failed to get node");
            return false;
        }
        bool ret = node_func(p_node);
        if (!ret) {
            this->set_err_msg("[CArrayShm::traverse] callback TRAVERSE_METHOD function return false");
            return false;
        }
    }
    return true;
}

template <class T>
bool CArrayShm<T>::get_header(ARRAY_SHM_HEADER* header) {
    if (header == nullptr) {
        this->set_err_msg("[CArrayShm::get_header] param header is null");
        return false;
    }
    if (!is_init_) {
        this->set_err_msg("[CArrayShm::get_header] init might be mistaken");
        return false;
    }
    return this->do_get_header(header);
}

}  // namespace thread_mem_shm_sdk
