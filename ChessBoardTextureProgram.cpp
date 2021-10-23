#include "ChessBoardTextureProgram.hpp"
#include <signal.h>
#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

#ifdef _MSC_VER
#define DEBUG_BREAK __debugbreak()
#elif __APPLE__
#define DEBUG_BREAK __builtin_trap()
#else
#define DEBUG_BREAK raise(SIGTRAP)
#endif

#define ASSERT(x) if (!(x)) DEBUG_BREAK;
#define GLCall(x) GLClearError();\
	x;\
	ASSERT(GLLogCall(#x, __FILE__, __LINE__))

static void GLClearError()
{
	while (glGetError() != GL_NO_ERROR);
}

static bool GLLogCall(const char* function, const char* file, int line)
{
	while (GLenum error = glGetError())
	{
		std::cout << "[OpenGL Error] ( " << error << " ) : " << function << " " << file << ": " << line << std::endl;
		return false;
	}
	return true;
}

Load < ChessBoardTextureProgram > chessboard_texture_program(LoadTagEarly);

GLuint ChessBoardTextureProgram::circle_index_buffer = -1U;
GLuint ChessBoardTextureProgram::rectangle_index_buffer = -1U;
GLuint ChessBoardTextureProgram::rectangle_texture = 0;

static Load <void> load_rectangle_index_buffer(LoadTagEarly, []() {
	unsigned int rectangle_index_buffer_content[] = { 0, 1, 2, 1, 2, 3 };
	GLCall(glGenBuffers(1, &ChessBoardTextureProgram::rectangle_index_buffer));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ChessBoardTextureProgram::rectangle_index_buffer));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), rectangle_index_buffer_content, GL_STATIC_DRAW));
});

static Load <void> load_circle_index_buffer(LoadTagEarly, []() {
	unsigned int circle_index_buffer_content[CIRCLE_VERTICES_COUNT * 3];
	for (size_t i = 0; i < CIRCLE_VERTICES_COUNT - 1; i++)
	{
		circle_index_buffer_content[i * 3] = 0;
		circle_index_buffer_content[i * 3 + 1] = (unsigned int)i + 1;
		circle_index_buffer_content[i * 3 + 2] = (unsigned int)i + 2;
	}

	circle_index_buffer_content[CIRCLE_VERTICES_COUNT * 3 - 3] = 0;
	circle_index_buffer_content[CIRCLE_VERTICES_COUNT * 3 - 2] = CIRCLE_VERTICES_COUNT;
	circle_index_buffer_content[CIRCLE_VERTICES_COUNT * 3 - 1] = 1;

	GLCall(glGenBuffers(1, &ChessBoardTextureProgram::circle_index_buffer));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ChessBoardTextureProgram::circle_index_buffer));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * CIRCLE_VERTICES_COUNT * sizeof(unsigned int), circle_index_buffer_content, GL_STATIC_DRAW));
});

static Load <void> load_rectangle_texture(LoadTagEarly, []() {
	GLCall(glGenTextures(1, &ChessBoardTextureProgram::rectangle_texture));
	GLCall(glBindTexture(GL_TEXTURE_2D, ChessBoardTextureProgram::rectangle_texture));
	std::vector <glm::u8vec4> tex_data(1, glm::u8vec4(0xff));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data()));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
});

ChessBoardTextureProgram::ChessBoardTextureProgram()
{
	GLCall(program = gl_compile_program(
		//vertex shader:
		"#version 330\n"
		"in vec4 Position;\n"
		"in vec4 Color;\n"
		"in vec2 TexCoord;\n"
		"out vec4 color;\n"
		"out vec2 texCoord;\n"
		"void main() {\n"
		"	gl_Position = Position;\n"
		"	color = Color;\n"
		"	texCoord = TexCoord;\n"
		"}\n"
		,
		//fragment shader:
		"#version 330\n"
		"uniform sampler2D TEX;\n"
		"in vec4 color;\n"
		"in vec2 texCoord;\n"
		"out vec4 fragColor;\n"
		"void main() {\n"
		"	fragColor = texture(TEX, texCoord) * color;\n"
		"}\n"
	));

	GLCall(Position_vec4 = glGetAttribLocation(program, "Position"));
	GLCall(Color_vec4 = glGetAttribLocation(program, "Color"));
	GLCall(TexCoord_vec2 = glGetAttribLocation(program, "TexCoord"));

	GLCall(GLuint Tex_sampler2D = glGetUniformLocation(program, "TEX"));

	GLCall(glUseProgram(program));

	GLCall(glUniform1i(Tex_sampler2D, 0));

	GLCall(glUseProgram(0));

	SetupChessBoard();

	// DELETE IT
	//#define HEX_TO_U8VEC42( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
	//const glm::u8vec4 yl_color = HEX_TO_U8VEC42(0xfbff12ff);
	//#undef HEX_TO_U8VEC42
	//SetupChessPiece(glm::vec2(-540.0f, -540.0f), yl_color);
}

ChessBoardTextureProgram::~ChessBoardTextureProgram()
{
	GLCall(glDeleteProgram(program));
	program = 0;
}

GLuint ChessBoardTextureProgram::GetVao(GLuint vertex_buffer) const 
{
	GLuint vao;
	GLCall(glGenVertexArrays(1, &vao));
	GLCall(glBindVertexArray(vao));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer));

	GLCall(glEnableVertexAttribArray(Position_vec4));
	GLCall(glVertexAttribPointer(Position_vec4, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, Position)));

	GLCall(glEnableVertexAttribArray(Color_vec4));
	GLCall(glVertexAttribPointer(Color_vec4, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (const void*)offsetof(Vertex, Color)));

	GLCall(glEnableVertexAttribArray(TexCoord_vec2));
	GLCall(glVertexAttribPointer(TexCoord_vec2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, TexCoord)));

	return vao;
}

void ChessBoardTextureProgram::SetupChessBoard() 
{
	#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
	//const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0x193b59ff);
	const glm::u8vec4 cb_color = HEX_TO_U8VEC4(0xeab676ff);
	const glm::u8vec4 bd_color = HEX_TO_U8VEC4(0x0c0f0aff);
	//const glm::u8vec4 rd_color = HEX_TO_U8VEC4(0xff206eff);
	//const glm::u8vec4 yl_color = HEX_TO_U8VEC4(0xfbff12ff);
	#undef HEX_TO_U8VEC4

	//4 Chess boarders
	Rectangle b1;
	b1.rectangle_size = glm::vec4( -CHESSBOARD_SIZE - CHESSBOARD_BOARDER, 
									-CHESSBOARD_SIZE - CHESSBOARD_BOARDER, 
									-CHESSBOARD_SIZE, 
									CHESSBOARD_SIZE + CHESSBOARD_BOARDER);
	b1.color = bd_color;
	board_assets.push_back(b1);

	Rectangle b2;
	b2.rectangle_size = glm::vec4( CHESSBOARD_SIZE,
									-CHESSBOARD_SIZE - CHESSBOARD_BOARDER,
									CHESSBOARD_SIZE + CHESSBOARD_BOARDER,
									CHESSBOARD_SIZE + CHESSBOARD_BOARDER);
	b2.color = bd_color;
	board_assets.push_back(b2);

	Rectangle b3;
	b3.rectangle_size = glm::vec4( -CHESSBOARD_SIZE - CHESSBOARD_BOARDER,
									CHESSBOARD_SIZE,
									 CHESSBOARD_SIZE + CHESSBOARD_BOARDER,
									CHESSBOARD_SIZE + CHESSBOARD_BOARDER);
	b3.color = bd_color;
	board_assets.push_back(b3);

	Rectangle b4;
	b4.rectangle_size = glm::vec4(-CHESSBOARD_SIZE - CHESSBOARD_BOARDER,
									-CHESSBOARD_SIZE - CHESSBOARD_BOARDER,
									CHESSBOARD_SIZE + CHESSBOARD_BOARDER,
									-CHESSBOARD_SIZE);
	b4.color = bd_color;
	board_assets.push_back(b4);


	// Chessboard
	Rectangle cb;
	cb.rectangle_size = glm::vec4(-CHESSBOARD_SIZE, -CHESSBOARD_SIZE, CHESSBOARD_SIZE, CHESSBOARD_SIZE);
	cb.color = cb_color;
	board_assets.push_back(cb);


	// Chessboard lines
	for (size_t i = 1; i < 14; i++)
	{
		Rectangle l;
		l.rectangle_size = glm::vec4( -CHESSBOARD_SIZE + i * CHESS_BOX_SIZE,
										-CHESSBOARD_SIZE,
									  -CHESSBOARD_SIZE + CHESSBOARD_BOARDER + i * CHESS_BOX_SIZE,
										CHESSBOARD_SIZE);
		l.color = bd_color;
		board_assets.push_back(l);

		Rectangle l2;
		l2.rectangle_size = glm::vec4( -CHESSBOARD_SIZE,
										-CHESSBOARD_SIZE + i * CHESS_BOX_SIZE,
										CHESSBOARD_SIZE,
										-CHESSBOARD_SIZE + i * CHESS_BOX_SIZE + CHESSBOARD_BOARDER);
		l2.color = bd_color;
		board_assets.push_back(l2);
	}

	//std::cout << board_assets.size() << std::endl;
	//for (auto& r : board_assets)
	//{
	//	GLCall(glGenBuffers(1, &r.vertex_buffer));
	//	GLCall(glBindBuffer(GL_ARRAY_BUFFER, r.vertex_buffer));
	//	Vertex vertices[]{
	//		{{r.rectangle_size[0], r.rectangle_size[1]}, r.color, {0,1}},
	//		{{r.rectangle_size[2], r.rectangle_size[1]}, r.color, {1,1}},
	//		{{r.rectangle_size[0], r.rectangle_size[3]}, r.color, {0,0}},
	//		{{r.rectangle_size[2], r.rectangle_size[3]}, r.color, {1,0}}
	//	};
	//	GLCall(glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vertex), static_cast<const void*>(vertices), GL_STATIC_DRAW));
	//	r.vertex_array = GetVao(r.vertex_buffer);
	//}
}

void ChessBoardTextureProgram::DrawChessBoard(glm::uvec2 const& drawable_size) const
{
	for (const auto& r : board_assets)
	{
		GLuint vertex_buffer = -1U;
		GLuint vertex_array = -1U;

		GLCall(glGenBuffers(1, &vertex_buffer));
		GLCall(glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer));
		Vertex vertices[]{
			{{r.rectangle_size[0] / drawable_size.x, r.rectangle_size[1] / drawable_size.y}, r.color, {0,1}},
			{{r.rectangle_size[2] / drawable_size.x, r.rectangle_size[1] / drawable_size.y}, r.color, {1,1}},
			{{r.rectangle_size[0] / drawable_size.x, r.rectangle_size[3] / drawable_size.y}, r.color, {0,0}},
			{{r.rectangle_size[2] / drawable_size.x, r.rectangle_size[3] / drawable_size.y}, r.color, {1,0}}
		};
		GLCall(glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vertex), static_cast<const void*>(vertices), GL_STATIC_DRAW));
		vertex_array = GetVao(vertex_buffer);

		GLCall(glUseProgram(program));
		GLCall(glBindVertexArray(vertex_array));
		GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rectangle_index_buffer));
		GLCall(glActiveTexture(GL_TEXTURE0));
		GLCall(glBindTexture(GL_TEXTURE_2D, rectangle_texture));
		GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, static_cast<const void*>(0)));

		GLCall(glDeleteBuffers(1, &vertex_buffer));
		GLCall(glDeleteVertexArrays(1, &vertex_array));

		//for (auto& r : board_assets)
		//{
		//	GLCall(glUseProgram(program));
		//	GLCall(glBindVertexArray(r.vertex_buffer));
		//	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rectangle_index_buffer));
		//	GLCall(glActiveTexture(GL_TEXTURE0));
		//	GLCall(glBindTexture(GL_TEXTURE_2D, rectangle_texture));
		//	GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, static_cast<const void*>(0)));
		//}
	}
}

void ChessBoardTextureProgram::SetupChessPiece(std::vector<Circle>& chess_pieces, const glm::vec2 origin, const glm::u8vec4 color) const
{
	Circle c1;
	c1.origin = origin;
	c1.color = color;

	chess_pieces.push_back(c1);
}

void ChessBoardTextureProgram::DrawChessPieces(std::vector<Circle>& chess_pieces, const glm::uvec2& drawable_size) const 
{
	for (const auto& c : chess_pieces)
	{
		GLuint vertex_buffer = -1U;
		GLuint vertex_array = -1U;

		GLCall(glGenBuffers(1, &vertex_buffer));
		vertex_array = GetVao(vertex_buffer);
		GLCall(glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer));

		std::vector<Vertex> vertices(CIRCLE_VERTICES_COUNT + 1);

		Vertex& o = vertices[0];
		o.Color = c.color;
		o.Position[0] = (c.origin[0] / drawable_size.x);
		o.Position[1] = (c.origin[1] / drawable_size.y);
		
		float angle_diff = 360.0f / CIRCLE_VERTICES_COUNT;
		float angle = 0.0f;

		for (size_t i = 0; i < CIRCLE_VERTICES_COUNT; i++)
		{
			Vertex& v = vertices[i + 1];
			v.Color = c.color;
			v.Position[0] = (c.origin[0] + CHESS_PIECE_RADIUS * glm::cos(glm::radians(angle))) / drawable_size.x;
			v.Position[1] = (c.origin[1] + CHESS_PIECE_RADIUS * glm::sin(glm::radians(angle))) / drawable_size.y;
			angle += angle_diff;
		}

		GLCall(glBufferData(GL_ARRAY_BUFFER, (CIRCLE_VERTICES_COUNT + 1) * sizeof(Vertex), static_cast<const void*>(vertices.data()), GL_STATIC_DRAW));
		

		GLCall(glUseProgram(program));
		GLCall(glBindVertexArray(vertex_array));
		GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, circle_index_buffer));
		GLCall(glActiveTexture(GL_TEXTURE0));
		GLCall(glBindTexture(GL_TEXTURE_2D, rectangle_texture));
		GLCall(glDrawElements(GL_TRIANGLES, CIRCLE_VERTICES_COUNT * 3, GL_UNSIGNED_INT, static_cast<const void*>(0)));

		GLCall(glDeleteBuffers(1, &vertex_buffer));
		GLCall(glDeleteVertexArrays(1, &vertex_array));
	}
}