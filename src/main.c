// ============================================================================
//	Includes
// ============================================================================

#include <stdio.h>
#include <stdlib.h>					// malloc(), free()
#include <GL/glut.h>
#include <AR/config.h>
#include <AR/video.h>
#include <AR/param.h>			// arParamDisp()
#include <AR/ar.h>
#include <AR/gsub_lite.h>
#include <curl/curl.h>
#include "jsmn.h"
#include <unistd.h>
#include "json.h"
#include "log.h"
#include "buf.h"


// ============================================================================
//	Constants
// ============================================================================

#define VIEW_SCALEFACTOR		0.025		// 1.0 ARToolKit unit becomes 0.025 of my OpenGL units.
#define VIEW_DISTANCE_MIN		0.1			// Objects closer to the camera than this will not be displayed.
#define VIEW_DISTANCE_MAX		100.0		// Objects further away from the camera than this will not be displayed.

// ============================================================================
//	Global variables
// ============================================================================

// Preferences.
static int prefWindowed = TRUE;
static int prefWidth = 640;					// Fullscreen mode width.
static int prefHeight = 480;				// Fullscreen mode height.
static int prefDepth = 32;					// Fullscreen mode bit depth.
static int prefRefresh = 0;					// Fullscreen mode refresh rate. Set to 0 to use default rate.

// Image acquisition.
static ARUint8		*gARTImage = NULL;

// Marker detection.
static int			gARTThreshhold = 100;
static long			gCallCountMarkerDetect = 0;

// Transformation matrix retrieval.
static double		gPatt_width     = 80.0;	// Per-marker, but we are using only 1 marker.
static double		gPatt_centre[2] = {0.0, 0.0}; // Per-marker, but we are using only 1 marker.
static double		gPatt_trans[3][4];		// Per-marker, but we are using only 1 marker.
static int			gPatt_found = FALSE;	// Per-marker, but we are using only 1 marker.
static int			gPatt_id;				// Per-marker, but we are using only 1 marker.

// Drawing.
static ARParam		gARTCparam;
static ARGL_CONTEXT_SETTINGS_REF gArglSettings = NULL;
static int gDrawRotate = FALSE;
static float gDrawRotateAngle = 0;			// For use in drawing.

//More keyboard control vars
static int snapToMarker = FALSE;


//janky data structs cuz i hate c
int num_verts = 100250;
struct triples {
        double x[100250][3];
};
static struct triples VERTS;

static int registered = FALSE; 

char *KEYS[] = {"norm_position_x", "norm_position_y", "norm_position_z" };

static float px0 = 0.0f;
static float py0 = 0.0f;
static float pz0 = 0.5f;
static float px1 = 0.0f;
static float py1 = 0.0f;
static float pz1 = 0.5f;
static float PX = 0.0f;
static float PY = 0.0f;
static float PZ = 0.5f;

// ============================================================================
//	Functions
// ============================================================================

static void DrawItem(void)
{
	printf("%s\n", "Start draw.");
	// printf("winx = %d\n", glutGet(GLUT_WINDOW_X));
	// printf("max x = %d\n", glutGet(GLUT_WINDOW_WIDTH));
	static GLuint polyList = 0;
	float fSize = 30.0f;
	int i = 0;
	if (!polyList) {
		polyList = glGenLists (1);
		glNewList(polyList, GL_COMPILE);
		// printf("%d\n", num_verts);
		glBegin (GL_POINTS);
		for (i = 0; i < num_verts; i++){
			glColor3f(1.0, 0.0, 1.0);
			glVertex3f(VERTS.x[i][0]*fSize, VERTS.x[i][1]*fSize, VERTS.x[i][2]*fSize);
		}
		glEnd ();
		// glColor3f (0.0, 0.0, 0.0); 
		// glBegin (GL_LINE_LOOP);
		// for (j = 0; j < 8; j++) {
		// 	glVertex3f(VERTS.x[i][0]*fSize, VERTS.x[i][1]*fSize, VERTS.x[i][2]*fSize);
		// }
		// glEnd ();
		glEndList ();
	}
	
	if (!registered){
		PX = 0.0f;
		PY = 0.0f;
		PZ = 0.0f;
	}

	glPushMatrix(); // Save world coordinate system.
	printf("PX %f", PX);
	printf(" PY %f", PY);
	printf(" PZ %f\n", PZ);
	glTranslatef(PX, PY, PZ); // Place base of cube on marker surface.
	glRotatef(gDrawRotateAngle, 0.0, 0.0, 1.0); // Rotate about z axis.
	// glDisable(GL_LIGHTING);	// Just use colours.
	glCallList(polyList);	// Draw the cube.
	glPopMatrix();	// Restore world coordinate system.
	printf("%s\n", "Done drawing!");
	registered = TRUE;
}

void lock(void)
{
	FILE *f0 = fopen("../src/refCount.txt", "w");
	if (f0 == NULL)
	{
	    printf("Error opening file!\n");
	    exit(1);
	}

	/* print integers and floats */
	fprintf(f0, "%f", 1);

	fclose(f0);	
}

void unlock(void)
{
	FILE *f0 = fopen("../src/refCount.txt", "w");
	if (f0 == NULL)
	{
	    printf("Error opening file!\n");
	    exit(1);
	}

	/* print integers and floats */
	fprintf(f0, "%f", 0);

	fclose(f0);	
}

int read_leap_dump(void)
{
	// printf("%s\n", "Reaadding???");
	lock();


	int count = 1;
    char* js = 0;
    long length;
    FILE * f = fopen ("../src/leap_data.json", "rb");
    if (f)
    {
      // printf("%s\n", "File Found");
      fseek (f, 0, SEEK_END);
      length = ftell (f);
      fseek (f, 0, SEEK_SET);
      js = malloc (length);
      if (js)
      {
      	// printf("%s\n", "File non trivial");
        fread (js, 1, length, f);
        *(js + length) = '\0';
      }
      fclose (f);
    }

    jsmntok_t *tokens = json_tokenise(js);

    /* The GitHub user API response is a single object. States required to
     * parse this are simple: start of the object, keys, values we want to
     * print, values we want to skip, and then a marker state for the end. */

    typedef enum { START, KEY, PRINT, SKIP, STOP } parse_state;
    parse_state state = START;

    size_t object_tokens = 0;
	size_t i; 
	size_t j;
    for (i = 0, j = 1; j > 0; i++, j--)
    {
        jsmntok_t *t = &tokens[i];

        // Should never reach uninitialized tokens
        log_assert(t->start != -1 && t->end != -1);

        if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
            j += t->size;

        switch (state)
        {
            case START:
                if (t->type != JSMN_OBJECT)
                    log_die("Invalid response: root element must be an object.");

                state = KEY;
                object_tokens = t->size;

                if (object_tokens == 0)
                    state = STOP;

                if (object_tokens % 2 != 0)
                    log_die("Invalid response: object must have even number of children.");

                break;

            case KEY:
                object_tokens--;

                if (t->type != JSMN_STRING)
                    log_die("Invalid response: object keys must be strings.");

                state = SKIP;
				size_t i;
                for (i = 0; i < sizeof(KEYS)/sizeof(char *); i++)
                {
                    if (json_token_streq(js, t, KEYS[i]))
                    {
                        printf("%s: ", KEYS[i]);
                        state = PRINT;
                        break;
                    }
                }

                break;

            case SKIP:
                if (t->type != JSMN_STRING && t->type != JSMN_PRIMITIVE)
                    log_die("Invalid response: object values must be strings or primitives.");

                object_tokens--;
                state = KEY;

                if (object_tokens == 0)
                    state = STOP;

                break;

            case PRINT:
                if (t->type != JSMN_STRING && t->type != JSMN_PRIMITIVE)
                    log_die("Invalid response: object values must be strings or primitives.");

                char *str = json_token_tostr(js, t);
                puts(str);

		        char * pch;
		        printf ("Splitting string \"%s\" into tokens:\n",str);
		        pch = strtok (str,":");
		        while (pch != NULL)
		        {
		            printf ("%s\n",pch);
		            if (count == 1) {
		            	py1 = atof(pch);
		            } 

		            if (count == 2) {
		            	pz1 = atof(pch);
		            }

		            if (count == 3){
		            	px1 = atof(pch);
		            }

		            pch = strtok (NULL, " ");
		        }                

                object_tokens--;
                state = KEY;

                count++;

                if (object_tokens == 0)
                    state = STOP;

                break;


            case STOP:
                // Just consume the tokens
                break;

            default:
                log_die("Invalid state %u", state);
        }
    }

    unlock();
    return 0;
}


void parse(void)
{
	printf("%s\n", "Starting to parse...");
	FILE *myfile;
	int i;
	int j;
	myfile=fopen("Data/dragon", "r");

	for(i = 0; i < num_verts; i++)
	{
		for (j = 0 ; j < 3; j++)
		{
		  fscanf(myfile,"%lf", &VERTS.x[i][j]);
		}
	}

	fclose(myfile);
	printf("%s\n", "Finshed parsing vertices!");
}

// Something to look at, draw a rotating colour cube.
// static void DrawCube(void)
// {
// 	// Colour cube data.
// 	static GLuint polyList = 0;
// 	float fSize = 0.5f;
// 	long f, i;	
// 	const GLfloat cube_vertices [8][3] = {
// 	{1.0, 1.0, 1.0}, {1.0, -1.0, 1.0}, {-1.0, -1.0, 1.0}, {-1.0, 1.0, 1.0},
// 	{1.0, 1.0, -1.0}, {1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0}, {-1.0, 1.0, -1.0} };
// 	const GLfloat cube_vertex_colors [8][3] = {
// 	{1.0, 1.0, 1.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 1.0, 1.0},
// 	{1.0, 0.0, 1.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0} };
// 	GLint cube_num_faces = 6;
// 	const short cube_faces [6][4] = {
// 	{3, 2, 1, 0}, {2, 3, 7, 6}, {0, 1, 5, 4}, {3, 0, 4, 7}, {1, 2, 6, 5}, {4, 5, 6, 7} };
	
// 	if (!polyList) {
// 		polyList = glGenLists (1);
// 		glNewList(polyList, GL_COMPILE);
// 		glBegin (GL_QUADS);
// 		for (f = 0; f < cube_num_faces; f++)
// 			for (i = 0; i < 4; i++) {
// 				glColor3f (cube_vertex_colors[cube_faces[f][i]][0], cube_vertex_colors[cube_faces[f][i]][1], cube_vertex_colors[cube_faces[f][i]][2]);
// 				glVertex3f(cube_vertices[cube_faces[f][i]][0] * fSize, cube_vertices[cube_faces[f][i]][1] * fSize, cube_vertices[cube_faces[f][i]][2] * fSize);
// 			}
// 		glEnd ();
// 		glColor3f (0.0, 0.0, 0.0);
// 		for (f = 0; f < cube_num_faces; f++) {
// 			glBegin (GL_LINE_LOOP);
// 			for (i = 0; i < 4; i++)
// 				glVertex3f(cube_vertices[cube_faces[f][i]][0] * fSize, cube_vertices[cube_faces[f][i]][1] * fSize, cube_vertices[cube_faces[f][i]][2] * fSize);
// 			glEnd ();
// 		}
// 		glEndList ();
// 	}
	
// 	glPushMatrix(); // Save world coordinate system.
// 	glTranslatef(0.0, 0.0, 0.5); // Place base of cube on marker surface.
// 	glRotatef(gDrawRotateAngle, 0.0, 0.0, 1.0); // Rotate about z axis.
// 	glDisable(GL_LIGHTING);	// Just use colours.
// 	glCallList(polyList);	// Draw the cube.
// 	glPopMatrix();	// Restore world coordinate system.
	
// }

static void DrawCubeUpdate(float timeDelta)
{
	if (gDrawRotate) {
		gDrawRotateAngle += timeDelta * 45.0f; // Rotate cube at 45 degrees per second.
		if (gDrawRotateAngle > 360.0f) gDrawRotateAngle -= 360.0f;
	}
}

static int setupCamera(const char *cparam_name, char *vconf, ARParam *cparam)
{	
    ARParam			wparam;
	int				xsize, ysize;

    // Open the video path.
    if (arVideoOpen(vconf) < 0) {
    	fprintf(stderr, "setupCamera(): Unable to open connection to camera.\n");
    	return (FALSE);
	}
	
    // Find the size of the window.
    if (arVideoInqSize(&xsize, &ysize) < 0) return (FALSE);
    fprintf(stdout, "Camera image size (x,y) = (%d,%d)\n", xsize, ysize);
	
	// Load the camera parameters, resize for the window and init.
    if (arParamLoad(cparam_name, 1, &wparam) < 0) {
		fprintf(stderr, "setupCamera(): Error loading parameter file %s for camera.\n", cparam_name);
        return (FALSE);
    }
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
static void debugReportMode(const ARGL_CONTEXT_SETTINGS_REF arglContextSettings)
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
	
	if (arglDrawModeGet(arglContextSettings) == AR_DRAW_BY_GL_DRAW_PIXELS) {
		fprintf(stderr, "DrawMode (C)   : GL_DRAW_PIXELS\n");
	} else if (arglTexmapModeGet(arglContextSettings) == AR_DRAW_TEXTURE_FULL_IMAGE) {
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
}

static void Quit(void)
{
	arglCleanup(gArglSettings);
	arVideoCapStop();
	arVideoClose();
	exit(0);
}

static void Keyboard(unsigned char key, int x, int y)
{
	int mode;
	switch (key) {
		case 0x1B:						// Quit.
		case 'Q':
		case 'q':
			Quit();
			break;
		case 'r':
			gDrawRotate = !gDrawRotate;
			break;
		case 'C':
		case 'c':
			mode = arglDrawModeGet(gArglSettings);
			if (mode == AR_DRAW_BY_GL_DRAW_PIXELS) {
				arglDrawModeSet(gArglSettings, AR_DRAW_BY_TEXTURE_MAPPING);
				arglTexmapModeSet(gArglSettings, AR_DRAW_TEXTURE_FULL_IMAGE);
			} else {
				mode = arglTexmapModeGet(gArglSettings);
				if (mode == AR_DRAW_TEXTURE_FULL_IMAGE)	arglTexmapModeSet(gArglSettings, AR_DRAW_TEXTURE_HALF_IMAGE);
				else arglDrawModeSet(gArglSettings, AR_DRAW_BY_GL_DRAW_PIXELS);
			}
			fprintf(stderr, "*** Camera - %f (frame/sec)\n", (double)gCallCountMarkerDetect/arUtilTimer());
			gCallCountMarkerDetect = 0;
			arUtilTimerReset();
			debugReportMode(gArglSettings);
			break;
		case 'D':
		case 'd':
			arDebug = !arDebug;
			break;
		case 'S':
		case 's':
			snapToMarker = !snapToMarker;
			printf("snapToMarker set to %s\n", snapToMarker);
		case 'X':
		case 'x':
			px0 = 0.0f;
			py0 = 0.0f;
			pz0 = 0.0f;
			px1 = 0.0f;
			py1 = 0.0f;
			pz1 = 0.0f;
			PX = 0.0f;
			PY = 0.0f;
			PZ = 0.0f;
		// case '?':
		// case '/':
		// 	printf("Keys:\n");
		// 	printf(" q or [esc]    Quit demo.\n");
		// 	printf(" c             Change arglDrawMode and arglTexmapMode.\n");
		// 	printf(" d             Activate / deactivate debug mode.\n");
		// 	printf(" ? or /        Show this help.\n");
		// 	printf("\nAdditionally, the ARVideo library supplied the following help text:\n");
		// 	arVideoDispOption();
		// 	break;
		default:
			break;
	}
}

static void Idle(void)
{
	static int ms_prev;
	int ms;
	float s_elapsed;
	ARUint8 *image;

	ARMarkerInfo    *marker_info;					// Pointer to array holding the details of detected markers.
    int             marker_num;						// Count of number of markers detected.
    int             j, k;
	
	// Find out how long since Idle() last ran.
	ms = glutGet(GLUT_ELAPSED_TIME);
	s_elapsed = (float)(ms - ms_prev) * 0.001;
	if (s_elapsed < 0.1f) return; // Don't update more often than 100 Hz.
	ms_prev = ms;
	
	// Update drawing.
	DrawCubeUpdate(s_elapsed);
	
	// Grab a video frame.
	if ((image = arVideoGetImage()) != NULL) {
		gARTImage = image;	// Save the fetched image.
		
		gCallCountMarkerDetect++; // Increment ARToolKit FPS counter.
		
		// Detect the markers in the video frame.
		if (arDetectMarker(gARTImage, gARTThreshhold, &marker_info, &marker_num) < 0) {
			exit(-1);
		}
		
		// Check through the marker_info array for highest confidence
		// visible marker matching our preferred pattern.
		k = -1;
		for (j = 0; j < marker_num; j++) {
			if (marker_info[j].id == gPatt_id) {
				if (k == -1) k = j; // First marker detected.
				else if(marker_info[j].cf > marker_info[k].cf) k = j; // Higher confidence marker detected.
			}
		}
		
		if (k != -1) {
			// Get the transformation between the marker and the real camera into gPatt_trans.
			arGetTransMat(&(marker_info[k]), gPatt_centre, gPatt_width, gPatt_trans);
			gPatt_found = TRUE;
		} else {
			gPatt_found = FALSE;
		}
		
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
		glutIdleFunc(Idle);
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
// This function is called when the window needs redrawing.
//
static void Display(void)
{
    GLdouble p[16];
	GLdouble m[16];
	
	// Select correct buffer for this context.
	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the buffers for new frame.
	
	arglDispImage(gARTImage, &gARTCparam, 1.0, gArglSettings);	// zoom = 1.0.
	arVideoCapNext();
	gARTImage = NULL; // Image data is no longer valid after calling arVideoCapNext().
				
	// Projection transformation.
	arglCameraFrustumRH(&gARTCparam, VIEW_DISTANCE_MIN, VIEW_DISTANCE_MAX, p);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(p);
	glMatrixMode(GL_MODELVIEW);
		
	// Viewing transformation.
	glLoadIdentity();
	// Lighting and geometry that moves with the camera should go here.
	// (I.e. must be specified before viewing transformations.)
	//none
	
	//Read leap dump
	int koo = read_leap_dump();

	float dx = px1 - px0;
	float dy = py1 - py0;
	float dz = pz1 - pz0;

	printf("dx %f\n", dx);

	px0 = px1;
	py0 = py1;
	pz0 = pz1;

	float stepX = 100.0f / glutGet(GLUT_WINDOW_X);
	float stepY = 100.0f/ glutGet(GLUT_WINDOW_Y);

	PX = stepX*dx + PX;
	PY = stepY*dy + PY;
	PZ = stepX*dz + PZ;

	if (snapToMarker && gPatt_found) {
	
		// Calculate the camera position relative to the marker.
		// Replace VIEW_SCALEFACTOR with 1.0 to make one drawing unit equal to 1.0 ARToolKit units (usually millimeters).
		arglCameraViewRH(gPatt_trans, m, VIEW_SCALEFACTOR);
		glLoadMatrixd(m);

		// All lighting and geometry to be drawn relative to the marker goes here.
		DrawItem();
	
	} // gPatt_found
	else {
		arglCameraViewRH(gPatt_trans, m, VIEW_SCALEFACTOR);
		glLoadMatrixd(m);

		// All lighting and geometry to be drawn relative to the marker goes here.
		DrawItem();

	}
	
	// Any 2D overlays go here.
	//none
	
	glutSwapBuffers();
}

int main(int argc, char** argv)
{
	char glutGamemode[32];
	const char *cparam_name = "Data/camera_para.dat";
	//
	// Camera configuration.
	//
	char *vconf = "v4l2src device=/dev/video0 ! ffmpegcolorspace ! capsfilter caps=video/x-raw-rgb,bpp=24,width=320,height=240 ! identity name=artoolkit ! fakesink";
	const char *patt_name  = "Data/patt.kanji";
	
	// ----------------------------------------------------------------------------
	// Library inits.
	//

	glutInit(&argc, argv);

	// ----------------------------------------------------------------------------
	// Hardware setup.
	//

	if (!setupCamera(cparam_name, vconf, &gARTCparam)) {
		fprintf(stderr, "main(): Unable to set up AR camera.\n");
		exit(-1);
	}

	//Read in ply file
	parse();

	// ----------------------------------------------------------------------------
	// Library setup.
	//

	// Set up GL context(s) for OpenGL to draw into.
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	if (!prefWindowed) {
		if (prefRefresh) sprintf(glutGamemode, "%ix%i:%i@%i", prefWidth, prefHeight, prefDepth, prefRefresh);
		else sprintf(glutGamemode, "%ix%i:%i", prefWidth, prefHeight, prefDepth);
		glutGameModeString(glutGamemode);
		glutEnterGameMode();
	} else {
		glutInitWindowSize(prefWidth, prefHeight);
		glutCreateWindow(argv[0]);
	}

	// Setup argl library for current context.
	if ((gArglSettings = arglSetupForCurrentContext()) == NULL) {
		fprintf(stderr, "main(): arglSetupForCurrentContext() returned error.\n");
		exit(-1);
	}
	debugReportMode(gArglSettings);
	glEnable(GL_DEPTH_TEST);
	arUtilTimerReset();
		
	if (!setupMarker(patt_name, &gPatt_id)) {
		fprintf(stderr, "main(): Unable to set up AR marker.\n");
		Quit();
	}
	
	// Register GLUT event-handling callbacks.
	// NB: Idle() is registered by Visibility.
	glutDisplayFunc(Display);
	glutReshapeFunc(Reshape);
	glutVisibilityFunc(Visibility);
	glutKeyboardFunc(Keyboard);
	
	glutMainLoop();

	return (0);
}
