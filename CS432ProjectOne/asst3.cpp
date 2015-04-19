//// Assignment 3
//// Author: Daniel Ehmig
//// Date Completed: 14 October 2014
//// Some code derived from the picking demo
//// code written by Dr. Vegdahl and Angel.
//
///*
//===========================================================
//
//************************** USAGE **************************
//
//LEFT-CLICK: Left clicking on any one domino causes it to 
//			start falling. It will start the "cascading"
//			of dominos, which causes successive dominos
//			to fall until the line reaches the end or
//			the line encounters an "immovable" domino.
//
//RIGHT-CLICK: Right clicking on any one domino will cause
//			 that domino to become "immovable". Any dominos
//			 that fall into it will have no effect. In
//			 addition, the domino will turn bright green.
//
//Disclaimer: Right clicking on an already fallen domino will
//			will cause it stand upright (but immovable). 
//			Attempts to fix this problem were futile.
//
//Another Disclaimer: The w,a,s,d key bindings seem backwards
//					but trust me, they work nicely for this.
//
//A (LOWER CASE): Rotate right around the scene.
//
//A (UPPER CASE): Rotate faster right around the scene.
//
//D (LOWER CASE): Rotate left around the scene.
//
//D (UPPER CASE): Rotate faster left around the scene.
//
//S (LOWER CASE): Rotate up (and around) the scene.
//
//S (UPPER CASE): Rotate faster up (and around) the scene.
//
//W (LOWER CASE): Rotate down (and around) the scene.
//
//W (UPPER CASE): Rotate faster down (and around) the scene.
//
//C (LOWER CASE): "Zoom" into the scene. No fast version.
//
//Z (LOWER CASE): "Zoom" out of the scene. No fast version.
//
//===========================================================
//
//
//#include "cs432.h"
//#include "vec.h"
//#include "mat.h"
//#include "matStack.h"
//#include "picking.h"
//#include "domino.h"
//#include <assert.h>
//
//// Typedefs of vec4 for readability
//typedef vec4 point4;
//typedef vec4 color4;
//
//// The tick interval for animations, units in milliseconds
//#define TICK_INTERVAL 40
//
//// The total number of dominos that we have
//#define NUM_DOMS 15
//#define NUM_DOMS_SPIRAL 250
//
//// The position of the first domino in the set
//// all other dominos are placed relative to this one
//#define STARTX -28.0
//#define STARTY 20.0
//#define STARTZ 0.0
//
//// Define constants for our faces; these act
//// as indices into the normals array
//#define POSX 0
//#define NEGX 1
//#define POSY 2
//#define NEGY 3
//#define POSZ 4
//#define NEGZ 5
//
//#define WINDOW_HEIGHT 600
//#define WINDOW_WIDTH 1000
//
//// Define the vertex arrays
//const int num_vertices = 36;
//static point4 points[num_vertices];
//static vec4 normals[num_vertices];
//
//// Vertices for a 2 x 0.5 x 1.0 domino, centered at the origin
//static point4 vertices[8] = 
//{
//	point4(-0.25, 0.0, 0.5, 1.0),	// front lower left
//	point4(0.25, 0.0, 0.5, 1.0),	// front lower right
//	point4(-0.25, 2.0, 0.5, 1.0),	// front upper left
//	point4(0.25, 2.0, 0.5, 1.0),	// front upper right
//	point4(-0.25, 0.0, -0.5, 1.0),	// back lower left
//	point4(0.25, 0.0, -0.5, 1.0),	// back lower right
//	point4(-0.25, 2.0, -0.5, 1.0),	// back upper left
//	point4(0.25, 2.0, -0.5, 1.0)		// back upper right
//};
//
//// Variables for matrix operations
//static MatrixStack mvstack;
//static mat4 model_view;
//static GLuint ModelView, Projection;
//
//// Lighting and material properties
//point4 light_position(20.0, 50.0, 35.0, 0.0);
//color4 light_ambient(0.2, 0.2, 0.2, 1.0);
//color4 light_diffuse(1.0, 1.0, 1.0, 1.0);
//color4 light_specular(1.0, 1.0, 1.0, 1.0);
//
//color4 material_ambient(0.2, 0.2, 0.2, 1.0);
//color4 material_diffuse(0.8, 0.05, 0.05, 1.0);
//color4 material_specular(1.0, 0.0, 0.0, 1.0);
//color4 mat_specular_immovable(0.0, 1.0, 0.0, 1.0);
//float material_shininess = 50.0;
//
//color4 ambient_product = light_ambient * material_ambient;
//color4 diffuse_product = light_diffuse * material_diffuse;
//color4 specular_product = light_specular * material_specular;
//color4 spec_immovable = light_specular * mat_specular_immovable;
//
//// Holds the program code; we need to refer to it in several functions
//GLuint program;
//
//// =====================================================================
//
//// The number of vertices we've stored in the points and color array thus far
//static int index = 0;
//
//// Scene rotation angle about x-axis
//GLfloat sceneXRotation = 15.0;
//
//// scene rotation angle about y-axis
//GLfloat sceneYRotation = -25.0;
//
//// Used for "zooming"
//GLfloat scaleFactor = 1.0;
//
//// Just statically store the normals, since we only have six
//vec4 face_normals[] = {
//	vec4(1.0, 0.0, 0.0, 0.0),
//	vec4(-1.0, 0.0, 0.0, 0.0),
//	vec4(0.0, 1.0, 0.0, 0.0),
//	vec4(0.0, -1.0, 0.0, 0.0),
//	vec4(0.0, 0.0, 1.0, 0.0),
//	vec4(0.0, 0.0, -1.0, 0.0)
//};
//
//static int mouseX = WINDOW_WIDTH/2;
//static int mouseY = WINDOW_HEIGHT/2;
//
//static GLfloat mouseYRot = 0.0;
//static GLfloat mouseXRot = 0.0;
//
//
///*
//===========================================================
//quad(): This function generates vertices for a quad, a
//		single face of a domino. The caller passes in four
//		indices for the corners of the quad that are used
//		to index into the vertices array. The final para-
//		-meter is one of six constants to specify which 
//		face is being generated (used to assign a normal).
//===========================================================
//*/
//static void quad(int topleft, int topright, int botleft, int botright, int face)
//{
//
//	// Generate vertices for the first triangle.
//	normals[index] = face_normals[face];
//	points[index] = vertices[topleft];
//	++index;
//	normals[index] = face_normals[face];
//	points[index] = vertices[topright];
//	++index;
//	normals[index] = face_normals[face];
//	points[index] = vertices[botleft];
//	++index;
//
//	// Generate vertices for the second triangle.
//	normals[index] = face_normals[face];
//	points[index] = vertices[topright];
//	++index;
//	normals[index] = face_normals[face];
//	points[index] = vertices[botleft];
//	++index;
//	normals[index] = face_normals[face];
//	points[index] = vertices[botright];
//	++index;
//}
//
///*
//===========================================================
//color_domino(): Driver function to create the domino object
//				that will be sent to the GPU.
//===========================================================
//*/
//static void color_domino()
//{
//	quad(2, 3, 0, 1, POSZ);
//	quad(6, 2, 4, 0, NEGX);
//	quad(6, 7, 2, 3, POSY);
//	quad(4, 5, 0, 1, NEGY);
//	quad(6, 7, 4, 5, NEGZ);
//	quad(3, 7, 1, 5, POSX);
//}
//
///*
//===========================================================
//The following arrays will store the domino objects that 
//comprise the scene. Each of the straight lines are stored
//in their own array. All of the spirals are stored in one
//array, however.
//===========================================================
//*/
//Domino straight_doms[NUM_DOMS];
//Domino straight_doms2[NUM_DOMS];
//Domino straight_doms3[NUM_DOMS];
//Domino straight_doms4[NUM_DOMS];
//Domino straight_doms5[NUM_DOMS];
//Domino spiral_doms[NUM_DOMS_SPIRAL];
//
//// Variable used for picking only; set to true
//// when the scene has been right clicked as opposed
//// to the typical left click.
//static bool right_clicked;
//
///*
//===========================================================
//picking_callback(): This is the picking callback function
//					that is passed into startPicking().
//					When the scene is clicked, the function
//					will be invoked with the id of the 
//					object that was clicked.
//===========================================================
//*/
//void picking_callback(int code)
//{
//	// The pick id's start at 1 (to avoid picking
//	// the background), so we need to decrement
//	// them to index into the domino arrays.
//	--code;
//
//	// Each clause here simply checks which array
//	// to index into; then, the code performs 
//	// different actions depending on whether the
//	// scene was left or right clicked.
//	if(code < NUM_DOMS)
//	{
//		right_clicked ? straight_doms[code].setImmovable(
//			!straight_doms[code].isImmovable()) : 
//			straight_doms[code].setFalling(true);
//	}
//	else if(code < (2 * NUM_DOMS))
//	{
//		right_clicked ? straight_doms2[code - NUM_DOMS].setImmovable(
//			!straight_doms2[code - NUM_DOMS].isImmovable()) : 
//			straight_doms2[code - NUM_DOMS].setFalling(true);
//	}
//	else if(code < (3 * NUM_DOMS))
//	{
//		right_clicked ? straight_doms3[code - (2 * NUM_DOMS)].setImmovable(
//			!straight_doms3[code - (2 * NUM_DOMS)].isImmovable()) : 
//			straight_doms3[code - (2 * NUM_DOMS)].setFalling(true);
//	}
//	else if(code < (4 * NUM_DOMS))
//	{
//		right_clicked ? straight_doms4[code - (3 * NUM_DOMS)].setImmovable(
//			!straight_doms4[code - (3 * NUM_DOMS)].isImmovable()) : 
//			straight_doms4[code - (3 * NUM_DOMS)].setFalling(true);
//	}
//	else if(code < (5 * NUM_DOMS))
//	{
//		right_clicked ? straight_doms5[code - (4 * NUM_DOMS)].setImmovable(
//			!straight_doms5[code - (4 * NUM_DOMS)].isImmovable()) : 
//			straight_doms5[code - (4 * NUM_DOMS)].setFalling(true);
//	}
//	else
//	{
//		right_clicked ? spiral_doms[code - (5 * NUM_DOMS)].setImmovable(
//			!spiral_doms[code - (5 * NUM_DOMS)].isImmovable()) : 
//			spiral_doms[code - (5 * NUM_DOMS)].setFalling(true);
//	}
//}
//
//// Maps a spiral to a straight line of dominos;
//// the indices unfortunately don't map nicely
//int spiral_mapping[5][2] = {
//	{0, 3}, {1, 2}, {2, 1}, {3, 0}, {4, 4}
//};
//
//// Array to store pointers to each of the straight arrays, for convenience.
//Domino * straight_arrays[] = {straight_doms, straight_doms2, straight_doms3, straight_doms4, straight_doms5};
//
///*
//===========================================================
//draw_straights(): Helper function that draws each of the 
//				  five straight sections of dominos in 
//				  their respective positions. Invoked in
//				  display().
//===========================================================
//*/
//static void draw_straights()
//{
//	// Start with the first domino in EACH straight section,
//	// then draw the second in each section, etc.
//	for(int i = NUM_DOMS - 1; i >= 0; --i)
//	{
//		for(int j = 0; j < 5; ++j)
//		{
//			mvstack.push(model_view);
//
//			vec3 pos = straight_arrays[j][i].getPosition();
//			GLfloat yRot = straight_arrays[j][i].getYRotation();
//			GLfloat zRot = straight_arrays[j][i].getZRotation();
//
//			model_view *= RotateY(yRot);
//
//			// Line up the straight section (roughly) with the 
//			// spiral that it ends up touching.
//			model_view *= RotateY(-spiral_doms[j].getYRotation());
//
//			// Translate the straight sections to line up with
//			// start of their respective spirals. 
//			model_view *= Translate(pos.x + 37, pos.y, pos.z);
//
//			// If the domino being drawn is NOT the first in the line.
//			if(i < NUM_DOMS - 1)
//			{
//				// Rotate down incrementally if the conditions are met.
//				GLfloat prevZRot = straight_arrays[j][i + 1].getZRotation();
//				if((prevZRot > 40.0 || straight_arrays[j][i].isFalling()) && 
//					!straight_arrays[j][i].isImmovable())
//				{
//					straight_arrays[j][i].setFalling(true);
//					if(zRot < 70.0)
//					{
//						model_view *= RotateZ(7.0 * (straight_arrays[j][i].getTime() % 11));
//						straight_arrays[j][i].setZRotation(7.0 * (straight_arrays[j][i].getTime() % 11));
//					}
//					else
//					{
//						// If it's already fallen all the way, just leave it there.
//						model_view *= RotateZ(70.0);
//					}
//				}
//
//			}
//			else
//			{
//				// The first domino in the section. In this case, we don't
//				// need to check previous dominos.
//				if(straight_arrays[j][i].isFalling() && !straight_arrays[j][i].isImmovable())
//				{
//					if(zRot < 70.0)
//					{
//						model_view *= RotateZ(7.0 * (straight_arrays[j][i].getTime() % 11));
//						straight_arrays[j][i].setZRotation(7.0 * (straight_arrays[j][i].getTime() % 11));
//					}
//					else
//					{
//						model_view *= RotateZ(70.0);
//					}
//				}
//			}
//
//			// The pick id of each domino will be set so
//			// that its index in its array can be retrieved.
//			setPickId((j * NUM_DOMS) + i + 1);
//
//			// Set the uniform variable on the GPU if the domino has been
//			// right clicked, so the coloring is done in the GPU.
//			if(straight_arrays[j][i].isImmovable())
//			{
//				glUniform1i(glGetUniformLocation(program, "Immovable"), 1);
//			}
//			else
//			{
//				glUniform1i(glGetUniformLocation(program, "Immovable"), 0);
//			}
//
//			glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
//			glDrawArrays(GL_TRIANGLES, 0, num_vertices);
//
//			model_view = mvstack.pop();
//		}
//	}
//}
//
///*
//===========================================================
//draw_spirals(): Helper function to draw the spirals of
//				dominos each frame. Invoked in display().
//===========================================================
//*/
//static void draw_spirals()
//{
//	// The spiral can't be "in the midst" of the straight sections.
//	model_view *= Translate(STARTX + 27.5, 0.0, 0.0);
//
//	// Because of the way the spirals are set up, we perform
//	// a similar operation as the straights: Draw the nth
//	// domino in each spiral before moving on to the (n+1)th.
//	for(int i = 0; i < NUM_DOMS_SPIRAL; i += 5)
//	{	
//		for(int j = 0; j < 5; ++j)
//		{
//			mvstack.push(model_view);
//
//			vec3 pos = spiral_doms[i + j].getPosition();
//			GLfloat yRot = spiral_doms[i + j].getYRotation();
//			GLfloat zRot = spiral_doms[i + j].getZRotation();
//
//			model_view *= RotateY(yRot);
//
//			// The domino is rotated first, so we just need 
//			// to move it along one axis.
//			model_view *= Translate(-10, pos.y, 0.0);
//
//			// Rotate so they're not facing inward.
//			model_view *= RotateY(-90);
//
//			/*
//			=================================================
//			The rest of the code in this function performs
//			the analogous operation to the straight sections.
//			=================================================
//			*/
//			if(i > 0)
//			{
//				GLfloat prevZRot = spiral_doms[(i + j) - 5].getZRotation();
//				if((prevZRot < -60.0 || spiral_doms[i + j].isFalling()) && 
//					!spiral_doms[i + j].isImmovable())
//				{
//					spiral_doms[i + j].setFalling(true);
//					if(zRot > -90.0)
//					{
//						model_view *= RotateZ(-7.0 * (spiral_doms[i + j].getTime() % 11));
//						spiral_doms[i + j].setZRotation((-7.0 * (spiral_doms[i + j].getTime() % 11)) - 30);
//					}
//					else
//					{
//						model_view *= RotateZ(-90.0);
//					}
//				}
//			}
//			else
//			{
//				if((straight_arrays[spiral_mapping[j][1]][0].getZRotation() > 40.0 || 
//					spiral_doms[j].isFalling()) && !spiral_doms[j].isImmovable())
//				{
//					spiral_doms[j].setFalling(true);
//					if(zRot > -90.0)
//					{
//						model_view *= RotateZ(-7.0 * (spiral_doms[j].getTime() % 11));
//						spiral_doms[j].setZRotation((-7.0 * (spiral_doms[j].getTime() % 11)) - 30);
//					}
//					else
//					{
//						model_view *= RotateZ(-90.0);
//					}
//				}
//			}
//
//			setPickId((5 * NUM_DOMS) + i + j + 1);
//
//			if(spiral_doms[i + j].isImmovable())
//			{
//				glUniform1i(glGetUniformLocation(program, "Immovable"), 1);
//			}
//			else
//			{
//				glUniform1i(glGetUniformLocation(program, "Immovable"), 0);
//			}
//
//			glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
//			glDrawArrays(GL_TRIANGLES, 0, num_vertices);
//
//			model_view = mvstack.pop();
//		}
//		
//	}
//	model_view = mvstack.pop();
//}
//
///*
//===========================================================
//display(): Callback function that is invoked each frame.
//		   Draws each domino on the screen with the 
//		   appropriate transformations applied.
//===========================================================
//*/
//static void display()
//{
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//	
//	mvstack.push(model_view);
//
//	// Fit the entire model in the scene at first.
//	model_view *= Scale(0.5, 0.5, 0.5);
//
//	// Perform the transformations needed from key presses.
//	model_view *= Scale(scaleFactor, scaleFactor, scaleFactor);
//	model_view *= RotateX(sceneXRotation);
//	model_view *= RotateY(sceneYRotation);
//
//	// Display the straight sections.
//	draw_straights();
//
//	mvstack.push(model_view);
//	
//	// Display the spiral sections.
//	draw_spirals();
//
//	model_view = mvstack.pop();
//	
//	if (inPickingMode()) {
//		endPicking();
//	}
//	else {
//		glutSwapBuffers();
//	}
//}
//
///*
//===========================================================
//mouse(): Mouse picking callback function; will initiate
//		 a "pick" so that we can interact with the scene.
//===========================================================
//*/
//static void mouse(int button, int state, int x, int y)
//{
//	if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
//	{
//		right_clicked = false;
//		startPicking(picking_callback, x, y);
//	}
//	else if(button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
//	{
//		right_clicked = true;
//		startPicking(picking_callback, x, y);
//	}
//}
//
//static void mouseMove(int x, int y)
//{
//	if(x < mouseX)
//	{
//		sceneYRotation -= 1.25;
//		
//	}
//	else if(x > mouseX)
//	{
//		sceneYRotation += 1.25;
//	}
//
//	if(y < mouseY)
//	{
//		sceneXRotation -= 1.25;
//	}
//	else if(y > mouseY)
//	{
//		sceneXRotation += 1.25;
//	}
//	mouseX = x;
//	mouseY = y;
//}
//
///*
//===========================================================
//reshape(): Callback function invoked when the window is
//		   resized. Every object is resize accordingly. It
//		   will perform a standard resize of the object so
//		   that it grows proportionally with the smaller
//		   of the two window dimensions.
//===========================================================
//*/
//static void reshape(int width, int height)
//{
//
//	glViewport(0, 0, width, height);
//
//	GLfloat left = -10.0;
//	GLfloat right = 10.0;
//	GLfloat bottom = -5.0;
//	GLfloat top = 15.0;
//	GLfloat zNear = -25.0;
//	GLfloat zFar = 25.0;
//	GLfloat aspect = GLfloat(width)/height;
//
//	if(aspect > 1.0)
//	{
//		left *= aspect;
//		right *= aspect;
//	}
//	else
//	{
//		bottom /= aspect;
//		top /= aspect;
//	}
//
//	// set up an orthographic projection
//	mat4 projection = Ortho(left, right, bottom, top, zNear, zFar);
//	glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);
//
//	// begin with no transformations
//	model_view = mat4(1.0);
//}
//
///*
//===========================================================
//init(): This function initializes uniform and attribute 
//		variables for the GPU shaders. The function also
//		defines the vertex and fragment shaders to be 
//		executed on the GPU.
//===========================================================
//*/
//static void init()
//{
//	// Create the vertices for a single domino
//	// we only need one on the GPU
//	color_domino();
//
//	// Create a vertex array object
//	GLuint vao;
//	glGenVertexArrays(1, &vao);
//	glBindVertexArray(vao);
//
//	// Create and initialize a buffer object
//	GLuint buffer;
//	glGenBuffers(1, &buffer);
//	glBindBuffer(GL_ARRAY_BUFFER, buffer);
//	glBufferData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(normals),
//					NULL, GL_DYNAMIC_DRAW);
//	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
//	glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(normals), normals);
//
//	// Load the shaders and the shader program
//	const GLchar* vertex_shader = 
//		"attribute vec4 vPosition; "
//		"attribute vec4 vNormal; "
//		"varying vec4 color; "
//		" "
//		"uniform mat4 ModelView; "
//		"uniform mat4 Projection; "
//		"uniform vec4 PickColor; "
//		"uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;"
//		"uniform vec4 LightPosition;"
//		"uniform float Shininess;"
//		"uniform bool Immovable;"
//		" "
//		"void main() "
//		"{ "
//		"if(PickColor.a >= 0.0) "
//		"{"
//		"	color = PickColor; "
//		"}"
//		"else if(Immovable)"
//		"{"
//		"	color = vec4(0.0, 1.0, 0.0, 1.0);"
//		"}"
//		"else{"
//		"	vec3 pos = (ModelView * vPosition).xyz;"
//		" "
//		"	vec3 L = normalize((ModelView*LightPosition).xyz - pos);"
//		"	vec3 E = normalize(-pos);"
//		"	vec3 H = normalize(L + E);"
//		" "
//		"	vec3 N = normalize(ModelView*vNormal).xyz;"
//		"	vec4 ambient = AmbientProduct;"
//		"	float Kd = max(dot(L, N), 0.0);"
//		"	vec4 diffuse = Kd*DiffuseProduct;"
//		"	float Ks = pow(max(dot(N, H), 0.0), Shininess);"
//		"	vec4 specular = Ks*SpecularProduct;"
//		" "
//		"	if(dot(L, N) < 0.0)"
//		"	{"
//		"		specular = vec4(0.0, 0.0, 0.0, 1.0);"
//		"	}"
//		"	color = ambient + diffuse + specular;"
//		"	color.a = 1.0;"
//		"}"
//		"gl_Position = Projection*ModelView*vPosition; "
//		"} ";
//
//	const GLchar* fragment_shader =
//		"varying vec4 color; "
//		"void main() "
//		"{ "
//		"	gl_FragColor = color; "
//		"} ";
//
//	program = InitShader2(vertex_shader, fragment_shader);
//
//	glUseProgram(program);
//
//	GLuint vPosition = glGetAttribLocation(program, "vPosition");
//	glEnableVertexAttribArray(vPosition);
//	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 
//							BUFFER_OFFSET(0));
//
//	GLuint vNormal = glGetAttribLocation(program, "vNormal");
//	glEnableVertexAttribArray(vNormal);
//	glVertexAttribPointer(vNormal, 4, GL_FLOAT, GL_FALSE, 0, 
//							BUFFER_OFFSET(sizeof(normals)));
//
//	ModelView = glGetUniformLocation(program, "ModelView");
//	Projection = glGetUniformLocation(program, "Projection");
//	setGpuPickColorId(glGetUniformLocation(program, "PickColor"));
//
//	glUniform4fv(glGetUniformLocation(program, "AmbientProduct"),
//					1, ambient_product);
//	glUniform4fv(glGetUniformLocation(program, "DiffuseProduct"),
//					1, diffuse_product);
//	glUniform4fv(glGetUniformLocation(program, "SpecularProduct"),
//					1, specular_product);
//	glUniform4fv(glGetUniformLocation(program, "LightPosition"),
//					1, light_position);
//	glUniform1f(glGetUniformLocation(program, "Shininess"),
//					material_shininess);
//	glUniform1i(glGetUniformLocation(program, "Immovable"), 0);
//
//	glClearColor(0.1, 0.1, 0.1, 1.0); // dark gray background
//
//	// Enable hidden object elimination
//	glEnable(GL_DEPTH_TEST);
//
//	// Gimme some filled in polygons
//	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//
//	// Initialize each domino array, lining up the dominos
//	// in each array nicely (numbers derived from trial and error).
//	for(int i = 0; i < NUM_DOMS; ++i)
//	{
//		straight_doms[i] = Domino(vec3((STARTX + (i * 1.5)), STARTY, -6.0), 0.0, 0.0);
//	}
//
//	for(int i = 0; i < NUM_DOMS; ++i)
//	{
//		straight_doms2[i] = Domino(vec3((STARTX + (i * 1.5)), STARTY, -6.0), 0.0, 0.0);
//	}
//
//	for(int i = 0; i < NUM_DOMS; ++i)
//	{
//		straight_doms3[i] = Domino(vec3((STARTX + (i * 1.5)), STARTY, -6.0), 0.0, 0.0);
//	}
//
//	for(int i = 0; i < NUM_DOMS; ++i)
//	{
//		straight_doms4[i] = Domino(vec3((STARTX + (i * 1.5)), STARTY, -6.0), 0.0, 0.0);
//	}
//
//	for(int i = 0; i < NUM_DOMS; ++i)
//	{
//		straight_doms5[i] = Domino(vec3((STARTX + (i * 1.5)), STARTY, -6.5), 0.0, 0.0);
//	}
//
//	for(int i = 0; i < 5; ++i)
//	{
//		for(int j = i; j < NUM_DOMS_SPIRAL; j += 5)
//		{
//
//			// Perform basic unit circle-esque operations
//			// to rotate each domino to correct rotation.
//			GLfloat xPos = 10 * sin(j * 2.5);
//			GLfloat zPos = 10 * cos(j * 2.5);
//			GLfloat yPos = STARTY - (j * 0.10);
//
//			spiral_doms[j] = Domino(vec3(xPos, yPos, zPos), 0.0, 0.0);
//
//			spiral_doms[j].setZRotation(-30);
//
//			// expand to [-180,+180] range
//			GLfloat theta = 2 * atan(zPos/xPos) * (180/M_PI);
//			
//			spiral_doms[j].setYRotation(theta);
//		}
//	}
//
//}
//
///*
//===========================================================
//keyboard(): Keyboard callback function used for navigating
//			around scene.
//===========================================================
//*/
//static void keyboard(unsigned char key, int x, int y)
//{
//	// Numbers derived via trial and error.
//	switch(key)
//	{
//	case 'w': 
//		sceneXRotation -= 3;
//		break;
//	case 'W':
//		sceneXRotation -= 15;
//		break;
//	case 'S':
//		sceneXRotation += 15;
//		break;
//	case 's':
//		sceneXRotation += 3;
//		break;
//	case 'd':
//		sceneYRotation += 3;
//		break;
//	case 'D':
//		sceneYRotation += 15;
//		break;
//	case 'A':
//		sceneYRotation -= 15;
//		break;
//	case 'a':
//		sceneYRotation -= 3;
//		break;
//	case 'c':
//		scaleFactor += 0.05;
//		break;
//	case 'z':
//		scaleFactor -= 0.05;
//		break;
//
//	}
//}
//
///*
//===========================================================
//tick(): Callback function invoked on each clock tick; used
//		to animate the scene at a fixed frame rate.
//===========================================================
//*/
//static void tick(int n)
//{
//	for(int i = 0; i < NUM_DOMS_SPIRAL; ++i)
//	{
//		// Update each domino objects' "time". The time
//		// is only started if the domino has started falling.
//		if(i < NUM_DOMS)
//		{
//			if(straight_doms[i].isFalling())
//			{
//				straight_doms[i].setTime(straight_doms[i].getTime() + 1);
//			}
//			if(straight_doms2[i].isFalling())
//			{
//				straight_doms2[i].setTime(straight_doms2[i].getTime() + 1);
//			}
//			if(straight_doms3[i].isFalling())
//			{
//				straight_doms3[i].setTime(straight_doms3[i].getTime() + 1);
//			}
//			if(straight_doms4[i].isFalling())
//			{
//				straight_doms4[i].setTime(straight_doms4[i].getTime() + 1);
//			}
//			if(straight_doms5[i].isFalling())
//			{
//				straight_doms5[i].setTime(straight_doms5[i].getTime() + 1);
//			}
//		}
//		if(spiral_doms[i].isFalling())
//		{
//			spiral_doms[i].setTime(spiral_doms[i].getTime() + 1);
//		}
//	}
//
//
//	// Well, we want another tick to occur
//	glutTimerFunc(n, tick, n);
//
//	// Draw the next frame
//	glutPostRedisplay();
//}
//
//
///*
//===========================================================
//main(): This function is where the program starts. It sets
//		up the environment and callbacks and initializes
//		the initialization.
//===========================================================
//*/
//int mainpoop(int argc, char* argv[])
//{
//	glutInit(&argc, argv);
//	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
//	glutInitWindowSize(WINDOW_WIDTH, WINDOW_WIDTH);
//	glutCreateWindow("Domino Rally");
//
//	// Running on the PC, so we need this call
//	glewInit();
//
//	init();
//
//	glutDisplayFunc(display);
//	glutReshapeFunc(reshape);
//	glutKeyboardFunc(keyboard);
//	glutMouseFunc(mouse);
//	glutMotionFunc(mouseMove);
//	glutTimerFunc(TICK_INTERVAL, tick, TICK_INTERVAL);
//
//	glutMainLoop();
//
//	return 0;
//}
