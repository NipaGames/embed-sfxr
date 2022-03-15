// C# wrapper for embed SFXR

// marshalling is pain

using System;
using System.Runtime.InteropServices;

public class SFXR {
    public enum SoundType : int {
        PICKUP = 0,
        LASER = 1,
        EXPLOSION = 2,
        POWERUP = 3,
        HIT = 4,
        JUMP = 5,
        SELECT = 6
    }
    [StructLayout(LayoutKind.Sequential, Size = 1)]
    public class Sample {
        public float soundVol;
        public byte filterOn;
        public int waveType;

        public float baseFreq;
        public float freqLimit;
        public float freqRamp;
        public float freqDramp;
        public float duty;
        public float dutyRamp;
        public float vibStrength;
        public float vibSpeed;
        public float vibDelay;
        public float envAttack;
        public float envSustain;
        public float envDecay;
        public float envPunch;
        public float lpfResonance;
        public float lpfFreq;
        public float lpfRamp;
        public float hpfFreq;
        public float hpfRamp;
        public float phaOffset;
        public float phaRamp;
        public float repeatSpeed;
        public float arpSpeed;
        public float arpMod;
    }

    [DllImport("SFXR.dll", EntryPoint = "SFXR_Init")]
    public static extern void Init();

    [DllImport("SFXR.dll", EntryPoint = "SFXR_Quit")]
    public static extern void Quit();

    [DllImport("SFXR.dll", EntryPoint = "SFXR_Mutate")]
    public static extern void Mutate(ref Sample s, float f);

    [DllImport("SFXR.dll", EntryPoint = "SFXR_GenerateRandomSample")]
    public static extern IntPtr GenerateRandomSamplePtr();
    public static Sample GenerateRandomSample() { return Marshal.PtrToStructure<Sample>(GenerateRandomSamplePtr()); }

    [DllImport("SFXR.dll", EntryPoint = "SFXR_GenerateSample")]
    public static extern IntPtr GenerateSamplePtr(int i);
    public static Sample GenerateSample(int i) { return Marshal.PtrToStructure<Sample>(GenerateSamplePtr(i)); }
    
    [DllImport("SFXR.dll", EntryPoint = "SFXR_PlaySample")]
    public static extern void PlaySample(Sample s);

    [DllImport("SFXR.dll", EntryPoint = "SFXR_ResetSample")]
    public static extern void ResetSample(ref Sample s);

    [DllImport("SFXR.dll", EntryPoint = "SFXR_StopPlaying")]
    [Obsolete]
    public static extern void StopPlaying(ref Sample s, bool b);

    [DllImport("SFXR.dll", EntryPoint = "SFXR_SetVolume")]
    public static extern void SetVolume(float v);

    [DllImport("SFXR.dll", EntryPoint = "SFXR_LoadSettings")]
    public static extern IntPtr LoadSettingsPtr([MarshalAs(UnmanagedType.LPStr)] byte[] b);
    public static Sample LoadSettings(byte[] b) { return Marshal.PtrToStructure<Sample>(LoadSettingsPtr(b)); }

    [DllImport("SFXR.dll", EntryPoint = "SFXR_LoadSettingsFromFile")]
    public static extern IntPtr LoadSettingsFromFilePtr([MarshalAs(UnmanagedType.LPStr)] string s);
    public static Sample LoadSettingsFromFile(string s) { return Marshal.PtrToStructure<Sample>(LoadSettingsFromFilePtr(s)); }
}
