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

GLuint ChessBoardTextureProgram::rectangle_index_buffer = -1U;
GLuint ChessBoardTextureProgram::rectangle_texture = 0;

static Load <void> load_rectangle_index_buffer(LoadTagEarly, []() {
	unsigned int rectangle_index_buffer_content[] = { 0, 1, 2, 1, 2, 3 };
	GLCall(glGenBuffers(1, &ChessBoardTextureProgram::rectangle_index_buffer));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ChessBoardTextureProgram::rectangle_index_buffer));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), rectangle_index_buffer_content, GL_STATIC_DRAW));
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
	//const glm::u8vec4 bd_color = HEX_TO_U8VEC4(0x0c0f0aff);
	//const glm::u8vec4 rd_color = HEX_TO_U8VEC4(0xff206eff);
	//const glm::u8vec4 yl_color = HEX_TO_U8VEC4(0xfbff12ff);
	#undef HEX_TO_U8VEC4

	//4 Chess boarders
	Rectangle b1;
	b1.rectangle_size = glm::vec4(-0.83f, -0.83f, 0.83f, 0.83f);
	b1.color = cb_color;
	board_assets.push_back(b1);

	

	for (auto& r : board_assets)
	{
		GLCall(glGenBuffers(1, &r.vertex_buffer));
		GLCall(glBindBuffer(GL_ARRAY_BUFFER, r.vertex_buffer));
		Vertex vertices[]{
			{{r.rectangle_size[0], r.rectangle_size[1]}, r.color, {0,1}},
			{{r.rectangle_size[2], r.rectangle_size[1]}, r.color, {1,1}},
			{{r.rectangle_size[0], r.rectangle_size[3]}, r.color, {0,0}},
			{{r.rectangle_size[2], r.rectangle_size[3]}, r.color, {1,0}}
		};
		GLCall(glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vertex), static_cast<const void*>(vertices), GL_STATIC_DRAW));
		r.vertex_array = GetVao(r.vertex_buffer);
	}
}

void ChessBoardTextureProgram::DrawChessBoard() const
{
	for (auto& r : board_assets)
	{
		GLCall(glUseProgram(program));
		GLCall(glBindVertexArray(r.vertex_buffer));
		GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rectangle_index_buffer));
		GLCall(glActiveTexture(GL_TEXTURE0));
		GLCall(glBindTexture(GL_TEXTURE_2D, rectangle_texture));
		GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, static_cast<const void*>(0)));
	}
}