#include <math.h>
#include <stdio.h>
#include <AR/ar.h>
#include <AR/param.h>
#include <AR/matrix.h>
#include "calib_camera.h"

static int calc_inp2( CALIB_PATT_T *patt, CALIB_COORD_T *screen, double *pos2d, double *pos3d,
                      double dist_factor[4], double x0, double y0, double f[2], double *err );
static void get_cpara( CALIB_COORD_T *world, CALIB_COORD_T *screen, int num, double para[3][3] );
static int  get_fl( double *p , double *q, int num, double f[2] );
static int  check_rotation( double rot[2][3] );

int calc_inp( CALIB_PATT_T *patt, double dist_factor[4], int xsize, int ysize, double mat[3][4] )
{
    CALIB_COORD_T  *screen, *sp;
    double         *pos2d, *pos3d, *pp;
    double         f[2];
    double         x0, y0;
    double         err, minerr;
    int            res;
    int            i, j, k;

    sp = screen = (CALIB_COORD_T *)malloc(sizeof(CALIB_COORD_T) * patt->h_num * patt->v_num * patt->loop_num);
    pp = pos2d = (double *)malloc(sizeof(double) * patt->h_num * patt->v_num * patt->loop_num * 2);
    pos3d = (double *)malloc(sizeof(double) * patt->h_num * patt->v_num * 2);
    for( k = 0; k < patt->loop_num; k++ ) {
        for( j = 0; j < patt->v_num; j++ ) {
            for( i = 0; i < patt->h_num; i++ ) {
                arParamObserv2Ideal( dist_factor, 
                                     patt->point[k][j*patt->h_num+i].x_coord,
                                     patt->point[k][j*patt->h_num+i].y_coord,
                                     &(sp->x_coord), &(sp->y_coord) );
                *(pp++) = sp->x_coord;
                *(pp++) = sp->y_coord;
                sp++;
            }
        }
    }
    pp = pos3d;
    for( j = 0; j < patt->v_num; j++ ) {
        for( i = 0; i < patt->h_num; i++ ) {
            *(pp++) = patt->world_coord[j*patt->h_num+i].x_coord;
            *(pp++) = patt->world_coord[j*patt->h_num+i].y_coord;
        }
    }

    minerr = 100000000000000000000000.0;
    for( j = -50; j <= 50; j++ ) {
        printf("-- loop:%d --\n", j);
        y0 = dist_factor[1] + j;
/*      y0 = ysize/2 + j;   */
        if( y0 < 0 || y0 >= ysize ) continue;

        for( i = -50; i <= 50; i++ ) {
            x0 = dist_factor[0] + i;
/*          x0 = xsize/2 + i;  */
            if( x0 < 0 || x0 >= xsize ) continue;

            res = calc_inp2( patt, screen, pos2d, pos3d, dist_factor, x0, y0, f, &err );
            if( res < 0 ) continue;
            if( err < minerr ) {
                printf("F = (%f,%f), Center = (%f,%f): err = %f\n", f[0], f[1], x0, y0, err);
                minerr = err;

                mat[0][0] = f[0];
                mat[0][1] = 0.0;
                mat[0][2] = x0;
                mat[0][3] = 0.0;
                mat[1][0] = 0.0;
                mat[1][1] = f[1];
                mat[1][2] = y0;
                mat[1][3] = 0.0;
                mat[2][0] = 0.0;
                mat[2][1] = 0.0;
                mat[2][2] = 1.0;
                mat[2][3] = 0.0;
            }
        }
    }

    free( screen );
    free( pos2d );
    free( pos3d );

    if( minerr >= 100.0 ) return -1;
    return 0;
}


static int calc_inp2 ( CALIB_PATT_T *patt, CALIB_COORD_T *screen, double *pos2d, double *pos3d,
                      double dist_factor[4], double x0, double y0, double f[2], double *err )
{
    double  x1, y1, x2, y2;
    double  p[LOOP_MAX], q[LOOP_MAX];
    double  para[3][3];
    double  rot[3][3], rot2[3][3];
    double  cpara[3][4], conv[3][4];
    double  ppos2d[4][2], ppos3d[4][2];
    double  d, werr, werr2;
    int     i, j, k, l;

    for( i =  0; i < patt->loop_num; i++ ) {
        get_cpara( patt->world_coord, &(screen[i*patt->h_num*patt->v_num]),
                   patt->h_num*patt->v_num, para );
        x1 = para[0][0] / para[2][0];
        y1 = para[1][0] / para[2][0];
        x2 = para[0][1] / para[2][1];
        y2 = para[1][1] / para[2][1];

        p[i] = (x1 - x0)*(x2 - x0);
        q[i] = (y1 - y0)*(y2 - y0);
    }
    if( get_fl(p, q, patt->loop_num, f) < 0 ) return -1;

    cpara[0][0] = f[0];
    cpara[0][1] = 0.0;
    cpara[0][2] = x0;
    cpara[0][3] = 0.0;
    cpara[1][0] = 0.0;
    cpara[1][1] = f[1];
    cpara[1][2] = y0;
    cpara[1][3] = 0.0;
    cpara[2][0] = 0.0;
    cpara[2][1] = 0.0;
    cpara[2][2] = 1.0;
    cpara[2][3] = 0.0;

    werr = 0.0;
    for( i =  0; i < patt->loop_num; i++ ) {
        get_cpara( patt->world_coord, &(screen[i*patt->h_num*patt->v_num]),
                   patt->h_num*patt->v_num, para );
        rot[0][0] = (para[0][0] - x0*para[2][0]) / f[0];
        rot[0][1] = (para[1][0] - y0*para[2][0]) / f[1];
        rot[0][2] = para[2][0];
        d = sqrt( rot[0][0]*rot[0][0] + rot[0][1]*rot[0][1] + rot[0][2]*rot[0][2] );
        rot[0][0] /= d;
        rot[0][1] /= d;
        rot[0][2] /= d;
        rot[1][0] = (para[0][1] - x0*para[2][1]) / f[0];
        rot[1][1] = (para[1][1] - y0*para[2][1]) / f[1];
        rot[1][2] = para[2][1];
        d = sqrt( rot[1][0]*rot[1][0] + rot[1][1]*rot[1][1] + rot[1][2]*rot[1][2] );
        rot[1][0] /= d;
        rot[1][1] /= d;
        rot[1][2] /= d;
        check_rotation( rot );
        rot[2][0] = rot[0][1]*rot[1][2] - rot[0][2]*rot[1][1];
        rot[2][1] = rot[0][2]*rot[1][0] - rot[0][0]*rot[1][2];
        rot[2][2] = rot[0][0]*rot[1][1] - rot[0][1]*rot[1][0];
        d = sqrt( rot[2][0]*rot[2][0] + rot[2][1]*rot[2][1] + rot[2][2]*rot[2][2] );
        rot[2][0] /= d;
        rot[2][1] /= d;
        rot[2][2] /= d;
        rot2[0][0] = rot[0][0];
        rot2[1][0] = rot[0][1];
        rot2[2][0] = rot[0][2];
        rot2[0][1] = rot[1][0];
        rot2[1][1] = rot[1][1];
        rot2[2][1] = rot[1][2];
        rot2[0][2] = rot[2][0];
        rot2[1][2] = rot[2][1];
        rot2[2][2] = rot[2][2];

        ppos2d[0][0] = pos2d[i*patt->h_num*patt->v_num*2 + 0];
        ppos2d[0][1] = pos2d[i*patt->h_num*patt->v_num*2 + 1];
        ppos2d[1][0] = pos2d[i*patt->h_num*patt->v_num*2 + (patt->h_num-1)*2 + 0];
        ppos2d[1][1] = pos2d[i*patt->h_num*patt->v_num*2 + (patt->h_num-1)*2 + 1];
        ppos2d[2][0] = pos2d[i*patt->h_num*patt->v_num*2 + (patt->h_num*(patt->v_num-1))*2 + 0];
        ppos2d[2][1] = pos2d[i*patt->h_num*patt->v_num*2 + (patt->h_num*(patt->v_num-1))*2 + 1];
        ppos2d[3][0] = pos2d[i*patt->h_num*patt->v_num*2 + (patt->h_num*patt->v_num-1)*2 + 0];
        ppos2d[3][1] = pos2d[i*patt->h_num*patt->v_num*2 + (patt->h_num*patt->v_num-1)*2 + 1];
        ppos3d[0][0] = pos3d[0];
        ppos3d[0][1] = pos3d[1];
        ppos3d[1][0] = pos3d[(patt->h_num-1)*2 + 0];
        ppos3d[1][1] = pos3d[(patt->h_num-1)*2 + 1];
        ppos3d[2][0] = pos3d[(patt->h_num*(patt->v_num-1))*2 + 0];
        ppos3d[2][1] = pos3d[(patt->h_num*(patt->v_num-1))*2 + 1];
        ppos3d[3][0] = pos3d[(patt->h_num*patt->v_num-1)*2 + 0];
        ppos3d[3][1] = pos3d[(patt->h_num*patt->v_num-1)*2 + 1];

        for( j = 0; j < 5; j++ ) {
            arFittingMode = AR_FITTING_TO_IDEAL;
            werr2 = arGetTransMat3( rot2, ppos2d, ppos3d, 4, conv, dist_factor, cpara );
            for( k = 0; k < 3; k++ ) {
            for( l = 0; l < 3; l++ ) {
                rot2[k][l] = conv[k][l];
            }}
        }
        werr += werr2;

    }
    *err = sqrt( werr / patt->loop_num );

    return 0;
}

static void get_cpara( CALIB_COORD_T *world, CALIB_COORD_T *screen, int num, double para[3][3] )
{
    ARMat   *a, *b, *c;
    ARMat   *at, *aa, res;
    int     i;

    a = arMatrixAlloc( num*2, 8 );
    b = arMatrixAlloc( num*2, 1 );
    c = arMatrixAlloc( 8, num*2 );
    at = arMatrixAlloc( 8, num*2 );
    aa = arMatrixAlloc( 8, 8 );
    for( i = 0; i < num; i++ ) {
        a->m[i*16+0]  = world[i].x_coord;
        a->m[i*16+1]  = world[i].y_coord;
        a->m[i*16+2]  = 1.0;
        a->m[i*16+3]  = 0.0;
        a->m[i*16+4]  = 0.0;
        a->m[i*16+5]  = 0.0;
        a->m[i*16+6]  = -world[i].x_coord * screen[i].x_coord;
        a->m[i*16+7]  = -world[i].y_coord * screen[i].x_coord;
        a->m[i*16+8]  = 0.0;
        a->m[i*16+9]  = 0.0;
        a->m[i*16+10] = 0.0;
        a->m[i*16+11] = world[i].x_coord;
        a->m[i*16+12] = world[i].y_coord;
        a->m[i*16+13] = 1.0;
        a->m[i*16+14] = -world[i].x_coord * screen[i].y_coord;
        a->m[i*16+15] = -world[i].y_coord * screen[i].y_coord;
        b->m[i*2+0] = screen[i].x_coord;
        b->m[i*2+1] = screen[i].y_coord;
    }
    arMatrixTrans( at, a );
    arMatrixMul( aa, at, a );
    arMatrixSelfInv( aa );
    arMatrixMul( c, aa, at );
    res.row = 8;
    res.clm = 1;
    res.m = &(para[0][0]);
    arMatrixMul( &res, c, b );
    para[2][2] = 1.0;

    arMatrixFree( a );
    arMatrixFree( b );
    arMatrixFree( c );
    arMatrixFree( at );
    arMatrixFree( aa );
}

static int get_fl( double *p , double *q, int num, double f[2] )
{
    ARMat   *a, *b, *c;
    ARMat   *at, *aa, *res;
    int     i;

#if 1
    a = arMatrixAlloc( num, 2 );
    b = arMatrixAlloc( num, 1 );
    c = arMatrixAlloc( 2, num );
    at = arMatrixAlloc( 2, num );
    aa = arMatrixAlloc( 2, 2 );
    res = arMatrixAlloc( 2, 1 );
    for( i = 0; i < num; i++ ) {
        a->m[i*2+0] = *(p++);
        a->m[i*2+1] = *(q++);
        b->m[i]     = -1.0;
    }
#else
    a = arMatrixAlloc( num-1, 2 );
    b = arMatrixAlloc( num-1, 1 );
    c = arMatrixAlloc( 2, num-1 );
    at = arMatrixAlloc( 2, num-1 );
    aa = arMatrixAlloc( 2, 2 );
    res = arMatrixAlloc( 2, 1 );
    p++; q++;
    for( i = 0; i < num-1; i++ ) {
        a->m[i*2+0] = *(p++);
        a->m[i*2+1] = *(q++);
        b->m[i]     = -1.0;
    }
#endif
    arMatrixTrans( at, a );
    arMatrixMul( aa, at, a );
    arMatrixSelfInv( aa );
    arMatrixMul( c, aa, at );
    arMatrixMul( res, c, b );

    if( res->m[0] < 0 || res->m[1] < 0 ) return -1;

    f[0] = sqrt( 1.0 / res->m[0] );
    f[1] = sqrt( 1.0 / res->m[1] );

    arMatrixFree( a );
    arMatrixFree( b );
    arMatrixFree( c );
    arMatrixFree( at );
    arMatrixFree( aa );
    arMatrixFree( res );

    return 0;
}

static int check_rotation( double rot[2][3] )
{
    double  v1[3], v2[3], v3[3];
    double  ca, cb, k1, k2, k3, k4;
    double  a, b, c, d;
    double  p1, q1, r1;
    double  p2, q2, r2;
    double  p3, q3, r3;
    double  p4, q4, r4;
    double  w;
    double  e1, e2, e3, e4;
    int     f;

    v1[0] = rot[0][0];
    v1[1] = rot[0][1];
    v1[2] = rot[0][2];
    v2[0] = rot[1][0];
    v2[1] = rot[1][1];
    v2[2] = rot[1][2];
    v3[0] = v1[1]*v2[2] - v1[2]*v2[1];
    v3[1] = v1[2]*v2[0] - v1[0]*v2[2];
    v3[2] = v1[0]*v2[1] - v1[1]*v2[0];
    w = sqrt( v3[0]*v3[0]+v3[1]*v3[1]+v3[2]*v3[2] );
    if( w == 0.0 ) return -1;
    v3[0] /= w;
    v3[1] /= w;
    v3[2] /= w;

    cb = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
    if( cb < 0 ) cb *= -1.0;
    ca = (sqrt(cb+1.0) + sqrt(1.0-cb)) * 0.5;

    if( v3[1]*v1[0] - v1[1]*v3[0] != 0.0 ) {
        f = 0;
    }
    else {
        if( v3[2]*v1[0] - v1[2]*v3[0] != 0.0 ) {
            w = v1[1]; v1[1] = v1[2]; v1[2] = w;
            w = v3[1]; v3[1] = v3[2]; v3[2] = w;
            f = 1;
        }
        else {
            w = v1[0]; v1[0] = v1[2]; v1[2] = w;
            w = v3[0]; v3[0] = v3[2]; v3[2] = w;
            f = 2;
        }
    }
    if( v3[1]*v1[0] - v1[1]*v3[0] == 0.0 ) return -1;
    k1 = (v1[1]*v3[2] - v3[1]*v1[2]) / (v3[1]*v1[0] - v1[1]*v3[0]);
    k2 = (v3[1] * ca) / (v3[1]*v1[0] - v1[1]*v3[0]);
    k3 = (v1[0]*v3[2] - v3[0]*v1[2]) / (v3[0]*v1[1] - v1[0]*v3[1]);
    k4 = (v3[0] * ca) / (v3[0]*v1[1] - v1[0]*v3[1]);

    a = k1*k1 + k3*k3 + 1;
    b = k1*k2 + k3*k4;
    c = k2*k2 + k4*k4 - 1;

    d = b*b - a*c;
    if( d < 0 ) return -1;
    r1 = (-b + sqrt(d))/a;
    p1 = k1*r1 + k2;
    q1 = k3*r1 + k4;
    r2 = (-b - sqrt(d))/a;
    p2 = k1*r2 + k2;
    q2 = k3*r2 + k4;
    if( f == 1 ) {
        w = q1; q1 = r1; r1 = w;
        w = q2; q2 = r2; r2 = w;
        w = v1[1]; v1[1] = v1[2]; v1[2] = w;
        w = v3[1]; v3[1] = v3[2]; v3[2] = w;
        f = 0;
    }
    if( f == 2 ) {
        w = p1; p1 = r1; r1 = w;
        w = p2; p2 = r2; r2 = w;
        w = v1[0]; v1[0] = v1[2]; v1[2] = w;
        w = v3[0]; v3[0] = v3[2]; v3[2] = w;
        f = 0;
    }

    if( v3[1]*v2[0] - v2[1]*v3[0] != 0.0 ) {
        f = 0;
    }
    else {
        if( v3[2]*v2[0] - v2[2]*v3[0] != 0.0 ) {
            w = v2[1]; v2[1] = v2[2]; v2[2] = w;
            w = v3[1]; v3[1] = v3[2]; v3[2] = w;
            f = 1;
        }
        else {
            w = v2[0]; v2[0] = v2[2]; v2[2] = w;
            w = v3[0]; v3[0] = v3[2]; v3[2] = w;
            f = 2;
        }
    }
    if( v3[1]*v2[0] - v2[1]*v3[0] == 0.0 ) return -1;
    k1 = (v2[1]*v3[2] - v3[1]*v2[2]) / (v3[1]*v2[0] - v2[1]*v3[0]);
    k2 = (v3[1] * ca) / (v3[1]*v2[0] - v2[1]*v3[0]);
    k3 = (v2[0]*v3[2] - v3[0]*v2[2]) / (v3[0]*v2[1] - v2[0]*v3[1]);
    k4 = (v3[0] * ca) / (v3[0]*v2[1] - v2[0]*v3[1]);

    a = k1*k1 + k3*k3 + 1;
    b = k1*k2 + k3*k4;
    c = k2*k2 + k4*k4 - 1;

    d = b*b - a*c;
    if( d < 0 ) return -1;
    r3 = (-b + sqrt(d))/a;
    p3 = k1*r3 + k2;
    q3 = k3*r3 + k4;
    r4 = (-b - sqrt(d))/a;
    p4 = k1*r4 + k2;
    q4 = k3*r4 + k4;
    if( f == 1 ) {
        w = q3; q3 = r3; r3 = w;
        w = q4; q4 = r4; r4 = w;
        w = v2[1]; v2[1] = v2[2]; v2[2] = w;
        w = v3[1]; v3[1] = v3[2]; v3[2] = w;
        f = 0;
    }
    if( f == 2 ) {
        w = p3; p3 = r3; r3 = w;
        w = p4; p4 = r4; r4 = w;
        w = v2[0]; v2[0] = v2[2]; v2[2] = w;
        w = v3[0]; v3[0] = v3[2]; v3[2] = w;
        f = 0;
    }

    e1 = p1*p3+q1*q3+r1*r3; if( e1 < 0 ) e1 = -e1;
    e2 = p1*p4+q1*q4+r1*r4; if( e2 < 0 ) e2 = -e2;
    e3 = p2*p3+q2*q3+r2*r3; if( e3 < 0 ) e3 = -e3;
    e4 = p2*p4+q2*q4+r2*r4; if( e4 < 0 ) e4 = -e4;
    if( e1 < e2 ) {
        if( e1 < e3 ) {
            if( e1 < e4 ) {
                rot[0][0] = p1;
                rot[0][1] = q1;
                rot[0][2] = r1;
                rot[1][0] = p3;
                rot[1][1] = q3;
                rot[1][2] = r3;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
        else {
            if( e3 < e4 ) {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p3;
                rot[1][1] = q3;
                rot[1][2] = r3;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
    }
    else {
        if( e2 < e3 ) {
            if( e2 < e4 ) {
                rot[0][0] = p1;
                rot[0][1] = q1;
                rot[0][2] = r1;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
        else {
            if( e3 < e4 ) {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p3;
                rot[1][1] = q3;
                rot[1][2] = r3;
            }
            else {
                rot[0][0] = p2;
                rot[0][1] = q2;
                rot[0][2] = r2;
                rot[1][0] = p4;
                rot[1][1] = q4;
                rot[1][2] = r4;
            }
        }
    }

    return 0;
}
