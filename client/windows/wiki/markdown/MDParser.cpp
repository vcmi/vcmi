/*
 * MDParser.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "MDParser.h"

// ============================================================================
// Block-level parsers
// ============================================================================

int MDParser::parseHeading(const std::string & line, std::string & outText)
{
	int    level = 0;
	size_t i     = 0;
	while(i < line.size() && line[i] == '#') { ++level; ++i; }
	if(level == 0 || level > 3 || i >= line.size() || line[i] != ' ')
		return 0;

	outText = line.substr(i + 1);
	while(!outText.empty() && (outText.back() == ' ' || outText.back() == '\t' || outText.back() == '\r'))
		outText.pop_back();
	return level;
}

bool MDParser::isHRule(const std::string & line)
{
	if(line.size() < 3) return false;
	const char rep = line[0];
	if(rep != '-' && rep != '_' && rep != '*') return false;

	int count = 0;
	for(char c : line)
	{
		if(c == rep)      ++count;
		else if(c != ' ') return false;
	}
	return count >= 3;
}

bool MDParser::parseBullet(const std::string & line, std::string & outText)
{
	if(line.size() >= 2 && (line[0] == '-' || line[0] == '*') && line[1] == ' ')
	{
		outText = line.substr(2);
		return true;
	}
	return false;
}

int MDParser::parseOrdered(const std::string & line, std::string & outText)
{
	size_t i = 0;
	while(i < line.size() && std::isdigit(static_cast<unsigned char>(line[i]))) ++i;
	if(i == 0 || i + 1 >= line.size() || line[i] != '.' || line[i + 1] != ' ')
		return -1;

	outText = line.substr(i + 2);
	return std::stoi(line.substr(0, i));
}

std::optional<MDAlignTag> MDParser::parseAlignTag(const std::string & line)
{
	if(line == "<center>")  return MDAlignTag{MDAlign::CENTER, false};
	if(line == "<left>")    return MDAlignTag{MDAlign::LEFT,   false};
	if(line == "<right>")   return MDAlignTag{MDAlign::RIGHT,  false};
	if(line == "</center>") return MDAlignTag{MDAlign::CENTER, true};
	if(line == "</left>")   return MDAlignTag{MDAlign::LEFT,   true};
	if(line == "</right>")  return MDAlignTag{MDAlign::RIGHT,  true};
	return std::nullopt;
}

std::string MDParser::parseAnchorTag(const std::string & s)
{
	// Matches <a id="name" /> or <a name="name" /> (case-insensitive)
	static const std::regex ANCHOR_RE(
		R"re(<a\s+(?:id|name)\s*=\s*"([^"]*?)"\s*/>)re", std::regex::icase);
	std::smatch m;
	return std::regex_search(s, m, ANCHOR_RE) ? m[1].str() : std::string{};
}

std::vector<std::string> MDParser::parseAllAnchorTags(const std::string & s)
{
	static const std::regex ANCHOR_RE(
		R"re(<a\s+(?:id|name)\s*=\s*"([^"]*?)"\s*/>)re", std::regex::icase);
	std::vector<std::string> ids;
	for(auto it = std::sregex_iterator(s.begin(), s.end(), ANCHOR_RE);
	    it != std::sregex_iterator(); ++it)
		ids.push_back((*it)[1].str());
	return ids;
}

std::string MDParser::stripAnchorTags(const std::string & s)
{
	static const std::regex ANCHOR_RE(
		R"re(<a\s+(?:id|name)\s*=\s*"[^"]*?"\s*/>)re", std::regex::icase);
	return std::regex_replace(s, ANCHOR_RE, "");
}

// ============================================================================
// Media line parser
// ============================================================================

bool MDParser::parseMediaLine(const std::string & line, ParsedMedia & out)
{
	out = {};

	std::string mediaPart = line;

	// Linked-media syntax: [![alt](path)](uri)
	if(line.size() > 6 && line[0] == '[' && line[1] == '!')
	{
		// inner starts with "![alt](path)](uri)"
		const std::string inner     = line.substr(1);
		const auto        altClose  = inner.find(']', 2);

		if(altClose != std::string::npos && altClose + 1 < inner.size() && inner[altClose + 1] == '(')
		{
			const auto pathClose = inner.find(')', altClose + 2);
			if(pathClose != std::string::npos
				&& pathClose + 2 < inner.size()
				&& inner[pathClose + 1] == ']'
				&& inner[pathClose + 2] == '(')
			{
				const auto uriClose = inner.find(')', pathClose + 3);
				if(uriClose != std::string::npos && uriClose + 1 == inner.size())
				{
					out.linkTarget = inner.substr(pathClose + 3, uriClose - pathClose - 3);
					mediaPart      = inner.substr(0, pathClose + 1);
				}
			}
		}
	}

	// Standard media syntax: ![alt](path)
	if(mediaPart.size() < 5 || mediaPart[0] != '!' || mediaPart[1] != '[') return false;
	const auto bracketClose = mediaPart.find(']', 2);
	if(bracketClose == std::string::npos)                                   return false;
	if(bracketClose + 1 >= mediaPart.size() || mediaPart[bracketClose + 1] != '(') return false;
	const auto parenClose = mediaPart.find(')', bracketClose + 2);
	if(parenClose == std::string::npos || parenClose + 1 != mediaPart.size()) return false;

	out.alt = mediaPart.substr(2, bracketClose - 2);
	std::string rawPath = mediaPart.substr(bracketClose + 2, parenClose - bracketClose - 2);
	if(rawPath.empty()) return false;

	// Optional frame suffix: path.def#N
	const auto hashPos = rawPath.rfind('#');
	if(hashPos != std::string::npos)
	{
		out.path     = rawPath.substr(0, hashPos);
		out.hasFrame = true;
		try   { out.frame = std::stoi(rawPath.substr(hashPos + 1)); }
		catch(const std::invalid_argument & e)
		{
			logGlobal->warn("WikiMarkdown: invalid frame index in '{}': {}", rawPath, e.what());
		}
		catch(const std::out_of_range & e)
		{
			logGlobal->warn("WikiMarkdown: frame index out of range in '{}': {}", rawPath, e.what());
		}
	}
	else
	{
		out.path = rawPath;
	}

	// Classify by file extension
	const auto dotPos = out.path.rfind('.');
	if(dotPos != std::string::npos)
	{
		std::string ext = out.path.substr(dotPos + 1);
		for(char & c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		out.isAnim  = (ext == "def" || ext == "json");
		out.isVideo = (ext == "bik" || ext == "smk" || ext == "webm" || ext == "mp4");
	}
	return true;
}

// ============================================================================
// Table helpers
// ============================================================================

bool MDParser::isTableRow(const std::string & line)
{
	if(line.size() < 3) return false;
	const size_t first = line.find_first_not_of(' ');
	const size_t last  = line.find_last_not_of(' ');
	return first != std::string::npos
	    && last  != std::string::npos
	    && line[first] == '|'
	    && line[last]  == '|';
}

bool MDParser::isTableSeparator(const std::string & row)
{
	bool hasDash = false;
	for(char c : row)
	{
		if(c == '-') { hasDash = true; continue; }
		if(c != '|' && c != ':' && c != ' ') return false;
	}
	return hasDash;
}

std::vector<std::string> MDParser::splitTableRow(const std::string & row)
{
	// Trim leading/trailing whitespace and strip outer '|' delimiters
	const size_t start = row.find_first_not_of(' ');
	const size_t end   = row.find_last_not_of(' ');
	if(start == std::string::npos) return {};

	std::string trimmed = row.substr(start, end - start + 1);
	if(!trimmed.empty() && trimmed.front() == '|') trimmed.erase(trimmed.begin());
	if(!trimmed.empty() && trimmed.back()  == '|') trimmed.pop_back();

	std::vector<std::string> cells;
	std::istringstream ss(trimmed);
	std::string cell;
	while(std::getline(ss, cell, '|'))
	{
		const size_t cs = cell.find_first_not_of(' ');
		const size_t ce = cell.find_last_not_of(' ');
		cells.push_back(cs == std::string::npos ? "" : cell.substr(cs, ce - cs + 1));
	}
	return cells;
}

// ============================================================================
// Inline tokenizer
// ============================================================================



std::vector<MDInlineToken> MDParser::tokenizeInline(const std::string & line)
{
	std::vector<MDInlineToken> result;
	std::string plainAcc;

	auto flushPlain = [&]()
	{
		if(!plainAcc.empty())
		{
			result.push_back({std::move(plainAcc), MDInlineType::PLAIN, {}});
			plainAcc.clear();
		}
	};

	size_t i = 0;
	while(i < line.size())
	{
		const char c = line[i];

		// Inline code: `...` — strip backticks, render inner text as plain
		if(c == '`')
		{
			const size_t end = line.find('`', i + 1);
			if(end == std::string::npos) { plainAcc += c; ++i; continue; }
			logGlobal->warn("WikiMarkdown: inline code syntax (backtick) is not supported, rendering as plain text");
			plainAcc += line.substr(i + 1, end - i - 1);
			i = end + 1;
			continue;
		}

		// Bold: **...** — strip markers, render inner text as plain
		if(c == '*' && i + 1 < line.size() && line[i + 1] == '*')
		{
			const size_t end = line.find("**", i + 2);
			if(end == std::string::npos) { plainAcc += c; ++i; continue; }
			logGlobal->warn("WikiMarkdown: bold text syntax (**) is not supported, rendering as plain text");
			plainAcc += line.substr(i + 2, end - i - 2);
			i = end + 2;
			continue;
		}

		// Italic: *...* — strip markers, render inner text as plain
		if(c == '*')
		{
			const size_t end = line.find('*', i + 1);
			if(end == std::string::npos) { plainAcc += c; ++i; continue; }
			logGlobal->warn("WikiMarkdown: italic text syntax (*) is not supported, rendering as plain text");
			plainAcc += line.substr(i + 1, end - i - 1);
			i = end + 1;
			continue;
		}

		// Link: [text](uri) — kept as a LINK token
		if(c == '[')
		{
			const size_t cb = line.find(']', i + 1);
			if(cb == std::string::npos || cb + 1 >= line.size() || line[cb + 1] != '(')
				{ plainAcc += c; ++i; continue; }

			const size_t cp = line.find(')', cb + 2);
			if(cp == std::string::npos) { plainAcc += c; ++i; continue; }

			flushPlain();
			result.push_back({
				line.substr(i + 1, cb - i - 1),
				MDInlineType::LINK,
				line.substr(cb + 2, cp - cb - 2)
			});
			i = cp + 1;
			continue;
		}

		plainAcc += c;
		++i;
	}
	flushPlain();
	return result;
}

bool MDParser::hasInlineMarkup(const std::string & s)
{
	// '*' covers both bold/italic (removed, but still detected to strip markers).
	// '`' covers inline code (removed, but still detected).
	// '[' covers wiki links (kept).
	return s.find('*') != std::string::npos
	    || s.find('`') != std::string::npos
	    || s.find('[') != std::string::npos;
}

std::vector<std::string> MDParser::splitPlainWithColorRewrap(const std::string & text)
{
	std::vector<std::string> result;
	std::string chunk;
	std::string colorPrefix; // active "{color|" prefix when we are inside a tag

	for(size_t i = 0; i < text.size(); ++i)
	{
		const char c = text[i];
		if(c == '{')
		{
			const size_t pipe  = text.find('|', i + 1);
			const size_t close = text.find('}', i + 1);
			if(pipe != std::string::npos && pipe < close)
				colorPrefix = text.substr(i, pipe - i + 1);
			chunk += c;
		}
		else if(c == '}')
		{
			colorPrefix.clear();
			chunk += c;
		}
		else if(c == ' ')
		{
			// Close the active colour tag before the space, re-open it after
			if(!colorPrefix.empty())
			{
				chunk += '}';
				chunk += ' ';
				result.push_back(std::move(chunk));
				chunk = colorPrefix;
			}
			else
			{
				chunk += ' ';
				result.push_back(std::move(chunk));
				chunk.clear();
			}
		}
		else
		{
			chunk += c;
		}
	}
	if(!chunk.empty())
		result.push_back(std::move(chunk));
	return result;
}

std::string MDParser::stripColorTags(const std::string & s)
{
	std::string out;
	out.reserve(s.size());
	for(size_t i = 0; i < s.size(); )
	{
		if(s[i] != '{') { out += s[i++]; continue; }
		++i;
		const size_t close = s.find('}', i);
		if(close == std::string::npos) { out += '{'; continue; }
		// Skip optional "color|" prefix; keep only the visible text part
		const size_t pipe      = s.find('|', i);
		const size_t textStart = (pipe != std::string::npos && pipe < close) ? pipe + 1 : i;
		out += s.substr(textStart, close - textStart);
		i    = close + 1;
	}
	return out;
}

std::string MDParser::replaceAll(std::string str, const std::string & from, const std::string & to)
{
	size_t pos = 0;
	while((pos = str.find(from, pos)) != std::string::npos)
	{
		str.replace(pos, from.size(), to);
		pos += to.size();
	}
	return str;
}

MDAlign MDParser::parseCellAlign(std::string & s)
{
	struct AlignTagPair
	{
		std::string_view open;
		std::string_view close;
		MDAlign          align;
	};
	static constexpr std::array<AlignTagPair, 3> TAGS {{
		{"<center>", "</center>", MDAlign::CENTER},
		{"<right>",  "</right>",  MDAlign::RIGHT },
		{"<left>",   "</left>",   MDAlign::LEFT  },
	}};
	for(const auto & tag : TAGS)
	{
		if(s.starts_with(tag.open))
		{
			s = s.substr(tag.open.size());
			if(s.ends_with(tag.close))
				s.resize(s.size() - tag.close.size());
			return tag.align;
		}
	}
	return MDAlign::LEFT;
}
