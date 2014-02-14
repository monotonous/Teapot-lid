/**
*	Based on the “ParametricSurfaceNET" code from the 373 course website.
*
*	Author: Joshua Parker
*
*	A program to display a "Utah Teapot" lid segment using Bezier Surfaces.
*
*	Use +/- to zoom in/out
*	Use 'l' to render the mesh
*	Use 's' to render the surface
*/

#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glut.h>
#include "Trackball.h"
#include "Lighting.h"
#include "Geometry.h"

const int windowWidth=400;
const int windowHeight=400;

GLuint displayListID;
bool controlPoints = true;
int numSegments = 4;

double basis1(double t){ return (1-t)*(1-t)*(1-t); }	// (1-t)^3
double basis2(double t){ return 3*t*(1-t)*(1-t); }		// 3t(1-t)^2
double basis3(double t){ return 3*t*t*(1-t); }			// 3t^2(1-t)
double basis4(double t){ return t*t*t; }				// t^3

double dB1(double t){ return -3*(1-t)*(1-t);}			// -3(1-t)^2
double dB2(double t){ return 3*(1-t)*(1-3*t);}			// 9t^2 - 12t + 3 => 3(1-t)(1-3t)
double dB3(double t){ return 3*t*(2-3*t);}				// 3(2-3t)t
double dB4(double t){ return 3*t*t;}					// 3t^2

typedef enum {LINE_MODE, SURFACE_MODE} RENDER_OPTION;
RENDER_OPTION current_render_option=SURFACE_MODE;

typedef double (*basisFunction)(double);
basisFunction bezierBasis[] = { &basis1, &basis2, &basis3, &basis4 };
basisFunction bezierDerivative[] = { &dB1, &dB2, &dB3, &dB4 };

// start from txt file
const int numVertices=22;
CVec3df vertices[numVertices]={
        CVec3df(0,0,3.15f),
        CVec3df(0.8f,0,3.15f),
        CVec3df(0.8f,-0.45f,3.15f),
        CVec3df(0.45f,-0.8f,3.15f),
        CVec3df(0,-0.8f,3.15f),
        CVec3df(0,0,2.85f),
        CVec3df(0.2f,0,2.7f),
        CVec3df(0.2f,-0.112f,2.7f),
        CVec3df(0.112f,-0.2f,2.7f),
        CVec3df(0,-0.2f,2.7f),
        CVec3df(0.4f,0,2.55f),
        CVec3df(0.4f,-0.224f,2.55f),
        CVec3df(0.224f,-0.4f,2.55f),
        CVec3df(0,-0.4f,2.55f),
        CVec3df(1.3f,0,2.55f),
        CVec3df(1.3f,-0.728f,2.55f),
        CVec3df(0.728f,-1.3f,2.55f),
        CVec3df(0,-1.3f,2.55f),
        CVec3df(1.3f,0,2.4f),
        CVec3df(1.3f,-0.728f,2.4f),
        CVec3df(0.728f,-1.3f,2.4f),
        CVec3df(0,-1.3f,2.4f)
};

const int NUM_PATCHES=2;
const int BEZIER_ORDER=4;	// use 4th order (i.e. cubic) Bezier curves in both parameter directions
int ctr_points[NUM_PATCHES][BEZIER_ORDER][BEZIER_ORDER] = {{{0,0,0,0},{1,2,3,4},{5,5,5,5},{6,7,8,9}},
						   	   {{6,7,8,9},{10,11,12,13},{14,15,16,17},{18,19,20,21}}};
// end from txt file

// material properties 
GLfloat mat_specular[4];
GLfloat mat_ambient_and_diffuse[4];
GLfloat mat_shininess[1];

CTrackball trackball;
CLighting lighting;

// Applies the bezier patch for the normal
CVec3df bezierNormal(int patchNum, double s, double t){
	CVec3df dT(0,0,0);
	CVec3df dS(0,0,0);

	for(int i = 0; i < BEZIER_ORDER; i++)
		for(int j = 0; j < BEZIER_ORDER; j++){
			dT += vertices[ctr_points[patchNum][i][j]] * bezierBasis[i](s) * bezierDerivative[j](t);
			dS += vertices[ctr_points[patchNum][i][j]] * bezierBasis[j](t) * bezierDerivative[i](s);
		}
	
	CVec3df normPartial = cross(dT, dS);
	double zero = 0.000001; // approx zero

	if (dT.length() < zero || dS.length() < zero){
		normPartial.setVector(0,0,1);
		return normPartial;
	}

	normPartial.normaliseDestructiveNoError();
	return normPartial;
}

//Applies the bezier patch for the point
CVec3df bezierPoint(int patchNum, double s, double t){
	CVec3df bPoint(0,0,0);
	for(int i = 0; i < BEZIER_ORDER; i++)
		for(int j = 0; j < BEZIER_ORDER; j++)
			bPoint += vertices[ctr_points[patchNum][i][j]] * bezierBasis[i](s) * bezierBasis[j](t);

	return bPoint;
}

//Generates the list to display
void generateDisplayList() {
	glNewList(displayListID, GL_COMPILE);
	glBegin(GL_QUAD_STRIP);
	for(int patchNum = 0; patchNum < NUM_PATCHES; patchNum++){
		for(int i = 0; i < numSegments; i++){
			for(int j = 0; j <= numSegments; j++){
				double sl = (double) i / (double) numSegments;
				double sr = (double) (i + 1) / (double) numSegments;
				double t = (double) j / (double) numSegments;

				glNormal3fv(bezierNormal(patchNum,sl,t).getArray());
				glVertex3fv(bezierPoint(patchNum,sl,t).getArray());
				
				glNormal3fv(bezierNormal(patchNum,sr,t).getArray());
				glVertex3fv(bezierPoint(patchNum,sr,t).getArray());
			}
		}
	}
	glEndList();
}

//handles mouse interaction
void handleMouseMotion(int x, int y)
{	
	trackball.tbMotion(x, y);
}

//handles mouse interaction
void handleMouseClick(int button, int state, int x, int y)
{
	trackball.tbMouse(button, state, x, y);
}

//handles keyboard events
void handleKeyboardEvent(unsigned char key, int x, int y)
{
	switch (key)
	{
		case 'L': // allows capital L
		case 'l': current_render_option=LINE_MODE; 
					glutPostRedisplay(); break;
		case 'S': // allows capital S
		case 's': current_render_option=SURFACE_MODE; 
					glutPostRedisplay(); break;
		case 'C': // allows capital C
		case 'c': controlPoints = !controlPoints; 
					glutPostRedisplay(); break;
		case '=':  // allows + or = to be used for zooming in
		case '+': if (numSegments < 100) { 
					numSegments++;
					generateDisplayList();
					glutPostRedisplay();
					break;
				  }
		case '_':  // allows - or _ to be used for zooming out
		case '-': if (numSegments > 1) {
					numSegments--;
					generateDisplayList();
					glutPostRedisplay();
					break;
				  }
		default: trackball.tbKeyboard(key);
	}
}

//Items to be displayed
void display(void)
{
	glMatrixMode( GL_MODELVIEW );	// Set the view matrix ...
	glLoadIdentity();				// ... to identity.
    gluLookAt(0,0,6, 0,0,0, 0,1,0); // camera is on the z-axis
	trackball.tbMatrix();			// rotate scene using the trackball

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_NORMALIZE);
	glFrontFace(GL_CW);
	if (current_render_option == LINE_MODE)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glPushMatrix();
	glTranslatef(0, 0, -3.0f);
	glPushMatrix();
	glCallList(displayListID);
	glPopMatrix();
	glEnd();
	if (controlPoints){
		glPointSize(5.0);
		glColor3f(0.0, 0.0, 1.0);
		glDisable(GL_LIGHTING);
		glBegin(GL_POINTS);
		for(int i = 0; i < numVertices; i++)
			glVertex3fv(vertices[i].getArray()); 
		glEnd();
		glEnable(GL_LIGHTING);
	}
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
	glFlush();
	glutSwapBuffers();
	glEnd();
}

//Initialise items
void init(void) 
{
	// select clearing color (for glClear)
	glClearColor (1,1,1,1);	// RGB-value for black
	// enable depth buffering
	glEnable(GL_DEPTH_TEST);
	// initialize view (simple orthographic projection)
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(33,1,2,8);
	trackball.tbInit(GLUT_LEFT_BUTTON);

	// material properties 
	mat_ambient_and_diffuse[0]=0.8;		// red material ...
	mat_ambient_and_diffuse[1]=0.0;
	mat_ambient_and_diffuse[2]=0.0;
	mat_ambient_and_diffuse[3]=1;
	mat_specular[0]=0.8f;				// ... with white highlights
	mat_specular[1]=0.8f;				// if light source is reflected
	mat_specular[2]=0.8f;				// on the material surface.
	mat_specular[3]=1;
	mat_shininess[0]=100;
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_ambient_and_diffuse);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	// enable shading and lighting
	lighting.enable();
	glShadeModel(GL_SMOOTH);
}

//Handles when the screen is resized
void reshape(int width, int height ) {
	// Called at start, and whenever user resizes component
	int size = min(width, height);
	glViewport(0, 0, size, size);  // Largest possible square
	trackball.tbReshape(width, height);
}

// create a double buffered colour window
int main(int argc, char** argv)
{
	glutInit(&argc, argv);		
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(windowWidth, windowHeight); 
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Utah Teapot Lid");
	init ();								// initialise view
	displayListID = glGenLists(1);
	generateDisplayList();
	glutMouseFunc(handleMouseClick);		// Set function to handle mouse clicks
	glutMotionFunc(handleMouseMotion);		// Set function to handle mouse motion
	glutKeyboardFunc(handleKeyboardEvent);	// Set function to handle keyboard input
	glutDisplayFunc(display);		// Set function to draw scene
	glutReshapeFunc(reshape);		// Set function called if window gets resized
	glutMainLoop();
	return 0;
}
