#ifdef ESP32

#include "composer.h"

////////////////////////////////////////////////////////////////////////////////////////////////

PayloadComposer::PayloadComposer()
    : m_payload_buffer()
    , m_current_payload { nullptr, 0 }
    , m_current_payload_gaps()
    , m_current_payload_uuid(0x00)
{
}

bool PayloadComposer::compose(const size_t payload_uuid, const uint8_t* const payload_data, const size_t payload_length, const size_t payload_index, const size_t payload_total, const TransferFunction transferFunction)
{
    if (!payload_uuid) {
        log_e("Invalid payload_uuid, payload_uuid must be != 0");
        return false;
    }

    const size_t payload_from_index = payload_index;
    const size_t payload_to_index = payload_index + payload_length;

    if (payload_uuid != m_current_payload_uuid) {
        log_d("Payload of the uuid %u begin", payload_uuid);

        m_current_payload_gaps.clear();
        m_current_payload_gaps.emplace_back(0, payload_total);

        m_current_payload_uuid = payload_uuid;

        try {
            m_current_payload.data = std::make_unique<uint8_t>(payload_total); // override the old m_payload_buffer by a new one
            m_current_payload.size = payload_total;
        } catch (std::bad_alloc e) {
            log_e("Failed to allocate enough memory.");
            m_current_payload.data = nullptr;
            m_current_payload.size = 0;
        }
    }

    if (payload_uuid == m_current_payload_uuid) {

        if (!m_current_payload.data || !m_current_payload.size) {
            log_w("Invalid Payload buffer");
            return false;
        }

        if (payload_to_index > m_current_payload.size) {
            log_e("Buffer is not big enough");
            m_current_payload.data = nullptr;
            m_current_payload.size = 0;
            m_current_payload_uuid = 0x00;
            return false;
        }

        log_d("Copying payload, to_index = %u, from_index = %u, length = %u", payload_to_index, payload_from_index, payload_length);
        if (!transferFunction(m_current_payload.data.get() + payload_from_index, payload_data, payload_length)) {
            log_e("Payload is not valid");
            m_current_payload.data = nullptr;
            m_current_payload.size = 0;
            return false;
        }

        for (auto gap_it = m_current_payload_gaps.begin(); gap_it != m_current_payload_gaps.end(); /* NOP */) {

            if ((payload_to_index > gap_it->from_index) && (payload_from_index < gap_it->to_index)) {
                if (payload_from_index > gap_it->from_index) {
                    m_current_payload_gaps.emplace_front(gap_it->from_index, payload_from_index);
                }
                if (payload_to_index < gap_it->to_index) {
                    m_current_payload_gaps.emplace_front(payload_to_index, gap_it->to_index);
                }
                gap_it = m_current_payload_gaps.erase(gap_it);
            } else {
                ++gap_it;
            }
        }

        if (m_current_payload_gaps.size() == 0) {
            log_d("Bytes end of the uuid %u", payload_uuid);

            //m_payload_buffer.emplace(std::move(m_current_payload), m_current_payload.size);
            m_payload_buffer.push(std::move(m_current_payload));

            m_current_payload.data = nullptr;
            m_current_payload.size = 0;
            m_current_payload_uuid = 0x00;
        }
    }

    return true;
}

size_t PayloadComposer::available()
{
    return m_payload_buffer.size();
}

bool PayloadComposer::read(std::unique_ptr<uint8_t>& payload_out, size_t& size_out)
{
    if (PayloadComposer::available()) {
        payload_out = std::move(m_payload_buffer.front().data);
        size_out = m_payload_buffer.front().size;
        m_payload_buffer.pop();
        return true;
    } else {
        return false;
    }
}

#endif
