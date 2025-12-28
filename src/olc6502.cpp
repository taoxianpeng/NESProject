#include "olc6502.h"
#include <cstdio>
#include <bus.h>

namespace nes {
void OLC6502::write(uint16_t address, uint8_t data)
{
    if (bus) {
        bus->write(address, data);
    }
    else {
        printf("Error: bus is nullptr!");
    }

}

uint8_t OLC6502::read(uint16_t address)
{
    if (bus) {
        return bus->read(address);
    }
    else {
        printf("Error: bus is nullptr!");
    }
}

void OLC6502::reset()
{
    // Get address to set program counter to
    addr_abs = 0xFFFC;
    uint16_t lo = read(addr_abs + 0);
    uint16_t hi = read(addr_abs + 1);

    // Set it
    pc = (hi << 8) | lo;

    accumulator_register = 0;
    x_register = 0;
    y_register = 0;
    sp = 0xFC;
    status_register = 0x00 | Flag::E_UNUSED;

    addr_abs = 0x0000;
    addr_rel = 0x0000;

    cycles = 0;
    cycle_count = 0;
}

uint8_t OLC6502::IMP()
{
    // 隐含寻址, 意思是该指令不需要使用参数，
    // 所以这个函数不需要获取下一个字节的参数作为操作数
    return 0;
}

uint8_t OLC6502::IMM()
{
    // 立即寻址（immediate mode addressing），意思是操作数就是该指令的一部分，
    // 指令后面的参数就是操作数(不需要再去内存地址中找出来)
    addr_abs = pc;
    pc++;
    return 0;
}

uint8_t OLC6502::ZP0()
{
    // 零页寻址, 数据存放在第0页内存中(地址范围0x00到0xFF),
    // 16位地址的高字节部分是页的地址, 低字节部分是数据
    // 高8位可以表示256个页面，低8位表示一个页面有256字节用来存储数据
    addr_abs = static_cast<uint16_t>(read(pc));
    pc++;
    addr_abs &= 0x00FF;
    return 0;
}

uint8_t OLC6502::ZPX()
{
    // 零页X变址（zero page addressing with X register offset）。
    // 提供给指令的零页地址要加上X寄存器的值组成新的内存地址，这样对于遍历内存区域很有用
    // 类似C语言，数组基地址+索引偏移量
    addr_abs = static_cast<uint16_t>(read(pc) + x_register);
    pc++;
    addr_abs &= 0x00FF;
    return 0;
}

uint8_t OLC6502::ZPY()
{
    // 零页Y变址，类似于零页X变址，只不过是使用Y寄存器作为偏移量
    addr_abs = static_cast<uint16_t>(read(pc) + y_register);
    pc++;
    addr_abs &= 0x00FF;
    return 0;
}

uint8_t OLC6502::REL()
{
    // 相对寻址模式，通常用于分支指令
    addr_rel = static_cast<uint16_t>(read(pc));
    // 检查第8是否为1（负数），为负数则要扩展16位，高8位全设置为1；
    if (addr_rel & 0x80) {
        addr_rel |= 0xFF00;
    }
    return 0;
}

uint8_t OLC6502::ABS()
{
    // 直接寻址模式（也叫绝对寻址模式）。需要两个字节来表示一个完整的16位地址
    // 所以需要读取第一个字节为低位地址和第二个字节为高位地址，
    // 然后将它们组合成一个16位地址
    const uint16_t addr_low = static_cast<uint16_t>(read(pc));
    pc++;
    const uint16_t addr_high = static_cast<uint16_t>(read(pc));
    pc++;
    addr_abs = (addr_high << 8) | addr_low;
    return 0;
}

uint8_t OLC6502::ABX()
{
    // 直接X变址，它与零页X变址相似，先读取一个基准地址，
    // 然后再加上X寄存器的值，形成新的地址。
    const uint16_t addr_low = static_cast<uint16_t>(read(pc));
    pc++;
    const uint16_t addr_high = static_cast<uint16_t>(read(pc));
    pc++;
    addr_abs = (addr_high << 8) | addr_low;
    addr_abs += x_register;
    if ((addr_abs & 0xFF00) != (addr_high << 8)) {
        return 1; // 跨页，增加一个周期
    }
    return 0;
}

uint8_t OLC6502::ABY()
{
    // 直接Y变址，类似于直接X变址，只不过是使用Y寄存器作为偏移量
    const uint16_t addr_low = static_cast<uint16_t>(read(pc));
    pc++;
    const uint16_t addr_high = static_cast<uint16_t>(read(pc));
    pc++;
    addr_abs = (addr_high << 8) | addr_low;
    addr_abs += y_register;
    if ((addr_abs & 0xFF00) != (addr_high << 8)) {
        return 1; // 跨页，增加一个周期
    }
    return 0;
}

uint8_t OLC6502::IND()
{
    // 间接寻址
    const uint16_t addr_low_ptr = static_cast<uint16_t>(read(pc));
    pc++;
    const uint16_t addr_high_ptr = static_cast<uint16_t>(read(pc));
    pc++;
    const uint16_t addr_ptr = ((addr_high_ptr << 8) | addr_low_ptr);
    // 实际上这里有个bug, 如果addr_ptr是 0xFF 那么addr_ptr+1 实际上是跨页了，需要多一个周期来处理,
    // 但这里没有处理这个问题！ TODO
    addr_abs = (static_cast<uint16_t>(read(addr_ptr + 1)) << 8) | static_cast<uint16_t>(read(addr_ptr + 0));
    return 0;
}

uint8_t OLC6502::IZX()
{
    // 先变址X后间接寻址
    const uint16_t addr_ptr_zp = static_cast<uint16_t>(read(pc) + x_register);
    pc++;
    const uint16_t addr_low_ptr = static_cast<uint16_t>(read(addr_ptr_zp) & 0x00FF);
    const uint16_t addr_high_ptr = static_cast<uint16_t>(read(addr_ptr_zp + 1) & 0x00FF);
    addr_abs = (addr_high_ptr << 8) | addr_low_ptr;
    return 0;
}

uint8_t OLC6502::IZY()
{
    // 先变址Y后间接寻址
    const uint16_t addr_ptr_zp = static_cast<uint16_t>(read(pc) + x_register);
    pc++;
    const uint16_t addr_low_ptr = static_cast<uint16_t>(read(addr_ptr_zp) & 0x00FF);
    const uint16_t addr_high_ptr = static_cast<uint16_t>(read(addr_ptr_zp + 1) & 0x00FF);
    addr_abs = (addr_high_ptr << 8) | addr_low_ptr;
    return 0;
}

uint8_t OLC6502::ADC()
{
    // 加法指令
    const uint8_t data = read(addr_abs);
    const uint16_t temp = static_cast<uint16_t>(accumulator_register) 
        + static_cast<uint16_t>(data) 
        + static_cast<uint16_t>(getFlag(Flag::E_CARRAY_BIT));
    // 如果temp大于255，则设置进位标志
    setFlag(Flag::E_CARRAY_BIT, temp > 255U);
    // 如果结果为0，则设置零标志
    setFlag(Flag::E_ZERO, (temp & 0x00FF) == 0x00);
    // 是否设置溢出标志
    setFlag(Flag::E_OVERFLOW,
        (~(static_cast<uint16_t>(accumulator_register) + static_cast<uint16_t>(data)))
        & (static_cast<uint16_t>(accumulator_register) ^ static_cast<uint16_t>(temp))
        & 0x80);
    // 是否为负数
    setFlag(Flag::E_NEGATIVE, temp & 0x80);
    // 将结果写回到acc寄存器中,(注意数据大小是8bit)
    accumulator_register = temp & 0x00FF;
    return 0;
}

uint8_t OLC6502::AND()
{
    // 逻辑与
    const uint8_t data = read(addr_abs);
    accumulator_register = accumulator_register & data;
    setFlag(Flag::E_ZERO, accumulator_register == 0x00);
    setFlag(Flag::E_NEGATIVE, accumulator_register & 0x80);
    return 1;
}

uint8_t OLC6502::ASL()
{
    return 0;
}

uint8_t OLC6502::BCC()
{
    // 无进位跳转指令
    if (getFlag(Flag::E_CARRAY_BIT) == 0x00) {
        cycles++;
        addr_abs = pc + addr_rel;
        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) {
            cycles++;
        }
        pc = addr_abs;
    }
    return 0;
}

uint8_t OLC6502::BCS()
{
    // 进位跳转指令
    if (getFlag(Flag::E_CARRAY_BIT) == 0x01) {
        cycles++;
        addr_abs = pc + addr_rel;
        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) {
            cycles++;
        }
        pc = addr_abs;
    }
    return 0;
}

uint8_t OLC6502::BEQ()
{
    // 相等转跳
    if (getFlag(Flag::E_ZERO) == 0x01) {
        cycles++;
        addr_abs = pc + addr_rel;
        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) {
            cycles++;
        }
        pc = addr_abs;
    }
    return 0;
}

uint8_t OLC6502::BIT()
{
    return 0;
}

uint8_t OLC6502::BMI()
{
    // 负数转跳
    if (getFlag(Flag::E_NEGATIVE) == 0x01) {
        cycles++;
        addr_abs = pc + addr_rel;
        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) {
            cycles++;
        }
        pc = addr_abs;
    }
    return 0;
}
uint8_t OLC6502::BNE()
{
    // 不相等跳转
    if (getFlag(Flag::E_NEGATIVE) == 0x00) {
        cycles++;
        addr_abs = pc + addr_rel;
        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) {
            cycles++;
        }
        pc = addr_abs;
    }
    return 0;
}
uint8_t OLC6502::BPL()
{
    // 正数或零跳转
    if (getFlag(Flag::E_NEGATIVE) == 0x00) {
        cycles++;
        addr_abs = pc + addr_rel;
        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) {
            cycles++;
        }
        pc = addr_abs;
    }
    return 0;
}
uint8_t OLC6502::BRK()
{
    return 0;
}
uint8_t OLC6502::BVC()
{
    // 溢出跳转
    if (getFlag(Flag::E_OVERFLOW) == 0x00) {
        cycles++;
        addr_abs = pc + addr_rel;
        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) {
            cycles++;
        }
        pc = addr_abs;
    }
    return 0;
}
uint8_t OLC6502::BVS()
{
    // 溢出跳转
    if (getFlag(Flag::E_OVERFLOW) == 0x01) {
        cycles++;
        addr_abs = pc + addr_rel;
        if ((addr_abs & 0xFF00) != (pc & 0xFF00)) {
            cycles++;
        }
        pc = addr_abs;
    }
    return 0;
}
uint8_t OLC6502::CLC()
{
    // 清除进位标志指令
    setFlag(Flag::E_CARRAY_BIT, false);
    return 0;
}
uint8_t OLC6502::CLD()
{
    // 清除十进制标志指令
    setFlag(Flag::E_DECIMAL_MODE, false);
    return 0;
}
uint8_t OLC6502::CLI()
{
    // 清除中断标志指令
    setFlag(Flag::E_DISABLE_INTERRUPTS, false);
    return 0;
}
uint8_t OLC6502::CLV()
{
    // 清除溢出标志指令
    setFlag(Flag::E_OVERFLOW, false);
    return 0;
}
uint8_t OLC6502::CMP()
{
    return 0;
}
uint8_t OLC6502::CPX()
{
    return 0;
}
uint8_t OLC6502::CPY()
{
    return 0;
}
uint8_t OLC6502::DEC()
{
    return 0;
}
uint8_t OLC6502::DEX()
{
    return 0;
}
uint8_t OLC6502::DEY()
{
    return 0;
}
uint8_t OLC6502::EOR()
{
    return 0;
}
uint8_t OLC6502::INC()
{
    return 0;
}
uint8_t OLC6502::INX()
{
    return 0;
}
uint8_t OLC6502::INY()
{
    return 0;
}
uint8_t OLC6502::JMP()
{
    return 0;
}
uint8_t OLC6502::JSR()
{
    return 0;
}
uint8_t OLC6502::LDA()
{
    return 0;
}
uint8_t OLC6502::LDX()
{
    return 0;
}
uint8_t OLC6502::LDY()
{
    return 0;
}
uint8_t OLC6502::LSR()
{
    return 0;
}
uint8_t OLC6502::NOP()
{
    return 0;
}
uint8_t OLC6502::ORA()
{
    return 0;
}
uint8_t OLC6502::PHA()
{
    return 0;
}
uint8_t OLC6502::PHP()
{
    return 0;
}
uint8_t OLC6502::PLA()
{
    return 0;
}
uint8_t OLC6502::PLP()
{
    return 0;
}
uint8_t OLC6502::ROL()
{
    return 0;
}
uint8_t OLC6502::ROR()
{
    return 0;
}
uint8_t OLC6502::RTI()
{
    return 0;
}
uint8_t OLC6502::RTS()
{
    return 0;
}
uint8_t OLC6502::SBC()
{
    // 减法指令

    return 0;
}
uint8_t OLC6502::SEC()
{
    return 0;
}
uint8_t OLC6502::SED()
{
    return 0;
}
uint8_t OLC6502::SEI()
{
    return 0;
}
uint8_t OLC6502::STA()
{
    return 0;
}
uint8_t OLC6502::STX()
{
    return 0;
}
uint8_t OLC6502::STY()
{
    return 0;
}
uint8_t OLC6502::TAX()
{
    return 0;
}
uint8_t OLC6502::TAY()
{
    return 0;
}
uint8_t OLC6502::TSX()
{
    return 0;
}
uint8_t OLC6502::TXA()
{
    return 0;
}
uint8_t OLC6502::TXS()
{
    return 0;
}
uint8_t OLC6502::TYA()
{
    return 0;
}
uint8_t OLC6502::XXX()
{
    return 0;
}
void OLC6502::clock()
{
    if (cycles == 0) {
        opcode = read(pc);
        setFlag(Flag::E_UNUSED, true);
        pc++;
        const auto& instruction = lookup[opcode];
        cycles = instruction.cycles;
        const auto additional_cycle1 = (this->*instruction.addrmode)();
        const auto additional_cycle2 = (this->*instruction.operate)();
        cycles += (additional_cycle1 & additional_cycle2); // 这里只表示两个操作是否影响了周期，影响了则周期+1，否则不变,后续优化实现TODO
        setFlag(Flag::E_UNUSED, true);
    }

    cycle_count++;

    cycles--;
}
}
