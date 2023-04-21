/**
 * @file semaphore.h
 * @author noahyzhang
 * @brief 
 * @version 0.1
 * @date 2023-04-19
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <stdint.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <stdio.h>

namespace thread_mem_shm_sdk {

/**
 * @brief 信号量封装
 * 
 */
class CSemaphore {
public:
    CSemaphore() = default;
    ~CSemaphore() = default;
    CSemaphore(const CSemaphore&) = delete;
    CSemaphore& operator=(const CSemaphore&) = delete;
    CSemaphore(CSemaphore&&) = delete;
    CSemaphore& operator=(CSemaphore&&) = delete;

public:
    /**
     * @brief 创建信号量
     * 
     * @param sem_key 
     * @param sems 
     * @return true 
     * @return false 
     */
    bool create(const int32_t sem_key, const int32_t sems = 1) {
        #if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
            /* union semun is defined by including <sys/sem.h> */
        #else
            /* according to X/OPEN we have to define it ourselves */
            union semun {
                int32_t val;                  /* value for SETVAL */
                struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
                unsigned short *array;    /* array for GETALL, SETALL */
                /* Linux specific part: */
                struct seminfo *__buf;    /* buffer for IPC_INFO */
            };
        #endif
        if ((sem_id_ = semget(sem_key, sems, 00666)) < 0) {
            if ((sem_id_ = semget(sem_key, sems, IPC_CREAT | IPC_EXCL | 00666)) < 0) {
                switch (errno) {
                case EACCES:
                    snprintf(err_msg_, ERR_MSG_SIZE, "A semaphore set exists for key,"
                        "but the calling process does not have permission to access the  set.\n");
                    break;
                case EEXIST:
                    snprintf(err_msg_, ERR_MSG_SIZE, "A semaphore set exists"
                        " for key and semflg was asserting both IPC_CREAT and IPC_EXCL.\n");
                    break;
                case ENOENT:
                    snprintf(err_msg_, ERR_MSG_SIZE, "No semaphore set exists for key"
                        " and semflg wasn't asserting IPC_CREAT.");
                    break;
                case EINVAL:
                    snprintf(err_msg_, ERR_MSG_SIZE, "nsems  is  less  than  0  or"
                        " greater  than  the limit on the number of semaphores per semaphore set(SEMMSL),"
                        " or a semaphore set corresponding to key already exists,"
                        " and nsems is larger than the number of semaphores in that set.\n");
                    break;
                case ENOMEM:
                    snprintf(err_msg_, ERR_MSG_SIZE, "A semaphore set has to be created"
                        " but the system has not enough memory for the new data structure.\n");
                    break;
                case ENOSPC:
                    snprintf(err_msg_, ERR_MSG_SIZE, "A semaphore set has to be created"
                        " but the system limit for the maximum number of semaphore sets(SEMMNI),"
                        " or the system wide maximum number of semaphores (SEMMNS), would be exceeded.\n");
                    break;
                default:
                    snprintf(err_msg_, ERR_MSG_SIZE, "unkown error.\n");
                    break;
                }
                return false;
            }
            is_create_ = true;
            if (is_create_) {
                union semun arg;
                arg.val = 1;
                if (semctl(sem_id_, 0, SETVAL, arg) == -1) {
                    snprintf(err_msg_, ERR_MSG_SIZE, "semctl setval error");
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * @brief 加锁
     * 
     * @param wait 
     * @return true 
     * @return false 
     */
    bool lock(const bool wait = true) {
        struct sembuf sem_buf[2] = {{0, -1, SEM_UNDO}, {0, -1, IPC_NOWAIT | SEM_UNDO}};
        if (sem_id_ == -1) {
            snprintf(err_msg_, ERR_MSG_SIZE, "no create sem.");
            return false;
        }
        if (semop(sem_id_, &sem_buf[wait ? 0 : 1], 1) < 0) {
            snprintf(err_msg_, ERR_MSG_SIZE, "semop err: (errno=%d)", errno);
            return false;
        }
        return true;
    }

    /**
     * @brief 解锁
     * 
     * @return true 
     * @return false 
     */
    bool unlock() {
        struct sembuf sem_buf[1] = {{0, 1, SEM_UNDO}};
        if (sem_id_ == -1) {
            snprintf(err_msg_, ERR_MSG_SIZE, "no create sem");
            return false;
        }
        if (semop(sem_id_, &sem_buf[0], 1) < 0) {
            snprintf(err_msg_, ERR_MSG_SIZE, "semop err: (errno=%d)", errno);
            return false;
        }
        return true;
    }

    /**
     * @brief 删除信号量
     * 
     * @return true 
     * @return false 
     */
    bool destroy() {
        if (semctl(sem_id_, 0, IPC_RMID) == -1) {
            snprintf(err_msg_, ERR_MSG_SIZE, "semctl IPC_RMID err: (errno=%d)", errno);
            return false;
        }
        return true;
    }

    /**
     * @brief 获取当前操作错误信息
     * 
     * @return const char* 
     */
    const char* get_err_msg() const { return err_msg_; }

private:
    static const int ERR_MSG_SIZE = 1023;
    char err_msg_[ERR_MSG_SIZE+1] = {0};
    int32_t sem_id_ = -1;
    // 是否创建信号量（true：是，false：否）
    bool is_create_ = false;
};

}  // namespace thread_mem_shm_sdk
