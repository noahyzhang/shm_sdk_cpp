/**
 * @file zy_utils.h
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
#include <stdio.h>
#include <time.h>
#include <stdarg.h>

namespace thread_mem_shm_sdk {

inline void xsnprintf(char* szBuffer, const uint32_t size, const char *sFormat, ...) {
    va_list ap;
    va_start(ap, sFormat);
    vsnprintf(szBuffer, size, sFormat, ap);
    va_end(ap);
}

uint32_t calc_crc_val(const uint8_t* p_buf, uint32_t length) {
    static uint32_t arrCRCTable[16] = {0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
        0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef};

    uint8_t uc = 0;
    uint32_t crc = 0;
    for (; length-- != 0;) {
        uc = ((uint8_t)(crc / 256)) / 16;
        crc <<= 4;
        crc ^= arrCRCTable[uc^(*p_buf / 16)];

        uc = ((uint8_t)(crc / 256)) / 16;
        crc <<= 4;
        crc ^= arrCRCTable[uc^(*p_buf & 0x0f)];
        p_buf++;
    }
    return crc;
}

inline uint64_t get_now_system_time_ns() {
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    return tp.tv_sec * static_cast<uint64_t>(1E9) + tp.tv_nsec;
}

}  // namespace thread_mem_shm_sdk
