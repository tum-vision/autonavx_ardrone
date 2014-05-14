/*
 * 
 * This file is part of ARToolKit.
 * 
 * ARToolKit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * ARToolKit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with ARToolKit; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

// ============================================================================
//	Includes
// ============================================================================

#ifdef _WIN32
#  include <windows.h>
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __APPLE__
#  include <GL/glut.h>
#  ifdef GL_VERSION_1_2
#    include <GL/glext.h>
#  endif
#else
#  include <GLUT/glut.h>
#  include <OpenGL/glext.h>
#endif
#include <AR/config.h>
#include <AR/ar.h>
#include <AR/video.h>
#include <AR/param.h>
#include <AR/gsub_lite.h>
#include "calib_cparam.h"

// ============================================================================
//	Constants
// ============================================================================

#define CALIB_CPARAM_DEBUG 0

// ============================================================================
//	Global variables
// ============================================================================

/* set up the video format globals */

#if defined(__sgi)
char            *vconf = "-size=FULL";
#elif defined(__linux)
#  if defined(AR_INPUT_GSTREAMER)
char 			*vconf = "";
#  elif defined(AR_INPUT_V4L)
char            *vconf = "-width=640 -height=480";
#  elif defined(AR_INPUT_1394CAM)
char            *vconf = "-mode=640x480_YUV411";
#  elif defined(AR_INPUT_DV)
char            *vconf = "";
#  endif
#elif defined(_WIN32)
char			*vconf = "Data\\WDM_camera_flipV.xml";
#elif defined(__APPLE__)
char			*vconf = "-width=640 -height=480";
#else
char			*vconf = "";
#endif

static ARUint8		*gARTImage = NULL;
static ARParam		gARTCparam; // Dummy parameter, to supply to gsub_lite.
static ARGL_CONTEXT_SETTINGS_REF gArglSettings = NULL;

static int             gWin;
static int             gXsize = 0;
static int             gYsize = 0;
int             refresh;

static unsigned char   *gSaveARTImage = NULL;
static ARGL_CONTEXT_SETTINGS_REF gSaveArglSettings = NULL;

static ARParam         param;
static double          dist_factor[4];

int             line_num;
int             loop_num;
int             line_mode[LINE_MAX];
double          inter_coord[LOOP_MAX][LINE_MAX][LINE_MAX][3];
double          line_info[LOOP_MAX][LINE_MAX][4];

int             mode;
int             line_num_current;
int             loop_num_current;
double          theta;
double          radius;
double          gStartX, gStartY, gEndX, gEndY;

// ============================================================================
//	Functions
// ============================================================================

int             main(int argc, char *argv[]);
static int      init(int argc, char *argv[]);
static void		Keyboard(unsigned char key, int x, int y);
static void		Special(int key, int x, int y);
static void		Mouse(int button, int state, int x, int y);
/* static void     Quit(void); */
static void     mainLoop(void);
static void     Visibility(int visible);
static void     Reshape(int w, int h);
static void     Display(void);
static void		drawPrevLine(void);
static void		drawNextLine(void);
static void		draw_warp_line(double sx, double ex, double sy, double ey);
static void		getCpara(void);
static void		intersection( double line1[4], double line2[4], double *screen_coord );
static void     cleanup(void);


int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
    if (!init(argc, argv)) exit(-1);

    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(gXsize, gYsize);
    glutInitWindowPosition(100,100);
    gWin = glutCreateWindow("Camera calibration");
	
	// Setup argl library for current context.
	// Turn off distortion compensation.. we are calibrating.
	if ((gArglSettings = arglSetupForCurrentContext()) == NULL) {
		fprintf(stderr, "main(): arglSetupForCurrentContext() returned error.\n");
		exit(-1);
	}
	arglDistortionCompensationSet(gArglSettings, FALSE);

	// Make a dummy camera parameter to supply when calling arglDispImage().
	gARTCparam.xsize = gXsize;
	gARTCparam.ysize = gYsize;
	
	// Setup a space to save our captured image.
	if ((gSaveArglSettings = arglSetupForCurrentContext()) == NULL) {
		fprintf(stderr, "grabImage(): arglSetupForCurrentContext() returned error.\n");
		exit(-1);
	}
	arglDistortionCompensationSet(gSaveArglSettings, FALSE);
	arMalloc(gSaveARTImage, unsigned char, gXsize*gYsize*AR_PIX_SIZE_DEFAULT);

	// Register GLUT event-handling callbacks.
	// NB: mainLoop() is registered by Visibility.
    glutDisplayFunc(Display);
	glutReshapeFunc(Reshape);
	glutVisibilityFunc(Visibility);
	glutKeyboardFunc(Keyboard);
	glutSpecialFunc(Special);
    glutMouseFunc(Mouse);
	
	if (arVideoCapStart() != 0) {
    	fprintf(stderr, "init(): Unable to begin camera data capture.\n");
		return (FALSE);		
	}
	mode = 0;
    line_num_current = 0;
    loop_num_current = 0;

    glutMainLoop();
	return (0);
}

static int init(int argc, char *argv[])
{
    printf("Input center coordinates: X = ");
    scanf("%lf", &dist_factor[0]);
    while (getchar() != '\n');
    printf("                        : Y = ");
    scanf("%lf", &dist_factor[1]);
    while (getchar() != '\n');
    printf("Input distortion ratio: F = ");
    scanf("%lf", &dist_factor[2]);
    while (getchar() != '\n');
    printf("Input size adjustment factor: S = ");
    scanf("%lf", &dist_factor[3]);
    while (getchar() != '\n');

    initLineModel(&line_num, &loop_num, line_mode, inter_coord);

	// Open the video path.
    if (arVideoOpen(vconf) < 0) {
    	fprintf(stderr, "init(): Unable to open connection to camera.\n");
    	return (FALSE);
	}

	// Find the size of the window.
    if (arVideoInqSize(&gXsize, &gYsize) < 0) return (FALSE);
    fprintf(stdout, "Camera image size (x,y) = (%d,%d)\n", gXsize, gYsize);
	
	// Allocate space for a save image.
	arMalloc(gSaveARTImage, unsigned char, gXsize * gYsize * AR_PIX_SIZE_DEFAULT);
	
    param.xsize = gXsize;
    param.ysize = gYsize;
    param.dist_factor[0] = dist_factor[0];
    param.dist_factor[1] = dist_factor[1];
    param.dist_factor[2] = dist_factor[2];
    param.dist_factor[3] = dist_factor[3];

	return (TRUE);
}

static void thetaRadiusSet(void) {
	if (line_num_current == 0) {
		if (line_mode[line_num_current] == L_HORIZONTAL) {
			theta  = 90;
			radius = gYsize/2;
		} else {
			theta  = 0;
			radius = gXsize/2;
		}
	} else {
		if (line_mode[line_num_current] == L_HORIZONTAL) {
			if (line_mode[line_num_current] == line_mode[line_num_current-1]) {
				radius += 10.0;
			} else {
				theta  = 90;
				radius = gYsize/2;
			}
		} else {
			if (line_mode[line_num_current] == line_mode[line_num_current-1]) {
				radius += 10.0;
			} else {
				theta  = 0;
				radius = gXsize/2;
			}
		}
	}
}

static void eventCancel(void) {
	if (mode == 0) {
		// Cancel from mode 0 should quit program.
		arVideoCapStop();
		cleanup();
		exit(0);
	} else if (mode == 1) {
		if (line_num_current == 0) {
			// Cancel from mode 1, first line, should go back to mode 0.
			arVideoCapStart();
			mode = 0;			
		} else {
			// Cancel from mode 1, non-first line, should go back to first line.
			line_num_current = 0;
			thetaRadiusSet();
			glutPostRedisplay();
		}
	}
}

static void Mouse(int button, int state, int x, int y)
{
	ARUint8 *image;
	
	if (state == GLUT_DOWN) {
		if (button == GLUT_RIGHT_BUTTON) {
			eventCancel();
		} else if (button == GLUT_LEFT_BUTTON && mode == 0) {
			// Processing a new image.
			// Copy an image to saved image buffer.
			do {
				image = arVideoGetImage();
			} while (image == NULL);
			memcpy(gSaveARTImage, image, gXsize*gYsize*AR_PIX_SIZE_DEFAULT);
			printf("Grabbed image.\n");
			arVideoCapStop();
			mode = 1;
			line_num_current = 0;
			thetaRadiusSet();
			glutPostRedisplay();
		}
	}
}

static void Keyboard(unsigned char key, int x, int y)
{
    ARParam  iparam;
    double   trans[3][4];
    char     name[256];
    int      k = 1;

    if( mode == 0 ) return;

    switch( key ) {
        case 0x1B:
            gStartX = -1.0;
            gStartY = -1.0;
            gEndX = -1.0;
            gEndY = -1.0;
			// fall through..
        case 0x0D:
            line_info[loop_num_current][line_num_current][0] = gStartX;
            line_info[loop_num_current][line_num_current][1] = gStartY;
            line_info[loop_num_current][line_num_current][2] = gEndX;
            line_info[loop_num_current][line_num_current][3] = gEndY;
            line_num_current++;
			if (line_num_current < line_num) {
				thetaRadiusSet();
            } else {
				// Completed placements for this image.
                loop_num_current++;
                if (loop_num_current < loop_num) {
                    arVideoCapStart();
                    mode = 0;
                } else {
					// All done. Calculate parameter, save it, and exit.
					getCpara();
					arParamDecomp( &param, &iparam, trans );
					arParamDisp( &iparam );
					
					printf("Input filename: ");
					scanf("%s", name);
					arParamSave( name, 1, &iparam );
					cleanup();
					exit(0);
				}
            }
            break;
        default:
            k = 0;
            break;
    }
    if (k) glutPostRedisplay();
}


static void Special(int key, int x, int y)
{
    double   mx, my;
    int      k = 1;

    if (mode == 0) return;

    if (line_mode[line_num_current] == L_HORIZONTAL) {
        switch (key) {
            case GLUT_KEY_UP:
                radius -= 1.0;
                break;
            case GLUT_KEY_DOWN:
                radius += 1.0;
                break;
            case GLUT_KEY_LEFT:
                mx = (gStartX + gEndX)/ 2.0;
                my = (gStartY + gEndY)/ 2.0;
                theta -= 0.25;
                radius = cos( (double)(theta*3.141592/180.0) ) * mx
                       + sin( (double)(theta*3.141592/180.0) ) * my;
                break;
            case GLUT_KEY_RIGHT:
                mx = (gStartX + gEndX)/ 2.0;
                my = (gStartY + gEndY)/ 2.0;
                theta += 0.25;
                radius = cos( (double)(theta*3.141592/180.0) ) * mx
                       + sin( (double)(theta*3.141592/180.0) ) * my;
                break;
            default:
                k = 0;
                break;
        }
    } else {
        switch (key) {
            case GLUT_KEY_UP:
                mx = (gStartX + gEndX)/ 2.0;
                my = (gStartY + gEndY)/ 2.0;
                theta += 0.25;
                radius = cos( (double)(theta*3.141592/180.0) ) * mx
                       + sin( (double)(theta*3.141592/180.0) ) * my;
                break;
            case GLUT_KEY_DOWN:
                mx = (gStartX + gEndX)/ 2.0;
                my = (gStartY + gEndY)/ 2.0;
                theta -= 0.25;
                radius = cos( (double)(theta*3.141592/180.0) ) * mx
                       + sin( (double)(theta*3.141592/180.0) ) * my;
                break;
            case GLUT_KEY_LEFT:
                radius -= 1.0;
                break;
            case GLUT_KEY_RIGHT:
                radius += 1.0;
                break;
            default:
                k = 0;
                break;
        }
    }

    if (k) glutPostRedisplay();
}

static void cleanup(void)
{
	if (gSaveArglSettings) arglCleanup(gSaveArglSettings);
	if (gArglSettings) arglCleanup(gArglSettings);
	if (gWin) glutDestroyWindow(gWin);
	arVideoClose();
}

static void mainLoop(void)
{
	static int ms_prev;
	int ms;
	float s_elapsed;
	ARUint8 *image;
	
	// Find out how long since mainLoop() last ran.
	ms = glutGet(GLUT_ELAPSED_TIME);
	s_elapsed = (float)(ms - ms_prev) * 0.001;
	if (s_elapsed < 0.01f) return; // Don't update more often than 100 Hz.
	ms_prev = ms;
	
	// Grab a video frame.
	if (mode == 0) {
		if ((image = arVideoGetImage()) != NULL) {
			gARTImage = image;
			// Tell GLUT the display has changed.
			glutPostRedisplay();
		}	
	}
}

//
//	This function is called on events when the visibility of the
//	GLUT window changes (including when it first becomes visible).
//
static void Visibility(int visible)
{
	if (visible == GLUT_VISIBLE) {
		glutIdleFunc(mainLoop);
	} else {
		glutIdleFunc(NULL);
	}
}

//
//	This function is called when the
//	GLUT window is resized.
//
static void Reshape(int w, int h)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	// Call through to anyone else who needs to know about window sizing here.
}

static void beginOrtho2D(int xsize, int ysize) {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, xsize, 0.0, ysize);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();	
}

static void endOrtho2D(void) {
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

static void Display(void)
{
	// Select correct buffer for this context.
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	beginOrtho2D(gXsize, gYsize);
	
    if (mode == 0) {
		
		arglDispImage(gARTImage, &gARTCparam, 1.0, gArglSettings);	// zoom = 1.0.
		arVideoCapNext();
		gARTImage = NULL;
		
    } else if (mode == 1) {

		arglDispImage(gSaveARTImage, &gARTCparam, 1.0, gSaveArglSettings);

        drawPrevLine();
        drawNextLine();
	}
	
	endOrtho2D();
    glutSwapBuffers();
}

static void drawPrevLine(void)
{
    int     i;

    glColor3f(0.0,0.0,1.0);
    for (i = 0; i < line_num_current; i++) {
        if (line_info[loop_num_current][i][0] == -1.0) continue;
        draw_warp_line(line_info[loop_num_current][i][0], line_info[loop_num_current][i][2],
                       line_info[loop_num_current][i][1], line_info[loop_num_current][i][3]);
    }
}

static void drawNextLine(void)
{
    double sin_theta, cos_theta;
    double x1, x2;
    double y1, y2;

    sin_theta = sin( (double)(theta*3.141592/180.0) );
    cos_theta = cos( (double)(theta*3.141592/180.0) );

    if (cos_theta != 0) {
        x1 = radius / cos_theta;
        x2 = (radius - (gYsize-1)*sin_theta) / cos_theta;
    } else {
        x1 = x2 = -1.0;
    }

    if (sin_theta != 0) {
        y1 = radius / sin_theta;
        y2 = (radius - (gXsize-1)*cos_theta) / sin_theta;
    } else {
        y1 = y2 = -1.0;
    }

    gEndY = -1;
    if (x1 >= 0 && x1 <= gXsize-1) {
         gStartX = x1;
         gStartY = 0;
         if (x2 >= 0 && x2 <= gXsize-1) {
             gEndX = x2;
             gEndY = gYsize-1;
         } else if (y1 >= 0 && y1 <= gYsize-1) {
             gEndX = 0;
             gEndY = y1;
         } else if (y2 >= 0 && y2 <= gYsize-1) {
             gEndX = gXsize-1;
             gEndY = y2;
         } else {
			 printf("???\n");
		 }
    } else if (y1 >= 0 && y1 <= gYsize-1) {
         gStartX = 0;
         gStartY = y1;
         if (x2 >= 0 && x2 <= gXsize-1) {
             gEndX = x2;
             gEndY = gYsize-1;
         } else if (y2 >= 0 && y2 <= gYsize-1) {
             gEndX = gXsize-1;
             gEndY = y2;
         } else {
			 printf("???\n");
		 }
    } else if (x2 >= 0 && x2 <= gXsize-1) {
         gStartX = x2;
         gStartY = gYsize-1;
         if (y2 >= 0 && y2 <= gYsize-1) {
             gEndX = gXsize-1;
             gEndY = y2;
         } else {
			 printf("???\n");
		 }
    }

    glColor3f(1.0,1.0,1.0);
    if (gEndY != -1) {
        draw_warp_line(gStartX, gEndX, gStartY, gEndY);
    }
}

static void draw_warp_line(double sx, double ex, double sy, double ey)
{
    double   a, b, c;
    double   x, y;
    double   x1, y1;
    int      i;

    a = ey - sy;
    b = sx - ex;
    c = sy*ex - sx*ey;

    glLineWidth(1.0f);
    glBegin(GL_LINE_STRIP);
    if (a*a >= b*b) {
        for (i = -20; i <= gYsize+20; i+=10) {
            x = -(b*i + c)/a;
            y = i;

            arParamIdeal2Observ(dist_factor, x, y, &x1, &y1);
            glVertex2f(x1, gYsize-1-y1);
        }
    } else {
        for (i = -20; i <= gXsize+20; i+=10) {
            x = i;
            y = -(a*i + c)/b;

            arParamIdeal2Observ(dist_factor, x, y, &x1, &y1);
            glVertex2f(x1, gYsize-1-y1);
        }
    }
    glEnd();
}



static void getCpara(void)
{
    double  *world_coord;
    double  *screen_coord;
    int     point_num;
    int     i, j, k;

    point_num = 0;
    for (k = 0; k < loop_num; k++) {
        for (i = 0; i < line_num; i++) {
            for (j = 0; j < line_num; j++) {
                if (inter_coord[k][i][j][0] != -10000.0) point_num++;
            }
        }
    }
	arMalloc(world_coord, double, point_num * 3);
	arMalloc(screen_coord, double, point_num * 2);

    point_num = 0;
    for (k = 0; k < loop_num; k++) {
        for (i = 0; i < line_num; i++) {
            if (line_info[k][i][0] == -1.0) continue;
            for (j = 0; j < line_num; j++) {
                if (line_info[k][j][0] == -1.0) continue;
                if (inter_coord[k][i][j][0] == -10000.0) continue;

                world_coord[point_num*3+0] = inter_coord[k][i][j][0];
                world_coord[point_num*3+1] = inter_coord[k][i][j][1];
                world_coord[point_num*3+2] = inter_coord[k][i][j][2];
                intersection(line_info[k][i], line_info[k][j],
                             &(screen_coord[point_num*2]));

                point_num++;
            }
        }
    }
    printf("point_num = %d\n", point_num);
    if (arParamGet((double (*)[3])world_coord, (double (*)[2])screen_coord, point_num, param.mat) < 0) {
        printf("ddd error!!\n");
        exit(0);
    }

    free(world_coord);
    free(screen_coord);
}

static void intersection(double line1[4], double line2[4], double *screen_coord)
{
    double a, b, c, d, e, f, g;

    a = line1[1] - line1[3];
    b = line1[2] - line1[0];
    c = line1[0] * a + line1[1] * b;
    d = line2[1] - line2[3];
    e = line2[2] - line2[0];
    f = line2[0] * d + line2[1] * e;

    g = a*e - b*d;
    if (g == 0.0) { printf("???\n"); exit(0); }

    screen_coord[0] = (c * e - b * f) / g;
    screen_coord[1] = (a * f - c * d) / g;
}
