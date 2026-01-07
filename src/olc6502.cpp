#include "olc6502.h"

#include <cstdio>
#include <spdlog/spdlog.h>

#include "bus.h"

namespace nes {
void OLC6502::write(uint16_t address, uint8_t data)
{
    if (!bus.expired()) {
        bus.lock()->write(address, data);
    }
    else {
        printf("Error: bus is nullptr!");
    }

}

uint8_t OLC6502::read(uint16_t address)
{
    if (!bus.expired()) {
        return bus.lock()->read(address);
    }
    else {
        spdlog::error("Error: bus is nullptr!");
    }
}

void OLC6502::reset()
{
    // Get address to set program counter to
    addr_abs = 0xFFFC;
    const uint16_t lo = read(addr_abs + 0);
    const uint16_t hi = read(addr_abs + 1);

    // Set it
    pc = (hi << 8) | lo;

    a = 0;
    x = 0;
    y = 0;
    sp = 0xFC;
    status = 0x00 | Flag::U;

    addr_abs = 0x0000;
    addr_rel = 0x0000;

    cycles = 8;
}

void OLC6502::irq()
{
    if (getFlag(I) == 0) {
        // 保存上下文
        write(STACK_OFFSET + static_cast<uint16_t>(sp), (pc >> 8) & 0x00FF);
        sp--;
        write(STACK_OFFSET + static_cast<uint16_t>(sp), pc & 0x00FF);
        sp--;

        setFlag(B, 0);
        setFlag(I, 1);
        setFlag(U, 1);

        write(STACK_OFFSET + static_cast<uint16_t>(sp), status);
        sp--;

        addr_abs = 0xFFFE;
        const uint16_t lo = read(addr_abs);
        const uint16_t hi = read(addr_abs + 1);
        pc = hi << 8 | lo;
        cycles = 7;
    }
}

void OLC6502::nmi()
{
    // 保存上下文
    write(STACK_OFFSET + static_cast<uint16_t>(sp), (pc >> 8) & 0x00FF);
    sp--;
    write(STACK_OFFSET + static_cast<uint16_t>(sp), pc & 0x00FF);
    sp--;

    setFlag(B, 0);
    setFlag(I, 1);
    setFlag(U, 1);

    write(STACK_OFFSET + static_cast<uint16_t>(sp), status);
    sp--;

    addr_abs = 0xFFFA;
    const uint16_t lo = read(addr_abs);
    const uint16_t hi = read(addr_abs + 1);
    pc = hi << 8 | lo;
    cycles = 7;
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
    addr_abs = static_cast<uint16_t>(read(pc) + x);
    pc++;
    addr_abs &= 0x00FF;
    return 0;
}

uint8_t OLC6502::ZPY()
{
    // 零页Y变址，类似于零页X变址，只不过是使用Y寄存器作为偏移量
    addr_abs = static_cast<uint16_t>(read(pc) + y);
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
    addr_abs += x;
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
    addr_abs += y;
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
    const uint16_t addr_ptr_zp = static_cast<uint16_t>(read(pc) + x);
    pc++;
    const uint16_t addr_low_ptr = static_cast<uint16_t>(read(addr_ptr_zp) & 0x00FF);
    const uint16_t addr_high_ptr = static_cast<uint16_t>(read(addr_ptr_zp + 1) & 0x00FF);
    addr_abs = (addr_high_ptr << 8) | addr_low_ptr;
    return 0;
}

uint8_t OLC6502::IZY()
{
    // 先变址Y后间接寻址
    const uint16_t addr_ptr_zp = static_cast<uint16_t>(read(pc) + x);
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
    const uint16_t temp = static_cast<uint16_t>(a) 
        + static_cast<uint16_t>(data) 
        + static_cast<uint16_t>(getFlag(Flag::C));
    // 如果temp大于255，则设置进位标志
    setFlag(Flag::C, temp > 255U);
    // 如果结果为0，则设置零标志
    setFlag(Flag::Z, (temp & 0x00FF) == 0x00);
    // 是否设置溢出标志
    setFlag(Flag::V,
        (~(static_cast<uint16_t>(a) + static_cast<uint16_t>(data)))
        & (static_cast<uint16_t>(a) ^ static_cast<uint16_t>(temp))
        & 0x80);
    // 是否为负数
    setFlag(Flag::N, temp & 0x80);
    // 将结果写回到acc寄存器中,(注意数据大小是8bit)
    a = temp & 0x00FF;
    return 0;
}

uint8_t OLC6502::AND()
{
    // 逻辑与
    const uint8_t data = read(addr_abs);
    a = a & data;
    setFlag(Flag::Z, a == 0x00);
    setFlag(Flag::N, a & 0x80);
    return 1;
}

uint8_t OLC6502::ASL()
{
    return 0;
}

uint8_t OLC6502::BCC()
{
    // 无进位跳转指令
    if (getFlag(Flag::C) == 0x00) {
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
    if (getFlag(Flag::C) == 0x01) {
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
    if (getFlag(Flag::Z) == 0x01) {
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
    if (getFlag(Flag::N) == 0x01) {
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
    if (getFlag(Flag::N) == 0x00) {
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
    if (getFlag(Flag::N) == 0x00) {
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
    if (getFlag(Flag::V) == 0x00) {
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
    if (getFlag(Flag::V) == 0x01) {
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
    setFlag(Flag::C, false);
    return 0;
}
uint8_t OLC6502::CLD()
{
    // 清除十进制标志指令
    setFlag(Flag::D, false);
    return 0;
}
uint8_t OLC6502::CLI()
{
    // 清除中断标志指令
    setFlag(Flag::I, false);
    return 0;
}
uint8_t OLC6502::CLV()
{
    // 清除溢出标志指令
    setFlag(Flag::V, false);
    return 0;
}
uint8_t OLC6502::CMP()
{
    // 累加寄存器与内存数据比较
    const uint16_t data = static_cast<uint16_t>(read(addr_abs));
    const uint16_t tmp = a - data;
    setFlag(C, a > data);
    setFlag(Z, tmp == 0x0000);
    setFlag(N, tmp & 0x0080);

    return 0;
}
uint8_t OLC6502::CPX()
{
    // X寄存器与内存数据比较
    const uint16_t data = static_cast<uint16_t>(read(addr_abs));
    const uint16_t tmp = x - data;
    setFlag(C, x > data);
    setFlag(Z, tmp == 0x0000);
    setFlag(N, tmp & 0x0080);
    return 0;
}
uint8_t OLC6502::CPY()
{
    // Y寄存器与内存数据比较
    const uint16_t data = static_cast<uint16_t>(read(addr_abs));
    const uint16_t tmp = y - data;
    setFlag(C, y > data);
    setFlag(Z, tmp == 0x0000);
    setFlag(N, tmp & 0x0080);
    return 0;
}
uint8_t OLC6502::DEC()
{
    // 对内存中的数值减一
    const uint8_t data = read(addr_abs);
    const uint8_t tmp = data - 0x01;
    setFlag(Z, tmp == 0x00);
    setFlag(N, tmp & 0x0080);
    write(addr_abs, tmp & 0x00FF);

    return 0;
}
uint8_t OLC6502::DEX()
{
    // 对x寄存器数值减一
    x--;
    setFlag(Z, x == 0x00);
    setFlag(N, x & 0x0080);

    return 0;
}
uint8_t OLC6502::DEY()
{
    // 对y寄存器数值减一
    y--;
    setFlag(Z, y == 0x00);
    setFlag(N, y & 0x0080);

    return 0;
}
uint8_t OLC6502::EOR()
{
    // 将内存的值与累加寄存器的值进行异或操作
    const uint8_t data = read(addr_abs);
    a ^= data;
    setFlag(Z, a == 0);
    setFlag(N, a & 0x0080);

    return 1;
}
uint8_t OLC6502::INC()
{
    // 将内存的值加1
    const uint8_t data = read(addr_abs);
    const uint8_t tmp = data + 1;
    setFlag(Z, tmp == 0);
    setFlag(N, tmp & 0x80);
    write(pc, tmp);

    return 0;
}
uint8_t OLC6502::INX()
{
    // 将x寄存器的值加1
    x++;
    setFlag(Z, x == 0);
    setFlag(N, x & 0x80);
    return 0;
}
uint8_t OLC6502::INY()
{
    // 将y寄存器的值加1
    y++;
    setFlag(Z, y == 0);
    setFlag(N, y & 0x80);
    return 0;
}
uint8_t OLC6502::JMP()
{
    // 转跳指令
    pc = addr_abs;
    return 0;
}
uint8_t OLC6502::JSR()
{
    // 转跳子程序
    // 将当前的pc地址存储到栈中，将pc设置为addr_abs保存的地址
    pc--;
    write(STACK_OFFSET + sp, (pc >> 8) & 0x00FF);
    sp--;
    write(STACK_OFFSET + sp, pc & 0x00FF);
    sp--;
    pc = addr_abs;
    return 0;
}
uint8_t OLC6502::LDA()
{
    // 将内存中的值加载到累加寄存器中
    a = read(addr_abs);
    setFlag(Z, a == 0);
    setFlag(N, a & 0x0080);
    return 0;
}
uint8_t OLC6502::LDX()
{
    // 将内存中的值加载到x寄存器中
    x = read(pc);
    setFlag(Z, x == 0);
    setFlag(N, x & 0x0080);
    return 0;
}
uint8_t OLC6502::LDY()
{
    // 将内存中的值加载到y寄存器中
    y = read(addr_abs);
    setFlag(Z, y == 0);
    setFlag(N, y & 0x0080);
    return 0;
}
uint8_t OLC6502::LSR()
{
    // 逻辑右移
    if (lookup[opcode].operate == &OLC6502::IMP) {
        const uint8_t data = a;
        const uint8_t tmp = data >> 1;
        setFlag(C, a & 0x01);
        setFlag(Z, tmp == 0);
        setFlag(N, 0);
        a = tmp;
    }
    else {
        const uint8_t data = read(addr_abs);
        const uint8_t tmp = data >> 1;
        setFlag(C, a & 0x01);
        setFlag(Z, tmp == 0);
        setFlag(N, 0);
        write(addr_abs, tmp);
    }
    
    return 0;
}
uint8_t OLC6502::NOP()
{
    // 无操作指令，只为了消耗cpu周期
    switch (opcode) {
    case 0x1C:
    case 0x3C:
    case 0x5C:
    case 0x7C:
    case 0xDC:
    case 0xFC:
        return 1;
        break;
    }
    return 0;
}
uint8_t OLC6502::ORA()
{
    const uint8_t data = read(addr_abs);
    a |= data;
    setFlag(Z, a == 0);
    setFlag(N, a & 0x80);
    return 0;
}
uint8_t OLC6502::PHA()
{
    // 入栈指令
    write(STACK_OFFSET + static_cast<uint16_t>(sp), a);
    sp--;

    return 0;
}
uint8_t OLC6502::PHP()
{
    write(STACK_OFFSET + sp, status | B | U);
    setFlag(B, 0);
    setFlag(U, 0);
    sp--;

    return 0;
}
uint8_t OLC6502::PLA()
{
    // 出栈指令
    sp++;
    a = read(STACK_OFFSET + static_cast<uint16_t>(sp));
    setFlag(Z, a == 0x00);
    setFlag(N, a & 0x80);

    return 0;
}
uint8_t OLC6502::PLP()
{
    // 从栈中弹出存储的状态值
    sp++;
    status = read(STACK_OFFSET + static_cast<uint16_t>(sp));
    setFlag(U, 1);

    return 0;
}
uint8_t OLC6502::ROL()
{
    // 向左旋转.ROL指令将内存值或累加器向左移位，将每个位的值移入下一个位，
    // 并将进位标志视为同时位于位7之上和位0之下
    if (lookup[opcode].operate == &OLC6502::IMP) {
        const uint8_t data = a;
        const uint8_t tmp = (data << 1) | getFlag(C);
        setFlag(C, data & 0x80);
        setFlag(Z, tmp == 0);
        setFlag(N, tmp & 0x80);
        a = tmp;
    }
    else {
        const uint8_t data = read(addr_abs);
        const uint8_t tmp = (data << 1) | getFlag(C);
        setFlag(C, data & 0x80);
        setFlag(Z, tmp == 0);
        setFlag(N, tmp & 0x80);
        write(addr_abs, tmp);
    }
    return 0;
}
uint8_t OLC6502::ROR()
{
    // 向右旋转.ROL指令将内存值或累加器向左移位，将每个位的值移入下一个位， 
    // 并将进位标志视为同时位于位7之上和位0之下
    if (lookup[opcode].operate == &OLC6502::IMP) {
        const uint8_t data = a;
        const uint8_t tmp = (data >> 1) | (getFlag(C)  << 7);
        setFlag(C, data & 0x01);
        setFlag(Z, tmp == 0);
        setFlag(N, tmp & 0x80);
        a = tmp;
    }
    else {
        const uint8_t data = read(addr_abs);
        const uint8_t tmp = (data << 1) | (getFlag(C) << 7);
        setFlag(C, data & 0x01);
        setFlag(Z, tmp == 0);
        setFlag(N, tmp & 0x80);
        write(addr_abs, tmp);
    }
    return 0;
}
uint8_t OLC6502::RTI()
{
    // 返回中断位置函数
    sp++;
    status = read(STACK_OFFSET + sp);
    status &= ~B;
    status &= ~U;
    sp++;
    const uint8_t lo = read(STACK_OFFSET + sp);
    sp++;
    const uint8_t hi = read(STACK_OFFSET + sp);
    pc = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);

    return 0;
}
uint8_t OLC6502::RTS()
{
    sp++;
    const uint8_t lo = read(STACK_OFFSET + sp);
    sp++;
    const uint8_t hi = read(STACK_OFFSET + sp);
    pc = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
    pc++;

    return 0;
}
uint8_t OLC6502::SBC()
{
    // 减法指令
    const uint8_t data = read(addr_abs);
    const uint16_t temp = static_cast<uint16_t>(a)
        + static_cast<uint16_t>(data)
        + static_cast<uint16_t>(getFlag(Flag::C));
    // 如果temp大于255，则设置进位标志
    setFlag(Flag::C, temp & 0x00FF);
    // 如果结果为0，则设置零标志
    setFlag(Flag::Z, (temp & 0x00FF) == 0x00);
    // 是否设置溢出标志
    setFlag(Flag::V,
        ((static_cast<uint16_t>(a) + static_cast<uint16_t>(data)))
        & (static_cast<uint16_t>(a) ^ static_cast<uint16_t>(temp))
        & 0x80);
    // 是否为负数
    setFlag(Flag::N, temp & 0x80);
    // 将结果写回到acc寄存器中,(注意数据大小是8bit)
    a = temp & 0x00FF;
    return 0;
}
uint8_t OLC6502::SEC()
{
    setFlag(C, 1);
    return 0;
}
uint8_t OLC6502::SED()
{
    setFlag(D, 1);
    return 0;
}
uint8_t OLC6502::SEI()
{
    setFlag(I, 1);
    return 0;
}
uint8_t OLC6502::STA()
{
    write(addr_abs, a);
    return 0;
}
uint8_t OLC6502::STX()
{
    write(addr_abs, x);
    return 0;
}
uint8_t OLC6502::STY()
{
    write(addr_abs, y);
    return 0;
}
uint8_t OLC6502::TAX()
{
    x = read(addr_abs);
    setFlag(Z, x == 0);
    setFlag(N, x & 0x80);
    return 0;
}
uint8_t OLC6502::TAY()
{
    y = read(addr_abs);
    setFlag(Z, y == 0);
    setFlag(N, y & 0x80);
    return 0;
}
uint8_t OLC6502::TSX()
{
    x = sp;
    setFlag(Z, x == 0);
    setFlag(N, x & 0x80);
    return 0;
}
uint8_t OLC6502::TXA()
{
    a = x;
    setFlag(Z, a == 0);
    setFlag(N, a & 0x80);
    return 0;
}
uint8_t OLC6502::TXS()
{
    sp = x;
    return 0;
}
uint8_t OLC6502::TYA()
{
    a = y;
    return 0;
}
uint8_t OLC6502::XXX()
{
    return 0;
}
std::unordered_map<uint16_t, std::string> OLC6502::disassemble(uint16_t nStart, uint16_t len)
{
    uint32_t addr = nStart;
    uint8_t value = 0x00;
    uint8_t lo = 0x00;
    uint8_t hi = 0x00;
    std::unordered_map<uint16_t, std::string> mapLines;
    uint16_t line_addr = 0;

    auto hex = [](uint32_t n, uint8_t d) -> std::string
    {
        std::string s(d, '0');
        for (auto i = d - 1; i >= 0, i--; n >>= 4) {
            s[i] = "0123456789ABCDEF"[n & 0xF];
        }
        return s;
    };

    std::string sInst = "$" + hex(addr, 4) + ": ";
    uint8_t opcode = 0;
    if (!bus.expired()) {
        uint8_t opcode = bus.lock()->read(addr); addr++;

        sInst += lookup[opcode].name + " ";
        if (lookup[opcode].addrmode == &OLC6502::IMP)
        {
            sInst += " {IMP}";
        }
        else if (lookup[opcode].addrmode == &OLC6502::IMM)
        {
            value = bus.lock()->read(addr); addr++;
            sInst += "#$" + hex(value, 2) + " {IMM}";
        }
        else if (lookup[opcode].addrmode == &OLC6502::ZP0)
        {
            lo = bus.lock()->read(addr); addr++;
            hi = 0x00;
            sInst += "$" + hex(lo, 2) + " {ZP0}";
        }
        else if (lookup[opcode].addrmode == &OLC6502::ZPX)
        {
            lo = bus.lock()->read(addr); addr++;
            hi = 0x00;
            sInst += "$" + hex(lo, 2) + ", X {ZPX}";
        }
        else if (lookup[opcode].addrmode == &OLC6502::ZPY)
        {
            lo = bus.lock()->read(addr); addr++;
            hi = 0x00;
            sInst += "$" + hex(lo, 2) + ", Y {ZPY}";
        }
        else if (lookup[opcode].addrmode == &OLC6502::IZX)
        {
            lo = bus.lock()->read(addr); addr++;
            hi = 0x00;
            sInst += "($" + hex(lo, 2) + ", X) {IZX}";
        }
        else if (lookup[opcode].addrmode == &OLC6502::IZY)
        {
            lo = bus.lock()->read(addr); addr++;
            hi = 0x00;
            sInst += "($" + hex(lo, 2) + "), Y {IZY}";
        }
        else if (lookup[opcode].addrmode == &OLC6502::ABS)
        {
            lo = bus.lock()->read(addr); addr++;
            hi = bus.lock()->read(addr); addr++;
            sInst += "$" + hex((uint16_t)(hi << 8) | lo, 4) + " {ABS}";
        }
        else if (lookup[opcode].addrmode == &OLC6502::ABX)
        {
            lo = bus.lock()->read(addr); addr++;
            hi = bus.lock()->read(addr); addr++;
            sInst += "$" + hex((uint16_t)(hi << 8) | lo, 4) + ", X {ABX}";
        }
        else if (lookup[opcode].addrmode == &OLC6502::ABY)
        {
            lo = bus.lock()->read(addr); addr++;
            hi = bus.lock()->read(addr); addr++;
            sInst += "$" + hex((uint16_t)(hi << 8) | lo, 4) + ", Y {ABY}";
        }
        else if (lookup[opcode].addrmode == &OLC6502::IND)
        {
            lo = bus.lock()->read(addr); addr++;
            hi = bus.lock()->read(addr); addr++;
            sInst += "($" + hex((uint16_t)(hi << 8) | lo, 4) + ") {IND}";
        }
        else if (lookup[opcode].addrmode == &OLC6502::REL)
        {
            value = bus.lock()->read(addr); addr++;
            sInst += "$" + hex(value, 2) + " [$" + hex(addr + value, 4) + "] {REL}";
        }

    }
    else {
        spdlog::error("bus point is expired!");
    }
    // Add the formed string to a std::map, using the instruction's
    // address as the key. This makes it convenient to look for later
    // as the instructions are variable in length, so a straight up
    // incremental index is not sufficient.
    mapLines[line_addr] = sInst;

    return mapLines;
}
bool OLC6502::complete()
{
    return cycles == 0;
}
void OLC6502::clock()
{
    if (cycles == 0) {
        opcode = read(pc);
        setFlag(Flag::U, true);
        pc++;
        const auto& instruction = lookup[opcode];
        cycles = instruction.cycles;
        const auto additional_cycle1 = (this->*instruction.addrmode)();
        const auto additional_cycle2 = (this->*instruction.operate)();
        cycles += (additional_cycle1 & additional_cycle2); // 这里只表示两个操作是否影响了周期，影响了则周期+1，否则不变,后续优化实现TODO
        setFlag(Flag::U, true);

        spdlog::info("cycle_count:{}, instruction:{}, cycles:{}, Register a:{}, x:{}, y:{}, status:{}; sp:{}, pc: {}",
            cycle_count, instruction.name, instruction.cycles, a, 
            x, y, status, sp, pc);
    }

    cycle_count++;
    cycles--;
}
}
