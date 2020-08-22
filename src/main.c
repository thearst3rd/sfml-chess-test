/*
 * SFML Chess board implementation
 * Created by thearst3rd on 08/06/2020
 */

#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include "main.h"

#define SQUARE_SIZE 45.0f

// Define resources
sfRenderWindow *window;

sfRectangleShape *boardSquares[64];
sfColor boardBlackColor;
sfColor boardWhiteColor;
sfColor boardHighlightColor;

sfRectangleShape *highlightSquare;
int highlight1File;
int highlight1Rank;
int highlight2File;
int highlight2Rank;

sfTexture *texWKing;
sfTexture *texWQueen;
sfTexture *texWRook;
sfTexture *texWKnight;
sfTexture *texWBishop;
sfTexture *texWPawn;

sfTexture *texBKing;
sfTexture *texBQueen;
sfTexture *texBRook;
sfTexture *texBKnight;
sfTexture *texBBishop;
sfTexture *texBPawn;

sfTexture *texPieces[12];

sfSprite *sprPiece;

// Define global variables
int isDragging = 0;
int draggingFile;
int draggingRank;

piece board[8][8] = {pEmpty};


int main(int argc, char *argv[])
{
	// Create the window
	sfVideoMode mode = {720, 720, 32};
	window = sfRenderWindow_create(mode, "SFML Chess Board", sfResize | sfClose, NULL);
	if (!window)
	{
		fprintf(stderr, "ERROR: Unable to create SFML window");
		return 1;
	}

	sfRenderWindow_setVerticalSyncEnabled(window, sfTrue);

	// Load textures
	texPieces[ 0] = texWKing   = sfTexture_createFromFile("img/wK.png", NULL);
	texPieces[ 1] = texWQueen  = sfTexture_createFromFile("img/wQ.png", NULL);
	texPieces[ 2] = texWRook   = sfTexture_createFromFile("img/wR.png", NULL);
	texPieces[ 3] = texWKnight = sfTexture_createFromFile("img/wN.png", NULL);
	texPieces[ 4] = texWBishop = sfTexture_createFromFile("img/wB.png", NULL);
	texPieces[ 5] = texWPawn   = sfTexture_createFromFile("img/wP.png", NULL);

	texPieces[ 6] = texBKing   = sfTexture_createFromFile("img/bK.png", NULL);
	texPieces[ 7] = texBQueen  = sfTexture_createFromFile("img/bQ.png", NULL);
	texPieces[ 8] = texBRook   = sfTexture_createFromFile("img/bR.png", NULL);
	texPieces[ 9] = texBKnight = sfTexture_createFromFile("img/bN.png", NULL);
	texPieces[10] = texBBishop = sfTexture_createFromFile("img/bB.png", NULL);
	texPieces[11] = texBPawn   = sfTexture_createFromFile("img/bP.png", NULL);

	// Set window icon
	sfVector2u textureSize = sfTexture_getSize(texBQueen);
	sfImage *iconImage = sfTexture_copyToImage(texBQueen);
	sfRenderWindow_setIcon(window, textureSize.x, textureSize.y, sfImage_getPixelsPtr(iconImage));

	// Set texture scaling
	for (int i = 0; i < 12; i++)
	{
		sfTexture_setSmooth(texPieces[i], sfTrue);
		sfTexture_generateMipmap(texPieces[i]);
	}

	// Create piece sprite. This will be reused for each piece drawing
	sprPiece = sfSprite_create();
	float size = (float) textureSize.x; 	// Assumes square pieces, all the same size
	sfSprite_setScale(sprPiece, (sfVector2f) {SQUARE_SIZE / size, SQUARE_SIZE / size});
	sfSprite_setOrigin(sprPiece, (sfVector2f) {size / 2.0f, size / 2.0f});

	// Define board colors
	boardBlackColor = sfColor_fromRGB(167, 129, 177);
	boardWhiteColor = sfColor_fromRGB(234, 223, 237);
	boardHighlightColor = sfColor_fromRGBA(255, 255, 0, 100);

	// Create board squares
	for (int i = 0; i < 64; i++)
	{
		sfRectangleShape *s = sfRectangleShape_create();

		sfRectangleShape_setSize(s, (sfVector2f) {SQUARE_SIZE, SQUARE_SIZE});
		sfRectangleShape_setPosition(s, (sfVector2f) {(float) (i % 8) * SQUARE_SIZE, floor(i / 8) * SQUARE_SIZE});

		int isBlack = (i + (int) floor(i / 8)) % 2;
		sfRectangleShape_setFillColor(s, isBlack ? boardBlackColor : boardWhiteColor);

		boardSquares[i] = s;
	}

	// Create highlight square
	highlightSquare = sfRectangleShape_create();

	sfRectangleShape_setSize(highlightSquare, (sfVector2f) {SQUARE_SIZE, SQUARE_SIZE});
	sfRectangleShape_setFillColor(highlightSquare, boardHighlightColor);

	calcView((float) mode.width, (float) mode.height);

	initChessBoard(INITIAL_POSITION);

	// Main loop
	while (sfRenderWindow_isOpen(window))
	{
		// Handle events
		sfEvent event;
		while (sfRenderWindow_pollEvent(window, &event))
		{
			if (event.type == sfEvtClosed)
			{
				sfRenderWindow_close(window);
			}
			else if (event.type == sfEvtResized)
			{
				calcView((float) event.size.width, (float) event.size.height);
			}
			else if (event.type == sfEvtMouseButtonPressed)
			{
				if (event.mouseButton.button == sfMouseLeft)
				{
					int file, rank;
					if (getMouseSquare(event.mouseButton.x, event.mouseButton.y, &file, &rank))
					{
						piece p = getPiece(file, rank);

						if (p)
						{
							isDragging = 1;
							draggingFile = file;
							draggingRank = rank;
						}
					}
				}
			}
			else if (event.type == sfEvtMouseButtonReleased)
			{
				if (event.mouseButton.button == sfMouseLeft)
				{
					if (isDragging)
					{
						isDragging = 0;

						int file, rank;
						if (getMouseSquare(event.mouseButton.x, event.mouseButton.y, &file, &rank))
						{
							if (file != draggingFile || rank != draggingRank)
							{
								setPiece(file, rank, getPiece(draggingFile, draggingRank));
								setPiece(draggingFile, draggingRank, pEmpty);

								highlight1File = draggingFile;
								highlight1Rank = draggingRank;
								highlight2File = file;
								highlight2Rank = rank;
							}
						}
					}
				}
			}
			else if (event.type == sfEvtKeyPressed)
			{
				if (!isDragging)
				{
					// TODO - only respond to certain keys
					initChessBoard(INITIAL_POSITION);
				}
			}
		}

		// Draw to window
		sfRenderWindow_clear(window, sfBlack);

		// Draw the board
		for (int i = 0; i < 64; i++)
		{
			int file = (i % 8) + 1;
			int rank = 8 - floor(i / 8);

			sfRenderWindow_drawRectangleShape(window, boardSquares[i], NULL);

			if ((file == highlight1File && rank == highlight1Rank) || (file == highlight2File && rank == highlight2Rank))
			{
				sfRectangleShape_setPosition(highlightSquare, sfRectangleShape_getPosition(boardSquares[i]));
				sfRenderWindow_drawRectangleShape(window, highlightSquare, NULL);
			}

			piece p = getPiece(file, rank);
			if (p)
				drawPiece(p, file, rank);
		}

		// Draw currently dragged piece
		if (isDragging)
		{
			piece p = getPiece(draggingFile, draggingRank);
			if (p)
			{
				sfTexture *tex = getPieceTex(p);

				sfVector2f coords = sfRenderWindow_mapPixelToCoords(window, sfMouse_getPosition((sfWindow *) window), NULL);

				sfSprite_setTexture(sprPiece, tex, sfFalse);
				sfSprite_setPosition(sprPiece, coords);
				sfRenderWindow_drawSprite(window, sprPiece, NULL);
			}
		}

		sfRenderWindow_display(window);
	}

	// Cleanup and exit
	sfRenderWindow_destroy(window);

	for (int i = 0; i < 64; i++)
		sfRectangleShape_destroy(boardSquares[i]);

	for (int i = 0; i < 12; i++)
		sfTexture_destroy(texPieces[i]);

	sfSprite_destroy(sprPiece);

	return 0;
}

void calcView(float width, float height)
{
	float x = 0.0f;
	float y = 0.0f;
	float w = SQUARE_SIZE * 8.0f;
	float h = SQUARE_SIZE * 8.0f;

	if (height > width)
	{
		y = (h * (height/width) - h);
		h += y;
		y /= -2.0f;
	}
	else
	{
		x = (w * (width/height) - w);
		w += x;
		x /= -2.0f;
	}

	sfView *newView = sfView_createFromRect((sfFloatRect) {x, y, w, h});
	sfRenderWindow_setView(window, newView);
	sfView_destroy(newView);
}

void drawPiece(piece p, int file, int rank)
{
	if (isDragging && draggingFile == file && draggingRank == rank)
		return;

	sfTexture *tex = getPieceTex(p);

	sfSprite_setTexture(sprPiece, tex, sfFalse);
	sfSprite_setPosition(sprPiece, (sfVector2f) {((float) file - 0.5) * SQUARE_SIZE, (8.5 - (float) rank) * SQUARE_SIZE});
	sfRenderWindow_drawSprite(window, sprPiece, NULL);
}

sfTexture *getPieceTex(piece p)
{
	switch (p)
	{
		case pWPawn:
			return texWPawn;
		case pWBishop:
			return texWBishop;
		case pWKnight:
			return texWKnight;
		case pWRook:
			return texWRook;
		case pWQueen:
			return texWQueen;
		case pWKing:
			return texWKing;
		case pBPawn:
			return texBPawn;
		case pBBishop:
			return texBBishop;
		case pBKnight:
			return texBKnight;
		case pBRook:
			return texBRook;
		case pBQueen:
			return texBQueen;
		case pBKing:
			return texBKing;
		default:
			return NULL;
	}
}

piece getPiece(int file, int rank)
{
	return board[file - 1][rank - 1];
}

void setPiece(int file, int rank, piece p)
{
	board[file - 1][rank - 1] = p;
}

// This sets the values at the pointers to the correct file and rank.
// Returns true if position is inside board, false otherwise
int getMouseSquare(int mouseX, int mouseY, int *file, int *rank)
{
	sfVector2f coords = sfRenderWindow_mapPixelToCoords(window, (sfVector2i) {mouseX, mouseY}, NULL);
	coords.x /= SQUARE_SIZE;
	coords.y /= SQUARE_SIZE;

	if (coords.x < 0 || coords.x >= 8 || coords.y < 0 || coords.y >= 8)
		return 0;

	*file = 1 + (int) floor(coords.x);
	*rank = 8 - (int) floor(coords.y);

	return 1;
}

void initChessBoard(char *fen)
{
	highlight1File = 0;
	highlight1Rank = 0;
	highlight2File = 0;
	highlight2Rank = 0;

	// Parse FEN
	int file = 1;
	int rank = 8;

	char c;

	while ((c = *fen))
	{
		if (c == ' ')
		{
			break;
		}
		else if (c >= '1' && c <= '8')
		{
			int num = c - '0';
			if (file + num > 9)
			{
				fprintf(stderr, "ERROR IN FEN: Spacer put file over the end\n");
				return;
			}
			for (int i = 0; i < num; i++)
			{
				setPiece(file, rank, pEmpty);
				file++;
			}
		}
		else if (c == '/')
		{
			if (file != 9)
			{
				fprintf(stderr, "ERROR IN FEN: Found '/' at wrong position in rank\n");
				return;
			}
			if (rank <= 1)
			{
				fprintf(stderr, "ERROR IN FEN: Found '/' after the last rank\n");
				return;
			}
			file = 1;
			rank--;
		}
		else
		{
			if (file > 8)
			{
				fprintf(stderr, "ERROR IN FEN: Found character '%c' after the end of a rank\n", c);
				return;
			}

			char lc = tolower(c);
			int isBlack = islower(c);

			switch (lc)
			{
				case 'p':
					setPiece(file, rank, isBlack ? pBPawn : pWPawn);
					break;

				case 'b':
					setPiece(file, rank, isBlack ? pBBishop : pWBishop);
					break;

				case 'n':
					setPiece(file, rank, isBlack ? pBKnight : pWKnight);
					break;

				case 'r':
					setPiece(file, rank, isBlack ? pBRook : pWRook);
					break;

				case 'q':
					setPiece(file, rank, isBlack ? pBQueen : pWQueen);
					break;

				case 'k':
					setPiece(file, rank, isBlack ? pBKing : pWKing);
					break;

				default:
					fprintf(stderr, "ERROR IN FEN: Unknown character '%c'\n", c);
					return;
			}

			file++;
		}

		fen++;
	}

	if ((rank != 1) || (file != 9))
	{
		fprintf(stderr, "ERROR IN FEN: Ended prematurely\n");
		return;
	}
}
