#ifdef _WIN32
#  include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef __APPLE__
#  include <GL/glut.h>
#else
#  include <GLUT/glut.h>
#endif
#include <AR/gsub.h>
#include <AR/param.h>
#include <AR/ar.h>
#include <AR/video.h>
#include <AR/arMulti.h>

#include "paddle.h"
int  draw_paddle( ARPaddleInfo *paddleInfo );

int             xsize, ysize;
int             thresh = 100;
int             count = 0;

char           *cparam_name    = "Data/camera_para.dat";
ARParam         cparam;

/* set up the video format globals */

#ifdef _WIN32
char			*vconf = "Data\\WDM_camera_flipV.xml";
#else
char			*vconf = "";
#endif

char                *config_name = "Data/multi/marker.dat";
ARMultiMarkerInfoT  *config;

//MB - paddle information 
int              marker_flag[AR_SQUARE_MAX];
ARPaddleInfo   *paddleInfo;
char           *paddle_name    = "Data/paddle_data";  

static void   init(void);
static void   cleanup(void);
static void   keyEvent( unsigned char key, int x, int y);
static void   mainLoop(void);
static void   draw( double trans1[3][4], double trans2[3][4], int mode );

int main(int argc, char **argv)
{
	//initialize application
	glutInit(&argc, argv);
    init();

	arVideoCapStart();

	//start the main event loop
    argMainLoop( NULL, keyEvent, mainLoop );

	return 0;
}

static void   keyEvent( unsigned char key, int x, int y)   
{
    /* quit if the ESC key is pressed */
    if( key == 0x1b ) {
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        cleanup();
        exit(0);
    }

	/* turn on and off the debug mode with d key */
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
    }
}

/* main loop */
static void mainLoop(void)
{
    ARUint8         *dataPtr;
    ARMarkerInfo    *marker_info;
    int             marker_num;
    int             i;
    double          err;
    
    /* grab a video frame */
    if( (dataPtr = (ARUint8 *)arVideoGetImage()) == NULL ) {
        arUtilSleep(2);
        return;
    }
	
    if( count == 0 ) arUtilTimerReset();  
    count++;
   
	/* detect the markers in the video frame */
    if( arDetectMarkerLite(dataPtr, thresh, &marker_info, &marker_num) < 0 ) {
        cleanup();
        exit(0);
    }

    argDrawMode2D();
    if( !arDebug ) {
        argDispImage( dataPtr, 0,0 );
    }
    else {
        argDispImage( dataPtr, 1, 1 );
        if( arImageProcMode == AR_IMAGE_PROC_IN_HALF )
            argDispHalfImage( arImage, 0, 0 );
        else
            argDispImage( arImage, 0, 0);

        glColor3f( 1.0, 0.0, 0.0 );
        glLineWidth( 1.0 );
        for( i = 0; i < marker_num; i++ ) {
            argDrawSquare( marker_info[i].vertex, 0, 0 );
        }
        glLineWidth( 1.0 );
    }

	arVideoCapNext();

	//MB - multiple marker tracking
	for( i = 0; i < marker_num; i++ ) marker_flag[i] = 0;
  
	/* get the paddle position */
	paddleGetTrans(paddleInfo, marker_info, marker_flag, 
				marker_num, &cparam);
	
	/* draw the 3D models */
	glClearDepth( 1.0 );
    glClear(GL_DEPTH_BUFFER_BIT);

	/* draw the paddle */
	if( paddleInfo->active ){ 
		draw_paddle( paddleInfo);
	}

	/* get the translation from the multimarker pattern */
	if( (err=arMultiGetTransMat(marker_info, marker_num, config)) < 0 ) {
        argSwapBuffers();
        return;
    }	
	
    //printf("err = %f\n", err);
    if(err > 100.0 ) {
        argSwapBuffers();
        return;
    }

	/* draw the multimarker pattern */
    for( i = 0; i < config->marker_num; i++ ) {
        if( config->marker[i].visible >= 0 ) 
				draw( config->trans, config->marker[i].trans, 0 );
        else                                 
				draw( config->trans, config->marker[i].trans, 1 );
    }

	argSwapBuffers();
}

static void init( void )
{
	ARParam  wparam;
	
    /* open the video path */
    if( arVideoOpen( vconf ) < 0 ) exit(0);
    /* find the size of the window */
    if( arVideoInqSize(&xsize, &ysize) < 0 ) exit(0);
    printf("Image size (x,y) = (%d,%d)\n", xsize, ysize);

    /* set the initial camera parameters */
    if( arParamLoad(cparam_name, 1, &wparam) < 0 ) {
        printf("Camera parameter load error !!\n");
        exit(0);
    }
    arParamChangeSize( &wparam, xsize, ysize, &cparam );
    arInitCparam( &cparam );
    printf("*** Camera Parameter ***\n");
    arParamDisp( &cparam );

	/* load the paddle marker file */
	if( (paddleInfo = paddleInit(paddle_name)) == NULL ) {
		printf("paddleInit error!!\n");
		exit(0);
	}
	printf("Loaded Paddle File\n");

	if( (config = arMultiReadConfigFile(config_name)) == NULL ) {
        printf("config data load error !!\n");
        exit(0);
    }
	printf("Loaded Multi Marker File\n");

    /* open the graphics window */
	//   argInit( &cparam, 2.0, 0, 2, 1, 0 );
argInit( &cparam, 1.0, 0, 0, 0, 0 );

}

/* cleanup function called when program exits */
static void cleanup(void)
{
	arVideoCapStop();
    arVideoClose();
    argCleanup();
}

static void draw( double trans1[3][4], double trans2[3][4], int mode )
{
    double    gl_para[16];
    GLfloat   mat_ambient[]     = {0.0, 0.0, 1.0, 1.0};
    GLfloat   mat_ambient1[]    = {1.0, 0.0, 0.0, 1.0};
    GLfloat   mat_flash[]       = {0.0, 0.0, 1.0, 1.0};
    GLfloat   mat_flash1[]      = {1.0, 0.0, 0.0, 1.0};
    GLfloat   mat_flash_shiny[] = {50.0};
    GLfloat   mat_flash_shiny1[]= {50.0};
    GLfloat   light_position[]  = {100.0,-200.0,200.0,0.0};
    GLfloat   ambi[]            = {0.1, 0.1, 0.1, 0.1};
    GLfloat   lightZeroColor[]  = {0.9, 0.9, 0.9, 0.1};
    
    argDrawMode3D();
    argDraw3dCamera( 0, 0 );
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    /* load the camera transformation matrix */
    glMatrixMode(GL_MODELVIEW);
    argConvGlpara(trans1, gl_para);
    glLoadMatrixd( gl_para );
    argConvGlpara(trans2, gl_para);
    glMultMatrixd( gl_para );

    if( mode == 0 ) {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0, GL_POSITION, light_position);
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambi);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
        glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash);
        glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny);	
        glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
    }
    else {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0, GL_POSITION, light_position);
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambi);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
        glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash1);
        glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny1);	
        glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient1);
    }
    glMatrixMode(GL_MODELVIEW);
    glTranslatef( 0.0, 0.0, 25.0 );
    if( !arDebug ) glutSolidCube(50.0);
     else          glutWireCube(50.0);
    glDisable( GL_LIGHTING );
    glDisable( GL_DEPTH_TEST );    
    argDrawMode2D();
}

/* draw the paddle - using a stencil buffer */
int  draw_paddle( ARPaddleInfo *paddleInfo )
{
    double  gl_para[16];
    int     i;

    argDrawMode3D();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    argDraw3dCamera( 0, 0 );
    argConvGlpara(paddleInfo->trans, gl_para);
      
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd( gl_para );

	/* draw the paddle graphics */
    glColor3f( 1.0, 0.0, 0.0 );
    glLineWidth(4.0);
    glBegin(GL_LINE_LOOP);
        glVertex2f( -25.0, -25.0 );
        glVertex2f(  25.0, -25.0 );
        glVertex2f(  25.0,  25.0 );
        glVertex2f( -25.0,  25.0 );
    glEnd();

    glColor3f( 0.0, 0.0, 1.0);
    glBegin(GL_LINE_LOOP);
    for( i = 0; i < 16; i++ ) {
        double  x, y;
        x = PADDLE_RADIUS * cos(i*3.141592*2/16);
        y = PADDLE_RADIUS * sin(i*3.141592*2/16);
        glVertex2d( x, y );
    }
    glEnd();
    glBegin(GL_LINE_LOOP);
        glVertex2f( -7.5,    0.0 );
        glVertex2f(  7.5,    0.0 );
        glVertex2f(  7.5, -105.0 );
        glVertex2f( -7.5, -105.0 );
    glEnd();

	/* start drawing the stencil */
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO,GL_ONE);
    
    glColor4f(1,1,1,0);    
	glBegin(GL_POLYGON);
    for( i = 0; i < 16; i++ ) {
        double  x, y;
        x = 40.0 * cos(i*3.141592*2/16);
        y = 40.0 * sin(i*3.141592*2/16);
        glVertex2d( x, y );    
	}
    glEnd();
    glBegin(GL_POLYGON);
        glVertex2f( -7.5,    0.0 );
        glVertex2f(  7.5,    0.0 );
        glVertex2f(  7.5, -105.0 );
        glVertex2f( -7.5, -105.0 );
    glEnd();
    glDisable(GL_BLEND);

    glDisable(GL_DEPTH_TEST);
	argDrawMode2D();
    return 0;
}
