#ifndef _BLOBJECT_H_
#define _BLOBJECT_H_ 1

#define INIT_SIZE 32

#include "vec.h"
#include "material.h"
#include <string>

// Triangle face -- holds indices into the vertex, 
// normal, and tex coord arrays
typedef struct face
{
	int v_index1;
	int v_index2;
	int v_index3;

	int t_index1;
	int t_index2;
	int t_index3;

	int n_index1;
	int n_index2;
	int n_index3;

} face;

/*
===========================================================
CLASS: Blobject

This class defines an object parsed from a .obj file. It is
basically a container for the object attributes such as 
vertices, normals, faces, and texture coordinates.
===========================================================
*/
class Blobject
{
public:

	// Constructor
	Blobject()
	{
		num_vertices = 0;
		num_texs = 0;
		x_center = 0.0;
		z_center = 0.0;
		num_faces = 0;
		num_normals = 0;
		textured = false;
		faces = new face[INIT_SIZE];
		length = INIT_SIZE;
		faces_length = INIT_SIZE;
		normals_length = INIT_SIZE;
		tex_length = INIT_SIZE;
		vertices = new vec4[INIT_SIZE];
		normals = new vec3[INIT_SIZE];
		texs = new vec2[INIT_SIZE];

		int i = 0;
		for(i = 0; i < INIT_SIZE; ++i)
		{
			vertices[i] = vec4(0.0, 0.0, 0.0, 0.0);
			normals[i] = vec3(0.0, 0.0, 0.0);
			texs[i] = vec2(0.0, 0.0);
		}

		obj_mat = none_mat();
		
	}

	// Add a texture coordinate to the texture coordinate array
	inline void add_tex(vec2 tex)
	{
		// Double the size of the array if we hit the limit
		if(num_texs >= tex_length)
		{
			vec2 * temp = new vec2[tex_length * 2];

			int i = 0;
			for(i = 0; i < tex_length; ++i)
			{
				temp[i] = texs[i];
			}

			delete [] texs;
			texs = temp;
			tex_length *= 2;
		}

		texs[num_texs] = tex;
		++num_texs;
	}

	// Return the texture coordinate array
	inline vec2 * get_texs()
	{
		return texs;
	}

	// Add a face to the array of faces for this object
	inline void add_face(face next_face)
	{
		if(num_faces >= faces_length)
		{
			face * temp = new face[faces_length * 2];

			int i = 0;
			for(i = 0; i < faces_length; ++i)
			{
				temp[i] = faces[i];
			}
			delete [] faces;
			faces = temp;
			faces_length *= 2;
		}
		
		faces[num_faces] = next_face;
		++num_faces;
	}
	
	// Return the faces
	inline face * get_faces()
	{
		return faces;
	}
	
	// Return the number of faces for this object
	inline int get_num_faces()
	{
		return num_faces;
	}

	// Return the number of vertices for this object
	inline int get_num_vertices()
	{
		return num_vertices;
	}

	// Return the number of texture coordinates for this object
	inline int get_num_texs()
	{
		return num_texs;
	}

	// Add a vertex to this object's set of vertices
	inline void add_vertex(vec4 vert)
	{
		if(num_vertices >= length)
		{
			vec4 * temp = new vec4[length * 2];

			int i = 0;
			for(i = 0; i < length; ++i)
			{
				temp[i] = vertices[i];
			}

			for(i = length; i < length * 2; ++i)
			{
				temp[i] = vec4(0.0, 0.0, 0.0, 0.0);
			}

			delete [] vertices;
			vertices = temp;
			length *= 2;
		}

		vertices[num_vertices] = vert;
		++num_vertices;

		// Used to calculate the center with a call to getXZCenter()
		x_center += vert.x;
		z_center += vert.z;
	}

	// Add a normal to this object's set of normals
	inline void add_normal(vec3 normal)
	{
		if(num_normals >= normals_length)
		{
			vec3 * temp = new vec3[normals_length * 2];

			int i = 0;
			for(i = 0; i < normals_length; ++i)
			{
				temp[i] = normals[i];
			}

			for(i = normals_length; i < normals_length * 2; ++i)
			{
				temp[i] = vec3(0.0, 0.0, 0.0);
			}

			delete [] normals;
			normals = temp;
			normals_length *= 2;
		}

		normals[num_normals] = normal;
		++num_normals;
	}

	// Return the average of the x and z coordinates;
	// This SHOULD be the center of the object on the x-z plane
	inline vec2 getXZCenter()
	{
		if(num_vertices > 0)
		{
			return vec2(x_center/num_vertices, z_center/num_vertices);
		}
		return vec2(-10000.0, -10000.0);
	}

	// Return a vertex from the array at the specified index
	inline vec4 get_vertex(int index)
	{
		if(index < 0 || index >= num_vertices)
		{
			return NULL;
		}

		return vertices[index];
	}


	// Return a normal from the array at the specified index
	inline vec4 get_normal(int index)
	{
		if(index < 0 || index >= num_vertices)
		{
			return NULL;
		}

		return normals[index];
	}

	// Return all of this object's vertices
	inline vec4 * get_vertices()
	{
		return vertices;
	}

	// Return all of this object's normals
	inline vec3 * get_normals()
	{
		return normals;
	}

	// Return the number of normals for this object
	inline int get_num_normals()
	{
		return num_normals;
	}

	// Set the object to be smoothly shaded
	inline void set_smoothed(bool val)
	{
		smoothed = val;
	}

	// Return whether this object is smoothly shaded
	inline bool is_smoothed()
	{
		return smoothed;
	}

	// Set to true if this object has valid texture coordinates
	inline void set_textured(bool val)
	{
		textured = val;
	}

	// Returns true if this object has valid texture coordinates
	inline bool is_textured()
	{
		return textured;
	}

	// Return the name of the object (what is specified in the
	// .obj file)
	inline std::string get_name()
	{
		return obj_name;
	}

	// Set the name of this object (as specified from
	// the .obj file)
	inline void set_name(std::string name)
	{
		obj_name = name;
	}

	// Set the material to be used with this object
	inline void set_material(material new_mat)
	{
		obj_mat = new_mat;
	}

	// Return the material used for this object
	inline material get_material()
	{
		return obj_mat;
	}

private:
	vec4 * vertices;
	vec3 * normals;
	vec2 * texs;
	bool smoothed;
	bool textured;
	std::string obj_name;
	material obj_mat;
	face * faces;
	int num_vertices;
	int num_faces;
	int num_texs;
	int tex_length;
	int faces_length;
	int length;
	int num_normals;
	int normals_length;
	GLfloat x_center;
	GLfloat z_center;

	// Returns a sort-of "default" material; used internally
	inline material none_mat()
	{
		material none;

		none.Ns = 0.0;
		none.Ka = vec3(0.0, 0.0, 0.0);
		none.Kd = vec3(0.8, 0.8, 0.8);
		none.Ks = vec3(0.8, 0.8, 0.8);
		none.Ni = 1.0;
		none.d = 1.0;
		none.illum = 2;
		none.mat_name = "None";
		none.texture = "None";

		return none;
	}
};

#endif