#include <Wire.h>
#include <PCF8574.h>
#include "VL53L0XMidiControl.h"

const int smoothSampleAmountPotPin = A1;
const int maxDistancePotPin = A0;

// En mm
const int minDistance = 30;
const int maxDistanceMin = 300;
const int maxDistanceMax = 800;

const int minSmoothSampleAmount = 1;
const int maxSmoothSampleAmount = 20;

const bool midi = false;

const int controlAmount = 4;
const int controlSetAmount = 3;

const int controlChannelSetSwitchPin = A3;
const int controlChannelSetSwitchValues[] = {931, 947, 959, 967};
const int controlChannelSetLedPin[3]{4, 7, 13};

int currentControlChannelSet = 0;
bool lastControlChannelSetSwitchState = true;

PCF8574 pcf;

int WSwitchValues[4] = {700, 893, 947, 967};
int XSwitchValues[4] = {842, 893, 959, 967};
VL53L0XMidiControl controls[controlAmount]
{
  // axisName controlChannel switchPin sensorXShutPin ledPin midi controlChannelAmount controlChannelSetAmount
	VL53L0XMidiControl("W", 0, A3, WSwitchValues, 11, 3,  midi, controlAmount, controlSetAmount),
	VL53L0XMidiControl("X", 1, A3, XSwitchValues, A2, 6,  midi, controlAmount, controlSetAmount),
	VL53L0XMidiControl("Y", 2, 2,                 12, 5,  midi, controlAmount, controlSetAmount),
	VL53L0XMidiControl("Z", 3, 8,                 9,  10, midi, controlAmount, controlSetAmount)
};

int I2CAddresses[controlAmount]{22, 23, 24, 25};

void setup()
{
	Serial.begin(115200);
	Wire.begin();

	pinMode(smoothSampleAmountPotPin, INPUT);
	pinMode(maxDistancePotPin, INPUT);

	for(int count = 0; count < controlAmount; count++)
	{
		controls[count].init(I2CAddresses[count]);
	}

	for(int count = 0; count < 3; count++)
	{
		pinMode(controlChannelSetLedPin[count], OUTPUT);
		digitalWrite(controlChannelSetLedPin[count], LOW);
	}

	digitalWrite(controlChannelSetLedPin[0], HIGH);

	pinMode(controlChannelSetSwitchPin, INPUT);
}

bool isAround(int value1, int value2)
{
	if(abs(value1 - value2) < 5)
	{
		return true;  
	}
	return false;
}

void nextControlChannelSet()
{
	int newControlChannelSet = currentControlChannelSet + 1;

	if(newControlChannelSet >= controlSetAmount)
	{
		newControlChannelSet = 0;
	}

	for(int count = 0; count < controlAmount; count++)
	{
		controls[count].nextControlChannelSet();
	}

	for(int count = 0; count < controlSetAmount; count++)
	{
		if(count == newControlChannelSet)
		{
			digitalWrite(controlChannelSetLedPin[count], HIGH);
		}
		else
		{
			digitalWrite(controlChannelSetLedPin[count], LOW);
		}
	}

	currentControlChannelSet = newControlChannelSet;
}

void checkSwitches()
{
	bool controlChannelSetSwitchState = true;
	int controlChannelSetSwitchValue = analogRead(controlChannelSetSwitchPin);

	for(int i = 0; i < (sizeof(controlChannelSetSwitchValues) / sizeof(int)); i++)
	{
		if(isAround(controlChannelSetSwitchValue, controlChannelSetSwitchValues[i]))
		{
			controlChannelSetSwitchState = false;
		}
	}

	if(controlChannelSetSwitchState == false && controlChannelSetSwitchState != lastControlChannelSetSwitchState)
	{
		nextControlChannelSet();
	}

	lastControlChannelSetSwitchState = controlChannelSetSwitchState;

	for(int count = 0; count < controlAmount; count++)
	{
		int switchAction = controls[count].getSwitchAction();

		if(switchAction == 1)
		{
			controls[count].toogleEnable();
		}
		else if(switchAction == 2)
		{
			for(int a = 0; a < controlAmount; a++)
			{
				if(a == count)
				{
					controls[a].setEnable(true);
				}
				else
				{
					controls[a].setEnable(false);
				}
			}
		}
		else if(switchAction == 3)
		{
			controls[count].toogleAutoReturn();
		}
	}
}

void loop()
{
	int smoothSampleAmountPotValue = analogRead(smoothSampleAmountPotPin);
	int smoothSampleAmount = map(smoothSampleAmountPotValue, 0, 1023, minSmoothSampleAmount, maxSmoothSampleAmount);

	int maxDistancePotValue = analogRead(maxDistancePotPin);
	int maxDistance = map(maxDistancePotValue, 0, 1023, maxDistanceMin, maxDistanceMax);
	for(int count = 0; count < controlAmount; count++)
	{
    //Serial.println(controls[count].getDistance());

		controls[count].refreshValue(minDistance, maxDistance, smoothSampleAmount);
		checkSwitches();
		//delay(5);
	}
}
