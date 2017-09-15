#include <Wire.h>
#include <PCF8574.h>
#include "VL53L0XMidiControl.h"
#include <EEPROM.h>

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
const int controlChannelSetSwitchValues[] = {512, 608, 642, 563};
const int controlChannelSetLedPin[3] {4, 7, 13};

const int eepromOffset = 16;

int currentControlChannelSet = 0;
bool lastControlChannelSetSwitchState = true;

PCF8574 pcf;

int WSwitchValues[4] = {180, 412, 563, 642};
int XSwitchValues[4] = {323, 412, 608, 642};
VL53L0XMidiControl controls[controlAmount]
{
    // axisName controlChannel switchPin sensorXShutPin ledPin midi controlChannelAmount controlChannelSetAmount
    VL53L0XMidiControl("W", 0, A3, WSwitchValues, 11, 3,  midi, controlAmount, controlSetAmount),
    VL53L0XMidiControl("X", 1, A3, XSwitchValues, A2, 6,  midi, controlAmount, controlSetAmount),
    VL53L0XMidiControl("Y", 2, 2,                 12, 5,  midi, controlAmount, controlSetAmount),
    VL53L0XMidiControl("Z", 3, 8,                 9,  10, midi, controlAmount, controlSetAmount)
};

int I2CAddresses[controlAmount] {22, 23, 24, 25};

bool assignmentMode = false;

bool previousEnableState[controlAmount];

bool getControlChannelSetSwitchState()
{
    bool controlChannelSetSwitchState = true;
    int controlChannelSetSwitchValue = analogRead(controlChannelSetSwitchPin);

    for (int i = 0; i < (sizeof(controlChannelSetSwitchValues) / sizeof(int)); i++)
    {
        if (isAround(controlChannelSetSwitchValue, controlChannelSetSwitchValues[i]))
        {
            controlChannelSetSwitchState = false;
        }
    }

    return controlChannelSetSwitchState;
}

int midiChannelSelection(int midiChannel)
{
    int buttonAmount = 0;

    if (controlAmount >= 4)
    {
        buttonAmount = 4;
    }
    else if (controlAmount >= 2)
    {
        buttonAmount = 2;
    }
    else
    {
        buttonAmount = 1;
    }

    int buttonWeights[buttonAmount];

    if (buttonAmount == 4)
    {
        buttonWeights[0] = -10;
        buttonWeights[1] = -1;
        buttonWeights[2] = 1;
        buttonWeights[3] = 10;
    }
    else if (buttonAmount == 2)
    {
        buttonWeights[0] = -1;
        buttonWeights[1] = 1;
    }
    else
    {
        buttonWeights[0] = 1;
    }

    for (int i = 0; i < controlAmount; i++) // On eteint les LEDS pour montrer qu'on est rentré en mode selection de canal
    {
        controls[i].forceLedState(false);
    }

    while (!getControlChannelSetSwitchState()) {}; // On attend que le boutton soit relâché

    bool previousSwitchState[buttonAmount];

    while (true)
    {
        if (!getControlChannelSetSwitchState())
        {
            unsigned long buttonPushedMillis = millis();

            while (!getControlChannelSetSwitchState())
            {
                if (millis() - buttonPushedMillis > 3000) // Sortie du mode changement de channel
                {
                    EEPROM.write(0, midiChannel);
                    return midiChannel;
                }
            }
        }

        for (int i = 0; i < controlAmount; i++) // On affiche le canal actuel en binaire
        {
            if (i < 4)
            {
                controls[controlAmount - i - 1].forceLedState(bitRead(midiChannel, i));
            }
        }

        for (int i = 0; i < buttonAmount; i++)
        {
            bool currentSwitchState = controls[i].getSwitchState();

            if (!currentSwitchState && currentSwitchState != previousSwitchState[i])
            {
                midiChannel += buttonWeights[i];

                if (buttonAmount == 1)
                {
                    if (midiChannel < 0)
                    {
                        midiChannel += 16;
                    }
                    else if (midiChannel > 15)
                    {
                        midiChannel -= 16;
                    }
                }
                else
                {
                    midiChannel = constrain(midiChannel, 0, 15);
                }
            }

            previousSwitchState[i] = currentSwitchState;
        }
    }
}

void setup()
{
    Serial.begin(115200);
    Wire.begin();

    pinMode(smoothSampleAmountPotPin, INPUT);
    pinMode(maxDistancePotPin, INPUT);

    for (int count = 0; count < controlAmount; count++)
    {
        controls[count].init(I2CAddresses[count]);

        for(int i = 0; i < controlSetAmount; i++)
        {
        	int eepromAddress = eepromOffset + count * 16 + i;
        	bool enableState = EEPROM.read(eepromAddress) == 0 ? false : true;
        	controls[count].setEnable(i, enableState);
        }

        previousEnableState[count] = controls[count].isEnabled();
    }

    for (int count = 0; count < 3; count++)
    {
        pinMode(controlChannelSetLedPin[count], OUTPUT);
        digitalWrite(controlChannelSetLedPin[count], LOW);
    }

    digitalWrite(controlChannelSetLedPin[0], HIGH);

    pinMode(controlChannelSetSwitchPin, INPUT);

    byte midiChannel = EEPROM.read(0);

    if (midiChannel > 15) // Valeur par défaut de l'eeprom : 255
    {
        midiChannel = 0;
    }

    while (!getControlChannelSetSwitchState())
    {
        if (millis() > 3000) // Mode changement de channel
        {
            midiChannel = midiChannelSelection(midiChannel);

            for (int i = 0; i < controlAmount; i++)
            {
                controls[i].forceLedState(true);
            }

            delay(250);

            for (int i = 0; i < controlAmount; i++)
            {
                controls[i].forceLedState(false);
            }

            delay(250);

            for (int i = 0; i < controlAmount; i++)
            {
                controls[i].forceLedState(true);
            }

            delay(250);

            while (!getControlChannelSetSwitchState()) {}; // On attend que le bouton soit relaché

            break;
        }
    }

    for (int i = 0; i < controlAmount; i++)
    {
        controls[i].setMidiChannel(midiChannel);
    }
}

bool isAround(int value1, int value2)
{
    if (abs(value1 - value2) < 15)
    {
        return true;
    }
    return false;
}

void nextControlChannelSet()
{
    int newControlChannelSet = currentControlChannelSet + 1;

    if (newControlChannelSet >= controlSetAmount)
    {
        newControlChannelSet = 0;
    }

    for (int count = 0; count < controlAmount; count++)
    {
        controls[count].nextControlChannelSet();

        previousEnableState[count] = controls[count].isEnabled();
    }

    for (int count = 0; count < controlSetAmount; count++)
    {
        if (count == newControlChannelSet)
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
    bool controlChannelSetSwitchState = getControlChannelSetSwitchState();

    if (controlChannelSetSwitchState == false && controlChannelSetSwitchState != lastControlChannelSetSwitchState)
    {
        if (!controls[0].getSwitchState() && !controls[controlAmount - 1].getSwitchState())
        {
            unsigned long assignmentModeButtonPushedMillis = millis();
            while (!controls[0].getSwitchState() && !controls[controlAmount - 1].getSwitchState() && !getControlChannelSetSwitchState())
            {
                if (millis() - assignmentModeButtonPushedMillis > 2000)
                {
                    assignmentMode = !assignmentMode;
                    for (int i = 0; i < controlAmount; i++)
                    {
                        controls[i].setAssignmentMode(assignmentMode);
                    }

                    while (!controls[0].getSwitchState() || !controls[controlAmount - 1].getSwitchState() || !getControlChannelSetSwitchState()) {}; // On attend que les boutons soient relachés
                    return;
                }
            }
        }
        else
        {
            nextControlChannelSet();
        }
    }

    lastControlChannelSetSwitchState = controlChannelSetSwitchState;

    for (int count = 0; count < controlAmount; count++)
    {
        int switchAction = controls[count].getSwitchAction();

        if (switchAction == 2 || assignmentMode && switchAction != 0)
        {
            for (int a = 0; a < controlAmount; a++)
            {
                if (a == count)
                {
                    controls[a].setEnable(true);
                }
                else
                {
                    controls[a].setEnable(false);
                }
            }
        }
        else if (switchAction == 1)
        {
            controls[count].toogleEnable();
        }
        else if (switchAction == 3)
        {
            controls[count].toogleAutoReturn();
        }
    }

    for (int count = 0; count < controlAmount; count++)
    {
        if (previousEnableState[count] != controls[count].isEnabled())
        {
            int eepromAddress = eepromOffset + count * 16 + currentControlChannelSet;
            
            if(!midi)
            {
            	Serial.print("EEPROM WRITE ");
            	Serial.print(eepromAddress);
            	Serial.print(" ");
            	Serial.println(controls[count].isEnabled());
            }

            EEPROM.write(eepromAddress, controls[count].isEnabled());
        }

        previousEnableState[count] = controls[count].isEnabled();
    }
}

void loop()
{
    int smoothSampleAmountPotValue = analogRead(smoothSampleAmountPotPin);
    int smoothSampleAmount = map(smoothSampleAmountPotValue, 0, 1023, minSmoothSampleAmount, maxSmoothSampleAmount);

    int maxDistancePotValue = analogRead(maxDistancePotPin);
    int maxDistance = map(maxDistancePotValue, 0, 1023, maxDistanceMin, maxDistanceMax);
    for (int count = 0; count < controlAmount; count++)
    {
        //Serial.println(controls[count].getDistance());

        controls[count].refreshValue(minDistance, maxDistance, smoothSampleAmount);
        checkSwitches();
    }
    
    if(!midi)
    {
      Serial.println();
      delay(100);
    }
}
