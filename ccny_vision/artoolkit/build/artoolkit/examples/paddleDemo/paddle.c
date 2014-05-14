#include <stdio.h>
#include <stdlib.h>
#include <AR/ar.h>
#include "paddle.h"

static char *get_buff( char *buf, int n, FILE *fp );

static int get_marker( ARMarkerInfo *markerInfo, int *markerFlag, 
                       int marker_num, int id,
                       ARMarkerInfo *prevInfo, int *pcount );


ARPaddleInfo *paddleInit( char *name )
{
    FILE          *fp;
    ARPaddleInfo  *paddleInfo;
    char           buf[256], buf1[256];
  
    if( (fp=fopen(name, "r")) == NULL ) return(0);

    arMalloc( paddleInfo, ARPaddleInfo, 1 );

    get_buff(buf, 256, fp);
    if( sscanf(buf, "%s", paddleInfo->name) != 1 ) {
      fclose(fp); free(paddleInfo); return 0;
    }

    get_buff(buf, 256, fp);
    if( sscanf(buf, "%s", buf1) != 1 ) {
      fclose(fp); free(paddleInfo); return 0;
    }
    if( (paddleInfo->marker_id = arLoadPatt(buf1)) < 0 ) {
      fclose(fp); free(paddleInfo); return 0;
    }

    get_buff(buf, 256, fp);
    if( sscanf(buf, "%lf", &paddleInfo->width) != 1 ) {
      fclose(fp); free(paddleInfo); return 0;
    }

    paddleInfo->center[0] = 0.0;
    paddleInfo->center[1] = 0.0;
    paddleInfo->active    = 0;
    paddleInfo->pcount    = 0;
    paddleInfo->pcountL = 0;
    paddleInfo->pcountR = 0;

    fclose(fp);

    return paddleInfo;
}

static char *get_buff( char *buf, int n, FILE *fp )
{
    char *ret;

    for(;;) {
        ret = fgets( buf, n, fp );
        if( ret == NULL ) return(NULL);
        if( buf[0] != '\n' && buf[0] != '#' ) return(ret);
    }
}

int paddleGetTrans( ARPaddleInfo *paddleInfo, ARMarkerInfo *markerInfo, int *markerFlag, int marker_num, ARParam *cparam )
{
    int   id;

    id = get_marker( markerInfo, markerFlag, marker_num, paddleInfo->marker_id,
                      &(paddleInfo->prevInfo), &(paddleInfo->pcount) );

    if( id == -1 ) { paddleInfo->active = 0; return 0; }

    arGetTransMat(&markerInfo[id],
                  paddleInfo->center, paddleInfo->width, paddleInfo->trans);
    markerFlag[id] = 1;

    paddleInfo->active = 1;

    return 0;
}

static int get_marker( ARMarkerInfo *markerInfo, int *markerFlag, int marker_num, int id,
                       ARMarkerInfo *prevInfo, int *pcount )
{
    double  rlen, rlenmin, rarea, diff, diffmin;
    int     cid, cdir;
    int     i, j;

    cid = -1;
    for( i = 0; i < marker_num; i++ ) {
        if( markerInfo[i].id == id && markerFlag[i] == 0 ) {
            if( cid == -1 ) cid = i;
            else {
                if( markerInfo[cid].cf < markerInfo[i].cf ) cid = i;
            }
        }
    }
    if( cid == -1 ) {
        if( *pcount == 0 ) return -1;

        rlenmin = 10.0;
        for( i = 0; i < marker_num; i++ ) {
            if( markerFlag[i] ) continue;
            rarea = (double)prevInfo->area / (double)markerInfo[i].area;
            if( rarea < 0.7 || rarea > 1.43 ) continue;
            rlen = ( (markerInfo[i].pos[0] - prevInfo->pos[0])
                   * (markerInfo[i].pos[0] - prevInfo->pos[0])
                   + (markerInfo[i].pos[1] - prevInfo->pos[1])
                   * (markerInfo[i].pos[1] - prevInfo->pos[1]) ) / markerInfo[i].area;
            if( rlen < 0.5 && rlen < rlenmin ) {
                rlenmin = rlen;
                cid = i;
            }
        }
        if( cid >= 0 && markerInfo[cid].cf < prevInfo->cf ) {
            markerInfo[cid].cf = prevInfo->cf;
            markerInfo[cid].id = prevInfo->id;
            diffmin = 10000.0 * 10000.0;
            cdir = -1;
            for( i = 0; i < 4; i++ ) {
                diff = 0.0;
                for( j = 0; j < 4; j++ ) {
                    diff += (prevInfo->vertex[j][0] - markerInfo[cid].vertex[(i+j)%4][0])
                          * (prevInfo->vertex[j][0] - markerInfo[cid].vertex[(i+j)%4][0])
                          + (prevInfo->vertex[j][1] - markerInfo[cid].vertex[(i+j)%4][1])
                          * (prevInfo->vertex[j][1] - markerInfo[cid].vertex[(i+j)%4][1]);
                }
                if( diff < diffmin ) {
                    diffmin = diff;
                    cdir = (prevInfo->dir - i + 4) % 4;
                }
            }
            markerInfo[cid].dir = cdir;

            *prevInfo = markerInfo[cid];
            *pcount = 1;
        }
        else {
            *pcount = 0;
            return -1;
        }
    }
    else {
        *prevInfo = markerInfo[cid];
        *pcount = 1;
    }

    return cid;
}
