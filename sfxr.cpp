/*	Embed SFXR
*	- a portable SFXR library -
*	
*	All credit goes to the creator of SFXR, Tomas Pettersson
*	GitHub link to the original file: https://github.com/grimfang4/sfxr/blob/master/sfxr/source/main.cpp
*	
*	I originally made this just to generate varied hit sounds for one of my projects
*	so this is a really barebones version of SFXR with many features including wav-exporting removed.
*
*	Also I had to use, well, some intresting coding practices mainly due to the original code being nearly valid C code 
*	although C++ was used.
*	I didn't bother to rewrite this as proper C++ code, the extensive use of C functions or pointers isn't a deal breaker for me.
*	(this led to me using C functions as well :D)
*	
*	Using this in Unity made some problems as well, marshalling isn't really my thing.
*	I had to return pointers in functions, I couldn't get returning objects to work.
*
*	I'm probably not going to update this for now, it's working well enough for my uses.
*/

#include "sfxr.h"

#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <iostream>

#include "SDL2/SDL.h"

#define rnd(n) (rand()%(n+1))

#define PI 3.14159265f

void error (const char *file, unsigned int line, const char *msg)
{
	fprintf(stderr, "[!] %s:%u  %s\n", file, line, msg);
	exit(1);
}

#define ERROR(x) error(__FILE__, __LINE__, #x)
#define VERIFY(x) do { if (!(x)) ERROR(x); } while (0)

float frnd(float range)
{
	return (float)rnd(10000)/10000*range;
}

float master_vol=0.05f;

void SFXR_SetVolume(float v) {
	master_vol = v;
}

bool playing_sample=false;
int phase;
double fperiod;
double fmaxperiod;
double fslide;
double fdslide;
int period;
float square_duty;
float square_slide;
int env_stage;
int env_time;
int env_length[3];
float env_vol;
float fphase;
float fdphase;
int iphase;
float phaser_buffer[1024];
int ipp;
float noise_buffer[32];
float fltp;
float fltdp;
float fltw;
float fltw_d;
float fltdmp;
float fltphp;
float flthp;
float flthp_d;
float vib_phase;
float vib_speed;
float vib_amp;
int rep_time;
int rep_limit;
int arp_time;
int arp_limit;
double arp_mod;

SFXR_Sample* current_sample;

float* vselected=NULL;
int vcurbutton=-1;

int wav_bits=16;
int wav_freq=44100;

void SFXR_ResetSample(SFXR_Sample* s)
{
	s->wave_type=0;

	s->base_freq=0.3f;
	s->freq_limit=0.0f;
	s->freq_ramp=0.0f;
	s->freq_dramp=0.0f;
	s->duty=0.0f;
	s->duty_ramp=0.0f;

	s->vib_strength=0.0f;
	s->vib_speed=0.0f;
	s->vib_delay=0.0f;

	s->env_attack=0.0f;
	s->env_sustain=0.3f;
	s->env_decay=0.4f;
	s->env_punch=0.0f;

	s->filter_on=false;
	s->lpf_resonance=0.0f;
	s->lpf_freq=1.0f;
	s->lpf_ramp=0.0f;
	s->hpf_freq=0.0f;
	s->hpf_ramp=0.0f;
	
	s->pha_offset=0.0f;
	s->pha_ramp=0.0f;

	s->repeat_speed=0.0f;

	s->arp_speed=0.0f;
	s->arp_mod=0.0f;
}

// Actually not sure what this is meant to do?
// I'm guessing it stops playing by resetting all global variables used in playing samples
// Also renamed this (originally ResetSample)
void SFXR_StopPlaying(SFXR_Sample* s, bool restart)
{
	if(!restart)
		phase=0;
	fperiod=100.0/(s->base_freq*s->base_freq+0.001);
	period=(int)fperiod;
	fmaxperiod=100.0/(s->freq_limit*s->freq_limit+0.001);
	fslide=1.0-pow((double)s->freq_ramp, 3.0)*0.01;
	fdslide=-pow((double)s->freq_dramp, 3.0)*0.000001;
	square_duty=0.5f-s->duty*0.5f;
	square_slide=-s->duty_ramp*0.00005f;
	if(s->arp_mod>=0.0f)
		arp_mod=1.0-pow((double)s->arp_mod, 2.0)*0.9;
	else
		arp_mod=1.0+pow((double)s->arp_mod, 2.0)*10.0;
	arp_time=0;
	arp_limit=(int)(pow(1.0f-s->arp_speed, 2.0f)*20000+32);
	if(s->arp_speed==1.0f)
		arp_limit=0;
	if(!restart)
	{
		// reset filter
		fltp=0.0f;
		fltdp=0.0f;
		fltw=pow(s->lpf_freq, 3.0f)*0.1f;
		fltw_d=1.0f+s->lpf_ramp*0.0001f;
		fltdmp=5.0f/(1.0f+pow(s->lpf_resonance, 2.0f)*20.0f)*(0.01f+fltw);
		if(fltdmp>0.8f) fltdmp=0.8f;
		fltphp=0.0f;
		flthp=pow(s->hpf_freq, 2.0f)*0.1f;
		flthp_d=1.0+s->hpf_ramp*0.0003f;
		// reset vibrato
		vib_phase=0.0f;
		vib_speed=pow(s->vib_speed, 2.0f)*0.01f;
		vib_amp=s->vib_strength*0.5f;
		// reset envelope
		env_vol=0.0f;
		env_stage=0;
		env_time=0;
		env_length[0]=(int)(s->env_attack*s->env_attack*100000.0f);
		env_length[1]=(int)(s->env_sustain*s->env_sustain*100000.0f);
		env_length[2]=(int)(s->env_decay*s->env_decay*100000.0f);

		fphase=pow(s->pha_offset, 2.0f)*1020.0f;
		if(s->pha_offset<0.0f) fphase=-fphase;
		fdphase=pow(s->pha_ramp, 2.0f)*1.0f;
		if(s->pha_ramp<0.0f) fdphase=-fdphase;
		iphase=abs((int)fphase);
		ipp=0;
		for(int i=0;i<1024;i++)
			phaser_buffer[i]=0.0f;

		for(int i=0;i<32;i++)
			noise_buffer[i]=frnd(2.0f)-1.0f;

		rep_time=0;
		rep_limit=(int)(pow(1.0f-s->repeat_speed, 2.0f)*20000+32);
		if(s->repeat_speed==0.0f)
			rep_limit=0;
	}
}

void SFXR_PlaySample(SFXR_Sample* s)
{
	current_sample = s;
	SFXR_StopPlaying(s, false);
	playing_sample=true;
}

void SynthSample(SFXR_Sample* s, int length, float* buffer)
{
	if (s == nullptr) return;
	for(int i=0;i<length;i++)
	{
		if(!playing_sample)
			break;

		rep_time++;
		if(rep_limit!=0 && rep_time>=rep_limit)
		{
			rep_time=0;
			SFXR_StopPlaying(s, true);
		}

		// frequency envelopes/arpeggios
		arp_time++;
		if(arp_limit!=0 && arp_time>=arp_limit)
		{
			arp_limit=0;
			fperiod*=arp_mod;
		}
		fslide+=fdslide;
		fperiod*=fslide;
		if(fperiod>fmaxperiod)
		{
			fperiod=fmaxperiod;
			if(s->freq_limit>0.0f)
				playing_sample=false;
		}
		float rfperiod=fperiod;
		if(vib_amp>0.0f)
		{
			vib_phase+=vib_speed;
			rfperiod=fperiod*(1.0+sin(vib_phase)*vib_amp);
		}
		period=(int)rfperiod;
		if(period<8) period=8;
		square_duty+=square_slide;
		if(square_duty<0.0f) square_duty=0.0f;
		if(square_duty>0.5f) square_duty=0.5f;		
		// volume envelope
		env_time++;
		if(env_time>env_length[env_stage])
		{
			env_time=0;
			env_stage++;
			if(env_stage==3)
				playing_sample=false;
		}
		if(env_stage==0)
			env_vol=(float)env_time/env_length[0];
		if(env_stage==1)
			env_vol=1.0f+pow(1.0f-(float)env_time/env_length[1], 1.0f)*2.0f*s->env_punch;
		if(env_stage==2)
			env_vol=1.0f-(float)env_time/env_length[2];

		// phaser step
		fphase+=fdphase;
		iphase=abs((int)fphase);
		if(iphase>1023) iphase=1023;

		if(flthp_d!=0.0f)
		{
			flthp*=flthp_d;
			if(flthp<0.00001f) flthp=0.00001f;
			if(flthp>0.1f) flthp=0.1f;
		}

		float ssample=0.0f;
		for(int si=0;si<8;si++) // 8x supersampling
		{
			float sample=0.0f;
			phase++;
			if(phase>=period)
			{
//				phase=0;
				phase%=period;
				if(s->wave_type==3)
					for(int i=0;i<32;i++)
						noise_buffer[i]=frnd(2.0f)-1.0f;
			}
			// base waveform
			float fp=(float)phase/period;
			switch(s->wave_type)
			{
			case 0: // square
				if(fp<square_duty)
					sample=0.5f;
				else
					sample=-0.5f;
				break;
			case 1: // sawtooth
				sample=1.0f-fp*2;
				break;
			case 2: // sine
				sample=(float)sin(fp*2*PI);
				break;
			case 3: // noise
				sample=noise_buffer[phase*32/period];
				break;
			}
			// lp filter
			float pp=fltp;
			fltw*=fltw_d;
			if(fltw<0.0f) fltw=0.0f;
			if(fltw>0.1f) fltw=0.1f;
			if(s->lpf_freq!=1.0f)
			{
				fltdp+=(sample-fltp)*fltw;
				fltdp-=fltdp*fltdmp;
			}
			else
			{
				fltp=sample;
				fltdp=0.0f;
			}
			fltp+=fltdp;
			// hp filter
			fltphp+=fltp-pp;
			fltphp-=fltphp*flthp;
			sample=fltphp;
			// phaser
			phaser_buffer[ipp&1023]=sample;
			sample+=phaser_buffer[(ipp-iphase+1024)&1023];
			ipp=(ipp+1)&1023;
			// final accumulation and envelope application
			ssample+=sample*env_vol;
		}
		ssample=ssample/8*master_vol;

		ssample*=2.0f*s->sound_vol;

		if(buffer!=NULL)
		{
			if(ssample>1.0f) ssample=1.0f;
			if(ssample<-1.0f) ssample=-1.0f;
			*buffer++=ssample;
		}
	}
}

// This is basically fread for char arrays
// Copies bytes from ptr to dest and increases ptr by read amount
void LoadParam(void* dest, void*& ptr, size_t size) {
	memcpy(dest, ptr, size);
	ptr = static_cast<char*>(ptr) + size;
}

SFXR_Sample* SFXR_LoadSettings(char* const file) {
	void* ptr = file;
	SFXR_Sample* s = new SFXR_Sample();
	int version = 0;
	LoadParam(&version, ptr, sizeof(int));
	if(version!=100 && version!=101 && version!=102) {
		std::cout << std::hex << version << std::endl;
		ERROR("Invalid file version!");
		return nullptr;
	}

	LoadParam(&s->wave_type, ptr, sizeof(int));

	s->sound_vol = 0.5f;
	if(version == 102)
		LoadParam(&s->sound_vol, ptr, sizeof(float));

	LoadParam(&s->base_freq, ptr, sizeof(float));
	LoadParam(&s->freq_limit, ptr, sizeof(float));
	LoadParam(&s->freq_ramp, ptr, sizeof(float));
	if(version>=101)
		LoadParam(&s->freq_dramp, ptr, sizeof(float));
	LoadParam(&s->duty, ptr, sizeof(float));
	LoadParam(&s->duty_ramp, ptr, sizeof(float));

	LoadParam(&s->vib_strength, ptr, sizeof(float));
	LoadParam(&s->vib_speed, ptr, sizeof(float));
	LoadParam(&s->vib_delay, ptr, sizeof(float));

	LoadParam(&s->env_attack, ptr, sizeof(float));
	LoadParam(&s->env_sustain, ptr, sizeof(float));
	LoadParam(&s->env_decay, ptr, sizeof(float));
	LoadParam(&s->env_punch, ptr, sizeof(float));

	LoadParam(&s->filter_on, ptr, sizeof(bool));
	LoadParam(&s->lpf_resonance, ptr, sizeof(float));
	LoadParam(&s->lpf_freq, ptr, sizeof(float));
	LoadParam(&s->lpf_ramp, ptr, sizeof(float));
	LoadParam(&s->hpf_freq, ptr, sizeof(float));
	LoadParam(&s->hpf_ramp, ptr, sizeof(float));
	
	LoadParam(&s->pha_offset, ptr, sizeof(float));
	LoadParam(&s->pha_ramp, ptr, sizeof(float));

	LoadParam(&s->repeat_speed, ptr, sizeof(float));

	if(version >= 101)
	{
		LoadParam(&s->arp_speed, ptr, sizeof(float));
		LoadParam(&s->arp_mod, ptr, sizeof(float));
	}
	return s;
}

SFXR_Sample* SFXR_LoadSettingsFromFile(const char* filename) {
	FILE* file = fopen(filename, "rb");
	if(!file) {
		return nullptr;
		ERROR("Invalid file location!");
	}
	fseek(file, 0, SEEK_END);
	long fsize = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* contents = (char*) malloc(fsize + 1);
	fread(contents, fsize, 1, file);
	fclose(file);
	SFXR_Sample* s = SFXR_LoadSettings(static_cast<char* const>(contents));
	delete contents;
	return s;
}


bool mute_stream;

//lets use SDL instead
static void SDLAudioCallback(void *userdata, uint8_t *stream, int len)
{
	if (playing_sample && !mute_stream)
	{
		unsigned int l = len/2;
		float* fbuf = new float[l];
		memset(fbuf, 0, sizeof(fbuf));
		SynthSample(current_sample, l, fbuf);
		while (l--)
		{
			float f = fbuf[l];
			if (f < -1.0) f = -1.0;
			if (f > 1.0) f = 1.0;
			((Sint16*)stream)[l] = (Sint16)(f * 32767);
		}
		delete[] fbuf;
	}
	else memset(stream, 0, len);
}

SFXR_Sample* SFXR_GenerateSample(uint8_t i)
{
	SFXR_Sample* s = new SFXR_Sample();
	SFXR_ResetSample(s);
	switch(i)
	{
	case SFXR_PICKUP: // pickup/coin
		s->base_freq=0.4f+frnd(0.5f);
		s->env_attack=0.0f;
		s->env_sustain=frnd(0.1f);
		s->env_decay=0.1f+frnd(0.4f);
		s->env_punch=0.3f+frnd(0.3f);
		if(rnd(1))
		{
			s->arp_speed=0.5f+frnd(0.2f);
			s->arp_mod=0.2f+frnd(0.4f);
		}
		break;
	case SFXR_LASER: // laser/shoot
		s->wave_type=rnd(2);
		if(s->wave_type==2 && rnd(1))
			s->wave_type=rnd(1);
		s->base_freq=0.5f+frnd(0.5f);
		s->freq_limit=s->base_freq-0.2f-frnd(0.6f);
		if(s->freq_limit<0.2f) s->freq_limit=0.2f;
		s->freq_ramp=-0.15f-frnd(0.2f);
		if(rnd(2)==0)
		{
			s->base_freq=0.3f+frnd(0.6f);
			s->freq_limit=frnd(0.1f);
			s->freq_ramp=-0.35f-frnd(0.3f);
		}
		if(rnd(1))
		{
			s->duty=frnd(0.5f);
			s->duty_ramp=frnd(0.2f);
		}
		else
		{
			s->duty=0.4f+frnd(0.5f);
			s->duty_ramp=-frnd(0.7f);
		}
		s->env_attack=0.0f;
		s->env_sustain=0.1f+frnd(0.2f);
		s->env_decay=frnd(0.4f);
		if(rnd(1))
			s->env_punch=frnd(0.3f);
		if(rnd(2)==0)
		{
			s->pha_offset=frnd(0.2f);
			s->pha_ramp=-frnd(0.2f);
		}
		if(rnd(1))
			s->hpf_freq=frnd(0.3f);
		break;
	case SFXR_EXPLOSION: // explosion
		s->wave_type=3;
		if(rnd(1))
		{
			s->base_freq=0.1f+frnd(0.4f);
			s->freq_ramp=-0.1f+frnd(0.4f);
		}
		else
		{
			s->base_freq=0.2f+frnd(0.7f);
			s->freq_ramp=-0.2f-frnd(0.2f);
		}
		s->base_freq*=s->base_freq;
		if(rnd(4)==0)
			s->freq_ramp=0.0f;
		if(rnd(2)==0)
			s->repeat_speed=0.3f+frnd(0.5f);
		s->env_attack=0.0f;
		s->env_sustain=0.1f+frnd(0.3f);
		s->env_decay=frnd(0.5f);
		if(rnd(1)==0)
		{
			s->pha_offset=-0.3f+frnd(0.9f);
			s->pha_ramp=-frnd(0.3f);
		}
		s->env_punch=0.2f+frnd(0.6f);
		if(rnd(1))
		{
			s->vib_strength=frnd(0.7f);
			s->vib_speed=frnd(0.6f);
		}
		if(rnd(2)==0)
		{
			s->arp_speed=0.6f+frnd(0.3f);
			s->arp_mod=0.8f-frnd(1.6f);
		}
		break;
	case SFXR_POWERUP: // powerup
		if(rnd(1))
			s->wave_type=1;
		else
			s->duty=frnd(0.6f);
		if(rnd(1))
		{
			s->base_freq=0.2f+frnd(0.3f);
			s->freq_ramp=0.1f+frnd(0.4f);
			s->repeat_speed=0.4f+frnd(0.4f);
		}
		else
		{
			s->base_freq=0.2f+frnd(0.3f);
			s->freq_ramp=0.05f+frnd(0.2f);
			if(rnd(1))
			{
				s->vib_strength=frnd(0.7f);
				s->vib_speed=frnd(0.6f);
			}
		}
		s->env_attack=0.0f;
		s->env_sustain=frnd(0.4f);
		s->env_decay=0.1f+frnd(0.4f);
		break;
	case SFXR_HIT: // hit/hurt
		s->wave_type=rnd(2);
		if(s->wave_type==2)
			s->wave_type=3;
		if(s->wave_type==0)
			s->duty=frnd(0.6f);
		s->base_freq=0.2f+frnd(0.6f);
		s->freq_ramp=-0.3f-frnd(0.4f);
		s->env_attack=0.0f;
		s->env_sustain=frnd(0.1f);
		s->env_decay=0.1f+frnd(0.2f);
		if(rnd(1))
			s->hpf_freq=frnd(0.3f);
		break;
	case SFXR_JUMP: // jump
		s->wave_type=0;
		s->duty=frnd(0.6f);
		s->base_freq=0.3f+frnd(0.3f);
		s->freq_ramp=0.1f+frnd(0.2f);
		s->env_attack=0.0f;
		s->env_sustain=0.1f+frnd(0.3f);
		s->env_decay=0.1f+frnd(0.2f);
		if(rnd(1))
			s->hpf_freq=frnd(0.3f);
		if(rnd(1))
			s->lpf_freq=1.0f-frnd(0.6f);
		break;
	case SFXR_SELECT: // blip/select
		s->wave_type=rnd(1);
		if(s->wave_type==0)
			s->duty=frnd(0.6f);
		s->base_freq=0.2f+frnd(0.4f);
		s->env_attack=0.0f;
		s->env_sustain=0.1f+frnd(0.1f);
		s->env_decay=frnd(0.2f);
		s->hpf_freq=0.1f;
		break;
	default:
		return nullptr;
	}
	return s;
}

SFXR_Sample* SFXR_GenerateRandomSample() {
	SFXR_Sample* s = new SFXR_Sample();
	SFXR_ResetSample(s);
	s->base_freq=pow(frnd(2.0f)-1.0f, 2.0f);
	if(rnd(1))
		s->base_freq=pow(frnd(2.0f)-1.0f, 3.0f)+0.5f;
	s->freq_limit=0.0f;
	s->freq_ramp=pow(frnd(2.0f)-1.0f, 5.0f);
	if(s->base_freq>0.7f && s->freq_ramp>0.2f)
		s->freq_ramp=-s->freq_ramp;
	if(s->base_freq<0.2f && s->freq_ramp<-0.05f)
		s->freq_ramp=-s->freq_ramp;
	s->freq_dramp=pow(frnd(2.0f)-1.0f, 3.0f);
	s->duty=frnd(2.0f)-1.0f;
	s->duty_ramp=pow(frnd(2.0f)-1.0f, 3.0f);
	s->vib_strength=pow(frnd(2.0f)-1.0f, 3.0f);
	s->vib_speed=frnd(2.0f)-1.0f;
	s->vib_delay=frnd(2.0f)-1.0f;
	s->env_attack=pow(frnd(2.0f)-1.0f, 3.0f);
	s->env_sustain=pow(frnd(2.0f)-1.0f, 2.0f);
	s->env_decay=frnd(2.0f)-1.0f;
	s->env_punch=pow(frnd(0.8f), 2.0f);
	if(s->env_attack+s->env_sustain+s->env_decay<0.2f)
	{
		s->env_sustain+=0.2f+frnd(0.3f);
		s->env_decay+=0.2f+frnd(0.3f);
	}
	s->lpf_resonance=frnd(2.0f)-1.0f;
	s->lpf_freq=1.0f-pow(frnd(1.0f), 3.0f);
	s->lpf_ramp=pow(frnd(2.0f)-1.0f, 3.0f);
	if(s->lpf_freq<0.1f && s->lpf_ramp<-0.05f)
		s->lpf_ramp=-s->lpf_ramp;
	s->hpf_freq=pow(frnd(1.0f), 5.0f);
	s->hpf_ramp=pow(frnd(2.0f)-1.0f, 5.0f);
	s->pha_offset=pow(frnd(2.0f)-1.0f, 3.0f);
	s->pha_ramp=pow(frnd(2.0f)-1.0f, 3.0f);
	s->repeat_speed=frnd(2.0f)-1.0f;
	s->arp_speed=frnd(2.0f)-1.0f;
	s->arp_mod=frnd(2.0f)-1.0f;
	return s;
}

// This could return the new sample but I think this is better :D
void SFXR_Mutate(SFXR_Sample* s, float amount) {
	if(rnd(1)) s->base_freq+=frnd(amount*2)-amount;
	if(rnd(1)) s->freq_ramp+=frnd(amount*2)-amount;
	if(rnd(1)) s->freq_dramp+=frnd(amount*2)-amount;
	if(rnd(1)) s->duty+=frnd(amount*2)-amount;
	if(rnd(1)) s->duty_ramp+=frnd(amount*2)-amount;
	if(rnd(1)) s->vib_strength+=frnd(amount*2)-amount;
	if(rnd(1)) s->vib_speed+=frnd(amount*2)-amount;
	if(rnd(1)) s->vib_delay+=frnd(amount*2)-amount;
	if(rnd(1)) s->env_attack+=frnd(amount*2)-amount;
	if(rnd(1)) s->env_sustain+=frnd(amount*2)-amount;
	if(rnd(1)) s->env_decay+=frnd(amount*2)-amount;
	if(rnd(1)) s->env_punch+=frnd(amount*2)-amount;
	if(rnd(1)) s->lpf_resonance+=frnd(amount*2)-amount;
	if(rnd(1)) s->lpf_freq+=frnd(amount*2)-amount;
	if(rnd(1)) s->lpf_ramp+=frnd(amount*2)-amount;
	if(rnd(1)) s->hpf_freq+=frnd(amount*2)-amount;
	if(rnd(1)) s->hpf_ramp+=frnd(amount*2)-amount;
	if(rnd(1)) s->pha_offset+=frnd(amount*2)-amount;
	if(rnd(1)) s->pha_ramp+=frnd(amount*2)-amount;
	if(rnd(1)) s->repeat_speed+=frnd(amount*2)-amount;
	if(rnd(1)) s->arp_speed+=frnd(amount*2)-amount;
	if(rnd(1)) s->arp_mod+=frnd(amount*2)-amount;
}

void SFXR_Init() {
	std::srand(std::time(nullptr));
	SDL_AudioSpec des;
	des.freq = 44100;
	des.format = AUDIO_S16SYS;
	des.channels = 1;
	des.samples = 512;
	des.callback = SDLAudioCallback;
	des.userdata = NULL;
	VERIFY(!SDL_OpenAudio(&des, NULL));
	SDL_PauseAudio(0);
}

void SFXR_Quit() {
	SDL_Quit();
}