#ifndef UTIL_H
#define UTIL_H

void   get_rot( double a, double b , double c, double trans[3][4] );
int    get_angle( double trans[3][4], double *wa, double *wb, double *wc );
double get_height( double x, double y, double trans[3][4], double boundary[3][2] );

/* RJS */
int
wrappedDropOntoMap(char *where, float *inPt, float *outPt);

#endif
