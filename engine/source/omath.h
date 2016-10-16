/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

#ifndef OMATH_H
#define OMATH_H

// *** INCLUDES ***
#include <math.h>

extern float sin_table[];
extern float cos_table[];

static volatile const double Infinity = INFINITY;
static volatile const double Tiny = 0x1p-1022;

// double2 represents a number equal to d0 + d1, with |d1| <= 1/2 ULP(d0).
typedef struct { double d0, d1; } double2;

double2 Add112RightSmaller(double, double);
double2 Add212RightSmaller(double2, double);
double Add221RightSmaller(double2, double2);
double Sub211RightSmaller(double2, double);
double Sub221RightSmaller(double2, double2);
double2 Mul112(double, double);
double Mul121Special(double, double2);
double Mul221(double2, double2);
double2 Mul222(double2, double2);
double Center(double);
double Tail(double);
double Gap(double);
double pTail(double);
double nTail(double);
double atani0(double);
double atani1(double);
double atani2(double);
double atani3(double);
double atani4(double);
double atani5(double);

// main
double aasin(double);
double aacos(double);
double aatan(double);
float degree_sin(float);
float degree_cos(float);
float norm_angle(float);
float mantix(float);
float invsqrt(float);


#endif

