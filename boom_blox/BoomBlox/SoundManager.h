#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <Eigen\Dense>
#include <OgreMath.h>

//#include "Global.h"
#include "fmod.hpp"
#include "fmod_errors.h"

class SoundManager
{
public:
	SoundManager(int userCreatedFrequency = 44100);
	~SoundManager();

	void SetListenerPose(Vector3 pos, float cameraPitch, float cameraHeading);

	void Update();

	void PlayTestSound(Vector3 pos, Vector3 vel);

	void PlayUserCreatedSample(float *data, int length, Vector3 pos, Vector3 vel);
	
	int GetUserCreatedFrequency();
	
	void SoundManager::fileLoader();
    float * stream;

private:
	FMOD::System    *system;
    FMOD::Sound     *sound1, *sound2, *sound3, *usersound;
    FMOD::Channel   *channel1, *channel2, *channel3, *channeluser;
    FMOD_SPEAKERMODE speakermode;
    FMOD_CAPS        caps;

	Vector3 m_listenerPosition;
	float m_listenerPitch;
	float m_listenerHeading;
	
	int m_userCreatedFrequency;

	// vector of sounds;
	std::vector<FMOD::Sound *> *sounds;
	std::vector<FMOD::Channel *> *channels;

    unsigned int     version;
    char             name[256];
    int              key, numdrivers;	
};

#endif
