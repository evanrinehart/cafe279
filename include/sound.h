enum SoundCode {
	SND_CALLING,
	SND_WRONG,
	SND_SUCCESS,
	SND_CLOSED,
	SND_DSRADIO,
	SND_GRANTED,
	SND_MURMUR
};

int loadSounds();
void playSound(enum SoundCode snd);
void stopSound(enum SoundCode snd);
