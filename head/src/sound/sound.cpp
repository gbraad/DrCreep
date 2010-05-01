#include "stdafx.h"
#include "sound.h"
#include "creep.h"
#include "resid-0.16/sid.h"

cSound *g_Sound;

/* Prototype of our callback function */
void my_audio_callback(void *userdata, Uint8 *stream, int len) {

	for( int i = 0; i < 0x00008000; ++i ) {
		g_Sound->sidGet()->clock();
		int ss = g_Sound->sidGet()->output(8);
		*stream++ = (byte) ss;
	}
}

cSound::cSound( cCreep *pCreep ) {

	mCreep = pCreep;

	mSID = new cSID();
  	//mSID->set_sampling_parameters(14318180.0, SAMPLE_FAST, 4 , -1, 0.97);
	mSID->set_sampling_parameters(1000000, SAMPLE_FAST, 4 , -1, 0.97);
  	mSID->enable_filter(true);
  
  	mSID->reset();
  	// Synchronize the waveform generators (must occur after reset)
  	mSID->write( 4, 0x08);
  	mSID->write(11, 0x08);
  	mSID->write(18, 0x08);
  	mSID->write( 4, 0x00);
  	mSID->write(11, 0x00);
  	mSID->write(18, 0x00);

	devicePrepare();
}

cSound::~cSound() {

	delete mAudioSpec;
}

bool cSound::devicePrepare() {

	g_Sound = this;

	/* Open the audio device */
	SDL_AudioSpec *desired, *obtained;

	/* Allocate a desired SDL_AudioSpec */
	desired = (SDL_AudioSpec*) malloc(sizeof(SDL_AudioSpec));

	/* Allocate space for the obtained SDL_AudioSpec */
	obtained = new SDL_AudioSpec();

	/* 22050Hz - FM Radio quality */
	desired->freq=22050;

	/* 16-bit signed audio */
	desired->format=AUDIO_S16LSB;

	/* Mono */
	desired->channels=0;

	/* Large audio buffer reduces risk of dropouts but increases response time */
	desired->samples=8192;

	/* Our callback function */
	desired->callback=my_audio_callback;

	desired->userdata=NULL;

	/* Open the audio device */
	if ( SDL_OpenAudio(desired, obtained) < 0 ){
	  fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
	  exit(-1);
	}

	mAudioSpec = obtained;

	/* desired spec is no longer needed */
	free(desired);

	SDL_PauseAudio(0);
}