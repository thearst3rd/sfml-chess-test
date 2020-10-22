/*
 * SFML Chess board implementation
 * Created by thearst3rd on 08/06/2020
 */

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include <SFML/Audio.h>

#include "main.h"

#define SQUARE_SIZE 45.0f

// Define resources
sfRenderWindow *window;

sfRectangleShape *editingRectangle;

sfRectangleShape *boardSquares[64];
sfColor boardBlackColor;
sfColor boardWhiteColor;
sfColor boardHighlightColor;
sfColor backgroundColor;
sfColor editingColor;
sfColor checkColor;
sfColor pieceTransparentColor;

sfRectangleShape *highlightSquare;
int highlight1File;
int highlight1Rank;
int highlight2File;
int highlight2Rank;

sfTexture *texWKing;
sfTexture *texWQueen;
sfTexture *texWRook;
sfTexture *texWBishop;
sfTexture *texWKnight;
sfTexture *texWPawn;

sfTexture *texBKing;
sfTexture *texBQueen;
sfTexture *texBRook;
sfTexture *texBBishop;
sfTexture *texBKnight;
sfTexture *texBPawn;

sfTexture *texPieces[12];

sfSprite *sprPiece;

sfSound *sndMove;
sfSound *sndCapture;
sfSound *sndCheck;
sfSound *sndCheckmate;

// Define global variables
int isDragging = 0;
int draggingFile;
int draggingRank;
piece newDraggingPiece;
int isFlipped = 0;
int isEditing = 0;
int isFullscreen = 0;
int showHighlighting = 1;
int playSound = 1;

piece board[8][8] = {pEmpty};


int main(int argc, char *argv[])
{
	char *initialFen = INITIAL_POSITION;

	// Parse command line input
	for (int i = 1; i < argc; i++)
	{
		if ((strcmp(argv[i], "--fen") == 0) || (strcmp(argv[i], "-f") == 0))
		{
			i++;
			if (i >= argc)
			{
				fprintf(stderr, "ERROR: You must supply FEN after the %s argument", argv[i - 1]);
				return 1;
			}
			initialFen = argv[i];
		}
	}

	// Create the window
	sfVideoMode mode = {720, 720, 32};
	window = sfRenderWindow_create(mode, "SFML Chess Board", sfDefaultStyle, NULL);
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
	texPieces[ 3] = texWBishop = sfTexture_createFromFile("img/wB.png", NULL);
	texPieces[ 4] = texWKnight = sfTexture_createFromFile("img/wN.png", NULL);
	texPieces[ 5] = texWPawn   = sfTexture_createFromFile("img/wP.png", NULL);

	texPieces[ 6] = texBKing   = sfTexture_createFromFile("img/bK.png", NULL);
	texPieces[ 7] = texBQueen  = sfTexture_createFromFile("img/bQ.png", NULL);
	texPieces[ 8] = texBRook   = sfTexture_createFromFile("img/bR.png", NULL);
	texPieces[ 9] = texBBishop = sfTexture_createFromFile("img/bB.png", NULL);
	texPieces[10] = texBKnight = sfTexture_createFromFile("img/bN.png", NULL);
	texPieces[11] = texBPawn   = sfTexture_createFromFile("img/bP.png", NULL);

	// Set window icon
	sfVector2u texSize = sfTexture_getSize(texBQueen);
	sfImage *iconImage = sfTexture_copyToImage(texBQueen);
	sfRenderWindow_setIcon(window, texSize.x, texSize.y, sfImage_getPixelsPtr(iconImage));

	// Set texture scaling
	for (int i = 0; i < 12; i++)
	{
		sfTexture_setSmooth(texPieces[i], sfTrue);
		sfTexture_generateMipmap(texPieces[i]);
	}

	// Create piece sprite. This will be reused for each piece drawing
	sprPiece = sfSprite_create();
	float size = (float) texSize.x; 	// Assumes square pieces, all the same size
	sfSprite_setScale(sprPiece, (sfVector2f) {SQUARE_SIZE / size, SQUARE_SIZE / size});
	sfSprite_setOrigin(sprPiece, (sfVector2f) {size / 2.0f, size / 2.0f});

	// Define board colors
	boardBlackColor = sfColor_fromRGB(167, 129, 177);
	boardWhiteColor = sfColor_fromRGB(234, 223, 237);
	boardHighlightColor = sfColor_fromRGBA(255, 255, 0, 100);
	backgroundColor = sfColor_fromRGB(25, 25, 25);
	editingColor = sfColor_fromRGB(75, 75, 75);
	checkColor = sfColor_fromRGBA(255, 0, 0, 100);
	pieceTransparentColor = sfColor_fromRGBA(255, 255, 255, 70);

	// Create editing rectangle
	editingRectangle = sfRectangleShape_create();
	sfRectangleShape_setFillColor(editingRectangle, editingColor);
	sfRectangleShape_setPosition(editingRectangle, (sfVector2f) {-1.5f * SQUARE_SIZE, 0.5f * SQUARE_SIZE});
	sfRectangleShape_setSize(editingRectangle, (sfVector2f) {11.0f * SQUARE_SIZE, 7.0f * SQUARE_SIZE});

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

	calcView();

	initChessBoard(initialFen);

	// Create sounds
	sfSoundBuffer *sbMove = sfSoundBuffer_createFromFile("snd/move.ogg");
	sfSoundBuffer *sbCapture = sfSoundBuffer_createFromFile("snd/capture.ogg");
	sfSoundBuffer *sbCheck = sfSoundBuffer_createFromFile("snd/check.ogg");
	sfSoundBuffer *sbCheckmate = sfSoundBuffer_createFromFile("snd/checkmate.ogg");

	sndMove = sfSound_create();
	sfSound_setBuffer(sndMove, sbMove);
	sndCapture = sfSound_create();
	sfSound_setBuffer(sndCapture, sbCapture);
	sndCheck = sfSound_create();
	sfSound_setBuffer(sndCheck, sbCheck);
	sndCheckmate = sfSound_create();
	sfSound_setBuffer(sndCheckmate, sbCheckmate);


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
				calcView();
			}
			else if (event.type == sfEvtMouseButtonPressed)
			{
				if (event.mouseButton.button == sfMouseLeft)
				{
					int file, rank;
					piece p;
					if (getMouseSquare(event.mouseButton.x, event.mouseButton.y, &file, &rank))
					{
						p = getPiece(file, rank);

						if (p)
						{
							isDragging = 1;
							draggingFile = file;
							draggingRank = rank;
							newDraggingPiece = pEmpty;
						}
					}
					else if (getMouseNewPiece(event.mouseButton.x, event.mouseButton.y, &p))
					{
						isDragging = 1;
						draggingFile = 0;
						draggingRank = 0;
						newDraggingPiece = p;
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

						if (newDraggingPiece)
						{
							int file, rank;
							if (getMouseSquare(event.mouseButton.x, event.mouseButton.y, &file, &rank))
							{
								setPiece(file, rank, newDraggingPiece);

								highlight1File = 0;
								highlight1Rank = 0;
								highlight2File = 0;
								highlight2Rank = 0;
							}
						}
						else
						{
							int file, rank;
							if (getMouseSquare(event.mouseButton.x, event.mouseButton.y, &file, &rank))
							{
								if (file != draggingFile || rank != draggingRank)
								{
									if (playSound && !isEditing)
									{
										if (getPiece(file, rank))
											sfSound_play(sndCapture);
										else
											sfSound_play(sndMove);
									}

									setPiece(file, rank, getPiece(draggingFile, draggingRank));
									setPiece(draggingFile, draggingRank, pEmpty);

									if (isEditing)
									{
										highlight1File = 0;
										highlight1Rank = 0;
										highlight2File = 0;
										highlight2Rank = 0;
									}
									else
									{
										highlight1File = draggingFile;
										highlight1Rank = draggingRank;
										highlight2File = file;
										highlight2Rank = rank;
									}
								}
							}
							else if (isEditing)
							{
								setPiece(draggingFile, draggingRank, pEmpty);

								highlight1File = 0;
								highlight1Rank = 0;
								highlight2File = 0;
								highlight2Rank = 0;
							}
						}
					}
				}
			}
			else if (event.type == sfEvtKeyPressed)
			{
				switch (event.key.code)
				{
					case sfKeyR:
						if (!isDragging)
							initChessBoard(initialFen);
						break;

					case sfKeyF:
						isFlipped = !isFlipped;
						break;

					case sfKeyE:
						isEditing = !isEditing;
						calcView();
						break;

					case sfKeyH:
						showHighlighting = !showHighlighting;
						break;

					case sfKeyS:
						playSound = !playSound;
						break;

					case sfKeyEnter:
						if (event.key.alt)
						{
							isFullscreen = !isFullscreen;
							sfRenderWindow_destroy(window);

							sfVideoMode videoMode = isFullscreen ? sfVideoMode_getDesktopMode() : mode;
							sfWindowStyle style = isFullscreen ? sfFullscreen : sfDefaultStyle;
							window = sfRenderWindow_create(videoMode, "SFML Chess Board", style, NULL);

							sfRenderWindow_setIcon(window, texSize.x, texSize.y, sfImage_getPixelsPtr(iconImage));
							sfRenderWindow_setVerticalSyncEnabled(window, sfTrue);

							calcView();
						}
						break;

					default:
						break;
				}
			}
		}

		// Draw to window
		sfRenderWindow_clear(window, backgroundColor);

		if (isEditing)
		{
			sfRenderWindow_drawRectangleShape(window, editingRectangle, NULL);

			drawPiece(isFlipped ? pWKing : pBKing, (sfVector2f) {-0.75f * SQUARE_SIZE, 1.5f * SQUARE_SIZE});
			drawPiece(isFlipped ? pWQueen : pBQueen, (sfVector2f) {-0.75f * SQUARE_SIZE, 2.5f * SQUARE_SIZE});
			drawPiece(isFlipped ? pWRook : pBRook, (sfVector2f) {-0.75f * SQUARE_SIZE, 3.5f * SQUARE_SIZE});
			drawPiece(isFlipped ? pWBishop : pBBishop, (sfVector2f) {-0.75f * SQUARE_SIZE, 4.5f * SQUARE_SIZE});
			drawPiece(isFlipped ? pWKnight : pBKnight, (sfVector2f) {-0.75f * SQUARE_SIZE, 5.5f * SQUARE_SIZE});
			drawPiece(isFlipped ? pWPawn : pBPawn, (sfVector2f) {-0.75f * SQUARE_SIZE, 6.5f * SQUARE_SIZE});

			drawPiece(isFlipped ? pBKing : pWKing, (sfVector2f) {8.75f * SQUARE_SIZE, 1.5f * SQUARE_SIZE});
			drawPiece(isFlipped ? pBQueen : pWQueen, (sfVector2f) {8.75f * SQUARE_SIZE, 2.5f * SQUARE_SIZE});
			drawPiece(isFlipped ? pBRook : pWRook, (sfVector2f) {8.75f * SQUARE_SIZE, 3.5f * SQUARE_SIZE});
			drawPiece(isFlipped ? pBBishop : pWBishop, (sfVector2f) {8.75f * SQUARE_SIZE, 4.5f * SQUARE_SIZE});
			drawPiece(isFlipped ? pBKnight : pWKnight, (sfVector2f) {8.75f * SQUARE_SIZE, 5.5f * SQUARE_SIZE});
			drawPiece(isFlipped ? pBPawn : pWPawn, (sfVector2f) {8.75f * SQUARE_SIZE, 6.5f * SQUARE_SIZE});
		}

		// Draw the board
		for (int i = 0; i < 64; i++)
		{
			int file = (i % 8) + 1;
			int rank = 8 - floor(i / 8);

			int index = isFlipped ? 63 - i : i;

			sfRenderWindow_drawRectangleShape(window, boardSquares[index], NULL);

			if (showHighlighting &&
					((file == highlight1File && rank == highlight1Rank) ||
					(file == highlight2File && rank == highlight2Rank)))
			{
				sfRectangleShape_setPosition(highlightSquare, sfRectangleShape_getPosition(boardSquares[index]));
				sfRenderWindow_drawRectangleShape(window, highlightSquare, NULL);
			}

			piece p = getPiece(file, rank);
			if (p)
				drawBoardPiece(p, file, rank);
		}

		// Draw currently dragged piece
		if (isDragging)
		{
			piece p;
			if (newDraggingPiece)
				p = newDraggingPiece;
			else
				p = getPiece(draggingFile, draggingRank);

			if (p)
			{
				sfVector2f coords = sfRenderWindow_mapPixelToCoords(window, sfMouse_getPosition((sfWindow *) window), NULL);

				drawPiece(p, coords);
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

	sfRectangleShape_destroy(highlightSquare);

	sfSprite_destroy(sprPiece);

	sfSoundBuffer_destroy(sbMove);
	sfSoundBuffer_destroy(sbCapture);
	sfSoundBuffer_destroy(sbCheck);
	sfSoundBuffer_destroy(sbCheckmate);

	sfSound_destroy(sndMove);
	sfSound_destroy(sndCapture);
	sfSound_destroy(sndCheck);
	sfSound_destroy(sndCheckmate);

	return 0;
}

void calcView()
{
	sfVector2u windowSize = sfRenderWindow_getSize(window);
	float width = (float) windowSize.x;
	float height = (float) windowSize.y;

	float x = (isEditing ? -1.5f * SQUARE_SIZE : 0.0f);
	float y = 0.0f;
	float w = SQUARE_SIZE * (isEditing ? 11.0f : 8.0f);
	float h = SQUARE_SIZE * 8.0f;

	float ratio = (w / h);
	float windowRatio = (width / height);

	if (windowRatio < ratio)
	{
		float yDiff = w * ((1.0f / windowRatio) - (1.0f / ratio));
		h += yDiff;
		y -= yDiff / 2.0f;
	}
	else
	{
		float xDiff = h * (windowRatio - ratio);
		w += xDiff;
		x -= xDiff / 2.0f;
	}

	sfView *newView = sfView_createFromRect((sfFloatRect) {x, y, w, h});
	sfRenderWindow_setView(window, newView);
	sfView_destroy(newView);
}

void drawPiece(piece p, sfVector2f coords)
{
	sfTexture *tex = getPieceTex(p);

	sfSprite_setTexture(sprPiece, tex, sfFalse);
	sfSprite_setPosition(sprPiece, coords);
	sfRenderWindow_drawSprite(window, sprPiece, NULL);
}

void drawBoardPiece(piece p, int file, int rank)
{
	int coordsFile, coordsRank;

	if (isFlipped)
	{
		coordsFile = 9 - file;
		coordsRank = 9 - rank;
	}
	else
	{
		coordsFile = file;
		coordsRank = rank;
	}

	sfVector2f coords = (sfVector2f) {((float) coordsFile - 0.5) * SQUARE_SIZE,
			(8.5 - (float) coordsRank) * SQUARE_SIZE};

	if (isDragging && draggingFile == file && draggingRank == rank)
	{
		sfSprite_setColor(sprPiece, pieceTransparentColor);
		drawPiece(p, coords);
		sfSprite_setColor(sprPiece, sfWhite);
	}
	else
	{
		drawPiece(p, coords);
	}
}

sfTexture *getPieceTex(piece p)
{
	switch (p)
	{
		case pWPawn:
			return texWPawn;
		case pWKnight:
			return texWKnight;
		case pWBishop:
			return texWBishop;
		case pWRook:
			return texWRook;
		case pWQueen:
			return texWQueen;
		case pWKing:
			return texWKing;
		case pBPawn:
			return texBPawn;
		case pBKnight:
			return texBKnight;
		case pBBishop:
			return texBBishop;
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

	if (coords.x < 0.0f || coords.x >= 8.0f || coords.y < 0.0f || coords.y >= 8.0f)
		return 0;

	*file = 1 + (int) floor(coords.x);
	*rank = 8 - (int) floor(coords.y);

	if (isFlipped)
	{
		*file = 9 - *file;
		*rank = 9 - *rank;
	}

	return 1;
}

int getMouseNewPiece(int mouseX, int mouseY, piece *p)
{
	if (!isEditing)
		return 0;

	sfVector2f coords = sfRenderWindow_mapPixelToCoords(window, (sfVector2i) {mouseX, mouseY}, NULL);
	coords.x /= SQUARE_SIZE;
	coords.y /= SQUARE_SIZE;

	if ((coords.y >= 1.0f && coords.y < 7.0f) &&
			((coords.x >= -1.25f && coords.x < -0.25f) ||
			(coords.x >= 8.25f && coords.x < 9.25f)))
	{
		if (coords.y < 2.0f)
			*p = pBKing;
		else if (coords.y < 3.0f)
			*p = pBQueen;
		else if (coords.y < 4.0f)
			*p = pBRook;
		else if (coords.y < 5.0f)
			*p = pBBishop;
		else if (coords.y < 6.0f)
			*p = pBKnight;
		else //if (coords.y < 7.0f)
			*p = pBPawn;

		// Should we flip to white
		if ((coords.x > 0.0f) ^ (isFlipped))
			*p -= 6;

		return 1;
	}

	return 0;
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

				case 'n':
					setPiece(file, rank, isBlack ? pBKnight : pWKnight);
					break;

				case 'b':
					setPiece(file, rank, isBlack ? pBBishop : pWBishop);
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
