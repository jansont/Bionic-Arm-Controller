/*
  Author: Abdullatif Hassan <abdullatif.hassan@mail.mcgill.ca>
  Source Repository: https://github.com/Abdul099/Bionic-Arm-Controller
  Last Updated: June 22, 2020

*/

#ifndef Arm_Sampler_h

#define Arm_Sampler_h

#include <Arduino.h>
#include <Arm_Settings.h>

class Arm_Sampler
{
	private: 
		bool _open;
		uint8_t _count;
		int _pin;
		int sampleRate;
		int read();
		int base; 
	public:
		Arm_Sampler();
		Arm_Sampler(int pin);
		void updateBaseline();
		byte evaluateSample(int signal, int threshhigh, int threshlow);
		int simpleSample();
		int simpleSample(byte delay);
		void checkBelow(int val, byte duration);
		int getBaseline();
		void setBaseline(short value);
};

#endif
