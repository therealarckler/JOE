#include "VL53L0XMidiControl.h"

VL53L0XMidiControl::VL53L0XMidiControl() {}

VL53L0XMidiControl::VL53L0XMidiControl(String newAxisName, int newControlChannel, int newSwitchPin, int newSensorXShutPin, int newLedPin, bool newMidiEnabled, int newControlChannelAmount, int newControlChannelSetAmount) : axisName(newAxisName), controlChannel(newControlChannel), switchPin(newSwitchPin), sensorXShutPin(newSensorXShutPin), ledPin(newLedPin), midiEnabled(newMidiEnabled), controlChannelAmount(newControlChannelAmount), controlChannelSetAmount(newControlChannelSetAmount)
{
    pinMode(sensorXShutPin, OUTPUT);
    digitalWrite(sensorXShutPin, LOW);

    initSkipArray();
    switchType = 0;
}

VL53L0XMidiControl::VL53L0XMidiControl(String newAxisName, int newControlChannel, int newSwitchPin, int newSwitchValues[4], int newSensorXShutPin, int newLedPin, bool newMidiEnabled, int newControlChannelAmount, int newControlChannelSetAmount) : axisName(newAxisName), controlChannel(newControlChannel), switchPin(newSwitchPin), sensorXShutPin(newSensorXShutPin), ledPin(newLedPin), midiEnabled(newMidiEnabled), controlChannelAmount(newControlChannelAmount), controlChannelSetAmount(newControlChannelSetAmount)
{
    pinMode(sensorXShutPin, OUTPUT);
    digitalWrite(sensorXShutPin, LOW);

    for (int i = 0; i < 4; i++)
    {
        switchValues[i] = newSwitchValues[i];
    }

    initSkipArray();
    switchType = 1;
}

/*
VL53L0XMidiControl::VL53L0XMidiControl(String newAxisName, int newControlChannel, int newSwitchPin, int newSensorXShutPin, int newLedPin, bool newMidiEnabled, int newControlChannelAmount, int newControlChannelSetAmount, PCF8574& newPcf) : axisName(newAxisName), controlChannel(newControlChannel), switchPin(newSwitchPin), sensorXShutPin(newSensorXShutPin), ledPin(newLedPin), midiEnabled(newMidiEnabled), controlChannelAmount(newControlChannelAmount), controlChannelSetAmount(newControlChannelSetAmount)
{
    pinMode(sensorXShutPin, OUTPUT);
    digitalWrite(sensorXShutPin, LOW);

    initSkipArray();
    switchType = 2;

    pcf = &newPcf;
}
*/

void VL53L0XMidiControl::initSkipArray()
{
    if (skip != false)
    {
        delete [] skip;
    }

    skip = new bool [controlChannelSetAmount];

    for (int count = 0; count < controlChannelSetAmount; count++)
    {
        skip[count] = false;
    }
}

int VL53L0XMidiControl::getDistance()
{
    return sensor.readRangeSingleMillimeters();
}

void VL53L0XMidiControl::sendMIDI(int command, int note, int velocity)
{
    Serial.write(command + midiChannel); // Le canal midi est spécifié par les 4 dernier bits
    Serial.write(note);
    Serial.write(velocity);

    // Serial1.write(command);
    // Serial1.write(note);
    // Serial1.write(velocity);
}

void VL53L0XMidiControl::init(int I2CAddress = 0)
{
    startControlChannel = controlChannel;

    if (switchType == 0)
    {
        pinMode(switchPin, INPUT_PULLUP);
    }
    else if (switchType == 1)
    {
        pinMode(switchPin, INPUT);
    }
    /*
    else if (switchType == 2)
    {
        pcf->pinMode(switchPin, INPUT);
    }
    */

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);
    //if(axisName != "X")
    if (true)
    {
        pinMode(sensorXShutPin, INPUT);

        delay(100);

        sensor.init();

        sensor.setTimeout(500);

        sensor.setMeasurementTimingBudget(20000);

        if (I2CAddress != 0)
        {
            sensor.setAddress((uint8_t)I2CAddress);
        }
    }
}

void VL53L0XMidiControl::refreshValue(int minDistance, int maxDistance, int smoothSampleAmount)
{
    shouldSendMidiBool = false;
    long value = getDistance();

	/*
    if (!midiEnabled)
    {
        Serial.print("Valeur brute " + axisName + " : ");
        Serial.print(value);
    }
	*/
	
    /*
    if(abs(value - lastValue) > rejectThreshold && abs(value - lastValidValue) > rejectThreshold)
    {
    lastValue = value;
    return;
    }
    */

    if (value == -1 || value > maxDistance + 200)
    {
        if (autoReturn)
        {
            value = autoReturnValue;
        }
    }
    else if (value == 0)
    {
        if (!lastZeroSkipped)
        {
            lastZeroSkipped = true;
            value = lastValidValue;
        }
    }
    else
    {
        lastZeroSkipped = false;
    }

    value = smooth(value, smoothSampleAmount);

	/*
    if (!midiEnabled)
    {
        Serial.print("\tValeur filtree : ");
        Serial.println(value);
    }
	*/
    int controlValue = 0;

    if (assignmentMode)
    {
        controlValue = 63;
    }
    else
    {
        if (value == -1 || value > maxDistance + 200)
        {
            controlValue = lastControlValue;
        }
        else if (value > maxDistance)
        {
            controlValue = 127;
        }
        else if (value < minDistance)
        {
            controlValue = 0;
        }
        else
        {
            controlValue =  map(value, minDistance, maxDistance, 0, 127);
        }

        lastValidValue = value;
        lastControlValue = controlValue;
    }

    if (!midiEnabled)
    {
        Serial.print("Control " + axisName + " : ");

        if (skip[currentControlChannelSet] || assignmentMode && assignmentSkip)
        {
            Serial.println("SKIPPED");
        }
        else
        {
            Serial.print(controlValue);
            Serial.print("\tCanal ");
            Serial.println(midiChannel);
        }
    }

    if (midiEnabled && (!assignmentMode && !skip[currentControlChannelSet] || (assignmentMode && !assignmentSkip)))
    {
        //sendMIDI(controlChange, controlChannel, controlValue);
        shouldSendMidiBool = true;
    }

    if (!assignmentMode)
    {
        refreshLed();
    }

    lastValue = value;
}

void VL53L0XMidiControl::setEnable(int channelSet, bool enableState)
{
    if (currentControlChannelSet == channelSet)
    {
        setEnable(enableState);
    }
    else
    {
        skip[channelSet] = !enableState;
    }
}

void VL53L0XMidiControl::setEnable(bool enableState)
{
    if (assignmentMode)
    {
        assignmentSkip = !enableState;
        digitalWrite(ledPin, enableState);
    }
    else
    {
        skip[currentControlChannelSet] = !enableState;

        if (skip[currentControlChannelSet])
        {
            currentLedState = false;
            digitalWrite(ledPin, LOW);
        }
        else
        {
            currentLedState = true;
            // On n'allume plus la LED car elle est gérée en pwm lors de l'envoi du MIDI
            //digitalWrite(ledPin, HIGH);
        }
    }


    if (!midiEnabled)
    {
        Serial.print("Module ");
        Serial.print(axisName);
        Serial.print(" ");
        Serial.println(enableState ? "active" : "desactive");
        Serial.println();
    }
}

void VL53L0XMidiControl::toogleEnable()
{
    setEnable(skip[currentControlChannelSet]);
}

bool VL53L0XMidiControl::getSwitchState()
{
    if (switchType == 0)
    {
        if (switchPin == A6 || switchPin == A7)
        {
            int analogValue = analogRead(switchPin);

            if (analogValue < 512)
            {
                return false;
            }
            else
            {
                return true;
            }
        }
        else
        {
            return digitalRead(switchPin);
        }
    }
    else if (switchType == 1)
    {
        int reading = analogRead(switchPin);

        for (int i = 0; i < (sizeof(switchValues) / sizeof(int)); i++)
        {
            if (isAround(reading, switchValues[i]))
            {
                return false;
            }
        }

        return true;
    }
    /*
    else if (switchType == 2)
    {
        return pcf->digitalRead(0);
    }
    */
}

bool VL53L0XMidiControl::isEnabled()
{
    return !skip[currentControlChannelSet];
}

int VL53L0XMidiControl::getSwitchAction()
{
    bool switchState = getSwitchState();

    int returnValue = 0;

    long currentMillis = millis();

    if (!switchState)
    {
        if (lastSwitchState != switchState)
        {
            switchPressedSinceMillis = currentMillis;

            if (currentMillis - lastSwitchSinglePressMillis < 300)
            {
                lastSwitchActionMillis = currentMillis;
                lastSwitchSinglePressMillis = 0;

                if (!midiEnabled)
                {
                    Serial.print("Appui double sur le module ");
                    Serial.println(axisName);
                    Serial.println();
                }

                returnValue = 2;
            }
        }
        else if (currentMillis - switchPressedSinceMillis >= 500)
        {
            if (switchPressedSinceMillis != 0)
            {
                switchPressedSinceMillis = 0;

                if (!midiEnabled)
                {
                    Serial.print("Appui long sur le module ");
                    Serial.println(axisName);
                    Serial.println();
                }

                returnValue = 3;
            }

            lastSwitchActionMillis = currentMillis;
        }
    }
    else
    {
        // Si on vient de lacher le bouton et que la dernière action a été faite il y a au moins 200ms
        if (lastSwitchState != switchState && currentMillis - lastSwitchActionMillis > 200)
        {
            lastSwitchActionMillis = currentMillis;
            switchPressedSinceMillis = 0;
            lastSwitchSinglePressMillis = currentMillis;

            if (!midiEnabled)
            {
                Serial.print("Appui simple sur le module ");
                Serial.println(axisName);
                Serial.println();
            }

            returnValue = 1;
        }
    }

    lastSwitchState = switchState;

    return returnValue;
}

void VL53L0XMidiControl::nextControlChannelSet()
{
    if (assignmentMode)
    {
        assignmentSkip = true;
        digitalWrite(ledPin, LOW);
    }
    else
    {
        if (autoReturn)
        {
            toogleAutoReturn();
        }

        int newControlChannelSet = currentControlChannelSet + 1;

        if (newControlChannelSet >= controlChannelSetAmount)
        {
            newControlChannelSet = 0;
        }

        controlChannel = newControlChannelSet * controlChannelAmount + startControlChannel;

        currentControlChannelSet = newControlChannelSet;

        if (skip[currentControlChannelSet])
        {
            currentLedState = false;
            digitalWrite(ledPin, LOW);
        }
        else
        {
            currentLedState = true;
            refreshLed();
        }
    }
}

void VL53L0XMidiControl::setControlChannel(int newControlChannel)
{
    controlChannel = newControlChannel;
}

void VL53L0XMidiControl::toogleAutoReturn()
{
    if (autoReturn)
    {
        autoReturn = false;
        if (!midiEnabled)
        {
            Serial.print("AutoReturn module ");
            Serial.print(axisName);
            Serial.println(" desactive");
            Serial.println();
        }
    }
    else
    {
        if (skip[currentControlChannelSet])
        {
            return;
        }
        else
        {
            autoReturn = true;
            autoReturnValue = lastValidValue;

            if (!midiEnabled)
            {
                Serial.print("AutoReturn module ");
                Serial.print(axisName);
                Serial.println(" active");
                Serial.println();
            }
        }
    }
}

void VL53L0XMidiControl::setMidiChannel(int newMidiEnabledChannel)
{
    midiChannel = newMidiEnabledChannel;
}

long VL53L0XMidiControl::smooth(long rawValue, int smoothSampleAmount)
{
    //int newValue = (rawValue * (1 - filterValue)) + (smoothedValue  *  filterValue);

    smoothedValue = smoothedValue + ((rawValue - smoothedValue) / smoothSampleAmount);

    return smoothedValue;
}

bool VL53L0XMidiControl::isAround(int value1, int value2)
{
    if (abs(value1 - value2) < 15)
    {
        return true;
    }
    return false;
}

void VL53L0XMidiControl::refreshLed()
{
    if (!skip[currentControlChannelSet])
    {
        if (autoReturn)
        {
            long currentMillis = millis();

            if (currentLedState && currentMillis - lastLedChangeMillis > 500)
            {
                currentLedState = false;
                lastLedChangeMillis = currentMillis;
                digitalWrite(ledPin, LOW);
            }
            else if (!currentLedState && currentMillis - lastLedChangeMillis > 100)
            {
                currentLedState = true;
                lastLedChangeMillis = currentMillis;
                analogWrite(ledPin, map(lastControlValue, 0, 127, 50, 255));
            }
        }
        else
        {
            analogWrite(ledPin, map(lastControlValue, 0, 127, 50, 255));
        }
    }
}

void VL53L0XMidiControl::forceLedState(bool ledState)
{
    digitalWrite(ledPin, ledState);
}

void VL53L0XMidiControl::setAssignmentMode(bool newAssignmentMode)
{
    assignmentMode = newAssignmentMode;
    lastSwitchState = true;

    if (assignmentMode)
    {
        setEnable(false);
    }
    else
    {
        refreshLed();
    }
}

int VL53L0XMidiControl::getControlValue()
{
    return controlValue;
}

int VL53L0XMidiControl::getControlChannel()
{
    return controlChannel;
}

bool VL53L0XMidiControl::shouldSendMidi()
{
    return shouldSendMidiBool;
}
