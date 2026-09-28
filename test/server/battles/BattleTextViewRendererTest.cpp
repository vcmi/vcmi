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
		lines.push_back(line);

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
		EXPECT_NE(frame.find("| line" + std::to_string(i) + "\n"), std::string::npos) << "dropped " << i;
	EXPECT_NE(frame.find("| line11\n"), std::string::npos);
	for(int i = 0; i < 4; ++i)
		EXPECT_EQ(frame.find("| line" + std::to_string(i) + "\n"), std::string::npos) << "oldest entry " << i << " leaked in";

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

TEST_F(BattleTextViewRendererTest, SiegeWalls)
{
	battle()->si.wallState[EWallPart::UPPER_WALL] = EWallState::DESTROYED;
	battle()->si.wallState[EWallPart::GATE] = EWallState::INTACT;
	battle()->si.gateState = EGateState::CLOSED;
	battle()->si.wallState[EWallPart::KEEP] = EWallState::INTACT;

	const auto lines = splitLines(renderPlainFrame());
	EXPECT_EQ(cellAt(lines, battle()->wallPartToBattleHex(EWallPart::UPPER_WALL)), "X   ");
	EXPECT_EQ(cellAt(lines, battle()->wallPartToBattleHex(EWallPart::GATE)), "G   ");
	EXPECT_EQ(cellAt(lines, battle()->wallPartToBattleHex(EWallPart::KEEP)), "K   ");

	EXPECT_EQ(cellAt(lines, BattleHex(BattleHex::HERO_ATTACKER)), "A   ");
	EXPECT_EQ(cellAt(lines, BattleHex(BattleHex::HERO_DEFENDER)), "D   ");
}

TEST_F(BattleTextViewRendererTest, AnsiVsPlain)
{
	const std::string ansi = battleTextView::renderBattleTextView(*battle(), {}, battleTextView::RenderOptions{true});
	EXPECT_TRUE(ansi.starts_with("\x1b[2J\x1b[H"));

	const std::string plain = renderPlainFrame();
	EXPECT_EQ(std::find(plain.begin(), plain.end(), '\x1b'), plain.end()) << "plain frame carries escape bytes";
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
