#ifndef __SERVO_H
#define __SERVO_H	 
#include "sys.h"
#include "delay.h"



void PWM_Init(void);
void Servo_SetAngle(float angle);
int SERVO_Test(void);



#endif
