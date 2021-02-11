#ifdef ESP32

#ifndef _MM_PAYLOAD_BUILDER_h
#define _MM_PAYLOAD_BUILDER_h

#include <Arduino.h>

#include <list>
#include <memory>
#include <queue>

#include "stl_make_unique.h"

class PayloadComposer {
private:
    struct gap_t {

        gap_t(size_t f, size_t t)
            : from_index(f)
            , to_index(t) {};

        gap_t(const gap_t& other)
            : from_index(other.from_index)
            , to_index(other.to_index) {};

        size_t from_index;
        size_t to_index;
    };

public:
    PayloadComposer();

    //returns false on proccessing fail
    bool compose(const size_t payload_uuid, const uint8_t* const payload_data, const size_t payload_length, const size_t payload_index, const size_t payload_total);

    //if payload is ready
    size_t available();

    //read the finished Bytes
    std::unique_ptr<std::vector<uint8_t>> read();

private:
    std::queue<std::unique_ptr<std::vector<uint8_t>>> m_payload_buffer;

    std::unique_ptr<std::vector<uint8_t>> m_current_payload;
    std::list<gap_t> m_current_payload_gaps;
    uint32_t m_current_payload_uuid = 0;
};

#endif
#endif