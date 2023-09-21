#include <raylib.h>

#include <sound.h>

static Sound callingSound;
static Sound wrongSound;
static Sound successSound;
static Sound closedSound;
static Sound dsradioSound;
static Sound grantedSound;
static Sound messageSound;

static Sound * soundTable[64];

int loadSounds(){

	callingSound = LoadSound("assets/sounds/calling.mp3");
	wrongSound   = LoadSound("assets/sounds/wrong.mp3");
	successSound = LoadSound("assets/sounds/success.wav");
	closedSound  = LoadSound("assets/sounds/closed.wav");
	dsradioSound = LoadSound("assets/sounds/dsradio.wav");
	grantedSound = LoadSound("assets/sounds/granted.wav");
	messageSound = LoadSound("assets/sounds/message.mp3");

	soundTable[SND_CALLING] = &callingSound;
	soundTable[SND_WRONG]   = &wrongSound;
	soundTable[SND_SUCCESS] = &successSound;
	soundTable[SND_CLOSED]  = &closedSound;
	soundTable[SND_DSRADIO] = &dsradioSound;
	soundTable[SND_GRANTED] = &grantedSound;
	soundTable[SND_MURMUR]  = &messageSound;

	return 0;
}

void playSound(enum SoundCode snd){
	PlaySound(*soundTable[snd]);
}

void stopSound(enum SoundCode snd){
	StopSound(*soundTable[snd]);
}
