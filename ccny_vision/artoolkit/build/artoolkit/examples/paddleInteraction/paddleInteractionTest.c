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
#include <AR/matrix.h>
#include <AR/ar.h>
#include <AR/video.h>
#include <AR/arMulti.h>

#define TOUCHED 1
#define NOT_TOUCHED -1
#define TARGET_NUM 5

#include "paddle.h"

/* set up the video format globals */

#ifdef _WIN32
char			*vconf = "Data\\WDM_camera_flipV.xml";
#else
char			*vconf = "";
#endif

/* define a target struct for the objects that are to be touched */
typedef struct {
	int id;
	int state;
	float     pos[3];
} targetInfo;

targetInfo myTarget[TARGET_NUM];

int  draw_paddle( ARPaddleInfo *paddleInfo );

int             xsize, ysize;
int             thresh = 100;
int             count = 0;

char           *cparam_name    = "Data/camera_para.dat";
ARParam         cparam;

char                *config_name = "Data/multi/marker.dat";
ARMultiMarkerInfoT  *config;

/* paddle information  */
int              marker_flag[AR_SQUARE_MAX];
ARPaddleInfo   *paddleInfo;
char           *paddle_name    = "Data/paddle_data";  

GLfloat   light_position[]  = {100.0,-200.0,200.0,0.0};
GLfloat   ambi[]            = {0.1, 0.1, 0.1, 0.1};
GLfloat   lightZeroColor[]  = {0.9, 0.9, 0.9, 0.1};

static void   init(void);
static void   cleanup(void);
static void   keyEvent( unsigned char key, int x, int y);
static void   mainLoop(void);
static void draw( targetInfo myTarget, double BaseTrans[3][4]);
int drawGroundGrid( double trans[3][4], int divisions, float x, float y, float height);
static int checkCollision(float Pos1[],float Pos2[], float range);
static void	  findPaddlePosition(float curPaddlePos[], double card_trans[3][4],double base_trans[3][4]);


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

/* keyboard events */
static void   keyEvent( unsigned char key, int x, int y)   
{
    /* quit if the ESC key is pressed */
    if( key == 0x1b ) {
        printf("*** %f (frame/sec)\n", (double)count/arUtilTimer());
        cleanup();
        exit(0);
    }

	/* change the threshold value when 't' key pressed */
    if( key == 't' ) {
        printf("Enter new threshold value (default = 100): ");
        scanf("%d",&thresh); while( getchar()!='\n' );
        printf("\n");
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
	float curPaddlePos[3];
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

	for( i = 0; i < marker_num; i++ ) marker_flag[i] = 0;
  
	/* get the paddle position */
	paddleGetTrans(paddleInfo, marker_info, marker_flag, 
				marker_num, &cparam);
	
	/* draw the 3D models */
	glClearDepth( 1.0 );
	glClear(GL_DEPTH_BUFFER_BIT);

	/* draw the paddle, base and menu */
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

	//draw a red ground grid
	drawGroundGrid( config->trans, 20, 150.0f, 105.0f, 0.0f);

	/* find the paddle position relative to the base */
	findPaddlePosition(curPaddlePos, paddleInfo->trans, config->trans);

	/* check for collisions with targets */
	for(i=0;i<TARGET_NUM;i++){
		myTarget[i].state = NOT_TOUCHED;
		if(checkCollision(curPaddlePos, myTarget[i].pos, 20.0f))
		  {
			myTarget[i].state = TOUCHED;
			fprintf(stderr,"touched !!\n");
		  }
	}

	/* draw the targets */
	for(i=0;i<TARGET_NUM;i++){
		draw(myTarget[i],config->trans);
	}

	argSwapBuffers();
}

static void init( void )
{
	ARParam  wparam;
	int i;

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

	/* initialize the targets */
	for (i=0;i<TARGET_NUM;i++){
		myTarget[i].pos[0] = 50.0*i;
		myTarget[i].pos[1] = -50.0*i;
		myTarget[i].pos[2] = 50.0*i;
		myTarget[i].state = NOT_TOUCHED;
	}

    /* open the graphics window */
    argInit( &cparam, 1.0, 0, 0, 0, 0 );
}

/* cleanup function called when program exits */
static void cleanup(void)
{
  arVideoCapStop();
  arVideoClose();
  argCleanup();
}

/* find the position of the paddle card relative to the base and set the dropped blob position to this */
static void	  findPaddlePosition(float curPaddlePos[], double card_trans[3][4],double base_trans[3][4])
{
	int i,j;

	ARMat   *mat_a,  *mat_b, *mat_c;
    double  x, y, z;

	/*get card position relative to base pattern */
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
    arMatrixSelfInv( mat_b );
    arMatrixMul( mat_c, mat_b, mat_a );

	//x,y,z is card position relative to base pattern
    x = mat_c->m[0*4+3];
    y = mat_c->m[1*4+3];
    z = mat_c->m[2*4+3];
    
    curPaddlePos[0] = x;
    curPaddlePos[1] = y;
    curPaddlePos[2] = z;
    
    printf("Position: %3.2f %3.2f %3.2f\n",x,-y,z);
    
    arMatrixFree( mat_a );
    arMatrixFree( mat_b );
    arMatrixFree( mat_c );
}

/* check collision between two points */
static int checkCollision(float pos1[],float pos2[], float range)
{
	float xdist,ydist,zdist,dist;

	xdist = pos1[0]-pos2[0];
	ydist = pos1[1]-pos2[1];
	zdist = pos1[2]-pos2[2];

	dist = xdist*xdist+ydist*ydist+zdist*zdist;
	//	fprintf(stderr,"dist=%f\n",dist);
	if(dist<(range*range))
		return 1;
	else 
		return 0;
}

/* draw the targets */
static void draw( targetInfo myTarget, double BaseTrans[3][4])
{
    double    gl_para[16];
    GLfloat   light_position[]  = {100.0,-200.0,200.0,0.0};
    GLfloat   ambi[]            = {0.1, 0.1, 0.1, 0.1};
    GLfloat   lightZeroColor[]  = {0.9, 0.9, 0.9, 0.1};
    GLfloat   mat_ambient2[]    = {0.0, 0.0, 1.0, 1.0};
	GLfloat   mat_ambient[]    = {1.0, 0.0, 0.0, 1.0};
    GLfloat   mat_flash2[]      = {0.0, 1.0, 1.0, 1.0};
    GLfloat   mat_flash_shiny2[]= {50.0};
	
    argDrawMode3D();
    argDraw3dCamera( 0, 0 );
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    /* load the camera transformation matrix */
    glMatrixMode(GL_MODELVIEW);
    argConvGlpara(BaseTrans, gl_para);
    glLoadMatrixd( gl_para );

	/* set the lighting and the materials */		
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambi);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash2);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny2);
	if(myTarget.state == TOUCHED)
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	else 
        glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient2);
    
    glMatrixMode(GL_MODELVIEW);
    glTranslatef( myTarget.pos[0], myTarget.pos[1], myTarget.pos[2] );
    glutSolidCube(40.0);

	if(myTarget.state == TOUCHED){
		glColor3f(1.0,1.0,1.0);	
		glLineWidth(6.0);
		glutWireCube(60.0);
		glLineWidth(1.0);
	}
    
	glDisable( GL_LIGHTING );
    glDisable( GL_DEPTH_TEST );    
    argDrawMode2D();
}

/* draw the paddle */
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

/*************************************************************************************
**
** drawGroundGrid - draws a ground plane
**
***************************************************************************************/
int drawGroundGrid( double trans[3][4], int divisions, float x, float y, float height)
{
	double        gl_para[16];
    int           i;
	float x0,x1,y0,y1;
	float deltaX, deltaY;

    argDrawMode3D();
    argDraw3dCamera( 0, 0 );
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    /* load the camera transformation matrix */
    glMatrixMode(GL_MODELVIEW); 
    argConvGlpara(trans, gl_para);
    glLoadMatrixd( gl_para );
    glTranslatef(x/2.,-y/2.,0.);
    //draw the grid
    glColor3f(1,0,0);
	glLineWidth(6.0);
	glBegin(GL_LINE_LOOP);
	glVertex3f( -x, y, height );
	glVertex3f(  x, y, height );  
	glVertex3f(  x, -y, height );
	glVertex3f( -x, -y, height );
	glEnd();
	glLineWidth(3.0);
	
	//draw a grid of lines
	//X direction
	x0 = -x; x1 = -x;
	y0 = -y; y1 = y;
	deltaX = (2*x)/divisions;

	for(i=0;i<divisions;i++){
		x0 = x0 + deltaX;
		glBegin(GL_LINES);
		glVertex3f(x0,y0,height);
		glVertex3f(x0,y1,height);
		glEnd();
	}

	x0 = -x; x1 = x;
	deltaY = (2*y)/divisions;

	for(i=0;i<divisions;i++){
		y0 = y0 + deltaY;
		glBegin(GL_LINES);
		glVertex3f(x0,y0,height);
		glVertex3f(x1,y0,height);
		glEnd();
	}

	glLineWidth(1.0);

	glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambi);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);

	glDisable( GL_LIGHTING );
    glDisable( GL_DEPTH_TEST );    
    argDrawMode2D();
    return 0;
}
