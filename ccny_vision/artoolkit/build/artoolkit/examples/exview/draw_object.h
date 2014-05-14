#ifndef DRAW_OBJECT_H
#define DRAW_OBJECT_H

void print_string( char *string );
int  draw( char name[], double trans[3][4], int xwin, int ywin );
int  draw_exview( double a, double b, double r, double trans[3][4], int xwin, int ywin );

#endif
