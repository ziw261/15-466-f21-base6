
#include "Connection.hpp"
#include "ChessBoardData.hpp"
#include "hex_dump.hpp"

#include <chrono>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <unordered_map>

#ifdef _WIN32
extern "C" { uint32_t GetACP(); }
#endif
int main(int argc, char **argv) {
#ifdef _WIN32
	{ //when compiled on windows, check that code page is forced to utf-8 (makes file loading/saving work right):
		//see: https://docs.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page
		uint32_t code_page = GetACP();
		if (code_page == 65001) {
			std::cout << "Code page is properly set to UTF-8." << std::endl;
		} else {
			std::cout << "WARNING: code page is set to " << code_page << " instead of 65001 (UTF-8). Some file handling functions may fail." << std::endl;
		}
	}

	//when compiled on windows, unhandled exceptions don't have their message printed, which can make debugging simple issues difficult.
	try {
#endif

	//------------ argument parsing ------------

	if (argc != 2) {
		std::cerr << "Usage:\n\t./server <port>" << std::endl;
		return 1;
	}

	//------------ initialization ------------

	Server server(argv[1]);


	//------------ main loop ------------
	constexpr float ServerTick = 1.0f / 10.0f; //TODO: set a server tick that makes sense for your game

	//server state:

	//per-client state:
	struct PlayerInfo {
		PlayerInfo() {
			static uint32_t next_player_id = 1;
			name = "Player" + std::to_string(next_player_id);
			next_player_id += 1;
		}
		std::string name;

		//uint32_t left_presses = 0;
		//uint32_t right_presses = 0;
		//uint32_t up_presses = 0;
		//uint32_t down_presses = 0;

		//int32_t total = 0;
		//int8_t last_pos_x = 0;
		//int8_t last_pos_y = 0;
		//int8_t chess_color = 0;
	};
	std::unordered_map< Connection *, PlayerInfo > players;

	// game state:
	uint8_t curr_player = 1;
	std::vector<std::vector<int>> chess_board(NUM_PIECES_PER_LINE_HALF * 2 + 1, std::vector<int>(NUM_PIECES_PER_LINE_HALF * 2 + 1, 0));
	int8_t last_pos_x = 0;
	int8_t last_pos_y = 0;
	uint8_t color_to_draw = 0;

	while (true) {
		static auto next_tick = std::chrono::steady_clock::now() + std::chrono::duration< double >(ServerTick);
		//process incoming data from clients until a tick has elapsed:
		while (true) {
			auto now = std::chrono::steady_clock::now();
			double remain = std::chrono::duration< double >(next_tick - now).count();
			if (remain < 0.0) {
				next_tick += std::chrono::duration< double >(ServerTick);
				break;
			}
			server.poll([&](Connection *c, Connection::Event evt){
				if (evt == Connection::OnOpen) {
					//client connected:

					if(players.size() < PLAYER_NUM)
						//create some player info for them:
						players.emplace(c, PlayerInfo());


				} else if (evt == Connection::OnClose) {
					//client disconnected:

					//remove them from the players list:
					auto f = players.find(c);
					assert(f != players.end());
					players.erase(f);


				} else { assert(evt == Connection::OnRecv);
					
					//look up in players list:
					auto f = players.find(c);
					assert(f != players.end());
					PlayerInfo &player = f->second;

					//got data from client:
					std::cout << "got bytes:\n" << hex_dump(c->recv_buffer); std::cout.flush();

					//handle messages from client:
					//TODO: update for the sorts of messages your clients send
					while (c->recv_buffer.size() >= 3) {
						//expecting five-byte messages 'b' (left count) (right count) (down count) (up count)
						char type = c->recv_buffer[0];
						if (type != 'a') {
							std::cout << " message of non-'b' type received from client!" << std::endl;
							//shut down client connection:
							c->close();
							return;
						}
						//uint8_t left_count = c->recv_buffer[1];
						//uint8_t right_count = c->recv_buffer[2];
						//uint8_t down_count = c->recv_buffer[3];
						//uint8_t up_count = c->recv_buffer[4];
						// 
						//player.left_presses += left_count;
						//player.right_presses += right_count;
						//player.down_presses += down_count;
						//player.up_presses += up_count;

						if ((player.name[6] - '0') == curr_player) {
							int8_t pos_x = c->recv_buffer[1];
							int8_t pos_y = c->recv_buffer[2];


							// Check valid
							if (chess_board[pos_x + NUM_PIECES_PER_LINE_HALF][pos_y + NUM_PIECES_PER_LINE_HALF] == 0) {
								chess_board[pos_x + NUM_PIECES_PER_LINE_HALF][pos_y + NUM_PIECES_PER_LINE_HALF] = curr_player;
								last_pos_x = pos_x;
								last_pos_y = pos_y;
								color_to_draw = curr_player;
								curr_player = (curr_player + 1 - 1) % PLAYER_NUM + 1;
							}
							else
							{
								std::cout << "This place already has a piece" << std::endl;
							}

							//curr_player = (curr_player + 1) % PLAYER_NUM;
						}

						c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 3);
					}
				}
			}, remain);
		}

		//update current game state
		//TODO: replace with *your* game state update
		std::string status_message = "";
		//int32_t overall_sum = 0;
		//for (auto &[c, player] : players) {
		//	(void)c; //work around "unused variable" warning on whatever version of g++ github actions is running
		//	for (; player.left_presses > 0; --player.left_presses) {
		//		player.total -= 1;
		//	}
		//	for (; player.right_presses > 0; --player.right_presses) {
		//		player.total += 1;
		//	}
		//	for (; player.down_presses > 0; --player.down_presses) {
		//		player.total -= 10;
		//	}
		//	for (; player.up_presses > 0; --player.up_presses) {
		//		player.total += 10;
		//	}
		//	if (status_message != "") status_message += " + ";
		//	status_message += std::to_string(player.total) + " (" + player.name + ")";

		//	overall_sum += player.total;
		//}
		//status_message += " = " + std::to_string(overall_sum);
		//std::cout << status_message << std::endl; //DEBUG

		
		status_message = std::to_string(color_to_draw) + "," + std::to_string(last_pos_x) + "," + std::to_string(last_pos_y);
		//send updated game state to all clients
		//TODO: update for your game state
		for (auto &[c, player] : players) {
			(void)player; //work around "unused variable" warning on whatever g++ github actions uses
			//send an update starting with 'm', a 24-bit size, and a blob of text:
			c->send('m');
			c->send(uint8_t(status_message.size() >> 16));
			c->send(uint8_t((status_message.size() >> 8) % 256));
			c->send(uint8_t(status_message.size() % 256));
			c->send_buffer.insert(c->send_buffer.end(), status_message.begin(), status_message.end());
		}
	}

	return 0;

#ifdef _WIN32
	} catch (std::exception const &e) {
		std::cerr << "Unhandled exception:\n" << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "Unhandled exception (unknown type)." << std::endl;
		throw;
	}
#endif
}
