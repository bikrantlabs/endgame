#pragma once

#include "position.h"
#include <string>

// Open a PolyGlot opening book. Pass empty string for embedded book.
// Returns true if a book was loaded (file or embedded).
bool book_open(const std::string &path = "");

// Close the book file (called automatically on re-open or exit)
void book_close();

// Returns true if a book is currently loaded
bool book_opened();

// Probe the book for a move at the given position.
// Returns a weighted-random legal move, or Move{} if not in book.
Move book_probe(const Position &pos);
