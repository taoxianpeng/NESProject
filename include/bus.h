#ifndef BUS_H
#define BUS_H

#include <array>

namespace nes {
class Bus {
public:
    explicit Bus() = default;
    ~Bus() = default;

    void write(uint16_t address, uint8_t data) {
        if (address < ram.size()) {
            ram[address] = data;
        }
    }

    uint8_t read(uint16_t address) {
        if (address < ram.size()) {
            return ram[address];
        }

        return 0x00;
    }

    void reset() noexcept {
        for (auto& n : ram) {
            n = 0U;
        }
    }

private:
    std::array<uint8_t, 64 * 1024> ram;
};
}
#endif // !BUS_H
