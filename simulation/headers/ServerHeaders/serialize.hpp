#pragma once
#include <cstdint>
#include <vector>

// PULSE Network Protocol Definition 
// Magic Header to verify packet integrity
const uint32_t PULSE_MAGIC = 0x50554C53; // "PULS" in ASCII

// Command Codes for Server-Backend Communication
enum class PacketType : uint8_t {
    INIT_SCENARIO = 0x01,
    UPDATE_FRAME = 0x02,
    AB_TEST_DATA = 0x03,
    TERMINATE = 0xFF
};

// Compact "Struct-of-Arrays" packet for high bandwidth efficiency
struct VehicleUpdatePacket {
    uint32_t vehicleID;
    float positionX;
    float positionY;
    float positionZ;
    float velocity;
    float orientation; // Y-axis rotation
    // Padding to align to 32 bytes for TCP stream optimization
    uint32_t _padding;
};

struct SimulationHeader {
    uint32_t magic;
    PacketType type;
    uint32_t payloadSize;
    uint64_t timestamp;
};

// Simulation State Buffer (To be serialized)
class NetworkBuffer {
public:
    std::vector<uint8_t> buffer;

    void serialize(const VehicleUpdatePacket& packet) {
        // Direct memory copy for speed
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&packet);
        buffer.insert(buffer.end(), ptr, ptr + sizeof(VehicleUpdatePacket));
    }
};