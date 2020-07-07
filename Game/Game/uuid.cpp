/**
 * Copyright (c) 2018 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

#include <stdio.h>
#include <stdint.h>

#if defined(_WIN32)
#include <windows.h>
#include <wincrypt.h>
#endif

#include "uuid.h"


static uint64_t seed[2];


static uint64_t xorshift128plus(uint64_t* s) {
    /* http://xorshift.di.unimi.it/xorshift128plus.c */
    uint64_t s1 = s[0];
    const uint64_t s0 = s[1];
    s[0] = s0;
    s1 ^= s1 << 23;
    s[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
    return s[1] + s0;
}


int uuid_init(void) {
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
    int res;
    FILE* fp = fopen("/dev/urandom", "rb");
    if (!fp) {
        return UUID4_EFAILURE;
    }
    res = fread(seed, 1, sizeof(seed), fp);
    fclose(fp);
    if (res != sizeof(seed)) {
        return UUID4_EFAILURE;
    }

#elif defined(_WIN32)
    int res;
    HCRYPTPROV hCryptProv;
    res = CryptAcquireContext(
        &hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    if (!res) {
        return UUID_EFAILURE;
    }
    res = CryptGenRandom(hCryptProv, (DWORD)sizeof(seed), (PBYTE)seed);
    CryptReleaseContext(hCryptProv, 0);
    if (!res) {
        return UUID_EFAILURE;
    }

#else
#error "unsupported platform"
#endif
    return UUID_ESUCCESS;
}

Game::UUID Game::UUID::invaild("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx");

void uuid_generate(char* dst) {
    static const char* temp = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
    static const char* chars = "0123456789abcdef";
    union { unsigned char b[16]; uint64_t word[2]; } s;
    const char* p;
    int i, n;
    /* get random */
    s.word[0] = xorshift128plus(seed);
    s.word[1] = xorshift128plus(seed);
    /* build string */
    p = temp;
    i = 0;
    while (*p) {
        n = s.b[i >> 1];
        n = (i & 1) ? (n >> 4) : (n & 0xf);
        switch (*p) {
        case 'x': *dst = chars[n];              i++;  break;
        case 'y': *dst = chars[(n & 0x3) + 8];  i++;  break;
        default: *dst = *p;
        }
        dst++, p++;
    }
    *dst = '\0';
}


Game::UUID::UUID(const char* buf) {
    memcpy(value, buf, 8);
    memcpy(value + 8, buf + 9, 4);
    memcpy(value + 12, buf + 14, 4);
    memcpy(value + 16, buf + 19, 4);
    memcpy(value + 20, buf + 24, 12);
}

std::string Game::UUID::to_string() {
    std::string res;
    for (int i = 0; i != 32; i++) {
        res.push_back(value[i]);
        if (i == 7 || i == 11 || i == 15 || i == 19)
            res.push_back('-');
    }

    return res;
}