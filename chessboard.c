
/*
 * chessboard.C 
 *
 * PostgreSQL chessboard Type:
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

#include "smallchesslib.h"

/**===============================CHESSBOARD================================**/

/*Using the definition of FEN it is relatively straightforward to compute an upper bound :

piece placement : 64 (pieces / squares) + 7 (slashes)
color : 1 (w / b)
castling : 4 (KQkq)
en passant : 2 (e3)
halfmove clock : 3 (100)
fullmove number : 4 (considering that maximum game length < 9999 due to the 50 - move rule)
spaces between fields : 5
So in total we get as an upper bound : 64 + 7 + 1 + 4 + 2 + 3 + 4 + 5 = 90
*/
typedef struct {
	
	char piecePlacement[71+1];
	char activeColor;
	char castling[4+1];
	char enPassant[2+1];
	int halfmoveClock;
	int fullmoveNumber;
	
} chessboard;


#define DatumGetChessBoardP(X) ((chessboard *) DatumGetPointer(X))
#define ChessBoardPGetDatum(X)  PointerGetDatum(X)
#define PG_GETARG_CHESSBOARD_P(n) DatumGetChessBoardP(PG_GETARG_DATUM(n))
#define PG_RETURN_CHESSBOARD_P(x) return ChessBoardPGetDatum(x)


static chessboard* chessboard_make(char piecePlacement[71+1], char activeColor, char castling[4+1], char enPassant[2+1], int halfmoveClock, int fullmoveNumber) {
	chessboard* c = palloc0(sizeof(chessboard));
	strcpy(c->piecePlacement,piecePlacement);
	c->activeColor = activeColor;
	strcpy(c->castling,castling);
	strcpy(c->enPassant,enPassant);
	c->halfmoveClock = halfmoveClock;
	c->fullmoveNumber = fullmoveNumber;
	return c;
}

//static chessboard* getStart() {
//	return chessboard_parse(SCL_FEN_START);
//}

static char** fen_parse(char* fen){
	
	int nbTokens = 6;
	int maxTokeSize = 100;
	char* token;
	int index;
	
	//------------------
	// allocate space for 6 pointers to strings
    char **tokenList = (char**)malloc(nbTokens*sizeof(char*));
    int i = 0;
    //allocate space for each string
    for(i = 0; i < nbTokens; i++){
        //printf("%d\n", i);
        tokenList[i] = (char*)malloc(maxTokeSize*sizeof(char));
    }
    //------------------

	// Extract the first token
	token = strtok(fen, " ");
	tokenList[0] = token;	
	index = 1;
	
	// loop through the string to extract all other tokens
	while( token != NULL ) {
		//printf( "< %s>\n", token ); //printing each token
		token = strtok(NULL, " ");
		if (token != NULL) {
			tokenList[index] = token;
			index += 1;
		}
	}
	if (index-1 != 5) {
		//int length;
		printf("Error: Wrong FEN statement for chessboard creation: '");
		//length = sizeof(*fen) / sizeof(fen[0]);
		//for (int loop = 0; loop < length; loop++) { printf("%s ", fen[loop]); }
		//printf("'");
		exit(-1);
	}

	return tokenList;
}

static chessboard* chessboard_parse(char* fen){
	char** tokenList = fen_parse(fen);
	return chessboard_make(tokenList[0], tokenList[1][0], tokenList[2], tokenList[3], atoi(tokenList[4]), atoi(tokenList[5]));
}

static char* chessboard_to_str(const chessboard* board){
	char* result = psprintf("(%s, %c, %s,%s, %d, %d)",
		board->piecePlacement,
		board->activeColor,
		board->castling,
		board->enPassant,
		board->halfmoveClock,
		board->fullmoveNumber);
	return result;
}

/**=========================================================================**/
/*****************************************************************************/

PG_FUNCTION_INFO_V1(chessboard_in);
Datum chessboard_in(PG_FUNCTION_ARGS){
	char* str = PG_GETARG_CSTRING(0);
	PG_RETURN_CHESSBOARD_P(chessboard_parse(str));
}

PG_FUNCTION_INFO_V1(chessboard_out);
Datum chessboard_out(PG_FUNCTION_ARGS){
	chessboard* c = PG_GETARG_CHESSBOARD_P(0);
	char* result = chessboard_to_str(c);
	PG_FREE_IF_COPY(c, 0);
	PG_RETURN_CSTRING(result);
}

static unsigned int pq_getmsgint8(StringInfo msg){return pq_getmsgint(msg,1);}

PG_FUNCTION_INFO_V1(chessboard_recv);
Datum chessboard_recv(PG_FUNCTION_ARGS){
	StringInfo  buf = (StringInfo)PG_GETARG_POINTER(0);
	chessboard* c = (chessboard*)palloc(sizeof(chessboard));
	
	strcpy(c->piecePlacement,pq_getmsgstring(buf));
	c->activeColor = *pq_getmsgstring(buf);
	strcpy(c->castling,pq_getmsgstring(buf));
	strcpy(c->enPassant,pq_getmsgstring(buf));
	c->halfmoveClock = pq_getmsgint8(buf);
	c->fullmoveNumber = pq_getmsgint8(buf);
	PG_RETURN_CHESSBOARD_P(c);
}

PG_FUNCTION_INFO_V1(chessboard_send); 
Datum chessboard_send(PG_FUNCTION_ARGS){
	chessboard* c = PG_GETARG_CHESSBOARD_P(0);
	StringInfoData buf;
	pq_begintypsend(&buf);
	
	pq_sendstring(&buf,c->piecePlacement);
	pq_sendbyte(&buf, c->activeColor);
	pq_sendstring(&buf,c->castling);
	pq_sendstring(&buf,c->enPassant);
	pq_sendint8(&buf, c->halfmoveClock);
	pq_sendint8(&buf, c->fullmoveNumber);
	
	PG_FREE_IF_COPY(c, 0);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

PG_FUNCTION_INFO_V1(chessboard_cast_from_text);
Datum chessboard_cast_from_text(PG_FUNCTION_ARGS){
	text* txt = PG_GETARG_TEXT_P(0);
	char* str = DatumGetCString(DirectFunctionCall1(textout, PointerGetDatum(txt)));
	PG_RETURN_CHESSBOARD_P(chessboard_parse(str));
}

PG_FUNCTION_INFO_V1(chessboard_cast_to_text); 
Datum chessboard_cast_to_text(PG_FUNCTION_ARGS){
	chessboard* c = PG_GETARG_CHESSBOARD_P(0);
	text* out = (text*)DirectFunctionCall1(textin, PointerGetDatum(chessboard_to_str(c)));
	PG_FREE_IF_COPY(c, 0);
	PG_RETURN_TEXT_P(out);
}

/*****************************************************************************/

PG_FUNCTION_INFO_V1(chessboard_constructor);
Datum chessboard_constructor(PG_FUNCTION_ARGS) {
	
	//char* combinedArgs = PG_GETARG_VARCHAR_P(0);
	text* t = PG_GETARG_TEXT_P(0);
	char* combinedArgs = text_to_cstring(t);
	char** tokenList = fen_parse(combinedArgs);

	char* piecePlacement = tokenList[0]; //maybe use VarChar*
	char activeColor = tokenList[1][0];
	char* castling = tokenList[2]; //maybe use VarChar*
	char* enPassant = tokenList[3]; //maybe use VarChar*
	int halfmoveClock = atoi(tokenList[4]);
	int fullmoveNumber = atoi(tokenList[5]);
	
	PG_RETURN_CHESSBOARD_P(chessboard_make(piecePlacement,activeColor,castling,enPassant,halfmoveClock,fullmoveNumber));
}

/*****************************************************************************/
