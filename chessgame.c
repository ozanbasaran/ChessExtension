
/*
 * chessgame.C 
 *
 * PostgreSQL chessgame Type:
 *
 * Authors: Zhuang Xianyun, Bouzaher Mohamed, Başaran Ozan, Demonceau Quentin
 */

#include <stdbool.h>
#include <stdio.h>
#include <postgres.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

#include "utils/builtins.h"
#include "libpq/pqformat.h"
#include "access/gin.h"
#include "access/stratnum.h"
#include "utils/array.h"
#include "utils/lsyscache.h"
#include "utility.c"
#include "smallchesslib.h"
#include "chessboard.c"


#define GinOverlapStrategy		1
#define GinContainsStrategy		2
#define GinContainedStrategy	3
#define GinEqualStrategy		4


#define DatumGetChessGameP(X)  ((chessgame *) DatumGetPointer(X))
#define ChessGamePGetDatum(X)  PointerGetDatum(X)
#define PG_GETARG_CHESSGAME_P(n) DatumGetChessGameP(PG_GETARG_DATUM(n))
#define PG_RETURN_CHESSGAME_P(x) return ChessGamePGetDatum(x)


void replaceChar(char* str, char find, char replace);
char** extractMoves(const char* chessgame, uint8_t * moveCount);
SCL_Board* createGameWithMoves(const char* moves, uint8_t * moveCount);

/**===============================CHESS=====================================**/

typedef struct {
	// Qe1xBe8(is the longest possible move 7 chars long, we have a 100 full move cap, that means 200 half moves + 1 for the ending (#) a ' ' and the score (1-0) = 200*7+5=1405
	char halfmoves[1405+1]; 
	char score[3+1];
	
} chessgame;

//Same as chessgame_make because we are parsing a string, and our data type is a string, so it's just a copy operation
static chessgame* chessgame_parse(char* moves) {
	// Create a chessgame instance
	chessgame* new_game = (chessgame*)palloc(sizeof(chessgame));

	// Ensure the length of the moves string does not exceed the size of 'halfmoves' array
	int len = strlen(moves);
	char gameSlice[1405 + 1] = "";
	char scoreSlice[3 + 1] = "";

	if (len > sizeof(gameSlice) - 1) {
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("Input moves string is too long for chessgame")));
	}
	
	// Copy moves to the 'halfmoves' array
	slice(moves, gameSlice, 0, len-1-3);
	strcpy(new_game->halfmoves, gameSlice);
	
	slice(moves, scoreSlice, len-1-2, len);
	strcpy(new_game->score, scoreSlice);

	return new_game;
}

static char* chessgame_to_str(const chessgame* chessGame) {
	// Allocate memory for the result string
	char* result = (char*)palloc(1405 + 1);  // +1 for the null terminator

	// Copy the 'halfmoves' array to the result string
	strcpy(result, chessGame->halfmoves);
	strcat(result," ");
	strcat(result, chessGame->score);

	return result;
}


/**=========================================================================**/
/*****************************************************************************/

PG_FUNCTION_INFO_V1(chessgame_in);
Datum chessgame_in(PG_FUNCTION_ARGS) {
	char* str = PG_GETARG_CSTRING(0);
	PG_RETURN_CHESSGAME_P(chessgame_parse(str));
}

PG_FUNCTION_INFO_V1(chessgame_out);
Datum chessgame_out(PG_FUNCTION_ARGS){
	chessgame* c = PG_GETARG_CHESSGAME_P(0);
	char* result = chessgame_to_str(c);
	PG_FREE_IF_COPY(c, 0);
	PG_RETURN_CSTRING(result);
}


PG_FUNCTION_INFO_V1(chessgame_recv);
Datum chessgame_recv(PG_FUNCTION_ARGS) {
	StringInfo buf = (StringInfo)PG_GETARG_POINTER(0);
	chessgame* c = (chessgame*)palloc(sizeof(chessgame));
	strcpy(c->halfmoves, pq_getmsgstring(buf));
	strcpy(c->score, pq_getmsgstring(buf));
	PG_RETURN_CHESSGAME_P(c);
}

PG_FUNCTION_INFO_V1(chessgame_send);
Datum chessgame_send(PG_FUNCTION_ARGS) {
	chessgame* c = PG_GETARG_CHESSGAME_P(0);
	StringInfoData buf;
	pq_begintypsend(&buf);
	pq_sendstring(&buf, c->halfmoves);
	pq_sendstring(&buf, c->score);
	PG_FREE_IF_COPY(c, 0);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

PG_FUNCTION_INFO_V1(chessgame_cast_from_text);
Datum chessgame_cast_from_text(PG_FUNCTION_ARGS) {
	text* txt = PG_GETARG_TEXT_P(0);
	char* str = text_to_cstring(txt);
	PG_RETURN_CHESSGAME_P(chessgame_parse(str));
}

PG_FUNCTION_INFO_V1(chessgame_cast_to_text); 
Datum chessgame_cast_to_text(PG_FUNCTION_ARGS){
	chessgame* c = PG_GETARG_CHESSGAME_P(0);
	text* out = cstring_to_text(chessgame_to_str(c));
	PG_FREE_IF_COPY(c, 0);
	PG_RETURN_TEXT_P(out);
}

/*****************************************************************************/

PG_FUNCTION_INFO_V1(chessgame_constructor);
Datum chessgame_constructor(PG_FUNCTION_ARGS) {
	text* t = PG_GETARG_TEXT_P(0);
	char* halfmovesStr = text_to_cstring(t);
	PG_RETURN_CHESSGAME_P(chessgame_parse(halfmovesStr));
}
void replaceChar(char* str, char find, char replace) {
    for (size_t i = 0; i < strlen(str); i++) {
        if (str[i] == find) {
            str[i] = replace;
        }
    }
}

char** extractMoves(const char* chessgame, uint8_t* moveCount) {
    // Replace '.' with spaces
    char* modifiedChessgame = strdup(chessgame);
    replaceChar(modifiedChessgame, '.', ' ');
    size_t currentIndex = 0;
    size_t moveIndex = 0;
    // Count the number of spaces
    size_t spaceCount = 0;
    for (size_t i = 0; i < strlen(modifiedChessgame); i++) {
        if (modifiedChessgame[i] == ' ') {
            spaceCount++;
        }
    }

    // Allocate memory for the result array
    *moveCount = (spaceCount / 3) * 2; // For alternating white and black moves
    char** moves = (char**)malloc(*moveCount * sizeof(char*));

    // Initialize the array
    for (size_t i = 0; i < *moveCount; i++) {
        moves[i] = NULL;
    }

    // Extract moves based on space count
    
    while (currentIndex < strlen(modifiedChessgame) && moveIndex < *moveCount) {
        size_t nextSpace = currentIndex;
        while (modifiedChessgame[nextSpace] != ' ' && modifiedChessgame[nextSpace] != '\0') {
            nextSpace++;
        }

        // Check if the current space is associated with a move
        if (currentIndex % 3 == 1) {
            moves[moveIndex] = strndup(modifiedChessgame + currentIndex, nextSpace - currentIndex);
            moveIndex++;
        }

        // Move to the next token
        currentIndex = nextSpace + 1;
    }

    // Free memory for the modifiedChessgame string
    free(modifiedChessgame);

    // Return the extracted moves array
    return moves;
}

SCL_Board* createGameWithMoves(const char* moves, uint8_t* moveCount) {
    SCL_Board startingBoard;
    char fenString[69];  // Adjust the size if necessary
    strcpy(fenString, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    SCL_boardFromFEN(startingBoard, fenString);

    // Initialize an SCL_Game instance
    SCL_Game game;
    SCL_gameInit(&game, startingBoard);

    // Extract move details from the move string to determine moveCount
    char** movesArray = extractMoves(moves, moveCount);

    // Array to store the resulting boards
    SCL_Board* boardsArray = (SCL_Board*)malloc((*moveCount + 1) * sizeof(SCL_Board));

    if (boardsArray == NULL) {
        // Handle memory allocation failure
        exit(EXIT_FAILURE);
    }
    memcpy(&boardsArray[0], &startingBoard, sizeof(SCL_Board));

    // Process each move
    for (uint8_t i = 0; i < moveCount; ++i) {
        // Extract move details from the move string
        uint8_t from, to;
        char promotion;
        if (!SCL_stringToMove(movesArray[i], &from, &to, &promotion)) {
            // Handle invalid move string
            exit(EXIT_FAILURE);
        }

        // Make the move on the current board
        SCL_gameMakeMove(&game, from, to, promotion);

        // Store the resulting board
        memcpy(&boardsArray[i + 1], &game.board, sizeof(SCL_Board));
    }

    // Free memory for movesArray
    for (uint8_t i = 0; i < moveCount; i++) {
        if (movesArray[i] != NULL) {
            free(movesArray[i]);
        }
    }
    free(movesArray);

    // Return the resulting SCL_Game instance
    return boardsArray;
}
PG_FUNCTION_INFO_V1(extractValue);
Datum extractValue(PG_FUNCTION_ARGS) {
    // Get the chessgame instance
    chessgame* itemValue = PG_GETARG_CHESSGAME_P(0);

    // Extract the chessboard array using your helper function
    size_t moveCount;
    SCL_Board* chessboardArray = createGameWithMoves(itemValue->halfmoves, &moveCount);

    // Set nkeys to moveCount + 1
    int32 nkeys = (int32)(moveCount + 1);

    // Set nullFlags to NULL since no keys will be null
    bool* nullFlags = NULL;

    // Return the chessboardArray
    PG_RETURN_POINTER(chessboardArray);
}
PG_FUNCTION_INFO_V1(extractQuery);
Datum extractQuery(PG_FUNCTION_ARGS) {
    // Convert FEN string to chessboard instance
    chessboard* chessboardKey = chessboard_constructor((text*)PG_GETARG_TEXT_P(0));

    // Convert chessboard instance to SCL_Board
    SCL_Board queryBoard;
    SCL_boardFromFEN(queryBoard, chessboard_to_fen(chessboardKey));

    // Allocate memory for the result array
    SCL_Board* resultArray = (SCL_Board*)palloc(sizeof(SCL_Board));
    memcpy(resultArray, &queryBoard, sizeof(SCL_Board));

    // Set nkeys to 1 since we have a single chessboard instance
    int32 nkeys = 1;

    // Set nullFlags to NULL since the key will not be null
    bool* nullFlags = NULL;

    // Return the resultArray
    PG_RETURN_POINTER(resultArray);
}

PG_FUNCTION_INFO_V1(consistent);
Datum consistent(PG_FUNCTION_ARGS) {
    // Get the indexed chessgame instance
    chessgame* indexedGame = PG_GETARG_CHESSGAME_P(0);

    // Get the extracted query chessboard array
    SCL_Board* queryChessboardArray = (SCL_Board*)PG_GETARG_POINTER(1);

    // Check if the indexed game's move sequence contains the query board state
    bool isConsistent = chessgame_contains_moves(indexedGame->halfmoves, queryChessboardArray);

    // Return true if consistent, false otherwise
    PG_RETURN_BOOL(isConsistent);
}





