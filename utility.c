
/*
 * utility.C 
 *
 * PostgreSQL chessgame & chessboard Type:
 *
 * Authors: Zhuang Xianyun, Bouzaher Mohamed, Başaran Ozan, Demonceau Quentin
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**===============================CHESS=====================================**/

static void slice(const char* str, char* result, size_t start, size_t end){
    strncpy(result, str + start, end - start);
}

typedef struct {
	char** list;
	size_t count;
} TList;

static TList allocateStringList(size_t maxNbStrings, size_t maxStringSize){
	//------------------
	// allocate space for the pointers to strings
	char** list = (char**)malloc(maxNbStrings*sizeof(char*));
	//allocate space for each string
	for(size_t i = 0; i < maxNbStrings; i++){
		//printf("%d\n", i);
		list[i] = (char*)malloc(maxStringSize*sizeof(char));
	}
	//------------------
	
	TList tList = { list, 0 };
	return tList;
}

static TList strToTokenList(char* game, TList tokenList){

	// Extract the first token
	char* token = strtok(game, " ");
	//tokenList[0] = token;	
	size_t index = 0;
	size_t counter = 1;
	
	// loop through the string to extract all other tokens
	while( token != NULL ) {
		//printf( "< %s>\n", token ); //printing each token
		token = strtok(NULL, " ");
		if (token != NULL && counter % 3 != 0) {
			tokenList.list[index] = token;
			index += 1;
		}
		counter += 1;
	}
	
	tokenList.count = index;
	
	return tokenList;
}

static char* tokenListToStrWithLimit(const TList* tokenList, char* string, size_t nbTokenLimit){
	char snum[5]; // allows for a integer of 5 digits
	size_t moveNum = 1;
	for(size_t i = 0; i < nbTokenLimit; i++){
		
		if (i%2 == 0){
			sprintf(snum, "%ld", moveNum);
			strcat(string,snum);
			strcat(string,". ");
			moveNum += 1;
		}
		
		strcat(string,tokenList->list[i]);
		strcat(string, " ");
	}
	
	return string;
}

static char* tokenListToStr(const TList* tokenList, char* string){
	return tokenListToStrWithLimit(tokenList, string, tokenList->count);
}
