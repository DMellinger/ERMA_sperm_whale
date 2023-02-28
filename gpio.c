/* 
 * gpio.c - control General Purpose Input-Output pins on the RPi
 * from Chris Jones, EOS
 *
 * To use this:
 * 1) Call gpio_export with the pin number you wish to use
 * 2) Call gpio_set_dir
 * 3) Call gpio_set_value or gpio_get_value
 * 4) Call gpio_unexport (not sure if this is required).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "gpio.h"

static char gpio_buffer[48];

/*
* gpio_export
* This makes a GPIO pin accessible for use.
*/
int gpio_export(unsigned int pin_number)
{
  int fd, len;
  char buf[3];
 
  fd = open("/sys/class/gpio/export", O_WRONLY);
  if (fd < 0)
  {
    perror("gpio/export");
    return fd;
  }
 
  len = snprintf(buf, sizeof(buf), "%d", pin_number);
  write(fd, buf, len);
  close(fd);
 
  return 0;
}

/*
 * gpio_unexport
 * This reverses the effects of gpio_export.
 */
int gpio_unexport(unsigned int pin_number)
{
  int fd, len;
  char buf[3];
 
  fd = open("/sys/class/gpio/unexport", O_WRONLY);
  if (fd < 0)
  {
    perror("gpio/unexport");
    return fd;
  }
  len = snprintf(buf, sizeof(buf), "%d", pin_number);
  write(fd, buf, len);
  close(fd);
  return 0;
}


/* 
 * gpio_set_direction
 * Configure the GPIO pin for either input or output.  Use out_flag=0 for
 * reading, 1 for writing.
 */
int gpio_set_direction(unsigned int pin_number, unsigned int out_flag)
{
  int fd;
 
  snprintf(gpio_buffer, sizeof(gpio_buffer),
	   "/sys/class/gpio/gpio%d/direction", pin_number);
 
  fd = open(gpio_buffer, O_WRONLY);
  if (fd < 0)
  {
    perror(gpio_buffer);
    return fd;
  }
 
  if (out_flag) {
    write(fd, "out", 4);
  } else {
    write(fd, "in", 3);
  }
 
  close(fd);
  return 0;
}

/*
 * gpio_set_value
 * For a GPIO pin set up for writing, set its state high (value != 0) or low
 * (value == 0). Returns 0 on a successful set and non-0 on failure.
 */
int gpio_set_value(unsigned int pin_number, unsigned int value)
{
  int fd;
 
  snprintf(gpio_buffer, sizeof(gpio_buffer),
	   "/sys/class/gpio/gpio%d/value", pin_number);

  fd = open(gpio_buffer, O_WRONLY);
  if (fd < 0)
  {
    perror(gpio_buffer);
    return fd;
  }
 
  if (value) {
    write(fd, "1", 2);
  } else {
    write(fd, "0", 2);
  } 
  close(fd);
  return 0;
}

/*
 * gpio_get_value
 * For a GPIO pin set up for reading, get its current state. On return, 'value'
 * will be 0 if the pin is currently low and 1 if the pin is currently
 * high. Returns 0 on a successful read and non-0 on failure.
 */
int gpio_get_value(unsigned int pin_number, unsigned int *value)
{
  int fd;
  char ch;

  snprintf(gpio_buffer, sizeof(gpio_buffer),
	   "/sys/class/gpio/gpio%d/value", pin_number);
 
  fd = open(gpio_buffer, O_RDONLY);
  if (fd < 0) {
    perror(gpio_buffer);
    return fd;
  }
 
  read(fd, &ch, 1);

  if (ch != '0') {
    *value = 1;
  } else {
    *value = 0;
  }
 
  close(fd);
  return 0;
}


