#include "objparser.h"

static int num_materials;

/*
===========================================================
parse_objects(): A function that takes two parameters: the
				 name of the .obj file that contains the
				 exported data from Blender, and a pointer
				 to an array of Blobjects that will contain
				 the result. The return value of the 
				 function is the total number of objects
				 in the resulting array. The associated 
				 .mtl file is automatically parsed as a
				 part of this function.
===========================================================
*/
int ObjParser::parse_objects(std::string filename, Blobject ** objs)
{
	// Open the file for reading
	std::ifstream obj_file;
	obj_file.open(filename);
	
	if(!obj_file.is_open())
	{
		// Not the best thing to do; fortunately, this software doesn't
		// control the country's Nuclear Missiles
		return NULL;
	}

	num_objects = 0;
	std::string mat_filename;
	std::string line;

	// This loop is simply to obtain the number of objects
	// in the .obj file, so that we can initialize the array
	while(getline(obj_file, line))
	{
		if(line.length() > 0)
		{
			if(line.length() > 5)
			{
				std::string subbed = line.substr(0, 6);
				if(subbed.compare("mtllib") == 0)
				{
					// We find the name of the .mtl file;
					// save it for later
					mat_filename = line.substr(7);
				}
			}

			char first = line.at(0);
			if(first == 'o')
			{
				++num_objects;
			}
		}
	}

	obj_file.close();

	// Create an array of materials first, so that they can
	// be assigned to objects when the objects are parsed
	material * obj_materials = parse_materials(mat_filename);
	
	if(obj_materials == NULL)
	{
		return NULL;
	}

	Blobject * objects = new Blobject[num_objects];
	
	obj_file.open(filename);

	char * buffer;
	int length = 0;

	// Obtain the actual length of the .obj file, 
	// and shove the entire file into a character array
	if(obj_file)
	{
		obj_file.seekg(0, obj_file.end);
		length = obj_file.tellg();
		obj_file.seekg(0, obj_file.beg);
		buffer = new char[length];

		obj_file.read(buffer, length);
		obj_file.close();
	}
	else
	{
		return NULL;
	}

	// Placing the entire file into a string allows for easier parsing
	std::string obj_string(buffer, length);

	// Now, parse using the string
	num_objects = 0;
	int i = 0;

	std::size_t num_vertices = 0;
	std::size_t num_normals = 0;

	for(i = 0; i < length; ++i)
	{
		int line_begin = i;
		while(obj_string[i] != '\n' && i < length) ++i;

		std::string line = obj_string.substr(line_begin, i - line_begin);

		if(line[0] == 'o')
		{
			// The object section could be of arbitrary length, 
			// so we have to count it first
			++i;
			int obj_start = line_begin;

			// Find the end of this objects section in the file
			for(; i < length; ++i)
			{
				int begin = i;
				while(obj_string[i] != '\n' && i < length) ++i;

				std::string sub_line = obj_string.substr(begin, i - begin);

				if(sub_line[0] == 'o')
				{
					i = begin - 1;
					break;
				}
			}

			// Using the carved out section for this object, parse all of its properties
			objects[num_objects] = parse_single(obj_string.substr(obj_start, i - obj_start), 
				&num_vertices, &num_normals, obj_materials);
			++num_objects;
		}
	}
	
	*objs = objects;
	return num_objects;
}

/*
===========================================================
parse_single(): Parse all of the attributes for a single
				object in the .obj file and return it.
===========================================================
*/
Blobject ObjParser::parse_single(std::string obj_string, std::size_t * num_vertices, 
								 std::size_t * num_normals, material * mats)
{
	int length = obj_string.length();

	std::size_t normal_begin = *num_normals;
	std::size_t vertex_begin = *num_vertices;

	Blobject next_obj;
	bool is_textured = false;
	
	int i = 0;
	for(i = 0; i < length; ++i)
	{
		int begin = i;
		while(obj_string[i] != '\n' && i < length) ++i;

		std::string line = obj_string.substr(begin, i - begin);

		if(line.length() > 2)
		{
			// Now, the flurry of if statements
			if(line[0] == 'o')
			{
				// We SHOULD only have 1 line starting with 'o',
				// so it SHOULD be the name of the current object
				next_obj.set_name(line.substr(2));
			}
			else if(line[0] == 'v')
			{
				float x;
				float y;
				float z;
				std::size_t index;

				// We found a normal -- parse it
				if(line[1] == 'n')
				{
					line = line.substr(3);
					x = stof(line, &index);
					line = line.substr(index);
					y = stof(line, &index);
					line = line.substr(index);
					z = stof(line);

					next_obj.add_normal(vec3(x, y, z));
					(*num_normals)++;
				}
				// We found a texture coordinate -- parse it
				else if(line[1] == 't')
				{
					is_textured = true;
					next_obj.set_textured(true);
					line = line.substr(3);
					x = stof(line, &index);
					line = line.substr(index);
					y = stof(line);

					next_obj.add_tex(vec2(x, y));

				}
				// We found a vertex -- parse it
				else
				{
					line = line.substr(2);
					x = stof(line, &index);
					line = line.substr(index);
					y = stof(line, &index);
					line = line.substr(index);
					z = stof(line);

					next_obj.add_vertex(vec4(x, y, z, 1.0));
					(*num_vertices)++;
				}
			}
			// Parse this property, even though it is not currently used
			else if(line[0] == 's')
			{
				if((line.substr(2).compare("off")) == 0)
				{
					next_obj.set_smoothed(false);
				}
				else
				{
					next_obj.set_smoothed(true);
				}
			}
			// Found the material -- look throught the array of parsed materials,
			// and assign the correct one to the object
			else if((line.substr(0, 6).compare("usemtl")) == 0)
			{
				line = line.substr(7);
				int j = 0;
				for(j = 0; j < num_materials; ++j)
				{
					if((mats[j].mat_name).compare(line) == 0)
					{
						next_obj.set_material(mats[j]);
						break;
					}
				}
			}
			// The object has to be exported with the 
			// TRIANGULATE FACES option selected
			else if(line[0] == 'f')
			{
				face next_face;
				line = line.substr(2);
				std::size_t index;

				// The face line will be parsed differently with tex coords
				if(is_textured)
				{
					next_face.v_index1 = stoi(line, &index) - 1;
					++index;
					line = line.substr(index);
					next_face.t_index1 = stoi(line, &index) - 1;
					++index;
					line = line.substr(index);
					next_face.n_index1 = stoi(line, &index) - 1;
					line = line.substr(index);

					next_face.v_index2 = stoi(line, &index) - 1;
					++index;
					line = line.substr(index);
					next_face.t_index2 = stoi(line, &index) - 1;
					++index;
					line = line.substr(index);
					next_face.n_index2 = stoi(line, &index) - 1;
					line = line.substr(index);

					next_face.v_index3 = stoi(line, &index) - 1;
					++index;
					line = line.substr(index);
					next_face.t_index3 = stoi(line, &index) - 1;
					++index;
					line = line.substr(index);
					next_face.n_index3 = stoi(line, &index) - 1;
				}
				else
				{
					// Maybe not the most efficient, but it's robust
					next_face.v_index1 = stoi(line, &index) - 1;
					++index;
					while(line[index] != '/') ++index;
					++index;
					line = line.substr(index);
					next_face.n_index1 = stoi(line, &index) - 1;
					line = line.substr(index);
					next_face.v_index2 = stoi(line, &index) - 1;
					++index;
					while(line[index] != '/') ++index;
					++index;
					line = line.substr(index);
					next_face.n_index2 = stoi(line, &index) - 1;
					line = line.substr(index);
					next_face.v_index3 = stoi(line, &index) - 1;
					++index;
					while(line[index] != '/') ++index;
					++index;
					line = line.substr(index);
					next_face.n_index3 = stoi(line, &index) - 1;
				}
				next_obj.add_face(next_face);
			}

		}
	}
	return next_obj;
}

/*
===========================================================
parse_materials(): Given the name of the .mtl file, parse
				   the materials for the objects in the 
				   .obj file.
===========================================================
*/
material * ObjParser::parse_materials(std::string filename)
{
	std::ifstream mat_file;
	mat_file.open(filename);

	if(!mat_file.is_open())
	{
		return NULL;
	}

	num_materials = 0;
	std::string line;

	// Loop to count the number of materials, so that 
	// we can initialize the array
	while(getline(mat_file, line))
	{
		if(line.length() > 6)
		{
			if((line.substr(0, 6).compare("newmtl")) == 0)
			{
				++num_materials;
			}
		}
	}

	mat_file.close();

	material * mats = new material[num_materials];

	mat_file.open(filename);

	if(!mat_file.is_open())
	{
		return NULL;
	}

	num_materials = 0;

	char * buffer;
	int length = 0;

	// As with the object file, we will shove
	// the entire file into a string for easier parsing
	mat_file.seekg(0, mat_file.end);
	length = mat_file.tellg();
	mat_file.seekg(0, mat_file.beg);

	buffer = new char[length];

	mat_file.read(buffer, length);
	mat_file.close();

	std::string mat_string(buffer, length);

	int i = 0;
	for(i = 0; i < length; ++i)
	{
		int line_begin = i;
		while(mat_string[i] != '\n' && i < length) ++i;

		std::string line = mat_string.substr(line_begin, i - line_begin);

		if((line.substr(0, 6).compare("newmtl")) == 0)
		{
			std::string name = line.substr(7, line.length() - 7);
			material next_mat;

			// Kind of a "default" material
			next_mat.d = 1.0;
			next_mat.illum = 2;
			next_mat.Ka = vec4(-1.0, -1.0, -1.0, 1.0);
			next_mat.Kd = vec4(-1.0, -1.0, -1.0, 1.0);
			next_mat.Ks = vec4(-1.0, -1.0, -1.0, 1.0);
			next_mat.Ni = 1.0;
			next_mat.Ns = 1.0;
			next_mat.mat_name = name;
			next_mat.texture = "stupid";

			// Now, we parse the individual material
			++i;
			for(; i < length; ++i)
			{
				int begin = i;

				while(mat_string[i] != '\n' && i < length) ++i;

				std::string sub_line = mat_string.substr(begin, i - begin);

				if((sub_line.substr(0, 6).compare("newmtl")) == 0)
				{
					i -= (sub_line.length() + 1);
					break;
				}
				else
				{
					std::string prop;
					// A lot of if statements coming here
					if(sub_line.length() > 2)
					{
						if(sub_line[0] == 'N')
						{
							prop = sub_line.substr(3);
							if(sub_line[1] == 's')
							{
								next_mat.Ns = stof(prop);
							}
							else if(sub_line[1] == 'i')
							{
								next_mat.Ni = stof(prop);
							}
						}
						else if(sub_line[0] == 'K')
						{
							// Each of the lines that start with 'K' will have
							// the same structure, fortunately

							prop = sub_line.substr(3);
							float r;
							float g;
							float b;
							std::size_t index;

							r = stof(prop, &index);
							prop = prop.substr(index);
							g = stof(prop, &index);
							prop = prop.substr(index);
							b = stof(prop);

							if(sub_line[1] == 'a')
							{
								// Ambient 
								next_mat.Ka = vec4(r, g, b, 1.0);
							}
							else if(sub_line[1] == 'd')
							{
								// Diffuse
								next_mat.Kd = vec4(r, g, b, 1.0);
							}
							else if (sub_line[1] == 's')
							{
								// Specular
								next_mat.Ks = vec4(r, g, b, 1.0);
							}
						}
						else if(sub_line[0] == 'd')
						{
							// "Dissolve" factor (for alpha value)
							prop = sub_line.substr(2);
							next_mat.d = stof(prop);
						}
						else if(sub_line[0] == 'i')
						{
							// Illumination model (Phong, etc.)
							// This is pretty much ignored in the project
							prop = sub_line.substr(6);
							next_mat.illum = stoi(prop);
						}
						else if(sub_line[0] == 'm')
						{
							// Name of the texture for this material
							next_mat.texture = sub_line.substr(7);
						}
					}
				}

			}
			mats[num_materials] = next_mat;
			++num_materials;
		}
		
	}
	return mats;
}