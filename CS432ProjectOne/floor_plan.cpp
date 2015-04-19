// Assignment 5
// Author: Daniel Ehmig
// Date Completed: 28 November 2014
// Some code derived from the demo
// code written by Dr. Vegdahl and Angel.

/*
===========================================================
BASIC DIRECTIONS

'w' key: Move your character forwards
's' key: Move your character backwards
'a' key: Strafe left
'd' key: Strafe right
'e' key: Interact with objects (the two doors, the lanterns
		 on the wall, the living room light, and the
		 bedroom light).

		 ***You need to be VERY CLOSE to the outside
		 lanterns to interact with them.***

Mouse Movement: Move the camera around (i.e., change where
				the character is looking)
===========================================================
*/

#include "cs432.h"
#include "vec.h"
#include "mat.h"
#include "matStack.h"
#include "picking.h"
#include "blobject.h"
#include "objparser.h"
#include "ppm.h"
#include <assert.h>
#include <fstream>
#include <set>

// Typedefs of vec4 for readability
typedef vec4 point4;
typedef vec4 color4;

// The tick interval for animations, units in milliseconds
#define TICK_INTERVAL 30

// Basically, how high the camera is off of the ground
#define FPS_HEIGHT 4.0

// I hope to God that whatever screen is being used
// doesn't exceed this resolution
#define WINDOW_HEIGHT 1600
#define WINDOW_WIDTH 2000

#define NUM_VERTICES 100000

#define TEX_HEIGHT 512
#define TEX_WIDTH 512

bool NO_TEXTURES = false;

// Variables for matrix operations
static MatrixStack mvstack;
static mat4 model_view;
static GLuint ModelView, Projection, AmbientProduct, 
	DiffuseProduct, SpecularProduct, Shininess, Dissolve,
	LanternOn, LivingOn, BedroomOn, Moon, DiffMoonProduct, SpecMoonProduct;
static GLuint Colliding;
static GLuint textures[7];
static GLubyte image[TEX_HEIGHT][TEX_WIDTH][3];
static GLubyte concrete[TEX_HEIGHT][TEX_WIDTH][3];
static GLubyte granite[TEX_HEIGHT][TEX_WIDTH][3];
static GLubyte garage[TEX_HEIGHT][TEX_WIDTH][3];
static GLubyte grass[TEX_HEIGHT][TEX_WIDTH][3];
static GLubyte stars[2048][2048][3];
static GLubyte wall[1024][2048][3];

// The positions (in x,y,z format) of the four
// interactive lights in the scene plus the street light and moon
GLfloat light_positions[18] = {-6.5, 4.5, -14.75, -9.4, 4.45, -9.7,
	-9.9, 4.73, 4.7, -7.65, 4.5, 13.5, -11.33, 6.75, -41.57, -45.0, 65.0, 40.0};

color4 light_ambient(0.2, 0.2, 0.8, 1.0);
color4 light_diffuse(1.0, 1.0, 1.0, 1.0);
color4 light_specular(1.0, 1.0, 1.0, 1.0);

color4 moon_diffuse(0.2, 0.2, 1.0, 1.0);
color4 moon_specular(0.2, 0.2, 1.0, 1.0);

color4 material_ambient(0.2, 0.2, 0.2, 1.0);
color4 material_diffuse(0.8, 0.05, 0.05, 1.0);
color4 material_specular(1.0, 0.0, 0.0, 1.0);
color4 mat_specular_immovable(0.0, 1.0, 0.0, 1.0);
float material_shininess = 50.0;
float dissolve = 1.0;

color4 ambient_product = material_ambient * light_ambient;
color4 diffuse_product = material_diffuse * light_diffuse;
color4 specular_product = material_specular * light_specular;
color4 moon_diff = material_diffuse * moon_diffuse;
color4 moon_spec = material_specular * moon_specular;
color4 spec_immovable;

// Holds the program identifier; we need to refer to it in several functions
GLuint program;

// Store the current height and width of the window,
// as the user might reshape it
static GLint currentHeight;
static GLint currentWidth;

static GLint mouseX = WINDOW_WIDTH/2;
static GLint mouseY = WINDOW_HEIGHT/2;

// Variables to store various pieces of information
// about the user's current position and orientation
static GLfloat sceneXRotation = 0.0;
static GLfloat sceneYRotation = 180.0;
static GLfloat currentX = 0.0;
static GLfloat currentZ = -100.0;
static GLfloat currentY = FPS_HEIGHT;
static GLfloat prevX = 0.0;
static GLfloat prevY = FPS_HEIGHT;
static GLfloat prevZ = -100.0;
static GLfloat atX = 0.0;
static GLfloat atY = 3.5;
static GLfloat atZ = 20.0;
GLfloat gunXRotation = 0.0;
bool gun_firing_up = false;
bool gun_firing_down = false;

vec4 * vertices;

// These are our three primary buffers that are sent to
// the graphics card during initialization
static vec3 normals[NUM_VERTICES];
static point4 points[NUM_VERTICES];
static vec2 tex_coords[NUM_VERTICES];

long vertex_count;
static long point_count = 21441;
long normal_count;

// Store the objects that were parsed during initialization
Blobject * objects;
GLint object_count = 0;

// Door interaction variables
bool front_closed = true;
bool garage_closed = true;
GLfloat front_slide = 0;
GLfloat garage_slide = 0;

// Light interaction variables
bool lantern_on = true;
bool living_on = false;
bool bedroom_on = false;

static void mouse(int button, int state, int x, int y)
{
	if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		if(!(gun_firing_up || gun_firing_down))
		{
			gun_firing_up = true;
		}
		else if(gun_firing_down)
		{
			gun_firing_down = false;
			gun_firing_up = true;
		}
		gunXRotation += 5.0;
	}
}

/* 
===========================================================
dist2D(): Calculate the distance between two points on a 
		  single plane using simple trigonometry.
===========================================================
*/
static GLfloat dist2D(vec2 p1, vec2 p2)
{
	GLfloat diffx = fabs(p2.x - p1.x);
	GLfloat diffz = fabs(p2.y - p1.y);

	return diffz/(sin(atanf(diffz/diffx)));
}

/*
===========================================================
has_collided(): Create a small ortho view clipping volume
				around the character. Then, check a sample
				of the pixels to see if they are the 
				background color. If not, we have clipped
				an object, so the user cannot move in that
				direction.
===========================================================
*/
static bool has_collided()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLfloat char_clip_height = currentHeight/500.0;
	GLfloat char_clip_width = currentWidth/1000.0;

	// Set a clipping volume around the character to check if any polygons
	// have "clipped" the character. The "bottom" is 0.25 to ignore the ground
	mat4 projection = Ortho(-char_clip_width * 0.8, char_clip_width * 0.8, -char_clip_height, char_clip_height, 0.0, 1.25);
	glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);
	glUniform1i(Colliding, 1);
	GLint i = 0;
	GLint vertex_num = 0;
	for(i = 0; i < object_count; ++i)
	{
		model_view = mat4(1.0);
		material mat = objects[i].get_material();

		// Transform the two doors based on their current status
		// i.e. - we want to go through an open door!
		if((objects[i].get_name().compare("FrontDoor_Cube.013")) == 0)
		{
			vec2 cent = objects[i].getXZCenter();
			model_view = Translate(cent.x + front_slide, 0.0, cent.y);
		}
		else if((objects[i].get_name().compare("GarageDoor_Cube.009")) == 0)
		{
			model_view = Translate(0.0, -garage_slide, 0.0);
		}
		model_view = Translate((-currentX)/5.0, -FPS_HEIGHT, (-currentZ)/5.0) * model_view;
		model_view = RotateY(sceneYRotation) * model_view;
		model_view = RotateX(sceneXRotation) * model_view;

		glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
		glDrawArrays(GL_TRIANGLES, vertex_num, objects[i].get_num_faces() * 3);
		vertex_num += objects[i].get_num_faces() * 3;
	}

	GLubyte pixel[5][4];

	// Test a sample of pixels on the screen to detect for collisions
	glReadPixels(currentWidth/2, currentHeight/2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel[0]);
	glReadPixels(0, currentHeight/2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel[1]);
	glReadPixels(currentWidth - 1, currentHeight/2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel[2]);
	glReadPixels(currentWidth/2, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel[3]);
	glReadPixels(currentWidth/2, currentHeight - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel[4]);
	
	GLubyte red;
	GLubyte green;
	GLubyte blue;
	for(i = 0; i < 5; ++i)
	{
		red = pixel[i][0];
		green = pixel[i][1];
		blue = pixel[i][2];

		// Test rgb values for the background color
		if(!(red == 0 && green == 0 && blue == 0))
		{
			GLfloat aspect = GLfloat(currentWidth)/currentHeight;
			projection = Perspective(80,aspect,1.0,20000);
			glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);
			glUniform1i(Colliding, 0);
			return true;
		}
	}

	GLfloat aspect = GLfloat(currentWidth)/currentHeight;
	projection = Perspective(80,aspect,1.0,20000);
	glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);
	glUniform1i(Colliding, 0);
	return false;
}


/*
===========================================================
display(): Callback function that is invoked each frame.
		   Draws each object on the screen with their 
		   correct material and transformation.
===========================================================
*/

std::set<std::string> gun_names;

static void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Transformations must occur in this order so the 
	// "FPS" style camera works correctly
	model_view = Translate((-currentX)/5.0, -FPS_HEIGHT, (-currentZ)/5.0);
	model_view = RotateY(sceneYRotation) * model_view;
	model_view = RotateX(sceneXRotation) * model_view;

	GLint i = 0;	
	GLint vertex_num = 0;
	for(i = 0; i < object_count; ++i)
	{
		material mat = objects[i].get_material();
		ambient_product = light_ambient * mat.Ka;
		diffuse_product = light_diffuse * mat.Kd;
		specular_product = light_specular * mat.Ks;
		material_shininess = mat.Ns;
		dissolve = mat.d;

		moon_diff = moon_diffuse * mat.Kd;
		moon_spec = moon_spec * mat.Ks;

		glUniform4fv(DiffMoonProduct, 1, moon_diff);
		glUniform4fv(SpecMoonProduct, 1, moon_spec);

		glUniform4fv(AmbientProduct, 1, ambient_product);
		glUniform4fv(DiffuseProduct, 1, diffuse_product);
		glUniform4fv(SpecularProduct, 1, specular_product);
		glUniform1f(Shininess, material_shininess);
		glUniform1f(Dissolve, dissolve);

		std::set<std::string>::iterator it = gun_names.find(objects[i].get_name());

		// The doors need to be animated based on their current status
		// i.e. - opening, closing, opened, and closed
		if(objects[i].get_name().compare("FrontDoor_Cube.013") == 0)
		{
			mvstack.push(model_view);

			vec2 cent = objects[i].getXZCenter();

			model_view = Translate(cent.x + front_slide, 0.0, cent.y);
			model_view = Translate(-(currentX/5.0), -FPS_HEIGHT, -(currentZ/5.0)) * model_view;
			model_view = RotateY(sceneYRotation) * model_view;
			model_view = RotateX(sceneXRotation) * model_view;

			glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
			model_view = mvstack.pop();
		}
		else if((objects[i].get_name().compare("GarageDoor_Cube.009")) == 0)
		{
			mvstack.push(model_view);
			model_view = Translate(-(currentX/5.0), -FPS_HEIGHT - garage_slide, -(currentZ/5.0));
			model_view = RotateY(sceneYRotation) * model_view;
			model_view = RotateX(sceneXRotation) * model_view;

			glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
			model_view = mvstack.pop();
		}
		else if(it != gun_names.end())
		{
			mvstack.push(model_view);

			vec2 center = objects[i].getXZCenter();

			model_view = RotateY(180);
			model_view = Translate(+9.0, -9.0, -18.0) * model_view;
			model_view = RotateX(gunXRotation) * model_view;
			glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
			model_view = mvstack.pop();
		}
		else
		{
			glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
		}

		if(objects[i].is_textured())
		{
			glUniform1i(glGetUniformLocation(program, "doTex"), 1);
			glActiveTexture(GL_TEXTURE0);

			// Make sure the objects receive the correct texture
			if(mat.texture.compare("tile.jpg") == 0)
			{
				glBindTexture(GL_TEXTURE_2D, textures[0]);
			}
			else if(mat.texture.compare("concrete.jpg") == 0)
			{
				glBindTexture(GL_TEXTURE_2D, textures[1]);
			}
			else if(mat.texture.compare("granite.jpg") == 0)
			{
				glBindTexture(GL_TEXTURE_2D, textures[3]);
			}
			else if(mat.texture.compare("garage.jpg") == 0)
			{
				glBindTexture(GL_TEXTURE_2D, textures[2]);
			}
			else if(mat.texture.compare("grass.jpg") == 0)
			{
				glBindTexture(GL_TEXTURE_2D, textures[4]);
			}
			else if(mat.texture.compare("stars.png") == 0)
			{
				glBindTexture(GL_TEXTURE_2D, textures[5]);
			}
			else if(mat.texture.compare("wall.jpg") == 0)
			{
				glBindTexture(GL_TEXTURE_2D, textures[6]);
			}
			else
			{
				// Actually, don't texture (even if is_textured()) because
				// it will get the wrong texture
				glUniform1i(glGetUniformLocation(program, "doTex"), 0);
			}
		}
		else
		{
			glUniform1i(glGetUniformLocation(program, "doTex"), 0);
		}
		if(NO_TEXTURES)
		{
			glUniform1i(glGetUniformLocation(program, "doTex"), 0);
		}
		if(objects[i].get_name().compare("Sphere") == 0)
		{
			glUniform1i(Moon, 1);
		}
		else
		{
			glUniform1i(Moon, 0);
		}
		glDrawArrays(GL_TRIANGLES, vertex_num, objects[i].get_num_faces() * 3);

		vertex_num += objects[i].get_num_faces() * 3;
	}
	glutSwapBuffers();	
	
}

/*
===========================================================
mouseMove(): Mouse movement callback function that is 
			 invoked whenever the mouse experiences any
			 passive motion in the window. The cursor
			 will "wrap" around the screen whenever it
			 hits a boundary. The window is full screen so
			 that the mouse cursor does not "jump" over
			 the boundary, so to speak.
===========================================================
*/
static void mouseMove(GLint x, GLint y)
{
	GLint delta_x = abs(mouseX - x);
	GLint delta_y = abs(mouseY - y);

	// Set the rotation of the camera depending on
	// the speed that the user moves the mouse
	if(mouseX > x)
	{
		sceneYRotation -= delta_x/20.0;
	}
	else if(mouseX < x)
	{
		sceneYRotation += delta_x/20.0;
	}

	if(mouseY > y)
	{
		sceneXRotation -= delta_y/20.0;
	}
	else if(mouseY < y)
	{
		sceneXRotation += delta_y/20.0;
	}

	mouseX = x;
	mouseY = y;

	bool changed = false;


	// Wrap the (hidden) mouse cursor around the screen
	// when it hits the edge, so that the user can 
	// continue to rotate the screen without interruption
	if(mouseX >= (currentWidth - 10))
	{
		mouseX = 20;
		changed = true;
	}
	else if(mouseX <= 10)
	{
		mouseX = currentWidth - 20;
		changed = true;
	}

	if(mouseY >= (currentHeight-10))
	{
		mouseY = 20;
		changed = true;
	}
	else if(mouseY <= 10)
	{
		mouseY = currentHeight - 20;
		changed = true;
	}
	if(changed)
	{
		glutWarpPointer(mouseX, mouseY);
	}
	
}

/*
===========================================================
initialize_images(): Helper function that parses each ppm
					 file for the texture images and scales
					 them accordingly.
===========================================================
*/
static void initialize_images()
{
	// The following could possibly go into a loop,
	// but each chunk is different enough to ensure
	// that it would be more trouble than it is worth
	const GLuint image_width = 636;
	const GLuint image_height = 640;
	static GLfloat pic[image_height][image_width][3];

	readPpmImage("tile.ppm", (GLfloat*)pic, 0, 0, image_width, image_height);
	gluScaleImage(
				GL_RGB,
				image_width,
				image_height,
				GL_FLOAT,
				pic,
				TEX_WIDTH,
				TEX_HEIGHT,
				GL_BYTE,
				image);

	const GLuint conc_width = 500;
	const GLuint conc_height = 375;
	static GLfloat pic1[conc_height][conc_width][3];

	readPpmImage("concrete.ppm", (GLfloat*)pic1, 0, 0, conc_width, conc_height);
	gluScaleImage(
				GL_RGB,
				conc_width,
				conc_height,
				GL_FLOAT,
				pic1,
				TEX_WIDTH,
				TEX_HEIGHT,
				GL_BYTE,
				concrete);

	const GLuint garage_width = 900;
	const GLuint garage_height = 600;
	static GLfloat pic2[garage_height][garage_width][3];

	readPpmImage("garage.ppm", (GLfloat*)pic2, 0, 0, garage_width, garage_height);
	gluScaleImage(
				GL_RGB,
				garage_width,
				garage_height,
				GL_FLOAT,
				pic2,
				TEX_WIDTH,
				TEX_HEIGHT,
				GL_BYTE,
				garage);

	const GLuint granite_width = 600;
	const GLuint granite_height = 399;
	static GLfloat pic3[granite_height][granite_width][3];

	readPpmImage("granite.ppm", (GLfloat*)pic3, 0, 0, granite_width, granite_height);
	gluScaleImage(
				GL_RGB,
				granite_width,
				garage_height,
				GL_FLOAT,
				pic3,
				TEX_WIDTH,
				TEX_HEIGHT,
				GL_BYTE,
				granite);

	const GLuint grass_width = 900;
	const GLuint grass_height = 675;
	static GLfloat pic4[grass_height][grass_width][3];

	readPpmImage("grass.ppm", (GLfloat*)pic4, 0, 0, grass_width, grass_height);
	gluScaleImage(
				GL_RGB,
				grass_width,
				grass_height,
				GL_FLOAT,
				pic4,
				TEX_WIDTH,
				TEX_HEIGHT,
				GL_BYTE,
				grass);
	const GLuint stars_width = 2400;
	const GLuint stars_height = 1800;
	static GLfloat pic5[stars_height][stars_width][3];

	readPpmImage("stars.ppm", (GLfloat*)pic5, 0, 0, stars_width, stars_height);
	gluScaleImage(
				GL_RGB,
				stars_width,
				stars_height,
				GL_FLOAT,
				pic5,
				2048,
				2048,
				GL_BYTE,
				stars);

	const GLuint wall_width = 2560;
	const GLuint wall_height = 1335;
	static GLfloat pic6[wall_height][wall_width][3];
	
	readPpmImage("wall.ppm", (GLfloat*)pic6, 0, 0, wall_width, wall_height);
	gluScaleImage(
				GL_RGB,
				wall_width,
				wall_height,
				GL_FLOAT,
				pic6,
				2048,
				1024,
				GL_BYTE,
				wall);
}

/*
===========================================================
init_textures(): Helper function to initialize the texture
				 properties and objects GPU-side. This 
				 function really just exists so that the
				 init() function looks less cluttered.
===========================================================
*/
static void init_textures()
{
	glGenTextures(7, textures);

	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_WIDTH, TEX_HEIGHT, 0,
					GL_RGB, GL_UNSIGNED_BYTE, image);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_WIDTH, TEX_HEIGHT, 0,
					GL_RGB, GL_UNSIGNED_BYTE, concrete);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, textures[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_WIDTH, TEX_HEIGHT, 0,
					GL_RGB, GL_UNSIGNED_BYTE, garage);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, textures[3]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_WIDTH, TEX_HEIGHT, 0,
					GL_RGB, GL_UNSIGNED_BYTE, granite);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, textures[4]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEX_WIDTH, TEX_HEIGHT, 0,
					GL_RGB, GL_UNSIGNED_BYTE, grass);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, textures[5]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2048, 2048, 0,
					GL_RGB, GL_UNSIGNED_BYTE, stars);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, textures[6]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2048, 1024, 0,
					GL_RGB, GL_UNSIGNED_BYTE, wall);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
}

/*
===========================================================
init(): This function initializes uniform and attribute 
		variables for the GPU shaders. The function also
		defines the vertex and fragment shaders to be 
		executed on the GPU.
===========================================================
*/
static void init()
{	

	std::ifstream conf;
	conf.open("CONF.txt");

	char val = 'a';
	conf.get(val);
	if(val == '0')
	{
		NO_TEXTURES = false;
	}
	else
	{
		NO_TEXTURES = true;
	}

	conf.close();

	// We'll first get the image operations out of the way
	if(!NO_TEXTURES)
	{
		initialize_images();
	}
	
	// First, we parse and load all of our objects
	object_count = ObjParser::parse_objects("CottagePlan.obj", &objects);

	 //Now, we need to get all of the vertices and normals
	GLint i = 0;
	vertex_count = 0;
	normal_count = 0;
	GLint num_faces = 0;
	GLint num_texs = 0;

	for(i = 0; i < object_count; ++i)
	{
		vertex_count += objects[i].get_num_vertices();
		normal_count += objects[i].get_num_normals();
		num_faces += objects[i].get_num_faces();
		num_texs += objects[i].get_num_texs();
	}

	vertices = new vec4[vertex_count];
	point_count = num_faces * 3;
	vec3 * temp_normals = new vec3[normal_count];
	vec2 * temp_texs = new vec2[num_texs];

	GLint v_num = 0;
	GLint n_num = 0;
	GLint t_num = 0;
	GLint index = 0;
	vec4 * verts;
	vec3 * norms;
	vec2 * texs;
	for(i = 0; i < object_count; ++i)
	{
		verts = objects[i].get_vertices();
		norms = objects[i].get_normals();
		texs = objects[i].get_texs();

		long count = objects[i].get_num_vertices();
		for(index = 0; index < count; ++index)
		{
			vertices[v_num] = verts[index];
			++v_num;
		}

		count = objects[i].get_num_normals();
		for(index = 0; index < count; ++index)
		{
			temp_normals[n_num] = norms[index];
			++n_num;
		}

		// If the object isn't textured, count will be 0
		count = objects[i].get_num_texs();
		for(index = 0; index < count; ++index)
		{
			temp_texs[t_num] = texs[index];
			++t_num;
		}
	}

	v_num = 0;	
	// Now, we have to parse the faces and actually
	// assign the vertices, normals, and tex coords to points
	for(i = 0; i < object_count; ++i)
	{
		face * obj_faces = objects[i].get_faces();
		num_faces = objects[i].get_num_faces();

		if((objects[i].get_name().compare("FrontDoor_Cube.013")) == 0)
		{
			vec2 cent = objects[i].getXZCenter();

			GLint j = 0;
			for(j = 0; j < num_faces; ++j)
			{
				vec4 p1 = vertices[obj_faces[j].v_index1];
				vec4 p2 = vertices[obj_faces[j].v_index2];
				vec4 p3 = vertices[obj_faces[j].v_index3];

				p1.x -= cent.x;
				p1.z -= cent.y;
				p2.x -= cent.x;
				p2.z -= cent.y;
				p3.x -= cent.x;
				p3.z -= cent.y;

				points[v_num] = p1;
				normals[v_num] = temp_normals[obj_faces[j].n_index1];
				tex_coords[v_num] = vec2(-1.0, -1.0);
				++v_num;

				points[v_num] = p2;
				normals[v_num] = temp_normals[obj_faces[j].n_index2];
				tex_coords[v_num] = vec2(-1.0, -1.0);
				++v_num;

				points[v_num] = p3;
				normals[v_num] = temp_normals[obj_faces[j].n_index3];
				tex_coords[v_num] = vec2(-1.0, -1.0);
				++v_num;
			}
		}
		else
		{
			GLint j = 0;
			for(j = 0; j < num_faces; ++j)
			{
				points[v_num] = vertices[obj_faces[j].v_index1];
				normals[v_num] = temp_normals[obj_faces[j].n_index1];
				++v_num;

				points[v_num] = vertices[obj_faces[j].v_index2];
				normals[v_num] = temp_normals[obj_faces[j].n_index2];
				++v_num;

				points[v_num] = vertices[obj_faces[j].v_index3];
				normals[v_num] = temp_normals[obj_faces[j].n_index3];
				++v_num;

				if(objects[i].is_textured())
				{
					tex_coords[v_num - 3] = temp_texs[obj_faces[j].t_index1];
					tex_coords[v_num - 2] = temp_texs[obj_faces[j].t_index2];
					tex_coords[v_num - 1] = temp_texs[obj_faces[j].t_index3];
				}
				else
				{
					tex_coords[v_num - 3] = vec2(-1.0, -1.0);
					tex_coords[v_num - 2] = vec2(-1.0, -1.0);
					tex_coords[v_num - 1] = vec2(-1.0, -1.0);
				}
			}
		}
		
	}

	// Initialize texture objects
	if(!NO_TEXTURES)
	{
		init_textures();
	}
	
	// Create a vertex array object
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create and initialize a buffer object
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(normals) + sizeof(tex_coords),
					NULL, GL_DYNAMIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(normals), normals);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(normals), sizeof(tex_coords), tex_coords);


	// Load the shaders and the shader program
	const GLchar* vertex_shader = 
		"attribute vec4 vPosition; "
		"attribute vec4 vNormal; "
		"attribute vec2 vTexCoord; "
		"varying vec4 color; "
		"varying vec2 texCoord; "
		" "
		"uniform mat4 ModelView; "
		"uniform mat4 Projection; "
		"uniform vec4 PickColor; "
		"uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct; "
		"uniform vec4 DiffMoonProduct, SpecMoonProduct; "
		"uniform float LightPositions[18]; "
		"uniform float Shininess; "
		"uniform bool Immovable; "
		"uniform bool LanternOn; "
		"uniform bool LivingOn; "
		"uniform bool BedroomOn; "
		"uniform float Dissolve; "
		"uniform bool Moon; "
		" "
		"void main() "
		"{ "
		"vec4 light_pos[6]; "
		"int i = 0; "
		"for(i = 0; i < 18; i+=3) "
		"{ "
		"	light_pos[i/3] = vec4(LightPositions[i], LightPositions[i+1], LightPositions[i+2], 0.0); "
		"} "
		"if(PickColor.a >= 0.0) "
		"{"
		"	color = PickColor; "
		"} "
		"else if(Immovable) "
		"{ "
		"	color = vec4(0.0, 1.0, 0.0, 1.0); "
		"} "
		"else if(Moon) "
		"{ "
		"	color = vec4(0.8, 0.8, 1.0, 1.0); "
		"} "
		"else{ "
		"	vec4 ambient = AmbientProduct; "
		"	vec4 diffuse; "
		"	vec4 specular; "
		"	vec3 pos = (vPosition).xyz; "
		"	vec3 E = normalize(-pos); "
		"	vec3 N = normalize(vNormal).xyz; "
		"	color = ambient; "
		"	for(i = 0; i < 5; i++) "
		"	{ "
		"		vec3 L = normalize((light_pos[i]).xyz - pos); "
		"		vec3 H = normalize(L + E); "
		"		float Kd = max(dot(L, N), 0.0); "
		"		diffuse = Kd*DiffuseProduct; "
		"		float Ks = pow(max(dot(N, H), 0.0), Shininess); "
		"		specular = Ks*SpecularProduct; "
		"		float dist = distance(light_pos[i].xyz, pos); "
		"		dist = 0.05 + (0.05 * dist) + (0.005 * pow(dist, 2)); "
		"		specular /= dist; "
		"		diffuse /= dist; "
		" "
		"		if(dot(L, N) < 0.0) "
		"		{ "
		"			specular = vec4(0.0, 0.0, 0.0, 0.0); "
		"		} "
		"		if((i == 0 || i == 3) && LanternOn) "
		"		{ "
		"			color = color + diffuse + specular; "
		"		} "
		"		else if(i == 1 && LivingOn) "
		"		{ "
		"			color = color + diffuse + specular; "
		"		} "
		"		else if(i == 2 && BedroomOn) "
		"		{ "
		"			color = color + diffuse + specular; "
		"		} "
		"		else if(i == 4) "
		"		{ "
		"			color = color + diffuse + specular; "
		"			color *= 1.2; "
		"		} "
		"	} "
		"	vec3 L = normalize((light_pos[5]).xyz - pos); "
		"	vec3 H = normalize(L + E); "
		"	float Kd = max(dot(L, N), 0.0); "
		"	diffuse = Kd*DiffMoonProduct; "
		"	float Ks = pow(max(dot(N, H), 0.0), Shininess); "
		"	specular = Ks*SpecMoonProduct; "
		"	float dist = distance(light_pos[5].xyz, pos); "
		"	dist = 0.005 + (0.0005 * dist) + (0.0005 * pow(dist, 2)); "
		"	specular /= dist; "
		"	diffuse /= dist; "
		"   color = color + diffuse + specular; "
		" "
		"	if(color.x > 1.0){"
		"		color.x = 1.0;"
		"	}"
		"	if(color.y > 1.0){"
		"		color.y = 1.0;"
		"	}"
		"	if(color.z > 1.0){"
		"		color.z = 1.0;"
		"	}"
		"	color.a = Dissolve; "
		"} "
		"texCoord = vTexCoord; "
		"gl_Position = Projection*ModelView*vPosition; "
		"} ";

	const GLchar* fragment_shader =
		"varying vec4 color; "
		"varying vec2 texCoord; "
		"uniform bool doTex; "
		"uniform bool Colliding; "
		"uniform sampler2D texture; "
		"void main() "
		"{ "
		"	if(doTex) "
		"	{ "
		"		gl_FragColor = color * texture2D(texture, texCoord); "
		"	} "
		"	else "
		"	{ "
		"		gl_FragColor = color; "
		"	} "
		"	if(Colliding) "
		"	{ "
		"		gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); "
		"	} "
		"} ";

	program = InitShader2(vertex_shader, fragment_shader);

	glUseProgram(program);

	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, 
							BUFFER_OFFSET(0));

	GLuint vNormal = glGetAttribLocation(program, "vNormal");
	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, 
							BUFFER_OFFSET(sizeof(points)));

	GLuint vTexCoord = glGetAttribLocation(program, "vTexCoord");
    glEnableVertexAttribArray(vTexCoord);
    glVertexAttribPointer( vTexCoord, 2, GL_FLOAT, GL_FALSE, 0,
						  BUFFER_OFFSET(sizeof(points) + sizeof(normals)));

	glUniform1i(glGetUniformLocation(program, "texture"), 0);
	glUniform1i(glGetUniformLocation(program, "doTex"), 0);
	
	Moon = glGetUniformLocation(program, "Moon");
	ModelView = glGetUniformLocation(program, "ModelView");
	Projection = glGetUniformLocation(program, "Projection");
	setGpuPickColorId(glGetUniformLocation(program, "PickColor"));
	AmbientProduct = glGetUniformLocation(program, "AmbientProduct");
	DiffuseProduct = glGetUniformLocation(program, "DiffuseProduct");
	SpecularProduct = glGetUniformLocation(program, "SpecularProduct");
	Shininess = glGetUniformLocation(program, "Shininess");
	Dissolve = glGetUniformLocation(program, "Dissolve");
	LanternOn = glGetUniformLocation(program, "LanternOn");
	LivingOn = glGetUniformLocation(program, "LivingOn");
	BedroomOn = glGetUniformLocation(program, "BedroomOn");
	SpecMoonProduct = glGetUniformLocation(program, "SpecMoonProduct");
	DiffMoonProduct = glGetUniformLocation(program, "DiffMoonProduct");
	Colliding = glGetUniformLocation(program, "Colliding");

	glUniform4fv(DiffMoonProduct, 1, moon_diff);
	glUniform4fv(SpecMoonProduct, 1, moon_spec);

	glUniform4fv(AmbientProduct, 1, ambient_product);
	glUniform4fv(DiffuseProduct, 1, diffuse_product);
	glUniform4fv(SpecularProduct, 1, specular_product);
	glUniform1fv(glGetUniformLocation(program, "LightPositions"),
					18, light_positions);
	glUniform1f(Shininess, material_shininess);
	glUniform1i(glGetUniformLocation(program, "Immovable"), 0);
	glUniform1i(Moon, 0);

	// Set the outside lanterns to be "on" at first,
	// so that the user can actually see the scene
	glUniform1i(LanternOn, 1);
	glUniform1i(LivingOn, 0);
	glUniform1i(BedroomOn, 0);
	glUniform1f(Dissolve, dissolve);
	glUniform1i(Colliding, 0);

	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
	glLineWidth(1.5);

	glClearColor(0.0, 0.0, 0.0, 1.0); // black background

	// Enable hidden object elimination
	glEnable(GL_DEPTH_TEST);

	// Gimme some filled in polygons
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// The cursor would just get in the way
	glutSetCursor(GLUT_CURSOR_NONE);
	glutWarpPointer(currentWidth/2, currentHeight/2);
}

/*
===========================================================
handleInteraction(): When the 'e' key is pressed, this 
					 function is invoked to determine which
					 object to interact with. Currently,
					 that would consist of five different
					 possibilities: the front door, the 
					 garage door, and three lights. This
					 function causes an interaction with
					 any of those five objects if the user
					 is within a certain distance in the 
					 x-z plane.
===========================================================
*/
static void handleInteraction()
{
	GLint i = 0;
	for(i = 0; i < object_count; ++i)
	{
		Blobject obj = objects[i];
		if((obj.get_name().compare("FrontDoor_Cube.013")) == 0)
		{
			if(dist2D(obj.getXZCenter(), vec2(currentX/5.0, currentZ/5.0)) < 5.0)
			{
				if(front_closed)
				{
					front_closed = false;
					front_slide += 0.06;
				}
				else
				{
					front_closed = true;
					front_slide -= 0.06;
				}
			}
		}
		else if((obj.get_name().compare("GarageDoor_Cube.009")) == 0)
		{
			if(dist2D(obj.getXZCenter(), vec2(currentX/5.0, currentZ/5.0)) < 5.0)
			{
				if(garage_closed)
				{
					garage_closed = false;
					garage_slide += 0.083;
				}
				else
				{
					garage_closed = true;
					garage_slide -= 0.083;
				}
			}
		}
		else if((obj.get_name().compare("LanternObj")) == 0)
		{
			if(dist2D(vec2(light_positions[0], light_positions[2]), vec2(currentX/5.0, currentZ/5.0)) < 2.0 ||
				dist2D(vec2(light_positions[9], light_positions[11]), vec2(currentX/5.0, currentZ/5.0)) < 2.0)
			{
				if(lantern_on)
				{
					lantern_on = false;
					glUniform1i(LanternOn, 0);
				}
				else
				{
					lantern_on = true;
					glUniform1i(LanternOn, 1);
				}
			}
		}
		else if((obj.get_name().compare("LivingRoomLight")) == 0)
		{
			if(dist2D(vec2(light_positions[3], light_positions[5]), vec2(currentX/5.0, currentZ/5.0)) < 5.0)
			{
				if(living_on)
				{
					living_on = false;
					glUniform1i(LivingOn, 0);
				}
				else
				{
					living_on = true;
					glUniform1i(LivingOn, 1);
				}
			}
		}
		else if((obj.get_name().compare("BedroomLight")) == 0)
		{
			// The bedroom light needs a little more "breathing room" (i.e, it's
			// hard to maneuver right next to the light
			if(dist2D(vec2(light_positions[6], light_positions[8]), vec2(currentX/5.0, currentZ/5.0)) < 10.0)
			{
				if(bedroom_on)
				{
					bedroom_on = false;
					glUniform1i(BedroomOn, 0);
				}
				else
				{
					bedroom_on = true;
					glUniform1i(BedroomOn, 1);
				}
			}
		}

	}
}

/*
===========================================================
keyboard(): Keyboard callback function used for navigating
			and interacting within scene.
===========================================================
*/
static void keyboard(unsigned char key, GLint x, GLint y)
{
	GLfloat xrot_rad, yrot_rad;
	prevX = currentX;
	prevY = currentY;
	prevZ = currentZ;

	switch(key)
	{
	case 'w':
		yrot_rad = sceneYRotation * (M_PI/180);
		xrot_rad = sceneXRotation * (M_PI/180);
		currentX += float(sin(yrot_rad));
	    currentZ -= float(cos(yrot_rad));
        currentY -= float(sin(xrot_rad));
		break;
	case 's':
		yrot_rad = sceneYRotation * (M_PI/180);
		xrot_rad = sceneXRotation * (M_PI/180);
		currentX -= float(sin(yrot_rad));
	    currentZ += float(cos(yrot_rad));
        currentY += float(sin(xrot_rad));
		break;
	case 'a':
		yrot_rad = sceneYRotation * (M_PI/180);
		currentX -= float(cos(yrot_rad)) * 0.5;
		currentZ -= float(sin(yrot_rad)) * 0.5;
		break;
	case 'd':
		yrot_rad = sceneYRotation * (M_PI/180);
		currentX += float(cos(yrot_rad)) * 0.5;
		currentZ += float(sin(yrot_rad)) * 0.5;
		break;
	case 'e':
		handleInteraction();
		break;
	case 'q':
		exit(0);
		break;
	}

	// This will tend to have the effect of being "stuck",
	// but since our method of collision detection doesn't
	// account for intersecting with actual polygons, we
	// don't know the normal vector of the object we collided
	// with. Thus, we can't implement any sort of "sliding
	// along the wall" effect
	if(has_collided())
	{
		currentX = prevX;
		currentY = prevY;
		currentZ = prevZ;
	}

	// Trap the user in the scene, so they don't go off
	// into infinity (or lose the house)
	if(currentX > 125.0 || currentX < -125.0)
	{
		currentX = -currentX;
	}

	if(currentZ > 125.0)
	{
		currentZ = -225.0;
	}
	else if(currentZ < -225.0)
	{
		currentZ = 125.0;
	}
}

/*
===========================================================
tick(): Callback function invoked on each clock tick; used
		to animate the scene and the doors at a fixed frame rate.
===========================================================
*/
static void tick(GLint n)
{
	// See if we need to animate the doors
	if(front_slide > 0.0 && front_slide < 2.90)
	{
		if(front_closed)
		{
			front_slide -= 0.06;
		}
		else
		{
			front_slide += 0.06;
		}
	}

	if(garage_slide > 0.0 && garage_slide < 7.5)
	{
		if(garage_closed)
		{
			garage_slide -= 0.083;
		}
		else
		{
			garage_slide += 0.083;
		}
	}

	if(gun_firing_up)
	{
		gunXRotation += 5.0;

		if(gunXRotation > 30.0)
		{
			gun_firing_up = false;
			gun_firing_down = true;
		}
	}
	else if(gun_firing_down)
	{
		gunXRotation -= 5.0;
		
		if(gunXRotation < 0.0)
		{
			gun_firing_down = false;
			gunXRotation = 0.0;
		}
	}


	// Well, we want another tick to occur
	glutTimerFunc(n, tick, n);

	// Draw the next frame
	glutPostRedisplay();
}

/*
===========================================================
reshape(): Callback function invoked when the window is
		   resized. Every object is resize accordingly. It
		   will perform a standard resize of the object so
		   that it grows proportionally with the smaller
		   of the two window dimensions.
===========================================================
*/
static void reshape(GLint width, GLint height)
{
	currentWidth = width;
	currentHeight = height;

	glViewport(0, 0, width, height);

	GLfloat left = -50.0;
	GLfloat right = 50.0;
	GLfloat bottom = -5.0;
	GLfloat top = 15.0;
	GLfloat zNear = -50.0;
	GLfloat zFar = 50.0;
	GLfloat aspect = GLfloat(width)/height;

	if(aspect > 1.0)
	{
		left *= aspect;
		right *= aspect;
	}
	else
	{
		bottom /= aspect;
		top /= aspect;
	}

	// set up a perspective projection
	mat4 projection = Frustum(left, right, bottom, top, 1.0, zNear+zFar);
	projection = Perspective(80,aspect,1.0,20000);
	glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

	model_view = LookAt(currentX, FPS_HEIGHT, currentZ, 0.0, 3.5, 20.0, 0.0, 1.0, 0.0);
}

/*
===========================================================
main(): This function is where the program starts. It sets
		up the environment and callbacks and initializes
		the initialization.
===========================================================
*/
GLint main(GLint argc, char* argv[])
{
	gun_names.insert("Slide");
	gun_names.insert("Trigger");
	gun_names.insert("Hammer");
	gun_names.insert("Mag");
	gun_names.insert("Mesh_0009");

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutCreateWindow("Virtual Floor Plan");

	currentWidth = WINDOW_WIDTH;
	currentHeight = WINDOW_HEIGHT;

	glutFullScreen();

	// Running on the PC, so we need this call
	glewInit();

	init();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutPassiveMotionFunc(mouseMove);
	glutTimerFunc(TICK_INTERVAL, tick, TICK_INTERVAL);
	glutMouseFunc(mouse);

	glutMainLoop();

	return 0;
}
