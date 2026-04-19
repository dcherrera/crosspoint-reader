#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "BidiClassifier.h"

namespace Bidi {

// A contiguous run of text with a single resolved direction.
struct Run {
  std::string text;
  BaseDir dir;  // Resolved visual direction of this run
};

// Reorder a UTF-8 string into visual order for display.
//
// Implements a simplified Unicode Bidi Algorithm:
// 1. Split text into directional runs (contiguous LTR, RTL, or neutral segments)
// 2. Resolve neutral runs based on surrounding strong types and paragraph direction
// 3. Reverse the character order within RTL runs
// 4. If the paragraph is RTL, reverse the overall run order
//
// The returned runs can be concatenated and drawn left-to-right by the renderer,
// producing correct visual order for both LTR and RTL paragraphs.
//
// Limitations vs full UBA (Unicode TR9):
// - No support for embedding levels beyond 0/1
// - No explicit directional overrides (LRO, RLO, LRE, RLE, etc.)
// - Numeric handling is simplified (digits stay in logical order)
std::vector<Run> reorder(const char* text, BaseDir paragraphDir);

// Convenience: reorder and flatten into a single string in visual order.
// This is the main entry point for simple rendering — pass the result
// directly to drawText for correct visual display.
std::string toVisualOrder(const char* text, BaseDir paragraphDir);

}  // namespace Bidi
