@echo off
g++ sfxr-test.cpp -I include SFXR.dll -o bin/SFXR_test.exe -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic || echo Compilation failed!