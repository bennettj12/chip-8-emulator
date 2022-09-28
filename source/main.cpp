#include "Platform.hpp"
#include "Chip8.hpp"
#include <chrono>
#include <iostream>


int main(int argc, char** argv) {
    if (argc != 4){

		std::cerr << "Usage: " << argv[0] << " <Scale> <Delay> <ROM>\n";
		std::exit(EXIT_FAILURE);

	}

    int videoScale = std::stoi(argv[1]);
	int cycleDelay = std::stoi(argv[2]);
	char const* romFilename = argv[3];

    uint32_t videoColorized[VIDEO_WIDTH * VIDEO_HEIGHT]{};
    uint32_t BLACK_COLOR = 0x33333333;
    uint32_t WHITE_COLOR = 0xEEEEEEEE;

    Platform platform("Chip-8 Emulator", VIDEO_WIDTH * videoScale, VIDEO_HEIGHT * videoScale, VIDEO_WIDTH, VIDEO_HEIGHT);

    Chip8 chip8;
    chip8.LoadROM(romFilename);

    int videoPitch = sizeof(chip8.video[0]) * VIDEO_WIDTH;

    auto lastCycleTime = std::chrono::high_resolution_clock::now();
	bool quit = false;


    while(!quit) 
    {
        quit = platform.ProcessInput(chip8.keypad);

        auto currentTime = std::chrono::high_resolution_clock::now();
		float dt = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastCycleTime).count();

        if (dt > cycleDelay) {
			lastCycleTime = currentTime;
            chip8.Cycle();
            for(unsigned int i = 0; i < (VIDEO_HEIGHT * VIDEO_WIDTH); ++i){
                if(chip8.video[i] == 0xFFFFFFFFu){
                    videoColorized[i] = 0x84b88900u;
                } else {
                    videoColorized[i] = 0x1d442100u;
                }
            }

			platform.Update(videoColorized, videoPitch);
		}


    }
    return 0;


};