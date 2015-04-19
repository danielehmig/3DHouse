#include <cassert>
#include <iostream>
#include "cs432.h"
#include "vec.h"
#include "mat.h"
#include "matStack.h"
#include "picking.h"

using namespace std;

//====================================================================================

#define TICK_INTERVAL 50

static void myCallback(int code) {
	cout << "callback with code = " << code << endl;
}

typedef vec4 point4;
typedef vec4 color4;

const int NumQuadVertices = 36;//(6 faces)(2 triangles/face)(3 vertices/triangle)
const int NumVertices = NumQuadVertices;

static point4 points[NumVertices];
static color4 colors[NumVertices];

static point4 vertices[8] = {
point4( -0.5, -0.5, 0.5, 1.0 ), // front-lower-left
point4( -0.5, 0.5, 0.5, 1.0 ), // front-upper-left
point4( 0.5, 0.5, 0.5, 1.0 ), // front-upper-right
point4( 0.5, -0.5, 0.5, 1.0 ), // front-lower-right
point4( -0.5, -0.5, -0.5, 1.0 ), // back-lower-left
point4( -0.5, 0.5, -0.5, 1.0 ), // back-upper-left
point4( 0.5, 0.5, -0.5, 1.0 ), // back-upper-right
point4( 0.5, -0.5, -0.5, 1.0 ), // back-lower-right
};

// RGBA colors
const color4 BLACK( 0.0, 0.0, 0.0, 1.0); // black
const color4 RED( 1.0, 0.0, 0.0, 1.0 ); // red
const color4 YELLOW ( 1.0, 1.0, 0.0, 1.0 );  // yellow
const color4 GREEN( 0.0, 1.0, 0.0, 1.0 ); // green
const color4 BLUE( 0.0, 0.0, 1.0, 1.0 );  // blue
const color4 MAGENTA( 1.0, 0.0, 1.0, 1.0 );  // magenta
const color4 WHITE( 1.0, 1.0, 1.0, 1.0 );  // white
const color4 CYAN( 0.0, 1.0, 1.0, 1.0 );   // cyan

const color4 BKGD(0.5,0.75,0.75,1.0); // background, light grey

// mapping colors
const color4 COLOR0( 0.0, 0.0, 0.0, -0.01);
const color4 COLOR1( 0.0, 0.0, 0.0, -1.0);
const color4 COLOR2( 0.0, 0.0, 0.0, -2.0);
const color4 COLOR3( 0.0, 0.0, 0.0, -3.0);
const color4 COLOR4( 0.0, 0.0, 0.0, -4.0);
const color4 COLOR5( 0.0, 0.0, 0.0, -5.0);
const color4 COLOR6( 0.0, 0.0, 0.0, -6.0);
const color4 COLOR7( 0.0, 0.0, 0.0, -7.0);

static MatrixStack mvstack;
static mat4         model_view;
static GLuint       ModelView, Projection;
static GLint		ColorMap;

static GLuint		program;

//----------------------------------------------------------------------------
#define VERTEX_SHADER_CODE " \
attribute vec4 vPosition; \
attribute vec4 vColor; \
varying vec4 color;  \
\
uniform mat4 ModelView;  \
uniform mat4 Projection;  \
uniform vec4 ColorMap[8];  \
uniform vec4 PickColor;  \
\
void main() { \
color = vColor; \
if (PickColor.a >= 0.0) { \
color = PickColor; \
} \
else if ( vColor.a < 0.0 ) {  \
int idx = int(-vColor.a);  \
color = ColorMap[idx];  \
}  \
gl_Position = Projection*ModelView*vPosition; \
}   \
"

#define FRAGMENT_SHADER_CODE " \
varying vec4 color; \
void main() \
{ \
gl_FragColor = color;  \
} \
"
static ObjRef cubeObj;

//----------------------------------------------------------------------------

static void setColorMap(color4 col, color4 col2=BLACK, color4 col3=BLACK, color4 col4=BLACK,
						color4 col5=BLACK, color4 col6=BLACK, color4 col7=BLACK, color4 col8=BLACK) {
	color4 temp[8] = {col, col2, col3, col4, col5, col6, col7, col8};
	glUniform4fv(ColorMap, 8, (const GLfloat*)temp);
}

static void setDefaultColorMap() {
	setColorMap(GREEN,RED,BLUE,WHITE,YELLOW,BLACK,CYAN,MAGENTA);
}

//----------------------------------------------------------------------------

static int Index[] = {0, NumVertices};

static void genVertex(color4 cc, point4 p, int* idx, point4* points, color4* colors) {
	if (idx[0] < idx[1]) {
		colors[*idx] = cc;
		points[*idx] = p;
	}
	++*idx;
}

static void
genTriangle(color4 cc, int a, int b, int c, int* idx, point4* points, color4* colors) {
	genVertex(cc, vertices[a], idx, points, colors);
	genVertex(cc, vertices[b], idx, points, colors);
	genVertex(cc, vertices[c], idx, points, colors);
}


static void
genTriangle(color4 cc, point4 a, point4 b, point4 c, int* idx, point4* points, color4* colors) {
	genVertex(cc, a, idx, points, colors);
	genVertex(cc, b, idx, points, colors);
	genVertex(cc, c, idx, points, colors);
}

static void
genQuad(color4 cc, int a, int b, int c, int d, int* idx, point4* points, color4* colors)
{
	genTriangle(cc, a, b, c, idx, points, colors);
	genTriangle(cc, a, c, d, idx, points, colors);
}

static void
genQuad(color4 cc, point4 a, point4 b, point4 c, point4 d, int* idx, point4* points, color4* colors)
{
	genTriangle(cc, a, b, c, idx, points, colors);
	genTriangle(cc, a, c, d, idx, points, colors);
}

static ObjRef genColorCube(int* idx, point4* points, color4* colors) {
	int indexStart = *idx;
	genQuad(COLOR0, 1, 0, 3, 2, idx, points, colors);
    genQuad(COLOR1, 2, 3, 7, 6, idx, points, colors);
    genQuad(COLOR2, 3, 0, 4, 7, idx, points, colors);
    genQuad(COLOR3, 6, 5, 1, 2, idx, points, colors);
    genQuad(COLOR4, 4, 5, 6, 7, idx, points, colors);
    genQuad(COLOR5, 5, 4, 0, 1, idx, points, colors);
	return ObjRef(indexStart, *idx);
}

static void drawObject(ObjRef objRef) {
	int start = objRef.getStartIdx();
    glDrawArrays(GL_TRIANGLES, start, objRef.getCount());
}

static float rotationAngle = 90.0;

static void display(void) {
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
    mvstack.push(model_view);
	model_view *= Scale(10,10,10);
	model_view *= RotateY(rotationAngle);
	
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
	
	setPickId(1234);
	drawObject(cubeObj);
	clearPickId();

	model_view *= Translate(-0.4,0,1);
	model_view *= Scale(0.5,1.5,0.5);
	glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
	
	setPickId(65432);
	drawObject(cubeObj);
	clearPickId();
	
	model_view = mvstack.pop();
	
	if (inPickingMode()) {
		endPicking();
	}
	else {
		glutSwapBuffers();
	}
}

//----------------------------------------------------------------------------

static void mouse( int button, int state, int x, int y ) {
    if ( button == GLUT_LEFT_BUTTON && state == GLUT_DOWN ) {
		cout << "mouse: " << x << ", " << y << endl;
		startPicking(myCallback, x, y);
    }
}

//----------------------------------------------------------------------------

static void
reshape( int width, int height )
{
    glViewport( 0, 0, width, height );
	
    GLfloat left = -200, right = 200.0;
    GLfloat bottom = -200.0, top = 200.0;
    GLfloat zNear = 1.0, zFar = 10000.0;
	
    GLfloat aspect = GLfloat( width ) / height;
	
    if ( aspect > 1.0 ) {
        left *= aspect;
        right *= aspect;
    }
    else {
        bottom /= aspect;
        top /= aspect;
    }
	
    mat4 projection = Ortho(left, right, bottom, top, zNear, zFar);
	const int NN = 10;
	projection = Ortho(-NN, NN, -NN, NN, NN, -NN);
	projection = Perspective(55, 1.0, zNear, zFar);
	//projection = mat4(1.0);
    glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );
	
	model_view = LookAt(20,20,20,0,0,0,0,1,0);
	//model_view = mat4(1.0);
}

//----------------------------------------------------------------------------

static void init(void) {
	cubeObj = genColorCube(Index, points, colors);
	
    // Create a vertex array object
    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );
	
    // Create and initialize a buffer object
    GLuint buffer;
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors),
				 NULL, GL_DYNAMIC_DRAW );
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(points), points );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(points), sizeof(colors),
					colors );
	
    // Load shaders and use the resulting shader program
    const GLchar* vShaderCode = VERTEX_SHADER_CODE;
    const GLchar* fShaderCode = FRAGMENT_SHADER_CODE;
	
	program = InitShader2(vShaderCode, fShaderCode);
	glUseProgram( program );
	
	GLuint vPosition = glGetAttribLocation( program, "vPosition" );
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0,
						  BUFFER_OFFSET(0) );
    GLuint vColor = glGetAttribLocation( program, "vColor" );
	glEnableVertexAttribArray( vColor );
    glVertexAttribPointer( vColor, 4, GL_FLOAT, GL_FALSE, 0,
						  BUFFER_OFFSET(sizeof(points)) );
	
	glClearColor(BKGD[0],BKGD[1],BKGD[2],BKGD[3]); //  background
	//showPickColors(true);

    ModelView = glGetUniformLocation( program, "ModelView" );
    Projection = glGetUniformLocation( program, "Projection" );
	ColorMap = glGetUniformLocation(program, "ColorMap");
	setGpuPickColorId(glGetUniformLocation(program, "PickColor"));
	
	setDefaultColorMap();

	
    glEnable(GL_DEPTH_TEST);
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
}

//----------------------------------------------------------------------------

static void
keyboard( unsigned char key, int x, int y )
{
    switch( key ) {
		case 033: // Escape Key
		case 'q': case 'Q':
			exit( EXIT_SUCCESS );
			break;
		case 'r':
			rotationAngle += 3;
			break;
		case 'R':
			rotationAngle += 18;
			break;
    }
}

//----------------------------------------------------------------------------
// callback function: occurs every time the clock ticks: update the angles and redraws the scene
static void tick(int n) {
	// set up next "tick"
	glutTimerFunc(n, tick, n);
	
	// draw the new scene
	glutPostRedisplay();
}

//----------------------------------------------------------------------------

int
main1q( int argc, char **argv )
{
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
    glutInitWindowSize( 500, 500 );
    glutCreateWindow( "picking test" );
	
	glewInit();
	
    init();
	
    glutDisplayFunc( display );
    glutReshapeFunc( reshape );
    glutKeyboardFunc( keyboard );
    glutMouseFunc( mouse );
	glutTimerFunc(TICK_INTERVAL, tick, TICK_INTERVAL); // timer callback
    glutMainLoop();
    return 0;
}

//------------------------------------------------------------------------------


