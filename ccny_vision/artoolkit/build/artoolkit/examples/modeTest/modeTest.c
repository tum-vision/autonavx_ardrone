/*
** simpleTest.c - test file to demonstrate the ARToolkit tracking code
**
** author: Mark Billinghurst, grof@hitl.washington.edu
**
** August 18th 1999
*/

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
#include "object.h"
#include "draw_object.h"



/* set up the video format globals */

#ifdef _WIN32
char			*vconf = "Data\\WDM_camera_flipV.xml";
#else
char			*vconf = "";
#endif

int             xsize;
int             ysize;
int             thresh = 100;

/* set up the matrix arrays for the camera and view transforms */
ARParam         cparam;

ObjectData_T    *object;
int             objectnum;
double          object_center[2] = {0.0, 0.0};

static int      count = 0;


/* function definitions */
static void   usage( char *com );
static int    init( int argc, char *argv[] );
static void   cleanup(void);
static void   keyEvent( unsigned char key, int x, int y);
static void   mainLoop(void);
static void   dispMode(void);

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
    argMainLoop( NULL, keyEvent, mainLoop );
	return (0);
}

static void   keyEvent( unsigned char key, int x, int y)
{
    /* quit if the ESC key is pressed */
    if( key == 0x1b ) {
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        /* print out frame/sec and shut everything down */
        cleanup();
        exit(0);
    }

    if( key == 'p' ) {
        arMatchingPCAMode = 1 - arMatchingPCAMode;
        count = 0;
        dispMode();
    }

    /* change the threshold value when 't' key pressed */
    if( key == 't' ) {
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        printf("Enter new threshold value (current = %d): ", thresh);
        scanf("%d",&thresh); while( getchar()!='\n' );
        printf("\n");
		dispMode();
        count = 0;
    }

    /* turn on and off the debug mode with right mouse */
    if( key == 'd' ) {
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        arDebug = 1 - arDebug;
        if( arDebug == 0 ) {
            glClearColor( 0.0, 0.0, 0.0, 0.0 );
            glClear(GL_COLOR_BUFFER_BIT);
            argSwapBuffers();
            glClear(GL_COLOR_BUFFER_BIT);
            argSwapBuffers();
        }
        count = 0;
        dispMode();
    }

    if( key == 'z' ) {
        if( arFittingMode == AR_FITTING_TO_IDEAL ) {
            arFittingMode  = AR_FITTING_TO_INPUT;
        }
        else {
            arFittingMode  = AR_FITTING_TO_IDEAL;
        }
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        count = 0;
        dispMode();
    }

    if( key == 'x' ) {
        if( arImageProcMode == AR_IMAGE_PROC_IN_FULL ) {
            arImageProcMode  = AR_IMAGE_PROC_IN_HALF;
        }
        else {
            arImageProcMode  = AR_IMAGE_PROC_IN_FULL;
        }
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        count = 0;
        dispMode();
    }

    if( key == 'c' ) {
        if( argDrawMode == AR_DRAW_BY_GL_DRAW_PIXELS ) {
            argDrawMode  = AR_DRAW_BY_TEXTURE_MAPPING;
            argTexmapMode = AR_DRAW_TEXTURE_FULL_IMAGE;
		}
		else if( argTexmapMode == AR_DRAW_TEXTURE_FULL_IMAGE ) {
            argTexmapMode = AR_DRAW_TEXTURE_HALF_IMAGE;
		}
		else {
            argDrawMode  = AR_DRAW_BY_GL_DRAW_PIXELS;
		}
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        count = 0;
        dispMode();
    }

    if( key == 'm' ) {
        if( arTemplateMatchingMode == AR_TEMPLATE_MATCHING_COLOR ) {
            arTemplateMatchingMode  = AR_TEMPLATE_MATCHING_BW;
        }
        else {
            arTemplateMatchingMode  = AR_TEMPLATE_MATCHING_COLOR;
        }
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        count = 0;
        dispMode();
    }
}

/* main loop */
static void mainLoop(void)
{
    ARUint8         *dataPtr;
    ARMarkerInfo    *marker_info;
    int             marker_num;
    int             i, j, k;

    if( count == 0 ) arUtilTimerReset();

    /* grab a vide frame */
    while( (dataPtr = (ARUint8 *)arVideoGetImage()) == NULL ) arUtilSleep(2);

    argDrawMode2D();
    argDispImage( dataPtr, 0, 0 );

    /* detect the markers in the video frame */
    if( arDetectMarker(dataPtr, thresh, &marker_info, &marker_num) < 0 ) {
        cleanup();
        exit(0);
    }

    /* if the debug mode is on draw squares 
       around the detected squares in the video image */
    if( arDebug ) {
        argDispImage( dataPtr, 1, 1 );
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
    arVideoCapNext();

    /* check for object visibility */
    for( i = 0; i < objectnum; i++ ) {
        object[i].visible = 0;
        k = -1;
        for( j = 0; j < marker_num; j++ ) {
            if( object[i].id == marker_info[j].id ) {
                if( k == -1 ) k = j;
                else {
                    if( marker_info[k].cf < marker_info[j].cf ) k = j;
                }
            }
        }
        if( k == -1 ) continue;

        /* get the transformation between the marker and the real camera */
        arGetTransMat(&marker_info[k], object_center, object[i].marker_width, object[i].trans);
        object[i].visible = 1;
    }

    /* draw the virtual objects attached to the cards */
    glClearDepth( 1.0 );
    glClear(GL_DEPTH_BUFFER_BIT);
    draw( object, objectnum );

    count++;
    if( count == 30 ) {
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        count = 0;
    }

    argSwapBuffers();
}

/* set up the application parameters - read in from command line*/
static int init( int argc, char *argv[] )
{
    char     cparaname[256];
    char     odataname[256];
    ARParam  wparam;
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

    dispMode();

    return 0;
}

static void dispMode( void )
{
	if( arFittingMode == AR_FITTING_TO_INPUT ) {
		printf("FittingMode (Z): INPUT IMAGE\n");
	}
	else {
		printf("FittingMode (Z): COMPENSATED IMAGE\n");
	}

	if( arImageProcMode == AR_IMAGE_PROC_IN_FULL ) {
		printf("ProcMode (X)   : FULL IMAGE\n");
	}
	else {
		printf("ProcMode (X)   : HALF IMAGE\n");
	}

	if( argDrawMode == AR_DRAW_BY_GL_DRAW_PIXELS ) {
		printf("DrawMode (C)   : GL_DRAW_PIXELS\n");
	}
	else if( argTexmapMode == AR_DRAW_TEXTURE_FULL_IMAGE ) {
		printf("DrawMode (C)   : TEXTURE MAPPING (FULL RESOLUTION)\n");
	}
	else {
		printf("DrawMode (C)   : TEXTURE MAPPING (HALF RESOLUTION)\n");
	}

	if( arTemplateMatchingMode == AR_TEMPLATE_MATCHING_COLOR ) {
		printf("TemplateMatchingMode (M)   : Color Template\n");
	}
	else {
		printf("TemplateMatchingMode (M)   : BW Template\n");
	}

        if( arMatchingPCAMode == AR_MATCHING_WITHOUT_PCA ) {
		printf("MatchingPCAMode (P)   : Without PCA\n");
        }
	else {
		printf("MatchingPCAMode (P)   : With PCA\n");
	}
}

/* cleanup function called when program exits */
static void cleanup(void)
{
    arVideoCapStop();
    arVideoClose();
    argCleanup();
}
