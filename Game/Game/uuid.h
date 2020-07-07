#pragma once
/**
 * Copyright (c) 2018 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

#include <string>

#define UUID_VERSION "1.0.0"
#define UUID_LEN 37

enum {
    UUID_ESUCCESS = 0,
    UUID_EFAILURE = -1
};

int  uuid_init(void);
void uuid_generate(char* dst);


namespace Game {
    class UUID {
    private:
        char value[32];
    public:
        UUID() { memcpy(value, invaild.value, sizeof(value)); }

        UUID(const char* buf);
        UUID(const UUID& uuid) { memcpy(value, uuid.value, sizeof(value)); }

        UUID(UUID&& uuid) noexcept {
            memcpy(value,uuid.value,sizeof(value));
            uuid = invaild;
        }

        const UUID& operator=(UUID&& uuid) noexcept {
            memcpy(value,uuid.value,sizeof(uuid.value));
            uuid = invaild;
            return *this;
        }
        
        const UUID& operator=(const UUID& uuid) { 
            memcpy(value, uuid.value, sizeof(value));
            return *this;
        }

        bool operator==(const UUID& uuid) const {
            return !memcmp(value,uuid.value,sizeof(value));
        }

        bool operator<(const UUID& uuid)  const {
           return  memcmp(value,uuid.value,sizeof(value)) < 0;
        }

        static UUID generate() {
            static bool inited = false;
            if (!inited) uuid_init();

            char buffer[37];
            uuid_generate(buffer);

            return UUID(buffer);
        }

        std::string to_string();

        static UUID invaild;
    };
}
