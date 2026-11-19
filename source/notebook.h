#ifndef SH1_NOTEBOOK_H
#define SH1_NOTEBOOK_H
#include "checkpoint.h"

typedef enum { NotebookClosed, NotebookLoaded, NotebookMissingArt } NotebookResult;
NotebookResult notebookMenu(const char *dir, bool loading, Checkpoint *point);
#endif
