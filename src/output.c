#include "output.h"
#include "gpio.h"
#include <stm32f4xx.h>
#include "systick.h"
#include "power.h"


static gpio_pin_t pin_led_0 = {GPIOB,8};
static gpio_pin_t pin_led_1 = {GPIOB,9};
#ifndef HW_VERSION
	#error No HW_VERSION defined!
#elif ((HW_VERSION == 3) || (HW_VERSION == 4) || (HW_VERSION == 10))
	static gpio_pin_t pin_led_power = {GPIOB,10};
#elif (HW_VERSION == 2)
	static gpio_pin_t pin_led_power = {GPIOE,7};
#else
	#error Defined HW_VERSION is not supported by this firmware!
#endif
static gpio_pin_t pin_speaker_1 = {GPIOE,8};
static gpio_pin_t pin_speaker_2 = {GPIOE,9};

static power_led_mode_t power_led_mode = POWER_LED_MODE_OFF;
static uint16_t power_led_brightness = 0;

static bool speaker_playingSequence = false;
static uint16_t speaker_frequency = 0;


// Gamma brightness lookup table <https://victornpb.github.io/gamma-table-generator>
// gamma = 2.00 steps = 1001 range = 0-1000
const uint16_t gamma_lut[1001] = {
     0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,
     1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   2,   2,
     2,   2,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,
     4,   4,   4,   4,   5,   5,   5,   5,   5,   5,   5,   6,   6,   6,   6,   6,
     6,   7,   7,   7,   7,   7,   7,   8,   8,   8,   8,   8,   8,   9,   9,   9,
     9,   9,  10,  10,  10,  10,  10,  11,  11,  11,  11,  11,  12,  12,  12,  12,
    13,  13,  13,  13,  13,  14,  14,  14,  14,  15,  15,  15,  15,  16,  16,  16,
    16,  17,  17,  17,  17,  18,  18,  18,  18,  19,  19,  19,  20,  20,  20,  20,
    21,  21,  21,  22,  22,  22,  23,  23,  23,  23,  24,  24,  24,  25,  25,  25,
    26,  26,  26,  27,  27,  27,  28,  28,  28,  29,  29,  29,  30,  30,  30,  31,
    31,  31,  32,  32,  32,  33,  33,  33,  34,  34,  35,  35,  35,  36,  36,  36,
    37,  37,  38,  38,  38,  39,  39,  40,  40,  40,  41,  41,  42,  42,  42,  43,
    43,  44,  44,  45,  45,  45,  46,  46,  47,  47,  48,  48,  48,  49,  49,  50,
    50,  51,  51,  52,  52,  52,  53,  53,  54,  54,  55,  55,  56,  56,  57,  57,
    58,  58,  59,  59,  60,  60,  61,  61,  62,  62,  63,  63,  64,  64,  65,  65,
    66,  66,  67,  67,  68,  68,  69,  69,  70,  70,  71,  71,  72,  72,  73,  73,
    74,  75,  75,  76,  76,  77,  77,  78,  78,  79,  80,  80,  81,  81,  82,  82,
    83,  84,  84,  85,  85,  86,  86,  87,  88,  88,  89,  89,  90,  91,  91,  92,
    92,  93,  94,  94,  95,  95,  96,  97,  97,  98,  99,  99, 100, 100, 101, 102,
   102, 103, 104, 104, 105, 106, 106, 107, 108, 108, 109, 110, 110, 111, 112, 112,
   113, 114, 114, 115, 116, 116, 117, 118, 118, 119, 120, 120, 121, 122, 123, 123,
   124, 125, 125, 126, 127, 127, 128, 129, 130, 130, 131, 132, 132, 133, 134, 135,
   135, 136, 137, 138, 138, 139, 140, 141, 141, 142, 143, 144, 144, 145, 146, 147,
   147, 148, 149, 150, 151, 151, 152, 153, 154, 154, 155, 156, 157, 158, 158, 159,
   160, 161, 162, 162, 163, 164, 165, 166, 166, 167, 168, 169, 170, 171, 171, 172,
   173, 174, 175, 176, 176, 177, 178, 179, 180, 181, 181, 182, 183, 184, 185, 186,
   187, 187, 188, 189, 190, 191, 192, 193, 194, 194, 195, 196, 197, 198, 199, 200,
   201, 202, 203, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 213, 214,
   215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 228, 229,
   230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245,
   246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259, 260, 261,
   262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 274, 275, 276, 277, 278,
   279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 291, 292, 293, 294, 295,
   296, 297, 298, 299, 300, 301, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312,
   314, 315, 316, 317, 318, 319, 320, 321, 323, 324, 325, 326, 327, 328, 329, 331,
   332, 333, 334, 335, 336, 338, 339, 340, 341, 342, 343, 345, 346, 347, 348, 349,
   350, 352, 353, 354, 355, 356, 358, 359, 360, 361, 362, 364, 365, 366, 367, 368,
   370, 371, 372, 373, 375, 376, 377, 378, 379, 381, 382, 383, 384, 386, 387, 388,
   389, 391, 392, 393, 394, 396, 397, 398, 399, 401, 402, 403, 404, 406, 407, 408,
   410, 411, 412, 413, 415, 416, 417, 419, 420, 421, 423, 424, 425, 426, 428, 429,
   430, 432, 433, 434, 436, 437, 438, 440, 441, 442, 444, 445, 446, 448, 449, 450,
   452, 453, 454, 456, 457, 458, 460, 461, 462, 464, 465, 466, 468, 469, 471, 472,
   473, 475, 476, 477, 479, 480, 482, 483, 484, 486, 487, 489, 490, 491, 493, 494,
   496, 497, 498, 500, 501, 503, 504, 506, 507, 508, 510, 511, 513, 514, 516, 517,
   518, 520, 521, 523, 524, 526, 527, 529, 530, 531, 533, 534, 536, 537, 539, 540,
   542, 543, 545, 546, 548, 549, 551, 552, 554, 555, 557, 558, 560, 561, 563, 564,
   566, 567, 569, 570, 572, 573, 575, 576, 578, 579, 581, 582, 584, 585, 587, 588,
   590, 591, 593, 594, 596, 598, 599, 601, 602, 604, 605, 607, 608, 610, 612, 613,
   615, 616, 618, 619, 621, 623, 624, 626, 627, 629, 630, 632, 634, 635, 637, 638,
   640, 642, 643, 645, 646, 648, 650, 651, 653, 654, 656, 658, 659, 661, 663, 664,
   666, 667, 669, 671, 672, 674, 676, 677, 679, 681, 682, 684, 686, 687, 689, 691,
   692, 694, 696, 697, 699, 701, 702, 704, 706, 707, 709, 711, 712, 714, 716, 717,
   719, 721, 723, 724, 726, 728, 729, 731, 733, 734, 736, 738, 740, 741, 743, 745,
   746, 748, 750, 752, 753, 755, 757, 759, 760, 762, 764, 766, 767, 769, 771, 773,
   774, 776, 778, 780, 781, 783, 785, 787, 789, 790, 792, 794, 796, 797, 799, 801,
   803, 805, 806, 808, 810, 812, 814, 815, 817, 819, 821, 823, 824, 826, 828, 830,
   832, 834, 835, 837, 839, 841, 843, 845, 846, 848, 850, 852, 854, 856, 857, 859,
   861, 863, 865, 867, 869, 870, 872, 874, 876, 878, 880, 882, 884, 885, 887, 889,
   891, 893, 895, 897, 899, 901, 903, 904, 906, 908, 910, 912, 914, 916, 918, 920,
   922, 924, 925, 927, 929, 931, 933, 935, 937, 939, 941, 943, 945, 947, 949, 951,
   953, 955, 956, 958, 960, 962, 964, 966, 968, 970, 972, 974, 976, 978, 980, 982,
   984, 986, 988, 990, 992, 994, 996, 998,1000,};


void output_init()
{
	gpio_pinCfg(pin_led_0, MODE_OUT|OTYPE_PP|SPEED_LOW, 0);
	gpio_pinCfg(pin_led_1, MODE_OUT|OTYPE_PP|SPEED_LOW, 0);
	gpio_pinCfg(pin_led_power, MODE_OUT|OTYPE_PP|SPEED_LOW, 0);

	gpio_pinCfg(pin_speaker_1, MODE_AF|OTYPE_PP|SPEED_LOW, 1); //speaker pin output, AF1
	gpio_pinCfg(pin_speaker_2, MODE_AF|OTYPE_PP|SPEED_LOW, 1); //speaker pin output, AF1

	//Timer 1 Init (speaker)
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; //enable clock (42MHz)
	TIM1->PSC = 419; //100kHz after prescaler
	TIM1->CCMR1 |= (TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_1); //toggle pin on compare match
	TIM1->CCER |= (TIM_CCER_CC1NE | TIM_CCER_CC1E); //compare 1 outputs enable
	TIM1->BDTR |= TIM_BDTR_MOE; //main output enable

	//Timer 5 Init (Power LED PWM)
	gpio_pinSet(pin_led_power, 0);
	RCC->APB1ENR |= RCC_APB1ENR_TIM5EN; //enable clock (84MHz)
	TIM5->PSC = 83; //prescaler = 84 --> 1MHz
	TIM5->ARR = 999; //auto-reload value = 999 --> 1kHz PWM frequency
	TIM5->CCMR1 |= TIM_CCMR1_OC1PE; //channel 1 preload enable
	TIM5->DIER |= (TIM_DIER_UIE | TIM_DIER_CC1IE); //overflow & ch1 compare interrupts enable
	TIM5->CCR1 = 0; // duty cycle 0
	TIM5->CR1 |= TIM_CR1_CEN; //enable timer
	NVIC_EnableIRQ(TIM5_IRQn); //enable TIM5 global Interrupt
	NVIC_SetPriority(TIM5_IRQn, 32); //interrupt priority
}


void led0(bool state)
{
	gpio_pinSet(pin_led_0, state);
}


void led1(bool state)
{
	gpio_pinSet(pin_led_1, state);
}

void powerLed(uint8_t mode)
{
	power_led_mode = mode;
}


static void speaker(uint16_t frequency)
{
	if((frequency < 50) || (frequency > 15000))
	{
		TIM1->CR1 &= ~TIM_CR1_CEN; //disable timer
	}
	else
	{
		TIM1->CR1 &= ~TIM_CR1_CEN; //disable timer
		TIM1->CNT = 0; //reset counter value
		TIM1->ARR = (uint32_t)(100000/frequency+0.5); //set auto-reload value to control frequency
		TIM1->CR1 |= TIM_CR1_CEN; //enable timer
	}
}

void speaker_setFrequency(uint16_t frequency)
{
	if(power_getEmergencyStop() && frequency) return;
	speaker_frequency = frequency;
	if(!speaker_playingSequence) speaker(speaker_frequency);
}

void speaker_startup()
{
	speaker_playingSequence = true;

	speaker(500);
	systick_busyWait(systick_timeToTicks(0, 0, 0, 200)); //wait 0.2s
	speaker(2000);
	systick_busyWait(systick_timeToTicks(0, 0, 0, 100)); //wait 0.1s
	speaker(0);

	speaker_playingSequence = false;
	speaker(speaker_frequency);
}

void speaker_shutdown()
{
	speaker_playingSequence = true;

	speaker(2000);
	systick_busyWait(systick_timeToTicks(0, 0, 0, 200)); //wait 0.2s
	speaker(500);
	systick_busyWait(systick_timeToTicks(0, 0, 0, 100)); //wait 0.1s
	speaker(0);

	speaker_playingSequence = false;
	speaker(speaker_frequency);
}


void output_update(void)
{
	if(power_getBatteryStatus() == BATTERY_STATUS_LOW)
	{
		if(systick_getUptime() % SPEAKER_BAT_LOW_INTERVAL == 0)
		{
			speaker_playingSequence = true;
			speaker(1000);
		}
		if((systick_getUptime() - 100) % SPEAKER_BAT_LOW_INTERVAL == 0)
			speaker(0);
		if((systick_getUptime() - 200) % SPEAKER_BAT_LOW_INTERVAL == 0)
			speaker(1000);
		if((systick_getUptime() - 300) % SPEAKER_BAT_LOW_INTERVAL == 0)
			speaker(0);
		if((systick_getUptime() - 400) % SPEAKER_BAT_LOW_INTERVAL == 0)
			speaker(1000);
		if((systick_getUptime() - 500) % SPEAKER_BAT_LOW_INTERVAL == 0)
		{
			speaker(0);
			speaker_playingSequence = false;
			speaker(speaker_frequency);
		}

		if(power_led_mode == POWER_LED_MODE_ON) power_led_mode = POWER_LED_MODE_BLINK;
	}
	else if(power_getBatteryStatus() == BATTERY_STATUS_OK)
	{
		if(power_led_mode == POWER_LED_MODE_BLINK) power_led_mode = POWER_LED_MODE_ON;
	}

	switch(power_led_mode)
	{
		case POWER_LED_MODE_OFF:
			power_led_brightness = 0;
			break;
		case POWER_LED_MODE_ON:
			power_led_brightness = 1000;
			break;
		case POWER_LED_MODE_BLINK:
			if(systick_getUptime() % 500 == 0)
				power_led_brightness = 1000;
			if(systick_getUptime() % 1000 == 0)
				power_led_brightness = 0;
			break;
		case POWER_LED_MODE_BOOT:
			if(power_led_brightness > 999) power_led_brightness = 0;
			else power_led_brightness += 1;
			break;
		case POWER_LED_MODE_SHUTDOWN:
			if(power_led_brightness < 1) power_led_brightness = 1000;
			else power_led_brightness -= 1;
			break;
	}
	TIM5->CCR1 = gamma_lut[power_led_brightness];
}


void TIM5_IRQHandler(void)
{
	if(TIM5->SR & TIM_SR_UIF) //counter overflow, start of pwm periods
	{
		TIM5->SR &= ~TIM_SR_UIF; //reset interrupt flag
		if(TIM5->CCR1 > 0) gpio_pinSet(pin_led_power, 1);
	}
	if(TIM5->SR & TIM_SR_CC1IF) //compare 1, end of pwm period
	{
		TIM5->SR &= ~TIM_SR_CC1IF; //reset interrupt flag
		if(TIM5->CCR1 < 1000) gpio_pinSet(pin_led_power, 0);
	}
}

