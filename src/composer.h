#ifdef ESP32

#ifndef _MM_PAYLOAD_BUILDER_h
#define _MM_PAYLOAD_BUILDER_h

#include <Arduino.h>

#include "stl_make_unique.h"

#include <FS.h>
#include <LITTLEFS.h>

#include <functional>
#include <list>
#include <memory>
#include <queue>

class Payload {

    struct payload_t {
        std::unique_ptr<uint8_t[]> data;
        size_t size;
        uint32_t uuid;
    };

public:
    typedef std::function<bool(uint8_t* destination, const uint8_t* source, size_t size)> TransferFunction;

    class Composer {

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
        Composer();

        // returns true on payload completetion
        bool compose(const uint32_t payload_uuid, const uint8_t* const payload_data, const size_t payload_length, const size_t payload_index, const size_t payload_total, const TransferFunction transferFunction = defalutTransferFuntion);
        // returns true if payload is composed in given range
        bool composed(const size_t from_payload_index, const size_t to_payload_index);
        uint32_t composing();

        // if payload is ready. returns uuid
        uint32_t available();

        // read the finished Bytes
        bool read(std::unique_ptr<uint8_t[]>& payload_out, size_t& size_out, uint32_t& uuid_out);
        bool read();

    private:
        payload_t m_current_payload;
        std::list<gap_t> m_current_payload_gaps;
        payload_t m_finished_payload;
        // uint32_t m_finished_payload_uuid;
    };

public:
    Payload()
        : m_composer(nullptr)
        , m_closed_payload_buffer()
        , m_closed_payload_index(0)
    {
        memset(m_closed_payload_buffer.data(), 0, m_closed_payload_buffer.size());
    }

    ~Payload()
    {
        delete m_composer;
    }

    Composer* getComposer()
    {
        return m_composer;
    }

    // returns true if payload is composed
    inline bool compose(const uint32_t payload_uuid, const uint8_t* const payload_data, const size_t payload_length, const size_t payload_index, const size_t payload_total, const TransferFunction transferFunction = defalutTransferFuntion)
    {
        for (size_t i = 0; i < m_closed_payload_buffer.size(); i++) {
            if (payload_uuid == m_closed_payload_buffer[i]) {
                log_i("composing finished payload");
                return true;
            }
        }

        if (!m_composer) {
            // try {
            m_composer = new Composer();
            // } catch (...) {
            //     log_e("Composer allocation ERROR");
            //     delete m_composer;
            //     m_composer = nullptr;
            //     return false;
            // }
        }

        if (m_composer->compose(payload_uuid, payload_data, payload_length, payload_index, payload_total, transferFunction)) {
            m_closed_payload_buffer[m_closed_payload_index++] = payload_uuid;

            if (m_closed_payload_index > m_closed_payload_buffer.size()) {
                m_closed_payload_index = 0;
            }

            return true;
        }

        return false;
    }

    // returns uuid of a ready payload
    inline uint32_t available()
    {
        if (!m_composer) {
            return 0x00;
        }

        return m_composer->available();
    }

    // read the finished Bytes
    inline bool read(std::unique_ptr<uint8_t[]>& payload_out, size_t& size_out, uint32_t& uuid, const bool keep_composer = false)
    {
        if (!m_composer) {
            return false;
        }

        bool success = m_composer->read(payload_out, size_out, uuid);
        if (!keep_composer && !m_composer->available()) {
            delete m_composer;
            m_composer = nullptr;
        }
        return success;
    }

    // read the finished Bytes
    inline bool read(const bool keep_composer = false)
    {
        if (!m_composer) {
            return false;
        }

        bool success = m_composer->read();
        if (!keep_composer && !m_composer->available()) {
            delete m_composer;
            m_composer = nullptr;
        }
        return success;
    }

    inline void reset()
    {
        delete m_composer;
        m_composer = nullptr;
    }

private:
    static bool defalutTransferFuntion(uint8_t* source, const uint8_t* destination, size_t size)
    {
        memcpy(source, destination, size);
        return true;
    };

    Composer* m_composer;

    std::array<uint32_t, 4> m_closed_payload_buffer;
    size_t m_closed_payload_index;
};

//////////////////////////////////////////////////////////////////////////////////

// class PayloadComposerFS {
// public:
//     typedef std::function<bool(uint8_t* destination, const uint8_t* source, size_t size)> TransferFunction;

// private:
//     struct payload_t {
//         std::unique_ptr<File> file;
//         size_t size;
//     };

//     struct gap_t {

//         gap_t(size_t f, size_t t)
//             : from_index(f)
//             , to_index(t) {};

//         gap_t(const gap_t& other)
//             : from_index(other.from_index)
//             , to_index(other.to_index) {};

//         size_t from_index;
//         size_t to_index;
//     };

// public:
//     PayloadComposerFS();

//     //returns false on proccessing fail
//     bool compose(const size_t payload_uuid, const uint8_t* const payload_data, const size_t payload_length, const size_t payload_index, const size_t payload_total, const TransferFunction transferFunction = defalutTransferFuntion);

//     //if payload is ready
//     size_t available();

//     //read the finished Bytes
//     bool read(std::unique_ptr<File>& payload_out, size_t& size_out);

// private:
//     static inline bool defalutTransferFuntion(uint8_t* source, const uint8_t* destination, size_t size)
//     {
//         memcpy(source, destination, size);
//         return true;
//     };

//     std::queue<payload_t> m_ready_payload_buffer;
//     payload_t m_current_payload;
//     std::list<gap_t> m_current_payload_gaps;
//     uint32_t m_current_payload_uuid = 0;
// };

#endif
#endif