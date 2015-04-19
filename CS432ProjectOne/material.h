#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include <string>
#include "vec.h"

typedef float GLfloat;

/*
===========================================================
material: Defines a material from a .mtl file; used with
		  objects from a .obj file.
===========================================================
*/
typedef struct material
{
	std::string mat_name;

	// Specular exponent
	GLfloat Ns;

	// Ambient color
	vec4 Ka;

	// Diffuse color
	vec4 Kd;

	// Specular color
	vec4 Ks;

	// Optical density or index of refraction
	GLfloat Ni;

	// Basically, a transparency or "dissolve" factor
	GLfloat d;

	// Refers to an index into a list of options;
	// I think I will probably ignore this for now
	int illum;

	// The file name of the texture
	std::string texture;
} material;

#endif