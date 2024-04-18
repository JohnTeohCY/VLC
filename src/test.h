#include <Arduino.h>
#include <TeensyTimerTool.h>
#include <SPI.h>

#define PACKETLEN 16384

extern const boolean p1_8PPM[PACKETLEN];
extern const boolean p2_8PPM[PACKETLEN];
extern const boolean p3_8PPM[PACKETLEN];
extern const boolean p4_8PPM[PACKETLEN];
extern const boolean p1_4PPM[PACKETLEN];
extern const boolean p2_4PPM[PACKETLEN];
extern const boolean p3_4PPM[PACKETLEN];
extern const boolean p1_2PPM[PACKETLEN];
extern const boolean p2_2PPM[PACKETLEN];
extern const boolean p3_2PPM[PACKETLEN];
extern const boolean p1_43OPPM[PACKETLEN];
extern const boolean p2_43OPPM[PACKETLEN];
extern const boolean p3_43OPPM[PACKETLEN];
