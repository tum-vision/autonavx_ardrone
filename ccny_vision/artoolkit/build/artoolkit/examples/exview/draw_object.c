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
#include <AR/matrix.h>
#include "draw_object.h"



static void setup_light( void );
static void draw_camera( double trans[3][4] );
static int  draw_object( char *name, double gl_para[16], int xwin, int ywin );
static void get_trans( double a, double b, double r, double trans[3][4] );
static void draw_axis( void );


void print_string( char *string )
{
  int     i;

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  /* display the position data */
  glTranslatef(-0.95, -0.20, 0.0);

  /* draw a white polygon */
  glColor3f(1.0, 1.0, 1.0);
  glBegin(GL_POLYGON);
    glVertex2f(1.50, 0.10);
    glVertex2f(1.50, -0.12);
    glVertex2f(0.001, -0.12);
    glVertex2f(0.001, 0.10);
  glEnd();

  /* draw red text on the polygon */
  glColor3f(0.75, 0.0, 0.0);
  glRasterPos2i(0.0, 0.0);
  for (i=0; i<(int)strlen(string); i++) {
      if(string[i] != '\n' ) {
          glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, string[i]);
      }
      else {
          glTranslatef(0.0, -0.07, 0.0);
          glRasterPos2i(0.0, 0.0);
      }
  }

  return;
}

int draw_exview( double a, double b, double r, double trans[3][4], int xwin, int ywin )
{
    double      vtrans[3][4];
    double      gl_para[16];
    int         i, j;

    argDrawMode3D();
    argDraw3dCamera( xwin, ywin );
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    get_trans( a, b, r, vtrans );
    argConvGlpara(vtrans, gl_para);
    glMatrixMode(GL_PROJECTION);
    glMultMatrixd( gl_para );

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    setup_light();

    glPushMatrix();
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
          
    for( j = -300; j <= 200; j+= 100 ) {
        for( i = -300; i <= 200; i+= 100 ) {
            glBegin(GL_QUADS);
            glNormal3f( 0.0, 0.0, 1.0 );
            if( (j/100+i/100)%2 ) glColor4f( 0.6, 0.6, 0.6, 1.0 );
             else                 glColor4f( 0.0, 0.3, 0.0, 1.0 );
            glVertex3f( i,     j,     0.0 );
            glVertex3f( i,     j+100, 0.0 );
            glVertex3f( i+100, j+100, 0.0 );
            glVertex3f( i+100, j,     0.0 );
            glEnd();
        }
    }
    draw_axis();

    glColor4f( 0.0, 0.0, 0.5, 1.0 );
    glTranslatef( 0.0, 0.0, 25.0 );
    glutSolidCube(50.0);

    glDisable( GL_LIGHTING );
    glPopMatrix();

    draw_camera( trans );

    glDisable(GL_NORMALIZE);
    glDisable( GL_DEPTH_TEST );
    argDrawMode2D();

    return 0;
}


static void draw_camera( double trans[3][4] )
{
/*
    double      gl_para[16];
*/
    double      btrans[3][4];
    double      quat[4], pos[3], angle;

    arUtilMatInv( trans, btrans );

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
      arUtilMat2QuatPos( btrans, quat, pos );
      angle = -acos(quat[3])*360.0/3.141592;
      glTranslatef( pos[0], pos[1], pos[2] );
      glRotated( angle, quat[0], quat[1], quat[2] );
/*
      argConvGlpara(btrans, gl_para);
      glMultMatrixd( gl_para );
*/

      glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT0);

      glPushMatrix();
        glColor4f( 0.9, 0.9, 0.0, 1.0 );
        glTranslatef( 0.0, 0.0, -10.0 );
        glScalef( 10.0, 10.0, 20.0 );
        glutSolidCube(1.0);
      glPopMatrix();

      glColor4f( 0.9, 0.0, 0.9, 1.0 );
      glPushMatrix();
        glTranslatef( 0.0, 0.0, -40.0 );
        glScalef( 30.0, 30.0, 50.0 );
        glutSolidCube(1.0);
      glPopMatrix();

      glDisable( GL_LIGHTING );
    glPopMatrix();

    return;
}


int draw( char *name, double trans[4][4], int xwin, int ywin )
{
    double      gl_para[16];
    
    argConvGlpara(trans, gl_para);
    draw_object( name, gl_para, xwin, ywin );
    
    return(0);
}

/* draw the user object */
static int  draw_object( char *name, double gl_para[16], int xwin, int ywin )
{
    argDrawMode3D();
    argDraw3dCamera( xwin, ywin );

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    /* load the camera transformation matrix */
    glMatrixMode(GL_PROJECTION);
    glMultMatrixd( gl_para );

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    setup_light();
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    if( strcmp(name, "target") == 0 ) {
        draw_axis();
    }
    else {
        printf("unknown object type!!\n");
    }

    glDisable( GL_LIGHTING );
    glDisable( GL_NORMALIZE );
    glDisable( GL_DEPTH_TEST );
    argDrawMode2D();

    return 0;
}

static void setup_light()
{
    static int  mat_f = 1;
    GLfloat     mat_amb_diff[]  = {0.9, 0.9, 0.0, 1.0};
    GLfloat     mat_specular[]  = {0.5, 0.5, 0.5, 1.0};
    GLfloat     mat_shininess[] = {10.0};
    GLfloat     light_ambient[] = { 0.01, 0.01, 0.01, 1.0 };
    GLfloat     light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat     light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat     light_position[] = { 100.0, 300.0, 700.0, 1.0 };

    if( mat_f ) {
      glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_amb_diff);
      glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
      glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);	
      glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
      glEnable(GL_COLOR_MATERIAL);
      mat_f = 0;
    }

    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
}

static void get_trans( double a, double b, double r, double trans[3][4] )
{
    ARMat   *mat;
    double  sa, ca, sb, cb;
    double  x, y, z;
    int     i, j;

    sa = sin(a*3.141592/180.0);
    ca = cos(a*3.141592/180.0);
    sb = sin(b*3.141592/180.0);
    cb = cos(b*3.141592/180.0);

    x = 0.0;
    y = -r * cb;
    z = -r * sb;

    mat = arMatrixAlloc( 4, 4 );

    mat->m[0*4+0] =  ca;
    mat->m[0*4+1] =  sa*sb;
    mat->m[0*4+2] =  sa*cb;
    mat->m[1*4+0] = -sa;
    mat->m[1*4+1] =  ca*sb;
    mat->m[1*4+2] =  ca*cb;
    mat->m[2*4+0] =  0;
    mat->m[2*4+1] = -cb;
    mat->m[2*4+2] =  sb;
    mat->m[0*4+3] =  x*ca + y*sa;
    mat->m[1*4+3] = -x*sa + y*ca;
    mat->m[2*4+3] = z;
    mat->m[3*4+0] = 0;
    mat->m[3*4+1] = 0;
    mat->m[3*4+2] = 0;
    mat->m[3*4+3] = 1;

    arMatrixSelfInv( mat );
    for( j = 0; j < 3; j++ ) {
        for( i = 0; i < 4; i++ ) {
            trans[j][i] = mat->m[j*4+i];
        }
    }
    arMatrixFree( mat );

    return;
}

static void draw_axis( void )
{
    glPushMatrix();
        glRotatef( 90.0, 0.0, 1.0, 0.0 );
        glColor4f( 1.0, 0.0, 0.0, 1.0 );
        glutSolidCone(5.0, 100.0, 20, 24);
    glPopMatrix();

    glPushMatrix();
        glRotatef( -90.0, 1.0, 0.0, 0.0 );
        glColor4f( 0.0, 1.0, 0.0, 1.0 );
        glutSolidCone(5.0, 100.0, 20, 24);
    glPopMatrix();

    glPushMatrix();
        glRotatef( 00.0, 0.0, 0.0, 1.0 );
        glColor4f( 0.0, 0.0, 1.0, 1.0 );
        glutSolidCone(5.0, 100.0, 20, 24);
    glPopMatrix();

    return;
}
