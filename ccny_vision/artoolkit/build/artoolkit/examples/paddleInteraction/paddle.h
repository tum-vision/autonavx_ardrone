#ifndef PADDLE_H
#define PADDLE_H

#include <AR/ar.h>

#define PADDLE_RADIUS   41.0

typedef struct {
  char       name[256];
  int        marker_id;
  double     width;
  double     center[2];
  
  double     trans[3][4];
  double     transL[3][4];
  double     transR[3][4];
  int        active;

  ARMarkerInfo  prevInfo;
  ARMarkerInfo  prevInfoL;
  ARMarkerInfo  prevInfoR;
  
  int           pcount;
  int           pcountL;
  int           pcountR;
} ARPaddleInfo;

ARPaddleInfo  *paddleInit( char *name );

int paddleGetTrans( ARPaddleInfo *paddleInfo, ARMarkerInfo *markerInfo, 
                    int *flagL, int marker_num, ARParam *cparam );

#endif
