#ifndef SOUND_H
#define SOUND_H

// Music Control
void init_music(void);
void cleanup_music(void);

// Sound Chip Control
void silence_all_voices(void);
void play_tandy_frequency(int freq, int volume);

// Speech Synthesis
void play_pcm_sample(unsigned char *data, int length, int sample_rate);
void generate_robot_voice(unsigned char *buffer, int length);

#endif
