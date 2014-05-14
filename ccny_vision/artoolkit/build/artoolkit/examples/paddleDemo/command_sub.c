#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <AR/param.h>
#include <AR/matrix.h>
#include <AR/ar.h>
#include "paddle.h"
#include "command_sub.h"
#include "util.h"


#define  SHAKE_BUF_SIZE     10
#define  PUNCH_BUF_SIZE      5
#define  PUSH_BUF_SIZE       2

typedef struct {
    double   mat[3][4];
    int      f;
} SHAKE_BUF_T;

typedef struct {
    double   x, y, z;
    int      f;
} PUNCH_BUF_T;

typedef struct {
    double   x, y, z;
    int      f;
} PUSH_BUF_T;

static SHAKE_BUF_T    shake_buf[SHAKE_BUF_SIZE];
static int            shake_buf_num = 0;
//static PUNCH_BUF_T    punch_buf[PUNCH_BUF_SIZE];
//static int            punch_buf_num = 0;
//static PUSH_BUF_T     push_buf[PUNCH_BUF_SIZE];
//static int            push_buf_num = 0;


int check_shake( double card_trans[3][4], int f )
{
    ARMat   *mat_a,  *mat_b, *mat_c;
    double  lxy, lz;
    int     i, j, k;

    if( shake_buf_num < SHAKE_BUF_SIZE ) {
        if( f ) {
            for( j = 0; j < 3; j++ ) {
                for( i = 0; i < 4; i++ ) {
                    shake_buf[shake_buf_num].mat[j][i] = card_trans[j][i];
                }
            }
            shake_buf[shake_buf_num].f = 1;
        }
        else {
            shake_buf[shake_buf_num].f = 0;
        }
        shake_buf_num++;

        return 0;
    }
    for( i = 1; i < shake_buf_num; i++ ) {
        shake_buf[i-1] = shake_buf[i];
    }
    if( f ) {
        for( j = 0; j < 3; j++ ) {
            for( i = 0; i < 4; i++ ) {
                shake_buf[shake_buf_num-1].mat[j][i] = card_trans[j][i];
            }
        }
        shake_buf[shake_buf_num-1].f = 1;
    }
    else {
        shake_buf[shake_buf_num-1].f = 0;

        return 0;
    }

    if( shake_buf[SHAKE_BUF_SIZE-3].f == 0
     || shake_buf[0].f == 0 ) return 0;


    mat_a = arMatrixAlloc( 4, 4 );
    mat_b = arMatrixAlloc( 4, 4 );
    mat_c = arMatrixAlloc( 4, 4 );
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            mat_a->m[j*4+i] = card_trans[j][i];
        }
    }
    mat_a->m[3*4+0] = 0.0;
    mat_a->m[3*4+1] = 0.0;
    mat_a->m[3*4+2] = 0.0;
    mat_a->m[3*4+3] = 1.0;
    arMatrixSelfInv( mat_a );

    for( k = 0 ; k < SHAKE_BUF_SIZE-3; k++ ) {
        if( shake_buf[k].f == 0 ) continue;

        for( j = 0; j < 3; j++ ) {
            for( i = 0; i < 4; i++ ) {
                mat_b->m[j*4+i] = shake_buf[k].mat[j][i];
            }
        }
        mat_b->m[3*4+0] = 0.0;
        mat_b->m[3*4+1] = 0.0;
        mat_b->m[3*4+2] = 0.0;
        mat_b->m[3*4+3] = 1.0;
        arMatrixMul( mat_c, mat_a, mat_b );

        lxy = sqrt( (mat_c->m[0*4+3])*(mat_c->m[0*4+3])
                   +(mat_c->m[1*4+3])*(mat_c->m[1*4+3]));
        lz  = mat_c->m[2*4+3];

        if( lxy < 20.0 && lz < 20.0 ) break;
    }
    if( k == SHAKE_BUF_SIZE-3 ) {
        arMatrixFree( mat_a );
        arMatrixFree( mat_b );
        arMatrixFree( mat_c );
        return 0;
    }

    for( ; k < SHAKE_BUF_SIZE-1; k++ ) {
        if( shake_buf[k].f == 0 ) continue;

        for( j = 0; j < 3; j++ ) {
            for( i = 0; i < 4; i++ ) {
                mat_b->m[j*4+i] = shake_buf[k].mat[j][i];
            }
        }
        mat_b->m[3*4+0] = 0.0;
        mat_b->m[3*4+1] = 0.0;
        mat_b->m[3*4+2] = 0.0;
        mat_b->m[3*4+3] = 1.0;
        arMatrixMul( mat_c, mat_a, mat_b );

        lxy = sqrt( (mat_c->m[0*4+3])*(mat_c->m[0*4+3])
                   +(mat_c->m[1*4+3])*(mat_c->m[1*4+3]));
        lz  = mat_c->m[2*4+3];

        if( lxy > 60.0 && lz < 20.0 ) break;
    }
    arMatrixFree( mat_a );
    arMatrixFree( mat_b );
    arMatrixFree( mat_c );

    if( k < SHAKE_BUF_SIZE-1 ) {
        shake_buf_num = 0;
        return 1;
    }

    return 0;
}


int check_incline( double card_trans[3][4], double base_trans[3][4], double *angle )
{
    ARMat   *mat_a, *mat_b, *mat_c;
    double  a, b, c;
    int     i, j;

    mat_a = arMatrixAlloc( 4, 4 );
    mat_b = arMatrixAlloc( 4, 4 );
    mat_c = arMatrixAlloc( 4, 4 );
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            mat_b->m[j*4+i] = base_trans[j][i];
        }
    }
    mat_b->m[3*4+0] = 0.0;
    mat_b->m[3*4+1] = 0.0;
    mat_b->m[3*4+2] = 0.0;
    mat_b->m[3*4+3] = 1.0;

    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            mat_a->m[j*4+i] = card_trans[j][i];
        }
    }
    mat_a->m[3*4+0] = 0.0;
    mat_a->m[3*4+1] = 0.0;
    mat_a->m[3*4+2] = 0.0;
    mat_a->m[3*4+3] = 1.0;
    arMatrixSelfInv( mat_a );
    arMatrixMul( mat_c, mat_a, mat_b );

    get_angle( (double (*)[4])(mat_c->m), &a, &b, &c );

    arMatrixFree( mat_a );
    arMatrixFree( mat_b );
    arMatrixFree( mat_c );

    if( b > 0.4 ) {
      *angle = a + 3.141592;
      return 1;
    }

    return 0;
}

int check_pickup(double card_trans[3][4], double base_trans[3][4], ItemList* itlist, double* angle)
{
    ARMat   *t1, *t2, *t3;
    double   x, y, z;
    double   lx, ly;
    double   a, b, c;
    int      ret;
    int      i, j;

    //    printf("checking pickup New \n");

    t1 = arMatrixAlloc( 4, 4 );
    for( j = 0; j < 3; j++ ) {
       for( i = 0; i < 4; i++ ) {
            t1->m[j*4+i] = base_trans[j][i];
        }
    }
    t1->m[12] = t1->m[13] = t1->m[14] = 0.0;
    t1->m[15] = 1.0;

    t2 = arMatrixAlloc( 4, 4 );
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            t2->m[j*4+i] = card_trans[j][i];
        }
    }
    t2->m[12] = t2->m[13] = t2->m[14] = 0.0;
    t2->m[15] = 1.0;

    if( arMatrixSelfInv(t1) != 0 ) {
        arMatrixFree( t1 );
        arMatrixFree( t2 );
        return -1;
    }    
    //    printf("past arMatrixSelfInv\n");
    
    t3 = arMatrixAllocMul(t1, t2);
    if( t3 == NULL ) {
        arMatrixFree( t1 );
        arMatrixFree( t2 );
        return -1;
    }

    //    printf("past arMatrixAllocMul\n");

    x = t3->m[0*4+3];
    y = t3->m[1*4+3];
    z = t3->m[2*4+3];

    //  printf("x: %f y: %f z: %f\n",x,y,z);

    ret = -1;
    for( i = 0; i < itlist->itemnum; i ++ ){
      lx = x - itlist->item[i].pos[0];
      ly = y - itlist->item[i].pos[1];
      //MB increased by a factor of 10
      if( lx*lx + ly*ly < 1000.0 && z < 20.0 ) {
          ret = i;
      }
    }

    if( ret >= 0 ) {
        get_angle( (double (*)[4])(t3->m), &a, &b, &c );
        *angle = -c;
    }

    arMatrixFree( t1 );
    arMatrixFree( t2 );
    arMatrixFree( t3 );

    return ret;
}
