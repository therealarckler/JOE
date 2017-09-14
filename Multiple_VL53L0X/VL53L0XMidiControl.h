#ifndef VL53L0X_MIDI_CONTROL
#define VL53L0X_MIDI_CONTROL

#include <Arduino.h>
#include <VL53L0X.h>
#include <PCF8574.h>

class VL53L0XMidiControl {

    public:
        VL53L0XMidiControl();
        VL53L0XMidiControl(String newAxisName, int newControlChannel, int newSwitchPin, int newSensorXShutPin, int newLedPin, bool newMidi, int newControlChannelAmount, int newControlChannelSetAmount);
        VL53L0XMidiControl(String newAxisName, int newControlChannel, int newSwitchPin, int newSwitchValues[4], int newSensorXShutPin, int newLedPin, bool newMidi, int newControlChannelAmount, int newControlChannelSetAmount);
        VL53L0XMidiControl(String newAxisName, int newControlChannel, int newSwitchPin, int newSensorXShutPin, int newLedPin, bool newMidi, int newControlChannelAmount, int newControlChannelSetAmount, PCF8574& newPcf);


        void init(int I2CAddress = 0);
        void refreshValue(int minDistance, int maxDistance, int smoothSampleAmount);
        void setEnable(int channelSet, bool enableState);
        void setEnable(bool enableState);
        void toogleEnable();
        void nextControlChannelSet();
        void setControlChannel(int newControlChannel);
        void toogleAutoReturn();
        void setMidiChannel(int newMidiChannel);
        void forceLedState(bool ledState);
        void setAssignmentMode(bool newAssignmentMode);

        bool getSwitchState();
        bool isEnabled();

        int getSwitchAction();
        int getDistance();

    private:
        static const int noteON = 144; //144 = 10010000 en binaire, commande "note on"
        static const int noteOFF = 128; //128 = 10000000 en binaire, commande "note off"
        static const int controlChange = 176; //176 = 10110000 en binaire, commande "controlChange"

        VL53L0X sensor;
        PCF8574* pcf;

        String axisName = "noName";

        int controlChannel;

        int switchType;
        int switchPin;
        int switchValues[4];
        int sensorXShutPin;
        int ledPin;

        int lastValue = 0;
        int lastValidValue = 0;
        int autoReturnValue = 0;

        int lastControlValue = -1;

        int startControlChannel = 0;
        int controlChannelAmount = 0;
        int controlChannelSetAmount = 0;
        int currentControlChannelSet = 0;

        int midiChannel = 0;

        int rejectThreshold = 20;

        float filterValue = 0.5;
        float smoothedValue = 0;

        bool lastSwitchState = true;
        bool midi = false;
        bool autoReturn = false;
        bool currentLedState = true;

        bool* skip = false;

        bool assignmentMode = false;
        bool assignmentSkip = true;

        bool lastZeroSkipped = false;

        long lastSwitchActionMillis = 0;
        long lastSwitchSinglePressMillis = 0;
        long switchPressedSinceMillis = 0;
        long lastLedChangeMillis = 0;

        void initSkipArray();
        void sendMIDI(int command, int note, int velocity);

        long smooth(long rawValue, int smoothSampleAmount);

        bool isAround(int value1, int value2);

        void refreshLed();
};

#endif // VL53L0X_MIDI_CONTROL
