
#include "cs432.h"
#include "vec.h"
#include "mat.h"
#include "matStack.h"
#include "picking.h"
#include <assert.h>

// a couple of typedefs, for readability
typedef vec4 point4;
typedef vec4 color4;

// animation tick interval, in milliseconds
#define TICK_INTERVAL 50

// define the vertex arrays
const int NumQuadVertices = 36;//(6 faces)(2 triangles/face)(3 vertices/triangle)
const int NumPyramidVertices = 18;//(4 triangles @ 3 per; 1 quad @ 6 per)
const int NumVertices = NumQuadVertices+NumPyramidVertices; 
static point4 points[NumVertices];
static color4 colors[NumVertices];

// vertices for a 1x1 cube, centered at the origin
// (last vertex is for the pyramid)
static point4 vertices[9] = {
point4( -0.5, -0.5, 0.5, 1.0 ), // front-lower-left
point4( -0.5, 0.5, 0.5, 1.0 ), // front-upper-left
point4( 0.5, 0.5, 0.5, 1.0 ), // front-upper-right
point4( 0.5, -0.5, 0.5, 1.0 ), // front-lower-right
point4( -0.5, -0.5, -0.5, 1.0 ), // back-lower-left
point4( -0.5, 0.5, -0.5, 1.0 ), // back-upper-left
point4( 0.5, 0.5, -0.5, 1.0 ), // back-upper-right
point4( 0.5, -0.5, -0.5, 1.0 ), // back-lower-right
point4( 0.0, 0.5, 0.0, 1.0 ), // mid-upper-mid
};

// colors for the vertices
// RGBA colors
static color4 vertex_colors[8] = {
color4( 0.0, 0.0, 0.0, 1.0),  // black
color4( 1.0, 0.0, 0.0, 1.0 ),  // red
color4( 1.0, 1.0, 0.0, 1.0 ),  // yellow
color4( 0.0, 1.0, 0.0, 1.0 ),  // green
color4( 0.0, 0.0, 1.0, 1.0 ),  // blue
color4( 1.0, 0.0, 1.0, 1.0 ),  // magenta
color4( 1.0, 1.0, 1.0, 1.0 ),  // white
color4( 0.0, 1.0, 1.0, 1.0 )   // cyan
};

// matrix-related variables
static MatrixStack  mvstack;
static mat4         model_view;
static GLuint       ModelView, Projection;

//----------------------------------------------------------------------------

// sizes for each component
#define TORSO_HEIGHT 5.0
#define TORSO_WIDTH 1.0
#define UPPER_ARM_HEIGHT 3.0
#define LOWER_ARM_HEIGHT 2.0
#define UPPER_LEG_WIDTH  0.5
#define LOWER_LEG_WIDTH  0.5
#define LOWER_LEG_HEIGHT 2.0
#define UPPER_LEG_HEIGHT 3.0
#define UPPER_LEG_WIDTH  0.5
#define UPPER_ARM_WIDTH  0.5
#define LOWER_ARM_WIDTH  0.5
#define HEAD_HEIGHT 1.5
#define HEAD_WIDTH 1.0

// Set up menu item indices, which we can also use with the joint angle array indices
enum {
    TorsoH,
    TorsoV,
    HeadH,
    HeadV,
    RightUpperArm,
    RightLowerArm,
    LeftUpperArm,
    LeftLowerArm,
    RightUpperLeg,
    RightLowerLeg,
    LeftUpperLeg,
    LeftLowerLeg,
    NumJointAngles,
    Quit
};

// define a unique picking ID for each object.  Normally, these would
// be 1, 2, 3, etc., but we're making them all signficantly different
// so that they look different when we display in "pick mode"
const int TORSO_PICK = 0xffffff;
const int HEAD_PICK = 0xffff00;
const int RU_LEG_PICK = 0xff00ff;
const int RL_LEG_PICK = 0xff0000;
const int RU_ARM_PICK = 0x00ffff;
const int RL_ARM_PICK = 0x00ff00;
const int LU_LEG_PICK = 0x0000ff;
const int LL_LEG_PICK = 0x777777;
const int LU_ARM_PICK = 0x77ff00;
const int LL_ARM_PICK = 0x7700ff;

// Joint angles with initial values
static GLfloat
theta[NumJointAngles] = {
0.0,    // TorsoH
0.0,    // TorsoV
0.0,    // HeadH
0.0,    // HeadV
0.0,    // RightUpperArm
0.0,    // RightLowerArm
0.0,    // LeftUpperArm
0.0,    // LeftLowerArm
180.0,  // RightUpperLeg
0.0,    // RightLowerLeg
180.0,  // LeftUpperLeg
0.0     // LeftLowerLeg
};

// the index of the currently-selected angle
static GLint angle = -1;

//----------------------------------------------------------------------------

// number of vertices we've stored in the points and color array so far
static int Index = 0;

// generate vertices for a quad
static void quad(int a, int b, int c, int d ) {
    colors[Index] = vertex_colors[a]; points[Index] = vertices[a]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[b]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[c]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[a]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[c]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[d]; Index++;
}

// generateds vertices for a color cube
static void colorcube( void )
{
    quad( 1, 0, 3, 2 );
    quad( 2, 3, 7, 6 );
    quad( 3, 0, 4, 7 );
    quad( 6, 5, 1, 2 );
    quad( 4, 5, 6, 7 );
    quad( 5, 4, 0, 1 );
}

// generates vertices for a triangle
static void triangle( int a, int b, int c ) {
    colors[Index] = vertex_colors[a]; points[Index] = vertices[a]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[b]; Index++;
    colors[Index] = vertex_colors[a]; points[Index] = vertices[c]; Index++;
}

// generates vertices for a pyramid
static void colorpyramid(void) {
	quad(0,3,4,7);
	triangle(0,3,8);
	triangle(3,4,8);
	triangle(4,7,8);
	triangle(7,0,8);
}

//----------------------------------------------------------------------------

// draws a torso
static void torso() {
    mvstack.push( model_view );
	
    mat4 instance = ( Translate( 0.0, 0.5 * TORSO_HEIGHT, 0.0 ) *
					 Scale( TORSO_WIDTH, TORSO_HEIGHT, TORSO_WIDTH ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glDrawArrays( GL_TRIANGLES, 0, NumQuadVertices );
	
    model_view = mvstack.pop();
}

// draws a head
static void head()
{
	
    mat4 instance = (Translate( 0.0, 0.5 * HEAD_HEIGHT, 0.0 ) *
					 Scale( HEAD_WIDTH, HEAD_HEIGHT, HEAD_WIDTH ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glDrawArrays( GL_TRIANGLES, 0, NumQuadVertices );
	
}

// draws upper part of left arm
static void left_upper_arm()
{
	
    mat4 instance = (Translate( 0.0, 0.5 * UPPER_ARM_HEIGHT, 0.0 ) *
					 Scale( UPPER_ARM_WIDTH,
						   UPPER_ARM_HEIGHT,
						   UPPER_ARM_WIDTH ) );
	
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glDrawArrays( GL_TRIANGLES, 0, NumQuadVertices );
	
}

// draws lower part of left arm
static void left_lower_arm()
{
	
    mat4 instance = ( Translate( 0.0, 0.5 * LOWER_ARM_HEIGHT, 0.0 ) *
					 Scale( LOWER_ARM_WIDTH,
						   LOWER_ARM_HEIGHT,
						   LOWER_ARM_WIDTH ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glDrawArrays( GL_TRIANGLES, 0, NumQuadVertices );
	
}

// draw upper part of right arm
static void right_upper_arm()
{
	
    mat4 instance = (Translate( 0.0, 0.5 * UPPER_ARM_HEIGHT, 0.0 ) *
					 Scale( UPPER_ARM_WIDTH,
						   UPPER_ARM_HEIGHT,
						   UPPER_ARM_WIDTH ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glDrawArrays( GL_TRIANGLES, 0, NumQuadVertices );
	
}

// draw lower part of right arm
static void right_lower_arm()
{
	
    mat4 instance = (Translate( 0.0, 0.5 * LOWER_ARM_HEIGHT, 0.0 ) *
					 Scale( LOWER_ARM_WIDTH,
						   LOWER_ARM_HEIGHT,
						   LOWER_ARM_WIDTH ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glDrawArrays( GL_TRIANGLES, 0, NumQuadVertices );
	
}

// draw upper part of left leg
static void left_upper_leg()
{
	
    mat4 instance = ( Translate( 0.0, 0.5 * UPPER_LEG_HEIGHT, 0.0 ) *
					 Scale( UPPER_LEG_WIDTH,
						   UPPER_LEG_HEIGHT,
						   UPPER_LEG_WIDTH ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glDrawArrays( GL_TRIANGLES, 0, NumQuadVertices );
	
}

// draw lower part of left leg
static void left_lower_leg()
{
	
    mat4 instance = (Translate( 0.0, 0.5 * LOWER_LEG_HEIGHT, 0.0 ) *
					 Scale( LOWER_LEG_WIDTH,
						   LOWER_LEG_HEIGHT,
						   LOWER_LEG_WIDTH ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glDrawArrays( GL_TRIANGLES, 0, NumQuadVertices );
	
}

// draw upper part of right leg
static void right_upper_leg()
{
	
    mat4 instance = (Translate( 0.0, 0.5 * UPPER_LEG_HEIGHT, 0.0 ) *
					 Scale( UPPER_LEG_WIDTH,
						   UPPER_LEG_HEIGHT,
						   UPPER_LEG_WIDTH ) );
	
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glDrawArrays( GL_TRIANGLES, 0, NumQuadVertices );
	
}

// draw lower part of right leg
static void right_lower_leg()
{
	
    mat4 instance = ( Translate( 0.0, 0.5 * LOWER_LEG_HEIGHT, 0.0 ) *
					 Scale( LOWER_LEG_WIDTH,
						   LOWER_LEG_HEIGHT,
						   LOWER_LEG_WIDTH ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view * instance );
    glDrawArrays( GL_TRIANGLES, NumQuadVertices, NumPyramidVertices );
	
}

//----------------------------------------------------------------------------

// picking callback function; returns code of object clicked, or 0 if the click
// was not on an object
void robotPickingFcn(int code) {
	switch (code) {
		case 0:
			// disable all rotations
			angle = -1;
			break;
		case TORSO_PICK:
			// set current rotation to torso-horizontal, unless already
			// there, in which case to torso-vertical
			if (angle == TorsoH) {
				angle = TorsoV;
			}
			else {
				angle = TorsoH;
			}
			break;
		case HEAD_PICK:
			// set current rotation to head-horizontal, unless already
			// there, in which case to head-vertical
			if (angle == HeadH) {
				angle = HeadV;
			}
			else {
				angle = HeadH;
			}
			break;
			/////////////
			// the rest of these just set the given part
			////////////
		case RU_LEG_PICK:
			angle = RightUpperLeg;
			break;
		case RL_LEG_PICK:
			angle = RightLowerLeg;
			break;
		case RU_ARM_PICK:
			angle = RightUpperArm;
			break;
		case RL_ARM_PICK:
			angle = RightLowerArm;
			break;
		case LU_LEG_PICK:
			angle = LeftUpperLeg;
			break;
		case LL_LEG_PICK:
			angle = LeftLowerLeg;
			break;
		case LU_ARM_PICK:
			angle = LeftUpperArm;
			break;
		case  LL_ARM_PICK:
			angle = LeftLowerArm;
			break;
	}
}

// display-callback
static void display()
{
	// initialize
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	
    mvstack.push( model_view );
	
	// draw torso
    model_view = RotateX(theta[TorsoV]) * RotateY( theta[TorsoH] );
	setPickId(TORSO_PICK);
    torso();
	
	mvstack.push(model_view);
	
	// draw head  (in relation to the torso)
    model_view *= ( Translate( 0.0, TORSO_HEIGHT + 0.5 * HEAD_HEIGHT, 0.0 ) *
				   RotateX( theta[HeadV] ) *
				   RotateY( theta[HeadH] ) *
				   Translate( 0.0, -0.5 * HEAD_HEIGHT, 0.0 ) );
	setPickId(HEAD_PICK);
    head();
    model_view = mvstack.pop();
	
	// draw the left-upper arm, in relation to torso
    mvstack.push( model_view );
    model_view *= ( Translate( -( TORSO_WIDTH + UPPER_ARM_WIDTH ),
							  0.9 * TORSO_HEIGHT, 0.0 ) *
				   RotateX( theta[LeftUpperArm] ) );
	setPickId(LU_ARM_PICK);
	left_upper_arm();
	
	// draw the left-lower arm, in relation to left-upper arm
    model_view *= ( Translate( 0.0, UPPER_ARM_HEIGHT, 0.0 ) *
				   RotateX( theta[LeftLowerArm] ) );
	setPickId(LL_ARM_PICK);
    left_lower_arm();
    model_view = mvstack.pop();
	
	// draw right-upper arm in relation to torso
    mvstack.push( model_view );
    model_view *= ( Translate( TORSO_WIDTH + UPPER_ARM_WIDTH,
							  0.9 * TORSO_HEIGHT, 0.0 ) *
				   RotateX( theta[RightUpperArm] ) );
	setPickId(RU_ARM_PICK);
    right_upper_arm();
	
	// draw right-lower arm, in relation to right-upper arm
    model_view *= ( Translate( 0.0, UPPER_ARM_HEIGHT, 0.0 ) *
				   RotateX( theta[RightLowerArm] ) );
	setPickId(RL_ARM_PICK);
    right_lower_arm();
    model_view = mvstack.pop();
	
	// draw left-upper leg, in relation to torso
    mvstack.push( model_view );
    model_view *= ( Translate( -( TORSO_WIDTH + UPPER_LEG_WIDTH ),
							  0.1 * UPPER_LEG_HEIGHT, 0.0 ) *
				   RotateX( theta[LeftUpperLeg] ) );
	setPickId(LU_LEG_PICK);
    left_upper_leg();
	
	// draw left-lower leg, in relation to left-upper leg
    model_view *= ( Translate( 0.0, UPPER_LEG_HEIGHT, 0.0 ) *
				   RotateX( theta[LeftLowerLeg] ) );
	setPickId(LL_LEG_PICK);
    left_lower_leg();
    model_view = mvstack.pop();
	
	// draw right-upper leg, in relation to torso
    mvstack.push( model_view );
    model_view *= ( Translate( TORSO_WIDTH + UPPER_LEG_WIDTH,
							  0.1 * UPPER_LEG_HEIGHT, 0.0 ) *
				   RotateX( theta[RightUpperLeg] ) );
	setPickId(RU_LEG_PICK);
    right_upper_leg();
	
	// draw right-lower leg, in relation to right-upper leg
    model_view *= ( Translate( 0.0, UPPER_LEG_HEIGHT, 0.0 ) *
				   RotateX( theta[RightLowerLeg] ) );
	setPickId(RL_LEG_PICK);
    right_lower_leg();
    model_view = mvstack.pop();
	
    model_view = mvstack.pop();
	
	// finish picking or display buffer, depending on mode
	if (inPickingMode()) {
		endPicking();
	}
	else {
		glutSwapBuffers();
	}
	

}

//----------------------------------------------------------------------------

// mouse-callback: starts a "pick"
static void mouse( int button, int state, int x, int y )
{
    if ( button == GLUT_LEFT_BUTTON && state == GLUT_DOWN ) {
		startPicking(robotPickingFcn, x, y);
	}
}

//----------------------------------------------------------------------------

// menu-selection callback
static void menu( int option )
{
	// if 'quit' was selected, do so
    if ( option == Quit ) {
		exit( EXIT_SUCCESS );
    }
	
	// not "quit": set body-part index to selected option
    angle = option;
}

//----------------------------------------------------------------------------

// window-reshape callback
static void reshape( int width, int height )
{
	// set up so object grows with the smaller of the window dimensions
    glViewport( 0, 0, width, height );
    GLfloat left = -10.0, right = 10.0;
    GLfloat bottom = -5.0, top = 15.0;
    GLfloat zNear = -10.0, zFar = 10.0;
    GLfloat aspect = GLfloat( width ) / height;
    if ( aspect > 1.0 ) {
        left *= aspect;
        right *= aspect;
    }
    else {
        bottom /= aspect;
        top /= aspect;
    }
	
	// do an orthographic projection
    mat4 projection = Ortho( left, right, bottom, top, zNear, zFar );
    glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );
	
	// start with identity matrix--no transformations
    model_view = mat4( 1.0 );
}

//----------------------------------------------------------------------------

// initialization
static void
init( void )
{
	// create the vertices
	colorcube();
	colorpyramid();
	
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
    const GLchar* vShaderCode =
	"attribute  vec4 vPosition; "
	"attribute  vec4 vColor; "
	"varying vec4 color; "
	" "
	"uniform mat4 ModelView; "
	"uniform mat4 Projection; "
	"uniform vec4 PickColor; "
	" "
	"void main()  "
	"{ "
	"color = vColor; "
	"if (PickColor.a >= 0.0) {"
	"  color = PickColor; "
	"}"
	"gl_Position = Projection*ModelView*vPosition; "
	"}  "
	;
    const GLchar* fShaderCode = 
	"varying  vec4 color; "
	"void main()  "
	"{  "
	"gl_FragColor = color; "
	"}  "
	;
    GLuint program = InitShader2(vShaderCode, fShaderCode);
	
    glUseProgram( program );
	
    GLuint vPosition = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray( vPosition );
    glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0,
						  BUFFER_OFFSET(0) );
	
    GLuint vColor = glGetAttribLocation( program, "vColor" );
    glEnableVertexAttribArray( vColor );
    glVertexAttribPointer( vColor, 4, GL_FLOAT, GL_FALSE, 0,
						  BUFFER_OFFSET(sizeof(points)) );
	
    ModelView = glGetUniformLocation( program, "ModelView" );
    Projection = glGetUniformLocation( program, "Projection" );
	setGpuPickColorId( glGetUniformLocation(program, "PickColor"));
	glClearColor(0.75,0.75,0.75,1.0); // light gray background
	
	//showPickColors(true);

	// set up drawing modes
    glEnable( GL_DEPTH_TEST );
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		
}

//----------------------------------------------------------------------------

// keyboard callback
static void
keyboard( unsigned char key, int x, int y )
{
    switch( key ) {
		case 033: // Escape, 'q', or 'Q': quit
		case 'q': case 'Q':
			exit( EXIT_SUCCESS );
			break;
		case 'r': // 'r' pressed: positive angle adjustment
			if (angle < 0) return;
			theta[angle] += 5.0;
			if ( theta[angle] > 360.0 ) { theta[angle] -= 360.0; }
			break;
		case 'R': // 'R' pressed: negative angle adjustment
			if (angle < 0) return;
			theta[angle] -= 5.0;
			if ( theta[angle] < 0.0 ) { theta[angle] += 360.0; }
			break;
    }
}

//----------------------------------------------------------------------------
// callback function: occurs every time the clock ticks: update the angles
// and redraw the scene
static void tick(int n) {
	// set up next "tick"
	glutTimerFunc(n, tick, n);
	
	// draw the new scene
	glutPostRedisplay();
}

//----------------------------------------------------------------------------

// main function: sets up environment, callbacks, menu
int main69_69_69( int argc, char **argv ) {
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
    glutInitWindowSize( 500, 500 );
    glutCreateWindow( "robot" );
	
	glewInit();
	
    init();
		
    glutDisplayFunc( display );
    glutReshapeFunc( reshape );
    glutKeyboardFunc( keyboard );
    glutMouseFunc( mouse );
	glutTimerFunc(TICK_INTERVAL, tick, TICK_INTERVAL); // timer callback
	
    glutCreateMenu( menu );
    glutAddMenuEntry( "torso/horizontal", TorsoH );
    glutAddMenuEntry( "torso/vertical", TorsoV );
    glutAddMenuEntry( "head/horizontal", HeadH );
    glutAddMenuEntry( "head/vertical", HeadV );
    glutAddMenuEntry( "right_upper_arm", RightUpperArm );
    glutAddMenuEntry( "right_lower_arm", RightLowerArm );
    glutAddMenuEntry( "left_upper_arm", LeftUpperArm );
    glutAddMenuEntry( "left_lower_arm", LeftLowerArm );
    glutAddMenuEntry( "right_upper_leg", RightUpperLeg );
    glutAddMenuEntry( "right_lower_leg", RightLowerLeg );
    glutAddMenuEntry( "left_upper_leg", LeftUpperLeg );
    glutAddMenuEntry( "left_lower_leg", LeftLowerLeg );
    glutAddMenuEntry( "quit", Quit );
    glutAttachMenu( GLUT_MIDDLE_BUTTON );
	
    glutMainLoop();
    return 0;
}
