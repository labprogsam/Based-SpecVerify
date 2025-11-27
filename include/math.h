/* Minimal math.h stub for ESBMC on Windows */
#ifndef _MATH_H
#define _MATH_H

#include <stddef.h>

/* Mathematical constants */
#define M_PI 3.14159265358979323846
#define M_E 2.71828182845904523536

/* Basic mathematical functions */
double fabs(double x);
float fabsf(float x);
long double fabsl(long double x);

double sqrt(double x);
float sqrtf(float x);
long double sqrtl(long double x);

double pow(double x, double y);
float powf(float x, float y);
long double powl(long double x, long double y);

double exp(double x);
float expf(float x);
long double expl(long double x);

double log(double x);
float logf(float x);
long double logl(long double x);

double log10(double x);
float log10f(float x);
long double log10l(long double x);

double sin(double x);
float sinf(float x);
long double sinl(long double x);

double cos(double x);
float cosf(float x);
long double cosl(long double x);

double tan(double x);
float tanf(float x);
long double tanl(long double x);

double asin(double x);
float asinf(float x);
long double asinl(long double x);

double acos(double x);
float acosf(float x);
long double acosl(long double x);

double atan(double x);
float atanf(float x);
long double atanl(long double x);

double atan2(double y, double x);
float atan2f(float y, float x);
long double atan2l(long double y, long double x);

double ceil(double x);
float ceilf(float x);
long double ceill(long double x);

double floor(double x);
float floorf(float x);
long double floorl(long double x);

double fmod(double x, double y);
float fmodf(float x, float y);
long double fmodl(long double x, long double y);

double round(double x);
float roundf(float x);
long double roundl(long double x);

double trunc(double x);
float truncf(float x);
long double truncl(long double x);

int isnan(double x);
int isnanf(float x);
int isnanl(long double x);

int isinf(double x);
int isinff(float x);
int isinfl(long double x);

double fmin(double x, double y);
float fminf(float x, float y);
long double fminl(long double x, long double y);

double fmax(double x, double y);
float fmaxf(float x, float y);
long double fmaxl(long double x, long double y);

#endif /* _MATH_H */

