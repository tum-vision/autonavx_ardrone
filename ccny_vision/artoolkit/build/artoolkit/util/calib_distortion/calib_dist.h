#ifndef CALIB_DIST_H
#define CALIB_DIST_H

#include <AR/gsub_lite.h>

#define  THRESH     100
#define  H_NUM        6
#define  V_NUM        4
#define  LOOP_MAX    20

typedef struct {
    double   x_coord;
    double   y_coord;
} CALIB_COORD_T;

typedef struct patt {
    unsigned char  *savedImage[LOOP_MAX];
	ARGL_CONTEXT_SETTINGS_REF arglSettings[LOOP_MAX];
    CALIB_COORD_T  *point[LOOP_MAX];
    int            h_num;				// Number of dots horizontally in the calibration pattern.
    int            v_num;				// Number of dots vertically in the calibration pattern.
    int            loop_num;			// How many images of the complete calibration patterns we have completed.
} CALIB_PATT_T;

void calc_distortion( CALIB_PATT_T *patt, int xsize, int ysize, double dist_factor[4] );

#endif
