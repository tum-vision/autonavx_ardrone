/*
 *	twoView.c
 *
 *  Some code to demonstrate grabbing from two video sources.
 *  Press '?' while running for help.
 *
 *  Copyright (c) 2004-2007 Philip Lamb (PRL) phil@eden.net.nz. All rights reserved.
 *
 *	Rev		Date		Who		Changes
 *	1.0.0	2004-10-27	PRL		Initial version.
 *
 */
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

#include <stdio.h>				// fprintf(), stderr
#include <stdlib.h>				// malloc(), free(), atexit()
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
#include <AR/param.h>			// arParamDisp()
#include <AR/ar.h>
#include <AR/gsub_lite.h>

// ============================================================================
//	Constants and types.
// ============================================================================

#define VIEW_SCALEFACTOR		0.025		// 1.0 ARToolKit unit becomes 0.025 of my OpenGL units.
#define VIEW_DISTANCE_MIN		0.1			// Objects closer to the camera than this will not be displayed.
#define VIEW_DISTANCE_MAX		100.0		// Objects further away from the camera than this will not be displayed.

// For cases in which we have multiple OpenGL contexts, never more than this many.
#define CONTEXTSACTIVECOUNT		2
#define CONTEXTSACTIVECOUNTMAX	CONTEXTSACTIVECOUNT

// Structure to keep track of per-camera variables.
typedef struct {
	int							apiContextIndex;	// API-specific index into an array of display contexts.
	ARParam						ARTCparam;			// Camera parameter.
	AR2VideoParamT				*ARTVideo;			// Video parameters
	ARUint8						*ARTImage;			// Most recent image.
	int							ARTThreshhold;		// Threshold for marker detection.
	long						callCountMarkerDetect;	// Frames received.
	double						patt_trans[3][4];	// Marker transformation.
	int						patt_found;			// Whether marker transformation is valid.
	ARGL_CONTEXT_SETTINGS_REF	arglSettings;		// Settings from ARGL.
} CONTEXT_INFO;

// ============================================================================
//	Global variables.
// ============================================================================

static GLuint *gDrawListBox = NULL;

CONTEXT_INFO *gContextsActive;
int gContextsActiveCount = 0;

// ARToolKit globals.
static long			gCallCountGetImage = 0;
static int			gPatt_id;
static double		gPatt_width     = 80.0;
static double		gPatt_centre[2] = {0.0, 0.0};

// Other globals.
static int gDrawRotate = FALSE;
static float gDrawRotateAngle = 0;			// For use in drawing.

// ============================================================================
//	Functions
// ============================================================================

static int DrawCubeInit(int contextsActiveCountMax)
{
	// Allocate room for display lists for all contexts.
	if (gDrawListBox) return (FALSE); // Sanity check.
	if ((gDrawListBox = (GLuint *)calloc(contextsActiveCountMax, sizeof(GLuint))) == NULL) {
		return (FALSE);
	}	
	return (TRUE);
	
}

static int DrawCubeSetup(int contextIndex)
{
	// Colour cube data.
	float fSize = 0.5f;
	long f, i;	
	const GLfloat cube_vertices [8][3] = {
	{1.0, 1.0, 1.0}, {1.0, -1.0, 1.0}, {-1.0, -1.0, 1.0}, {-1.0, 1.0, 1.0},
	{1.0, 1.0, -1.0}, {1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0}, {-1.0, 1.0, -1.0} };
	const GLfloat cube_vertex_colors [8][3] = {
	{1.0, 1.0, 1.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 1.0, 1.0},
	{1.0, 0.0, 1.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0} };
	GLint cube_num_faces = 6;
	const short cube_faces [6][4] = { {3, 2, 1, 0}, {2, 3, 7, 6}, {0, 1, 5, 4}, {3, 0, 4, 7}, {1, 2, 6, 5}, {4, 5, 6, 7} };
	
	if (!gDrawListBox[contextIndex]) {
		gDrawListBox[contextIndex] = glGenLists (1);
		glNewList(gDrawListBox[contextIndex], GL_COMPILE);
		glBegin (GL_QUADS);
		for (f = 0; f < cube_num_faces; f++)
			for (i = 0; i < 4; i++) {
				glColor3f (cube_vertex_colors[cube_faces[f][i]][0], cube_vertex_colors[cube_faces[f][i]][1], cube_vertex_colors[cube_faces[f][i]][2]);
				glVertex3f(cube_vertices[cube_faces[f][i]][0] * fSize, cube_vertices[cube_faces[f][i]][1] * fSize, cube_vertices[cube_faces[f][i]][2] * fSize);
			}
				glEnd ();
		glColor3f (0.0, 0.0, 0.0);
		for (f = 0; f < cube_num_faces; f++) {
			glBegin (GL_LINE_LOOP);
			for (i = 0; i < 4; i++)
				glVertex3f(cube_vertices[cube_faces[f][i]][0] * fSize, cube_vertices[cube_faces[f][i]][1] * fSize, cube_vertices[cube_faces[f][i]][2] * fSize);
			glEnd ();
		}
		glEndList ();
	}
	
	return (TRUE);
}

// Something to look at, draw a rotating colour cube.
static void DrawCube(int contextIndex)
{
	// Draw the colour cube.
	glPushMatrix(); // Save world coordinate system.
	glTranslatef(0.0, 0.0, 0.5); // Place base of cube on marker surface.
	glRotatef(gDrawRotateAngle, 0.0, 0.0, 1.0); // Rotate about z axis.
	glDisable(GL_LIGHTING);	// Just use colours.
	glCallList(gDrawListBox[contextIndex]);	// Draw the cube.
	glPopMatrix();	// Restore world coordinate system.
}

static void DrawCubeUpdate(float timeDelta)
{
	if (gDrawRotate) {
		gDrawRotateAngle += timeDelta * 45.0f; // Rotate cube at 45 degrees per second.
		if (gDrawRotateAngle > 360.0f) gDrawRotateAngle -= 360.0f;
	}
}

static int DrawCubeCleanup(int contextIndex)
{
	if (contextIndex >= gContextsActiveCount) return (FALSE); // Sanity check.
	
	// Destroy display lists...
	if (gDrawListBox[contextIndex]) {
		glDeleteLists(gDrawListBox[contextIndex], 1);
		gDrawListBox[contextIndex] = 0;
	}
	
	return (TRUE);
}

static int DrawCubeFinal(void)
{
	if (gDrawListBox) {
		free(gDrawListBox);
		gDrawListBox = NULL;
	}
	return (TRUE);	
}

// Sets up fields ARTVideo, ARTCparam of gContextsActive[0] through gContextsActive[cameraCount - 1].
static int setupCameras(const int cameraCount, const char *cparam_names[], char *vconfs[])
{
	int i;
	ARParam wparam;
	int xsize, ysize;
	
	for (i = 0; i < cameraCount; i++) {
		
		// Open the video path.
		if ((gContextsActive[i].ARTVideo = ar2VideoOpen(vconfs[i])) == NULL) {
			fprintf(stderr, "setupCameras(): Unable to open connection to camera %d.\n", i + 1);
			return (FALSE);
		}
		
		// Find the size of the window.
		if (ar2VideoInqSize(gContextsActive[i].ARTVideo, &xsize, &ysize) < 0) return (FALSE);
		fprintf(stderr, "setupCameras(): Camera %d image size (x,y) = (%d,%d)\n", i + 1, xsize, ysize);

		// Load the camera parameters, resize for the window and init.
		if (arParamLoad(cparam_names[i], 1, &wparam) < 0) {
			fprintf(stderr, "setupCameras(): Error loading parameter file %s for camera %d.\n", cparam_names[i], i + 1);
			return (FALSE);
		}
		arParamChangeSize(&wparam, xsize, ysize, &(gContextsActive[i].ARTCparam));
		arInitCparam(&(gContextsActive[i].ARTCparam));
		fprintf(stderr, "*** Camera %d parameter ***\n", i + 1);
		arParamDisp(&(gContextsActive[i].ARTCparam));
		gContextsActive[i].ARTThreshhold = 100;
		
		// Start the video capture for this camera.
		if (ar2VideoCapStart(gContextsActive[i].ARTVideo) != 0) {
			fprintf(stderr, "setupCameras(): Unable to begin camera data capture for camera %d.\n", i + 1);
			return (FALSE);		
		}
		
	}
	return (TRUE);
}

static int setupMarker(const char *patt_name, int *patt_id)
{
	// Loading only 1 pattern in this example.
    if ((*patt_id = arLoadPatt(patt_name)) < 0) {
        fprintf(stderr, "setupMarker(): pattern load error !!\n");
        return (FALSE);
    }
	
	return (TRUE);
}

// Report state of ARToolKit global variables arFittingMode,
// arImageProcMode, arglDrawMode, arTemplateMatchingMode, arMatchingPCAMode.
static void debugReportMode(ARGL_CONTEXT_SETTINGS_REF	arglSettings)
{
	if (arFittingMode == AR_FITTING_TO_INPUT) {
		fprintf(stderr, "FittingMode (Z): INPUT IMAGE\n");
	} else {
		fprintf(stderr, "FittingMode (Z): COMPENSATED IMAGE\n");
	}
	
	if (arImageProcMode == AR_IMAGE_PROC_IN_FULL) {
		fprintf(stderr, "ProcMode (X)   : FULL IMAGE\n");
	} else {
		fprintf(stderr, "ProcMode (X)   : HALF IMAGE\n");
	}
	
	if (arglDrawModeGet(arglSettings) == AR_DRAW_BY_GL_DRAW_PIXELS) {
		fprintf(stderr, "DrawMode (C)   : GL_DRAW_PIXELS\n");
	} else if (arglTexmapModeGet(arglSettings) == AR_DRAW_TEXTURE_FULL_IMAGE) {
		fprintf(stderr, "DrawMode (C)   : TEXTURE MAPPING (FULL RESOLUTION)\n");
	} else {
		fprintf(stderr, "DrawMode (C)   : TEXTURE MAPPING (HALF RESOLUTION)\n");
	}
	
	if (arTemplateMatchingMode == AR_TEMPLATE_MATCHING_COLOR) {
		fprintf(stderr, "TemplateMatchingMode (M)   : Color Template\n");
	} else {
		fprintf(stderr, "TemplateMatchingMode (M)   : BW Template\n");
	}
	
	if (arMatchingPCAMode == AR_MATCHING_WITHOUT_PCA) {
		fprintf(stderr, "MatchingPCAMode (P)   : Without PCA\n");
	} else {
		fprintf(stderr, "MatchingPCAMode (P)   : With PCA\n");
	}
#ifdef APPLE_TEXTURE_FAST_TRANSFER
	fprintf(stderr, "arglAppleClientStorage is %d.\n", arglAppleClientStorage);
	fprintf(stderr, "arglAppleTextureRange is %d.\n", arglAppleTextureRange);
#endif // APPLE_TEXTURE_FAST_TRANSFER
}

// Function to clean up and then exit. Will be
// installed by atexit() and called when program exit()s.
static void Quit(void)
{
	int i;
	
	fprintf(stdout, "Quitting...\n");

	// OpenGL per-context cleanup.
	for (i = 0; i < gContextsActiveCount; i++) {
		if (gContextsActive[i].apiContextIndex) {
			glutSetWindow(gContextsActive[i].apiContextIndex);
			arglCleanup(gContextsActive[i].arglSettings);
			DrawCubeCleanup(i);
			glutDestroyWindow(gContextsActive[i].apiContextIndex);
			gContextsActive[i].apiContextIndex = 0;
		}
		ar2VideoCapStop(gContextsActive[i].ARTVideo);
		ar2VideoClose(gContextsActive[i].ARTVideo);
	}
	gContextsActiveCount = 0;
	
	// Library finals (in reverse order to inits.)
	DrawCubeFinal();
}

static void Keyboard(unsigned char key, int x, int y)
{
	int i;
	int mode;
	
	switch (key) {
		case 0x1B:						// Quit.
		case 'Q':
		case 'q':
			exit(0);
			break;
		case ' ':
			gDrawRotate = !gDrawRotate;
			break;
		case 'C':
		case 'c':
			for (i = 0; i < gContextsActiveCount; i++) {
				mode = arglDrawModeGet(gContextsActive[i].arglSettings);
				if (mode == AR_DRAW_BY_GL_DRAW_PIXELS) {
					arglDrawModeSet(gContextsActive[i].arglSettings, AR_DRAW_BY_TEXTURE_MAPPING);
					arglTexmapModeSet(gContextsActive[i].arglSettings, AR_DRAW_TEXTURE_FULL_IMAGE);
				} else {
					mode = arglTexmapModeGet(gContextsActive[i].arglSettings);
					if (mode == AR_DRAW_TEXTURE_FULL_IMAGE)	arglTexmapModeSet(gContextsActive[i].arglSettings, AR_DRAW_TEXTURE_HALF_IMAGE);
					else arglDrawModeSet(gContextsActive[i].arglSettings, AR_DRAW_BY_GL_DRAW_PIXELS);
				}				
				fprintf(stderr, "*** Camera %2d - %f (frame/sec)\n", i + 1, (double)(gContextsActive[i].callCountMarkerDetect)/arUtilTimer());
				gContextsActive[i].callCountMarkerDetect = 0;
				debugReportMode(gContextsActive[i].arglSettings);
			}
			arUtilTimerReset();
			gCallCountGetImage = 0;
			break;
		case 'D':
		case 'd':
			arDebug = !arDebug;
			break;
		case '?':
		case '/':
			printf("Keys:\n");
			printf(" q or [esc]    Quit demo.\n");
			printf(" c             Change arglDrawMode and arglTexmapMode.\n");
			printf(" d             Activate / deactivate debug mode.\n");
			printf(" ? or /        Show this help.\n");
			printf("\nAdditionally, the ARVideo library supplied the following help text:\n");
			arVideoDispOption();
			break;
		default:
			break;
	}
}

static void mainLoop(void)
{
	int i;
	static int ms_prev;
	int ms;
	float s_elapsed;
	ARUint8 *image;

	ARMarkerInfo    *marker_info;					// Pointer to array holding the details of detected markers.
    int             marker_num;						// Count of number of markers detected.
    int             j, k;
	
	// Find out how long since mainLoop() last ran.
	ms = glutGet(GLUT_ELAPSED_TIME);
	s_elapsed = (float)(ms - ms_prev) * 0.001;
	if (s_elapsed < 0.01f) return; // Don't update more often than 100 Hz.
	ms_prev = ms;
	
	// Update drawing.
	DrawCubeUpdate(s_elapsed);
	
	gCallCountGetImage++; // Increment mainLoop() counter.
	
	for (i = 0; i < gContextsActiveCount; i++) {
		
		// Grab a video frame.
		if ((image = ar2VideoGetImage(gContextsActive[i].ARTVideo)) != NULL) {
			gContextsActive[i].ARTImage = image;	// Save the fetched image.
			
			gContextsActive[i].callCountMarkerDetect++; // Increment ARToolKit FPS counter.
			//fprintf(stderr, "mainLoop(): Got image #%ld from cam %d on attempt #%ld.\n", gContextsActive[i].callCountMarkerDetect, i + 1, gCallCountGetImage);
			
			// Detect the markers in the video frame.
			if (arDetectMarkerLite(gContextsActive[i].ARTImage, gContextsActive[i].ARTThreshhold, &marker_info, &marker_num) < 0) {
				exit(-1);
			}
			
			// Check through the marker_info array for highest confidence
			// visible marker matching our preferred pattern.
			k = -1;
			for (j = 0; j < marker_num; j++) {
				if (marker_info[j].id == gPatt_id) {
					if (k == -1) k = j; // First marker detected.
					else if (marker_info[j].cf > marker_info[k].cf) k = j; // Higher confidence marker detected.
				}
			}
			
			if(k != -1) {
				// Get the transformation between the marker and the real camera into gPatt_trans1.
				arGetTransMat(&(marker_info[k]), gPatt_centre, gPatt_width, gContextsActive[i].patt_trans);
				gContextsActive[i].patt_found = TRUE;
			} else {
				gContextsActive[i].patt_found = FALSE;
			}
	
			glutPostWindowRedisplay(gContextsActive[i].apiContextIndex);
		}
		
	}
}

//
//	The function is called on events when the visibility of a
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

//
// This function is called when a window needs redrawing.
//
static void DisplayPerContext(const int drawContextIndex)
{
    GLdouble p[16];
	GLdouble m[16];
	
	// Select correct buffer for this context.
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear the buffers for new frame.
	
	arglDispImage(gContextsActive[drawContextIndex].ARTImage,
				  &(gContextsActive[drawContextIndex].ARTCparam),
				  1.0,
				  gContextsActive[drawContextIndex].arglSettings);	// zoom = 1.0.
	ar2VideoCapNext(gContextsActive[drawContextIndex].ARTVideo);
	gContextsActive[drawContextIndex].ARTImage = NULL; // Image data is no longer valid after calling ar2VideoCapNext().
	
	// Projection transformation.
	arglCameraFrustumRH(&(gContextsActive[drawContextIndex].ARTCparam), VIEW_DISTANCE_MIN, VIEW_DISTANCE_MAX, p);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(p);
	glMatrixMode(GL_MODELVIEW);

	// Viewing transformation.
	glLoadIdentity();
	// Lighting and geometry that moves with the camera should go here.
	// (I.e. must be specified before viewing transformations.)
	//none

	if (gContextsActive[drawContextIndex].patt_found) {
		
		// Calculate the camera position relative to the marker.
		// Replace VIEW_SCALEFACTOR with 1.0 to make one drawing unit equal to 1.0 ARToolKit units (usually millimeters).
		arglCameraViewRH(gContextsActive[drawContextIndex].patt_trans, m, VIEW_SCALEFACTOR);
		glLoadMatrixd(m);
		
		// All lighting and geometry to be drawn relative to the marker goes here.
		DrawCube(drawContextIndex);

	} // patt_found
	
	// Any 2D overlays go here.
	//none
				
	// Drawing for this context complete.
}

// Linear search through all active contexts to find context index for the current glut window.
int getContextIndexForCurrentGLUTWindow(void)
{
	int i, window;
	
	if ((window = glutGetWindow()) != 0) {
		for (i = 0; i < gContextsActiveCount; i++) {
			if (gContextsActive[i].apiContextIndex == window) return (i);
		}
	}
	return (-1);		
}

static void Display(void)
{
	int contextIndex;
	
	if ((contextIndex = getContextIndexForCurrentGLUTWindow()) != -1) {
		DisplayPerContext(contextIndex);
		glutSwapBuffers();
	}
}

int main(int argc, char** argv)
{
	int i;
	char windowTitle[32] = {0};
	const char *cparam_names[] = { // Camera parameter names.
		"Data/camera_para.dat",
		"Data/camera_para.dat",
	};
	char *vconfs[] = {					// Camera configuration.
#if defined(_WIN32)
		"Data\\WDM_camera_flipV.xml",
		"Data\\WDM_camera_flipV.xml",
#elif defined(__APPLE__)
		"",
		"",
#else
		"-dev=/dev/video0 -channel=0 -palette=YUV420P -width=320 -height=240",
		"-dev=/dev/video1 -channel=0 -palette=YUV420P -width=320 -height=240",
#endif
	};
	const char *patt_name  = "Data/patt.hiro";
	
	// ----------------------------------------------------------------------------
	// Library inits.
	//

	glutInit(&argc, argv);

	// Register a cleanup function to be called upon exit().
	if (atexit(Quit) < 0) {
		fprintf(stderr, "main(): Unable to register exit function.\n");
		exit(-1); // Bail out if we can't even register our exit function.
	}

	// Initialise drawing libraries.
	if (!DrawCubeInit(CONTEXTSACTIVECOUNTMAX)) {
		fprintf(stderr, "main(): DrawCubeInit returned error.\n");
		exit(-1);
	}
	
	
	// ----------------------------------------------------------------------------
	// Hardware setup.
	//

	if ((gContextsActive = (CONTEXT_INFO *)calloc(CONTEXTSACTIVECOUNTMAX, sizeof(CONTEXT_INFO))) == NULL) exit(-1);
	if (!setupCameras(CONTEXTSACTIVECOUNT, cparam_names, vconfs)) {
		fprintf(stderr, "main(): Unable to set up %d AR cameras.\n", CONTEXTSACTIVECOUNT);
		exit(-1);
	}
	gContextsActiveCount = CONTEXTSACTIVECOUNT;

	// ----------------------------------------------------------------------------
	// Library setup.
	//
	
	// Per- GL context setup.
	for (i = 0; i < gContextsActiveCount; i++ ) {
		
		// Set up GL context(s) for OpenGL to draw into.
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
		glutInitWindowSize(gContextsActive[i].ARTCparam.xsize, gContextsActive[i].ARTCparam.ysize);
		glutInitWindowPosition(10 + 10*i, 20 + 10*i); // First window at 20,10, subsequent windows staggered.
		sprintf(windowTitle, "Video source %i", i);
		if ((gContextsActive[i].apiContextIndex = glutCreateWindow(windowTitle)) < 1) {
			fprintf(stderr, "main(): Unable to create window.\n");
			exit(-1);
		}
		glutDisplayFunc(Display);
		glutReshapeFunc(Reshape);
		glutVisibilityFunc(Visibility);
		glutKeyboardFunc(Keyboard);
		
		DrawCubeSetup(i);

		if ((gContextsActive[i].arglSettings = arglSetupForCurrentContext()) == NULL) {
			fprintf(stderr, "main(): arglSetupForCurrentContext() returned error.\n");
			exit(-1);
		}
		debugReportMode(gContextsActive[i].arglSettings);
		glEnable(GL_DEPTH_TEST);
	}
	arUtilTimerReset();
	
	if (!setupMarker(patt_name, &gPatt_id)) {
		fprintf(stderr, "main(): Unable to set up AR marker in context %d.\n", i);
		exit(-1);
	}	
		
	// Register GLUT event-handling callbacks.
	// NB: mainLoop() is registered by Visibility.
	glutMainLoop();

	return (0);
}
