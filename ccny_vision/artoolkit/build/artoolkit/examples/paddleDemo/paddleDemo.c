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
#include <AR/video.h>
#include <AR/param.h>
#include <AR/matrix.h>
#include <AR/ar.h>
#include <AR/arMulti.h>

/* set up the video format globals */

#ifdef _WIN32
char    *vconf = "Data\\WDM_camera_flipV.xml";
#else
char    *vconf = "";
#endif

#define TARGET_NUM 5

#include "paddle.h"
#include "command_sub.h"

PaddleItemInfo myPaddleItem;
ItemList myListItem;

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


PaddleItemInfo myPaddleItem;
ItemList myListItem;

static int    draw_paddle( ARPaddleInfo *paddleInfo, PaddleItemInfo *myPaddleItem);
static void   init(void);
static void   cleanup(void);
static void   keyEvent( unsigned char key, int x, int y);
static void   mainLoop(void);
static int    draw_paddle( ARPaddleInfo *paddleInfo, PaddleItemInfo *myPaddleItem);
static int    drawGroundGrid( double trans[3][4], int divisions, float x, float y, float height);
static void   findPaddlePosition(float curPaddlePos[], double card_trans[3][4],double base_trans[3][4]);
static void drawItems(double trans[3][4],ItemList* list);

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
    double			angle;

	err=0.;
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

	for( i = 0; i < marker_num; i++ ) 
		marker_flag[i] = 0;
  
	/* get the paddle position */
	paddleGetTrans(paddleInfo, marker_info, marker_flag, 
				marker_num, &cparam);
	/* draw the 3D models */
	glClearDepth( 1.0 );
	glClear(GL_DEPTH_BUFFER_BIT);


	/* get the translation from the multimarker pattern */
	if( (err=arMultiGetTransMat(marker_info, marker_num, config)) < 0 ) {
        argSwapBuffers();
        return;
    }	
	
	//    printf("err = %f\n", err);
    if(err > 100.0 ) {
        argSwapBuffers();
        return;
    }
	
	//draw a red ground grid
	drawGroundGrid( config->trans, 15, 150.0, 110.0, 0.0);

	/* find the paddle position relative to the base */
	if (paddleInfo->active)
		findPaddlePosition(curPaddlePos,paddleInfo->trans,config->trans);

	/* checking for paddle gesture */
	if( paddleInfo->active) 
	  {
	    int findItem=-1;
	    if (myPaddleItem.item!=-1)
	      {

		  if( check_incline(paddleInfo->trans, config->trans, &angle) ) {
		      myPaddleItem.x += 2.0 * cos(angle);
		      myPaddleItem.y += 2.0 * sin(angle);
		      if( myPaddleItem.x*myPaddleItem.x + 
			  myPaddleItem.y*myPaddleItem.y > 900.0 ) {
			  myPaddleItem.x -= 2.0 * cos(angle);
			  myPaddleItem.y -= 2.0 * sin(angle);
			  myListItem.item[myPaddleItem.item].onpaddle=0;		     
			  myListItem.item[myPaddleItem.item].pos[0]=curPaddlePos[0]; 
			  myListItem.item[myPaddleItem.item].pos[1]=curPaddlePos[1];  
			  myPaddleItem.item = -1;
			}
		  }
	      }
	    else
	      {
		if ((findItem=check_pickup(paddleInfo->trans, config->trans,&myListItem, &angle))!=-1)  {
		    
		    myPaddleItem.item=findItem;
		    myPaddleItem.x =0.0;
		    myPaddleItem.y =0.0;
		    myPaddleItem.angle = 0.0;
		    myListItem.item[myPaddleItem.item].onpaddle=1;
		  }
	      }
	  }

	/* draw the item */
	drawItems(config->trans,&myListItem);

	/* draw the paddle */
	if( paddleInfo->active ){ 
	  draw_paddle(paddleInfo,&myPaddleItem);
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

	/* init items */
	myListItem.itemnum=4;
	myListItem.item[0].pos[0]=0.;myListItem.item[0].pos[1]=0.;myListItem.item[0].onpaddle=0;
	myListItem.item[1].pos[0]=100.;myListItem.item[1].pos[1]=-100.;myListItem.item[1].onpaddle=0;
	myListItem.item[2].pos[0]=200.;myListItem.item[2].pos[1]=0.;myListItem.item[2].onpaddle=0;
	myListItem.item[3].pos[0]=0.;myListItem.item[3].pos[1]=0.;myListItem.item[3].onpaddle=1;	

	/* set up the initial paddle contents */
	myPaddleItem.item = 3;
	myPaddleItem.angle = 0.0;
	myPaddleItem.x = 0.0;
	myPaddleItem.y = 0.0;

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

	//get card position relative to base pattern 
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

	//	printf("Position: %3.2f %3.2f %3.2f\n",x,y,z);

	arMatrixFree( mat_a );
    arMatrixFree( mat_b );
    arMatrixFree( mat_c );
	
}


/* draw the paddle */
int  draw_paddle( ARPaddleInfo *paddleInfo, PaddleItemInfo *paddleItemInfo )
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

	/* draw any objects on the paddle */
	if( (i=paddleItemInfo->item) !=-1) {

        glPushMatrix();
        glTranslatef( paddleItemInfo->x, paddleItemInfo->y, 10.0 );
        glRotatef( paddleItemInfo->angle * 180.0/3.141592, 0.0, 0.0, 1.0 );
	glColor3f(0.0,1.0,0.0);
	glutSolidSphere(10,10,10);
	//	glutSolidTeapot(10.);
        glPopMatrix();
	}

    glDisable(GL_DEPTH_TEST);
	argDrawMode2D();
    return 0;
}

/* draw the items on the ground */
void drawItems(double trans[3][4], ItemList* itlist)
{ 
  int i;
  double        gl_para[16];
  GLfloat   light_position[]  = {100.0,-200.0,200.0,0.0};
  GLfloat   ambi[]            = {0.1, 0.1, 0.1, 0.1};
  GLfloat   lightZeroColor[]  = {0.9, 0.9, 0.9, 0.1};
  GLfloat   mat_ambient[]    = {0.0, 1.0, 0.0, 1.0};
  GLfloat   mat_flash2[]      = {0.0, 1.0, 1.0, 1.0};
  GLfloat   mat_flash_shiny2[]= {50.0};

  argDrawMode3D();
  argDraw3dCamera( 0, 0 );
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  
  glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambi);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash2);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny2);
    glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);

    /* load the camera transformation matrix */
    glMatrixMode(GL_MODELVIEW);
    argConvGlpara(trans, gl_para);
    glLoadMatrixd( gl_para );
    for(i = 0; i < itlist->itemnum; i ++ )
      {
	if (!itlist->item[i].onpaddle)
	  {
	    glPushMatrix();
	    glTranslatef(itlist->item[i].pos[0],itlist->item[i].pos[1], 10.0 );
	     glColor3f(0.0,1.0,0.0);
	     glutSolidSphere(10,10,10);
	    glPopMatrix();
	  }
      }
    glDisable( GL_LIGHTING );
    glDisable( GL_DEPTH_TEST );    
    argDrawMode2D();
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
