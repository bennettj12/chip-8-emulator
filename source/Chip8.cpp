#include "Chip8.hpp"
#include <cstdint>
#include <fstream>
#include <chrono>
#include <random>
#include <cstring>

    const unsigned int START_ADDRESS = 0x200;
    const unsigned int FONTSET_START_ADDRESS = 0x50;
    const unsigned int FONTSET_SIZE = 80;
    
    uint8_t fontset[FONTSET_SIZE] = 
    {
        // font characters represented as 16x5 sprites
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        /*  example:
            11110000
            10010000
            10010000
            10010000
            11110000
        */
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };



    Chip8::Chip8() 
    //seed random using system clock
    : randGen(std::chrono::system_clock::now().time_since_epoch().count()) 
    {
        // pc begins outside of reserved memory
        pc = START_ADDRESS;

        // load font into memory
        for(unsigned int i = 0; i < FONTSET_SIZE; ++i) {
            memory[FONTSET_START_ADDRESS + i] = fontset[i];
        }

        // initialize random
        randByte = std::uniform_int_distribution<uint8_t>(0,255);

        // function pointer table

        /*
            The master table is looking at the 4 leftmost bits in the opcode since that determines most of the opcodes.
            For instructions with multiple options for the respective leftmost hex, 
            a secondary table looks at the rightmost 4 or 8 bits to determine correct opcode.
        */
		table[0x0] = &Chip8::Table0;
		table[0x1] = &Chip8::OP_1nnn;
		table[0x2] = &Chip8::OP_2nnn;
		table[0x3] = &Chip8::OP_3xkk;
		table[0x4] = &Chip8::OP_4xkk;
		table[0x5] = &Chip8::OP_5xy0;
		table[0x6] = &Chip8::OP_6xkk;
		table[0x7] = &Chip8::OP_7xkk;
		table[0x8] = &Chip8::Table8;
		table[0x9] = &Chip8::OP_9xy0;
		table[0xA] = &Chip8::OP_Annn;
		table[0xB] = &Chip8::OP_Bnnn;
		table[0xC] = &Chip8::OP_Cxkk;
		table[0xD] = &Chip8::OP_Dxyn;
		table[0xE] = &Chip8::TableE;
		table[0xF] = &Chip8::TableF;

		for (size_t i = 0; i <= 0xE; i++)
		{
			table0[i] = &Chip8::OP_NULL;
			table8[i] = &Chip8::OP_NULL;
			tableE[i] = &Chip8::OP_NULL;
		}

		table0[0x0] = &Chip8::OP_00E0;
		table0[0xE] = &Chip8::OP_00EE;

		table8[0x0] = &Chip8::OP_8xy0;
		table8[0x1] = &Chip8::OP_8xy1;
		table8[0x2] = &Chip8::OP_8xy2;
		table8[0x3] = &Chip8::OP_8xy3;
		table8[0x4] = &Chip8::OP_8xy4;
		table8[0x5] = &Chip8::OP_8xy5;
		table8[0x6] = &Chip8::OP_8xy6;
		table8[0x7] = &Chip8::OP_8xy7;
		table8[0xE] = &Chip8::OP_8xyE;

		tableE[0x1] = &Chip8::OP_ExA1;
		tableE[0xE] = &Chip8::OP_Ex9E;

		for (size_t i = 0; i <= 0x65; i++)
		{
			tableF[i] = &Chip8::OP_NULL;
		}

		tableF[0x07] = &Chip8::OP_Fx07;
		tableF[0x0A] = &Chip8::OP_Fx0A;
		tableF[0x15] = &Chip8::OP_Fx15;
		tableF[0x18] = &Chip8::OP_Fx18;
		tableF[0x1E] = &Chip8::OP_Fx1E;
		tableF[0x29] = &Chip8::OP_Fx29;
		tableF[0x33] = &Chip8::OP_Fx33;
		tableF[0x55] = &Chip8::OP_Fx55;
		tableF[0x65] = &Chip8::OP_Fx65;

    }
    void Chip8::Table0(){
		((*this).*(table0[opcode & 0x000Fu]))();
	}
	void Chip8::Table8(){
		((*this).*(table8[opcode & 0x000Fu]))();
	}
	void Chip8::TableE(){
		((*this).*(tableE[opcode & 0x000Fu]))();
	}
	void Chip8::TableF(){
		((*this).*(tableF[opcode & 0x00FFu]))();
	}
	void Chip8::OP_NULL(){}

    void Chip8::Cycle()
    {
        // fetch decode execute cycle

        // fetch current opcode (bitshift 8 to set leftmost 8 bits to current opcode), rightmost 8 bits to next
        opcode = (memory[pc] << 8u | memory[pc + 1]);

        pc+=2;
        // decode + execute
        // call function at first digit table
        ((*this).*(table[(opcode & 0xF000u) >> 12u]))();

        // decrement delay if set
        if(delayTimer > 0){
            --delayTimer;
        }

        // decrement sound if set
        if(soundTimer > 0) {
            --soundTimer;
        }

    }


    void Chip8::LoadROM(char const* filename) {

        // open file in binary mode, output position at end of file
        std::ifstream file(filename, std::ios::binary | std::ios::ate);

        if(file.is_open()) {

            // get filesize by reading position at eof
            std::streampos size = file.tellg();
            // buffer of file size
            char* buffer = new char[size];

            // return to start and read into buffer
            file.seekg(0, std::ios::beg);
            file.read(buffer,size);
            file.close();

            // load file into memory
            for(long i = 0; i < size; ++i) {

                memory[START_ADDRESS + i] = buffer[i];

            }
        }

    }

    // INSTRUCTIONS:
    void Chip8::OP_00E0() {
        // cls | clear screen
        memset(video, 0, sizeof(video));
    }

    void Chip8::OP_00EE() {
        // ret | return
        --sp;
        pc = stack[sp];
        
    }

    void Chip8::OP_1nnn() {
        // jmp | jump to location
        uint16_t address = opcode & 0x0FFFu;
        pc = address;
    }

    void Chip8::OP_2nnn() {
        // call subroutine at nnn
        uint16_t address = opcode & 0x0FFFu;

        stack[sp] = pc;
        ++sp;
        pc = address;
        
    }

    void Chip8::OP_3xkk() {
        // skip next instruction if contents of register x (0N00) == the given byte (00NN)
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t byte = opcode & 0x00FFu;

        if(registers[Vx] = byte){
            pc += 2;
        }
    }
    void Chip8::OP_4xkk() {
        // skip next instruction if contents of register x (0N00) != the given byte (00NN)
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t byte = opcode & 0x00FFu;

        if(registers[Vx] != byte){
            pc += 2;
        }
    }
    void Chip8::OP_5xy0() {
        // skip next if two registers are equal
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;

        if (registers[Vx] == registers[Vy])
        {
            pc += 2;
        }
    }

    void Chip8::OP_6xkk() {
        // set a register to a given byte
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t byte = (opcode & 0x00FFu);
        registers[Vx] = byte;
    }

    void Chip8::OP_7xkk() {
        // add byte to register, than store in register
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t byte = (opcode & 0x00FFu);
        registers[Vx] += byte;
    }
    void Chip8::OP_8xy0() {
        // set register Vx to value of register Vy
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;
        registers[Vx] = registers[Vy];
    }
    void Chip8::OP_8xy1() {
        // set register Vx to bitwise OR of register Vy
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;
        registers[Vx] |= registers[Vy];
    }
    void Chip8::OP_8xy2() {
        // set register Vx to bitwise AND of register Vy
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;
        registers[Vx] &= registers[Vy];
    }
    void Chip8::OP_8xy3() {
        // set register Vx to bitwise XOR of register Vy
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;
        registers[Vx] ^= registers[Vy];
    }
    void Chip8::OP_8xy4() {
        // set register Vx to reg Vx + reg Vy, store the carry in VF
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;
        
        uint16_t sum = registers[Vx] + registers[Vy];

        registers[0xF] = (sum > 255u) ? 1 : 0;

        registers[Vx] = sum & 0xFFu;
    }
    void Chip8::OP_8xy5() {
        // set register Vx to reg Vx - reg Vy, set flag to 1 if Vx > Vy
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;

        registers[0xF] = (registers[Vx] > registers[Vy]) ? 1 : 0;

        registers[Vx] -= registers[Vy];

    }

    void Chip8::OP_8xy6() {
        // set flag to 1 if least-significatn bit of Vx is 1, otherwise set flag to zero.
        // shift right vx.

        uint8_t Vx = (opcode & 0x0F00u) >> 8u;

        registers[0xF] = (registers[Vx] & 0x1u);

        registers[Vx] >>= 1;

    }

    void Chip8::OP_8xy7() {
        // set Vx to (Vy - Vx), set VF to 1 if Vy > Vx
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;

        registers[0xF] = (registers[Vy] > registers[Vx]) ? 1 : 0;
        registers[Vx] = registers[Vy] - registers[Vx];
    }
    void Chip8::OP_8xyE() {
        // shift left, set flag to most significant bit
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;

        registers[0xF] = (registers[Vx] & 0x80u) >> 7u;
        registers[Vx] <<= 1;
    }
    void Chip8::OP_9xy0() {
        //skip next instruction if register Vx != Vy
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;
        if(registers[Vx] != registers[Vy]){
            pc += 2;
        }
    }
    void Chip8::OP_Annn() {
        // set index to nnn
        uint16_t address = (opcode & 0x0FFFu);
        index = address;
    }
    void Chip8::OP_Bnnn() {
        // jump to location at V0 + nnn;
        uint16_t address = (opcode & 0x0FFFu);

        pc = address + registers[0];
    }
    void Chip8::OP_Cxkk() {
        // set register Vx to a random byte AND a given byte
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	    uint8_t byte = opcode & 0x00FFu;

        registers[Vx] = randByte(randGen) & byte;
    }
    void Chip8::OP_Dxyn() {
        // draw n-byte sprite starting at memory location index at (Vx,Vy), set VF = collision
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t Vy = (opcode & 0x00F0u) >> 4u;
        uint8_t height = opcode & 0x000Fu;

        uint8_t xPos = registers[Vx] % VIDEO_WIDTH;
        uint8_t yPos = registers[Vx] % VIDEO_HEIGHT;

        registers[0xF] = 0;

        for(unsigned int row = 0; row < height; ++row) {
            uint8_t spriteByte = memory[index + row];

            for(unsigned int col = 0; col < 8; ++col){

                uint8_t spritePixel = spriteByte & (0x80u >> col);
                uint32_t* screenPixel = &video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];

                if(spritePixel){
                    // when the sprite pixel should be on, check if that pixel is already turned on in video.
                    if (*screenPixel == 0xFFFFFFFF){
                        // pixel is already on, collision flag set
                        registers[0xF] = 1;
                    }
                    // set the pixel to the XOR of itself, effectively toggling it
                    *screenPixel ^= 0xFFFFFFFF;
                }

            }

        }
    }

    void Chip8::OP_Ex9E() {
        // skip next instruction if key value Vx is pressed.

        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	    uint8_t key = registers[Vx];

        if(keypad[key]) {
            pc+= 2;
        }

    }

    void Chip8::OP_ExA1() {
        // skip next instruction if key Vx is not pressed
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	    uint8_t key = registers[Vx];
        if(!keypad[key]) {
            pc+= 2;
        }
    }

    void Chip8::OP_Fx07() {
        // set Vx to the current delay timer value
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;

        registers[Vx] = delayTimer;

    }

    void Chip8::OP_Fx0A() {
        // wait for a key press, store the value of the key in vx
	    uint8_t Vx = (opcode & 0x0F00u) >> 8u;

        for(unsigned int i = 0; i < 16; i++) {
            if(keypad[i]){
                registers[Vx] = i;
                return;
            }
        }
        pc -= 2;
    }

    void Chip8::OP_Fx15() {
        // set delay timer to Vx
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;

        delayTimer = registers[Vx];
    }

    void Chip8::OP_Fx18() {
        // set sound timer to Vx
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;

        soundTimer = registers[Vx];
    }

    void Chip8::OP_Fx1E() {
        // Increase Index by Vx
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        index += registers[Vx];

    }

    void Chip8::OP_Fx29() {
        //Sets the index to the beginning of character requested in vx

        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t digit = registers[Vx];

        index = FONTSET_START_ADDRESS + (5 * digit);
    }

    void Chip8::OP_Fx33() {
        // Stores BCD (Binary Coded Decimal) representation of Vx at i, i+1, i+2
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        uint8_t number = registers[Vx];
        
        memory[index + 2] = number % 10;
        number /= 10;

        memory[index + 1] = number % 10;
        number /= 10;

        memory[index] = number % 10;
    }

    void Chip8::OP_Fx55() {
        // store registers v0 to vx into memory starting at index.
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        for(unsigned int i = 0; i <= Vx; ++i) {
            memory[index + i] = registers[i];
        }

    }

    void Chip8::OP_Fx65() {
        // reads memory into registers v0 to vx starting at index.
        uint8_t Vx = (opcode & 0x0F00u) >> 8u;
        for(unsigned int i = 0; i <= Vx; ++i) {
            registers[i] = memory[index + i];
        }
    }



