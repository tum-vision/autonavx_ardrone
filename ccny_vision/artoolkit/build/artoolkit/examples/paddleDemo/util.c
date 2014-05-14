#include <stdio.h>
#include <math.h>
#include "util.h"

void get_rot( double a, double b , double c, double trans[3][4] )
{
    double  sina, sinb, sinc, cosa, cosb, cosc;

    sina = sin(a); cosa = cos(a);
    sinb = sin(b); cosb = cos(b);
    sinc = sin(c); cosc = cos(c);
    trans[0][0] = cosa*cosa*cosb*cosc+sina*sina*cosc+sina*cosa*cosb*sinc-sina*cosa*sinc;
    trans[0][1] = -cosa*cosa*cosb*sinc-sina*sina*sinc+sina*cosa*cosb*cosc-sina*cosa*cosc;
    trans[0][2] = cosa*sinb;
    trans[1][0] = sina*cosa*cosb*cosc-sina*cosa*cosc+sina*sina*cosb*sinc+cosa*cosa*sinc;
    trans[1][1] = -sina*cosa*cosb*sinc+sina*cosa*sinc+sina*sina*cosb*cosc+cosa*cosa*cosc;
    trans[1][2] = sina*sinb;
    trans[2][0] = -cosa*sinb*cosc-sina*sinb*sinc;
    trans[2][1] = cosa*sinb*sinc-sina*sinb*cosc;
    trans[2][2] = cosb;

    return;
}

int get_angle( double trans[3][4], double *wa, double *wb, double *wc )
{
    double      a, b, c;
    double      sina, cosa, sinb, cosb, sinc, cosc;

    if( trans[2][2] > 1.0 ) {
        printf("cos(beta) = %f\n", trans[2][2]);
        trans[2][2] = -1.0;
    }
    else if( trans[2][2] < -1.0 ) {
        printf("cos(beta) = %f\n", trans[2][2]);
        trans[2][2] = -1.0;
    }
    cosb = trans[2][2];
    b = acos( cosb );
    sinb = sin( b );
    if( b >= 0.000001 || b <= -0.000001) {
        cosa = trans[0][2] / sinb;
        sina = trans[1][2] / sinb;
        if( cosa > 1.0 ) {
            printf("cos(alph) = %f\n", cosa);
            cosa = 1.0;
            sina = 0.0;
        }
        if( cosa < -1.0 ) {
            printf("cos(alph) = %f\n", cosa);
            cosa = -1.0;
            sina =  0.0;
        }
        if( sina > 1.0 ) {
            printf("sin(alph) = %f\n", sina);
            sina = 1.0;
            cosa = 0.0;
        }
        if( sina < -1.0 ) {
            printf("sin(alph) = %f\n", sina);
            sina = -1.0;
            cosa =  0.0;
        }
        a = acos( cosa );
        if( sina < 0 ) a = -a;

        sinc =  (trans[2][1]*trans[0][2]-trans[2][0]*trans[1][2])
              / (trans[0][2]*trans[0][2]+trans[1][2]*trans[1][2]);
        cosc =  -(trans[0][2]*trans[2][0]+trans[1][2]*trans[2][1])
               / (trans[0][2]*trans[0][2]+trans[1][2]*trans[1][2]);
        if( cosc > 1.0 ) {
            printf("cos(r) = %f\n", cosc);
            cosc = 1.0;
            sinc = 0.0;
        }
        if( cosc < -1.0 ) {
            printf("cos(r) = %f\n", cosc);
            cosc = -1.0;
            sinc =  0.0;
        }
        if( sinc > 1.0 ) {
            printf("sin(r) = %f\n", sinc);
            sinc = 1.0;
            cosc = 0.0;
        }
        if( sinc < -1.0 ) {
            printf("sin(r) = %f\n", sinc);
            sinc = -1.0;
            cosc =  0.0;
        }
        c = acos( cosc );
        if( sinc < 0 ) c = -c;
    }
    else {
        a = b = 0.0;
        cosa = cosb = 1.0;
        sina = sinb = 0.0;
        cosc = trans[0][0];
        sinc = trans[1][0];
        if( cosc > 1.0 ) {
            printf("cos(r) = %f\n", cosc);
            cosc = 1.0;
            sinc = 0.0;
        }
        if( cosc < -1.0 ) {
            printf("cos(r) = %f\n", cosc);
            cosc = -1.0;
            sinc =  0.0;
        }
        if( sinc > 1.0 ) {
            printf("sin(r) = %f\n", sinc);
            sinc = 1.0;
            cosc = 0.0;
        }
        if( sinc < -1.0 ) {
            printf("sin(r) = %f\n", sinc);
            sinc = -1.0;
            cosc =  0.0;
        }
        c = acos( cosc );
        if( sinc < 0 ) c = -c;
    }

    *wa = a;
    *wb = b;
    *wc = c;

    return(0);
}

double get_height( double px, double py, double trans[3][4], double boundary[3][2] )
{
    double x, x0, x1, x2;
    double y, y0, y1, y2;
    double d, m00, m01, m10, m11;
    double rx, ry;

    x = boundary[0][0];
    y = boundary[1][0];
    x0 = trans[0][0]*x + trans[0][1]*y + trans[0][3];
    y0 = trans[1][0]*x + trans[1][1]*y + trans[1][3];
    x = boundary[0][1];
    y = boundary[1][0];
    x1 = trans[0][0]*x + trans[0][1]*y + trans[0][3];
    y1 = trans[1][0]*x + trans[1][1]*y + trans[1][3];
    x = boundary[0][0];
    y = boundary[1][1];
    x2 = trans[0][0]*x + trans[0][1]*y + trans[0][3];
    y2 = trans[1][0]*x + trans[1][1]*y + trans[1][3];

    x1 -= x0; y1 -= y0;
    x2 -= x0; y2 -= y0;
    x0 = px - x0;
    y0 = py - y0;

    d = x1*y2 - x2*y1;
    if( d == 0.0 ) return 0.0;
    m00 =  y2 / d;
    m01 = -x2 / d;
    m10 = -y1 / d;
    m11 =  x1 / d;

    rx = m00*x0 + m01*y0;
    ry = m10*x0 + m11*y0;

    if( rx >= 0.0 && rx <= 1.0 && ry >= 0.0 && ry <= 1.0 ) {
        return trans[2][3] + boundary[2][1];
    }

    return 0.0;
}
