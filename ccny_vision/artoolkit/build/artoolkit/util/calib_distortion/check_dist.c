#ifdef _WIN32
#  include <windows.h>
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <AR/param.h>
#include <AR/matrix.h>
#include "calib_dist.h"

static double   get_fitting_error( CALIB_PATT_T *patt, double dist_factor[4] );
static double   check_error( double *x, double *y, int num, double dist_factor[4] );
static double   calc_distortion2( CALIB_PATT_T *patt, double dist_factor[4] );
static double   get_size_factor( double dist_factor[4], int xsize, int ysize );

void calc_distortion( CALIB_PATT_T *patt, int xsize, int ysize, double dist_factor[4] )
{
    int     i, j;
    double  bx, by;
    double  bf[4];
    double  error, min;
    double  factor[4];

    bx = xsize / 2;
    by = ysize / 2;
    factor[0] = bx;
    factor[1] = by;
    factor[3] = 1.0;
    min = calc_distortion2( patt, factor );
    bf[0] = factor[0];
    bf[1] = factor[1];
    bf[2] = factor[2];
    bf[3] = 1.0;
printf("[%5.1f, %5.1f, %5.1f] %f\n", bf[0], bf[1], bf[2], min);
    for( j = -10; j <= 10; j++ ) {
        factor[1] = by + j*5;
        for( i = -10; i <= 10; i++ ) {
            factor[0] = bx + i*5;
            error = calc_distortion2( patt, factor );
            if( error < min ) { bf[0] = factor[0]; bf[1] = factor[1];
                                bf[2] = factor[2]; min = error; }
        }
printf("[%5.1f, %5.1f, %5.1f] %f\n", bf[0], bf[1], bf[2], min);
    }

    bx = bf[0];
    by = bf[1];
    for( j = -10; j <= 10; j++ ) {
        factor[1] = by + 0.5 * j;
        for( i = -10; i <= 10; i++ ) {
            factor[0] = bx + 0.5 * i;
            error = calc_distortion2( patt, factor );
            if( error < min ) { bf[0] = factor[0]; bf[1] = factor[1];
                                bf[2] = factor[2]; min = error; }
        }
printf("[%5.1f, %5.1f, %5.1f] %f\n", bf[0], bf[1], bf[2], min);
    }

    dist_factor[0] = bf[0];
    dist_factor[1] = bf[1];
    dist_factor[2] = bf[2];
    dist_factor[3] = get_size_factor( bf, xsize, ysize );
}

static double get_size_factor( double dist_factor[4], int xsize, int ysize )
{
    double  ox, oy, ix, iy;
    double  olen, ilen;
    double  sf, sf1;

    sf = 100.0;

    ox = 0.0;
    oy = dist_factor[1];
    olen = dist_factor[0];
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy );
    ilen = dist_factor[0] - ix;
printf("Olen = %f, Ilen = %f\n", olen, ilen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    ox = xsize;
    oy = dist_factor[1];
    olen = xsize - dist_factor[0];
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy );
    ilen = ix - dist_factor[0];
printf("Olen = %f, Ilen = %f\n", olen, ilen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    ox = dist_factor[0];
    oy = 0.0;
    olen = dist_factor[1];
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy );
    ilen = dist_factor[1] - iy;
printf("Olen = %f, Ilen = %f\n", olen, ilen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    ox = dist_factor[0];
    oy = ysize;
    olen = ysize - dist_factor[1];
    arParamObserv2Ideal( dist_factor, ox, oy, &ix, &iy );
    ilen = iy - dist_factor[1];
printf("Olen = %f, Ilen = %f\n", olen, ilen);
    if( ilen > 0 ) {
        sf1 = ilen / olen;
        if( sf1 < sf ) sf = sf1;
    }

    if( sf == 0.0 ) sf = 1.0;

    return sf;
}

static double calc_distortion2( CALIB_PATT_T *patt, double dist_factor[4] )
{
    double    min, err, f, fb;
    int       i;

    dist_factor[2] = 0.0;
    min = get_fitting_error( patt, dist_factor );

    f = dist_factor[2];
    for( i = 10; i < 200; i+=10 ) {
        dist_factor[2] = i;
        err = get_fitting_error( patt, dist_factor );
        if( err < min ) { min = err; f = dist_factor[2]; }
    }

    fb = f;
    for( i = -10; i <= 10; i++ ) {
        dist_factor[2] = fb + i;
        if( dist_factor[2] < 0 ) continue;
        err = get_fitting_error( patt, dist_factor );
        if( err < min ) { min = err; f = dist_factor[2]; }
    }

    fb = f;
    for( i = -10; i <= 10; i++ ) {
        dist_factor[2] = fb + 0.1 * i;
        if( dist_factor[2] < 0 ) continue;
        err = get_fitting_error( patt, dist_factor );
        if( err < min ) { min = err; f = dist_factor[2]; }
    }

    dist_factor[2] = f;
    return min;
}

static double get_fitting_error( CALIB_PATT_T *patt, double dist_factor[4] )
{
    double   *x, *y;
    double   error;
    int      max;
    int      i, j, k, l;
    int      p;

    max = (patt->v_num > patt->h_num)? patt->v_num: patt->h_num;
    x = (double *)malloc( sizeof(double)*max );
    y = (double *)malloc( sizeof(double)*max );
    if( x == NULL || y == NULL ) exit(0);

    error = 0.0;
    for( i = 0; i < patt->loop_num; i++ ) {
        for( j = 0; j < patt->v_num; j++ ) {
            for( k = 0; k < patt->h_num; k++ ) {
                x[k] = patt->point[i][j*patt->h_num+k].x_coord;
                y[k] = patt->point[i][j*patt->h_num+k].y_coord;
            }
            error += check_error( x, y, patt->h_num, dist_factor );
        }

        for( j = 0; j < patt->h_num; j++ ) {
            for( k = 0; k < patt->v_num; k++ ) {
                x[k] = patt->point[i][k*patt->h_num+j].x_coord;
                y[k] = patt->point[i][k*patt->h_num+j].y_coord;
            }
            error += check_error( x, y, patt->v_num, dist_factor );
        }

        for( j = 3 - patt->v_num; j < patt->h_num - 2; j++ ) {
            p = 0;
            for( k = 0; k < patt->v_num; k++ ) {
                l = j+k;
                if( l < 0 || l >= patt->h_num ) continue;
                x[p] = patt->point[i][k*patt->h_num+l].x_coord;
                y[p] = patt->point[i][k*patt->h_num+l].y_coord;
                p++;
            }
            error += check_error( x, y, p, dist_factor );
        }

        for( j = 2; j < patt->h_num + patt->v_num - 3; j++ ) {
            p = 0;
            for( k = 0; k < patt->v_num; k++ ) {
                l = j-k;
                if( l < 0 || l >= patt->h_num ) continue;
                x[p] = patt->point[i][k*patt->h_num+l].x_coord;
                y[p] = patt->point[i][k*patt->h_num+l].y_coord;
                p++;
            }
            error += check_error( x, y, p, dist_factor );
        }
    }

    free( x );
    free( y );

    return error;
}

static double check_error( double *x, double *y, int num, double dist_factor[4] )
{
    ARMat    *input, *evec;
    ARVec    *ev, *mean;
    double   a, b, c;
    double   error;
    int      i;

    ev     = arVecAlloc( 2 );
    mean   = arVecAlloc( 2 );
    evec   = arMatrixAlloc( 2, 2 );

    input  = arMatrixAlloc( num, 2 );
    for( i = 0; i < num; i++ ) {
        arParamObserv2Ideal( dist_factor, x[i], y[i],
                             &(input->m[i*2+0]), &(input->m[i*2+1]) );
    }
    if( arMatrixPCA(input, evec, ev, mean) < 0 ) exit(0);
    a =  evec->m[1];
    b = -evec->m[0];
    c = -(a*mean->v[0] + b*mean->v[1]);

    error = 0.0;
    for( i = 0; i < num; i++ ) {
        error += (a*input->m[i*2+0] + b*input->m[i*2+1] + c)
               * (a*input->m[i*2+0] + b*input->m[i*2+1] + c);
    }
    error /= (a*a + b*b);

    arMatrixFree( input );
    arMatrixFree( evec );
    arVecFree( mean );
    arVecFree( ev );

    return error;
}
