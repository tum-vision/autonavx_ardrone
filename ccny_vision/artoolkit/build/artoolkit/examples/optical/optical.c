#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __APPLE__
#include <GL/gl.h>
#include <GL/glut.h>
#else
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#endif
#include <AR/gsubUtil.h>
#include <AR/gsub.h>
#include <AR/video.h>
#include <AR/param.h>
#include <AR/ar.h>
#include "object.h"
#include "draw_object.h"

#define  TARGET_PATT_FILE  "Data/patt.calib"

/* set up the video format globals */

#ifdef _WIN32
char			*vconf = "Data\\WDM_camera_flipV.xml";
#else
char			*vconf = "";
#endif

int             xsize;
int             ysize;
int             thresh = 100;
int             opticalFlag = 0;
int             saveFittingMode;

/* set up the matrix arrays for the camera and view transforms */
ARParam         cparam;
int             hmd_param_flag;
int             target_id;
ObjectData_T    *object;
int             objectnum;
double          object_center[2] = {0.0, 0.0};

static int      count = 0;

/* function definitions */
static void   usage( char *com );
static int    init( int argc, char *argv[] );
static void   cleanup(void);

static void   keyEvent( unsigned char key, int x, int y);
static void   mouseEvent(int button, int state, int x, int y);
static void   mainLoop(void);
static void   calibPostFunc(ARParam *lpara, ARParam *rpara);

static void usage( char *com )
{
    printf("Usage: %s [options]\n", com);
    printf("   Options:\n");
    printf("      -c <camera parameter filename>\n");
    printf("      -o <object data filename>\n");
    printf("\n");
    exit(0);
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
    if( init( argc, argv ) < 0 ) exit(0);

    arVideoCapStart();
    argMainLoop( mouseEvent, keyEvent, mainLoop );
	return (0);
}

static void   keyEvent( unsigned char key, int x, int y)
{
    /* quit if the ESC key is pressed */
    if( key == 0x1b ) {
        /* print out frame/sec and shut everything down */
        cleanup();
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        exit(0);
    }

    /* change the threshold value when 't' key pressed */
    if( key == 't' ) {
        printf("Enter new threshold value (default = 100): ");
        scanf("%d",&thresh); while( getchar()!='\n' );
        printf("\n");
        count = 0;
    }

    /* turn on and off the debug mode with right mouse */
    if( key == 'd' ) {
        arDebug = 1 - arDebug;
        if( arDebug == 0 ) {
            glClearColor( 0.0, 0.0, 0.0, 0.0 );
            glClear(GL_COLOR_BUFFER_BIT);
            argSwapBuffers();
            glClear(GL_COLOR_BUFFER_BIT);
            argSwapBuffers();
        }
    }
}

/* mouse event handling function */
static void mouseEvent(int button, int state, int x, int y)
{
    /* change display modes on left mouse button
        - between video see-through and optical see-through */
    if( button == GLUT_LEFT_BUTTON  && state == GLUT_DOWN ) {
        if( hmd_param_flag ) {
			opticalFlag = 1 - opticalFlag;
			if( opticalFlag == 0 ) {
                arFittingMode = saveFittingMode;
			}
			else {
                arFittingMode = AR_FITTING_TO_IDEAL;
			}
		}
    }

    /* turn on and off the debug mode with middle mouse */
    if( button == GLUT_RIGHT_BUTTON  && state == GLUT_DOWN ) {
        argUtilCalibHMD( target_id, thresh, calibPostFunc );
    }
}

static void calibPostFunc(ARParam *lpara, ARParam *rpara)
{
    count = 0;

    if( lpara == NULL || rpara == NULL ) {
        printf("Calibration error!!\n");
        return;
    }

    hmd_param_flag = 1;
    opticalFlag  = 1;
    arFittingMode = AR_FITTING_TO_IDEAL;

    printf("*** HMD Parameter ***\n");
    printf("LEFT\n");
    arParamDisp( lpara );
    printf("Right\n");
    arParamDisp( rpara );
    printf("\n");
}

/* main loop */
static void mainLoop(void)
{
    ARUint8         *dataPtr;
    ARMarkerInfo    *marker_info;
    int             marker_num;
    int             i, j, k;

    /* grab a vide frame */
    if( (dataPtr = (ARUint8 *)arVideoGetImage()) == NULL ) {
        arUtilSleep(2);
        return;
    }
    if( count == 0 ) arUtilTimerReset();
    count++;

    if( opticalFlag == 0 ) {
        argDispImage( dataPtr, 0, 0 );
        if( !arDebug ) arVideoCapNext();
        glClearDepth( 1.0 );
        glClear(GL_DEPTH_BUFFER_BIT);
    }
    else {
        if( !arDebug ) arVideoCapNext();
        glClearDepth( 1.0 );
        glClearColor( 0.0, 0.0, 0.0, 0.0 );
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    /* detect the markers in the video frame */
    if( arDetectMarker(dataPtr, thresh, &marker_info, &marker_num) < 0 ) {
        cleanup();
        exit(0);
    }

    /* if the debug mode is on draw squares 
       around the detected squares in the video image */
    if( arDebug ) {
        argDispImage( dataPtr, 1, 1 );
        arVideoCapNext();
        if( arImageProcMode == AR_IMAGE_PROC_IN_HALF )
            argDispHalfImage( arImage, 2, 1 );
        else
            argDispImage( arImage, 2, 1);

        glColor3f( 1.0, 0.0, 0.0 );
        glLineWidth( 3.0 );
        for( i = 0; i < marker_num; i++ ) {
            if( marker_info[i].id < 0 ) continue;
            argDrawSquare( marker_info[i].vertex, 2, 1 );
        }
        glLineWidth( 1.0 );
    }

    /* check for object visibility */
    for( i = 0; i < objectnum; i++ ) {
        k = -1;
        for( j = 0; j < marker_num; j++ ) {
            if( object[i].id == marker_info[j].id ) {
                if( k == -1 ) k = j;
                else {
                    if( marker_info[k].cf < marker_info[j].cf ) k = j;
                }
            }
        }
        if( k == -1 ) {
            object[i].visible = 0;
            continue;
        }

        /* get the transformation between the marker and the real camera */
        if( arGetTransMat(&marker_info[k],
                          object_center, object[i].marker_width, object[i].trans) < 0 ) {
            object[i].visible = 0;
        }
        else {
            object[i].visible = 1;
        }
    }

    /* draw the virtual objects attached to the cards */
    draw( object, objectnum, opticalFlag );

    argSwapBuffers();
}

/* set up the application parameters - read in from command line*/
static int init( int argc, char *argv[] )
{
    char     cparaname[256];
    char     odataname[256];
    ARParam  wparam;
/*
    ARParam  wlpara, wrpara;
*/
    int      i;

    /* copy in name of the camera parameter file, the hmd parameter file 
       and the object data file */
    strcpy( cparaname, "Data/camera_para.dat" );
    strcpy( odataname, "Data/object_data" );
    
    /* read in the parameters from the various files */
    for( i = 1; i < argc; i++ ) {
        if( strcmp(argv[i],"-c") == 0 ) {
            if( i < argc-1 && argv[i+1][0] != '-' ) {
                strcpy( cparaname, argv[i+1] );
                i++;
            }
            else usage( argv[0] );
        }
        else if( strcmp(argv[i],"-o") == 0 ) {
            if( i < argc-1 && argv[i+1][0] != '-' ) {
                strcpy( odataname, argv[i+1] );
                i++;
            }
            else usage( argv[0] );
        }
        else usage( argv[0] );
    }

    /* load in the object data - trained markers and associated bitmap files */
    if( (object=read_objectdata(odataname,&objectnum)) == NULL ) exit(0);
    if( (target_id = arLoadPatt(TARGET_PATT_FILE)) < 0 ) {
        printf("Target pattern load error!!\n");
        exit(0);
    }

    /* open the video path */
    if( arVideoOpen( vconf ) < 0 ) exit(0);
    /* find the size of the window */
    if( arVideoInqSize(&xsize, &ysize) < 0 ) exit(0);
    printf("Image size (x,y) = (%d,%d)\n", xsize, ysize);

    /* set the initial camera parameters */
    if( arParamLoad(cparaname, 1, &wparam) < 0 ) {
       printf("Camera parameter load error !!\n");
        exit(0);
    }
    arParamChangeSize( &wparam, xsize, ysize, &cparam );
    arInitCparam( &cparam );
    printf("*** Camera Parameter ***\n");
    arParamDisp( &cparam );

/*
    arParamLoad( "Data/hmd_para", 2, &wlpara, &wrpara);
    argLoadHMDparam( &wrpara, &wlpara );
    hmd_param_flag = 1;
*/
    hmd_param_flag = 0;

    /* open the graphics window */
    argInit( &cparam, 1.0, 0, 2, 1, 1 );

    /* initialize lights and material properties */
    init_lights();

    saveFittingMode = arFittingMode;
    arDebug       = 0;

    return 0;
}

/* cleanup function called when program exits */
static void cleanup(void)
{
    arVideoCapStop();
    arVideoClose();
    argCleanup();
}
