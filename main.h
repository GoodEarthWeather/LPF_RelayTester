/*
 * main.h
 *
 *  Created on: May 16, 2018
 *      Author: dmcneill
 */


#ifndef MYINCLUDE_MAIN_H_
#define MYINCLUDE_MAIN_H_

#include "driverlib.h"


// Define all I/O

#define SELECT_40M 0b01101001
#define SELECT_2030M 0b10100110
#define SELECT_1517M 0b10011010

#define delay_ms(x)     __delay_cycles((long) x* 1000 * 8)
#define delay_us(x)     __delay_cycles((long) x * 8)



#endif /* MYINCLUDE_MAIN_H_ */
