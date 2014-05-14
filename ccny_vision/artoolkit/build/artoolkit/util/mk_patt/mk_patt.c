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
#include <AR/ar.h>
#include <AR/gsub_lite.h>

// ============================================================================
//	Constants
// ============================================================================

// ============================================================================
//	Global variables
// ============================================================================

/* set up the video format globals */

#if defined(__sgi)
char            *vconf = "-size=FULL";
#elif defined(__linux)
#  if defined(AR_INPUT_GSTREAMER)
char 			*vconf = "videotestsrc";
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

// Image acquisition.
static ARUint8		*gARTImage = NULL;
static ARUint8		*gARTsaveImage = NULL;

// Marker detection.
static int			gARTThreshhold = 100;
static ARMarkerInfo* gTarget  = NULL;


// Drawing.
static ARParam		gARTCparam;
static ARGL_CONTEXT_SETTINGS_REF gArglSettings = NULL;

// ============================================================================
//	Functions
// ============================================================================

void lineSeg(double x1, double y1, double x2, double y2, ARGL_CONTEXT_SETTINGS_REF contextSettings, ARParam cparam, double zoom)
{
	int enable;
    float   ox, oy;
    double  xx1, yy1, xx2, yy2;
	
	if (!contextSettings) return;
	arglDistortionCompensationGet(contextSettings, &enable);
    if (arglDrawModeGet(contextSettings) == AR_DRAW_BY_TEXTURE_MAPPING && enable) {
        xx1 = x1;  yy1 = y1;
        xx2 = x2;  yy2 = y2;
    } else {
        arParamIdeal2Observ(cparam.dist_factor, x1, y1, &xx1, &yy1);
        arParamIdeal2Observ(cparam.dist_factor, x2, y2, &xx2, &yy2);
    }
	
    xx1 *= zoom; yy1 *= zoom;
    xx2 *= zoom; yy2 *= zoom;
	
	ox = 0;
	oy = cparam.ysize - 1;
	glBegin(GL_LINES);
	glVertex2f(ox + xx1, oy - yy1);
	glVertex2f(ox + xx2, oy - yy2);
	glEnd();
    glFlush();
}

static int setupCamera(ARParam *cparam)
{
    ARParam  wparam;
    char     name1[256], name2[256];
	int				xsize, ysize;

    printf("Enter camera parameter filename");
    printf("(Data/camera_para.dat): ");
    if (fgets(name1, 256, stdin) == NULL) exit(0);
    if (sscanf(name1, "%s", name2) != 1) {
        strcpy(name2, "Data/camera_para.dat");
    }

	// Load the camera parameters.
	if (arParamLoad(name2, 1, &wparam) < 0 ) {
        printf("Parameter load error !!\n");
        return (FALSE);
    }
	
    // Open the video path.
    if (arVideoOpen(vconf) < 0) {
    	fprintf(stderr, "setupCamera(): Unable to open connection to camera.\n");
    	return (FALSE);
	}
	
    // Find the size of the window.
    if (arVideoInqSize(&xsize, &ysize) < 0) return (FALSE);
    fprintf(stdout, "Camera image size (x,y) = (%d,%d)\n", xsize, ysize);
	
	// Resize for the window and init.
    arParamChangeSize(&wparam, xsize, ysize, cparam);
    fprintf(stdout, "*** Camera Parameter ***\n");
    arParamDisp(cparam);
	
    arInitCparam(cparam);
	
	if (arVideoCapStart() != 0) {
    	fprintf(stderr, "setupCamera(): Unable to begin camera data capture.\n");
		return (FALSE);		
	}
	
	return (TRUE);
}

static void cleanup(void)
{
	free(gARTsaveImage); gARTsaveImage = NULL;
	arglCleanup(gArglSettings);
	arVideoCapStop();
	arVideoClose();
}

static void Keyboard(unsigned char key, int x, int y)
{
	switch (key) {
		case 0x1B:						// Quit.
		case 'Q':
		case 'q':
			cleanup();
			exit(0);
			break;
		case 'T':
		case 't':
		printf("Enter new threshold value (default = 100): ");
			scanf("%d", &gARTThreshhold); while (getchar()!='\n');
				printf("\n");
			break;
		case '?':
		case '/':
			printf("Keys:\n");
			printf(" q or [esc]    Quit demo.\n");
			printf(" t             Enter new binarization threshold value.\n");
			printf(" ? or /        Show this help.\n");
			printf("\nAdditionally, the ARVideo library supplied the following help text:\n");
			arVideoDispOption();
			break;
		default:
			break;
    }
}

static void Mouse(int button, int state, int x, int y)
{
    char   name1[256], name2[256];
	
	if (state == GLUT_DOWN) {
		if (button == GLUT_RIGHT_BUTTON) {
			cleanup();
			exit(0);
		} else if (button == GLUT_MIDDLE_BUTTON) {
			printf("Enter new threshold value (default = 100): ");
			scanf("%d", &gARTThreshhold); while (getchar() != '\n');
			printf("\n");
		} else if (button == GLUT_LEFT_BUTTON && gARTsaveImage && gTarget) {
			printf("Enter filename: ");
			if (fgets(name1, 256, stdin) == NULL) return;
			if (sscanf(name1, "%s", name2) != 1 ) return;
			if (arSavePatt(gARTsaveImage, gTarget, name2) < 0) {
				printf("ERROR!!\n");
			} else {
				printf("  Saved\n");
			}
		}
	}
}

static void mainLoop(void)
{
	static int ms_prev;
	int ms;
	float s_elapsed;
	ARUint8 *image;
    int             areamax;
	ARMarkerInfo    *marker_info;					// Pointer to array holding the details of detected markers.
    int             marker_num;						// Count of number of markers detected.
    int             i;
	
	// Find out how long since mainLoop() last ran.
	ms = glutGet(GLUT_ELAPSED_TIME);
	s_elapsed = (float)(ms - ms_prev) * 0.001;
	if (s_elapsed < 0.01f) return; // Don't update more often than 100 Hz.
	ms_prev = ms;
	
	// Grab a video frame.
	if ((image = arVideoGetImage()) != NULL) {
		gARTImage = image;
		
		if (arDetectMarker(gARTImage, gARTThreshhold, &marker_info, &marker_num) < 0) {
			exit(-1);
		}
		
		areamax = 0;
		gTarget = NULL;
		for (i = 0; i < marker_num; i++) {
			if (marker_info[i].area > areamax) {
				areamax = marker_info[i].area;
				gTarget = &(marker_info[i]);
			}
		}
		memcpy(gARTsaveImage, gARTImage,  gARTCparam.xsize * gARTCparam.ysize * AR_PIX_SIZE_DEFAULT);
		
		// Tell GLUT the display has changed.
		glutPostRedisplay();
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

//
// This function is called when the window needs redrawing.
//
static void Display(void)
{
	// Select correct buffer for this context.
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT); // Clear the buffers for new frame.
	
	arglDispImage(gARTImage, &gARTCparam, 1.0, gArglSettings);	// zoom = 1.0.
	arVideoCapNext();
	gARTImage = NULL; // Image data is no longer valid after calling arVideoCapNext().

	if (gTarget != NULL) {
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		beginOrtho2D(gARTCparam.xsize, gARTCparam.ysize);
        glLineWidth(2.0f);
        glColor3d(0.0, 1.0, 0.0);
        lineSeg(gTarget->vertex[0][0], gTarget->vertex[0][1],
				gTarget->vertex[1][0], gTarget->vertex[1][1], gArglSettings, gARTCparam, 1.0);
        lineSeg(gTarget->vertex[3][0], gTarget->vertex[3][1],
				gTarget->vertex[0][0], gTarget->vertex[0][1], gArglSettings, gARTCparam, 1.0);
        glColor3d(1.0, 0.0, 0.0);
        lineSeg(gTarget->vertex[1][0], gTarget->vertex[1][1],
				gTarget->vertex[2][0], gTarget->vertex[2][1], gArglSettings, gARTCparam, 1.0);
        lineSeg(gTarget->vertex[2][0], gTarget->vertex[2][1],
				gTarget->vertex[3][0], gTarget->vertex[3][1], gArglSettings, gARTCparam, 1.0);
		endOrtho2D();
    }

	glutSwapBuffers();
}

int main(int argc, char *argv[])
{
	// ----------------------------------------------------------------------------
	// Library inits.
	//
	
	glutInit(&argc, argv);
	
	// ----------------------------------------------------------------------------
	// Hardware setup.
	//
	
	if (!setupCamera(&gARTCparam)) {
		fprintf(stderr, "main(): Unable to set up AR camera.\n");
		exit(-1);
	}
	
	// ----------------------------------------------------------------------------
	// Library setup.
	//
	
	// Set up GL context(s) for OpenGL to draw into.
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowSize(gARTCparam.xsize, gARTCparam.ysize);
	glutCreateWindow(argv[0]);
	
	// Setup argl library for current context.
	if ((gArglSettings = arglSetupForCurrentContext()) == NULL) {
		fprintf(stderr, "main(): arglSetupForCurrentContext() returned error.\n");
		exit(-1);
	}
	
	arMalloc(gARTsaveImage, ARUint8, gARTCparam.xsize * gARTCparam.ysize * AR_PIX_SIZE_DEFAULT);
	
	// Register GLUT event-handling callbacks.
	// NB: mainLoop() is registered by Visibility.
	glutDisplayFunc(Display);
	glutReshapeFunc(Reshape);
	glutVisibilityFunc(Visibility);
	glutKeyboardFunc(Keyboard);
    glutMouseFunc(Mouse);
	
	glutMainLoop();
	
	return (0);
}

