// Starter file for CS 432, Assignment 1.
// Author: Daniel Ehmig
// Date Completed: 30 August 2014

#include "cs432.h"
#include "vec.h"
#include "mat.h"
#include "matStack.h"
#include <assert.h>

// The initial size and position of our window
#define INIT_WINDOW_XPOS 100
#define INIT_WINDOW_YPOS 100
#define INIT_WINDOW_WIDTH 500
#define INIT_WINDOW_HEIGHT 500

// The time between ticks, in milliseconds
#define TICK_INTERVAL 50

// The number of distance units that are in our viewport
#define VIEWSIZE 120

#define NUM_OBJECTS 5

// For genShape4(), we will use the following (approximate) constant to 
// avoid an excessive number of divisions.
#define ONE_OVER_ROOT2 .7071067812

// The current spinning angle of our five objects
static GLfloat globalAngle = 0.0;

// boolean that tells whether we are spinning (initially true)
static int spinning = true;

// define typedefs for readability
typedef vec4 point4;
typedef vec4 color4;

// a large number, to ensure that there is enough room
const int NumVertices = 10000; 

// arrays of points and colors, to send to the graphics card
static point4 points[NumVertices];
static color4 colors[NumVertices];

// default vertext shader code
#define VERTEX_SHADER_CODE "\
attribute  vec4 vPosition; \
attribute  vec4 vColor; \
varying vec4 color; \
\
uniform mat4 ModelView; \
uniform mat4 Projection; \
\
void main() \
{ \
color = vColor; \
gl_Position = Projection*ModelView*vPosition; \
}  \
"
;

// default fragment shader code
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
static GLuint       ModelView, Projection;

//----------------------------------------------------------------------------
// the number of vertices we've loaded so far into the points and colors arrays
static int Index = 0;


//----------------------------------------------------------------------------
// creates a skinny triangle
//  - this function is for demo purposes
static ObjRef genSkinnyTriangle(color4 theColor, GLfloat zVal, int *idxVar, point4 *pointsArray, color4 *colorArray) {
	
	// save start index, as it will be part of our return value
	int startIdx = *idxVar;
	
	// add the first vertex to the array(s)
	pointsArray[*idxVar] = point4(-0.1, -0.5, zVal, 1.0);
	colorArray[*idxVar] = theColor;
	(*idxVar)++;
	
	// add second vertex
	pointsArray[*idxVar] = point4(0.1, -0.5, zVal, 1.0);
	colorArray[*idxVar] = theColor;
	(*idxVar)++;
	
	// add third vertex
	pointsArray[*idxVar] = point4(0.0, 0.5, zVal, 1.0);
	colorArray[*idxVar] = theColor;
	(*idxVar)++;
	
	// return the object reference (first and last point)
	return ObjRef(startIdx, *idxVar);
}

//----------------------------------------------------------------------------
// a helper-function that returns a random number in the range [0.0,1.0)
static GLfloat randUniform() {
	return rand()/(GLfloat)RAND_MAX;
}

//----------------------------------------------------------------------------
// This function should generate an equlateral triangle, centered approximately at (0,0,0),
// with each side approximately sin(pi/3) in length
static ObjRef genShape0(int *idxVar, point4 *pointsArray, color4 *colorArray) {
	
	// save start index, as it will be part of our return value
	int startIdx = *idxVar;

	color4 whiteColor = color4(1.0, 1.0, 1.0, 1.0);

	// create the first vertex, the "top" vertex in a Cartesian plane
	pointsArray[*idxVar] = point4(0.0, 0.5, 0.0, 1.0);
	colorArray[*idxVar] = whiteColor;
	(*idxVar)++;

	// create the 2nd vertex, the "bottom-left" vertex in a Cartesian plane
	pointsArray[*idxVar] = point4(-sin(M_PI/3)/2, -sin(M_PI/6)/2, 0.0, 1.0);
	colorArray[*idxVar] = whiteColor;
	(*idxVar)++;

	// create the final vertex, which will be the "bottom-right" vertex in a Cartesian plane
	pointsArray[*idxVar] = point4(sin(M_PI/3)/2, -sin(M_PI/6)/2, 0.0, 1.0);
	colorArray[*idxVar] = whiteColor;
	(*idxVar)++;
	
	// return the object reference (first and last point)
	return ObjRef(startIdx, *idxVar);
}

// This function will generate a hexagon (6-sided) polygon, centered at (0.0,0.0,0.0),
// out of six equilateral triangle (like slices of pizza).
static ObjRef genShape1(int *idxVar, point4 *pointsArray, color4 *colorArray) {
	
	// save start index, as it will be part of our return value
	int startIdx = *idxVar;

	color4 lightGreen = color4(0.49, 1.0, 0.0, 1.0);

	// Many redundant pieces of code (there are only 7 unique points), 
	// yet it adds to clarity to what we're trying to accomplish 

	// construct the bottom triangle
	pointsArray[*idxVar] = point4(0.0, 0.0, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	pointsArray[*idxVar] = point4(-sin(M_PI/6)/2, -sin(M_PI/3)/2, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	pointsArray[*idxVar] = point4(sin(M_PI/6)/2, -sin(M_PI/3)/2, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	// bottom left triangle
	pointsArray[*idxVar] = point4(0.0, 0.0, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	pointsArray[*idxVar] = point4(-sin(M_PI/6)/2, -sin(M_PI/3)/2, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	pointsArray[*idxVar] = point4(-0.5, 0.0, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	// top left triangle
	pointsArray[*idxVar] = point4(0.0, 0.0, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	pointsArray[*idxVar] = point4(-sin(M_PI/6)/2, sin(M_PI/3)/2, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	pointsArray[*idxVar] = point4(-0.5, 0.0, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	// top triangle
	pointsArray[*idxVar] = point4(0.0, 0.0, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	pointsArray[*idxVar] = point4(-sin(M_PI/6)/2, sin(M_PI/3)/2, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	pointsArray[*idxVar] = point4(sin(M_PI/6)/2, sin(M_PI/3)/2, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	// top right triangle
	pointsArray[*idxVar] = point4(0.0, 0.0, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	pointsArray[*idxVar] = point4(0.5, 0.0, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	pointsArray[*idxVar] = point4(sin(M_PI/6)/2, sin(M_PI/3)/2, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	// bottom right triangle
	pointsArray[*idxVar] = point4(0.0, 0.0, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	pointsArray[*idxVar] = point4(0.5, 0.0, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	pointsArray[*idxVar] = point4(sin(M_PI/6)/2, -sin(M_PI/3)/2, 0.0, 1.0);
	colorArray[*idxVar] = lightGreen;
	(*idxVar)++;

	// return the object reference (first and last point)
	return ObjRef(startIdx, *idxVar);
	
}

// This function should generate a 1x1x1 cube, centered at (0,0,0). The cube will
// be constructed from 12 equal sized triangles, with 2 triangles on each face.
static ObjRef genShape2(int *idxVar, point4 *pointsArray, color4 *colorArray) {
	
	// save start index, as it will be part of our return value
	int startIdx = *idxVar;

	// array to store precomputed vertex values;
	// each block of six lines corresponds to two triangles
	// that represent a single face of the cube
	point4 vertices[] = { // Top face
						point4(-0.5, 0.5, -0.5, 1.0),
						point4(-0.5, -0.5, -0.5, 1.0),
						point4(0.5, 0.5, -0.5, 1.0),
						point4(-0.5, -0.5, -0.5, 1.0),
						point4(0.5, 0.5, -0.5, 1.0),
						point4(0.5, -0.5, -0.5, 1.0),
						// West face
						point4(-0.5, 0.5, -0.5, 1.0),
						point4(-0.5, -0.5, -0.5, 1.0),
						point4(-0.5, 0.5, 0.5, 1.0),
						point4(-0.5, -0.5, -0.5, 1.0),
						point4(-0.5, -0.5, 0.5, 1.0),
						point4(-0.5, 0.5, 0.5, 1.0),
						// North face
						point4(-0.5, 0.5, -0.5, 1.0),
						point4(0.5, 0.5, -0.5, 1.0),
						point4(-0.5, 0.5, 0.5, 1.0),
						point4(0.5, 0.5, -0.5, 1.0),
						point4(0.5, 0.5, 0.5, 1.0),
						point4(-0.5, 0.5, 0.5, 1.0),
						// East face
						point4(0.5, 0.5, -0.5, 1.0),
						point4(0.5, -0.5, -0.5, 1.0),
						point4(0.5, 0.5, 0.5, 1.0),
						point4(0.5, -0.5, -0.5, 1.0),
						point4(0.5, -0.5, 0.5, 1.0),
						point4(0.5, 0.5, 0.5, 1.0),
						// South face
						point4(-0.5, -0.5, -0.5, 1.0),
						point4(0.5, -0.5, -0.5, 1.0),
						point4(0.5, -0.5, 0.5, 1.0),
						point4(-0.5, -0.5, -0.5, 1.0),
						point4(-0.5, -0.5, 0.5, 1.0),
						point4(0.5, -0.5, 0.5, 1.0),
						// Top face
						point4(-0.5, 0.5, 0.5, 1.0),
						point4(-0.5, -0.5, 0.5, 1.0),
						point4(0.5, 0.5, 0.5, 1.0),
						point4(-0.5, -0.5, 0.5, 1.0),
						point4(0.5, 0.5, 0.5, 1.0),
						point4(0.5, -0.5, 0.5, 1.0)
	};


	// the colors are generated randomly; assumes the probably of getting
	// the exact same color for two or more faces is infinitesimal
	color4 colors[] = { color4(randUniform(), randUniform(), randUniform(), 1.0),
						color4(randUniform(), randUniform(), randUniform(), 1.0),
						color4(randUniform(), randUniform(), randUniform(), 1.0),
						color4(randUniform(), randUniform(), randUniform(), 1.0),
						color4(randUniform(), randUniform(), randUniform(), 1.0),
						color4(randUniform(), randUniform(), randUniform(), 1.0)
	};
		
	// Iterate over the vertices array, assigning the vertices to the points array.
	// Avoids the massively redundant code that would be here otherwise
	int num_faces = 6;
	int index = 0;
	int inner = 0;
	for (index = 0; index < num_faces; ++index)
	{
		for (inner = 0; inner < num_faces; ++inner)
		{
			pointsArray[*idxVar] = vertices[(num_faces * index) + inner];
			colorArray[*idxVar] = colors[index];
			(*idxVar)++;
		}
	}
	
	// return the object reference (first and last point)
	return ObjRef(startIdx, *idxVar);
}

// This function should generate a tetrahedron or a pyramid, centered approximately
// at (0,0,0). The tetrahedron will be a regular tetrahedron.
static ObjRef genShape3(int *idxVar, point4 *pointsArray, color4 *colorArray) {
	
	// save start index, as it will be part of our return value
	int startIdx = *idxVar;

	// again, we will store all of the vertices for the shape in a temporary
	// array ahead of time to avoid redundant code
	point4 vertices[] = {

		// each block of three point4's defines a face of the regular tetrahedron
		point4(0.5, 0.0, -sqrt(2)/4, 1.0),
		point4(-0.5, 0.0, -sqrt(2)/4, 1.0),
		point4(0.0, 0.5, sqrt(2)/4, 1.0),

		point4(0.0, -0.5, sqrt(2)/4, 1.0),
		point4(-0.5, 0.0, -sqrt(2)/4, 1.0),
		point4(0.0, 0.5, sqrt(2)/4, 1.0),

		point4(0.5, 0.0, -sqrt(2)/4, 1.0),
		point4(0.0, -0.5, sqrt(2)/4, 1.0),
		point4(0.0, 0.5, sqrt(2)/4, 1.0),

		point4(0.5, 0.0, -sqrt(2)/4, 1.0),
		point4(-0.5, 0.0, -sqrt(2)/4, 1.0),
		point4(0.0, -0.5, sqrt(2)/4, 1.0)

	};

	// store random colors for each face in a temp array
	color4 faceColors[] = {
		color4(randUniform(), randUniform(), randUniform(), 1.0),
		color4(randUniform(), randUniform(), randUniform(), 1.0),
		color4(randUniform(), randUniform(), randUniform(), 1.0),
		color4(randUniform(), randUniform(), randUniform(), 1.0)
	};
	
	
	int num_faces = 4;
	int num_verts = 3;
	int index = 0;
	int inner = 0;
	
	// load up the vertices into the points array from the vertices array
	for(index = 0; index < num_faces; ++index)
	{
		for(inner = 0; inner < num_verts; ++inner)
		{
			pointsArray[*idxVar] = vertices[(index * num_verts) + inner];
			colorArray[*idxVar] = faceColors[index];
			(*idxVar)++;
		}
	}

	// return the object reference (first and last point)
	return ObjRef(startIdx, *idxVar);
	
}

// This function will generate a regular octahedron centered at (0.0, 0.0, 0.0).
static ObjRef genShape4(int *idxVar, point4 *pointsArray, color4 *colorArray) {
	
	// save start index, as it will be part of our return value
	int startIdx = *idxVar;

	// again, we will store the points in a temporary array to 
	// avoid redundant code
	point4 vertices[] = {
		point4(-ONE_OVER_ROOT2, 0.0, 0.0, 1.0),
		point4(0.0, ONE_OVER_ROOT2, 0.0, 1.0),
		point4(0.0, 0.0, ONE_OVER_ROOT2, 1.0),

		point4(0.0, ONE_OVER_ROOT2, 0.0, 1.0),
		point4(ONE_OVER_ROOT2, 0.0, 0.0, 1.0),
		point4(0.0, 0.0, ONE_OVER_ROOT2, 1.0),

		point4(ONE_OVER_ROOT2, 0.0, 0.0, 1.0),
		point4(0.0, -ONE_OVER_ROOT2, 0.0, 1.0),
		point4(0.0, 0.0, ONE_OVER_ROOT2, 1.0),

		point4(0.0, -ONE_OVER_ROOT2, 0.0, 1.0),
		point4(-ONE_OVER_ROOT2, 0.0, 0.0, 1.0),
		point4(0.0, 0.0, ONE_OVER_ROOT2, 1.0),

		point4(-ONE_OVER_ROOT2, 0.0, 0.0, 1.0),
		point4(0.0, ONE_OVER_ROOT2, 0.0, 1.0),
		point4(0.0, 0.0, -ONE_OVER_ROOT2, 1.0),

		point4(0.0, ONE_OVER_ROOT2, 0.0, 1.0),
		point4(ONE_OVER_ROOT2, 0.0, 0.0, 1.0),
		point4(0.0, 0.0, -ONE_OVER_ROOT2, 1.0),

		point4(ONE_OVER_ROOT2, 0.0, 0.0, 1.0),
		point4(0.0, -ONE_OVER_ROOT2, 0.0, 1.0),
		point4(0.0, 0.0, -ONE_OVER_ROOT2, 1.0),

		point4(0.0, -ONE_OVER_ROOT2, 0.0, 1.0),
		point4(-ONE_OVER_ROOT2, 0.0, 0.0, 1.0),
		point4(0.0, 0.0, -ONE_OVER_ROOT2, 1.0)
	};
	
	// we will use a temporary array to store the eight random
	// colors that we will need for each face
	color4 faceColors[] = {
		color4(randUniform(), randUniform(), randUniform(), 1.0),
		color4(randUniform(), randUniform(), randUniform(), 1.0),
		color4(randUniform(), randUniform(), randUniform(), 1.0),
		color4(randUniform(), randUniform(), randUniform(), 1.0),
		color4(randUniform(), randUniform(), randUniform(), 1.0),
		color4(randUniform(), randUniform(), randUniform(), 1.0),
		color4(randUniform(), randUniform(), randUniform(), 1.0),
		color4(randUniform(), randUniform(), randUniform(), 1.0)
	};

	// Now, we simply loop through the temporary vertices array and assign
	// them to the buffer that will be sent to the GPU
	int index = 0;
	int inner = 0;
	int num_faces = 8;
	int verts_per_face = 3;

	for(index = 0; index < num_faces; ++index)
	{
		for(inner = 0; inner < verts_per_face; ++inner)
		{
			pointsArray[*idxVar] = vertices[(index * verts_per_face) + inner];
			colorArray[*idxVar] = faceColors[index];
			(*idxVar)++;
		}
	}
	
	// return the object reference (first and last point)
	return ObjRef(startIdx, *idxVar);
	
}

//----------------------------------------------------------------------------
// array of GPU object references
static ObjRef objects[NUM_OBJECTS];

//----------------------------------------------------------------------------
// The current orientation of each of our objects.  The first element of each pair
// specifies an x-axis rotation; the second specifies a y-axis rotation
static GLfloat localAngles[NUM_OBJECTS][2] = {
{23.0, 45.0},
{34.0, 91.0},
{264.0, 7.0},
{67.0, 334.0},
{179.0, 146.0},
};

//----------------------------------------------------------------------------
// Updates both the global rotation of the objects around the center (globalAngle),
// but only if 'spinning' is true.
static void updateAngles() {
	if (spinning) { // only apply transforms if we're spinning
		// modify each local angle entry by a small, relatively uniform amount
		for (int i = 0; i < sizeof localAngles/sizeof *localAngles; i++) {
			localAngles[i][0] += 5.0+randUniform();
			localAngles[i][1] += 3.0+randUniform();
		}
		
		// modify the global angle by two degrees
		globalAngle += 2.0;
	}
}

//----------------------------------------------------------------------------
// draws our scene
//   CS 432 students should NOT modlfy this function
static void drawScene() {
	// clear scene, handle depth
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// enter domain for local transformations
    mvstack.push(model_view);
	
	// scale the scene so that it fits nicely in the window
	model_view *= Scale(2.2, 2.2, 2.2);
	
	// rotate the entire scene by the global angle
    model_view *= RotateZ(globalAngle);
	
	// rotate each individual piece
	for (int i = 0; i < NUM_OBJECTS; i++) {
		mvstack.push( model_view ); // enter local transformation domain
		model_view *= Translate(30, 0, 0); // move 30 away from center in (logical) x-direction
		model_view *= Scale(20, 20, 20); // make it larger by a factor of 20
		model_view *= RotateX(localAngles[i][0]); // rotate by current x-rotation angle
		model_view *= RotateY(localAngles[i][1]); // rotate by current y-rotation angle
		
		// send the transformation matrix to the GPU
		glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
		
		// draw the (transformed) object
		glDrawArrays(GL_TRIANGLES, objects[i].getStartIdx(), objects[i].getCount());
		
		model_view = mvstack.pop(); // undo transformations for the just-drawn object
		
		// rotate by 1/Nth of circle to go to position for next object
		model_view *= RotateZ(360.0/NUM_OBJECTS);
	}
	
	model_view = mvstack.pop(); // undo transformations for this entire draw
	
	// swap buffers so that just-drawn image is displayed
	glutSwapBuffers();
}

//----------------------------------------------------------------------------
// callback function: handles a mouse-press
static void mouse(int btn, int state, int x, int y) {
	if (btn == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		// toggle the "spinning" boolean if the left mouse is down
		spinning = !spinning;
	}
}

//----------------------------------------------------------------------------
// callback function: handles a window-resizing
static void reshape(int width, int height) {
    glViewport(0, 0, width, height);
	
	// define the viewport to be 200x200x200, centered at origin
	GLfloat left = -100.0, right = 100.0;
    GLfloat bottom = -100.0, top = 100.0;
	GLfloat zNear = -100.0, zFar = 100.0;
	
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
    glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );
	
	// define the model-view matrix so that it initially does no transformations
    model_view = mat4( 1.0 );   // An Identity matrix
}

//----------------------------------------------------------------------------
// initialize the world
//----------------------------------------------------------------------------
static void init(void) {
	
	// generate the objects, storing a reference to each in the 'objects' array
	objects[0] = genShape0(&Index, points, colors);
	objects[1] = genShape1(&Index, points, colors);
	objects[2] = genShape2(&Index, points, colors);	
	objects[3] = genShape3(&Index, points, colors);
	objects[4] = genShape4(&Index, points, colors);
	
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
	
	// enable the depth test so that an objects in front will appear
	// rather than objects that are behind it
	glEnable(GL_DEPTH_TEST);	
	
	// drawing should be of filled-in triangles, not of outlines or points;
	// both backs and fronts should be visible
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	// set the background to gray
    glClearColor(0.5,0.5,0.5,1.0);
	
}


//----------------------------------------------------------------------------
// callback function: occurs every time the clock ticks: update the angles and redraws the scene
static void tick(int n) {
	// set up next "tick"
	glutTimerFunc(n, tick, n);
	
	// update our angles
	updateAngles();
	
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
int main( int argc, char **argv ) {
	
	// set up the window
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
	glutInitWindowPosition(INIT_WINDOW_XPOS, INIT_WINDOW_YPOS);
	glutInitWindowSize(INIT_WINDOW_WIDTH,INIT_WINDOW_HEIGHT);
	glutCreateWindow("CS 432, Assignment 1");
	
	glShadeModel(GL_FLAT);
	
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

