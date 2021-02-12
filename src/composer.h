#ifdef ESP32

#ifndef _MM_PAYLOAD_BUILDER_h
#define _MM_PAYLOAD_BUILDER_h

#include <Arduino.h>

#include "stl_make_unique.h"

#include <functional>
#include <list>
#include <memory>
#include <queue>

class PayloadComposer {
public:
    typedef std::function<bool(uint8_t* destination, const uint8_t* source, size_t size)> TransferFunction;

private:
    struct payload_t {
        std::unique_ptr<uint8_t[]> data;
        size_t size;
    };

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
    bool compose(const size_t payload_uuid, const uint8_t* const payload_data, const size_t payload_length, const size_t payload_index, const size_t payload_total, const TransferFunction transferFunction = defalutTransferFuntion);

    //if payload is ready
    size_t available();

    //read the finished Bytes
    bool read(std::unique_ptr<uint8_t[]>& payload_out, size_t& size_out);

private:
    static inline bool defalutTransferFuntion(uint8_t* source, const uint8_t* destination, size_t size)
    {
        memcpy(source, destination, size);
        return true;
    };

    std::queue<payload_t> m_payload_buffer;
    payload_t m_current_payload;
    std::list<gap_t> m_current_payload_gaps;
    uint32_t m_current_payload_uuid = 0;
};

#endif
#endif