#ifdef ESP32

#include "composer.h"

////////////////////////////////////////////////////////////////////////////////////////////////

Composer_::Payload_::Payload_(const size_t size, const uint32_t custom)
    : m_size(size)
    , m_custom(custom)
    , m_data(std::make_unique<uint8_t[]>(size))
    , m_gaps()
    , m_finished(false)
{
    m_gaps.emplace_back(0, m_size);
}

bool Composer_::Payload_::compose(const uint8_t* const chunk_data, const size_t chunk_length, const size_t chunk_index, const TransferFunction transferFunction)
{

    log_d("payload_size=%u",m_size);
    log_d("chunk_index=%u", chunk_index);
    log_d("chunk_length=%u", chunk_length);

    // if (payload_uuid == m_finished_payload_uuid) {
    //     return true;
    // }

    // from_payload_index in payload, to_payload_index out interval <from_payload_index;to_payload_index]
    
    const size_t from_payload_index = chunk_index;
    const size_t to_payload_index = chunk_index + chunk_length;

    // if (payload_uuid != m_current_payload.uuid) {
    //     log_d("New payload %u begin. Current was %u", payload_uuid, m_current_payload.uuid);

    //     m_gaps.clear();
    //     m_gaps.emplace_back(0, payload_total);

    //     try {
    //         m_current_payload.data = std::make_unique<uint8_t[]>(payload_total); // override the old m_current_payload by a new one
    //         m_current_payload.size = payload_total;
    //         m_current_payload.uuid = payload_uuid;
    //     } catch (...) {
    //         log_e("Failed to allocate enough memory.");
    //         m_finished_payload = payload_t { nullptr, 0, 0x00 };
    //         return false;
    //     }
    // }

    if (from_payload_index == to_payload_index) {
        log_d("Composing empty chunk");
        return false;
    }

    if (from_payload_index > to_payload_index) {
        log_e("Invalid data");
        return false;
    }

    if (to_payload_index > m_size) {
        log_e("Payload chunk out of bounds");
        return false;
    }

    log_d("Copying payload, to_index = %u, from_index = %u, length = %u", to_payload_index, from_payload_index, chunk_length);
    if (!transferFunction(m_data.get() + from_payload_index, chunk_data, chunk_length)) {
        log_e("Error while composing payload");
        return false;
    }

    for (auto gap_it = m_gaps.begin(); gap_it != m_gaps.end(); /* NOP */) {

        if ((to_payload_index > gap_it->from_index) && (from_payload_index < gap_it->to_index)) {
            if (from_payload_index > gap_it->from_index) {
                m_gaps.emplace_front(gap_it->from_index, from_payload_index);
            }
            if (to_payload_index < gap_it->to_index) {
                m_gaps.emplace_front(to_payload_index, gap_it->to_index);
            }
            gap_it = m_gaps.erase(gap_it);
        } else {
            ++gap_it;
        }
    }

    if (m_gaps.size() == 0) {
        log_d("Payload finished");
        m_finished = true;
        return true;
    }

    return false;
}

bool Composer_::Payload_::composed(const size_t from_payload_index, const size_t to_payload_index)
{
    for (auto& gap : m_gaps) {
        if ((to_payload_index > gap.from_index) && (from_payload_index < gap.to_index)) {
            return false;
        }
    }

    return true;
}

bool Composer_::Payload_::read(std::unique_ptr<uint8_t[]>& payload_out, size_t& size_out, uint32_t& custom_out)
{
    if (!m_finished) {
        return false;
    }

    payload_out = std::move(m_data);
    m_data = nullptr;
    size_out = m_size;
    m_size = 0;
    custom_out = m_custom;
    m_custom = 0;

    return true;
}

/////////////////////////////////////////////////////////////////////////////////

// PayloadComposerFS::PayloadComposerFS()
//     : m_ready_payload_buffer()
//     , m_current_payload { nullptr }
//     , m_gaps()
//     , m_current_payload_uuid(0x00)
// {
// }

// bool PayloadComposerFS::compose(const size_t payload_uuid, const uint8_t* const chunk_data, const size_t chunk_length, const size_t chunk_index, const size_t payload_total, const TransferFunction transferFunction)
// {
//     if (!payload_uuid) {
//         log_e("Invalid payload_uuid, payload_uuid must be != 0");
//         return false;
//     }

//     const size_t from_payload_index = chunk_index;
//     const size_t to_payload_index = chunk_index + chunk_length;

//     if (payload_uuid != m_current_payload_uuid) {
//         log_d("Payload of the uuid %u begin", payload_uuid);

//         LITTLEFS.begin(false);

//         m_gaps.clear();
//         m_gaps.emplace_back(0, payload_total);

//         m_current_payload_uuid = payload_uuid;

//         m_current_payload.file = std::make_unique<File>(); // override the old file by a new one
//         m_current_payload.size = payload_total;
//     }

//     if (payload_uuid == m_current_payload_uuid) {

//         if (!m_current_payload.file || !m_current_payload.size) {
//             log_w("Invalid Payload buffer");
//             return false;
//         }

//         if (to_payload_index > m_current_payload.size) {
//             log_e("Buffer is not big enough");
//             m_current_payload.file = nullptr;
//             m_current_payload.size = 0;
//             m_current_payload_uuid = 0x00;
//             return false;
//         }

//         log_d("Copying payload, to_index = %u, from_index = %u, length = %u", to_payload_index, from_payload_index, chunk_length);
//         if (!transferFunction(m_current_payload.file.get(), chunk_data, chunk_length)) {
//             log_e("Payload is not valid");
//             m_current_payload.file = nullptr;
//             m_current_payload.size = 0;
//             return false;
//         }

//         for (auto gap_it = m_gaps.begin(); gap_it != m_gaps.end(); /* NOP */) {

//             if ((to_payload_index > gap_it->from_index) && (from_payload_index < gap_it->to_index)) {
//                 if (from_payload_index > gap_it->from_index) {
//                     m_gaps.emplace_front(gap_it->from_index, from_payload_index);
//                 }
//                 if (to_payload_index < gap_it->to_index) {
//                     m_gaps.emplace_front(to_payload_index, gap_it->to_index);
//                 }
//                 gap_it = m_gaps.erase(gap_it);
//             } else {
//                 ++gap_it;
//             }
//         }

//         if (m_gaps.size() == 0) {
//             log_d("Bytes end of the uuid %u", payload_uuid);

//             //m_ready_payload_buffer.emplace(std::move(m_current_payload), m_current_payload.size);
//             m_ready_payload_buffer.push(std::move(m_current_payload));

//             m_current_payload.file = nullptr;
//             m_current_payload.size = 0;
//             m_current_payload_uuid = 0x00;
//         }
//     }

//     return true;
// }

// size_t PayloadComposerFS::available()
// {
//     return m_ready_payload_buffer.size();
// }

// bool PayloadComposerFS::read(std::unique_ptr<File>& payload_out, size_t& size_out)
// {
//     if (PayloadComposerFS::available()) {
//         payload_out = std::move(m_ready_payload_buffer.front().file);
//         size_out = m_ready_payload_buffer.front().size;
//         m_ready_payload_buffer.pop();
//         return true;
//     } else {
//         return false;
//     }
// }

#endif
