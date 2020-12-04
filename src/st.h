#ifndef SRC_ST_H_
#define SRC_ST_H_


#include <stdint.h>


#define PRESSURE_THRESHOLD 30.0
#define PRESSURE_HYSTERESIS 1.0

void st_init(void);
int32_t st_getThrust(uint8_t i);
void st_setPressurethreshold(uint8_t threshold); // bar
void st_loop(void);


#endif /* SRC_ST_H_ */
