#ifndef _OBJPARSER_H_
#define _OBJPARSER_H_

#include <string>
#include <fstream>
#include "blobject.h"

static int num_objects;

class ObjParser
{
public:
	static int parse_objects(std::string filename, Blobject ** objs);
	//static int num_objects;
private:
	static material * parse_materials(std::string filename);
	static Blobject parse_single(std::string obj_string, std::size_t * num_vertices, 
		std::size_t * num_normals, material * mats);

};

#endif