/*
 * BattleTextViewRendererTest.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BattleTestFixture.h"

#include "../../../server/battles/BattleTextViewRenderer.h"

#include "../../../lib/CStack.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/ObstacleHandler.h"
#include "../../../lib/battle/BattleHex.h"
#include "../../../lib/battle/BattleInfo.h"
#include "../../../lib/battle/CObstacleInstance.h"
#include "../../../lib/constants/Enumerations.h"

namespace
{
constexpr size_t maxLineLength = 80;
constexpr int gridRowCount = 11;
constexpr int gridColumnCount = 17;
constexpr int gridCellWidth = 4;
constexpr int rowLabelLength = 3; // "NN|"
constexpr size_t oddRowIndent = 2;
constexpr size_t firstGridLine = 2; // header, blank, then the rows

std::vector<std::string> splitLines(const std::string & text)
{
	std::istringstream stream(text);
	std::vector<std::string> lines;
	std::string line;

	while(std::getline(stream, line))
	{
		// telnet frames terminate every line with CRLF; assertions match the bare payload
		if(!line.empty() && line.back() == '\r')
			line.pop_back();
		lines.push_back(line);
	}

	return lines;
}

/// The four characters of the grid cell the hex occupies, or "" when the frame has no such cell.
std::string cellAt(const std::vector<std::string> & lines, const BattleHex & hex)
{
	const size_t lineIndex = firstGridLine + hex.getY();
	const size_t cellStart = rowLabelLength + (hex.getY() % 2 ? oddRowIndent : 0) + hex.getX() * gridCellWidth;

	if(lineIndex >= lines.size() || cellStart + gridCellWidth > lines[lineIndex].size())
		return {};

	return lines[lineIndex].substr(cellStart, gridCellWidth);
}
}

class BattleTextViewRendererTest : public BattleTestFixture
{
public:
	void SetUp() override
	{
		BattleTestFixture::SetUp();
		startGame();
		startBattle();
	}

	std::string renderPlainFrame(const std::vector<std::string> & logTail = {}) const
	{
		return battleTextView::renderBattleTextView(*battle(), logTail, battleTextView::RenderOptions{false});
	}
};

TEST_F(BattleTextViewRendererTest, GridStructure)
{
	const std::string frame = renderPlainFrame();
	ASSERT_FALSE(frame.empty());
	EXPECT_EQ(frame.back(), '\n');

	const auto lines = splitLines(frame);
	ASSERT_GE(lines.size(), firstGridLine + gridRowCount);
	EXPECT_TRUE(lines[1].empty()) << "grid follows the header across a blank line";
	EXPECT_NE(lines[0].find("VCMI battle mirror - battle #"), std::string::npos);

	for(int row = 0; row < gridRowCount; ++row)
	{
		SCOPED_TRACE("grid row " + std::to_string(row));

		std::string prefix = (row < 10 ? "0" : "") + std::to_string(row) + "|";
		if(row % 2 == 1)
			prefix += "  ";

		const std::string & line = lines[firstGridLine + row];
		EXPECT_EQ(line.substr(0, prefix.size()), prefix);
		EXPECT_EQ(line.size(), prefix.size() + gridColumnCount * gridCellWidth);
		EXPECT_LE(line.size(), maxLineLength);
	}

	EXPECT_EQ(lines[firstGridLine + 1].size() - lines[firstGridLine].size(), oddRowIndent)
		<< "odd rows carry the two-space indent even rows do not";

	for(const auto & line : lines)
		EXPECT_LE(line.size(), maxLineLength);
}

TEST_F(BattleTextViewRendererTest, StackRenderedAtPosition)
{
	const std::string frame = renderPlainFrame();

	EXPECT_NE(frame.find("aPik"), std::string::npos);
	EXPECT_NE(frame.find("dPik"), std::string::npos);

	std::vector<const CStack *> living;
	for(const auto & stack : battle()->stacks)
		if(stack->alive())
			living.push_back(stack.get());
	ASSERT_EQ(living.size(), 2u) << "fixture fields one Pikeman per side";

	std::sort(living.begin(), living.end(), [](const CStack * a, const CStack * b)
	{
		return a->unitId() < b->unitId();
	});

	const auto lines = splitLines(frame);
	size_t searchFrom = 0;

	for(const CStack * stack : living)
	{
		SCOPED_TRACE("stack " + std::to_string(stack->unitId()));

		const std::string sideLetter = stack->unitSide() == BattleSide::ATTACKER ? "a" : "d";
		EXPECT_NE(frame.find(sideLetter + stack->unitType()->getNameSingularTranslated().substr(0, 3)), std::string::npos);

		const std::string legend = sideLetter + " " + stack->getName()
			+ "  count " + std::to_string(stack->getCount())
			+ "  HP " + std::to_string(stack->getFirstHPleft()) + "/" + std::to_string(stack->getMaxHealth());

		const auto at = std::find(lines.begin() + searchFrom, lines.end(), legend);
		ASSERT_NE(at, lines.end()) << "missing legend line " << legend;
		searchFrom = at - lines.begin() + 1;
	}
}

TEST_F(BattleTextViewRendererTest, DoubleWideStackOccupiesBothHexes)
{
	CStack * champion = addStack(BattleSide::ATTACKER, creatureByName("core:champion"), BattleHex(leftHex), 1);
	ASSERT_NE(champion, nullptr);
	ASSERT_TRUE(champion->doubleWide());

	const std::string tag = "a" + champion->unitType()->getNameSingularTranslated().substr(0, 3);

	const auto lines = splitLines(renderPlainFrame());
	EXPECT_EQ(cellAt(lines, champion->getPosition()), tag);
	EXPECT_EQ(cellAt(lines, champion->occupiedHex()), tag);
}

TEST_F(BattleTextViewRendererTest, DeadStackRemovedFromGridAndLegend)
{
	CStack * victim = nullptr;
	for(const auto & stack : battle()->stacks)
		if(stack->alive() && stack->unitSide() == BattleSide::ATTACKER)
		{
			victim = stack.get();
			break;
		}
	ASSERT_NE(victim, nullptr);

	const BattleHex position = victim->getPosition();
	const std::string tag = "a" + victim->unitType()->getNameSingularTranslated().substr(0, 3);
	// grid cells never carry a space in position 2, headers start with "VCMI", log lines with '|'
	const auto isLegendLine = [](const std::string & line)
	{
		return line.size() > 2 && (line[0] == 'a' || line[0] == 'd') && line[1] == ' ';
	};

	const auto linesBefore = splitLines(renderPlainFrame());
	EXPECT_EQ(cellAt(linesBefore, position), tag) << "victim starts on the grid";
	EXPECT_EQ(std::count_if(linesBefore.begin(), linesBefore.end(), isLegendLine), 2u)
		<< "both stacks reported before the death";

	int64_t fatalDamage = 1'000'000'000;
	victim->damage(fatalDamage);
	ASSERT_FALSE(victim->alive());

	const auto lines = splitLines(renderPlainFrame());
	EXPECT_NE(cellAt(lines, position), tag) << "dead stack still rendered on the grid";

	std::vector<std::string> legends;
	for(const std::string & line : lines)
		if(isLegendLine(line))
			legends.push_back(line);
	ASSERT_EQ(legends.size(), 1u) << "only the surviving defender may remain in the legend";
	EXPECT_EQ(legends[0].substr(0, 2), "d ");
}

TEST_F(BattleTextViewRendererTest, ActiveStackMarker)
{
	const std::string before = renderPlainFrame();
	EXPECT_NE(before.find(", active: -"), std::string::npos) << "no active stack before the combat begins";
	EXPECT_EQ(before.find('*'), std::string::npos);

	beginCombat();

	const CStack * active = battle()->getStack(battle()->activeStack);
	ASSERT_NE(active, nullptr);

	const std::string frame = renderPlainFrame();
	EXPECT_NE(frame.find("*Pik"), std::string::npos);
	EXPECT_NE(frame.find("*" + active->unitType()->getNameSingularTranslated().substr(0, 3)), std::string::npos);

	const auto lines = splitLines(frame);
	ASSERT_FALSE(lines.empty());
	EXPECT_NE(lines[0].find(", active: " + active->getName()), std::string::npos);
}

TEST_F(BattleTextViewRendererTest, RoundInHeader)
{
	const auto lines = splitLines(renderPlainFrame());
	ASSERT_FALSE(lines.empty());

	EXPECT_NE(lines[0].find("VCMI battle mirror - battle #0, round " + std::to_string(battle()->getRound()) + ","),
		std::string::npos);
}

TEST_F(BattleTextViewRendererTest, LogTailRenderedAndBounded)
{
	std::vector<std::string> produced;
	for(int i = 0; i < 12; ++i)
		produced.push_back("line" + std::to_string(i));

	// the caller bounds the tail, so the renderer sees only the last eight and drops none of them
	const std::vector<std::string> bounded(produced.end() - 8, produced.end());
	const std::string frame = renderPlainFrame(bounded);

	for(int i = 4; i < 12; ++i)
		EXPECT_NE(frame.find("| line" + std::to_string(i) + "\r\n"), std::string::npos) << "dropped " << i;
	EXPECT_NE(frame.find("| line11\r\n"), std::string::npos);
	for(int i = 0; i < 4; ++i)
		EXPECT_EQ(frame.find("| line" + std::to_string(i) + "\r\n"), std::string::npos) << "oldest entry " << i << " leaked in";

	EXPECT_NE(frame.find("VCMI battle mirror - battle #0, round"), std::string::npos);
	EXPECT_NE(frame.find("\r\n"), std::string::npos) << "telnet NVT requires CRLF line endings";
	for(size_t i = 0; i < frame.size(); ++i)
	{
		if(frame[i] != '\n')
			continue;
		ASSERT_TRUE(i > 0 && frame[i - 1] == '\r') << "bare LF at offset " << i;
	}

	const std::string full = renderPlainFrame({"log a", "log b", "log c"});
	for(const char * entry : {"| log a", "| log b", "| log c"})
		EXPECT_NE(full.find(entry), std::string::npos);

	const auto lines = splitLines(full);
	ASSERT_GE(lines.size(), 4u);
	EXPECT_TRUE(lines[lines.size() - 4].empty()) << "log follows the legend across a blank line";
	EXPECT_EQ(lines[lines.size() - 3], "| log a");
	EXPECT_EQ(lines[lines.size() - 2], "| log b");
	EXPECT_EQ(lines[lines.size() - 1], "| log c");
}

TEST_F(BattleTextViewRendererTest, ObstacleMarkers)
{
	auto spellObstacle = std::make_shared<SpellCreatedObstacle>();
	spellObstacle->pos = BattleHex(leftHex);
	battle()->obstacles.push_back(spellObstacle);

	auto moatObstacle = std::make_shared<CObstacleInstance>();
	moatObstacle->obstacleType = CObstacleInstance::MOAT;
	moatObstacle->pos = BattleHex(leftHex + GameConstants::BFIELD_WIDTH);
	battle()->obstacles.push_back(moatObstacle);

	const auto lines = splitLines(renderPlainFrame());
	EXPECT_EQ(cellAt(lines, BattleHex(leftHex)), "%   ");
	EXPECT_EQ(cellAt(lines, BattleHex(leftHex + GameConstants::BFIELD_WIDTH)), "~   ");
}

TEST_F(BattleTextViewRendererTest, ObstacleCoversWholeFootprint)
{
	auto spellObstacle = std::make_shared<SpellCreatedObstacle>();
	spellObstacle->pos = BattleHex(leftHex);
	spellObstacle->customSize = BattleHexArray{BattleHex(leftHex), BattleHex(rightHex)};
	battle()->obstacles.push_back(spellObstacle);

	const auto lines = splitLines(renderPlainFrame());
	EXPECT_EQ(cellAt(lines, BattleHex(leftHex)), "%   ");
	EXPECT_EQ(cellAt(lines, BattleHex(rightHex)), "%   ");
}

TEST_F(BattleTextViewRendererTest, SpellCreatedMoatRendersWholeFootprint)
{
	auto moat = std::make_shared<SpellCreatedObstacle>();
	moat->obstacleType = CObstacleInstance::MOAT;
	moat->pos = BattleHex(leftHex);
	moat->customSize = BattleHexArray{BattleHex(leftHex), BattleHex(rightHex)};
	battle()->obstacles.push_back(moat);

	const auto lines = splitLines(renderPlainFrame());
	EXPECT_EQ(cellAt(lines, BattleHex(leftHex)), "~   ");
	EXPECT_EQ(cellAt(lines, BattleHex(rightHex)), "~   ");
}

TEST_F(BattleTextViewRendererTest, AbsoluteObstacleRendersWholeFootprint)
{
	// looked up at runtime so the test survives changes in obstacle handler data
	const ObstacleInfo * absolute = nullptr;
	for(const auto & info : LIBRARY->obstacleHandler->objects)
		if(info->isAbsoluteObstacle)
		{
			absolute = info.get();
			break;
		}
	ASSERT_NE(absolute, nullptr) << "no absolute obstacle in handler data";

	auto obstacle = std::make_shared<CObstacleInstance>();
	obstacle->obstacleType = CObstacleInstance::ABSOLUTE_OBSTACLE;
	obstacle->ID = absolute->getId().getNum();
	battle()->obstacles.push_back(obstacle);

	const BattleHexArray footprint = obstacle->getAffectedTiles();
	ASSERT_FALSE(footprint.empty());

	const auto lines = splitLines(renderPlainFrame());
	for(const BattleHex & hex : footprint)
	{
		SCOPED_TRACE("footprint hex " + std::to_string(hex.toInt()));
		EXPECT_EQ(cellAt(lines, hex), "#   ");
	}
}

// this test pins only the deterministic semantics: valid entries render and a non-empty footprint
// replaces the pos fallback; the skipped-slot guard itself is observable only through the
// out-of-bounds write it prevents, which requires a sanitizer no CI preset runs
TEST_F(BattleTextViewRendererTest, InvalidFootprintEntriesAreSkippedButFallbackNotTaken)
{
	auto spellObstacle = std::make_shared<SpellCreatedObstacle>();
	spellObstacle->pos = BattleHex(leftHex);
	// resize() default-fills with INVALID hexes and set() rewrites single slots - the shape a dirty deserialize leaves
	spellObstacle->customSize.resize(2);
	spellObstacle->customSize.set(0, BattleHex(rightHex));
	battle()->obstacles.push_back(spellObstacle);

	const auto lines = splitLines(renderPlainFrame());
	EXPECT_EQ(cellAt(lines, BattleHex(rightHex)), "%   ");
	EXPECT_NE(cellAt(lines, BattleHex(leftHex)), "%   ") << "non-empty footprint replaces the pos fallback";
}

TEST_F(BattleTextViewRendererTest, StackOverrulesMarkersAtSameHex)
{
	CStack * bystander = addStack(BattleSide::ATTACKER, creatureByName("core:pikeman"), BattleHex(leftHex), 1);
	ASSERT_NE(bystander, nullptr);

	auto spellObstacle = std::make_shared<SpellCreatedObstacle>();
	spellObstacle->pos = BattleHex(leftHex);
	spellObstacle->customSize = BattleHexArray{BattleHex(leftHex), BattleHex(rightHex)};
	battle()->obstacles.push_back(spellObstacle);

	const std::string tag = "a" + bystander->unitType()->getNameSingularTranslated().substr(0, 3);

	const auto lines = splitLines(renderPlainFrame());
	EXPECT_EQ(cellAt(lines, BattleHex(leftHex)), tag) << "obstacle marker hides the living stack";
	EXPECT_EQ(cellAt(lines, BattleHex(rightHex)), "%   ") << "the footprint hex without the stack keeps its obstacle marker";
}

TEST_F(BattleTextViewRendererTest, SiegeWalls)
{
	battle()->si.wallState[EWallPart::UPPER_WALL] = EWallState::DESTROYED;
	battle()->si.wallState[EWallPart::BOTTOM_WALL] = EWallState::DAMAGED;
	battle()->si.wallState[EWallPart::OVER_GATE] = EWallState::INTACT;
	battle()->si.wallState[EWallPart::GATE] = EWallState::INTACT;
	battle()->si.gateState = EGateState::CLOSED;
	battle()->si.wallState[EWallPart::KEEP] = EWallState::INTACT;
	battle()->si.wallState[EWallPart::UPPER_TOWER] = EWallState::INTACT;

	const auto lines = splitLines(renderPlainFrame());
	EXPECT_EQ(cellAt(lines, battle()->wallPartToBattleHex(EWallPart::UPPER_WALL)), "X   ");
	EXPECT_EQ(cellAt(lines, battle()->wallPartToBattleHex(EWallPart::BOTTOM_WALL)), "x   ");
	EXPECT_EQ(cellAt(lines, battle()->wallPartToBattleHex(EWallPart::OVER_GATE)), "=   ");
	EXPECT_EQ(cellAt(lines, battle()->wallPartToBattleHex(EWallPart::GATE)), "G   ");
	EXPECT_EQ(cellAt(lines, battle()->wallPartToBattleHex(EWallPart::KEEP)), "K   ");
	EXPECT_EQ(cellAt(lines, battle()->wallPartToBattleHex(EWallPart::UPPER_TOWER)), "T   ");

	EXPECT_EQ(cellAt(lines, BattleHex(BattleHex::HERO_ATTACKER)), "A   ");
	EXPECT_EQ(cellAt(lines, BattleHex(BattleHex::HERO_DEFENDER)), "D   ");
}

TEST_F(BattleTextViewRendererTest, IndestructibleSegmentsAndDrawbridge)
{
	EXPECT_EQ(renderPlainFrame().find('W'), std::string::npos) << "open field must not grow indestructible segments";

	battle()->si.wallState[EWallPart::GATE] = EWallState::INTACT;
	battle()->si.gateState = EGateState::CLOSED;

	auto lines = splitLines(renderPlainFrame());
	constexpr si16 indestructibleHexes[] = {45, 62, 112, 147, 165, BattleHex::GATE_OUTER};
	for(const si16 hex : indestructibleHexes)
	{
		SCOPED_TRACE("indestructible hex " + std::to_string(hex));
		EXPECT_EQ(cellAt(lines, BattleHex(hex)), "W   ");
	}
	EXPECT_EQ(cellAt(lines, BattleHex(BattleHex::GATE_BRIDGE)), "    ") << "closed gate leaves the bridge up";

	battle()->si.gateState = EGateState::OPENED;
	lines = splitLines(renderPlainFrame());
	EXPECT_EQ(cellAt(lines, BattleHex(BattleHex::GATE_BRIDGE)), "=   ");
}

TEST_F(BattleTextViewRendererTest, DestroyedKeepAndTowerRenderAsRuins)
{
	battle()->si.wallState[EWallPart::KEEP] = EWallState::DESTROYED;
	battle()->si.wallState[EWallPart::UPPER_TOWER] = EWallState::DESTROYED;
	battle()->si.wallState[EWallPart::BOTTOM_TOWER] = EWallState::INTACT;

	const auto lines = splitLines(renderPlainFrame());
	EXPECT_EQ(cellAt(lines, battle()->wallPartToBattleHex(EWallPart::KEEP)), "X   ");
	EXPECT_EQ(cellAt(lines, battle()->wallPartToBattleHex(EWallPart::UPPER_TOWER)), "X   ");
	EXPECT_EQ(cellAt(lines, battle()->wallPartToBattleHex(EWallPart::BOTTOM_TOWER)), "T   ");
}

TEST_F(BattleTextViewRendererTest, GateStatesCoverOpenDestroyedBlocked)
{
	battle()->si.wallState[EWallPart::GATE] = EWallState::INTACT;

	const std::pair<EGateState, std::string> scenarios[] = {
		{EGateState::OPENED, "g   "},
		{EGateState::DESTROYED, "X   "},
		{EGateState::BLOCKED, "G   "},
	};

	for(const auto & [state, marker] : scenarios)
	{
		SCOPED_TRACE("gate marker " + marker);
		battle()->si.gateState = state;
		EXPECT_EQ(cellAt(splitLines(renderPlainFrame()), battle()->wallPartToBattleHex(EWallPart::GATE)), marker);
	}
}

TEST_F(BattleTextViewRendererTest, AnsiVsPlain)
{
	const std::string ansi = battleTextView::renderBattleTextView(*battle(), {}, battleTextView::RenderOptions{true});
	EXPECT_TRUE(ansi.starts_with("\x1b[2J\x1b[H"));

	const std::string plain = renderPlainFrame();
	EXPECT_EQ(std::find(plain.begin(), plain.end(), '\x1b'), plain.end()) << "plain frame carries escape bytes";
}

TEST_F(BattleTextViewRendererTest, ActiveCellAnsiHighlight)
{
	beginCombat();

	const CStack * active = battle()->getStack(battle()->activeStack);
	ASSERT_NE(active, nullptr);

	std::string cell = active->unitSide() == BattleSide::DEFENDER ? "d" : "a";
	cell += active->unitType()->getNameSingularTranslated().substr(0, 3);
	if(cell.size() < 4)
		cell.append(4 - cell.size(), ' ');

	const std::string frame = battleTextView::renderBattleTextView(*battle(), {}, battleTextView::RenderOptions{true});
	EXPECT_NE(frame.find("\x1b[7m"), std::string::npos);
	EXPECT_NE(frame.find("\x1b[27m"), std::string::npos);
	EXPECT_NE(frame.find("\x1b[7m" + cell + "\x1b[27m"), std::string::npos)
		<< "reverse video does not wrap the active stack's cell";
}

TEST_F(BattleTextViewRendererTest, SummaryFrames)
{
	const std::string summary = battleTextView::renderBattleSummary(BattleID(0), PlayerColor(0), battleTextView::RenderOptions{false});
	EXPECT_NE(summary.find("VCMI battle mirror - battle #0"), std::string::npos);
	EXPECT_NE(summary.find("Battle ended, victor: player 0"), std::string::npos);
	EXPECT_EQ(std::find(summary.begin(), summary.end(), '\x1b'), summary.end());

	const std::string cancelled = battleTextView::renderBattleCancelled(BattleID(0), battleTextView::RenderOptions{false});
	EXPECT_NE(cancelled.find("VCMI battle mirror - battle #0"), std::string::npos);
	EXPECT_NE(cancelled.find("Battle cancelled."), std::string::npos);

	EXPECT_TRUE(battleTextView::renderBattleSummary(BattleID(0), PlayerColor(0), {}).starts_with("\x1b[2J\x1b[H"));
	EXPECT_TRUE(battleTextView::renderBattleCancelled(BattleID(0), {}).starts_with("\x1b[2J\x1b[H"));
}

TEST(BattleTextViewAbbreviateName, ClipsOnUtf8CodePointBoundaries)
{
	// Cyrillic and Latin-1 Supplement spell out the byte pattern explicitly: raw non-ASCII in
	// source survives too many editors to stay trustworthy here
	EXPECT_EQ(battleTextView::abbreviateName("\xD0\x9E\xD0\xB3\xD1\x80\xD1\x8B", 3), "\xD0\x9E\xD0\xB3\xD1\x80");
	EXPECT_EQ(battleTextView::abbreviateName("\xC3\x84rzte", 3), "\xC3\x84rz");
	EXPECT_EQ(battleTextView::abbreviateName("Pikeman", 3), "Pik");
	EXPECT_EQ(battleTextView::abbreviateName("Gr", 3), "Gr");
	EXPECT_EQ(battleTextView::abbreviateName("", 3), "");
}
