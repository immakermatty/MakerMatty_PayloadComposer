#ifdef ESP32

#ifndef _MM_PAYLOAD_BUILDER_h
#define _MM_PAYLOAD_BUILDER_h

#include <Arduino.h>

#include "stl_make_unique.h"

#include <FS.h>
// #include <LITTLEFS.h>

#include <functional>
#include <list>
#include <memory>
#include <queue>

enum payload_status_t {
    TRANFER_DONE = 0,
    TRANFER_CLOSED = 1,
    TRANSFER_IN_PROGRESS = 2,
    TRANFER_NOT_FOUND = 3
};

class Composer_ {

public:
    typedef std::function<bool(uint8_t* destination, const uint8_t* source, size_t size)> TransferFunction;

    class Payload_ {

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
        Payload_(const size_t size, const uint32_t custom);

        // returns true on payload completetion
        bool compose(const uint8_t* const chunk_data, const size_t chunk_length, const size_t chunk_index, const TransferFunction transferFunction = defalutTransferFuntion);
        // returns true if payload is composed in given range
        bool composed(const size_t from_payload_index, const size_t to_payload_index);

        // read the finished Bytes. After read
        bool read(std::unique_ptr<uint8_t[]>& payload_out, size_t& size_out, uint32_t& custom_out);

        size_t size() const { return m_size; }
        
        bool finished() const { return m_finished; }

    private:
        size_t m_size;
        uint32_t m_custom;
        std::unique_ptr<uint8_t[]> m_data;
        std::list<gap_t> m_gaps;
        bool m_finished;
    };

    struct closed_t {

        closed_t()
            : uuid(0x00) {};

        closed_t(const uint64_t uuid)
            : uuid(uuid) {};

        uint64_t uuid;
    };

    struct payload_t {

        payload_t(const uint64_t uuid, const size_t size, const uint32_t custom, const uint32_t timeout)
            : uuid(uuid)
            , payload(size, custom) {};

        uint64_t uuid;
        uint32_t updated;
        uint32_t timeout;
        Payload_ payload;
    };

public:
    Composer_()
        : m_payloads()
        , m_closed()
        , m_closedIndex(0)
    {
    }

    Payload_* getPayload(const uint64_t payload_uuid)
    {
        for (payload_t& p : m_payloads) {
            if (p.uuid == payload_uuid) {
                return &(p.payload);
            }
        }

        return nullptr;
    }

    // returns true if payload is composed
    payload_status_t compose_(const uint64_t payload_uuid, const size_t payload_size, const uint32_t payload_custom, const uint8_t* const chunk_data, const size_t chunk_length, const size_t chunk_index, const TransferFunction transferFunction = defalutTransferFuntion)
    {
        log_d("m_payloads.size=%u", m_payloads.size());

        const uint32_t now = millis();

        for (const closed_t& c : m_closed) {
            if (c.uuid == payload_uuid) {
                log_d("TRANFER_CLOSED");
                return payload_status_t::TRANFER_CLOSED;
            }
        }

        std::list<payload_t>::iterator payload_it = m_payloads.end();

        // "garbage collection"
        {
            std::list<payload_t>::iterator it = m_payloads.begin();
            while (it != m_payloads.end()) {
                if (it->uuid == payload_uuid && payload_it == m_payloads.end()) {
                    it->updated = now;
                    payload_it = it++;
                    continue;
                }
                if (now - it->updated >= it->timeout) {
                    log_w("Payload %" PRIu64 " timeouted", it->uuid);
                    it = m_payloads.erase(it);
                    continue;
                }
                it++;
            }
        }

        // create new if the payload doesnt exist
        if (payload_it == m_payloads.end()) {
            m_payloads.emplace_front(payload_uuid, payload_size, payload_custom, 60000);
            payload_it = m_payloads.begin();
        }

        if (payload_it->payload.compose(chunk_data, chunk_length, chunk_index, transferFunction)) {

            m_closed[m_closedIndex++].uuid = payload_it->uuid;
            m_closedIndex %= m_closed.size();

            log_d("TRANFER_DONE");
            return payload_status_t::TRANFER_DONE;
        }

        log_d("TRANSFER_IN_PROGRESS");
        return payload_status_t::TRANSFER_IN_PROGRESS;
    }

    // // returns uuid of a ready payload
    //  uint32_t available()
    // {
    //     if (!m_composer) {
    //         return 0x00;
    //     }

    //     return m_composer->available();
    // }

    // read the finished bytes
    bool read(const uint64_t payload_uuid, std::unique_ptr<uint8_t[]>& payload_out, size_t& size_out, uint32_t& custom_out)
    {
        for (auto payload_it = m_payloads.begin(); payload_it != m_payloads.end(); ++payload_it) {
            if (payload_it->uuid == payload_uuid) {
                if (payload_it->payload.finished()) {
                    payload_it->payload.read(payload_out, size_out, custom_out);
                    m_payloads.erase(payload_it);
                    return true;
                } else {
                    return false;
                }
            }
        }

        return false;
    }

    // // read the finished Bytes
    //  bool read(const bool keep_composer = false)
    // {
    //     if (!m_composer) {
    //         return false;
    //     }

    //     bool success = m_composer->read();
    //     if (!keep_composer && !m_composer->available()) {
    //         delete m_composer;
    //         m_composer = nullptr;
    //     }
    //     return success;
    // }

    payload_status_t status(const uint64_t payload_uuid)
    {
        for (const payload_t& p : m_payloads) {
            if (p.uuid == payload_uuid) {
                if (p.payload.finished()) {
                    log_d("TRANFER_DONE");
                    return payload_status_t::TRANFER_DONE;
                } else {
                    log_d("TRANSFER_IN_PROGRESS");
                    return payload_status_t::TRANSFER_IN_PROGRESS;
                }
            }
        }

        for (const closed_t& c : m_closed) {
            if (c.uuid == payload_uuid) {
                log_d("TRANFER_CLOSED");
                return payload_status_t::TRANFER_CLOSED;
            }
        }

        log_d("TRANFER_NOT_FOUND");
        return payload_status_t::TRANFER_NOT_FOUND;
    }

    // void reset()
    // {
    //     delete m_composer;
    //     m_composer = nullptr;
    // }


private:

    static bool defalutTransferFuntion(uint8_t* source, const uint8_t* destination, size_t size)
    {
        memcpy(source, destination, size);
        return true;
    };

    std::list<payload_t> m_payloads;
    std::array<closed_t, 16> m_closed;
    size_t m_closedIndex;
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
//     std::list<gap_t> m_gaps;
//     uint32_t m_current_payload_uuid = 0;
// };

#endif
#endif