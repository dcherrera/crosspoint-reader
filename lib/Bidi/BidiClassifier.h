#pragma once

#include <Utf8.h>

#include <cstdint>

namespace Bidi {

// Simplified Unicode bidirectional character types.
// Full UBA has 23 types; we use 4 for the common cases.
enum class CharDir : uint8_t {
  LTR,      // Left-to-right (Latin, Greek, Cyrillic, etc.)
  RTL,      // Right-to-left (Hebrew, Arabic, etc.)
  Neutral,  // Whitespace, punctuation, control characters
  Numeric   // European digits 0-9
};

// Paragraph-level base direction
enum class BaseDir : uint8_t { LTR, RTL };

// Classify a Unicode codepoint's bidirectional direction.
// Uses range checks (no lookup tables) to conserve RAM.
inline CharDir classifyCodepoint(uint32_t cp) {
  // ASCII fast path
  if (cp <= 0x007F) {
    if (cp >= 0x0041 && cp <= 0x005A) return CharDir::LTR;  // A-Z
    if (cp >= 0x0061 && cp <= 0x007A) return CharDir::LTR;  // a-z
    if (cp >= 0x0030 && cp <= 0x0039) return CharDir::Numeric;
    return CharDir::Neutral;  // space, punctuation, control
  }

  // Latin Extended, Greek, Cyrillic, etc.
  if (cp >= 0x0080 && cp <= 0x058F) return CharDir::LTR;

  // Hebrew (U+0590-U+05FF) and Hebrew presentation forms (U+FB1D-U+FB4F)
  if (cp >= 0x0590 && cp <= 0x05FF) return CharDir::RTL;
  if (cp >= 0xFB1D && cp <= 0xFB4F) return CharDir::RTL;

  // Arabic (U+0600-U+06FF), Arabic Supplement, Extended, Presentation Forms
  if (cp >= 0x0600 && cp <= 0x06FF) return CharDir::RTL;
  if (cp >= 0x0750 && cp <= 0x077F) return CharDir::RTL;  // Arabic Supplement
  if (cp >= 0x0870 && cp <= 0x089F) return CharDir::RTL;  // Arabic Extended-B
  if (cp >= 0x08A0 && cp <= 0x08FF) return CharDir::RTL;  // Arabic Extended-A
  if (cp >= 0xFB50 && cp <= 0xFDFF) return CharDir::RTL;  // Arabic Presentation Forms-A
  if (cp >= 0xFE70 && cp <= 0xFEFF) return CharDir::RTL;  // Arabic Presentation Forms-B

  // Extended Latin, Vietnamese, etc.
  if (cp >= 0x1E00 && cp <= 0x1EFF) return CharDir::LTR;

  // General punctuation, math, symbols — neutral
  if (cp >= 0x2000 && cp <= 0x2BFF) return CharDir::Neutral;

  // Default: treat as LTR (covers CJK, remaining scripts)
  return CharDir::LTR;
}

// Detect the dominant paragraph direction by scanning for the first strong
// (LTR or RTL) character. This follows the Unicode Bidi Algorithm P2/P3 rule.
// Returns BaseDir::LTR if no strong character is found.
inline BaseDir detectParagraphDirection(const char* text) {
  if (!text) return BaseDir::LTR;
  const auto* p = reinterpret_cast<const uint8_t*>(text);
  uint32_t cp;
  while ((cp = utf8NextCodepoint(&p))) {
    CharDir dir = classifyCodepoint(cp);
    if (dir == CharDir::RTL) return BaseDir::RTL;
    if (dir == CharDir::LTR) return BaseDir::LTR;
    // Skip neutral and numeric — keep scanning
  }
  return BaseDir::LTR;
}

}  // namespace Bidi
