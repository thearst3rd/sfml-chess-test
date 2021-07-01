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

#include "chesslib/chess.h"

#include "main.h"

#define SQUARE_SIZE 45.0f

// Define resources
sfRenderWindow *window;

sfRectangleShape **boardSquares;
sfColor boardBlackColor;
sfColor boardWhiteColor;
sfColor boardHighlightColor;
sfColor backgroundColor;
sfColor checkColor;
sfColor pieceTransparentColor;
sfColor legalMoveColor;

sfRectangleShape *highlightSquare;
sq highlight1Sq;
sq highlight2Sq;

sfCircleShape *checkIndicator;

sqSet *legalMoveSet = NULL;
sfCircleShape *legalMoveIndicator;
sfCircleShape *legalCaptureIndicator;

pieceSet cburnett;

pieceSet *currentPieceSet = &cburnett;

sfSprite *sprPiece;

sfSound *sndMove;
sfSound *sndCapture;
sfSound *sndCheck;
sfSound *sndTerminal;

// Define global variables
int isDragging = 0;
sq draggingSq;
piece newDraggingPiece;
int isFlipped = 0;
int autoFlip = 0;
int isFullscreen = 0;
int showHighlighting = 1;
int playSound = 1;
int doRandomMoves = 0;
int showLegals = 1;

const char *initialFen;
int boardWidth = 8;
int boardHeight = 8;
#define NUM_SQUARES (boardWidth * boardHeight)

chess *g = NULL;


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
		else if ((strcmp(argv[i], "--width") == 0) || (strcmp(argv[i], "-w") == 0))
		{
			i++;
			if (i >= argc)
			{
				fprintf(stderr, "ERROR: Expected width after \"%s\"", argv[i - 1]);
				return 1;
			}
			boardWidth = atoi(argv[i]);
		}
		else if ((strcmp(argv[i], "--height") == 0) || (strcmp(argv[i], "-h") == 0))
		{
			i++;
			if (i >= argc)
			{
				fprintf(stderr, "ERROR: Expected height after \"%s\"", argv[i - 1]);
				return 1;
			}
			boardHeight = atoi(argv[i]);
		}
		else if ((strcmp(argv[i], "--dimensions") == 0) || (strcmp(argv[i], "-d") == 0))
		{
			i += 2;
			if (i >= argc)
			{
				fprintf(stderr, "ERROR: Expected width, height after \"%s\"", argv[i - 2]);
				return 1;
			}
			boardWidth = atoi(argv[i - 1]);
			boardHeight = atoi(argv[i]);
		}
		else if (i == (argc - 1))
		{
			// Treat last argument as FEN
			initialFen = argv[i];
		}
	}

	// Create the window
	sfVideoMode mode = {720, 720, 32};
	// Default values taken from https://www.sfml-dev.org/documentation/2.5.1/structsf_1_1ContextSettings.php
	sfContextSettings contextSettings = (sfContextSettings)
	{
		0, // depth
		0, // stencil
		4, // antialiasing
		1, // major
		1, // minor
		sfContextDefault, // attributes
		sfFalse // sRgb
	};
	window = sfRenderWindow_create(mode, "SFML Chess Board", sfDefaultStyle, &contextSettings);
	if (!window)
	{
		fprintf(stderr, "ERROR: Unable to create SFML window");
		return 1;
	}

	sfRenderWindow_setVerticalSyncEnabled(window, sfTrue);

	// Load textures
	loadPieceSet(&cburnett, "cburnett");

	// Set window icon
	sfVector2u texSize = sfTexture_getSize(cburnett.wN);
	sfImage *iconImage = sfTexture_copyToImage(cburnett.wN);
	sfRenderWindow_setIcon(window, texSize.x, texSize.y, sfImage_getPixelsPtr(iconImage));

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
	legalMoveColor = sfColor_fromRGBA(0, 0, 0, 100);

	// Create board squares
	boardSquares = (sfRectangleShape **) malloc(NUM_SQUARES * sizeof(sfRectangleShape *));
	for (int i = 0; i < NUM_SQUARES; i++)
	{
		sfRectangleShape *s = sfRectangleShape_create();

		sfRectangleShape_setSize(s, (sfVector2f) {SQUARE_SIZE, SQUARE_SIZE});
		sfRectangleShape_setPosition(s, (sfVector2f) {(float) (i % boardWidth) * SQUARE_SIZE, floor(i / boardWidth)
				* SQUARE_SIZE});

		int file = (i % boardWidth) + 1;
		int rank = boardHeight - floor(i / boardWidth);
		int isDark = sqIsDark((sq) {file, rank});
		sfRectangleShape_setFillColor(s, isDark ? boardBlackColor : boardWhiteColor);

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

	// Create legal move indicator
	legalMoveIndicator = sfCircleShape_create();

	sfCircleShape_setRadius(legalMoveIndicator, SQUARE_SIZE / 5.0f);
	sfCircleShape_setOrigin(legalMoveIndicator, (sfVector2f) {SQUARE_SIZE / 5.0f, SQUARE_SIZE / 5.0f});
	sfCircleShape_setFillColor(legalMoveIndicator, legalMoveColor);

	legalCaptureIndicator = sfCircleShape_create();

	sfCircleShape_setRadius(legalCaptureIndicator, SQUARE_SIZE / 2.2f);
	sfCircleShape_setOrigin(legalCaptureIndicator, (sfVector2f) {SQUARE_SIZE / 2.2f, SQUARE_SIZE / 2.2f});
	sfCircleShape_setFillColor(legalCaptureIndicator, legalMoveColor);

	calcView();

	initChess();

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
					sq s;
					piece p;
					if (getMouseSquare(event.mouseButton.x, event.mouseButton.y, &s))
					{
						if (chessGetTerminalState(g) == tsOngoing)
						{
							p = chessGetPiece(g, s);

							if (p && (pieceGetColor(p) == chessGetPlayer(g)))
							{
								isDragging = 1;
								draggingSq = s;
								newDraggingPiece = pEmpty;

								if (legalMoveSet)
									sqSetFree(legalMoveSet);
								legalMoveSet = getLegalSquareSet(s);
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

						sq s;
						if (getMouseSquare(event.mouseButton.x, event.mouseButton.y, &s))
						{
							move m = moveSq(draggingSq, s);
							if (pieceGetType(chessGetPiece(g, draggingSq)) == ptPawn && (s.rank == 1 || s.rank == boardHeight))
								m.promotion = ptQueen;

							uint8_t isCapture = (chessGetPiece(g, s) != pEmpty) ||
									(pieceGetType(chessGetPiece(g, draggingSq)) == ptPawn && (s.file != draggingSq.file));

							if (!chessPlayMove(g, m))
							{
								uint8_t isCheck = chessIsInCheck(g) && (chessGetTerminalState(g) == tsOngoing);

								if (doRandomMoves && (chessGetTerminalState(g) == tsOngoing))
								{
									board *aiBoard = chessGetBoard(g);
									playAiMove();

									move aiMove = chessGetMoveHistory(g)->tail->move;

									isCapture |= boardGetPiece(aiBoard, aiMove.to) ||
											(pieceGetType(boardGetPiece(aiBoard, aiMove.from)) == ptPawn
											&& aiMove.to.file != aiMove.from.file);

									isCheck |= chessIsInCheck(g);
								}

								if (playSound)
								{
									if (isCapture)
										sfSound_play(sndCapture);
									else
										sfSound_play(sndMove);

									if (chessGetTerminalState(g) != tsOngoing)
										sfSound_play(sndTerminal);
									else if (isCheck)
										sfSound_play(sndCheck);
								}

								updateGameState();
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
						if (isDragging)
							break;
						initChess();
						break;

					case sfKeyF:
						isFlipped = !isFlipped;
						break;

					case sfKeyA:
						autoFlip = !autoFlip;
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
						if (chessGetTerminalState(g) == tsOngoing)
						{
							board *aiBoard = chessGetBoard(g);
							playAiMove();

							move aiMove = chessGetMoveHistory(g)->tail->move;

							uint8_t isCapture = boardGetPiece(aiBoard, aiMove.to) ||
									(pieceGetType(boardGetPiece(aiBoard, aiMove.from)) == ptPawn
									&& aiMove.to.file != aiMove.from.file);

							if (playSound)
							{
								if (isCapture)
									sfSound_play(sndCapture);
								else
									sfSound_play(sndMove);

								if (chessGetTerminalState(g) != tsOngoing)
									sfSound_play(sndTerminal);
								else if (chessIsInCheck(g))
									sfSound_play(sndCheck);
							}

							updateGameState();
						}
						break;

					case sfKeyEnter:
						if (event.key.alt)
						{
							isFullscreen = !isFullscreen;
							sfRenderWindow_destroy(window);

							sfVideoMode videoMode = isFullscreen ? sfVideoMode_getDesktopMode() : mode;
							sfWindowStyle style = isFullscreen ? sfFullscreen : sfDefaultStyle;

							window = sfRenderWindow_create(videoMode, "SFML Chess Board", style, NULL);
							updateWindowTitle();

							sfRenderWindow_setIcon(window, texSize.x, texSize.y, sfImage_getPixelsPtr(iconImage));
							sfRenderWindow_setVerticalSyncEnabled(window, sfTrue);

							calcView();
						}
						break;

					case sfKeyG:
						if (isDragging)
							break;
						if (chessGetTerminalState(g) != tsOngoing)
							initChess();
						while (chessGetTerminalState(g) == tsOngoing)
						{
							move m = aiGetMove();
							chessPlayMove(g, m);
						}

						updateGameState();
						break;

					case sfKeyZ:
						if (isDragging)
							break;
						chessUndo(g);

						updateGameState();
						break;

					case sfKeyC:
						if (isDragging)
							break;
						if (chessCanClaimDraw50(g))
						{
							chessClaimDraw50(g);
							updateWindowTitle();
							if (playSound)
								sfSound_play(sndTerminal);
						}
						else if (chessCanClaimDrawThreefold(g))
						{
							chessClaimDrawThreefold(g);
							updateWindowTitle();
							if (playSound)
								sfSound_play(sndTerminal);
						}
						break;

					case sfKeyL:
						showLegals = !showLegals;
						break;

					default:
						break;
				}
			}
		}

		// Draw to window
		sfRenderWindow_clear(window, backgroundColor);

		// Draw the board
		for (int i = 0; i < NUM_SQUARES; i++)
		{
			sq s;
			s.file = (i % boardWidth) + 1;
			s.rank = boardHeight - floor(i / boardWidth);

			int index = isFlipped ? (NUM_SQUARES - 1) - i : i;

			piece p = chessGetPiece(g, s);
			pieceType pt = pieceGetType(p);

			if (p != pBlocker)
				sfRenderWindow_drawRectangleShape(window, boardSquares[index], NULL);

			if (showHighlighting && (sqEq(s, highlight1Sq) || sqEq(s, highlight2Sq)))
			{
				sfRectangleShape_setPosition(highlightSquare, sfRectangleShape_getPosition(boardSquares[index]));
				sfRenderWindow_drawRectangleShape(window, highlightSquare, NULL);
			}

			if (p != pEmpty && p != pBlocker)
			{
				if (pt == ptKing && chessIsSquareAttacked(g, s))
					drawCircleShape(checkIndicator, s);
				drawBoardPiece(p, s);
			}

			if (showLegals && isDragging)
			{
				if (sqSetGet(legalMoveSet, s))
					drawCircleShape(p ? legalCaptureIndicator : legalMoveIndicator, s);
			}
		}

		// Draw currently dragged piece
		if (isDragging)
		{
			piece p;
			if (newDraggingPiece)
				p = newDraggingPiece;
			else
				p = chessGetPiece(g, draggingSq);

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

	for (int i = 0; i < NUM_SQUARES; i++)
		sfRectangleShape_destroy(boardSquares[i]);
	free(boardSquares);

	destroyPieceSet(&cburnett);

	sfRectangleShape_destroy(highlightSquare);
	sfCircleShape_destroy(checkIndicator);
	sfCircleShape_destroy(legalMoveIndicator);
	sfCircleShape_destroy(legalCaptureIndicator);

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

void loadPieceSet(pieceSet *ps, const char *name)
{
	char *filename = (char *) malloc(strlen(name) + 12); // img/NAME/xX.png

	sprintf(filename, "img/%s/wP.png", name);
	ps->wP = sfTexture_createFromFile(filename, NULL);
	sprintf(filename, "img/%s/wN.png", name);
	ps->wN = sfTexture_createFromFile(filename, NULL);
	sprintf(filename, "img/%s/wB.png", name);
	ps->wB = sfTexture_createFromFile(filename, NULL);
	sprintf(filename, "img/%s/wR.png", name);
	ps->wR = sfTexture_createFromFile(filename, NULL);
	sprintf(filename, "img/%s/wQ.png", name);
	ps->wQ = sfTexture_createFromFile(filename, NULL);
	sprintf(filename, "img/%s/wK.png", name);
	ps->wK = sfTexture_createFromFile(filename, NULL);

	sprintf(filename, "img/%s/bP.png", name);
	ps->bP = sfTexture_createFromFile(filename, NULL);
	sprintf(filename, "img/%s/bN.png", name);
	ps->bN = sfTexture_createFromFile(filename, NULL);
	sprintf(filename, "img/%s/bB.png", name);
	ps->bB = sfTexture_createFromFile(filename, NULL);
	sprintf(filename, "img/%s/bR.png", name);
	ps->bR = sfTexture_createFromFile(filename, NULL);
	sprintf(filename, "img/%s/bQ.png", name);
	ps->bQ = sfTexture_createFromFile(filename, NULL);
	sprintf(filename, "img/%s/bK.png", name);
	ps->bK = sfTexture_createFromFile(filename, NULL);

	free(filename);

	// Set texture scaling
	sfTexture **pieceSetTextureArray = (sfTexture **) ps;
	for (int i = 0; i < 12; i++)
	{
		sfTexture_setSmooth(pieceSetTextureArray[i], sfTrue);
		sfTexture_generateMipmap(pieceSetTextureArray[i]);
	}
}

void destroyPieceSet(pieceSet *ps)
{
	sfTexture **pieceSetTextureArray = (sfTexture **) ps;
	for (int i = 0; i < 12; i++)
		sfTexture_destroy(pieceSetTextureArray[i]);
}

void calcView()
{
	sfVector2u windowSize = sfRenderWindow_getSize(window);
	float width = (float) windowSize.x;
	float height = (float) windowSize.y;

	float x = 0.0f;
	float y = 0.0f;
	float w = SQUARE_SIZE * ((float) boardWidth);
	float h = SQUARE_SIZE * ((float) boardHeight);

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

void drawBoardPiece(piece p, sq s)
{
	int coordsFile, coordsRank;

	if (isFlipped)
	{
		coordsFile = (boardWidth + 1) - s.file;
		coordsRank = (boardHeight + 1) - s.rank;
	}
	else
	{
		coordsFile = s.file;
		coordsRank = s.rank;
	}

	sfVector2f coords = (sfVector2f) {((float) coordsFile - 0.5f) * SQUARE_SIZE,
			((((float) boardHeight) + 0.5f) - (float) coordsRank) * SQUARE_SIZE};

	if (isDragging && sqEq(draggingSq, s))
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

void drawCircleShape(sfCircleShape *shape, sq s)
{
	int file = s.file;
	int rank = s.rank;

	if (isFlipped)
	{
		file = (boardWidth + 1) - file;
		rank = (boardHeight + 1) - rank;
	}

	sfCircleShape_setPosition(shape, (sfVector2f) {((float) file - 0.5f) * SQUARE_SIZE,
			((((float) boardHeight) + 0.5f) - (float) rank) * SQUARE_SIZE});
	sfRenderWindow_drawCircleShape(window, shape, NULL);
}

sfTexture *getPieceTex(piece p)
{
	switch (p)
	{
		case pWPawn:
			return currentPieceSet->wP;
		case pWKnight:
			return currentPieceSet->wN;
		case pWBishop:
			return currentPieceSet->wB;
		case pWRook:
			return currentPieceSet->wR;
		case pWQueen:
			return currentPieceSet->wQ;
		case pWKing:
			return currentPieceSet->wK;
		case pBPawn:
			return currentPieceSet->bP;
		case pBKnight:
			return currentPieceSet->bN;
		case pBBishop:
			return currentPieceSet->bB;
		case pBRook:
			return currentPieceSet->bR;
		case pBQueen:
			return currentPieceSet->bQ;
		case pBKing:
			return currentPieceSet->bK;
		default:
			return NULL;
	}
}

// This sets the values at the pointers to the correct file and rank.
// Returns true if position is inside board, false otherwise
int getMouseSquare(int mouseX, int mouseY, sq *s)
{
	sfVector2f coords = sfRenderWindow_mapPixelToCoords(window, (sfVector2i) {mouseX, mouseY}, NULL);
	coords.x /= SQUARE_SIZE;
	coords.y /= SQUARE_SIZE;

	if (coords.x < 0.0f || coords.x >= (float) boardWidth || coords.y < 0.0f || coords.y >= (float) boardHeight)
		return 0;

	s->file = 1 + (int) floor(coords.x);
	s->rank = boardHeight - (int) floor(coords.y);

	if (isFlipped)
	{
		s->file = (boardWidth + 1) - s->file;
		s->rank = (boardHeight + 1) - s->rank;
	}

	return 1;
}

void initChess()
{
	if (g)
		chessFree(g);

	g = chessCreateCustomDimensions(boardWidth, boardHeight, initialFen);

	if (!g)
	{
		fprintf(stderr, "Unable to create chess game");
		exit(1);
	}

	updateGameState();
}

void updateWindowTitle()
{
	char *fen = chessGetFen(g);
	size_t fenLen = strlen(fen);
	char *message = malloc(fenLen + 50);	// Upper bound on title length
	if (chessGetTerminalState(g) != tsOngoing)
	{
		char termMessage[40];
		switch (chessGetTerminalState(g))
		{
			case tsCheckmate:
				sprintf(termMessage, "%s wins: Checkmate", chessGetPlayer(g) == pcWhite ? "Black" : "White");
				break;
			case tsDrawStalemate:
				strcpy(termMessage, "Draw: Stalemate");
				break;
			case tsDrawClaimed50MoveRule:
				strcpy(termMessage, "Draw: 50 move");
				break;
			case tsDraw75MoveRule:
				strcpy(termMessage, "Draw: 75 move");
				break;
			case tsDrawClaimedThreefold:
				strcpy(termMessage, "Draw: Threefold");
				break;
			case tsDrawFivefold:
				strcpy(termMessage, "Draw: Fivefold");
				break;
			case tsDrawInsufficient:
				strcpy(termMessage, "Draw: Insufficient");
				break;
			default:
				sprintf(termMessage, "Unknown terminalState %d", chessGetTerminalState(g));
		}
		sprintf(message, "%s   %s", termMessage, fen);
	}
	else if (chessGetRepetitions(g) != 1)
	{
		sprintf(message, "Repetitions: %d   %s", chessGetRepetitions(g), fen);
	}
	else
	{
		strcpy(message, fen);
	}
	sfRenderWindow_setTitle(window, message);
	free(fen);
	free(message);
}

void updateGameState()
{
	updateWindowTitle();

	if (chessGetMoveHistory(g)->tail == NULL)
	{
		highlight1Sq = SQ_INVALID;
		highlight2Sq = SQ_INVALID;
	}
	else
	{
		move m = chessGetMoveHistory(g)->tail->move;

		highlight1Sq = m.from;
		highlight2Sq = m.to;
	}

	if (chessGetTerminalState(g) == tsOngoing)
	{
		if (autoFlip)
			isFlipped = chessGetPlayer(g) == pcBlack;
	}
}

// Returns the squares that can be reached by a legal move from the given starting square
sqSet *getLegalSquareSet(sq s)
{
	sqSet *ss = sqSetCreateCustomDimensions(boardWidth, boardHeight);

	for (moveListNode *n = chessGetLegalMoves(g)->head; n; n = n->next)
	{
		move m = n->move;
		if (sqEq(m.from, s))
			sqSetSet(ss, m.to, 1);
	}

	return ss;
}

/////////////////////////////
// DIFFERENT AI STRATEGIES //
/////////////////////////////

// TODO - make it so that the player can choose which AI to play against

// Strategy: PICK RANDOM MOVE
move aiRandomMove()
{
	moveList *list = chessGetLegalMoves(g);
	int randIndex = rand() % list->size;

	moveListNode *n = list->head;
	for (int i = 0; i < randIndex; i++)
		n = n->next;

	return n->move;
}

// Strategy: MINIMIZE OPPONENTS MOVES
// It will play a random move such that the number of responses is minimized
move aiMinOpponentMoves()
{
	moveList *list = chessGetLegalMoves(g);
	int size = list->size;
	int *responses = (int *) malloc(size * sizeof(int));

	// Figure out how many responses each move will let the opponent have
	board scratchBoard;
	for (int i = 0; i < size; i++)
	{
		memcpy(&scratchBoard, chessGetBoard(g), sizeof(board));

		move m = moveListGet(list, i);
		boardPlayMoveInPlace(&scratchBoard, m);
		moveList *newList = boardGenerateMoves(&scratchBoard);

		responses[i] = newList->size;

		moveListFree(newList);
	}

	// Determine what the number of least responses and how many there are
	int leastResponses = 1000000;
	int leastResponsesCount = 0;

	for (int i = 0; i < size; i++)
	{
		if (responses[i] < leastResponses)
		{
			leastResponses = responses[i];
			leastResponsesCount = 1;
		}
		else if (responses[i] == leastResponses)
		{
			leastResponsesCount++;
		}
	}

	// Now that we know how many, pick a random move out of those moves
	int randIndex = rand() % leastResponsesCount;

	// Move to the first move with the least number of responses
	int moveIndex = 0;
	while (responses[moveIndex] > leastResponses)
		moveIndex++;

	// Move to the next index where the random move is
	for (int i = 0; i < randIndex; i++)
	{
		moveIndex++;

		while (responses[moveIndex] > leastResponses)
			moveIndex++;
	}

	free(responses);

	// Play the given move
	return moveListGet(list, moveIndex);
}

// This is the function which determines which strategy the AI will use
move aiGetMove()
{
	return aiRandomMove();
}

void playAiMove()
{
	if (chessGetTerminalState(g) != tsOngoing)
		return;

	move m = aiGetMove();
	chessPlayMove(g, m);
}
