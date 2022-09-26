#include <cstdint>
#include <fstream>

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

    const unsigned int START_ADDRESS = 0x200;


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
};
