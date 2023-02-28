#ifndef _GPIO_H_
#define _GPIO_H_


/* 
 * gpio.h 
 */


extern int gpio_export(unsigned int gpio);
extern int gpio_unexport(unsigned int gpio);
extern int gpio_set_direction(unsigned int gpio, unsigned int out_flag);
extern int gpio_set_value(unsigned int gpio, unsigned int value);
extern int gpio_get_value(unsigned int gpio, unsigned int *value);


#endif    /* _GPIO_H_ */
