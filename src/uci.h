// UCI engine support

#include "chesslib/chess.h"

void uciCreate(const char *execName);
void uciFree();

void uciSetInitialFen(const char *newInitialFen);
void uciSetLimit(const char *newLimit);

void uciSetOption(const char *name, const char *value);

move uciGetMove(chess *game);
