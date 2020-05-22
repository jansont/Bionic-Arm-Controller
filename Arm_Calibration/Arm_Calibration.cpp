/*
  Author: Abdullatif Hassan <abdullatif.hassan@mail.mcgill.ca>
  Source Repository: https://github.com/Abdul099/Bionic-Arm-Controller
  Last Updated: May 22, 2020
*/
#include <Arduino.h>
#include <Arm_Calibration.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Arm_Settings.h>
#include <Arm_Screen.h>

//constructors
Arm_Calibration::Arm_Calibration()
{
	_emg_pin = A0;
	_averageMin = 0;
	_peak = 0;
}

Arm_Calibration::Arm_Calibration(int pin)
{
	_emg_pin = pin;
	_averageMin = 0;
	_peak = 0;
}

/*basic calibrate function used for quick calibration
Finds average resting state and average contracting state 
Assigns a threshold at 20% inside the range between both averages
*/
int Arm_Calibration::Calibrate()
{
	Arm_Screen screen = Arm_Screen();
	screen.prepare();
	screen.printToScreen(" Ready to Calibrate");
	delay(500);
	screen.printToScreen("  Prepare  to Relax");
 	delay(1000); 
	screen.printToScreen("  Relax");
   	delay(1000); //wait for a second before we actually start sampling

   	_numberSamples = 0;
   	_averageMin = 0;
  
  	while (_numberSamples < samples) {  ///take samples at 100 HZ ~ 10s
  		delay(10);
     	_amplitude = analogRead(_emg_pin);
	 	printToLaptop(_amplitude);    //print the amplitude to the graph
    	_averageMin += _amplitude;
    	_numberSamples++;
	 }

	 _averageMin /=_numberSamples;
 	screen.printToScreen("Prepare    to      Contract");
   	delay(2000);                
	screen.printToScreen(" Contract   Fully");

  	_numberSamples = 0;
  	_peak = 0;

   	while (_numberSamples < samples) {  //rest for 10 seconds to find the max value
  		delay(10);
    	_amplitude = analogRead(_emg_pin);
		printToLaptop(_amplitude);  //print the amplitude to the graph
     	if(_amplitude >= _peak) _peak = _amplitude;
    	_numberSamples++;
	}

	int threshold = _averageMin + 0.2 * (_peak - _averageMin); //assign threshold at 20% within the range

	screen.printToScreen(" Computing Results");
  	delay(500);
	screen.printToScreen("Min", _averageMin);
	delay(1000);
	screen.printToScreen("MAX:", _peak);
	delay(1000);
	screen.printToScreen("Thresh", threshold);
	delay(2000);
	screen.printToScreen("Done Calibration");
   	return threshold;
}

//Advanced version of calibrate involving resampling to determine a value for the threshold
int Arm_Calibration::CalibrateAdvanced(int* steadyclose)
{
	Arm_Screen screen = Arm_Screen();
	screen.prepare();
	// screen.printToScreen("Prepare   to Relax");
 // 	delay(1000); //wait 1 second for patient to 
	screen.printToScreen("Relax");
   	delay(1000); //wait for a second before we actually start sampling

   	_numberSamples = 0;
   	_averageMin = 0;
  
  	while (_numberSamples < samples) {  ///take samples at 100 HZ ~ 10s
  		delay(10);
     	_amplitude = analogRead(_emg_pin);
	 	printToLaptop(_amplitude);    //print the amplitude to the graph
    	_averageMin += _amplitude;
    	_numberSamples++;
	 }

	 _averageMin /=_numberSamples;
 	// screen.printToScreen("Prepare   to        Contract");
  //  	delay(2000);                
	screen.printToScreen("Contract  Fully");

  	_numberSamples = 0;
  	_peak = 0;

   	while (_numberSamples < samples) {  //rest for 10 seconds to find the max value
  		delay(10);
    	_amplitude = analogRead(_emg_pin);
		printToLaptop(_amplitude);  //print the amplitude to the graph
     	if(_amplitude >= _peak) _peak = _amplitude;
    	_numberSamples++;
	}

	//screen.printToScreen("");
	delay(1500);

	struct candidate{ //create struct for 10 candidate thresholds, of whoch 1 will be chosen at the end
		unsigned int threshVal : 10;
		unsigned int score : 6; //true positive
		unsigned int falsepos : 6; //false positive
	};

	candidate candidates[10];
    uint8_t* trainingData = (uint8_t*) malloc(SIZE_TRAININGDATA*sizeof(uint8_t));
    
	for (int i = 0; i<10; i++){
		candidates[i].threshVal = _averageMin+((i)*(_peak - _averageMin))/10;//in increments of 10% starting at averageMin
		candidates[i].score = 0;//initialize scores with zeros
		candidates[i].falsepos = 0;
	}

	for (int i = 0; i<SIZE_TRAININGDATA; i++){
  		trainingData[i] = 0; //initialize an array of zeros for trainingData
	}

	for(int c = 0; c<NUM_CONTRACTIONS; c++){ // for each contraction
		screen.printToScreen("Contract", c+1);

		for(int i =0; i<SIZE_TRAININGDATA; i++){//fill each datapoint in trainingData
			delay(50);
			_amplitude = analogRead(_emg_pin);
			printToLaptop(_amplitude);
			trainingData[i] = _amplitude/4; //compress the 10 bit ADC reading into an 8bit in order to store it
		}

		for(int i = 0; i<10; i++){//for each candidate
			bool added = 0;
			for(int j = 0; j<SIZE_TRAININGDATA; j++){//for each datapoint in trainingData
				if(!added && (trainingData[j]*4)>candidates[i].threshVal){ //decompress the value from training data and compare it
					candidates[i].score++;
					added = 1;
					// Serial.print(i);
					// Serial.println("added");
				}
			}
		}

		screen.printToScreen("Relax");
		delay(500); //allow for patient to relax

		for(int i =0; i<SIZE_TRAININGDATA; i++){//fill each datapoint in trainingData
			delay(50);
			_amplitude = analogRead(_emg_pin);
			printToLaptop(_amplitude);
			trainingData[i] = _amplitude/4; //compress the 10 bit ADC reading into an 8bit in order to store it
		}

		for(int i = 0; i<10; i++){//for each candidate
			bool added = 0;
			for(int j = 0; j<SIZE_TRAININGDATA; j++){//for each datapoint in trainingData
				if(!added && (trainingData[j]*4)>candidates[i].threshVal){ //decompress the value from training data and compare it
					candidates[i].falsepos++;
					added = 1;
				}
			}
		}
    }

	free(trainingData);
	int selectedIndex = 2; //default value

	for (int i=0; i<10; i++){
		Serial.print(i);
		Serial.print(": ");
		Serial.print("True positive:");
		Serial.print(candidates[i].score);
		Serial.print("  False positive:");
		Serial.println(candidates[i].falsepos);
	}

	for(int i=9; i>0; i--){ 
		if(candidates[i].score>=8 && candidates[i].falsepos<=3){ // aim at capturing 80 percent of contractions with less than 30% false positive
			selectedIndex = i;
			break;
		}
	}

    int threshold = candidates[selectedIndex].threshVal;
    
    if(selectedIndex>=4) *steadyclose = candidates[selectedIndex-4].threshVal;
    else *steadyclose = candidates[1].threshVal;

    screen.printToScreen("Results:");
   	delay(500);
    screen.printToScreen("Min", _averageMin);
    delay(2000);
    screen.printToScreen("Peak", _peak);
    delay(2000);
    screen.printToScreen("Index     Chosen", selectedIndex);
	delay(3000);
	screen.printToScreen("Thresh",threshold);
	delay(3000);

	screen.printToScreen("Done");
   	return threshold;
}

//helper method that allows us to print to the graph of the Arduino Serial Monitor
void Arm_Calibration::printToLaptop(int val)
{
    Serial.print(1000);
   	Serial.print(" ");
   	Serial.print(0);
   	Serial.print(" ");
   	Serial.println(val);
}