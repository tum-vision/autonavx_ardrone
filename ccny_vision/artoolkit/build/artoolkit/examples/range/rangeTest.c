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

char           *patt_name      = "Data/patt.hiro";
int             patt_id;

int             marker_width     = 80.0;
double          marker_center[2] = {0.0, 0.0};
double          marker_trans[3][4];

int             xsize, ysize;
int				thresh = 100;
int             count = 0;

/* set up the video format globals */

#ifdef _WIN32
char			*vconf = "Data\\WDM_camera_flipV.xml";
#else
char			*vconf = "";
#endif

char           *cparam_name    = "Data/camera_para.dat";
ARParam         cparam;

static void   init(void);
static void   cleanup(void);
static void   keyEvent( unsigned char key, int x, int y);
static void   mainLoop(void);
static void   draw(double marker_trans[3][4],double range);

int main(int argc, char **argv)
{
	//initialize applications
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
}

/* main loop */
static void mainLoop(void)
{
    ARUint8         *dataPtr;
    ARMarkerInfo    *marker_info;
    int             marker_num;
    int             i,k;
	float			Xpos, Ypos, Zpos;
	double			range;

    /* grab a video frame */
    if( (dataPtr = (ARUint8 *)arVideoGetImage()) == NULL ) {
        arUtilSleep(2);
        return;
    }
	
    if( count == 0 ) arUtilTimerReset();  
    count++;

	/*draw the video*/
    argDrawMode2D();
    argDispImage( dataPtr, 0,0 );

	glColor3f( 1.0, 0.0, 0.0 );
	glLineWidth(6.0);

	/* detect the markers in the video frame */ 
	if(arDetectMarker(dataPtr, thresh, 
		&marker_info, &marker_num) < 0 ) {
		cleanup(); 
		exit(0);
	}

	arVideoCapNext();

	for( i = 0; i < marker_num; i++ ) {
		argDrawSquare(marker_info[i].vertex,0,0);
	}

	/* check for known patterns */
    k = -1;
    for( i = 0; i < marker_num; i++ ) {
        if( marker_info[i].id  == patt_id) {

			/* you've found a pattern */
			printf("Found pattern: %d ",patt_id);
			glColor3f( 0.0, 1.0, 0.0 );
            argDrawSquare(marker_info[i].vertex,0,0);

			if( k == -1 ) k = i;
            else /* make sure you have the best pattern (highest confidence factor) */
				if( marker_info[k].cf < marker_info[i].cf ) k = i;
        }
    }
	if( k == -1 ) {
        argSwapBuffers();
        return;
    }

    /* get the transformation between the marker and the real camera */
    arGetTransMat(&marker_info[k], marker_center, marker_width, marker_trans);

	/* find the range */
	Xpos = marker_trans[0][3];
	Ypos = marker_trans[1][3];
	Zpos = marker_trans[2][3];
	range = sqrt(Xpos*Xpos+Ypos*Ypos+Zpos*Zpos);

	printf(" X: %3.2f Y: %3.2f Z: %3.2f Range: %3.2f \n",Xpos,Ypos,Zpos,range);

	/* draw the AR graphics */
    draw(marker_trans,range);

	/*swap the graphics buffers*/
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

	/* load pattern file */
	if(patt_id=arLoadPatt (patt_name) < 0)
	{
		printf ("Pattern file load error !! \n"); 
		exit(0); 
	} 

    /* open the graphics window */
    argInit( &cparam, 2.0, 0, 0, 0, 0 );
}

/* cleanup function called when program exits */
static void cleanup(void)
{
	arVideoCapStop();
    arVideoClose();
    argCleanup();
}

static void draw(double marker_trans[3][4], double range)
{
    double    gl_para[16];
	double	  myScale;

    GLfloat   mat_ambient[]     = {0.0, 0.0, 1.0, 1.0};
    GLfloat   mat_flash[]       = {0.0, 0.0, 1.0, 1.0};
    GLfloat   mat_flash_shiny[] = {50.0};
    GLfloat   light_position[]  = {100.0,-200.0,200.0,0.0};
    GLfloat   ambi[]            = {0.1, 0.1, 0.1, 0.1};
    GLfloat   lightZeroColor[]  = {0.9, 0.9, 0.9, 0.1};
 
	/* set the openGL projection mode */
    argDrawMode3D();
    argDraw3dCamera( 0, 0 );
    glClearDepth( 1.0 );
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    /* load the camera transformation matrix */
    argConvGlpara(marker_trans, gl_para);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixd( gl_para );

	/* set the material */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambi);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny);	
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	
	/* draw the object */	
	myScale = range/200.0;

	glScalef(myScale,myScale,myScale);
	glRotatef( 90, 1.0, 0.0, 0.0 );
	glTranslatef( 0.0, 17.5, 0.0 );

	glutSolidTeapot(30);
    
    glDisable( GL_LIGHTING );
    glDisable( GL_DEPTH_TEST );
}
