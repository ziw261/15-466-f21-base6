#include "Mode.hpp"

#include "Connection.hpp"
#include "ChessBoardData.hpp"
#include "ColorTextureProgram.hpp"
#include "ChessBoardTextureProgram.hpp"
#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode(Client &client);
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	bool CheckMouseClickValid(const glm::uvec2& window_size);

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	glm::vec2 mouse_pos;
	std::vector<ChessBoardTextureProgram::Circle> chess_pieces;

	//last message from server:
	std::string server_message;

	//connection to server:
	Client &client;
};
