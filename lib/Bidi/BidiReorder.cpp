#include "BidiReorder.h"

#include <Utf8.h>

#include <algorithm>

namespace Bidi {

// Extract a single UTF-8 codepoint starting at buf[pos], advance pos past it.
// Returns the codepoint and the byte length consumed.
static uint32_t readCodepoint(const char* buf, size_t len, size_t& pos, size_t& cpLen) {
  const auto* p = reinterpret_cast<const uint8_t*>(buf + pos);
  const auto* start = p;
  uint32_t cp = utf8NextCodepoint(&p);
  cpLen = static_cast<size_t>(p - start);
  pos += cpLen;
  return cp;
}

// Reverse the UTF-8 codepoints in a string (not bytes — preserves multi-byte sequences).
static std::string reverseUtf8(const std::string& s) {
  // Collect codepoint byte ranges
  std::vector<std::pair<size_t, size_t>> ranges;  // (start, length)
  ranges.reserve(s.size() / 2);                   // rough estimate

  size_t pos = 0;
  while (pos < s.size()) {
    size_t cpLen;
    readCodepoint(s.data(), s.size(), pos, cpLen);
    ranges.emplace_back(pos - cpLen, cpLen);
  }

  std::string result;
  result.reserve(s.size());
  for (auto it = ranges.rbegin(); it != ranges.rend(); ++it) {
    result.append(s, it->first, it->second);
  }
  return result;
}

// Classify an entire run: if it contains any strong character, use that direction.
// Otherwise it's neutral.
static CharDir classifyRun(const std::string& text) {
  const auto* p = reinterpret_cast<const uint8_t*>(text.data());
  uint32_t cp;
  bool hasRTL = false;
  bool hasLTR = false;
  while ((cp = utf8NextCodepoint(&p))) {
    CharDir cd = classifyCodepoint(cp);
    if (cd == CharDir::RTL) hasRTL = true;
    if (cd == CharDir::LTR) hasLTR = true;
  }
  if (hasRTL) return CharDir::RTL;
  if (hasLTR) return CharDir::LTR;
  return CharDir::Neutral;
}

std::vector<Run> reorder(const char* text, BaseDir paragraphDir) {
  std::vector<Run> runs;
  if (!text || *text == '\0') return runs;

  // Step 1: Split into runs of same character direction.
  // Group consecutive characters that share the same CharDir into runs.
  struct LogicalRun {
    std::string text;
    CharDir dir;
  };
  std::vector<LogicalRun> logical;
  logical.reserve(8);

  size_t pos = 0;
  size_t len = 0;
  while (text[len]) len++;

  CharDir prevDir = CharDir::Neutral;
  std::string currentText;

  while (pos < len) {
    size_t startPos = pos;
    size_t cpLen;
    uint32_t cp = readCodepoint(text, len, pos, cpLen);

    // Combining marks inherit the direction of their base character
    CharDir cd;
    if (utf8IsCombiningMark(cp)) {
      cd = prevDir;
    } else {
      cd = classifyCodepoint(cp);
    }

    if (!currentText.empty() && cd != prevDir) {
      logical.push_back({std::move(currentText), prevDir});
      currentText.clear();
    }
    currentText.append(text + startPos, cpLen);
    prevDir = cd;
  }
  if (!currentText.empty()) {
    logical.push_back({std::move(currentText), prevDir});
  }

  if (logical.empty()) return runs;

  // Step 2: Resolve neutral runs.
  // Neutrals between two runs of the same direction adopt that direction.
  // Neutrals at the start/end adopt the paragraph direction.
  // Neutrals between LTR and RTL adopt the paragraph direction.
  for (size_t i = 0; i < logical.size(); i++) {
    if (logical[i].dir != CharDir::Neutral && logical[i].dir != CharDir::Numeric) continue;

    // Find surrounding strong directions
    CharDir before = (paragraphDir == BaseDir::RTL) ? CharDir::RTL : CharDir::LTR;
    CharDir after = before;

    for (int j = static_cast<int>(i) - 1; j >= 0; j--) {
      if (logical[j].dir == CharDir::LTR || logical[j].dir == CharDir::RTL) {
        before = logical[j].dir;
        break;
      }
    }
    for (size_t j = i + 1; j < logical.size(); j++) {
      if (logical[j].dir == CharDir::LTR || logical[j].dir == CharDir::RTL) {
        after = logical[j].dir;
        break;
      }
    }

    if (logical[i].dir == CharDir::Numeric) {
      // Numbers always display LTR in their logical order
      logical[i].dir = CharDir::LTR;
    } else if (before == after) {
      logical[i].dir = before;
    } else {
      logical[i].dir = (paragraphDir == BaseDir::RTL) ? CharDir::RTL : CharDir::LTR;
    }
  }

  // Step 3: Merge adjacent runs with the same resolved direction.
  std::vector<LogicalRun> merged;
  merged.reserve(logical.size());
  for (auto& lr : logical) {
    if (!merged.empty() && merged.back().dir == lr.dir) {
      merged.back().text += lr.text;
    } else {
      merged.push_back(std::move(lr));
    }
  }

  // Step 4: Reverse character order within RTL runs.
  // Step 5: Build output runs.
  runs.reserve(merged.size());
  for (auto& mr : merged) {
    BaseDir runDir = (mr.dir == CharDir::RTL) ? BaseDir::RTL : BaseDir::LTR;
    if (runDir == BaseDir::RTL) {
      runs.push_back({reverseUtf8(mr.text), runDir});
    } else {
      runs.push_back({std::move(mr.text), runDir});
    }
  }

  // Step 6: If paragraph direction is RTL, reverse the run order.
  // This makes the first logical run appear on the right.
  if (paragraphDir == BaseDir::RTL) {
    std::reverse(runs.begin(), runs.end());
  }

  return runs;
}

std::string toVisualOrder(const char* text, BaseDir paragraphDir) {
  auto runs = reorder(text, paragraphDir);
  std::string result;
  for (const auto& run : runs) {
    result += run.text;
  }
  return result;
}

}  // namespace Bidi
