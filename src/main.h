/*
 * SFML Chess board declarations
 * Created by thearst3rd on 08/06/2020
 */

#include <SFML/Graphics.h>

#include "chesslib/piece.h"

int main(int argc, char *argv[]);

void calcView();
void drawPiece(piece p, sfVector2f coords);
void drawBoardPiece(piece p, int file, int rank);
void drawCheckIndicator(int file, int rank);

sfTexture *getPieceTex(piece p);

// This sets the values at the pointers to the correct file and rank.
// Returns true if position is inside board, false otherwise
int getMouseSquare(int mouseX, int mouseY, int *file, int *rank);
// This sets the values at the pointer to the correct piece.
// Returns true if new position inside a new piece, false otherwise
int getMouseNewPiece(int mouseX, int mouseY, piece *p);

void initChessBoard();

uint8_t isTerminal();
