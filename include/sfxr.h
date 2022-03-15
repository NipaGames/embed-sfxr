#pragma once
#include <stdint.h>

#ifdef __cplusplus
    extern "C" {
#endif

#ifdef DLL_BUILD
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT __declspec(dllimport)
#endif

#define SFXR_PICKUP 0
#define SFXR_LASER 1
#define SFXR_EXPLOSION 2
#define SFXR_POWERUP 3
#define SFXR_HIT 4
#define SFXR_JUMP 5
#define SFXR_SELECT 6

struct SFXR_Sample {
	float sound_vol=0.5f;
	bool filter_on;
	int wave_type;

	float base_freq;
	float freq_limit;
	float freq_ramp;
	float freq_dramp;
	float duty;
	float duty_ramp;
	float vib_strength;
	float vib_speed;
	float vib_delay;
	float env_attack;
	float env_sustain;
	float env_decay;
	float env_punch;
	float lpf_resonance;
	float lpf_freq;
	float lpf_ramp;
	float hpf_freq;
	float hpf_ramp;
	float pha_offset;
	float pha_ramp;
	float repeat_speed;
	float arp_speed;
	float arp_mod;
};

DLL_EXPORT void SFXR_Init();
DLL_EXPORT void SFXR_Quit();
DLL_EXPORT void SFXR_Mutate(SFXR_Sample*, float);
DLL_EXPORT SFXR_Sample* SFXR_GenerateRandomSample();
DLL_EXPORT SFXR_Sample* SFXR_GenerateSample(uint8_t);
DLL_EXPORT void SFXR_ResetSample(SFXR_Sample*);
DLL_EXPORT void SFXR_StopPlaying(SFXR_Sample*, bool);
DLL_EXPORT void SFXR_PlaySample(SFXR_Sample*);
DLL_EXPORT void SFXR_SetVolume(float);
DLL_EXPORT SFXR_Sample* SFXR_LoadSettings(char* const);
DLL_EXPORT SFXR_Sample* SFXR_LoadSettingsFromFile(const char*);

#ifdef __cplusplus
    }
#endif