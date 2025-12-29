#ifndef OLC6502_H
#define OLC6502_H

#include <cstdint>
#include <string>
#include <vector>

namespace nes {

constexpr uint16_t STACK_OFFSET = 0x0100;

class Bus;

enum Flag : uint8_t
{
    E_CARRAY_BIT = (1 << 0),	        // 进位标志位
    E_ZERO = (1 << 1),	                // 零标志位
    E_DISABLE_INTERRUPTS = (1 << 2),	// 关闭中断标志位
    E_DECIMAL_MODE = (1 << 3),	        // Decimal Mode (unused in this implementation)
    E_BREAK = (1 << 4),	                // Break
    E_UNUSED = (1 << 5),	            // Unused
    E_OVERFLOW = (1 << 6),	            // Overflow
    E_NEGATIVE = (1 << 7),	            // Negative
};

class OLC6502 {
public:
    explicit OLC6502() = default;
    ~OLC6502() = default;

    OLC6502(const OLC6502&) = delete;
    void operator=(const OLC6502&) = delete;
    void operator=(const OLC6502&&) = delete;

    void connectBus(Bus* bus) {
        this->bus = bus;
    }

    void write(uint16_t address, uint8_t data);

    uint8_t read(uint16_t address);

    void reset();
    void irq(); // 中断请求函数
    void nmi(); // 不可屏蔽中断请求函数

public:
    uint8_t  accumulator_register = 0x00;		// 累计寄存器
    uint8_t  x_register = 0x00;		            // X 寄存器
    uint8_t  y_register = 0x00;		            // Y 寄存器
    uint8_t  sp = 0x00;		                    // 堆栈指针
    uint16_t pc = 0x0000;	                    // 程序计数器
    uint8_t  status_register = 0x00;		    // 状态寄存器

private:
    uint8_t IMP();	uint8_t IMM();
    uint8_t ZP0();	uint8_t ZPX();
    uint8_t ZPY();	uint8_t REL();
    uint8_t ABS();	uint8_t ABX();
    uint8_t ABY();	uint8_t IND();
    uint8_t IZX();	uint8_t IZY();

    uint8_t ADC();	uint8_t AND();	uint8_t ASL();	uint8_t BCC();
    uint8_t BCS();	uint8_t BEQ();	uint8_t BIT();	uint8_t BMI();
    uint8_t BNE();	uint8_t BPL();	uint8_t BRK();	uint8_t BVC();
    uint8_t BVS();	uint8_t CLC();	uint8_t CLD();	uint8_t CLI();
    uint8_t CLV();	uint8_t CMP();	uint8_t CPX();	uint8_t CPY();
    uint8_t DEC();	uint8_t DEX();	uint8_t DEY();	uint8_t EOR();
    uint8_t INC();	uint8_t INX();	uint8_t INY();	uint8_t JMP();
    uint8_t JSR();	uint8_t LDA();	uint8_t LDX();	uint8_t LDY();
    uint8_t LSR();	uint8_t NOP();	uint8_t ORA();	uint8_t PHA();
    uint8_t PHP();	uint8_t PLA();	uint8_t PLP();	uint8_t ROL();
    uint8_t ROR();	uint8_t RTI();	uint8_t RTS();	uint8_t SBC();
    uint8_t SEC();	uint8_t SED();	uint8_t SEI();	uint8_t STA();
    uint8_t STX();	uint8_t STY();	uint8_t TAX();	uint8_t TAY();
    uint8_t TSX();	uint8_t TXA();	uint8_t TXS();	uint8_t TYA();

    uint8_t XXX();

    uint8_t getFlag(Flag flag) {
        return (status_register & flag) > 0 ? 1 : 0;
    }
    
    void setFlag(Flag flag, bool value) {
        if (value) {
            status_register |= flag;
        }
        else {
            status_register &= ~flag;
        }
    }

    void clock();

private:
    Bus* bus;
    uint16_t addr_abs = 0x0000;
    uint16_t addr_rel = 0x0000;
    uint8_t opcode = 0x00;
    uint8_t cycles = 0x00;
    uint64_t cycle_count = 0LLU;

    struct Instruction
    {
        std::string name;
        uint8_t(OLC6502::* operate)(void) = nullptr;
        uint8_t(OLC6502::* addrmode)(void) = nullptr;
        uint8_t cycles = 0;
    };

    const std::vector<Instruction> lookup = {
        { "BRK", &nes::OLC6502::BRK, &nes::OLC6502::IMM, 7 },{ "ORA", &nes::OLC6502::ORA, &nes::OLC6502::IZX, 6 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 8 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 3 },{ "ORA", &nes::OLC6502::ORA, &nes::OLC6502::ZP0, 3 },{ "ASL", &nes::OLC6502::ASL, &nes::OLC6502::ZP0, 5 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 5 },{ "PHP", &nes::OLC6502::PHP, &nes::OLC6502::IMP, 3 },{ "ORA", &nes::OLC6502::ORA, &nes::OLC6502::IMM, 2 },{ "ASL", &nes::OLC6502::ASL, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 4 },{ "ORA", &nes::OLC6502::ORA, &nes::OLC6502::ABS, 4 },{ "ASL", &nes::OLC6502::ASL, &nes::OLC6502::ABS, 6 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 6 },
        { "BPL", &nes::OLC6502::BPL, &nes::OLC6502::REL, 2 },{ "ORA", &nes::OLC6502::ORA, &nes::OLC6502::IZY, 5 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 8 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 4 },{ "ORA", &nes::OLC6502::ORA, &nes::OLC6502::ZPX, 4 },{ "ASL", &nes::OLC6502::ASL, &nes::OLC6502::ZPX, 6 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 6 },{ "CLC", &nes::OLC6502::CLC, &nes::OLC6502::IMP, 2 },{ "ORA", &nes::OLC6502::ORA, &nes::OLC6502::ABY, 4 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 7 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 4 },{ "ORA", &nes::OLC6502::ORA, &nes::OLC6502::ABX, 4 },{ "ASL", &nes::OLC6502::ASL, &nes::OLC6502::ABX, 7 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 7 },
        { "JSR", &nes::OLC6502::JSR, &nes::OLC6502::ABS, 6 },{ "AND", &nes::OLC6502::AND, &nes::OLC6502::IZX, 6 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 8 },{ "BIT", &nes::OLC6502::BIT, &nes::OLC6502::ZP0, 3 },{ "AND", &nes::OLC6502::AND, &nes::OLC6502::ZP0, 3 },{ "ROL", &nes::OLC6502::ROL, &nes::OLC6502::ZP0, 5 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 5 },{ "PLP", &nes::OLC6502::PLP, &nes::OLC6502::IMP, 4 },{ "AND", &nes::OLC6502::AND, &nes::OLC6502::IMM, 2 },{ "ROL", &nes::OLC6502::ROL, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "BIT", &nes::OLC6502::BIT, &nes::OLC6502::ABS, 4 },{ "AND", &nes::OLC6502::AND, &nes::OLC6502::ABS, 4 },{ "ROL", &nes::OLC6502::ROL, &nes::OLC6502::ABS, 6 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 6 },
        { "BMI", &nes::OLC6502::BMI, &nes::OLC6502::REL, 2 },{ "AND", &nes::OLC6502::AND, &nes::OLC6502::IZY, 5 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 8 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 4 },{ "AND", &nes::OLC6502::AND, &nes::OLC6502::ZPX, 4 },{ "ROL", &nes::OLC6502::ROL, &nes::OLC6502::ZPX, 6 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 6 },{ "SEC", &nes::OLC6502::SEC, &nes::OLC6502::IMP, 2 },{ "AND", &nes::OLC6502::AND, &nes::OLC6502::ABY, 4 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 7 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 4 },{ "AND", &nes::OLC6502::AND, &nes::OLC6502::ABX, 4 },{ "ROL", &nes::OLC6502::ROL, &nes::OLC6502::ABX, 7 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 7 },
        { "RTI", &nes::OLC6502::RTI, &nes::OLC6502::IMP, 6 },{ "EOR", &nes::OLC6502::EOR, &nes::OLC6502::IZX, 6 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 8 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 3 },{ "EOR", &nes::OLC6502::EOR, &nes::OLC6502::ZP0, 3 },{ "LSR", &nes::OLC6502::LSR, &nes::OLC6502::ZP0, 5 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 5 },{ "PHA", &nes::OLC6502::PHA, &nes::OLC6502::IMP, 3 },{ "EOR", &nes::OLC6502::EOR, &nes::OLC6502::IMM, 2 },{ "LSR", &nes::OLC6502::LSR, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "JMP", &nes::OLC6502::JMP, &nes::OLC6502::ABS, 3 },{ "EOR", &nes::OLC6502::EOR, &nes::OLC6502::ABS, 4 },{ "LSR", &nes::OLC6502::LSR, &nes::OLC6502::ABS, 6 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 6 },
        { "BVC", &nes::OLC6502::BVC, &nes::OLC6502::REL, 2 },{ "EOR", &nes::OLC6502::EOR, &nes::OLC6502::IZY, 5 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 8 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 4 },{ "EOR", &nes::OLC6502::EOR, &nes::OLC6502::ZPX, 4 },{ "LSR", &nes::OLC6502::LSR, &nes::OLC6502::ZPX, 6 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 6 },{ "CLI", &nes::OLC6502::CLI, &nes::OLC6502::IMP, 2 },{ "EOR", &nes::OLC6502::EOR, &nes::OLC6502::ABY, 4 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 7 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 4 },{ "EOR", &nes::OLC6502::EOR, &nes::OLC6502::ABX, 4 },{ "LSR", &nes::OLC6502::LSR, &nes::OLC6502::ABX, 7 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 7 },
        { "RTS", &nes::OLC6502::RTS, &nes::OLC6502::IMP, 6 },{ "ADC", &nes::OLC6502::ADC, &nes::OLC6502::IZX, 6 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 8 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 3 },{ "ADC", &nes::OLC6502::ADC, &nes::OLC6502::ZP0, 3 },{ "ROR", &nes::OLC6502::ROR, &nes::OLC6502::ZP0, 5 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 5 },{ "PLA", &nes::OLC6502::PLA, &nes::OLC6502::IMP, 4 },{ "ADC", &nes::OLC6502::ADC, &nes::OLC6502::IMM, 2 },{ "ROR", &nes::OLC6502::ROR, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "JMP", &nes::OLC6502::JMP, &nes::OLC6502::IND, 5 },{ "ADC", &nes::OLC6502::ADC, &nes::OLC6502::ABS, 4 },{ "ROR", &nes::OLC6502::ROR, &nes::OLC6502::ABS, 6 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 6 },
        { "BVS", &nes::OLC6502::BVS, &nes::OLC6502::REL, 2 },{ "ADC", &nes::OLC6502::ADC, &nes::OLC6502::IZY, 5 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 8 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 4 },{ "ADC", &nes::OLC6502::ADC, &nes::OLC6502::ZPX, 4 },{ "ROR", &nes::OLC6502::ROR, &nes::OLC6502::ZPX, 6 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 6 },{ "SEI", &nes::OLC6502::SEI, &nes::OLC6502::IMP, 2 },{ "ADC", &nes::OLC6502::ADC, &nes::OLC6502::ABY, 4 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 7 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 4 },{ "ADC", &nes::OLC6502::ADC, &nes::OLC6502::ABX, 4 },{ "ROR", &nes::OLC6502::ROR, &nes::OLC6502::ABX, 7 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 7 },
        { "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 2 },{ "STA", &nes::OLC6502::STA, &nes::OLC6502::IZX, 6 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 6 },{ "STY", &nes::OLC6502::STY, &nes::OLC6502::ZP0, 3 },{ "STA", &nes::OLC6502::STA, &nes::OLC6502::ZP0, 3 },{ "STX", &nes::OLC6502::STX, &nes::OLC6502::ZP0, 3 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 3 },{ "DEY", &nes::OLC6502::DEY, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 2 },{ "TXA", &nes::OLC6502::TXA, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "STY", &nes::OLC6502::STY, &nes::OLC6502::ABS, 4 },{ "STA", &nes::OLC6502::STA, &nes::OLC6502::ABS, 4 },{ "STX", &nes::OLC6502::STX, &nes::OLC6502::ABS, 4 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 4 },
        { "BCC", &nes::OLC6502::BCC, &nes::OLC6502::REL, 2 },{ "STA", &nes::OLC6502::STA, &nes::OLC6502::IZY, 6 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 6 },{ "STY", &nes::OLC6502::STY, &nes::OLC6502::ZPX, 4 },{ "STA", &nes::OLC6502::STA, &nes::OLC6502::ZPX, 4 },{ "STX", &nes::OLC6502::STX, &nes::OLC6502::ZPY, 4 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 4 },{ "TYA", &nes::OLC6502::TYA, &nes::OLC6502::IMP, 2 },{ "STA", &nes::OLC6502::STA, &nes::OLC6502::ABY, 5 },{ "TXS", &nes::OLC6502::TXS, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 5 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 5 },{ "STA", &nes::OLC6502::STA, &nes::OLC6502::ABX, 5 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 5 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 5 },
        { "LDY", &nes::OLC6502::LDY, &nes::OLC6502::IMM, 2 },{ "LDA", &nes::OLC6502::LDA, &nes::OLC6502::IZX, 6 },{ "LDX", &nes::OLC6502::LDX, &nes::OLC6502::IMM, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 6 },{ "LDY", &nes::OLC6502::LDY, &nes::OLC6502::ZP0, 3 },{ "LDA", &nes::OLC6502::LDA, &nes::OLC6502::ZP0, 3 },{ "LDX", &nes::OLC6502::LDX, &nes::OLC6502::ZP0, 3 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 3 },{ "TAY", &nes::OLC6502::TAY, &nes::OLC6502::IMP, 2 },{ "LDA", &nes::OLC6502::LDA, &nes::OLC6502::IMM, 2 },{ "TAX", &nes::OLC6502::TAX, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "LDY", &nes::OLC6502::LDY, &nes::OLC6502::ABS, 4 },{ "LDA", &nes::OLC6502::LDA, &nes::OLC6502::ABS, 4 },{ "LDX", &nes::OLC6502::LDX, &nes::OLC6502::ABS, 4 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 4 },
        { "BCS", &nes::OLC6502::BCS, &nes::OLC6502::REL, 2 },{ "LDA", &nes::OLC6502::LDA, &nes::OLC6502::IZY, 5 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 5 },{ "LDY", &nes::OLC6502::LDY, &nes::OLC6502::ZPX, 4 },{ "LDA", &nes::OLC6502::LDA, &nes::OLC6502::ZPX, 4 },{ "LDX", &nes::OLC6502::LDX, &nes::OLC6502::ZPY, 4 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 4 },{ "CLV", &nes::OLC6502::CLV, &nes::OLC6502::IMP, 2 },{ "LDA", &nes::OLC6502::LDA, &nes::OLC6502::ABY, 4 },{ "TSX", &nes::OLC6502::TSX, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 4 },{ "LDY", &nes::OLC6502::LDY, &nes::OLC6502::ABX, 4 },{ "LDA", &nes::OLC6502::LDA, &nes::OLC6502::ABX, 4 },{ "LDX", &nes::OLC6502::LDX, &nes::OLC6502::ABY, 4 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 4 },
        { "CPY", &nes::OLC6502::CPY, &nes::OLC6502::IMM, 2 },{ "CMP", &nes::OLC6502::CMP, &nes::OLC6502::IZX, 6 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 8 },{ "CPY", &nes::OLC6502::CPY, &nes::OLC6502::ZP0, 3 },{ "CMP", &nes::OLC6502::CMP, &nes::OLC6502::ZP0, 3 },{ "DEC", &nes::OLC6502::DEC, &nes::OLC6502::ZP0, 5 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 5 },{ "INY", &nes::OLC6502::INY, &nes::OLC6502::IMP, 2 },{ "CMP", &nes::OLC6502::CMP, &nes::OLC6502::IMM, 2 },{ "DEX", &nes::OLC6502::DEX, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "CPY", &nes::OLC6502::CPY, &nes::OLC6502::ABS, 4 },{ "CMP", &nes::OLC6502::CMP, &nes::OLC6502::ABS, 4 },{ "DEC", &nes::OLC6502::DEC, &nes::OLC6502::ABS, 6 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 6 },
        { "BNE", &nes::OLC6502::BNE, &nes::OLC6502::REL, 2 },{ "CMP", &nes::OLC6502::CMP, &nes::OLC6502::IZY, 5 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 8 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 4 },{ "CMP", &nes::OLC6502::CMP, &nes::OLC6502::ZPX, 4 },{ "DEC", &nes::OLC6502::DEC, &nes::OLC6502::ZPX, 6 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 6 },{ "CLD", &nes::OLC6502::CLD, &nes::OLC6502::IMP, 2 },{ "CMP", &nes::OLC6502::CMP, &nes::OLC6502::ABY, 4 },{ "NOP", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 7 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 4 },{ "CMP", &nes::OLC6502::CMP, &nes::OLC6502::ABX, 4 },{ "DEC", &nes::OLC6502::DEC, &nes::OLC6502::ABX, 7 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 7 },
        { "CPX", &nes::OLC6502::CPX, &nes::OLC6502::IMM, 2 },{ "SBC", &nes::OLC6502::SBC, &nes::OLC6502::IZX, 6 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 8 },{ "CPX", &nes::OLC6502::CPX, &nes::OLC6502::ZP0, 3 },{ "SBC", &nes::OLC6502::SBC, &nes::OLC6502::ZP0, 3 },{ "INC", &nes::OLC6502::INC, &nes::OLC6502::ZP0, 5 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 5 },{ "INX", &nes::OLC6502::INX, &nes::OLC6502::IMP, 2 },{ "SBC", &nes::OLC6502::SBC, &nes::OLC6502::IMM, 2 },{ "NOP", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::SBC, &nes::OLC6502::IMP, 2 },{ "CPX", &nes::OLC6502::CPX, &nes::OLC6502::ABS, 4 },{ "SBC", &nes::OLC6502::SBC, &nes::OLC6502::ABS, 4 },{ "INC", &nes::OLC6502::INC, &nes::OLC6502::ABS, 6 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 6 },
        { "BEQ", &nes::OLC6502::BEQ, &nes::OLC6502::REL, 2 },{ "SBC", &nes::OLC6502::SBC, &nes::OLC6502::IZY, 5 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 8 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 4 },{ "SBC", &nes::OLC6502::SBC, &nes::OLC6502::ZPX, 4 },{ "INC", &nes::OLC6502::INC, &nes::OLC6502::ZPX, 6 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 6 },{ "SED", &nes::OLC6502::SED, &nes::OLC6502::IMP, 2 },{ "SBC", &nes::OLC6502::SBC, &nes::OLC6502::ABY, 4 },{ "NOP", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 2 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 7 },{ "???", &nes::OLC6502::NOP, &nes::OLC6502::IMP, 4 },{ "SBC", &nes::OLC6502::SBC, &nes::OLC6502::ABX, 4 },{ "INC", &nes::OLC6502::INC, &nes::OLC6502::ABX, 7 },{ "???", &nes::OLC6502::XXX, &nes::OLC6502::IMP, 7 },
    };



};
}

#endif
