#ifndef ADC_H_
#define ADC_H_


#include <stdint.h>
#include <stdbool.h>


#define ANALOG_COUNT 16

//input voltage in mV when adc-value = 0x0FFF (voltage divider and 1/4 internal divider)

#ifndef HW_VERSION
	#error No HW_VERSION defined!
#elif (HW_VERSION == 10)
	#define MAX_INPUT_VOLTAGE 101200 //v1.0
#elif (HW_VERSION == 4)
	#define MAX_INPUT_VOLTAGE 101200 //v0.4
#elif (HW_VERSION == 3)
	#define MAX_INPUT_VOLTAGE 79200 //v0.3
#elif (HW_VERSION == 2)
	#define MAX_INPUT_VOLTAGE 112200 //v0.2
#else
	#error Defined HW_VERSION is not supported by this firmware!
#endif

#define SMOOTHING_FACTOR 0.3


//init ADC for analog inputs and battery voltage
void adc_init();

//returns 12bit (0-4095) analog input value, input: 0-15
uint16_t adc_getAnalogInput(uint8_t input);

//returns input voltage in mV
uint16_t adc_getInputVoltage_mV();


#endif /* ADC_H_ */
