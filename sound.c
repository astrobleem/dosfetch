#include <i86.h>
#include <math.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include "sound.h"

// ==========================================
// Background Music Implementation (INT 1C)
// ==========================================

// Global Music State
void (__interrupt __far *old_handler)();
volatile int music_tick_counter = 0;
volatile int current_note_index = 0;
volatile int note_duration_counter = 0;
volatile int music_playing = 0;
volatile int is_active_handler = 0; // Safety flag to prevent "Ghost" handlers

// Simple Music Data (Frequency, Duration in Ticks)
// 0 Frequency = Silence
typedef struct {
    int freq;
    int duration;
} Note;

// A simple tune (Arpeggios)
Note song[] = {
    {261, 4}, {329, 4}, {392, 4}, {523, 8}, // C Major
    {261, 4}, {329, 4}, {392, 4}, {523, 8},
    {293, 4}, {349, 4}, {440, 4}, {587, 8}, // D Minor
    {293, 4}, {349, 4}, {440, 4}, {587, 8},
    {329, 4}, {415, 4}, {493, 4}, {659, 8}, // E Major
    {329, 4}, {415, 4}, {493, 4}, {659, 8},
    {261, 4}, {329, 4}, {392, 4}, {523, 16}, // C Major End
    {0, 0} // End of song
};

void silence_all_voices(void) {
    // Silence all 3 Tone Channels and 1 Noise Channel
    outp(0xC0, 0x9F); // Voice 0 Off
    outp(0xC0, 0xBF); // Voice 1 Off
    outp(0xC0, 0xDF); // Voice 2 Off
    outp(0xC0, 0xFF); // Noise Off
}

void play_tandy_frequency(int freq, int volume) {
    // Tandy SN76496 Sound Chip (Port 0xC0)
    // Frequency = 3579545 / (32 * freq)
    // 10-bit counter value
    
    unsigned int count;
    unsigned char byte1, byte2;
    
    if (freq == 0) {
        // Silence Voice 0
        outp(0xC0, 0x9F); // 1001 1111 (Voice 0, Attenuation 15/Off)
        return;
    }
    
    count = 3579545L / (32L * freq);
    if (count > 1023) count = 1023;
    
    // Byte 1: 1ccf ffff (1, Channel 0, Freq Low 4 bits)
    byte1 = 0x80 | (count & 0x0F);
    // Byte 2: 00ff ffff (0, Freq High 6 bits)
    byte2 = (count >> 4) & 0x3F;
    
    outp(0xC0, byte1);
    outp(0xC0, byte2);
    
    // Set Volume (0=Loudest, 15=Silent)
    // 1001 vvvv (1, Channel 0, Attenuation)
    outp(0xC0, 0x90 | (volume & 0x0F));
}

void __interrupt __far timer_handler(void) {
    // This function is called 18.2 times per second
    
    // SAFETY CHECK: If this handler is a "Ghost" (from a previous crashed run),
    // is_active_handler will likely be 0 (cleared by cleanup) or garbage.
    // We only play if we are the ACTIVE handler.
    if (is_active_handler && music_playing) {
        if (note_duration_counter <= 0) {
            // Load next note
            int freq = song[current_note_index].freq;
            int dur = song[current_note_index].duration;
            
            if (dur == 0) {
                // End of song, loop
                current_note_index = 0;
                freq = song[0].freq;
                dur = song[0].duration;
            }
            
            play_tandy_frequency(freq, 2); // Volume 2 (Pretty loud)
            note_duration_counter = dur;
            current_note_index++;
        } else {
            note_duration_counter--;
        }
    }
    
    // Call the original handler to keep system time ticking
    _chain_intr(old_handler);
}

void init_music(void) {
    if (music_playing) return;
    
    // Silence everything first
    silence_all_voices();
    
    // Save old interrupt vector
    old_handler = _dos_getvect(0x1C);
    
    // Reset state
    current_note_index = 0;
    note_duration_counter = 0;
    music_playing = 1;
    is_active_handler = 1; // Mark this handler as ACTIVE
    
    // Set new interrupt vector
    _dos_setvect(0x1C, timer_handler);
}

void cleanup_music(void) {
    if (!music_playing) return;
    
    // Mark this handler as INACTIVE immediately
    is_active_handler = 0;
    music_playing = 0;
    
    // Restore old interrupt vector
    _dos_setvect(0x1C, old_handler);
    
    // Silence the chip
    silence_all_voices();
}

// ==========================================
// Speech Synthesis (PCM Playback)
// ==========================================

void play_pcm_sample(unsigned char *data, int length, int sample_rate) {
    // Emulate a 4-bit DAC using the Volume Register of Voice 0
    // Data is expected to be 4-bit (0-15)
    
    int i;
    int delay_loops;
    
    // 1. Set Voice 0 to 0Hz (DC Offset) or High Frequency
    // Setting it to a very low frequency (or 0) essentially makes it a DC output
    // controlled by the volume register.
    // 1000 0000 0000 0000 (Voice 0, Freq 0)
    outp(0xC0, 0x80);
    outp(0xC0, 0x00);
    
    // Calculate rough delay loop for sample rate
    // This is highly dependent on CPU speed (cycles per loop)
    // On a fast DOSBox (3000 cycles), a loop is maybe 10-20 cycles?
    // Let's try a heuristic.
    delay_loops = 10000 / sample_rate; 
    if (delay_loops < 1) delay_loops = 1;
    
    // 2. Play Loop
    for (i = 0; i < length; i++) {
        unsigned char sample = data[i] & 0x0F;
        
        // Invert sample because 0=Loudest, 15=Silent in SN76496
        // We want 15=Loudest, 0=Silent
        sample = 15 - sample;
        
        // Write Volume: 1001 vvvv
        outp(0xC0, 0x90 | sample);
        
        // Delay
        // Use a volatile variable to prevent optimization
        {
            volatile int d;
            for (d = 0; d < delay_loops; d++);
        }
    }
    
    // Silence
    outp(0xC0, 0x9F);
}

void generate_robot_voice(unsigned char *buffer, int length) {
    // Generate a synthetic "Robot" sound
    // AM/FM modulated square wave
    int i;
    for (i = 0; i < length; i++) {
        // Carrier: Fast sine/square
        int carrier = (i % 20) < 10 ? 15 : 0;
        
        // Modulator: Slow sine
        double mod = sin((double)i * 0.05); // Slow LFO
        
        // Apply modulation
        int sample = (int)(carrier * ((mod + 1.0) / 2.0));
        
        // Add some noise/glitch
        if ((i % 50) == 0) sample = rand() % 16;
        
        buffer[i] = (unsigned char)(sample & 0x0F);
    }
}
