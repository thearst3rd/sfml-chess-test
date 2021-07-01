/*
 * SFML Chess board declarations
 * Created by thearst3rd on 08/06/2020
 */

#include <SFML/Graphics.h>

#include "chesslib/piece.h"

typedef struct
{
	sfTexture *wP;
	sfTexture *wN;
	sfTexture *wB;
	sfTexture *wR;
	sfTexture *wQ;
	sfTexture *wK;
	sfTexture *bP;
	sfTexture *bN;
	sfTexture *bB;
	sfTexture *bR;
	sfTexture *bQ;
	sfTexture *bK;
} pieceSet;

int main(int argc, char *argv[]);

void loadPieceSet(pieceSet *ps, const char *name);
void destroyPieceSet(pieceSet *ps);

void calcView();
void drawPiece(piece p, sfVector2f coords);
void drawBoardPiece(piece p, sq s);
void drawCircleShape(sfCircleShape *shape, sq s);

sfTexture *getPieceTex(piece p);

// This sets the values at the pointers to the correct file and rank.
// Returns true if position is inside board, false otherwise
int getMouseSquare(int mouseX, int mouseY, sq *s);

void initChess();
void updateWindowTitle();
void updateGameState();

// Returns the squares that can be reached by a legal move from the given starting square
sqSet getLegalSquareSet(sq s);

// playAiMove spawns a thread to play the move chosen by aiGetMove
move aiGetMove();
void playAiMove(sfTime delay);
void playAiMoveThreadFunc(void *userData);

int isAiPlaying();
void setAiPlaying(int value);
void seedRand(unsigned int seed);
int threadRand(); 	// Uses our own thread safe wichman-hill generator
