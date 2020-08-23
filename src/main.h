/*
 * SFML Chess board declarations
 * Created by thearst3rd on 08/06/2020
 */

#include <SFML/Graphics.h>

#define INITIAL_POSITION "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"

typedef enum
{
	pEmpty,
	pWPawn,
	pWKnight,
	pWBishop,
	pWRook,
	pWQueen,
	pWKing,
	pBPawn,
	pBKnight,
	pBBishop,
	pBRook,
	pBQueen,
	pBKing
} piece;

int main(int argc, char *argv[]);

void calcView();
void drawPiece(piece p, sfVector2f coords);
void drawBoardPiece(piece p, int file, int rank);

sfTexture *getPieceTex(piece p);
piece getPiece(int file, int rank);
void setPiece(int file, int rank, piece p);

// This sets the values at the pointers to the correct file and rank.
// Returns true if position is inside board, false otherwise
int getMouseSquare(int mouseX, int mouseY, int *file, int *rank);
// This sets the values at the pointer to the correct piece.
// Returns true if new position inside a new piece, false otherwise
int getMouseNewPiece(int mouseX, int mouseY, piece *p);

void initChessBoard();
