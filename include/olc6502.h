#ifndef OLC6502_H
#define OLC6502_H

#include <cstdint>
#include <string>
#include <vector>

namespace nes {
class Bus;

enum Flag : uint8_t
{
    E_CARRAY_BIT = (1 << 0),	// Carry Bit
    E_ZERO = (1 << 1),	// Zero
    E_DISABLE_INTERRUPTS = (1 << 2),	// Disable Interrupts
    E_DECIMAL_MODE = (1 << 3),	// Decimal Mode (unused in this implementation)
    E_BREAK = (1 << 4),	// Break
    E_UNUSED = (1 << 5),	// Unused
    E_OVERFLOW = (1 << 6),	// Overflow
    E_NEGATIVE = (1 << 7),	// Negative
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
        { "BRK", &BRK, &IMM, 7 },{ "ORA", &ORA, &IZX, 6 },{ "???", &XXX, &IMP, 2 },{ "???", &XXX, &IMP, 8 },{ "???", &NOP, &IMP, 3 },{ "ORA", &ORA, &ZP0, 3 },{ "ASL", &ASL, &ZP0, 5 },{ "???", &XXX, &IMP, 5 },{ "PHP", &PHP, &IMP, 3 },{ "ORA", &ORA, &IMM, 2 },{ "ASL", &ASL, &IMP, 2 },{ "???", &XXX, &IMP, 2 },{ "???", &NOP, &IMP, 4 },{ "ORA", &ORA, &ABS, 4 },{ "ASL", &ASL, &ABS, 6 },{ "???", &XXX, &IMP, 6 },
        { "BPL", &BPL, &REL, 2 },{ "ORA", &ORA, &IZY, 5 },{ "???", &XXX, &IMP, 2 },{ "???", &XXX, &IMP, 8 },{ "???", &NOP, &IMP, 4 },{ "ORA", &ORA, &ZPX, 4 },{ "ASL", &ASL, &ZPX, 6 },{ "???", &XXX, &IMP, 6 },{ "CLC", &CLC, &IMP, 2 },{ "ORA", &ORA, &ABY, 4 },{ "???", &NOP, &IMP, 2 },{ "???", &XXX, &IMP, 7 },{ "???", &NOP, &IMP, 4 },{ "ORA", &ORA, &ABX, 4 },{ "ASL", &ASL, &ABX, 7 },{ "???", &XXX, &IMP, 7 },
        { "JSR", &JSR, &ABS, 6 },{ "AND", &AND, &IZX, 6 },{ "???", &XXX, &IMP, 2 },{ "???", &XXX, &IMP, 8 },{ "BIT", &BIT, &ZP0, 3 },{ "AND", &AND, &ZP0, 3 },{ "ROL", &ROL, &ZP0, 5 },{ "???", &XXX, &IMP, 5 },{ "PLP", &PLP, &IMP, 4 },{ "AND", &AND, &IMM, 2 },{ "ROL", &ROL, &IMP, 2 },{ "???", &XXX, &IMP, 2 },{ "BIT", &BIT, &ABS, 4 },{ "AND", &AND, &ABS, 4 },{ "ROL", &ROL, &ABS, 6 },{ "???", &XXX, &IMP, 6 },
        { "BMI", &BMI, &REL, 2 },{ "AND", &AND, &IZY, 5 },{ "???", &XXX, &IMP, 2 },{ "???", &XXX, &IMP, 8 },{ "???", &NOP, &IMP, 4 },{ "AND", &AND, &ZPX, 4 },{ "ROL", &ROL, &ZPX, 6 },{ "???", &XXX, &IMP, 6 },{ "SEC", &SEC, &IMP, 2 },{ "AND", &AND, &ABY, 4 },{ "???", &NOP, &IMP, 2 },{ "???", &XXX, &IMP, 7 },{ "???", &NOP, &IMP, 4 },{ "AND", &AND, &ABX, 4 },{ "ROL", &ROL, &ABX, 7 },{ "???", &XXX, &IMP, 7 },
        { "RTI", &RTI, &IMP, 6 },{ "EOR", &EOR, &IZX, 6 },{ "???", &XXX, &IMP, 2 },{ "???", &XXX, &IMP, 8 },{ "???", &NOP, &IMP, 3 },{ "EOR", &EOR, &ZP0, 3 },{ "LSR", &LSR, &ZP0, 5 },{ "???", &XXX, &IMP, 5 },{ "PHA", &PHA, &IMP, 3 },{ "EOR", &EOR, &IMM, 2 },{ "LSR", &LSR, &IMP, 2 },{ "???", &XXX, &IMP, 2 },{ "JMP", &JMP, &ABS, 3 },{ "EOR", &EOR, &ABS, 4 },{ "LSR", &LSR, &ABS, 6 },{ "???", &XXX, &IMP, 6 },
        { "BVC", &BVC, &REL, 2 },{ "EOR", &EOR, &IZY, 5 },{ "???", &XXX, &IMP, 2 },{ "???", &XXX, &IMP, 8 },{ "???", &NOP, &IMP, 4 },{ "EOR", &EOR, &ZPX, 4 },{ "LSR", &LSR, &ZPX, 6 },{ "???", &XXX, &IMP, 6 },{ "CLI", &CLI, &IMP, 2 },{ "EOR", &EOR, &ABY, 4 },{ "???", &NOP, &IMP, 2 },{ "???", &XXX, &IMP, 7 },{ "???", &NOP, &IMP, 4 },{ "EOR", &EOR, &ABX, 4 },{ "LSR", &LSR, &ABX, 7 },{ "???", &XXX, &IMP, 7 },
        { "RTS", &RTS, &IMP, 6 },{ "ADC", &ADC, &IZX, 6 },{ "???", &XXX, &IMP, 2 },{ "???", &XXX, &IMP, 8 },{ "???", &NOP, &IMP, 3 },{ "ADC", &ADC, &ZP0, 3 },{ "ROR", &ROR, &ZP0, 5 },{ "???", &XXX, &IMP, 5 },{ "PLA", &PLA, &IMP, 4 },{ "ADC", &ADC, &IMM, 2 },{ "ROR", &ROR, &IMP, 2 },{ "???", &XXX, &IMP, 2 },{ "JMP", &JMP, &IND, 5 },{ "ADC", &ADC, &ABS, 4 },{ "ROR", &ROR, &ABS, 6 },{ "???", &XXX, &IMP, 6 },
        { "BVS", &BVS, &REL, 2 },{ "ADC", &ADC, &IZY, 5 },{ "???", &XXX, &IMP, 2 },{ "???", &XXX, &IMP, 8 },{ "???", &NOP, &IMP, 4 },{ "ADC", &ADC, &ZPX, 4 },{ "ROR", &ROR, &ZPX, 6 },{ "???", &XXX, &IMP, 6 },{ "SEI", &SEI, &IMP, 2 },{ "ADC", &ADC, &ABY, 4 },{ "???", &NOP, &IMP, 2 },{ "???", &XXX, &IMP, 7 },{ "???", &NOP, &IMP, 4 },{ "ADC", &ADC, &ABX, 4 },{ "ROR", &ROR, &ABX, 7 },{ "???", &XXX, &IMP, 7 },
        { "???", &NOP, &IMP, 2 },{ "STA", &STA, &IZX, 6 },{ "???", &NOP, &IMP, 2 },{ "???", &XXX, &IMP, 6 },{ "STY", &STY, &ZP0, 3 },{ "STA", &STA, &ZP0, 3 },{ "STX", &STX, &ZP0, 3 },{ "???", &XXX, &IMP, 3 },{ "DEY", &DEY, &IMP, 2 },{ "???", &NOP, &IMP, 2 },{ "TXA", &TXA, &IMP, 2 },{ "???", &XXX, &IMP, 2 },{ "STY", &STY, &ABS, 4 },{ "STA", &STA, &ABS, 4 },{ "STX", &STX, &ABS, 4 },{ "???", &XXX, &IMP, 4 },
        { "BCC", &BCC, &REL, 2 },{ "STA", &STA, &IZY, 6 },{ "???", &XXX, &IMP, 2 },{ "???", &XXX, &IMP, 6 },{ "STY", &STY, &ZPX, 4 },{ "STA", &STA, &ZPX, 4 },{ "STX", &STX, &ZPY, 4 },{ "???", &XXX, &IMP, 4 },{ "TYA", &TYA, &IMP, 2 },{ "STA", &STA, &ABY, 5 },{ "TXS", &TXS, &IMP, 2 },{ "???", &XXX, &IMP, 5 },{ "???", &NOP, &IMP, 5 },{ "STA", &STA, &ABX, 5 },{ "???", &XXX, &IMP, 5 },{ "???", &XXX, &IMP, 5 },
        { "LDY", &LDY, &IMM, 2 },{ "LDA", &LDA, &IZX, 6 },{ "LDX", &LDX, &IMM, 2 },{ "???", &XXX, &IMP, 6 },{ "LDY", &LDY, &ZP0, 3 },{ "LDA", &LDA, &ZP0, 3 },{ "LDX", &LDX, &ZP0, 3 },{ "???", &XXX, &IMP, 3 },{ "TAY", &TAY, &IMP, 2 },{ "LDA", &LDA, &IMM, 2 },{ "TAX", &TAX, &IMP, 2 },{ "???", &XXX, &IMP, 2 },{ "LDY", &LDY, &ABS, 4 },{ "LDA", &LDA, &ABS, 4 },{ "LDX", &LDX, &ABS, 4 },{ "???", &XXX, &IMP, 4 },
        { "BCS", &BCS, &REL, 2 },{ "LDA", &LDA, &IZY, 5 },{ "???", &XXX, &IMP, 2 },{ "???", &XXX, &IMP, 5 },{ "LDY", &LDY, &ZPX, 4 },{ "LDA", &LDA, &ZPX, 4 },{ "LDX", &LDX, &ZPY, 4 },{ "???", &XXX, &IMP, 4 },{ "CLV", &CLV, &IMP, 2 },{ "LDA", &LDA, &ABY, 4 },{ "TSX", &TSX, &IMP, 2 },{ "???", &XXX, &IMP, 4 },{ "LDY", &LDY, &ABX, 4 },{ "LDA", &LDA, &ABX, 4 },{ "LDX", &LDX, &ABY, 4 },{ "???", &XXX, &IMP, 4 },
        { "CPY", &CPY, &IMM, 2 },{ "CMP", &CMP, &IZX, 6 },{ "???", &NOP, &IMP, 2 },{ "???", &XXX, &IMP, 8 },{ "CPY", &CPY, &ZP0, 3 },{ "CMP", &CMP, &ZP0, 3 },{ "DEC", &DEC, &ZP0, 5 },{ "???", &XXX, &IMP, 5 },{ "INY", &INY, &IMP, 2 },{ "CMP", &CMP, &IMM, 2 },{ "DEX", &DEX, &IMP, 2 },{ "???", &XXX, &IMP, 2 },{ "CPY", &CPY, &ABS, 4 },{ "CMP", &CMP, &ABS, 4 },{ "DEC", &DEC, &ABS, 6 },{ "???", &XXX, &IMP, 6 },
        { "BNE", &BNE, &REL, 2 },{ "CMP", &CMP, &IZY, 5 },{ "???", &XXX, &IMP, 2 },{ "???", &XXX, &IMP, 8 },{ "???", &NOP, &IMP, 4 },{ "CMP", &CMP, &ZPX, 4 },{ "DEC", &DEC, &ZPX, 6 },{ "???", &XXX, &IMP, 6 },{ "CLD", &CLD, &IMP, 2 },{ "CMP", &CMP, &ABY, 4 },{ "NOP", &NOP, &IMP, 2 },{ "???", &XXX, &IMP, 7 },{ "???", &NOP, &IMP, 4 },{ "CMP", &CMP, &ABX, 4 },{ "DEC", &DEC, &ABX, 7 },{ "???", &XXX, &IMP, 7 },
        { "CPX", &CPX, &IMM, 2 },{ "SBC", &SBC, &IZX, 6 },{ "???", &NOP, &IMP, 2 },{ "???", &XXX, &IMP, 8 },{ "CPX", &CPX, &ZP0, 3 },{ "SBC", &SBC, &ZP0, 3 },{ "INC", &INC, &ZP0, 5 },{ "???", &XXX, &IMP, 5 },{ "INX", &INX, &IMP, 2 },{ "SBC", &SBC, &IMM, 2 },{ "NOP", &NOP, &IMP, 2 },{ "???", &SBC, &IMP, 2 },{ "CPX", &CPX, &ABS, 4 },{ "SBC", &SBC, &ABS, 4 },{ "INC", &INC, &ABS, 6 },{ "???", &XXX, &IMP, 6 },
        { "BEQ", &BEQ, &REL, 2 },{ "SBC", &SBC, &IZY, 5 },{ "???", &XXX, &IMP, 2 },{ "???", &XXX, &IMP, 8 },{ "???", &NOP, &IMP, 4 },{ "SBC", &SBC, &ZPX, 4 },{ "INC", &INC, &ZPX, 6 },{ "???", &XXX, &IMP, 6 },{ "SED", &SED, &IMP, 2 },{ "SBC", &SBC, &ABY, 4 },{ "NOP", &NOP, &IMP, 2 },{ "???", &XXX, &IMP, 7 },{ "???", &NOP, &IMP, 4 },{ "SBC", &SBC, &ABX, 4 },{ "INC", &INC, &ABX, 7 },{ "???", &XXX, &IMP, 7 },
    };



};
}

#endif
