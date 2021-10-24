#pragma once

#define CHESSBOARD_SIZE 630.0f
#define CHESSBOARD_BOARDER 5.0f
#define CHESSBOX_SIZE 2.0f
#define CIRCLE_VERTICES_COUNT 36
#define CHESS_PIECE_RADIUS 45
#define CHESS_BOX_SIZE 90
#define PLAYER_NUM 3
#define NUM_PIECES_PER_LINE_HALF static_cast<int>((CHESSBOARD_SIZE * 2 / CHESS_BOX_SIZE - 1) / 2)
#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
#define NUM_PIECE_TO_WIN 4