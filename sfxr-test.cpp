/*
*   Demo project for my SFXR library.
*   
*/

#include "sfxr.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <filesystem>

int main(int argc, char** argv) {
    SFXR_Sample s;
    if (argc > 1) {
        std::stringstream ss;
        for (int i = 1; i < argc; i++) {
            ss << argv[i] << " ";
        }
        std::string soundPath = ss.str();
        if (!std::filesystem::exists(soundPath)) {
            std::cout << "Invalid file path" << std::endl;
            return EXIT_FAILURE;
        }
        s = *SFXR_LoadSettingsFromFile(soundPath.c_str());
    }
    else {
        s = *SFXR_GenerateRandomSample();
    }
    SFXR_Init();
    
    float mutateAmount = 0;
    std::string mutateInput;
    std::cout << "Enter sound mutation amount (0.05 is pretty good)" << std::endl;
    std::cin >> mutateInput;
    try {
        std::replace(mutateInput.begin(), mutateInput.end(), ',', '.');
        mutateAmount = std::stof(mutateInput);
    } catch (std::exception e) {
        std::cout << "Invalid mutation amount!" << std::endl;
        return EXIT_FAILURE;
    }
    SFXR_SetVolume(0.05f);
    while(true) {
        SFXR_Sample s1 = s;
        SFXR_Mutate(&s1, mutateAmount);
        s1.repeat_speed = 0;
        SFXR_PlaySample(&s1);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return EXIT_SUCCESS;
}