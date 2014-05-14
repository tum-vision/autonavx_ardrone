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
#include <AR/gsub.h>
#include <AR/video.h>
#include <AR/param.h>
#include <AR/ar.h>
#include "draw_object.h"



/* set up the video format globals */

#ifdef _WIN32
char    *vconf = "Data\\WDM_camera_flipV.xml";
#else
char    *vconf = "";
#endif

int             xsize;
int             ysize;
int             thresh = 100;
ARParam         cparam;
int             outputMode = 0;

int             mouse_ox;
int             mouse_oy;
int             mouse_st = 0;
int             disp_mode = 1;
double          a =   0.0;
double          b = -45.0;
double          r = 500.0;

int             target_id;
double          target_center[2] = {0.0, 0.0};
double          target_width = 80.0;


/* function definitions */
static int    init(void);
static void   cleanup(void);
static void   keyEvent( unsigned char key, int x, int y);
static void   mouseEvent(int button, int state, int x, int y);
static void   motionEvent( int x, int y );
static void   mainLoop(void);

static void   getResultRaw( ARMarkerInfo *marker_info );
static void   getResultQuat( ARMarkerInfo *marker_info );

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
    if( init() < 0 ) exit(0);

    arVideoCapStart();
    glutMotionFunc( motionEvent );
    argMainLoop( mouseEvent, keyEvent, mainLoop );
	return (0);
}

static void mainLoop(void)
{
    ARUint8         *dataPtr;
    ARMarkerInfo    *marker_info;
    int             marker_num;
    int             j, k;

    /* grab a vide frame */
    if( (dataPtr = (ARUint8 *)arVideoGetImage()) == NULL ) {
        arUtilSleep(2);
        return;
    }

    glClearColor( 0.0, 0.0, 0.0, 0.0 );
    glClearDepth( 1.0 );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    argDrawMode2D();
    if( disp_mode ) {
        argDispImage( dataPtr, 0, 0 );
    }
    else {
        argDispImage( dataPtr, 1, 1 );
    }

    /* detect the markers in the video frame */
    if( arDetectMarker(dataPtr, thresh, &marker_info, &marker_num) < 0 ) {
        cleanup();
        exit(0);
    }
    arVideoCapNext();

    /* if the debug mode is on draw squares 
       around the detected squares in the video image */
    if( arDebug ) {
        if( arImageProcMode == AR_IMAGE_PROC_IN_HALF )
            argDispHalfImage( arImage, 2, 1 );
        else
            argDispImage( arImage, 2, 1);
    }

    /* check for object visibility */
    k = -1;
    for( j = 0; j < marker_num; j++ ) {
        if( marker_info[j].id == target_id ) {
            if( k == -1 ) k = j;
            else {
                if( marker_info[k].cf < marker_info[j].cf ) k = j;
            }
        }
    }
    if( k != -1 ) {
        glDisable(GL_DEPTH_TEST);
        switch( outputMode ) {
            case 0:
                getResultRaw( &marker_info[k] );
                break;
            case 1:
                getResultQuat( &marker_info[k] );
                break;
        }
    }

    argSwapBuffers();
}

static void getResultRaw( ARMarkerInfo *marker_info )
{
    double      target_trans[3][4];
    double      cam_trans[3][4];
    char        string[256];

    if( arGetTransMat(marker_info, target_center, target_width, target_trans) < 0 ) return;
    if( arUtilMatInv(target_trans, cam_trans) < 0 ) return;

    sprintf(string," RAW: Cam Pos x: %3.1f  y: %3.1f  z: %3.1f",
            cam_trans[0][3], cam_trans[1][3], cam_trans[2][3]);

    if( disp_mode ) {
        draw( "target", target_trans, 0, 0 );
        draw_exview( a, b, r, target_trans, 1, 1 );
    }
    else {
        draw( "target", target_trans, 1, 1 );
        draw_exview( a, b, r, target_trans, 0, 0 );
    }
    print_string( string );

    return;
}

static void getResultQuat( ARMarkerInfo *marker_info )
{
    double      target_trans[3][4];
    double      cam_trans[3][4];
    double      quat[4], pos[3];
    char        string1[256];
    char        string2[256];

    if( arGetTransMat(marker_info, target_center, target_width, target_trans) < 0 ) return;
    if( arUtilMatInv(target_trans, cam_trans) < 0 ) return;
    if( arUtilMat2QuatPos(cam_trans, quat, pos) < 0 ) return;

    sprintf(string1," QUAT: Pos x: %3.1f  y: %3.1f  z: %3.1f\n",
            pos[0], pos[1], pos[2]);
    sprintf(string2, "      Quat qx: %3.2f qy: %3.2f qz: %3.2f qw: %3.2f ",
            quat[0], quat[1], quat[2], quat[3]);
    strcat( string1, string2 );

    if( disp_mode ) {
        draw( "target", target_trans, 0, 0 );
        draw_exview( a, b, r, target_trans, 1, 1 );
    }
    else {
        draw( "target", target_trans, 1, 1 );
        draw_exview( a, b, r, target_trans, 0, 0 );
    }
    print_string( string1 );

    return;
}

/* set up the application parameters - read in from command line*/
static int init(void)
{
    char     cparaname[256];
    char     pattname[256];
    ARParam  wparam;

    strcpy( cparaname, "Data/camera_para.dat" );
    strcpy( pattname,  "Data/patt.hiro" );
    
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

    /* open the graphics window */
    argInit( &cparam, 1.0, 0, 2, 1, 0 );

    if( (target_id = arLoadPatt(pattname)) < 0 ) {
        printf("Target pattern load error!!\n");
        exit(0);
    }

    arDebug = 0;

    return 0;
}

/* cleanup function called when program exits */
static void cleanup(void)
{
    arVideoCapStop();
    arVideoClose();
    argCleanup();
}

static void   keyEvent( unsigned char key, int x, int y)
{
    /* quit if the ESC key is pressed */
    if( key == 0x1b ) {
        cleanup();
        exit(0);
    }

    /* change the threshold value when 't' key pressed */
    if( key == 't' ) {
        printf("Enter new threshold value (default = 100): ");
        scanf("%d",&thresh); while( getchar()!='\n' );
        printf("\n");
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

    if(key == 'o') {
        outputMode = (outputMode + 1) % 2;
    }

    if(key == 'c' ) {
        disp_mode = 1 - disp_mode;
    }

}

static void motionEvent( int x, int y )
{
    if( mouse_st == 1 ) {
        a += ((double)x - mouse_ox) / 2.0;
        b -= ((double)y - mouse_oy) / 2.0;
        if( a <   0.0 ) a += 360.0;
        if( a > 360.0 ) a -= 360.0;
        if( b < -90.0 ) b =  -90.0;
        if( b >   0.0 ) b =    0.0;
    }
    else if( mouse_st == 2 ) {
        r *= (1.0 + ((double)y - mouse_oy)*0.01);
        if( r < 10.0 ) r = 10.0;
    }

    mouse_ox = x;
    mouse_oy = y;
}

static void mouseEvent(int button, int state, int x, int y)
{
    if( state == GLUT_UP ) {
        mouse_st = 0;
    }
    else if( state == GLUT_DOWN ) {
        if( button == GLUT_LEFT_BUTTON ) {
            mouse_st = 1;
            mouse_ox = x;
            mouse_oy = y;
        }
        else if( button == GLUT_MIDDLE_BUTTON ) {
            disp_mode = 1 - disp_mode;
        }
        else if( button == GLUT_RIGHT_BUTTON ) {
            mouse_st = 2;
            mouse_ox = x;
            mouse_oy = y;
        }
    }
}
