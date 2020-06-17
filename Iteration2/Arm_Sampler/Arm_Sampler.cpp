/*
  Author: Abdullatif Hassan <abdullatif.hassan@mail.mcgill.ca>
  Source Repository: https://github.com/Abdul099/Bionic-Arm-Controller
  Last Updated:June 16, 2020
*/

#include <Arm_Sampler.h>
#include <Arm_Settings.h>
#include "EMGFilters.h"

//constructor
Arm_Sampler::Arm_Sampler()
{
	_open = 1;
	_count = 0;
	_pin = A0;
	sampleRate = SAMPLE_FREQ_1000HZ;
	humFreq = NOTCH_FREQ_60HZ;
}

Arm_Sampler::Arm_Sampler(int pin)
{
	_open = 1;
	_count = 0;
	_pin = pin;
}

bool Arm_Sampler::registerSample(int threshhigh, int threshlow)
{
	int signal = analogRead(_pin);

	if(_open){
		if(signal>=threshhigh) _count++;
		else _count = 0; //reset counter because the strak is broken
		if(_count>=THRESHOLD_TOLERENCE){ //if enough readings have been registered above threshold 
			_count = 0; //reset the count variable
			_open = 0; //close the arm
			return 0; //close the arm
		}
		else return 1;//remain open
	}

	else{ //if in closed state
		if(signal<threshlow) _count++; // if the signal falls below the threshold, start counting
		else _count = 0;
		if(_count>=THRESHOLD_TOLERENCE2){
			_count = 0;
			_open = 1;//open the hand
			return 1;
		}
		else return 0; //keep closed
	}
}

int Arm_Sampler::rawSample()
{
	read();
	delay(10);
}

void Arm_Sampler::checkBelow(int val, byte duration){
	short counter = duration; 
	while(counter >0){
		int reading = read();
		if (reading<=val) counter--; 
		else counter = duration; //reset the counter
	}
	delay(10);
}

int Arm_Sampler::read()
{
  	int Value = analogRead(_pin);
  	int DataAfterFilter = myFilter.update(Value);
  	int envlope = sq(DataAfterFilter);
  	return envlope;
}

