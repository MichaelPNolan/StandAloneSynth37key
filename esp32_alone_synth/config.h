/*
 * config.h
 *
 * Put all your project settings here (defines, numbers, etc.)
 * configurations which are requiring knowledge of types etc.
 * shall be placed in z_config.ino (will be included at the end)
 *
 *  Created on: 12.05.2021
 *      Author: Marcel Licence
 */

#ifndef CONFIG_H_
#define CONFIG_H_

//#define ESP32_AUDIO_KIT
//#define INTERNAL_DAC 
//defaults to some kind of external DAC probably of the PCM5102A Breakout board variety
#define extraButtons  //(additional buttons to be serviced direct to board)
//#define MIDI_VIA_USB_ENABLED
#define DISPLAY_1306

/* this will force using const velocity for all notes, remove this to get dynamic velocity */
//#define MIDI_USE_CONST_VELOCITY

#ifdef ESP32_AUDIO_KIT

/* on board led */

#define ADC_INPUTS  8
#define ADC_MUL_S0_PIN  23
#define ADC_MUL_S1_PIN  18
#define ADC_MUL_S2_PIN  14
#define ADC_MUL_S3_PIN  5    /* <- not used, this has not been tested */
#define ADC_MUL_SIG_PIN 12

#else /* ESP32_AUDIO_KIT */

/* on board led */
#define LED_PIN     12

/*
 * Define and connect your PINS to DAC here
 */

#ifdef I2S_NODAC
#define I2S_NODAC_OUT_PIN   22  /* noisy sound without DAC, add capacitor in series! */
#endif
#ifdef INTERNAL_DAC
#define I2S_BCLK_PIN    26   //lets assume this is for the PCM5102A on 32 bit mode
#define I2S_WCLK_PIN    25   //word clock is to set which bytes are left channel and right channel could be LRCK
#define I2S_DOUT_PIN    22 
#else 
/*
 * pins to connect a real DAC like PCM5201
 */

#define I2S_BCLK_PIN    25    //purple  bck
#define I2S_WCLK_PIN    27    //purple0    lck
#define I2S_DOUT_PIN    26    //green   din                                    


#endif
// Michael's added simple ADC related parts 
#define  ADC_DIRECT_TL   39 // top left pot directly wired to ESP32 analogue GPIO 35, 34, 39, 36 
#define  ADC_DIRECT_TR   34 //top right
#define  ADC_DIRECT_BL   35 //bottom left
#define  ADC_DIRECT_BR   36 //bottom right

//This code is half relevant but initializing the ADC calls in here is 
//1) Incompatible with the device libraries beyond 1.0.4 after which they changed the ADC calls for ESP on arduino
//2) I'm not using the ADC multiplexer at this standalone device - trying to maximize the efficiency of a single ESP32
#define ADC_INPUTS  8
#define ADC_MUL_S0_PIN  33
#define ADC_MUL_S1_PIN  32
#define ADC_MUL_S2_PIN  13
#define ADC_MUL_SIG_PIN 2

#endif /* ESP32_AUDIO_KIT */

/*
 * You can modify the sample rate as you want
 */

#ifdef ESP32_AUDIO_KIT
#define SAMPLE_RATE 44100
#define SAMPLE_SIZE_16BIT
#else
#define SAMPLE_RATE 48000  //even 32khz - kind of sucks and you get gliches and wrong sustained notes and fails
#define SAMPLE_SIZE_32BIT //previous default 32bit is possible but go with 16bit
//#define SAMPLE_SIZE_8BIT  //default experimental
#endif


#define ADC_TO_MIDI_ENABLED /* this will enable the adc module */
#define ADC_TO_MIDI_LOOKUP_SIZE 8 /* should match ADC_INPUTS */

#endif /* CONFIG_H_ */
