#ifdef _WIN32
#  include <windows.h>
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "calib_cparam.h"

void   initLineModel( int   *line_num, int *loop_num,
                      int    line_mode[LINE_MAX],
                      double inter_coord[LOOP_MAX][LINE_MAX][LINE_MAX][3] )
{
    int     i, j, k;
    char    buf[256];
    int     hno, vno;
    double  dist1, dist2;

    hno = 7;
    vno = 9;
    *loop_num = 5;
    dist1 =  40.0;
    dist2 = 100.0;

    printf("Number of horizontal lines (%d): ", hno);
    fgets(buf, 256, stdin);
    sscanf( buf, "%d", &hno );
    printf("Number of vertical lines (%d): ", vno);
    fgets(buf, 256, stdin);
    sscanf( buf, "%d", &vno );
    if( hno + vno > LINE_MAX ) {
        printf("too many lines!!\n");
        exit(0);
    }

    printf("Number of iteration (%d): ", *loop_num);
    fgets(buf, 256, stdin);
    sscanf( buf, "%d", loop_num );
    if( *loop_num > LOOP_MAX ) {
        printf("too many iteration!!\n");
        exit(0);
    }

    printf("Distance among lines (%f): ", dist1);
    fgets(buf, 256, stdin);
    sscanf( buf, "%lf", &dist1 );
    printf("Distance to move (%f): ", dist2);
    fgets(buf, 256, stdin);
    sscanf( buf, "%lf", &dist2 );


    *line_num = hno + vno;
    for( i = 0; i < hno; i++ ) line_mode[i]     = L_HORIZONTAL;
    for( i = 0; i < vno; i++ ) line_mode[i+hno] = L_VERTICAL;

    for( k = 0; k < LOOP_MAX; k++ ) {
        for( i = 0; i < LINE_MAX; i++ ) {
            for( j = 0; j < LINE_MAX; j++ ) {
                inter_coord[k][j][i][0] = -10000.0;
                inter_coord[k][j][i][1] = -10000.0;
                inter_coord[k][j][i][2] = -10000.0;
            }
        }
    }

    for( k = 0; k < *loop_num; k++ ) {
        for( j = 0; j < hno; j++ ) {
            for( i = 0; i < vno; i++ ) {
                inter_coord[k][j][i+hno][0] =   dist1 * i;
                inter_coord[k][j][i+hno][1] =   dist1 * j;
                inter_coord[k][j][i+hno][2] =   dist2 * k;
            }
        }
    }
}
