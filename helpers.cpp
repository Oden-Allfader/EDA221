#include "config.hpp"
#include "helpers.hpp"

#include "core/Log.h"
#include "core/Misc.h"
#include "core/opengl.hpp"
#include "core/various.hpp"
#include "external/lodepng.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <cassert>

static std::vector<u8>
getTextureData(std::string const& filename, u32& width, u32& height, bool flip)
{
	auto const path = config::resources_path(filename);
	std::vector<unsigned char> image;
	if (lodepng::decode(image, width, height, path, LCT_RGBA) != 0) {
		LogWarning("Couldn't load or decode image file %s", path.c_str());
		return image;
	}
	if (!flip)
		return image;

	auto const channels_nb = 4u;
	auto flipBuffer = std::vector<u8>(width * height * channels_nb);
	for (u32 y = 0; y < height; y++)
		memcpy(flipBuffer.data() + (height - 1 - y) * width * channels_nb, &image[y * width * channels_nb], width * channels_nb);

	return flipBuffer;
}

std::vector<eda221::mesh_data>
eda221::loadObjects(std::string const& filename)
{
	std::vector<eda221::mesh_data> objects;

	auto const scene_filepath = config::resources_path("scenes/" + filename);
	Assimp::Importer importer;
	auto const assimp_scene = importer.ReadFile(scene_filepath, 0u);
	if (assimp_scene == nullptr) {
		LogError("Assimp failed to load \"%s\": %s", scene_filepath.c_str(), importer.GetErrorString());
		return objects;
	}

	if (assimp_scene->mNumMeshes == 0u) {
		LogError("No mesh available; loading \"%s\" must have had issues", scene_filepath.c_str());
		return objects;
	}

	objects.reserve(assimp_scene->mNumMeshes);
	for (size_t j = 0; j < assimp_scene->mNumMeshes; ++j) {
		auto const assimp_object_mesh = assimp_scene->mMeshes[j];

		if (!assimp_object_mesh->HasFaces()) {
			LogError("Unsupported object \"%s\" has no faces", assimp_object_mesh->mName.C_Str());
			continue;
		}
		if ((assimp_object_mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) != aiPrimitiveType_TRIANGLE) {
			LogError("Unsupported object \"%s\" uses non-triangle faces", assimp_object_mesh->mName.C_Str());
			continue;
		}
		if (!assimp_object_mesh->HasPositions()) {
			LogError("Unsupported object \"%s\" has no positions", assimp_object_mesh->mName.C_Str());
			continue;
		}

		eda221::mesh_data object;
		object.vao = 0u;

		glGenVertexArrays(1, &object.vao);
		assert(object.vao != 0u);
		glBindVertexArray(object.vao);

		auto const vertices_offset = 0u;
		auto const vertices_size = static_cast<GLsizeiptr>(assimp_object_mesh->mNumVertices * sizeof(glm::vec3));

		auto const normals_offset = vertices_size;
		auto const normals_size = assimp_object_mesh->HasNormals() ? vertices_size : 0u;

		auto const texcoords_offset = normals_offset + normals_size;
		auto const texcoords_size = assimp_object_mesh->HasTextureCoords(0u) ? vertices_size : 0u;

		auto const tangents_offset = texcoords_offset + texcoords_size;
		auto const tangents_size = assimp_object_mesh->HasTangentsAndBitangents() ? vertices_size : 0u;

		auto const binormals_offset = tangents_offset + tangents_size;
		auto const binormals_size = assimp_object_mesh->HasTangentsAndBitangents() ? vertices_size : 0u;

		auto const bo_size = static_cast<GLsizeiptr>(vertices_size
		                                            +normals_size
		                                            +texcoords_size
		                                            +tangents_size
		                                            +binormals_size
		                                            );
		glGenBuffers(1, &object.bo);
		assert(object.bo != 0u);
		glBindBuffer(GL_ARRAY_BUFFER, object.bo);
		glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

		glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const*>(assimp_object_mesh->mVertices));
		glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::vertices));
		glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0x0));

		if (assimp_object_mesh->HasNormals()) {
			glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size, static_cast<GLvoid const*>(assimp_object_mesh->mNormals));
			glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::normals));
			glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::normals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(normals_offset));
		}

		if (assimp_object_mesh->HasTextureCoords(0u)) {
			glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const*>(assimp_object_mesh->mTextureCoords[0u]));
			glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::texcoords));
			glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(texcoords_offset));
		}

		if (assimp_object_mesh->HasTangentsAndBitangents()) {
			glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size, static_cast<GLvoid const*>(assimp_object_mesh->mTangents));
			glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::tangents));
			glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::tangents), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(tangents_offset));

			glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size, static_cast<GLvoid const*>(assimp_object_mesh->mBitangents));
			glEnableVertexAttribArray(static_cast<unsigned int>(eda221::shader_bindings::binormals));
			glVertexAttribPointer(static_cast<unsigned int>(eda221::shader_bindings::binormals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(binormals_offset));
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0u);

		object.indices_nb = assimp_object_mesh->mNumFaces * 3u;
		auto object_indices = std::make_unique<GLuint[]>(static_cast<size_t>(object.indices_nb));
		for (size_t i = 0u; i < assimp_object_mesh->mNumFaces; ++i) {
			auto const& face = assimp_object_mesh->mFaces[i];
			assert(face.mNumIndices == 3u);
			object_indices[3u * i + 0u] = face.mIndices[0u];
			object_indices[3u * i + 1u] = face.mIndices[1u];
			object_indices[3u * i + 2u] = face.mIndices[2u];
		}
		object.ibo = 0u;
		glGenBuffers(1, &object.ibo);
		assert(object.ibo != 0u);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, object.ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<unsigned int>(object.indices_nb) * sizeof(GL_UNSIGNED_INT), reinterpret_cast<GLvoid const*>(object_indices.get()), GL_STATIC_DRAW);
		object_indices.reset(nullptr);

		glBindVertexArray(0u);
		glBindBuffer(GL_ARRAY_BUFFER, 0u);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

		objects.push_back(object);

		LogInfo("Loaded object \"%s\" with normals:%d, tangents&bitangents:%d, texcoords:%d",
		        assimp_object_mesh->mName.C_Str(), assimp_object_mesh->HasNormals(),
		        assimp_object_mesh->HasTangentsAndBitangents(), assimp_object_mesh->HasTextureCoords(0));
	}

	return objects;
}

GLuint
eda221::loadTexture2D(std::string const& filename)
{
	u32 width, height;
	auto const data = getTextureData("textures/" + filename, width, height, true);
	if (data.empty())
		return 0u;

	GLuint texture = 0u;
	glGenTextures(1, &texture);
	assert(texture != 0u);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0, GL_RGBA, GL_UNSIGNED_BYTE, reinterpret_cast<GLvoid const*>(data.data()));
	glBindTexture(GL_TEXTURE_2D, 0u);

	return texture;
}

GLuint
eda221::loadTextureCubeMap(std::string const& posx, std::string const& negx,
	std::string const& posy, std::string const& negy,
	std::string const& posz, std::string const& negz,
	bool generate_mipmap)
{
	GLuint texture = 0u;
	// Create an OpenGL texture object. Similarly to `glGenVertexArrays()`
	// and `glGenBuffers()` that were used in assignment 2,
	// `glGenTextures()` can create `n` texture objects at once. Here we
	// only one texture object that will contain our whole cube map.
	glGenTextures(1, &texture);
	assert(texture != 0u);

	// Similarly to vertex arrays and buffers, we first need to bind the
	// texture object in orther to use it. Here we will bind it to the
	// GL_TEXTURE_CUBE_MAP target to indicate we want a cube map. If you
	// look at `eda221::loadTexture2D()` just above, you will see that
	// GL_TEXTURE_2D is used there, as we want a simple 2D-texture.
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

	// Set the wrapping properties of the texture; you can have a look on
	// http://docs.gl to learn more about them
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Set the minification and magnification properties of the textures;
	// you can have a look on http://docs.gl to lear more about them, or
	// attend EDAN35 in the next period ;-)
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, generate_mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// We need to fill in the cube map using the images passed in as
	// argument. The function `getTextureData()` uses lodepng to read in
	// the image files and return a `std::vector<u8>` containing all the
	// texels.
	u32 width, height;
	auto data = getTextureData("cubemaps/" + negx, width, height, false);
	if (data.empty()) {
		glDeleteTextures(1, &texture);
		return 0u;
	}
	// With all the texels available on the CPU, we now want to push them
	// to the GPU: this is done using `glTexImage2D()` (among others). You
	// might have thought that the target used here would be the same as
	// the one passed to `glBindTexture()` or `glTexParameteri()`, similar
	// to what is done `eda221::loadTexture2D()`. However, we want to fill
	// in a cube map, which has six different faces, so instead we specify
	// as the target the face we want to fill in. In this case, we will
	// start by filling the face sitting on the negative side of the
	// x-axis by specifying GL_TEXTURE_CUBE_MAP_NEGATIVE_X.
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		/* mipmap level, you'll see that in EDAN35 */0,
		/* how are the components internally stored */GL_RGBA,
		/* the width of the cube map's face */static_cast<GLsizei>(width),
		/* the height of the cube map's face */static_cast<GLsizei>(height),
		/* must always be 0 */0,
		/* the format of the pixel data: which components are available */GL_RGBA,
		/* the type of each component */GL_UNSIGNED_BYTE,
		/* the pointer to the actual data on the CPU */reinterpret_cast<GLvoid const*>(data.data()));

	data = getTextureData("cubemaps/" + posx, width, height, false);
	if (data.empty()) {
		glDeleteTextures(1, &texture);
		return 0u;
	}
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		/* mipmap level, you'll see that in EDAN35 */0,
		/* how are the components internally stored */GL_RGBA,
		/* the width of the cube map's face */static_cast<GLsizei>(width),
		/* the height of the cube map's face */static_cast<GLsizei>(height),
		/* must always be 0 */0,
		/* the format of the pixel data: which components are available */GL_RGBA,
		/* the type of each component */GL_UNSIGNED_BYTE,
		/* the pointer to the actual data on the CPU */reinterpret_cast<GLvoid const*>(data.data()));
	data = getTextureData("cubemaps/" + negy, width, height, false);
	if (data.empty()) {
		glDeleteTextures(1, &texture);
		return 0u;
	}
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		/* mipmap level, you'll see that in EDAN35 */0,
		/* how are the components internally stored */GL_RGBA,
		/* the width of the cube map's face */static_cast<GLsizei>(width),
		/* the height of the cube map's face */static_cast<GLsizei>(height),
		/* must always be 0 */0,
		/* the format of the pixel data: which components are available */GL_RGBA,
		/* the type of each component */GL_UNSIGNED_BYTE,
		/* the pointer to the actual data on the CPU */reinterpret_cast<GLvoid const*>(data.data()));
	data = getTextureData("cubemaps/" + posy, width, height, false);
	if (data.empty()) {
		glDeleteTextures(1, &texture);
		return 0u;
	}
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		/* mipmap level, you'll see that in EDAN35 */0,
		/* how are the components internally stored */GL_RGBA,
		/* the width of the cube map's face */static_cast<GLsizei>(width),
		/* the height of the cube map's face */static_cast<GLsizei>(height),
		/* must always be 0 */0,
		/* the format of the pixel data: which components are available */GL_RGBA,
		/* the type of each component */GL_UNSIGNED_BYTE,
		/* the pointer to the actual data on the CPU */reinterpret_cast<GLvoid const*>(data.data()));
	data = getTextureData("cubemaps/" + negz, width, height, false);
	if (data.empty()) {
		glDeleteTextures(1, &texture);
		return 0u;
	}
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
		/* mipmap level, you'll see that in EDAN35 */0,
		/* how are the components internally stored */GL_RGBA,
		/* the width of the cube map's face */static_cast<GLsizei>(width),
		/* the height of the cube map's face */static_cast<GLsizei>(height),
		/* must always be 0 */0,
		/* the format of the pixel data: which components are available */GL_RGBA,
		/* the type of each component */GL_UNSIGNED_BYTE,
		/* the pointer to the actual data on the CPU */reinterpret_cast<GLvoid const*>(data.data()));
	data = getTextureData("cubemaps/" + posz, width, height, false);
	if (data.empty()) {
		glDeleteTextures(1, &texture);
		return 0u;
	}
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		/* mipmap level, you'll see that in EDAN35 */0,
		/* how are the components internally stored */GL_RGBA,
		/* the width of the cube map's face */static_cast<GLsizei>(width),
		/* the height of the cube map's face */static_cast<GLsizei>(height),
		/* must always be 0 */0,
		/* the format of the pixel data: which components are available */GL_RGBA,
		/* the type of each component */GL_UNSIGNED_BYTE,
		/* the pointer to the actual data on the CPU */reinterpret_cast<GLvoid const*>(data.data()));

	if (generate_mipmap)
		// Generate the mipmap hierarchy; wait for EDAN35 to understand
		// what it does
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0u);

	return texture;
}
GLuint
eda221::createProgram(std::string const& vert_shader_source_path, std::string const& frag_shader_source_path)
{
	auto const vertex_shader_source = utils::slurp_file(config::shaders_path("EDA221/" + vert_shader_source_path));
	GLuint vertex_shader = utils::opengl::shader::generate_shader(GL_VERTEX_SHADER, vertex_shader_source);
	assert(vertex_shader != 0u);

	auto const fragment_shader_source = utils::slurp_file(config::shaders_path("EDA221/" + frag_shader_source_path));
	GLuint fragment_shader = utils::opengl::shader::generate_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
	assert(fragment_shader != 0u);

	GLuint program = utils::opengl::shader::generate_program({ vertex_shader, fragment_shader });
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	assert(program != 0u);
	return program;
}
