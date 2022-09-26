#include <cstdint>
#include <fstream>
#include <chrono>
#include <random>

class Chip8 {

public:
    uint8_t registers[16]{};
    uint8_t memory[4096]{};
    uint16_t index{};
    uint16_t pc{};
    uint16_t stack[16]{};
    uint8_t sp{};
    uint8_t delayTimer{};
    uint8_t soundTimer{};
    uint8_t keypad[16]{};
    uint32_t video[64 * 32]{};
    uint16_t opcode;

    std::default_random_engine randGen;
    std::uniform_int_distribution<uint8_t> randByte;

    const unsigned int START_ADDRESS = 0x200;
    const unsigned int FONTSET_START_ADDRESS = 0x50;
    static const unsigned int FONTSET_SIZE = 80;
    
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



};
