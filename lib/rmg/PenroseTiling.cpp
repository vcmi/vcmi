/*
 * © 2020 Michael Percival <m@michaelpercival.xyz>
 *   See LICENSE file for copyright and license details.
 */

// Adapted from https://github.com/mpizzzle/penrose

// FIXME: Find library for geometry representation
//https://www.boost.org/doc/libs/1_72_0/libs/geometry/doc/html/geometry/reference/adapted/boost_polygon/point_data.html

//#include <GL/glew.h>
//#include <GLFW/glfw3.h>

//#include <glm/glm.hpp>
//#include <glm/gtx/rotate_vector.hpp>

#include "StdInc.h"

#include <array>
#include <random>
#include <string>
#include <vector>

#include <boost/geometry/strategies/transform/matrix_transformers.hpp>
//https://www.boost.org/doc/libs/1_72_0/libs/geometry/doc/html/geometry/reference/strategies/strategy_transform_rotate_transformer.html

//#include "shader.hpp"
//#include "png_writer.hpp"

#include "PenroseTiling.h"

VCMI_LIB_NAMESPACE_BEGIN

//static const std::string file_name = "penrose.png";

Triangle::Triangle(bool t_123, const TIndices & inds):
	tiling(t_123),
	indices(inds)
{}

void PenroseTiling::split(Triangle& p, std::vector<glm::vec2>& points,
	std::array<std::vector<uint32_t>, 5>& indices, uint32_t depth) 
{
	uint32_t s = points.size();
	TIndices& i = p.indices;

	const auto p2 = P2;

	if (depth > 0)
	{
		if (p.tiling ^ !p2)
		{
			points.push_back(glm::vec2(((1.0f - PHI) * points[i[0]]) + (PHI * points[i[2]])));
			points.push_back(glm::vec2(((1.0f - PHI) * points[i[p2]]) + (PHI * points[i[!p2]])));

			Triangle t1(p2, TIndices({ i[(!p2) + 1], p2 ? i[2] : s, p2 ? s : i[1] }));
			Triangle t2(true, TIndices({ p2 ? i[1] : s, s + 1, p2 ? s : i[1] }));
			Triangle t3(false, TIndices({ s, s + 1, i[0] }));

			// FIXME: Make sure these are not destroyed when we leave the scope
			p.subTriangles = { &t1, &t2, &t3 };
		}
		else
		{
			points.push_back(glm::vec2(((1.0f - PHI) * points[i[p2 * 2]]) + (PHI * points[i[!p2]])));

			Triangle t1(true, TIndices({ i[2], s, i[1] }));
			Triangle t2(false, TIndices({ i[(!p2) + 1], s, i[0] }));

			p.subTriangles = { &t1, &t2 };
		}

		for (auto& t : p.subTriangles)
		{
			if (depth == 1)
			{
				for (uint32_t k = 0; k < 3; ++k)
				{
					if (k != (t->tiling ^ !p2 ? 2 : 1))
					{
						indices[indices.size() - 1].push_back(t->indices[k]);
						indices[indices.size() - 1].push_back(t->indices[((k + 1) % 3)]);
					}
				}

				indices[t->tiling + (p.tiling ? 0 : 2)].insert(indices[t->tiling + (p.tiling ? 0 : 2)].end(), t->indices.begin(), t->indices.end());
			}

			// Split recursively
			split(*t, points, indices, depth - 1);
		}
	}

	return;
}

// TODO: Return something
void generatePenroseTiling(size_t numZones, CRandomGenerator * rand);
{
	float scale = 100.f / (numZones + 20); //TODO: Use it to initialize the large tile

	//static std::default_random_engine e(std::random_device{}());
	static std::uniform_real_distribution<> d(0, 1);

	/*
	std::vector<glm::vec3> colours = { glm::vec3(d(e), d(e), d(e)), glm::vec3(d(e), d(e), d(e)),
									   glm::vec3(d(e), d(e), d(e)), glm::vec3(d(e), d(e), d(e)),
									   glm::vec3(d(e), d(e), d(e)), glm::vec3(d(e), d(e), d(e)) };
	*/

	float polyAngle = glm::radians(360.0f / POLY);

	std::vector<glm::vec2> points = { glm::vec2(0.0f, 0.0f), glm::vec2(0.0f, 1.0f) };
	std::array<std::vector<uint32_t>, 5> indices;

	for (uint32_t i = 1; i < POLY; ++i)
	{
		//TODO: Use boost to rotate
		glm::vec2 next = glm::rotate(points[i], polyAngle);
		points.push_back(next);
	}

	// TODO: Scale to unit square
	for (auto& p : points)
	{
		p.x = (p.x / window_w) * window_h;
	}

	for (uint32_t i = 0; i < POLY; i++)
	{
		std::array<uint32_t, 2> p = { (i % (POLY + 1)) + 1, ((i + 1) % POLY) + 1 };

		triangle t(true, TIndices({ 0, p[i & 1], p[!(i & 1)] }));

		split(t, points, indices, DEPTH);
	}

	// TODO: Return collection of triangles
	// TODO: Get center point of the triangle

	// Do not draw anything
	/*
	if(!glfwInit())
	{
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(window_w, window_h, "penrose", NULL, NULL);

	if(window == NULL) {
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glewExperimental=true;

	if (glewInit() != GLEW_OK) {
		return -1;
	}

	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	uint32_t VAOs[5], VBO, EBOs[5];

	glGenVertexArrays(5, VAOs);
	glGenBuffers(1, &VBO);
	glGenBuffers(5, EBOs);
	glLineWidth(line_w);

	for (uint32_t i = 0; i < indices.size(); ++i) {
		glBindVertexArray(VAOs[i]);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, points.size() * 4 * 2, &points[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOs[i]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices[i].size() * 4, &indices[i][0], GL_STATIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
	}

	uint32_t programID = Shader::loadShaders("vertex.vert", "fragment.frag");
	GLint paint = glGetUniformLocation(programID, "paint");

	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 && paint != -1) {
		glViewport(-1.0 * (window_w / scale) * ((0.5 * scale) - 0.5), -1.0 * (window_h / scale) * ((0.5 * scale) - 0.5), window_w, window_h);
		glClearColor(colours.back().x, colours.back().y, colours.back().z, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(programID);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		for (uint32_t i = 0; i < indices.size(); ++i) {
			glPolygonMode(GL_FRONT_AND_BACK, i < indices.size() - 1 ? GL_FILL : GL_LINE);
			glUniform3fv(paint, 1, &colours[i][0]);
			glBindVertexArray(VAOs[i]);
			glDrawElements(i < indices.size() - 1 ? GL_TRIANGLES : GL_LINES, indices[i].size(), GL_UNSIGNED_INT, 0);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	int frame_w, frame_h;
	glfwGetFramebufferSize(window, &frame_w, &frame_h);

	png_bytep* row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * frame_h);

	for (int y = 0; y < frame_h; ++y) {
		row_pointers[y] = (png_byte*) malloc((4 * sizeof(png_byte)) * frame_w);
		glReadPixels(0, y, frame_w, 1, GL_RGBA, GL_UNSIGNED_BYTE, row_pointers[y]);
	}

	PngWriter::write_png_file(file_name, frame_w, frame_h, row_pointers);

	return 0;
	*/
}

VCMI_LIB_NAMESPACE_END