#include "PlayMode.hpp"
#include <signal.h>
#include "DrawLines.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"
#include "hex_dump.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

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



PlayMode::PlayMode(Client &client_) : client(client_) {
	setup_opengl();
}

PlayMode::~PlayMode() {
}

void PlayMode::setup_opengl() {
	//----- allocate OpenGL resources -----
	//vertex buffer:
	GLCall(glGenBuffers(1, &vertex_buffer));
	//for now, buffer will be un-filled.
	
	//vertex array mapping buffer for color_texture_program:
	//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
	GLCall(glGenVertexArrays(1, &vertex_buffer_for_color_texture_program));

	//set vertex_buffer_for_color_texture_program as the current vertex array object:
	GLCall(glBindVertexArray(vertex_buffer_for_color_texture_program));

	//set vertex_buffer as the source of glVertexAttribPointer() commands:
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer));

	//set up the vertex array object to describe arrays of PongMode::Vertex:
	GLCall(glVertexAttribPointer(
		color_texture_program.Position_vec4, //attribute
		3, //size
		GL_FLOAT, //type
		GL_FALSE, //normalized
		sizeof(Vertex), //stride
		(GLbyte*)0 + 0 //offset
	));
	GLCall(glEnableVertexAttribArray(color_texture_program.Position_vec4));
	//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

	GLCall(glVertexAttribPointer(
		color_texture_program.Color_vec4, //attribute
		4, //size
		GL_UNSIGNED_BYTE, //type
		GL_TRUE, //normalized
		sizeof(Vertex), //stride
		(GLbyte*)0 + 4 * 3 //offset
	));
	GLCall(glEnableVertexAttribArray(color_texture_program.Color_vec4));

	GLCall(glVertexAttribPointer(
		color_texture_program.TexCoord_vec2, //attribute
		2, //size
		GL_FLOAT, //type
		GL_FALSE, //normalized
		sizeof(Vertex), //stride
		(GLbyte*)0 + 4 * 3 + 4 * 1 //offset
	));
	GLCall(glEnableVertexAttribArray(color_texture_program.TexCoord_vec2));

	//done referring to vertex_buffer, so unbind it:
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));

	//done setting up vertex array object, so unbind it:
	GLCall(glBindVertexArray(0));
	

	 //solid white texture:
	//ask OpenGL to fill white_tex with the name of an unused texture object:
	GLCall(glGenTextures(1, &white_tex));

	//bind that texture object as a GL_TEXTURE_2D-type texture:
	GLCall(glBindTexture(GL_TEXTURE_2D, white_tex));

	//upload a 1x1 image of solid white to the texture:
	glm::uvec2 size = glm::uvec2(1, 1);
	std::vector< glm::u8vec4 > data(size.x * size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data()));

	//set filtering and wrapping parameters:
	//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

	//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
	GLCall(glGenerateMipmap(GL_TEXTURE_2D));

	//Okay, texture uploaded, can unbind it:
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));	
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.repeat) {
			//ignore repeats
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	}
	else if (evt.type == SDL_MOUSEMOTION) {
		mouse_pos = glm::vec2(
			evt.motion.x / float(window_size.x) * 2 - 1.0f,
			-evt.motion.y / float(window_size.y) * 2 + 1.0f
		);

		return true;
	}
	else if (evt.type == SDL_MOUSEBUTTONUP) {
		if (evt.button.button == SDL_BUTTON_LEFT) {
			is_mouse_up = true;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//queue data for sending to server:
	//TODO: send something that makes sense for your game
	if (left.downs || right.downs || down.downs || up.downs) {
		//send a five-byte message of type 'b':
		client.connections.back().send('b');
		client.connections.back().send(left.downs);
		client.connections.back().send(right.downs);
		client.connections.back().send(down.downs);
		client.connections.back().send(up.downs);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;

	//send/receive data:
	client.poll([this](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			std::cout << "[" << c->socket << "] opened" << std::endl;
		} else if (event == Connection::OnClose) {
			std::cout << "[" << c->socket << "] closed (!)" << std::endl;
			throw std::runtime_error("Lost connection to server!");
		} else { assert(event == Connection::OnRecv);
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

void PlayMode::draw(glm::uvec2 const &drawable_size) {

	std::vector< Vertex > vertices;

	#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
	const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0x193b59ff);
	const glm::u8vec4 cb_color = HEX_TO_U8VEC4(0xeab676ff);
	const glm::u8vec4 bd_color = HEX_TO_U8VEC4(0x0c0f0aff);
	const glm::u8vec4 rd_color = HEX_TO_U8VEC4(0xff206eff);
	const glm::u8vec4 yl_color = HEX_TO_U8VEC4(0xfbff12ff);
	#undef HEX_TO_U8VEC4


	auto draw_rectangle = [&vertices](glm::vec2 const& center, glm::vec2 const& radius, glm::u8vec4 const& color) {
		//draw rectangle as two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(center.x - radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x + radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x + radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x - radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x + radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x - radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};


	const float padding = 0.14f; //padding between outside of walls and edge of window

	// Chessboard
	{
		// Chessboard boarder
		draw_rectangle(glm::vec2(-chessboard_size.x - boarder_radius, 0.0f), glm::vec2(boarder_radius, chessboard_size.y + 2.0f * boarder_radius), bd_color);
		draw_rectangle(glm::vec2(chessboard_size.x + boarder_radius, 0.0f), glm::vec2(boarder_radius, chessboard_size.y + 2.0f * boarder_radius), bd_color);
		draw_rectangle(glm::vec2(0.0f, -chessboard_size.y - boarder_radius), glm::vec2(chessboard_size.x, boarder_radius), bd_color);
		draw_rectangle(glm::vec2(0.0f, chessboard_size.y + boarder_radius), glm::vec2(chessboard_size.x, boarder_radius), bd_color);

		// Chessboard
		draw_rectangle(glm::vec2(0, 0), glm::vec2(chessboard_size.x, chessboard_size.y), cb_color);

		// Lines
		for (uint8_t i = 1; i < CHESSBOARD_SIZE; i++) {
			draw_rectangle(glm::vec2(-chessboard_size.x + i * CHESSBOX_SIZE, 0.0f), glm::vec2(boarder_radius, chessboard_size.y), bd_color);
			draw_rectangle(glm::vec2(0.0f, -chessboard_size.y + i * CHESSBOX_SIZE), glm::vec2(chessboard_size.x, boarder_radius), bd_color);
		}
	}


	//compute area that should be visible:
	glm::vec2 scene_min = glm::vec2(
		-chessboard_size.x - 2.0f * boarder_radius - padding,
		-chessboard_size.y - 2.0f * boarder_radius - padding
	);
	glm::vec2 scene_max = glm::vec2(
		chessboard_size.x + 2.0f * boarder_radius + padding,
		chessboard_size.y + 2.0f * boarder_radius + padding
	);

	//compute window aspect ratio:
	float aspect = drawable_size.x / float(drawable_size.y);
	//we'll scale the x coordinate by 1.0 / aspect to make sure things stay square.

	//compute scale factor for court given that...
	float scale = std::min(
		(2.0f * aspect) / (scene_max.x - scene_min.x), //... x must fit in [-aspect,aspect] ...
		(2.0f) / (scene_max.y - scene_min.y) //... y must fit in [-1,1].
	);


	float dest_x = ScreenToAnchor(-chessboard_size.x + CHESSBOX_SIZE, drawable_size.x * scale);
	float dest_y = ScreenToAnchor(-chessboard_size.y + CHESSBOX_SIZE, drawable_size.y * scale);

	if (is_mouse_up) {
		std::cout << mouse_pos.x << " " << mouse_pos.y << std::endl;
		std::cout << dest_x << " " << dest_y << std::endl;
		std::cout << drawable_size.x*scale << " " << drawable_size.y*scale << std::endl;
		if (mouse_pos.x == dest_x && mouse_pos.y == dest_y)
			change_color = true;
	}


	if (change_color)
		draw_rectangle(glm::vec2(-chessboard_size.x + CHESSBOX_SIZE, -chessboard_size.y + CHESSBOX_SIZE), glm::vec2(1.0f), yl_color);
	else
		draw_rectangle(glm::vec2(-chessboard_size.x + CHESSBOX_SIZE, -chessboard_size.y + CHESSBOX_SIZE), glm::vec2(1.0f), rd_color);

	draw_rectangle(glm::vec2(0.0f), glm::vec2(1.0f), yl_color);

	//------ compute court-to-window transform ------

	////compute area that should be visible:
	//glm::vec2 scene_min = glm::vec2(
	//	-chessboard_size.x - 2.0f * boarder_radius - padding,
	//	-chessboard_size.y - 2.0f * boarder_radius - padding
	//);
	//glm::vec2 scene_max = glm::vec2(
	//	chessboard_size.x + 2.0f * boarder_radius + padding,
	//	chessboard_size.y + 2.0f * boarder_radius + padding
	//);

	////compute window aspect ratio:
	//float aspect = drawable_size.x / float(drawable_size.y);
	////we'll scale the x coordinate by 1.0 / aspect to make sure things stay square.

	////compute scale factor for court given that...
	//float scale = std::min(
	//	(2.0f * aspect) / (scene_max.x - scene_min.x), //... x must fit in [-aspect,aspect] ...
	//	(2.0f) / (scene_max.y - scene_min.y) //... y must fit in [-1,1].
	//);

	glm::vec2 center = 0.5f * (scene_max + scene_min);

	//build matrix that scales and translates appropriately:
	glm::mat4 court_to_clip = glm::mat4(
		glm::vec4(scale / aspect, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, scale, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(-center.x * (scale / aspect), -center.y * scale, 0.0f, 1.0f)
	);

	//NOTE: glm matrices are specified in *Column-Major* order,
	// so each line above is specifying a *column* of the matrix(!)

	//also build the matrix that takes clip coordinates to court coordinates (used for mouse handling):
	clip_to_court = glm::mat3x2(
		glm::vec2(aspect / scale, 0.0f),
		glm::vec2(0.0f, 1.0f / scale),
		glm::vec2(center.x, center.y)
	);


	//---- actual drawing ----

	//clear the color buffer:
	GLCall(glClearColor(bg_color.r / 255.0f, bg_color.g / 255.0f, bg_color.b / 255.0f, bg_color.a / 255.0f));
	GLCall(glClear(GL_COLOR_BUFFER_BIT));

	//use alpha blending:
	GLCall(glEnable(GL_BLEND));
	GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
	//don't use the depth test:
	GLCall(glDisable(GL_DEPTH_TEST));

	//upload vertices to vertex_buffer:
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer)); //set vertex_buffer as current
	GLCall(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW)); //upload vertices array
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));

	//set color_texture_program as current program:
	GLCall(glUseProgram(color_texture_program.program));

	//upload OBJECT_TO_CLIP to the proper uniform location:
	GLCall(glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(court_to_clip)));
		
	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	GLCall(glBindVertexArray(vertex_buffer_for_color_texture_program));

	//bind the solid white texture to location zero so things will be drawn just with their colors:
	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, white_tex));

	//run the OpenGL pipeline:
	GLCall(glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size())));

	//unbind the solid white texture:
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));

	//reset vertex array to none:
	GLCall(glBindVertexArray(0));

	//reset current program to none:
	GLCall(glUseProgram(0));
	
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
