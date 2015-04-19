// Class that represents a domino on the screen
// used to store various attributes about a particular domino
// (and so we can create an array of domino objects)
// Author: Daniel Ehmig
// Date Completed: 14 October 2014

#include "cs432.h"
#include "vec.h"

class Domino
{
public:

	/*
	=======================================================
	Domino(): Default Constructor for the domino class; 
	created so that we can declare an array of dominos.
	=======================================================
	*/
	Domino()
	{
		xPos = 0.0;
		yPos = 0.0;
		zPos = 0.0;
		yRotation = 0.0;
		zRotation = 0.0;
		immovable = false;
		current_time = 0;
		falling = false;
	}

	/*
	=======================================================
	Domino(vec3, GLfloat, GLfloat): The constructor that 
	will be used most often to initialize a domino object
	with relevant properties set.
	=======================================================
	*/
	Domino(vec3 pos, GLfloat yRot, GLfloat zRot)
	{
		xPos = pos.x;
		yPos = pos.y;
		zPos = pos.z;
		yRotation = yRot;
		zRotation = zRot;
		immovable = false;
		current_time = 0;
		falling = false;
	}
	
	/*
	=======================================================
	The following (12) functions act as "getters" and 
	"setters" for each of the properties of a domino. They
	exist for both convenience and necessity.
	=======================================================
	*/
	inline void setPosition(GLfloat x, GLfloat y, GLfloat z)
	{
		xPos = x;
		yPos = y;
		zPos = z;
	}

	inline vec3 getPosition()
	{
		return vec3(xPos, yPos, zPos);
	}

	inline void setYRotation(GLfloat theta)
	{
		yRotation = theta;
	}

	inline GLfloat getYRotation()
	{
		return yRotation;
	}

	inline void setZRotation(GLfloat theta)
	{
		zRotation = theta;
	}

	inline GLfloat getZRotation()
	{
		return zRotation;
	}

	inline void setImmovable(bool val)
	{
			immovable = val;
	}

	inline bool isImmovable()
	{
		return immovable;
	}

	inline void setTime(long time)
	{
		current_time = time;
	}

	inline long getTime()
	{
		return current_time;
	}

	inline void setFalling(bool val)
	{
		falling = val;
	}

	inline bool isFalling()
	{
		return falling;
	}
	
private:

	// The current position of this domino along the x-axis.
	GLfloat xPos;

	// The current position of this domino along the y-axis.
	GLfloat yPos;

	// The current position of this domino along the z-axis.
	GLfloat zPos;

	// The rotation about the y-axis that we will rotate 
	// this domino each time it is drawn.
	GLfloat yRotation;

	// The rotation about the z-axis that we will rotate
	// this domino each time it is drawn.
	GLfloat zRotation;

	// Each domino will have its own "time". This attribute
	// is VERY important for achieving the falling dominos effect.
	unsigned long current_time;

	// Whether this domino has been right-clicked (made immovable).
	bool immovable;

	// Whether this domino is currently falling (either by being 
	// left-clicked or from the previous domino falling upon it).
	bool falling;
};