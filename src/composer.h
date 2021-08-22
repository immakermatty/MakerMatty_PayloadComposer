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

struct payload_t {
    std::unique_ptr<uint8_t[]> data;
    size_t size;
};

class Payload {
public:
    typedef uint32_t payload_uuid_t;
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

        //returns false on proccessing fail
        bool compose(const payload_uuid_t payload_uuid, const uint8_t* const payload_data, const size_t payload_length, const size_t payload_index, const size_t payload_total, const TransferFunction transferFunction = defalutTransferFuntion);
        bool composed(const size_t from_payload_index, const size_t to_payload_index);
        payload_uuid_t composing();

        //if payload is ready
        size_t available();

        //read the finished Bytes
        bool read(std::unique_ptr<uint8_t[]>& payload_out, size_t& size_out);

    private:
        std::queue<payload_t> m_payload_buffer;
        payload_t m_current_payload;
        std::list<gap_t> m_current_payload_gaps;
        payload_uuid_t m_current_payload_uuid = 0;
    };

public:
    Payload()
        : m_composer(nullptr)
    {
    }

    ~Payload()
    {
        delete m_composer;
    }

    Composer* getComposer()
    {
        return m_composer;
    }

    //returns false on proccessing fail
    inline bool compose(const payload_uuid_t payload_uuid, const uint8_t* const payload_data, const size_t payload_length, const size_t payload_index, const size_t payload_total, const TransferFunction transferFunction = defalutTransferFuntion)
    {
        if (!m_composer) {
            m_composer = new Composer();
        }

        return m_composer->compose(payload_uuid, payload_data, payload_length, payload_index, payload_total, transferFunction);
    }

    //if payload is ready
    inline size_t available()
    {
        if (m_composer) {
            return m_composer->available();
        } else {
            return 0;
        }
    }

    //read the finished Bytes
    inline bool read(std::unique_ptr<uint8_t[]>& payload_out, size_t& size_out)
    {
        if (m_composer) {
            bool success = m_composer->read(payload_out, size_out);
            if (!m_composer->available()) {
                delete m_composer;
                m_composer = nullptr;
            }
            return success;
        } else {
            return false;
        }
    }

private:
    static bool defalutTransferFuntion(uint8_t* source, const uint8_t* destination, size_t size)
    {
        memcpy(source, destination, size);
        return true;
    };

    Composer* m_composer;
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

//     std::queue<payload_t> m_payload_buffer;
//     payload_t m_current_payload;
//     std::list<gap_t> m_current_payload_gaps;
//     uint32_t m_current_payload_uuid = 0;
// };

#endif
#endif