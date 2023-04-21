/**
 * @file zy_base_shm.h
 * @author noahyzhang
 * @brief 
 * @version 0.1
 * @date 2023-04-19
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <sys/ipc.h>
#include <sys/shm.h>
#include <stddef.h>
#include <string.h>
#include <string>
#include <utility>

namespace thread_mem_shm_sdk {

/**
 * @brief 共享内存封装
 * 
 * @tparam T 
 * @tparam TH 
 */
template <class T, class TH>
class CShm {
public:
    // (address, length)
    using SHM_TYPE = std::pair<void*, size_t>;
    using TRAVERSE_METHOD_FUNC = bool (*)(T* node);

public:
    CShm() = default;
    ~CShm() = default;
    CShm(const CShm&) = delete;
    CShm& operator=(const CShm&) = delete;
    CShm(CShm&&) = delete;
    CShm& operator=(CShm&&) = delete;

public:
    /**
     * @brief 初始化
     * 
     * @param shm_key 
     * @param shm_body_size 
     * @param is_create 
     * @return true 
     * @return false 
     */
    bool init(size_t shm_key, size_t shm_body_size = 0, bool is_create = false);

    /**
     * @brief 设置错误信息
     * 
     * @param err_msg 
     */
    void set_err_msg(const std::string& err_msg);

    /**
     * @brief 获取错误信息
     * 
     * @return std::string 
     */
    std::string get_err_msg() const;

protected:
    /**
     * @brief 设置内存头
     * 
     * @return true 
     * @return false 
     */
    virtual bool set_header() = 0;

    /**
     * @brief 解析内存头
     * 
     * @param header 
     * @return uint32_t 
     */
    virtual uint32_t parse_header(TH* header) = 0;

public:
    /**
     * @brief 遍历
     * 
     * @param node_func 
     * @return true 
     * @return false 
     */
    virtual bool traverse(TRAVERSE_METHOD_FUNC node_func) = 0;

protected:
    /**
     * @brief 获取节点的地址
     * 
     * @param pos 
     * @param offset 
     * @return T* 
     */
    T* get_node_by_pos(size_t pos, size_t offset = 0) const;

    /**
     * @brief 设置内存头函数
     * 
     * @param header 
     * @return true 
     * @return false 
     */
    bool do_set_header(const TH& header);

    /**
     * @brief 获取内存头
     * 
     * @param header 
     * @return true 
     * @return false 
     */
    bool do_get_header(TH* header);

    /**
     * @brief 获取内存头大小，注意已经处理了 struct 为空的特殊情况
     * 
     * @return size_t 
     */
    size_t get_header_size() const {
        return ((sizeof(TH) > 1) ? sizeof(TH) : 0);
    }

    /**
     * @brief 获取单个节点大小
     * 
     * @return size_t 
     */
    size_t get_node_size() const {
        return sizeof(T);
    }

private:
    /**
     * @brief 创建共享内存
     * 
     * @return true 
     * @return false 
     */
    bool create();

    /**
     * @brief 挂载共享内存
     * 
     * @return true 
     * @return false 
     */
    bool attach();

    /**
     * @brief 卸载共享内存
     * 
     * @return true 
     * @return false 
     */
    bool detach();

    /**
     * @brief 实际的挂载共享内存
     * 
     * @param length 
     * @return void* 
     */
    void* do_attach(size_t length);

    /**
     * @brief 实际的卸载共享内存
     * 
     * @param p_shm 
     * @return true 
     * @return false 
     */
    bool do_detach(void* p_shm);

private:
    /**
     * @brief 获取共享内存指针
     * 
     * @param shm_key 
     * @param shm_size 
     * @param flag 
     * @return void* 
     */
    void* get_shm(size_t shm_key, size_t shm_size, int flag);

    /**
     * @brief 获取共享内存指针(重载版本)
     * 
     * @param shm 
     * @param shm_key 
     * @param shm_size 
     * @param flag 
     * @return int 
     */
    int get_shm(void** shm, size_t shm_key, size_t shm_size, int flag);

private:
    bool is_init_{false};
    bool is_create_{false};
    bool is_attach_{false};
    bool is_set_callback_{false};

private:
    size_t shm_key_{0};
    size_t shm_length_{0};
    size_t shm_header_len_{0};
    size_t shm_body_len_{0};
    std::string err_msg_;

    SHM_TYPE shm_;
    SHM_TYPE shm_header_;
    SHM_TYPE shm_body_;
};

template <class T, class TH>
bool CShm<T, TH>::init(size_t shm_key, size_t shm_body_size /* =0 */, bool is_create /* =false */) {
    if (!is_create) {
        shm_body_size = 0;
    }
    shm_key_ = shm_key;
    is_create_ = is_create;

    shm_header_len_ = get_header_size();
    shm_body_len_ = shm_body_size;
    shm_length_ = shm_header_len_ + shm_body_len_;

    is_init_ = false;
    if (is_create_ && shm_body_len_ == 0) {
        set_err_msg("The specifying length is invalid (==0) when creating SHM");
        return false;
    }
    // 尝试挂载，如果挂载成功说明不需要重新 create
    TH* header = reinterpret_cast<TH*>(do_attach(shm_header_len_));
    if (nullptr != header) {
        // 挂载成功
        is_create_ = false;
    }
    if (!is_create_) {
        if (header == nullptr) {
            return false;
        }
        // 虚函数，继承类实现，返回共享内存的大小
        size_t length = this->parse_header(header);
        do_detach(header);
        if (length <= 0) {
            return false;
        }
        shm_length_ = length;
        attach();
    } else {
        // // 尝试挂载内存失败，或者指定需要创建的情况
        if (!create()) {
            return false;
        }
        // 虚函数，继承类实现
        if (!this->set_header()) {
            return false;
        }
    }
    is_init_ = true;
    return true;
}

template <class T, class TH>
bool CShm<T, TH>::create() {
    if (!is_create_ || shm_length_ == 0) {
        err_msg_ = "Parameter initialized error";
        return false;
    }
    void* p_shm = nullptr;
    int flag = 0666 | IPC_CREAT;
    int ret = get_shm(&p_shm, shm_key_, shm_length_, flag);
    if (ret < 0) {
        err_msg_ = "Failed to create shared memory";
        return false;
    }
    shm_.first = p_shm;
    shm_.second = shm_length_;

    shm_header_.first = p_shm;
    shm_header_.second = shm_header_len_;

    shm_body_.first = reinterpret_cast<char*>(p_shm) + shm_header_len_;
    shm_body_.second = shm_length_ - shm_header_len_;

    is_attach_ = true;
    return is_attach_;
}

template <class T, class TH>
bool CShm<T, TH>::attach() {
    if (is_create_ || shm_length_ == 0) {
        err_msg_ = "Not initialized";
        return false;
    }
    void* p_shm = do_attach(shm_length_);
    if (p_shm == nullptr) {
        return false;
    }
    shm_.first = p_shm;
    shm_.second = shm_length_;

    shm_header_. first = p_shm;
    shm_header_.second = shm_header_len_;

    shm_body_.first = reinterpret_cast<char*>(p_shm) + shm_header_len_;
    shm_body_.second = shm_length_ - shm_header_len_;

    is_attach_ = true;
    return is_attach_;
}

template <class T, class TH>
bool CShm<T, TH>::detach() {
    if (!is_attach_) {
        err_msg_ = "Not attach";
        return false;
    }
    do_detach(shm_.first);
    is_attach_ = false;
    return true;
}

template <class T, class TH>
void* CShm<T, TH>::do_attach(size_t length) {
    int flag = 0666;
    void* p_shm = get_shm(shm_key_, length, flag);
    if (p_shm == nullptr) {
        err_msg_ = "Failed to attach shared memory";
        return nullptr;
    }
    return p_shm;
}

template <class T, class TH>
bool CShm<T, TH>::do_detach(void* p_shm) {
    if (nullptr == p_shm) {
        return false;
    }
    shmdt(p_shm);
    return true;
}

template <class T, class TH>
void CShm<T, TH>::set_err_msg(const std::string& err_msg) {
    err_msg_ = err_msg;
}

template <class T, class TH>
std::string CShm<T, TH>::get_err_msg() const {
    return err_msg_;
}

template <class T, class TH>
bool CShm<T, TH>::do_set_header(const TH& header) {
    if (false == is_attach_) {
        err_msg_ = "Not attach";
        return false;
    }
    memcpy(shm_header_.first, &header, get_header_size());
    return true;
}

template <class T, class TH>
bool CShm<T, TH>::do_get_header(TH* header) {
    if (false == is_attach_) {
        err_msg_ = "Not attach";
        return false;
    }
    memcpy(header, shm_header_.first, get_header_size());
    return true;
}

template <class T, class TH>
T* CShm<T, TH>::get_node_by_pos(size_t pos, size_t offset) const {
    if (false == is_attach_) {
        return nullptr;
    }
    T* shm_body = reinterpret_cast<T*>(reinterpret_cast<char*>(shm_body_.first) + offset);
    return (shm_body + pos);
}

template <class T, class TH>
void* CShm<T, TH>::get_shm(size_t shm_key, size_t shm_size, int flag) {
    char msg[1024] = {0};
    if (shm_key == 0) {
        snprintf(msg, sizeof(msg), "[CShm::get_shm] shm_key: %zu should lager than 0", shm_key);
        set_err_msg(msg);
        return nullptr;
    }
    int shm_id = shmget(shm_key, shm_size, flag);
    if (shm_id < 0) {
        snprintf(msg, sizeof(msg), "[CShm::get_shm] Failed to call shmget, ret: %d, key: %zu, size: %zu, reason: %s",
            shm_id, shm_key, shm_size, strerror(errno));
        set_err_msg(msg);
        return nullptr;
    }
    void* p_shm = shmat(shm_id, nullptr, 0);
    if (p_shm == nullptr) {
        snprintf(msg, sizeof(msg), "[CShm::get_shm] Failed to call shmat");
        return nullptr;
    }
    return p_shm;
}

template <class T, class TH>
int CShm<T, TH>::get_shm(void** pp_shm, size_t shm_key, size_t shm_size, int flag) {
    char msg[1024] = {0};
    if (shm_key == 0) {
        snprintf(msg, sizeof(msg), "[CShm::get_shm] shm_key: %zu should lager than 0", shm_key);
        set_err_msg(msg);
        return -1;
    }
    void* p_shm = get_shm(shm_key, shm_size, flag & (~IPC_CREAT));
    if (nullptr == p_shm) {
        if (!(flag & IPC_CREAT)) {
            snprintf(msg, sizeof(msg), "[CShm::get_shm] Try to attach shm which is not exist! shm_key: %zu", shm_key);
            set_err_msg(msg);
            return -2;
        }
        p_shm = get_shm(shm_key, shm_size, flag);
        if (nullptr == p_shm) {
            return -3;
        }
        memset(p_shm, 0, shm_size);
        *pp_shm = p_shm;
        return 1;
    }
    *pp_shm = p_shm;
    return 0;
}

}  // namespace thread_mem_shm_sdk
