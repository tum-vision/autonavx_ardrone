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
#include <AR/video.h>
#include <AR/param.h>
#include <AR/matrix.h>
#include <AR/gsub_lite.h>
#include "calib_dist.h"

// ============================================================================
//	Constants
// ============================================================================

#define CALIB_DIST_DEBUG 0

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
static int             gThresh = THRESH;
static unsigned char   *gClipImage = NULL;

static CALIB_PATT_T    gPatt;          
static double          dist_factor[4];

static int             point_num;
static int             gDragStartX = -1, gDragStartY = -1, gDragEndX = -1, gDragEndY = -1;	// x and y coordinates of start and end of mouse drag.

static int             gStatus;	// 0 = Waiting to grab image, 1 = Drawing bounding boxes, 2 = Placing warp lines.
static int             check_num;

// ============================================================================
//	Functions
// ============================================================================

int             main(int argc, char *argv[]);
static int      init(int argc, char *argv[]);
static void     Mouse(int button, int state, int x, int y);
static void     Motion(int x, int y);
static void     Keyboard(unsigned char key, int x, int y);
/* static void     Quit(void); */
static void     mainLoop(void);
static void     Visibility(int visible);
static void     Reshape(int w, int h);
static void     Display(void);
static void     draw_warp_line(double a, double b , double c);
static void     draw_line(void);
static void     draw_line2(double *x, double *y, int num);
static void     draw_warp_line(double a, double b , double c);
static void     print_comment(int status);
static void     cleanup(void);

int main(int argc, char *argv[])
{
	glutInit(&argc, argv);
    if (!init(argc, argv)) exit(-1);
	
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutInitWindowSize(gXsize, gYsize);
    glutInitWindowPosition(100,100);
    gWin = glutCreateWindow("Calibrate distortion");
	
	// Setup argl library for current context.
	// Turn off distortion compensation.. we don't know distortion yet!
	if ((gArglSettings = arglSetupForCurrentContext()) == NULL) {
		fprintf(stderr, "main(): arglSetupForCurrentContext() returned error.\n");
		exit(-1);
	}
	arglDistortionCompensationSet(gArglSettings, FALSE);
	
	// Make a dummy camera parameter to supply when calling arglDispImage().
	gARTCparam.xsize = gXsize;
	gARTCparam.ysize = gYsize;
	
	// Register GLUT event-handling callbacks.
	// NB: mainLoop() is registered by Visibility.
    glutDisplayFunc(Display);
	glutReshapeFunc(Reshape);
	glutVisibilityFunc(Visibility);
	glutKeyboardFunc(Keyboard);
    glutMouseFunc(Mouse);
    glutMotionFunc(Motion);
	
	// Start grabbing.
	if (arVideoCapStart() != 0) {
    	fprintf(stderr, "init(): Unable to begin camera data capture.\n");
		return (FALSE);		
	}
	point_num = 0;
    gStatus = 0;
    print_comment(0);
	
    glutMainLoop();
	
	return (0);
}

static int init(int argc, char *argv[])
{
    char    line[512];
    int     i;
	
    gPatt.h_num    = H_NUM;
    gPatt.v_num    = V_NUM;
    gPatt.loop_num = 0;
    if (gPatt.h_num < 3 || gPatt.v_num < 3) exit(0);
	
	// Add command-line arguments to vconf string.
    strcpy(line, vconf);
    for (i = 1; i < argc; i++) {
        strcat(line, " ");
        strcat(line, argv[i]);
    }
	
	// Open the video path.
    if (arVideoOpen(line) < 0) {
    	fprintf(stderr, "init(): Unable to open connection to camera.\n");
    	return (FALSE);
	}
	
	// Find the size of the window.
    if (arVideoInqSize(&gXsize, &gYsize) < 0) return (FALSE);
    fprintf(stdout, "Camera image size (x,y) = (%d,%d)\n", gXsize, gYsize);
	
	// Allocate space for a clipping image (luminance only).
	arMalloc(gClipImage, unsigned char, gXsize * gYsize);

	return (TRUE);
}

static void grabImage(void) {
	ARUint8 *image;
	
	// Processing a new image.
	// Copy an image to saved image buffer.
	do {
		image = arVideoGetImage();
	} while (image == NULL);
	gPatt.loop_num++;
	if ((gPatt.arglSettings[gPatt.loop_num-1] = arglSetupForCurrentContext()) == NULL) {
		fprintf(stderr, "grabImage(): arglSetupForCurrentContext() returned error.\n");
		exit(-1);
	}
	arglDistortionCompensationSet(gPatt.arglSettings[gPatt.loop_num-1], FALSE);
	arMalloc((gPatt.savedImage)[gPatt.loop_num-1], unsigned char, gXsize*gYsize*AR_PIX_SIZE_DEFAULT);
	memcpy((gPatt.savedImage)[gPatt.loop_num-1], image, gXsize*gYsize*AR_PIX_SIZE_DEFAULT);
	printf("Grabbed image %d.\n", gPatt.loop_num);
	arMalloc(gPatt.point[gPatt.loop_num-1], CALIB_COORD_T, gPatt.h_num*gPatt.v_num);
}

static void ungrabImage(void) {
	if (gPatt.loop_num == 0) {printf("error!!\n"); exit(0);}
	free(gPatt.point[gPatt.loop_num-1]);
	gPatt.point[gPatt.loop_num-1] = NULL;
	free(gPatt.savedImage[gPatt.loop_num-1]);
	gPatt.savedImage[gPatt.loop_num-1] = NULL;
	arglCleanup(gPatt.arglSettings[gPatt.loop_num-1]);
	gPatt.loop_num--;			
}

void checkFit(void) {
    printf("\n-----------\n");
	if (check_num < gPatt.loop_num) {
		printf("Checking fit on image %3d of %3d.\n", check_num + 1, gPatt.loop_num);
		if (check_num + 1 < gPatt.loop_num) {
			printf("Press mouse button to check fit of next image.\n");
		} else {
			printf("Press mouse button to finish.\n");
		}
		glutPostRedisplay();
	} else {
		cleanup();
		exit(0);
	}				
}

static void eventCancel(void) {
	// What was cancelled?
	if (gStatus == 0) {
		// Cancelled grabbing.
		// Live video will not be needed from here on.
		arVideoCapStop();
		if (gPatt.loop_num == 0) {
			// No images with all features identified, so quit.
			cleanup();
			exit(0);
		} else {
			// At least one image with all features identified,
			// so calculate distortion.
			calc_distortion(&gPatt, gXsize, gYsize, dist_factor);
			printf("--------------\n");
			printf("Center X: %f\n", dist_factor[0]);
			printf("       Y: %f\n", dist_factor[1]);
			printf("Dist Factor: %f\n", dist_factor[2]);
			printf("Size Adjust: %f\n", dist_factor[3]);
			printf("--------------\n");
			// Distortion calculation done. Check fit.
			gStatus = 2;
			check_num = 0;
			checkFit();
		}
	} else if (gStatus == 1) {
		// Cancelled rubber-bounding.
		ungrabImage();
		// Restart grabbing.
		point_num = 0;
		arVideoCapStart();
		gStatus = 0;
		if (gPatt.loop_num == 0) print_comment(0);
		else                     print_comment(4);	
	}
}

static void Mouse(int button, int state, int x, int y)
{
    unsigned char   *p;
    int             ssx, ssy, eex, eey;
    int             i, j, k;

	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
		eventCancel();
	} else if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			if (gStatus == 0 && gPatt.loop_num < LOOP_MAX) {
				// End grabbing.
				// Begin waiting for drag for rubber-bounding of a feature.
				grabImage();
				gDragStartX = gDragStartY = gDragEndX = gDragEndY = -1;
				arVideoCapStop();
				gStatus = 1;
				print_comment(1);				
			} else if (gStatus == 1) {
				if (point_num < gPatt.h_num*gPatt.v_num) {
					// Drag for rubber-bounding of a feature has begun.
					gDragStartX = gDragEndX = x;
					gDragStartY = gDragEndY = y;
				} else {
					// Feature point locations have been accepted.
					printf("### Image no.%d ###\n", gPatt.loop_num);
					for( j = 0; j < gPatt.v_num; j++ ) {
						for( i = 0; i < gPatt.h_num; i++ ) {
							printf("%2d, %2d: %6.2f, %6.2f\n", i+1, j+1,
								   gPatt.point[gPatt.loop_num-1][j*gPatt.h_num+i].x_coord,
								   gPatt.point[gPatt.loop_num-1][j*gPatt.h_num+i].y_coord);
						}
					}
					printf("\n\n");
					// Restart grabbing.
					point_num = 0;
					arVideoCapStart();
					gStatus = 0;
					if (gPatt.loop_num < LOOP_MAX) print_comment(4);
					else                           print_comment(5);
				}
			} else if (gStatus == 2) {
				check_num++;
				checkFit();
			}
		} else if (state == GLUT_UP) {
			if (gStatus == 1
				&& gDragStartX != -1 && gDragStartY != -1
				&& gDragEndX != -1 && gDragEndY != -1
				&& point_num < gPatt.h_num*gPatt.v_num) {
				// Drag for rubber-bounding of a feature has finished. Begin identification
				// of center of white region in gClipImage.
				if (gDragStartX < gDragEndX) { ssx = gDragStartX; eex = gDragEndX; }
				else         { ssx = gDragEndX; eex = gDragStartX; }
				if (gDragStartY < gDragEndY) { ssy = gDragStartY; eey = gDragEndY; }
				else         { ssy = gDragEndY; eey = gDragStartY; }
				
				gPatt.point[gPatt.loop_num-1][point_num].x_coord = 0.0;
				gPatt.point[gPatt.loop_num-1][point_num].y_coord = 0.0;
				p = gClipImage;
				k = 0;
				for (j = 0; j < (eey-ssy+1); j++) {
					for (i = 0; i < (eex-ssx+1); i++) {
						gPatt.point[gPatt.loop_num-1][point_num].x_coord += i * *p;
						gPatt.point[gPatt.loop_num-1][point_num].y_coord += j * *p;
						k += *p;
						p++;
					}
				}
				if (k != 0) {
					gPatt.point[gPatt.loop_num-1][point_num].x_coord /= k;
					gPatt.point[gPatt.loop_num-1][point_num].y_coord /= k;
					gPatt.point[gPatt.loop_num-1][point_num].x_coord += ssx;
					gPatt.point[gPatt.loop_num-1][point_num].y_coord += ssy;
					point_num++;
					printf("Marked feature position %3d of %3d\n", point_num, gPatt.h_num*gPatt.v_num);
					if (point_num == gPatt.h_num*gPatt.v_num) print_comment(2);
				}
				gDragStartX = gDragStartY = gDragEndX = gDragEndY = -1;
				glutPostRedisplay();
			}
		}
	}
}

static void Motion(int x, int y)
{
    unsigned char   *p, *p1;
    int             ssx, ssy, eex, eey;
    int             i, j;
	
	// During mouse drag.
    if (gStatus == 1 && gDragStartX != -1 && gDragStartY != -1) {
		
		// Set x,y of end of mouse drag region.
        gDragEndX = x;
        gDragEndY = y;
		
		// Check that start is above and to left of end.
		if (gDragStartX < gDragEndX) { ssx = gDragStartX; eex = gDragEndX; }
		else         { ssx = gDragEndX; eex = gDragStartX; }
		if (gDragStartY < gDragEndY) { ssy = gDragStartY; eey = gDragEndY; }
		else         { ssy = gDragEndY; eey = gDragStartY; }
		
		// Threshold clipping area, copy it into gClipImage.
        p1 = gClipImage;
        for (j = ssy; j <= eey; j++) {
            p = &(gPatt.savedImage[gPatt.loop_num-1][(j*gXsize+ssx)*AR_PIX_SIZE_DEFAULT]);
            for (i = ssx; i <= eex; i++) {
#if (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_BGRA)
                *p1 = (((255*3 - (*(p+0) + *(p+1) + *(p+2))) / 3) < gThresh ? 0 : 255);
#elif (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_ABGR)
                *p1 = (((255*3 - (*(p+1) + *(p+2) + *(p+3))) / 3) < gThresh ? 0 : 255);
#elif (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_ARGB)
                *p1 = (((255*3 - (*(p+1) + *(p+2) + *(p+3))) / 3) < gThresh ? 0 : 255);
#elif (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_BGR)
                *p1 = (((255*3 - (*(p+0) + *(p+1) + *(p+2))) / 3) < gThresh ? 0 : 255);
#elif (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_RGBA)
                *p1 = (((255*3 - (*(p+0) + *(p+1) + *(p+2))) / 3) < gThresh ? 0 : 255);
#elif (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_RGB)
                *p1 = (((255*3 - (*(p+0) + *(p+1) + *(p+2))) / 3) < gThresh ? 0 : 255);
#elif (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_2vuy)
                *p1 = ((255 - *(p+1)) < gThresh ? 0 : 255);
#elif (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_yuvs)
                *p1 = ((255 - *(p+0)) < gThresh ? 0 : 255);
#elif (AR_DEFAULT_PIXEL_FORMAT == AR_PIXEL_FORMAT_MONO)
                *p1 = ((255 - *(p)) < gThresh ? 0 : 255);
#else
#  error Unknown default pixel format defined in config.h
#endif
                p  += AR_PIX_SIZE_DEFAULT;
                p1++;
            }
        }
		// Tell GLUT the display has changed.
		glutPostRedisplay();
    }
}

static void Keyboard(unsigned char key, int x, int y)
{
	switch (key) {
		case 0x1B:
			eventCancel();
			break;
		case 'T':
		case 't':
			printf("Enter new threshold value (now = %d): ", gThresh);
			scanf("%d",&gThresh); while( getchar()!='\n' );
				printf("\n");
			break;
		case '1':
			gThresh -= 5;
			if (gThresh < 0) gThresh = 0;
				break;
		case '2':
			gThresh += 5;
			if (gThresh > 255) gThresh = 255;
				break;
		default:
			break;
	}
}

static void cleanup(void)
{
	if (gClipImage) {
		free(gClipImage);
		gClipImage = NULL;
	}
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
	if (gStatus == 0) {
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

static void beginOrtho2D(void) {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, gXsize, 0.0, gYsize);
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
    double         x, y;
    int            ssx, eex, ssy, eey;
    int            i;
	
	// Select correct buffer for this context.
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	beginOrtho2D();
	
    if (gStatus == 0) {
		
		arglDispImage(gARTImage, &gARTCparam, 1.0, gArglSettings);	// zoom = 1.0.
		arVideoCapNext();
		gARTImage = NULL;
		
    } else if (gStatus == 1) {
		
        arglDispImage(gPatt.savedImage[gPatt.loop_num-1], &gARTCparam, 1.0, gPatt.arglSettings[gPatt.loop_num-1]);
		// Draw red crosses on the points in the image.
        for (i = 0; i < point_num; i++) {
            x = gPatt.point[gPatt.loop_num-1][i].x_coord;
            y = gPatt.point[gPatt.loop_num-1][i].y_coord;
            glColor3f(1.0f, 0.0f, 0.0f);
            glBegin(GL_LINES);
			glVertex2d(x-10.0, (GLdouble)(gYsize-1)-y);
			glVertex2d(x+10.0, (GLdouble)(gYsize-1)-y);
			glVertex2d(x, (GLdouble)(gYsize-1)-(y-10.0));
			glVertex2d(x, (GLdouble)(gYsize-1)-(y+10.0));
            glEnd();
        }
		
		// Draw the current mouse drag clipping area.
        if (gDragStartX != -1 && gDragStartY != -1
			&& gDragEndX != -1 && gDragEndY != -1) {
			if (gDragStartX < gDragEndX) { ssx = gDragStartX; eex = gDragEndX; }
			else         { ssx = gDragEndX; eex = gDragStartX; }
			if (gDragStartY < gDragEndY) { ssy = gDragStartY; eey = gDragEndY; }
			else         { ssy = gDragEndY; eey = gDragStartY; }
#if 1
			if (gClipImage) {
				glPixelZoom(1.0f, -1.0f);	// ARToolKit bitmap 0.0 is at upper-left, OpenGL bitmap 0.0 is at lower-left.
				glRasterPos2f((GLfloat)(ssx), (GLfloat)(gYsize-1-ssy));
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glDrawPixels(eex-ssx+1, eey-ssy+1, GL_LUMINANCE, GL_UNSIGNED_BYTE, gClipImage);
			}
#else
            glColor3f(0.0f, 0.0f, 1.0f);
            glBegin(GL_LINE_LOOP);
			glVertex2i(gDragStartX, (gYsize-1)-gDragStartY);
			glVertex2i(gDragEndX, (gYsize-1)-gDragStartY);
			glVertex2i(gDragEndX, (gYsize-1)-gDragEndY);
			glVertex2i(gDragStartX, (gYsize-1)-gDragEndY);
            glEnd();
#endif
        }
		
    } else if (gStatus == 2) {
		
        arglDispImage(gPatt.savedImage[check_num], &gARTCparam, 1.0, gPatt.arglSettings[check_num]);
        for (i = 0; i < gPatt.h_num*gPatt.v_num; i++) {
            x = gPatt.point[check_num][i].x_coord;
            y = gPatt.point[check_num][i].y_coord;
            glColor3f(1.0f, 0.0f, 0.0f);
            glBegin(GL_LINES);
			glVertex2d(x-10.0, (GLdouble)(gYsize-1)-y);
			glVertex2d(x+10.0, (GLdouble)(gYsize-1)-y);
			glVertex2d(x, (GLdouble)(gYsize-1)-(y-10.0));
			glVertex2d(x, (GLdouble)(gYsize-1)-(y+10.0));
            glEnd();
        }
        draw_line();
		
    }
	
	endOrtho2D();
    glutSwapBuffers();
}

static void draw_line(void)
{
    double   *x, *y;
    int      max;
    // int      num; // unreferenced
    int      i, j, k, l;
    int      p;
	
    max = (gPatt.v_num > gPatt.h_num) ? gPatt.v_num : gPatt.h_num;
    arMalloc(x, double, max);
    arMalloc(y, double, max);
	
    i = check_num;
	
    for (j = 0; j < gPatt.v_num; j++) {
        for (k = 0; k < gPatt.h_num; k++) {
            x[k] = gPatt.point[i][j*gPatt.h_num+k].x_coord;
            y[k] = gPatt.point[i][j*gPatt.h_num+k].y_coord;
        }
        draw_line2(x, y, gPatt.h_num);
    }
	
    for (j = 0; j < gPatt.h_num; j++) {
        for (k = 0; k < gPatt.v_num; k++) {
            x[k] = gPatt.point[i][k*gPatt.h_num+j].x_coord;
            y[k] = gPatt.point[i][k*gPatt.h_num+j].y_coord;
        }
        draw_line2(x, y, gPatt.v_num);
    }
	
    for (j = 3 - gPatt.v_num; j < gPatt.h_num - 2; j++) {
        p = 0;
        for (k = 0; k < gPatt.v_num; k++) {
            l = j+k;
            if (l < 0 || l >= gPatt.h_num) continue;
            x[p] = gPatt.point[i][k*gPatt.h_num+l].x_coord;
            y[p] = gPatt.point[i][k*gPatt.h_num+l].y_coord;
            p++;
        }
        draw_line2(x, y, p);
    }
	
    for (j = 2; j < gPatt.h_num + gPatt.v_num - 3; j++) {
        p = 0;
        for (k = 0; k < gPatt.v_num; k++) {
            l = j-k;
            if (l < 0 || l >= gPatt.h_num) continue;
            x[p] = gPatt.point[i][k*gPatt.h_num+l].x_coord;
            y[p] = gPatt.point[i][k*gPatt.h_num+l].y_coord;
            p++;
        }
        draw_line2(x, y, p);
    }
	
    free(x);
    free(y);
}

static void draw_line2( double *x, double *y, int num )
{
    ARMat    *input, *evec;
    ARVec    *ev, *mean;
    double   a, b, c;
    int      i;
	
    ev     = arVecAlloc(2);
    mean   = arVecAlloc(2);
    evec   = arMatrixAlloc(2, 2);
	
    input  = arMatrixAlloc(num, 2);
    for (i = 0; i < num; i++) {
        arParamObserv2Ideal(dist_factor, x[i], y[i],
							&(input->m[i*2+0]), &(input->m[i*2+1]));
    }
    if (arMatrixPCA(input, evec, ev, mean) < 0) exit(0);
    a =  evec->m[1];
    b = -evec->m[0];
    c = -(a*mean->v[0] + b*mean->v[1]);
	
    arMatrixFree(input);
    arMatrixFree(evec);
    arVecFree(mean);
    arVecFree(ev);
	
    draw_warp_line(a, b, c);
}

static void draw_warp_line(double a, double b , double c)
{
    double   x, y;
    double   x1, y1;
    int      i;
	
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
        for(i = -20; i <= gXsize+20; i+=10) {
            x = i;
            y = -(a*i + c)/b;
			
            arParamIdeal2Observ(dist_factor, x, y, &x1, &y1);
            glVertex2f(x1, gYsize-1-y1);
        }
    }
    glEnd();
}

static void print_comment(int status)
{
    printf("\n-----------\n");
    switch(status) {
        case 0:
			printf("Press mouse button to grab first image,\n");
			printf("or press right mouse button or [esc] to quit.\n");
			break;
        case 1:
			printf("Press mouse button and drag mouse to rubber-bound features (%d x %d),\n", gPatt.h_num, gPatt.v_num);
			printf("or press right mouse button or [esc] to cancel rubber-bounding & retry grabbing.\n");
			break;
        case 2:
			printf("Press mouse button to save feature positions,\n");
			printf("or press right mouse button or [esc] to discard feature positions & retry grabbing.\n");
			break;
        case 4:
			printf("Press mouse button to grab next image,\n");
			printf("or press right mouse button or [esc] to calculate distortion parameter.\n");
			break;
        case 5:
			printf("Press right mouse button or [esc] to calculate distortion parameter.\n");
			break;
    }
}
