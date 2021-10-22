#pragma once

#include "GL.hpp"
#include "Load.hpp"
#include "Scene.hpp"

#define CHESSBOARD_SIZE 14.0f
#define CHESSBOX_SIZE 2.0f

struct ChessBoardTextureProgram
{
	ChessBoardTextureProgram();
	~ChessBoardTextureProgram();

	static GLuint rectangle_index_buffer;
	static GLuint rectangle_texture;

	void DrawChessBoard() const;

private:
	GLuint program = 0;

	GLuint Position_vec4 = -1U;
	GLuint Color_vec4 = -1U;
	GLuint TexCoord_vec2 = -1U;

	struct Vertex
	{
		glm::vec2 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};

	struct Rectangle
	{
		GLuint vertex_array = -1U;
		GLuint vertex_buffer = -1U;
		glm::vec4 rectangle_size;
		glm::u8vec4 color {0xff, 0xff, 0xff, 0xff};
	};

	std::vector <Rectangle> board_assets;

	GLuint GetVao(GLuint vertex_buffer) const;
	void SetupChessBoard();
};

extern Load < ChessBoardTextureProgram > chessboard_texture_program;