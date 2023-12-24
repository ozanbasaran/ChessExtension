
/*
 * chessboard.C 
 *
 * PostgreSQL chessgame & chessboard Type:
 *
 * Authors: Zhuang Xianyun, Bouzaher Mohamed, Başaran Ozan, Demonceau Quentin
 */

#include <stdbool.h>
#include <stdio.h>
#include <postgres.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "utils/builtins.h"
#include "libpq/pqformat.h"

#include "smallchesslib.h"

#include "chessboard.c"
#include "chessgame.c"

PG_MODULE_MAGIC;



const int max_game_size = 1046;

chessboard* getBoardInternal(chessgame* game, int nbHalfmoves);
static chessgame* getOpeningInternal(chessgame, int, chessgame* truncatedGame);
Datum getBoard(PG_FUNCTION_ARGS);

chessboard* getBoardInternal(chessgame* game, int nbHalfmoves ) { //chessboard* getBoard(chessgame* game, int nbHalfmoves)
	/* Return the board state at a given half-move (0 is start)*/

	SCL_Game SCLgame;
	SCL_Board startState = SCL_BOARD_START_STATE;
	char string[256];
	chessgame* c;
	
	// truncate the game
	chessgame* truncatedGame = (chessgame*)malloc(sizeof(chessgame));
	c = getOpeningInternal(*game, nbHalfmoves,truncatedGame);
	
	// Use SCL to obtain the FEN board string
	SCL_boardFromFEN(startState, c->halfmoves);
	SCL_gameInit(&SCLgame, startState);
	SCL_boardToFEN(SCLgame.board, string);
	
	// return the converted FEN board string into a chessboard
	return chessboard_parse(string);
}

static chessgame* getOpeningInternal(chessgame game, int nbHalfmoves, chessgame* truncatedGame) {
	/* (=getFirstMoves)
	 * Returns the chessgame truncated to its first "nbHalfmoves" half-moves.
	 * */

	int moves = 0;
	int index = 0;
	while (moves < nbHalfmoves && game.halfmoves[index] != '\0') {
		if (game.halfmoves[index] == ' ' && game.halfmoves[index - 1] != '.') {
			moves++;
		}
		truncatedGame->halfmoves[index] = game.halfmoves[index];
		index++;

		if (moves == nbHalfmoves) {
			truncatedGame->halfmoves[index] = '\0';
		}
	}

	return truncatedGame;
}

PG_FUNCTION_INFO_V1(getOpening);
Datum getOpening(PG_FUNCTION_ARGS){
	chessgame* game = PG_GETARG_CHESSGAME_P(0);
	int nbHalfmoves = PG_GETARG_INT16(1);

	chessgame* truncatedGame = (chessgame*)malloc(sizeof(chessgame));
	truncatedGame = getOpeningInternal(*game, nbHalfmoves, truncatedGame);

	PG_RETURN_CHESSGAME_P(truncatedGame);
}

// static bool hasOpening(chessgame gameToCheck, chessgame gameOpening) {
// 	/* Returns true if the first chess game starts with the exact same set of moves as the second chess game. 
// 	 * The second parameter should not be a full game, but should only contain the opening moves that we want to check (any length of half-moves)*/
// }

PG_FUNCTION_INFO_V1(hasOpening);
Datum hasOpening(PG_FUNCTION_ARGS) {
	chessgame* game1 = PG_GETARG_CHESSGAME_P(0);
	chessgame* game2 = PG_GETARG_CHESSGAME_P(1);
	
	bool result = true;
	for (int i = 0; i < max_game_size; i++){
		// If one of the games ended, we stop comparing
		if(game1->halfmoves[i] == '\0' || game2->halfmoves[i] == '\0'){
			break;
		}
		// Compare both chars at position i
		if (game1->halfmoves[i] != game2->halfmoves[i]){
			result = false;
			break;
		}
	}

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(getBoard);
Datum getBoard(PG_FUNCTION_ARGS) { //chessboard* getBoard(chessgame* game, int nbHalfmoves)
	/* Return the board state at a given half-move (0 is start)*/
	
	chessgame* game = PG_GETARG_CHESSGAME_P(0);
	int nbHalfmoves = PG_GETARG_INT16(1);
	
	PG_RETURN_CHESSBOARD_P(getBoardInternal(game, nbHalfmoves));
	
}

PG_FUNCTION_INFO_V1(hasBoard);
Datum hasBoard(PG_FUNCTION_ARGS) {
	/* Returns true if the chessgame contains the given board state in its first "nbHalfmoves" half-moves.
	 * Only compare the state of the pieces */

	chessgame* game = PG_GETARG_CHESSGAME_P(0);
	chessboard* board = PG_GETARG_CHESSBOARD_P(1);
	int nbHalfmoves = PG_GETARG_INT16(2);

	chessboard* currentBoard = getBoardInternal(game, nbHalfmoves);
	int cmp = strcmp(currentBoard->piecePlacement, board->piecePlacement);
	bool result = (cmp == 0); 
	PG_RETURN_BOOL(result);
}



/*****************************************************************************/
PG_FUNCTION_INFO_V1(chessgame_lt);
Datum chessgame_lt(PG_FUNCTION_ARGS) {
	chessgame* left = PG_GETARG_CHESSGAME_P(0);
	chessgame* right = PG_GETARG_CHESSGAME_P(1);

	// Use strcmp for lexicographical comparison of character arrays
	int result = strcmp(left->halfmoves, right->halfmoves);

	bool t = (result < 0);
	// Return true if left < right
	PG_RETURN_BOOL(t);
}

PG_FUNCTION_INFO_V1(chessgame_lte);
Datum chessgame_lte(PG_FUNCTION_ARGS) {
	chessgame* left = PG_GETARG_CHESSGAME_P(0);
	chessgame* right = PG_GETARG_CHESSGAME_P(1);

	// Use strcmp for lexicographical comparison of character arrays
	int result = strcmp(left->halfmoves, right->halfmoves);

	bool t = (result < 0 || result == 0);
	// Return true if left < right
	PG_RETURN_BOOL(t);
}

PG_FUNCTION_INFO_V1(chessgame_eq);
Datum chessgame_eq(PG_FUNCTION_ARGS) {
	chessgame* left = PG_GETARG_CHESSGAME_P(0);
	chessgame* right = PG_GETARG_CHESSGAME_P(1);

	// Use strcmp for lexicographical comparison of character arrays
	int result = strcmp(left->halfmoves, right->halfmoves);

	bool t = (result == 0);
	// Return true if left == right
	PG_RETURN_BOOL(t);
}

PG_FUNCTION_INFO_V1(chessgame_gt);
Datum chessgame_gt(PG_FUNCTION_ARGS) {
	chessgame* left = PG_GETARG_CHESSGAME_P(0);
	chessgame* right = PG_GETARG_CHESSGAME_P(1);

	// Use strcmp for lexicographical comparison of character arrays
	int result = strcmp(left->halfmoves, right->halfmoves);
	bool t = (result > 0);
	// Return true if left > right
	PG_RETURN_BOOL(t);
}

PG_FUNCTION_INFO_V1(chessgame_gte);
Datum chessgame_gte(PG_FUNCTION_ARGS) {
	chessgame* left = PG_GETARG_CHESSGAME_P(0);
	chessgame* right = PG_GETARG_CHESSGAME_P(1);

	// Use strcmp for lexicographical comparison of character arrays
	int result = strcmp(left->halfmoves, right->halfmoves);
	bool t = (result > 0 || result == 0);
	// Return true if left > right
	PG_RETURN_BOOL(t);
}



static int
chessgame_cmp_internal(chessgame *a, chessgame *b)
{
  	int result = strcmp(a->halfmoves, b->halfmoves);
    return result;
}

PG_FUNCTION_INFO_V1(chessgame_cmp);
Datum
chessgame_cmp(PG_FUNCTION_ARGS)
{
  chessgame *c = PG_GETARG_CHESSGAME_P(0);
  chessgame *d = PG_GETARG_CHESSGAME_P(1);
  int result = chessgame_cmp_internal(c, d);
  PG_FREE_IF_COPY(c, 0);
  PG_FREE_IF_COPY(d, 1);
  PG_RETURN_INT32(result);
}
