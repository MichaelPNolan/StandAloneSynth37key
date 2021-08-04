/*
 * pinout of ESP32 DevKit found here:
 * https://circuits4you.com/2018/12/31/esp32-devkit-esp32-wroom-gpio-pinout/
 *
 * Author: Marcel Licence
 */

#include "config.h"

/*
 * required include files
 */
#include <Arduino.h>
#include <WiFi.h>

/* this is used to add a task to core 0 */
TaskHandle_t  Core0TaskHnd ;
boolean       USBConnected;
uint16_t      task0cycles;

void setup()
{
    /*
     * this code runs once
     */
    delay(500);

    Serial.begin(115200);

    Serial.println();

    Delay_Init();

    Serial.printf("Initialize Synth Module\n");
    Synth_Init();
    Serial.printf("Initialize I2S Module\n");

    // setup_reverb();
    USBConnected = LOW;
    //Blink_Setup();
    
#ifdef ESP32_AUDIO_KIT
    ac101_setup();
    Serial.printf("Use AC101 features of A1S variant");
#endif

    //setupKeyboard();
    setup_i2s();
    Serial.printf("Initialize Midi Module\n");
    Midi_Setup();

    Serial.printf("Turn off Wifi/Bluetooth\n");
#if 0
    setup_wifi();
#else
    WiFi.mode(WIFI_OFF);
#endif

#ifndef ESP8266
    btStop();
    // esp_wifi_deinit();
#endif

#ifdef MIDI_VIA_USB_ENABLED
    UsbMidi_Setup();
#endif



    Serial.printf("ESP.getFreeHeap() %d\n", ESP.getFreeHeap());
    Serial.printf("ESP.getMinFreeHeap() %d\n", ESP.getMinFreeHeap());
    Serial.printf("ESP.getHeapSize() %d\n", ESP.getHeapSize());
    Serial.printf("ESP.getMaxAllocHeap() %d\n", ESP.getMaxAllocHeap());

    Serial.printf("Firmware started successfully\n");


#if 0 /* activate this line to get a tone on startup to test the DAC */
    //Synth_NoteOn(0, 64, 1.0f);
#endif

#if (defined ADC_TO_MIDI_ENABLED) || (defined MIDI_VIA_USB_ENABLED)
    xTaskCreatePinnedToCore(Core0Task, "Core0Task", 8000, NULL, 999, &Core0TaskHnd, 0);
    Serial.println("Setup Pin To Core 0 loop");
#endif
}


void Core0TaskSetup()
{
    /*
     * init your stuff for core0 here
     */
   #ifdef DISPLAY_1306
   setup1306(); //display first before buttons and pots because they call to write to screen
   #endif
   setupKeyboard();
   //setupADC_MINMAX();
   setupButtons();
   Serial.println("Simple Analog");
   
   

   task0cycles = 0;
  
#ifdef ADC_TO_MIDI_ENABLED
    //AdcMul_Init();
#endif
}

void Core0TaskLoop()
{
    /*
     * put your loop stuff for core0 here
     */
  #ifdef ADC_TO_MIDI_ENABLED              
    //AdcMul_Process();
  #endif
   readSimplePots();
   #ifdef DISPLAY_1306
    displayRefresh();
   #endif
  
   processButtons();
   
   serviceKeyboardMatrix();
}

void Core0Task(void *parameter)
{
    
    Core0TaskSetup();
    //Serial.print("Core0 Setup- confirm core:");
    //Serial.println(xPortGetCoreID());
    while (true)
    {
        Core0TaskLoop();
       
        delay(1);
        yield();
    }
}

/*
 * use this if something should happen every second
 * - you can drive a blinking LED for example
 */
inline void Loop_1Hz(void)
{
   //Blink_Process();
   
}


/*
 * our main loop
 * - all is done in a blocking context
 * - do not block the loop otherwise you will get problems with your audio
 */
float fl_sample, fr_sample;

void loop()
{
    static uint32_t loop_cnt_1hz;
    static uint8_t loop_count_u8 = 0;

    loop_count_u8++;

    loop_cnt_1hz ++;
    if (loop_cnt_1hz >= SAMPLE_RATE)
    {
        Loop_1Hz();
        loop_cnt_1hz = 0;
    }

#ifdef I2S_NODAC
    if (writeDAC(l_sample))
    {
        l_sample = Synth_Process();
    }
#else

    if (i2s_write_stereo_samples(&fl_sample, &fr_sample))
    {
        /* nothing for here */
    }
    Synth_Process(&fl_sample, &fr_sample);
    /*
     * process delay line
     */
    Delay_Process(&fl_sample, &fr_sample);

#endif

    /*
     * Midi does not required to be checked after every processed sample
     * - we divide our operation by 8
     */
    if (loop_count_u8 % 24 == 0)
    {
       
        //if(!USBConnected) {UsbMidi_Loop(); Serial.print(".");}
       //Midi_Process();
      #ifdef MIDI_VIA_USB_ENABLED
        UsbMidi_ProcessSync();
      #endif
        
       

        
    }
    if (loop_count_u8 % 100 == 0)
    {
        #ifdef MIDI_VIA_USB_ENABLED
       UsbMidi_Loop();
       #endif
       #ifdef DISPLAY_1306
         //testanimate(logo_bmp, LOGO_WIDTH, LOGO_HEIGHT);
       #endif
    } 
}

    
