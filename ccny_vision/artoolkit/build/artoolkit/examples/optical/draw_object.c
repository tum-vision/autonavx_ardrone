/*
** draw_object function for AR tracking sample code 
** uses glut functions to draw simple objects
**
*/
#include <stdio.h>
#include <string.h>
#include <math.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <strings.h>
#endif
#ifndef __APPLE__
#include <GL/gl.h>
#include <GL/glut.h>
#else
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#endif
#include <AR/gsub.h>
#include <AR/ar.h>
#include "draw_object.h"
#include "object.h"

/* material properties */
GLfloat mat_specular1[] = {0.2, 0.0, 0.0, 1.0};
GLfloat mat_shininess1[] = {50.0};
GLfloat mat_specular2[] = {0.0, 0.0, 0.2, 1.0};
GLfloat mat_shininess2[] = {25.0};

GLfloat mat_ambient1[] = {1.0, 0.0, 0.0, 1.0};
GLfloat mat_ambient2[] = {0.0, 0.0, 1.0, 1.0};
GLfloat mat_flash_ambient1[] = {0.0, 1.0, 0.0, 1.0};

GLfloat mat_flash1[] = {1.0, 0.0, 0.0, 1.0};
GLfloat mat_flash_shiny1[] = {25.0};
GLfloat mat_flash2[] = {0.0, 0.0, 1.0, 1.0};
GLfloat mat_flash_shiny2[] = {50.0};

static int  draw_object( char *name, double gl_para[16], int dispmode);

/* draw the the AR objects */
int draw( ObjectData_T *object, int objectnum, int dispmode )
{
    int     i;
    double  gl_para[16];
    
    /* calculate the viewing parameters - gl_para */
    for( i = 0; i < objectnum; i++ ) {
        if( object[i].visible == 0 ) continue;

        argConvGlpara(object[i].trans, gl_para);
      
        /* draw the object */
        draw_object( object[i].name, gl_para, dispmode);
    }
    
    return(0);
}

/* draw the user object */
static int  draw_object( char *name, double gl_para[16], int dispmode)
{
    int    i;

    argDrawMode3D();
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    
    /* if in 3D display mode render views
       from both eye - otherwise from one eye */
    for( i = 0; i < 3; i++ ) {
        if( dispmode == 1 ) {
            switch(i) {
                case 0:  argDraw3dLeft();  break;
                case 1:  argDraw3dRight(); break;
                case 2:  argDraw3dCamera( 1, 1 ); break;
            }
        }
        else argDraw3dCamera( 0, 0 );

        /* load the camera transformation matrix */
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixd( gl_para );
        init_lights();


        /* draw the user object here 
           - using the object name to select the object */
        if( strcmp(name, "torus") == 0 ) {
          /* set object color */
          glEnable(GL_LIGHTING);
          glEnable(GL_LIGHT0);
          glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash1);
          glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny1);
          glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient1);

          /* draw a simple torus */
          glMatrixMode(GL_MODELVIEW);
          glTranslatef( 0.0, 0.0, 10.0 );
          glutSolidTorus(10.0, 40.0, 24, 24);
          glDisable( GL_LIGHTING );
        }
        else if( strcmp(name, "sphere") == 0 ) {
          glEnable(GL_LIGHTING);
          glEnable(GL_LIGHT0);
          glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash1);
          glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny1);
          glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient1);
    
          /* draw a sphere */
          glMatrixMode(GL_MODELVIEW);
          glTranslatef( 0.0, 0.0, 40.0 );
          glutSolidSphere(40.0, 24, 24);
          glDisable( GL_LIGHTING );
        }
        else if( strcmp(name, "cube") == 0 ) {
          glEnable(GL_LIGHTING);
          glEnable(GL_LIGHT0);
          glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash2);
          glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny2);
          glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient2);

          glMatrixMode(GL_MODELVIEW);
          glTranslatef( 0.0, 0.0, 25.0 );
          glutSolidCube(50.0);
          glDisable( GL_LIGHTING );
        }
        else if( strcmp(name, "cone") == 0 ) {
          glEnable(GL_LIGHTING);
          glEnable(GL_LIGHT0);
          glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash1);
          glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny1);
          glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient1);

          glMatrixMode(GL_MODELVIEW);
          glutSolidCone(25.0, 100.0, 20, 24);
          glDisable( GL_LIGHTING );
        }
        else {
          printf("unknown object type!!\n");
        }

        if( dispmode == 0 ) break;
        if( arDebug == 0 && i == 1 ) break;
    }

    glDisable( GL_DEPTH_TEST );
    argDrawMode2D();

    return 0;
}

/* initialize the lights in the scene */
void init_lights()
{
    GLfloat light_position[] = {0.0,-200.0,0.0,0.0};
    GLfloat ambi[] = {0.1, 0.1, 0.1, 0.1};
    GLfloat lightZeroColor[]    = {0.9, 0.9, 0.9, 0.1};

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambi);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
}
