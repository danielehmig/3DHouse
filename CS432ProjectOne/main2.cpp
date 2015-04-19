// Starter file for CS 432, Assignment 2; Fall 2014
// Author: Daniel Ehmig
// Date Completed: 10 September 2014

#include "cs432.h"
#include "vec.h"
#include "mat.h"
#include "matStack.h"
#include "torus.h"
#include <assert.h>

// The initial size and position of our window
#define INIT_WINDOW_XPOS 100
#define INIT_WINDOW_YPOS 30
#define INIT_WINDOW_WIDTH 900
#define INIT_WINDOW_HEIGHT 900

// The time between ticks, in milliseconds
#define TICK_INTERVAL 50

// number of vertices our array can store
const int NumVertices = 30000; 

// The number of distance units that are in our viewport
#define H_VIEWSIZE 200
#define V_VIEWSIZE 200
#define D_VIEWSIZE 200

// The number of tori in our Garland
#define NUM_TORI 150

// typdefs for readability
typedef vec4 point4;
typedef vec4 color4;

// the amount of time in seconds since the the program started
GLfloat currentTime = 0.0;

static point4 points[NumVertices];
static color4 colors[NumVertices];

// define our shader code
#define VERTEX_SHADER_CODE "\
attribute  vec4 vPosition; \
attribute  vec4 vColor; \
varying vec4 color; \
\
uniform mat4 ModelView; \
uniform mat4 Projection; \
uniform vec4 VariableColor; \
\
void main() \
{ \
  color = vColor; \
  if ( vColor[3] < 0.0 ) { \
   color = VariableColor; \
} \
  gl_Position = Projection*ModelView*vPosition; \
}  \
"
;
#define FRAGMENT_SHADER_CODE "\
varying  vec4 color; \
void main() \
{ \
gl_FragColor = color; \
} \
"

//----------------------------------------------------------------------------
// our matrix stack
static MatrixStack  mvstack;

// the model-view matrix, defining the current transformation
static mat4         model_view;

// GPU IDs for the projection and model-view matrices
static GLuint       ModelView, Projection, VariableColor;

// A matrix that will simply be the identity matrix,
// for frame of reference purposes
static mat4			identity_matrix;

//----------------------------------------------------------------------------
// the number of vertices we've loaded so far into the points and colors arrays
static int Index[2] = {0, NumVertices};

//----------------------------------------------------------------------------
// a "color" that tells the shader to use the variable color
const color4 colorIsVarying(0.0,0.0,0.0,-1.0);

//----------------------------------------------------------------------------
// sets the shader's variable color
static void setVariableColor(color4 col) {
	glUniform4fv(VariableColor, 1, col);
}

// The current spinning angle
static GLfloat spinningAngle = 0.0;

// boolean that tells whether we are spinning (initially false)
static int spinning = 0;


//----------------------------------------------------------------------------
// a helper-function that returns a random number in the range [0.0,1.0)
static GLfloat randUniform() {
	return rand()/(GLfloat)RAND_MAX;
}

// Performs a rotation along the x- and y-axes based on the time.
// The intent is that this be used to rotate the entire scene.
static void rotate() {
	model_view *= RotateX(spinningAngle);
	model_view *= RotateY(spinningAngle*0.324);
}

//----------------------------------------------------------------------------
// reference to our torus object
static ObjRef torus;

// static references to the colors for each torus
static color4 tori_colors[NUM_TORI];

//----------------------------------------------------------------------------
// Function to draw each tori in the Garland in their correct position and 
// rotation based on time and position within the Garland.
static void drawTorusCube(int position)
{
	if(position < 0)
	{
		return;
	}
	else
	{
		// save the current transformation matrix
		mvstack.push(model_view);
		
		// obtain a time in the range 0.05 - 20.0 seconds
		GLfloat time = fmod(currentTime, 20.0);

		// lie to each torus about what the "real" time is
		time -= (NUM_TORI - 1 - position) * ((1.5*TICK_INTERVAL)/1000.0);

		// translate the tori in one largle circle
		model_view *= Translate(10 * cos((time * 18 * (M_PI/180))), 
			10 * sin((time * 18 * (M_PI/180))), 0.0);

		// translate the tori for the small circle (to make the spiral)
		model_view *= Translate(0.0, 2.5 * sin((time * 144 * (M_PI/180))), 
			2.5 * cos((time * 144 * (M_PI/180))));

		// flip alternate tori to obtain the correct garland look
		if(position % 2 == 0)
		{
			// rotate the torus so it "flows" correctly with the spiral
			model_view *= RotateX((time * -144) - 90);

			// flip alternate tori
			model_view *= RotateX(90);

			// rotate so it looks like the torus is being "pulled" 
			// in the direction of movement
			model_view *= RotateY(20);

			// twist the torus along its axis of movement
			model_view *= RotateY(time * 90);
		}
		else
		{
			// rotate the torus so it "flows" correctly with the spiral
			model_view *= RotateX((time * -144) - 90);

			// rotation required to flip these alternate tori (or 
			// else the tori don't link together correctly)
			model_view *= RotateY(-90);

			// twist the torus along its axis of movement
			model_view *= RotateX(time * 90);
		}

		// send the transformations to the GPU
		glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);

		// set the color for this particular torus
		setVariableColor(tori_colors[position]);
		
		// modify the frame buffer
		glDrawArrays(GL_TRIANGLES, torus.getStartIdx(), torus.getCount());

		// retrieve the previous transformation matrix
		model_view = mvstack.pop();

		// draw the next torus in the garland
		drawTorusCube(position - 1);
		
	}
}

//----------------------------------------------------------------------------
// draws our scene
// a garland is drawn twisting along its axis of movement in a spiral all along
// one large circle; thus, there is essentially three "dimensions" of movement
static void drawScene() {
	
	// clear scene, handle depth
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
	// enter domain for this scene's transformations
    mvstack.push(model_view);
	
	// rotate the scene based on the current spinning angle
	model_view *= RotateX(spinningAngle*0.4);
	model_view *= RotateY(spinningAngle*0.75);

	// scale the scene so that it fits nicely in the window
	model_view *= Scale(15,15,15);

	// draw the garland based on time and button presses
	drawTorusCube(NUM_TORI - 1);

	model_view = mvstack.pop();
	
	// swap buffers so that just-drawn image is displayed
	glutSwapBuffers();
}

//----------------------------------------------------------------------------
// callback function: handles a mouse-press
static void mouse(int btn, int state, int x, int y) {
	if (btn == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		// toggle the "pause time" boolean if the left mouse is down
		spinning = !spinning;
	}
}

//----------------------------------------------------------------------------
// callback function: handles a window-resizing
static void reshape(int width, int height) {
    glViewport(0, 0, width, height);
	
	// define the viewport to be 200x200x200, centered at origin
	GLfloat left = -H_VIEWSIZE, right = H_VIEWSIZE;
    GLfloat bottom = -V_VIEWSIZE, top = V_VIEWSIZE;
	GLfloat zNear = -D_VIEWSIZE, zFar = D_VIEWSIZE;
	
	// scale everything by the smallest of the two window-dimensions, so that
	// everything is visible. The larger dimension will "see" more
    GLfloat aspect = GLfloat( width ) / height;
	
    if ( aspect > 1.0 ) {
        left *= aspect;
        right *= aspect;
    }
    else {
        bottom /= aspect;
        top /= aspect;
    }
	
	// define the projection matrix, based on the computed dimensions;
	// send the matrix to the GPU
    mat4 projection = Ortho( left, right, bottom, top, zNear, zFar );

	// rotate the projection matrix so we're looking "up" at the circle
	projection *= RotateX(-90);
    glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );
	
	// define the model-view matrix so that it initially does no transformations
    model_view = mat4( 1.0 );   // An Identity matrix
}

//----------------------------------------------------------------------------
// initialize the world
//----------------------------------------------------------------------------
static void init(void) {
	
	// generate the torus' vertices, with a color of "varying"
	torus = genTorus(colorIsVarying, 20, 20, 0.65, Index, points, colors);	

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
    GLuint program = InitShader2(VERTEX_SHADER_CODE, FRAGMENT_SHADER_CODE);
    glUseProgram( program );
	
	// get (and enable) GPU ID for the vertex position
    GLuint vPosition = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray( vPosition );
    glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0,
						  BUFFER_OFFSET(0) );
	
	// get (and enable) the GPU ID for the vertex color
    GLuint vColor = glGetAttribLocation( program, "vColor" );
    glEnableVertexAttribArray( vColor );
    glVertexAttribPointer( vColor, 4, GL_FLOAT, GL_FALSE, 0,
						  BUFFER_OFFSET(sizeof(points)) );
	
	// get the GPU IDs for the two transformation matrices
    ModelView = glGetUniformLocation( program, "ModelView" );
    Projection = glGetUniformLocation( program, "Projection" );
	VariableColor = glGetUniformLocation( program, "VariableColor" );
	
	// enable the depth test so that an objects in front will appear
	// rather than objects that are behind it
	glEnable(GL_DEPTH_TEST);	
	
	// drawing should be of filled-in triangles, not of outlines or points;
	// both backs and fronts should be visible
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	// set the background to light gray
    glClearColor(0.7,0.7,0.7,1.0);

	// initialize the array of random colors for each torus
	int i = 0;
	for(i = 0; i < NUM_TORI; ++i)
	{
		tori_colors[i] = color4(randUniform(), randUniform(), randUniform(), 1.0);
	}

}

//----------------------------------------------------------------------------
// callback function: occurs every time the clock ticks: update the angles and redraws the scene
static void tick(int n) {
	// set up next "tick"
	glutTimerFunc(n, tick, n);
	
	// advance the clock
	currentTime += TICK_INTERVAL/1000.0;
	
	if (spinning) {
		spinningAngle += 4;
	}
	
	// draw the new scene
	drawScene();
}

//----------------------------------------------------------------------------
// callback function, responding to a key-press. Program will terminate if the escape key
// or upper- or lower-case 'Q' is pressed.
static void keyboard( unsigned char key, int x, int y )
{
    switch( key ) {
		case 033: // Escape Key
		case 'q': case 'Q':
			exit( EXIT_SUCCESS );
			break;
    }
}

//----------------------------------------------------------------------------
// main program
int main69( int argc, char **argv ) {
	
	// set up the window
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
	glutInitWindowPosition(INIT_WINDOW_XPOS, INIT_WINDOW_YPOS);
	glutInitWindowSize(INIT_WINDOW_WIDTH,INIT_WINDOW_HEIGHT);
	glutCreateWindow("CS 432, Assignment 2");
	
	// initialize GLEW, if needed
	glewInit();
	
	// call the initializer function
    init();
	
	// set up callback functions
	glutDisplayFunc(drawScene);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
	glutTimerFunc(TICK_INTERVAL, tick, TICK_INTERVAL); // timer callback
    glutMouseFunc(mouse);
	
	// start processing
    glutMainLoop();
	
	// (we should never get here)
    return 0;
}