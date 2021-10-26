#pragma once

#include "ChessBoardData.hpp"
#include "GL.hpp"
#include "Load.hpp"
#include "Scene.hpp"

struct ChessBoardTextureProgram
{
	ChessBoardTextureProgram();
	~ChessBoardTextureProgram();

	struct Vertex
	{
		glm::vec2 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};

	struct Rectangle
	{
		glm::vec4 rectangle_size;
		glm::u8vec4 color{ 0xff, 0xff, 0xff, 0xff };
	};

	struct Circle
	{
		glm::vec2 origin;
		glm::u8vec4 color{ 0xff, 0xff, 0xff, 0xff };
	};

	static GLuint rectangle_index_buffer;
	static GLuint rectangle_texture;
	static GLuint circle_index_buffer;

	void SetupChessPiece(std::vector<Circle>& chess_pieces, const glm::vec2 origin, const glm::u8vec4 color) const;
	void DrawChessPieces(std::vector<Circle>& chess_pieces, const glm::uvec2& drawable_size) const;

	void DrawChessBoard(glm::uvec2 const& drawable_size) const;
private:
	GLuint program = 0;

	GLuint Position_vec4 = -1U;
	GLuint Color_vec4 = -1U;
	GLuint TexCoord_vec2 = -1U;

	std::vector <Rectangle> board_assets;

	GLuint GetVao(GLuint vertex_buffer) const;
	void SetupChessBoard();
};

extern Load < ChessBoardTextureProgram > chessboard_texture_program;