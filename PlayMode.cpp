#include "PlayMode.hpp"

#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

PlayMode::PlayMode(Client& client_) : client(client_) {
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const& evt, glm::uvec2 const& window_size) {

	//if (evt.type == SDL_KEYDOWN) {
	//	if (evt.key.repeat) {
	//		//ignore repeats
	//	}
	//	else if (evt.key.keysym.sym == SDLK_a) {
	//		left.downs += 1;
	//		left.pressed = true;
	//		return true;
	//	}
	//	else if (evt.key.keysym.sym == SDLK_d) {
	//		right.downs += 1;
	//		right.pressed = true;
	//		return true;
	//	}
	//	else if (evt.key.keysym.sym == SDLK_w) {
	//		up.downs += 1;
	//		up.pressed = true;
	//		return true;
	//	}
	//	else if (evt.key.keysym.sym == SDLK_s) {
	//		down.downs += 1;
	//		down.pressed = true;
	//		return true;
	//	}
	//}
	//else if (evt.type == SDL_KEYUP) {
	//	if (evt.key.keysym.sym == SDLK_a) {
	//		left.pressed = false;
	//		return true;
	//	}
	//	else if (evt.key.keysym.sym == SDLK_d) {
	//		right.pressed = false;
	//		return true;
	//	}
	//	else if (evt.key.keysym.sym == SDLK_w) {
	//		up.pressed = false;
	//		return true;
	//	}
	//	else if (evt.key.keysym.sym == SDLK_s) {
	//		down.pressed = false;
	//		return true;
	//	}
	//}

	if (evt.type == SDL_MOUSEMOTION) {
		mouse_pos = glm::vec2(
			evt.motion.x / float(window_size.x) * 2 - 1.0f,
			-evt.motion.y / float(window_size.y) * 2 + 1.0f
		);

		//std::cout << "Mouse motion: x: " << mouse_pos.x << ", y: " << mouse_pos.y << "\n";
		return true;
	}
	else if (evt.type == SDL_MOUSEBUTTONUP) {
		if (evt.button.button == SDL_BUTTON_LEFT) {
			//std::cout << mouse_pos.x << " " << mouse_pos.y << std::endl;
			//std::cout << (-540.0f / (float)window_size.x) << " " << (-540.0f / (float)window_size.y) << std::endl;
			//if (mouse_pos.x == -540.0f / (float)window_size.x)
			//	if (mouse_pos.y == -540.0f / (float)window_size.y)
			//	{
			//	}
			CheckMouseClickValid(window_size);
		}
	}

	return false;
}

bool PlayMode::CheckMouseClickValid(const glm::uvec2& window_size) {
	glm::vec2 pixel_size = {mouse_pos.x * window_size.x, mouse_pos.y * window_size.y};
	int chessboard_x = static_cast<int>(std::round(pixel_size.x / CHESS_BOX_SIZE));
	int chessboard_y = static_cast<int>(std::round(pixel_size.y / CHESS_BOX_SIZE));

	int chesspiece_origin_x = chessboard_x * CHESS_BOX_SIZE;
	int chesspiece_origin_y = chessboard_y * CHESS_BOX_SIZE;
	
	int piece_boundary_pos = static_cast<int>((CHESSBOARD_SIZE * 2 / CHESS_BOX_SIZE - 1) / 2);

	if (chessboard_x < -piece_boundary_pos || chessboard_x > piece_boundary_pos
		|| chessboard_y < -piece_boundary_pos || chessboard_y > piece_boundary_pos)
		return false;

	chessboard_texture_program->SetupChessPiece(chess_pieces, glm::vec2(chesspiece_origin_x, chesspiece_origin_y), glm::u8vec4(0xff));	

	return true;
}

void PlayMode::update(float elapsed) {

	//queue data for sending to server:
	//TODO: send something that makes sense for your game
	//if (left.downs || right.downs || down.downs || up.downs) {
	//	//send a five-byte message of type 'b':
	//	client.connections.back().send('b');
	//	client.connections.back().send(left.downs);
	//	client.connections.back().send(right.downs);
	//	client.connections.back().send(down.downs);
	//	client.connections.back().send(up.downs);
	//}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;

	//send/receive data:
	client.poll([this](Connection* c, Connection::Event event) {
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		}
		else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		}
		else {
			assert(event == Connection::OnRecv);
			std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush();
			//expecting message(s) like 'm' + 3-byte length + length bytes of text:
			while (c->recv_buffer.size() >= 4) {
				std::cout << "[" << c->socket << "] recv'd data. Current buffer:\n" << hex_dump(c->recv_buffer); std::cout.flush();
				char type = c->recv_buffer[0];
				if (type != 'm') {
					throw std::runtime_error("Server sent unknown message type '" + std::to_string(type) + "'");
				}
				uint32_t size = (
					(uint32_t(c->recv_buffer[1]) << 16) | (uint32_t(c->recv_buffer[2]) << 8) | (uint32_t(c->recv_buffer[3]))
					);
				if (c->recv_buffer.size() < 4 + size) break; //if whole message isn't here, can't process
				//whole message *is* here, so set current server message:
				server_message = std::string(c->recv_buffer.begin() + 4, c->recv_buffer.begin() + 4 + size);

				//and consume this part of the buffer:
				c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 4 + size);
			}
		}
		}, 0.0);
}

void PlayMode::draw(glm::uvec2 const& drawable_size) {
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//GL_ERRORS();

	chessboard_texture_program->DrawChessBoard(drawable_size);
	chessboard_texture_program->DrawChessPieces(chess_pieces, drawable_size);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		auto draw_text = [&](glm::vec2 const& at, std::string const& text, float H) {
			lines.draw_text(text,
				glm::vec3(at.x, at.y, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			float ofs = 2.0f / drawable_size.y;
			lines.draw_text(text,
				glm::vec3(at.x + ofs, at.y + ofs, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		};

		draw_text(glm::vec2(-aspect + 0.1f, 0.0f), server_message, 0.09f);

		draw_text(glm::vec2(-aspect + 0.1f, -0.9f), "(press WASD to change your total)", 0.09f);
	}
}