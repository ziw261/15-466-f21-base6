#include "Mode.hpp"

#include "Connection.hpp"

#include "ColorTextureProgram.hpp"
#include <glm/glm.hpp>

#include <vector>
#include <deque>


#define CHESSBOARD_SIZE 14.0f
#define CHESSBOX_SIZE 2.0f

struct PlayMode : Mode {
	PlayMode(Client &client);
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	void setup_opengl();


	struct Vertex {
		Vertex(glm::vec3 const& Position_, glm::u8vec4 const& Color_, glm::vec2 const& TexCoord_) :
			Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};
	static_assert(sizeof(Vertex) == 4 * 3 + 1 * 4 + 4 * 2, "Vertex should be packed");

	inline float ScreenToAnchor(float pos, int size) {
		//return (2 * pos) / size;
		return (2 * pos) / size;
		//return pos;
	}


	//----- game state -----
	const float boarder_radius = 0.05f;
	glm::vec2 chessboard_size = glm::vec2(CHESSBOARD_SIZE, CHESSBOARD_SIZE);
	bool is_mouse_up = false;
	bool change_color = false;

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	glm::vec2 mouse_pos;

	//last message from server:
	std::string server_message;

	//connection to server:
	Client &client;



	//Shader program that draws transformed, vertices tinted with vertex colors:
	ColorTextureProgram color_texture_program;

	//Buffer used to hold vertex data during drawing:
	GLuint vertex_buffer = 0;

	//Vertex Array Object that maps buffer locations to color_texture_program attribute locations:
	GLuint vertex_buffer_for_color_texture_program = 0;

	//Solid white texture:
	GLuint white_tex = 0;

	//matrix that maps from clip coordinates to court-space coordinates:
	glm::mat3x2 clip_to_court = glm::mat3x2(1.0f);
	// computed in draw() as the inverse of OBJECT_TO_CLIP
	// (stored here so that the mouse handling code can use it to position the paddle)

};
