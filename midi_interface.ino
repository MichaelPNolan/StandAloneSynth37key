/*
 * a simple implementation to use midi
 *
 * Author: Marcel Licence
 */

/*
 * look for midi interface using 1N136
 * to convert the MIDI din signal to
 * a uart compatible signal
 */

#ifdef ESP32_AUDIO_KIT
#define RXD2 22 /* U2RRXD */
#else
#define RXD2 16 /* U2RRXD */
//#define TXD2 17
#endif

/* use define to dump midi data */
//#define DUMP_SERIAL2_TO_SERIAL

/*
 * structure is used to build the mapping table
 */
struct midiControllerMapping
{
    uint8_t channel;
    uint8_t data1;
    const char *desc;
    void(*callback_mid)(uint8_t ch, uint8_t data1, uint8_t data2);
    void(*callback_val)(uint8_t userdata, float value);
    uint8_t user_data;
};

struct midiMapping_s
{
    void (*noteOn)(uint8_t ch, uint8_t note, float vel);
    void (*noteOff)(uint8_t ch, uint8_t note);
    void (*pitchBend)(uint8_t ch, float bend);
    void (*modWheel)(uint8_t ch, float value);

    struct midiControllerMapping *controlMapping;
    int mapSize;
};

extern struct midiMapping_s midiMapping; /* definition in z_config.ino */

/* constant to normalize midi value to 0.0 - 1.0f */
#define NORM127MUL	0.007874f

inline void Midi_NoteOn(uint8_t ch, uint8_t note, uint8_t vel)
{
    if (vel > 127)
    {
        /* we will end up here in case of problems with the MIDI connection */
        vel = 127;
        Serial.printf("to loud note detected!!!!!!!!!!!!!!!!!!!!!!!\n");
    }

    if (midiMapping.noteOn != NULL)
    {
        midiMapping.noteOn(ch, note, pow(2, ((vel * NORM127MUL) - 1.0f) * 6));
    }
}

inline void Midi_NoteOff(uint8_t ch, uint8_t note)
{
    if (midiMapping.noteOff != NULL)
    {
        midiMapping.noteOff(ch, note);
    }
}

/*
 * this function will be called when a control change message has been received
 */
inline void Midi_ControlChange(uint8_t channel, uint8_t data1, uint8_t data2)
{
    for (int i = 0; i < midiMapping.mapSize; i++)
    {
        if ((midiMapping.controlMapping[i].channel == channel) && (midiMapping.controlMapping[i].data1 == data1))
        {
            if (midiMapping.controlMapping[i].callback_mid != NULL)
            {
                midiMapping.controlMapping[i].callback_mid(channel, data1, data2);
            }
            if (midiMapping.controlMapping[i].callback_val != NULL)
            {
                midiMapping.controlMapping[i].callback_val(midiMapping.controlMapping[i].user_data, (float)data2 * NORM127MUL);
            }
        }
    }

    if (data1 == 1)
    {
        if (midiMapping.modWheel != NULL)
        {
            midiMapping.modWheel(channel, (float)data2 * NORM127MUL);
        }
    }

#if 0
    if (data1 == 17)
    {
        if (channel < 10)
        {
            Synth_SetSlider(channel,  data2 * NORM127MUL);
        }
    }

    if (data1 == 17)
    {
        if (channel < 10)
        {
            Synth_SetSlider(channel,  data2 * NORM127MUL);
        }
    }
    if ((data1 == 18) && (channel == 1))
    {
        Synth_SetSlider(8,  data2 * NORM127MUL);
    }

    if ((data1 == 16) && (channel < 9))
    {
        Synth_SetRotary(channel, data2 * NORM127MUL);

    }
    if ((data1 == 18) && (channel == 0))
    {
        Synth_SetRotary(8,  data2 * NORM127MUL);
    }
#endif
}

inline void Midi_PitchBend(uint8_t ch, uint16_t bend)
{
    float value = ((float)bend - 8192.0f) * (1.0f / 8192.0f) - 1.0f;
    if (midiMapping.pitchBend != NULL)
    {
        midiMapping.pitchBend(ch, value);
    }
}

/*
 * function will be called when a short message has been received over midi
 */
inline void HandleShortMsg(uint8_t *data)
{
    uint8_t ch = data[0] & 0x0F;

    switch (data[0] & 0xF0)
    {
    /* note on */
    case 0x90:
        if (data[2] > 0)
        {
            Midi_NoteOn(ch, data[1], data[2]);
        }
        else
        {
            Midi_NoteOff(ch, data[1]);
        }
        break;
    /* note off */
    case 0x80:
        Midi_NoteOff(ch, data[1]);
        break;
    case 0xb0:
        Midi_ControlChange(ch, data[1], data[2]);
        break;
    /* pitchbend */
    case 0xe0:
        Midi_PitchBend(ch, ((((uint16_t)data[1]) ) + ((uint16_t)data[2] << 8)));
        break;
    }
}

void Midi_Setup()
{
#ifdef TXD2
    Serial2.begin(31250, SERIAL_8N1, RXD2, TXD2);
#else
    Serial2.begin(31250, SERIAL_8N1, RXD2);
#endif
    pinMode(RXD2, INPUT_PULLUP);  /* 25: GPIO 16, u2_RXD */
}

void Midi_CheckSerial2(void)
{
    /*
     * watchdog to avoid getting stuck by receiving incomplete or wrong data
     */
    static uint32_t inMsgWd = 0;
    static uint8_t inMsg[3];
    static uint8_t inMsgIndex = 0;

    //Choose Serial1 or Serial2 as required

    if (Serial2.available())
    {
        uint8_t incomingByte = Serial2.read();

#ifdef DUMP_SERIAL2_TO_SERIAL
        Serial.printf("%02x", incomingByte);
#endif
        /* ignore live messages */
        if ((incomingByte & 0xF0) == 0xF0)
        {
            return;
        }

        if (inMsgIndex == 0)
        {
            if ((incomingByte & 0x80) != 0x80)
            {
                inMsgIndex = 1;
            }
        }

        inMsg[inMsgIndex] = incomingByte;
        inMsgIndex += 1;

        if (inMsgIndex >= 3)
        {
#ifdef DUMP_SERIAL2_TO_SERIAL
            Serial.printf(">%02x %02x %02x\n", inMsg[0], inMsg[1], inMsg[2]);
#endif
            HandleShortMsg(inMsg);
            inMsgIndex = 0;
        }

        /*
         * reset watchdog to allow new bytes to be received
         */
        inMsgWd = 0;
    }
    else
    {
        if (inMsgIndex > 0)
        {
            inMsgWd++;
            if (inMsgWd == 0xFFF)
            {
                inMsgIndex = 0;
            }
        }
    }
}

void Midi_CheckSerial(void)
{
    /*
     * watchdog to avoid getting stuck by receiving incomplete or wrong data
     */
    static uint32_t inMsgWd = 0;
    static uint8_t inMsg[3];
    static uint8_t inMsgIndex = 0;

    //Choose Serial1 or Serial2 as required

    if (Serial.available())
    {
        uint8_t incomingByte = Serial.read();

        /* ignore live messages */
        if ((incomingByte & 0xF0) == 0xF0)
        {
            return;
        }

        if (inMsgIndex == 0)
        {
            if ((incomingByte & 0x80) != 0x80)
            {
                inMsgIndex = 1;
            }
        }

        inMsg[inMsgIndex] = incomingByte;
        inMsgIndex += 1;

        if (inMsgIndex >= 3)
        {
            HandleShortMsg(inMsg);
            inMsgIndex = 0;
        }

        /*
         * reset watchdog to allow new bytes to be received
         */
        inMsgWd = 0;
    }
    else
    {
        if (inMsgIndex > 0)
        {
            inMsgWd++;
            if (inMsgWd == 0xFFF)
            {
                inMsgIndex = 0;
            }
        }
    }
}

/*
 * this function should be called continuously to ensure that incoming messages can be processed
 */
void Midi_Process()
{
    Midi_CheckSerial2();
#ifdef MIDI_RECV_FROM_SERIAL
    Midi_CheckSerial();
#endif
}


void Midi_SendShortMessage(uint8_t *msg)
{
    Serial2.write(msg, 3);
}
