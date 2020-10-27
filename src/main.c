/*
 * SFML Chess board implementation
 * Created by thearst3rd on 08/06/2020
 */

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <SFML/Audio.h>

#include "chesslib/chessgame.h"

#include "main.h"

#define SQUARE_SIZE 45.0f

// Define resources
sfRenderWindow *window;

sfRectangleShape *boardSquares[64];
sfColor boardBlackColor;
sfColor boardWhiteColor;
sfColor boardHighlightColor;
sfColor backgroundColor;
sfColor checkColor;
sfColor pieceTransparentColor;

sfRectangleShape *highlightSquare;
int highlight1File;
int highlight1Rank;
int highlight2File;
int highlight2Rank;

sfCircleShape *checkIndicator;

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
sfSound *sndTerminal;

// Define global variables
int isDragging = 0;
int draggingFile;
int draggingRank;
piece newDraggingPiece;
int isFlipped = 0;
int isFullscreen = 0;
int showHighlighting = 1;
int playSound = 1;
int doRandomMoves = 0;

const char *initialFen;
chessGame g;


int main(int argc, char *argv[])
{
	// Set random seed
	srand(time(NULL));

	initialFen = INITIAL_FEN;

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
	checkColor = sfColor_fromRGBA(255, 0, 0, 100);
	pieceTransparentColor = sfColor_fromRGBA(255, 255, 255, 70);

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

	// Create check indicator
	checkIndicator = sfCircleShape_create();

	sfCircleShape_setRadius(checkIndicator, SQUARE_SIZE / 2.1f);
	sfCircleShape_setOrigin(checkIndicator, (sfVector2f) {SQUARE_SIZE / 2.1f, SQUARE_SIZE / 2.1f});
	sfCircleShape_setFillColor(checkIndicator, checkColor);

	calcView();

	chessGameInit(&g); 	// Stupid... but it will populate the fields so initChessGame can free them...
	initChessGame();

	// Create sounds
	sfSoundBuffer *sbMove = sfSoundBuffer_createFromFile("snd/move.wav");
	sfSoundBuffer *sbCapture = sfSoundBuffer_createFromFile("snd/capture.wav");
	sfSoundBuffer *sbCheck = sfSoundBuffer_createFromFile("snd/check.wav");
	sfSoundBuffer *sbTerminal = sfSoundBuffer_createFromFile("snd/terminal.wav");

	sndMove = sfSound_create();
	sfSound_setBuffer(sndMove, sbMove);
	sndCapture = sfSound_create();
	sfSound_setBuffer(sndCapture, sbCapture);
	sndCheck = sfSound_create();
	sfSound_setBuffer(sndCheck, sbCheck);
	sndTerminal = sfSound_create();
	sfSound_setBuffer(sndTerminal, sbTerminal);


	// Main loop
	while (sfRenderWindow_isOpen(window))
	{
		board b = chessGameGetCurrentBoard(&g);

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
						if (g.terminal == tsOngoing)
						{
							p = boardGetPiece(&b, sqI(file, rank));

							if (p && (pieceGetColor(p) == b.currentPlayer))
							{
								isDragging = 1;
								draggingFile = file;
								draggingRank = rank;
								newDraggingPiece = pEmpty;
							}
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
							// Just in case...
							b = chessGameGetCurrentBoard(&g);

							sq from = sqI(draggingFile, draggingRank);
							sq to = sqI(file, rank);
							move m = moveSq(from, to);
							if (pieceGetType(boardGetPiece(&b, from)) == ptPawn && (rank == 1 || rank == 8))
								m.promotion = ptQueen;

							uint8_t isCapture = (boardGetPiece(&b, to) != pEmpty) ||
									(pieceGetType(boardGetPiece(&b, from)) == ptPawn && (file != draggingFile));

							if (!chessGamePlayMove(&g, m))
							{
								b = chessGameGetCurrentBoard(&g);

								highlight1File = draggingFile;
								highlight1Rank = draggingRank;
								highlight2File = file;
								highlight2Rank = rank;

								uint8_t isCheck = boardIsInCheck(&b) && (g.terminal == tsOngoing);
								if (playSound && (g.terminal != tsOngoing))
								{
									sfSound_play(sndTerminal);
								}

								if (doRandomMoves && (g.terminal == tsOngoing))
								{
									moveList *list = g.currentLegalMoves;
									uint8_t index = rand() % list->size;

									moveListNode *n = list->head;
									for (int i = 0; i < index; i++)
									{
										n = n->next;
									}

									move randomM = n->move;

									isCapture |= boardGetPiece(&b, randomM.to) ||
											(pieceGetType(boardGetPiece(&b, randomM.from)) == ptPawn && randomM.to.file != randomM.from.file);

									chessGamePlayMove(&g, randomM);
									b = chessGameGetCurrentBoard(&g);

									highlight1File = randomM.from.file;
									highlight1Rank = randomM.from.rank;
									highlight2File = randomM.to.file;
									highlight2Rank = randomM.to.rank;

									isCheck |= boardIsInCheck(&b);

									if (g.terminal != tsOngoing)
									{
										isCheck = 0;

										if (playSound)
											sfSound_play(sndTerminal);
									}
								}

								if (playSound)
								{
									if (isCapture)
										sfSound_play(sndCapture);
									else
										sfSound_play(sndMove);

									if (isCheck)
										sfSound_play(sndCheck);
								}

								char *fen = boardGetFen(&b);
								sfRenderWindow_setTitle(window, fen);
								free(fen);
							}
						}
					}
				}
			}
			else if (event.type == sfEvtKeyPressed)
			{
				moveList *list;
				switch (event.key.code)
				{
					case sfKeyR:
						if (isDragging)
							break;
						initChessGame();
						break;

					case sfKeyF:
						isFlipped = !isFlipped;
						break;

					case sfKeyH:
						showHighlighting = !showHighlighting;
						break;

					case sfKeyS:
						playSound = !playSound;
						break;

					case sfKeyM:
						doRandomMoves = !doRandomMoves;
						break;

					case sfKeySpace:
						if (isDragging)
							break;
						if (g.terminal == tsOngoing)
						{
							list = g.currentLegalMoves;
							uint8_t index = rand() % list->size;

							moveListNode *n = list->head;
							for (int i = 0; i < index; i++)
							{
								n = n->next;
							}

							move randomM = n->move;

							uint8_t isCapture = boardGetPiece(&b, randomM.to) ||
									(pieceGetType(boardGetPiece(&b, randomM.from)) == ptPawn && randomM.to.file != randomM.from.file);

							chessGamePlayMove(&g, randomM);
							b = chessGameGetCurrentBoard(&g);

							highlight1File = randomM.from.file;
							highlight1Rank = randomM.from.rank;
							highlight2File = randomM.to.file;
							highlight2Rank = randomM.to.rank;

							char *fen = boardGetFen(&b);
							sfRenderWindow_setTitle(window, fen);
							free(fen);

							if (playSound)
							{
								if (isCapture)
									sfSound_play(sndCapture);
								else
									sfSound_play(sndMove);

								if (g.terminal != tsOngoing)
									sfSound_play(sndTerminal);
								else if (boardIsInCheck(&b))
									sfSound_play(sndCheck);
							}
						}
						break;

					case sfKeyEnter:
						if (event.key.alt)
						{
							isFullscreen = !isFullscreen;
							sfRenderWindow_destroy(window);

							sfVideoMode videoMode = isFullscreen ? sfVideoMode_getDesktopMode() : mode;
							sfWindowStyle style = isFullscreen ? sfFullscreen : sfDefaultStyle;

							char *fen = boardGetFen(&b);

							window = sfRenderWindow_create(videoMode, fen, style, NULL);

							free(fen);

							sfRenderWindow_setIcon(window, texSize.x, texSize.y, sfImage_getPixelsPtr(iconImage));
							sfRenderWindow_setVerticalSyncEnabled(window, sfTrue);

							calcView();
						}
						break;

					case sfKeyG:
						if (isDragging)
							break;
						initChessGame();
						while (g.terminal == tsOngoing)
						{
							moveList *list = g.currentLegalMoves;

							int index = rand() % list->size;
							moveListNode *n = list->head;
							for (int i = 0; i < index; i++)
								n = n->next;
							move m = n->move;
							chessGamePlayMove(&g, m);

							highlight1File = m.from.file;
							highlight1Rank = m.from.rank;
							highlight2File = m.to.file;
							highlight2Rank = m.to.rank;
						}
						b = chessGameGetCurrentBoard(&g);
						char *fen = boardGetFen(&b);
						sfRenderWindow_setTitle(window, fen);
						free(fen);
						break;

					case sfKeyZ:
						if (isDragging)
							break;
						chessGameUndo(&g);

						moveListNode *lastMove = g.moveHistory->tail;
						if (lastMove)
						{
							highlight1File = lastMove->move.from.file;
							highlight1Rank = lastMove->move.from.rank;
							highlight2File = lastMove->move.to.file;
							highlight2Rank = lastMove->move.to.rank;
						}
						else
						{
							highlight1File = -1;
							highlight1Rank = -1;
							highlight2File = -1;
							highlight2Rank = -1;
						}

						b = chessGameGetCurrentBoard(&g);
						fen = boardGetFen(&b);
						sfRenderWindow_setTitle(window, fen);
						free(fen);
						break;

					case sfKeyC:
						if (isDragging)
							break;
						if (g.terminal == tsOngoing)
						{
							if (g.repetitions >= 3)
							{
								g.terminal = tsDrawClaimedThreefold;
								if (playSound)
									sfSound_play(sndTerminal);
							}
							else if (b.halfMoveClock >= 100)
							{
								g.terminal = tsDrawClaimed50MoveRule;
								if (playSound)
									sfSound_play(sndTerminal);
							}
						}
						break;

					default:
						break;
				}
			}
		}

		// Draw to window
		sfRenderWindow_clear(window, backgroundColor);

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

			piece p = boardGetPiece(&b, sqI(file, rank));
			if (p)
			{
				pieceType pt = pieceGetType(p);

				if (pt == ptKing && boardIsSquareAttacked(&b, sqI(file, rank), b.currentPlayer == pcWhite ? pcBlack : pcWhite))
					drawCheckIndicator(file, rank);
				drawBoardPiece(p, file, rank);
			}
		}

		// Draw currently dragged piece
		if (isDragging)
		{
			piece p;
			if (newDraggingPiece)
				p = newDraggingPiece;
			else
				p = boardGetPiece(&b, sqI(draggingFile, draggingRank));

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
	sfCircleShape_destroy(checkIndicator);

	sfSprite_destroy(sprPiece);

	sfSoundBuffer_destroy(sbMove);
	sfSoundBuffer_destroy(sbCapture);
	sfSoundBuffer_destroy(sbCheck);
	sfSoundBuffer_destroy(sbTerminal);

	sfSound_destroy(sndMove);
	sfSound_destroy(sndCapture);
	sfSound_destroy(sndCheck);
	sfSound_destroy(sndTerminal);

	return 0;
}

void calcView()
{
	sfVector2u windowSize = sfRenderWindow_getSize(window);
	float width = (float) windowSize.x;
	float height = (float) windowSize.y;

	float x = 0.0f;
	float y = 0.0f;
	float w = SQUARE_SIZE * 8.0f;
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

void drawCheckIndicator(int file, int rank)
{
	if (isFlipped)
	{
		file = 9 - file;
		rank = 9 - rank;
	}

	sfCircleShape_setPosition(checkIndicator, (sfVector2f) {((float) file - 0.5) * SQUARE_SIZE, (8.5 - (float) rank) * SQUARE_SIZE});
	sfRenderWindow_drawCircleShape(window, checkIndicator, NULL);
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

void initChessGame()
{
	highlight1File = 0;
	highlight1Rank = 0;
	highlight2File = 0;
	highlight2Rank = 0;

	chessGameFreeComponents(&g);
	chessGameInitFromFen(&g, initialFen);

	board b = chessGameGetCurrentBoard(&g);
	char *newFen = boardGetFen(&b);
	sfRenderWindow_setTitle(window, newFen);
	free(newFen);
}
