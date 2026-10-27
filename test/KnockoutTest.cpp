#include <boost/test/unit_test.hpp>

#include <deque>
#include <set>
#include <random>

#include "KnockoutQueue.h"
#include "KnockoutTurns.h"
#include "WaitingPlayers.h"
#include "Crowd.h"
#include "PlayerProfile.h"
#include "SquishAnimation.h"
#include "DuelMatch.h"
#include "GameConstants.h"
#include "InputSource.h"
#include "GameLogic.h"
#include "DuelMatchState.h"

namespace
{
	void endTurn(KnockoutQueue& queue, PlayerSide side)
	{
		bool turnOver[MAX_PLAYERS] = {side == LEFT_PLAYER, side == RIGHT_PLAYER};
		bool error[MAX_PLAYERS] = {false, false};
		queue.update(turnOver, error);
	}

	void mistake(KnockoutQueue& queue, PlayerSide side)
	{
		bool touched[MAX_PLAYERS] = {false, false};
		bool error[MAX_PLAYERS] = {side == LEFT_PLAYER, side == RIGHT_PLAYER};
		queue.update(touched, error);
	}

	void passTime(GameLogicPtr& logic)
	{
		for (int i = 0; i < 50; ++i)
			logic->step(DuelMatchState());
	}
}

BOOST_AUTO_TEST_SUITE( knockout )

BOOST_AUTO_TEST_CASE( start_positions )
{
	KnockoutQueue queue(4, 1);
	BOOST_CHECK_EQUAL(queue.getActive(LEFT_PLAYER), 0);
	BOOST_CHECK_EQUAL(queue.getActive(RIGHT_PLAYER), 1);
	BOOST_CHECK(queue.getWaiting() == std::deque<int>({2, 3}));
	BOOST_CHECK_EQUAL(queue.getRemaining(), 4);
	BOOST_CHECK_EQUAL(queue.getWinner(), -1);
}

BOOST_AUTO_TEST_CASE( finished_turn_passes_blob_to_next_in_queue )
{
	KnockoutQueue queue(4, 1);

	endTurn(queue, LEFT_PLAYER);
	BOOST_CHECK_EQUAL(queue.getActive(LEFT_PLAYER), 2);
	BOOST_CHECK(queue.getWaiting() == std::deque<int>({3, 0}));

	// players take over whichever side becomes free
	endTurn(queue, RIGHT_PLAYER);
	BOOST_CHECK_EQUAL(queue.getActive(RIGHT_PLAYER), 3);
	BOOST_CHECK(queue.getWaiting() == std::deque<int>({0, 1}));

	endTurn(queue, RIGHT_PLAYER);
	BOOST_CHECK_EQUAL(queue.getActive(RIGHT_PLAYER), 0);	// player 0 now plays on the right
	BOOST_CHECK(queue.getWaiting() == std::deque<int>({1, 3}));
}

BOOST_AUTO_TEST_CASE( two_players_stay_on_court )
{
	KnockoutQueue queue(2, 1);
	endTurn(queue, LEFT_PLAYER);
	endTurn(queue, RIGHT_PLAYER);
	BOOST_CHECK_EQUAL(queue.getActive(LEFT_PLAYER), 0);
	BOOST_CHECK_EQUAL(queue.getActive(RIGHT_PLAYER), 1);
}

BOOST_AUTO_TEST_CASE( mistake_knocks_player_out )
{
	KnockoutQueue queue(3, 1);

	mistake(queue, LEFT_PLAYER);
	BOOST_CHECK(queue.isKnockedOut(0));
	BOOST_CHECK(queue.getKnockedOut() == std::vector<int>({0}));
	BOOST_CHECK_EQUAL(queue.getActive(LEFT_PLAYER), 2);
	BOOST_CHECK(queue.getWaiting().empty());			// knocked out players do not queue up
	BOOST_CHECK_EQUAL(queue.getRemaining(), 2);
	BOOST_CHECK_EQUAL(queue.getWinner(), -1);

	// the last two players stay on court
	endTurn(queue, RIGHT_PLAYER);
	BOOST_CHECK_EQUAL(queue.getActive(RIGHT_PLAYER), 1);
	BOOST_CHECK(queue.getKnockedOut().empty());

	mistake(queue, RIGHT_PLAYER);
	BOOST_CHECK_EQUAL(queue.getRemaining(), 1);
	BOOST_CHECK_EQUAL(queue.getWinner(), 2);
}

BOOST_AUTO_TEST_CASE( lives )
{
	KnockoutQueue queue(3, 2);

	mistake(queue, LEFT_PLAYER);
	BOOST_CHECK_EQUAL(queue.getLives(0), 1);
	BOOST_CHECK(queue.getKnockedOut().empty());
	BOOST_CHECK_EQUAL(queue.getActive(LEFT_PLAYER), 2);	// losing a life also ends the turn
	BOOST_CHECK(queue.getWaiting() == std::deque<int>({0}));

	mistake(queue, LEFT_PLAYER);
	BOOST_CHECK_EQUAL(queue.getActive(LEFT_PLAYER), 0);
	mistake(queue, LEFT_PLAYER);							// player 0 loses the second life
	BOOST_CHECK(queue.getKnockedOut() == std::vector<int>({0}));
	BOOST_CHECK_EQUAL(queue.getActive(LEFT_PLAYER), 2);
	BOOST_CHECK_EQUAL(queue.getLives(2), 1);
	BOOST_CHECK_EQUAL(queue.getRemaining(), 2);
}

// a mistake in the same frame in which the turn ends: the player must not leave twice
BOOST_AUTO_TEST_CASE( mistake_and_turn_over_together )
{
	KnockoutQueue queue(4, 2);
	bool turnOver[MAX_PLAYERS] = {true, false};
	bool error[MAX_PLAYERS] = {true, false};
	queue.update(turnOver, error);

	BOOST_CHECK_EQUAL(queue.getLives(0), 1);
	BOOST_CHECK_EQUAL(queue.getActive(LEFT_PLAYER), 2);
	BOOST_CHECK(queue.getWaiting() == std::deque<int>({3, 0}));
}

BOOST_AUTO_TEST_CASE( join_during_match )
{
	KnockoutQueue queue(2, 2);
	BOOST_CHECK_EQUAL(queue.addPlayer(2), 2);
	BOOST_CHECK_EQUAL(queue.getPlayerCount(), 3);
	BOOST_CHECK(queue.getWaiting() == std::deque<int>({2}));
	BOOST_CHECK_EQUAL(queue.getRemaining(), 3);

	// the new player takes over the next blob that becomes free
	endTurn(queue, RIGHT_PLAYER);
	BOOST_CHECK_EQUAL(queue.getActive(RIGHT_PLAYER), 2);
	BOOST_CHECK(queue.getWaiting() == std::deque<int>({1}));
}

BOOST_AUTO_TEST_CASE( join_after_last_two_remain )
{
	KnockoutQueue queue(3, 1);
	mistake(queue, LEFT_PLAYER);		// player 0 out, nobody waits any more
	BOOST_CHECK(queue.getWaiting().empty());

	BOOST_CHECK_EQUAL(queue.addPlayer(1), 3);
	BOOST_CHECK_EQUAL(queue.getRemaining(), 3);
	endTurn(queue, LEFT_PLAYER);			// player 2 makes room for the newcomer
	BOOST_CHECK_EQUAL(queue.getActive(LEFT_PLAYER), 3);
	BOOST_CHECK(queue.getWaiting() == std::deque<int>({2}));
}

BOOST_AUTO_TEST_CASE( no_join_after_match_is_decided )
{
	KnockoutQueue queue(2, 1);
	mistake(queue, LEFT_PLAYER);
	BOOST_CHECK_EQUAL(queue.getWinner(), 1);
	BOOST_CHECK_EQUAL(queue.addPlayer(1), -1);
	BOOST_CHECK_EQUAL(queue.getPlayerCount(), 2);
	BOOST_CHECK_EQUAL(queue.getWinner(), 1);
}

// random matches always end with exactly one winner, and nobody is ever on both sides
BOOST_AUTO_TEST_CASE( random_matches_have_one_winner )
{
	std::mt19937 random(42);
	for (int players = 2; players <= 8; ++players)
	{
		for (int lives = 1; lives <= 3; ++lives)
		{
			KnockoutQueue queue(players, lives);
			int frames = 0;
			while (queue.getWinner() == -1 && frames < 100000)
			{
				PlayerSide side = random() % 2 ? LEFT_PLAYER : RIGHT_PLAYER;
				if (queue.getPlayerCount() < 8 && random() % 50 == 0)
					queue.addPlayer(lives);
				else if (random() % 5 == 0)
					mistake(queue, side);
				else
					endTurn(queue, side);

				BOOST_REQUIRE_NE(queue.getActive(LEFT_PLAYER), queue.getActive(RIGHT_PLAYER));
				if (queue.getWinner() == -1)
				{
					BOOST_REQUIRE(!queue.isKnockedOut(queue.getActive(LEFT_PLAYER)));
					BOOST_REQUIRE(!queue.isKnockedOut(queue.getActive(RIGHT_PLAYER)));
					BOOST_REQUIRE_EQUAL(queue.getWaiting().size() + 2, (unsigned)queue.getRemaining());
				}
				++frames;
			}

			int winner = queue.getWinner();
			BOOST_REQUIRE_NE(winner, -1);
			BOOST_CHECK(winner == queue.getActive(LEFT_PLAYER) || winner == queue.getActive(RIGHT_PLAYER));
			for (int player = 0; player < queue.getPlayerCount(); ++player)
				BOOST_CHECK_EQUAL(queue.isKnockedOut(player), player != winner);
		}
	}
}

// the rule engine reports which hits count as touches, including the faulty fourth one
BOOST_AUTO_TEST_CASE( game_logic_reports_valid_touches )
{
	GameLogicPtr logic = createGameLogic(FALLBACK_RULES_NAME, 15);
	passTime(logic);

	BOOST_CHECK(logic->onBallHitsPlayer(LEFT_PLAYER));
	// the ball is still in contact with the blob in the next frame: not a new touch
	logic->step(DuelMatchState());
	BOOST_CHECK(!logic->onBallHitsPlayer(LEFT_PLAYER));

	for (int i = 0; i < 2; ++i)
	{
		passTime(logic);
		BOOST_CHECK(logic->onBallHitsPlayer(LEFT_PLAYER));
		BOOST_CHECK_EQUAL(logic->getLastErrorSide(), NO_PLAYER);
	}

	passTime(logic);
	BOOST_CHECK(logic->onBallHitsPlayer(LEFT_PLAYER));		// fourth touch
	BOOST_CHECK_EQUAL(logic->getLastErrorSide(), LEFT_PLAYER);
	BOOST_CHECK_EQUAL(logic->getTouches(LEFT_PLAYER), 0);	// why the touch counter cannot be used
}

// ---------------------------------------------------------------------------------------
// turns
// ---------------------------------------------------------------------------------------

namespace
{
	struct TurnFixture
	{
		TurnTracker tracker;
		bool turnOver[MAX_PLAYERS] = {false, false};

		// one frame, returns whether any turn ended
		bool frame(float ballX, bool touchLeft = false, bool touchRight = false, bool rally = true)
		{
			bool touched[MAX_PLAYERS] = {touchLeft, touchRight};
			tracker.update(touched, ballX, rally, turnOver);
			return turnOver[LEFT_PLAYER] || turnOver[RIGHT_PLAYER];
		}
	};
}

BOOST_FIXTURE_TEST_CASE( turn_ends_when_ball_passes_net_after_touch, TurnFixture )
{
	BOOST_CHECK(!frame(200));
	BOOST_CHECK(!frame(200, true));		// a touch alone does not end the turn
	BOOST_CHECK(!frame(300));
	BOOST_CHECK(!frame(400));			// above the net
	BOOST_CHECK(frame(450));			// passed to the right
	BOOST_CHECK(turnOver[LEFT_PLAYER]);
	BOOST_CHECK(!turnOver[RIGHT_PLAYER]);
	BOOST_CHECK(!frame(500));			// only once
}

BOOST_FIXTURE_TEST_CASE( several_touches_one_turn, TurnFixture )
{
	frame(600, false, true);
	frame(600, false, true);
	frame(600, false, true);
	BOOST_CHECK(frame(300));
	BOOST_CHECK(turnOver[RIGHT_PLAYER]);
}

BOOST_FIXTURE_TEST_CASE( ball_falling_back_from_net_keeps_turn, TurnFixture )
{
	frame(200, true);
	BOOST_CHECK(!frame(390));
	BOOST_CHECK(!frame(420));			// touches the net on the other side, but comes back
	BOOST_CHECK(!frame(300));
	BOOST_CHECK(frame(500));			// the next attempt goes over
	BOOST_CHECK(turnOver[LEFT_PLAYER]);
}

BOOST_FIXTURE_TEST_CASE( ball_passing_untouched_ends_no_turn, TurnFixture )
{
	frame(600);
	BOOST_CHECK(!frame(200));
}

BOOST_FIXTURE_TEST_CASE( touch_in_the_frame_the_ball_passes, TurnFixture )
{
	frame(330);
	BOOST_CHECK(frame(460, true));
	BOOST_CHECK(turnOver[LEFT_PLAYER]);
}

BOOST_FIXTURE_TEST_CASE( no_turns_while_ball_is_dead, TurnFixture )
{
	frame(200, true);
	BOOST_CHECK(!frame(600, false, false, false));	// rolling over after the point
	BOOST_CHECK(!frame(200, false, false, false));
	// new serve on the right: the reset position must not count as a pass
	BOOST_CHECK(!frame(650));
	BOOST_CHECK(!frame(650, false, true));
	BOOST_CHECK(frame(150));
	BOOST_CHECK(turnOver[RIGHT_PLAYER]);
}

// ---------------------------------------------------------------------------------------
// waiting players
// ---------------------------------------------------------------------------------------

namespace
{
	PlayerInput noInput(int) { return PlayerInput(); }

	const WaitingPlayers::View& viewOf(const std::vector<WaitingPlayers::View>& views, int player)
	{
		for (const auto& view : views)
		{
			if (view.player == player)
				return view;
		}
		BOOST_FAIL("player not shown");
		return views.front();
	}
}

BOOST_AUTO_TEST_CASE( waiting_players_ordered_by_depth )
{
	WaitingPlayers waiting;
	waiting.setQueue({3, 1, 2});

	auto views = waiting.getViews();
	BOOST_REQUIRE_EQUAL(views.size(), 3u);
	// drawn from the back to the front: the next player last, on top
	BOOST_CHECK_EQUAL(views[0].player, 2);
	BOOST_CHECK_EQUAL(views[1].player, 1);
	BOOST_CHECK_EQUAL(views[2].player, 3);
	// rows further back are higher on screen and smaller
	BOOST_CHECK_GT(views[2].scale, views[1].scale);
	BOOST_CHECK_GT(views[1].scale, views[0].scale);
	BOOST_CHECK_GT(WaitingPlayers::getRowGround(0), WaitingPlayers::getRowGround(1));
}

BOOST_AUTO_TEST_CASE( waiting_players_walk_and_jump )
{
	WaitingPlayers waiting;
	waiting.setQueue({0});
	Vector2 start = waiting.getViews()[0].position;

	for (int i = 0; i < 10; ++i)
		waiting.step([](int) { return PlayerInput(false, true, false); });
	BOOST_CHECK_GT(waiting.getViews()[0].position.x, start.x + 20);

	for (int i = 0; i < 1000; ++i)
		waiting.step([](int) { return PlayerInput(true, false, false); });
	float leftmost = waiting.getViews()[0].position.x;
	BOOST_CHECK_GT(leftmost, 0);							// stays on screen

	waiting.step([](int) { return PlayerInput(false, false, true); });
	float highest = waiting.getViews()[0].position.y;
	for (int i = 0; i < 10; ++i)
	{
		waiting.step(noInput);
		highest = std::min(highest, waiting.getViews()[0].position.y);
	}
	BOOST_CHECK_LT(highest, start.y - 30);					// jumped
	for (int i = 0; i < 100; ++i)
		waiting.step(noInput);
	BOOST_CHECK_CLOSE(waiting.getViews()[0].position.y, start.y, 0.01);	// and landed
	BOOST_CHECK_EQUAL(waiting.getViews()[0].position.x, leftmost);
}

BOOST_AUTO_TEST_CASE( player_from_court_walks_to_the_back )
{
	WaitingPlayers waiting;
	waiting.setQueue({1});
	waiting.arrive(5, 300);
	waiting.setQueue({1, 5});

	// arrives in front of the queue at the position of their court blob
	auto views = waiting.getViews();
	BOOST_CHECK_EQUAL(views.back().player, 5);
	BOOST_CHECK_EQUAL(viewOf(views, 5).position.x, 300);

	for (int i = 0; i < 100; ++i)
		waiting.step(noInput);
	views = waiting.getViews();
	BOOST_CHECK_EQUAL(views.front().player, 5);				// now behind player 1
	BOOST_CHECK_CLOSE(viewOf(views, 5).scale, WaitingPlayers::getRowScale(1), 0.01);

	// player 1 goes on court, player 5 moves up to the front row
	waiting.setQueue({5});
	for (int i = 0; i < 100; ++i)
		waiting.step(noInput);
	views = waiting.getViews();
	BOOST_REQUIRE_EQUAL(views.size(), 1u);
	BOOST_CHECK_CLOSE(views[0].scale, WaitingPlayers::getRowScale(0), 0.01);
}

// ---------------------------------------------------------------------------------------
// player profiles and squishing
// ---------------------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( profiles_are_parsed )
{
	auto profiles = PlayerProfile::parse(
		"# the usual crowd\n"
		"Anna;anna.png\n"
		"\n"
		"  Chlo\xc3\xa9 ; chloe face.jpg \r\n"
		"Bruno\n"
		";nameless.png\n");

	BOOST_REQUIRE_EQUAL(profiles.size(), 3u);
	BOOST_CHECK_EQUAL(profiles[0].name, "Anna");
	BOOST_CHECK_EQUAL(profiles[0].face, "anna.png");
	BOOST_CHECK_EQUAL(profiles[1].name, "Chlo\xc3\xa9");
	BOOST_CHECK_EQUAL(profiles[1].face, "chloe face.jpg");
	BOOST_CHECK_EQUAL(profiles[2].name, "Bruno");
	BOOST_CHECK(profiles[2].face.empty());
}

BOOST_AUTO_TEST_CASE( squish_goes_down_and_back )
{
	SquishAnimation squish;
	BOOST_CHECK_EQUAL(squish.frame(), 0);
	squish.start();

	int deepest = 0;
	int frames = 0;
	do
	{
		squish.step();
		deepest = std::max(deepest, squish.frame());
		++frames;
	} while (squish.speed != 0 && frames < 100);

	BOOST_CHECK_EQUAL(deepest, 4);
	BOOST_CHECK_EQUAL(squish.frame(), 0);
	BOOST_CHECK_LT(frames, 30);
}

BOOST_AUTO_TEST_CASE( waiting_blob_squishes_when_jumping )
{
	WaitingPlayers waiting;
	waiting.setQueue({0});
	waiting.step([](int) { return PlayerInput(false, false, true); });
	int deepest = 0;
	for (int i = 0; i < 6; ++i)
	{
		waiting.step(noInput);
		deepest = std::max(deepest, waiting.getViews()[0].frame);
	}
	BOOST_CHECK_GT(deepest, 1);
}

// ---------------------------------------------------------------------------------------
// swapping players
// ---------------------------------------------------------------------------------------

namespace
{
	class FixedInputSource : public InputSource
	{
		public:
			explicit FixedInputSource(PlayerInputAbs input) : mInput(input) {}
		private:
			PlayerInputAbs getNextInput() override { return mInput; }
			PlayerInputAbs mInput;
	};
}

// the next player takes over the blob standing in the middle of its side
BOOST_AUTO_TEST_CASE( respawn_puts_blob_in_the_middle )
{
	DuelMatch match(false, FALLBACK_RULES_NAME, 15);
	match.setInputSources(std::make_shared<FixedInputSource>(PlayerInputAbs(false, true, true)),
						  std::make_shared<FixedInputSource>(PlayerInputAbs(true, false, false)));
	for (int i = 0; i < 20; ++i)
		match.step();

	// the left blob ran to the right and jumped, the right one ran to the left
	BOOST_CHECK_GT(match.getBlobPosition(LEFT_PLAYER).x, 250);
	BOOST_CHECK_LT(match.getBlobPosition(LEFT_PLAYER).y, GROUND_PLANE_HEIGHT - 10);
	BOOST_CHECK_LT(match.getBlobPosition(RIGHT_PLAYER).x, 550);

	match.respawnBlob(LEFT_PLAYER);
	match.respawnBlob(RIGHT_PLAYER);
	BOOST_CHECK_EQUAL(match.getBlobPosition(LEFT_PLAYER).x, 200);
	BOOST_CHECK_EQUAL(match.getBlobPosition(LEFT_PLAYER).y, GROUND_PLANE_HEIGHT);
	BOOST_CHECK_EQUAL(match.getBlobPosition(RIGHT_PLAYER).x, 600);
	BOOST_CHECK_EQUAL(match.getBlobVelocity(LEFT_PLAYER).x, 0);
	BOOST_CHECK_EQUAL(match.getBlobVelocity(LEFT_PLAYER).y, 0);

	// it lands with a squish
	match.setInputSources(std::make_shared<InputSource>(), std::make_shared<InputSource>());
	match.step();
	BOOST_CHECK_GT(match.getBlobState(LEFT_PLAYER), 0);
}

namespace
{
	/// the bottom of a spectator
	float crowdFeet(const Crowd::View& view)
	{
		return view.position.y + BLOBBY_HEIGHT * view.scale / 2;
	}

	/// whether a spectator is jumping: their feet are above the ground at their depth
	bool inTheAir(const Crowd::View& view)
	{
		float depth = (view.scale - Crowd::FRONT_SCALE) / (Crowd::BACK_SCALE - Crowd::FRONT_SCALE);
		float ground = Crowd::FRONT_GROUND + (Crowd::BACK_GROUND - Crowd::FRONT_GROUND) * depth;
		return crowdFeet(view) < ground - 1;
	}

	const Crowd::View* findSpectator(const std::vector<Crowd::View>& views, int member)
	{
		for (const auto& view : views)
		{
			if (view.member == member)
				return &view;
		}
		return nullptr;
	}
}

// spectators come and go, the others keep their place
BOOST_AUTO_TEST_CASE( crowd_members_come_and_go )
{
	Crowd crowd(1);
	crowd.setMembers({0, 1, 2});
	auto before = crowd.getViews();
	BOOST_CHECK_EQUAL(before.size(), 3u);

	crowd.setMembers({1, 2, 5});
	auto after = crowd.getViews();
	BOOST_REQUIRE_EQUAL(after.size(), 3u);
	BOOST_CHECK(findSpectator(after, 0) == nullptr);
	BOOST_REQUIRE(findSpectator(after, 5) != nullptr);
	for (int member : {1, 2})
	{
		BOOST_CHECK_EQUAL(findSpectator(after, member)->position.x, findSpectator(before, member)->position.x);
		BOOST_CHECK_EQUAL(findSpectator(after, member)->scale, findSpectator(before, member)->scale);
	}
}

// the crowd is smaller than the last row of waiting players, spread over the beach and
// drawn from the back to the front
BOOST_AUTO_TEST_CASE( crowd_stands_behind_the_queue )
{
	Crowd crowd(2);
	std::vector<int> members;
	for (int i = 0; i < 38; ++i)
		members.push_back(i);
	crowd.setMembers(members);

	// of the eight players at most six wait, in rows 0 to 5
	const float backRowScale = WaitingPlayers::getRowScale(5);
	float left = 800, right = 0;
	for (int frame = 0; frame < 300; ++frame)
	{
		auto views = crowd.getViews();
		BOOST_REQUIRE_EQUAL(views.size(), members.size());
		for (unsigned i = 0; i < views.size(); ++i)
		{
			BOOST_CHECK_LT(views[i].scale, backRowScale);
			BOOST_CHECK_LE(crowdFeet(views[i]), Crowd::FRONT_GROUND + 0.01);
			if (i > 0)
				BOOST_CHECK_LE(views[i - 1].scale, views[i].scale);
			left = std::min(left, views[i].position.x);
			right = std::max(right, views[i].position.x);
		}
		crowd.step();
	}
	BOOST_CHECK_LT(left, 100);
	BOOST_CHECK_GT(right, 700);
}

// left alone, spectators hop around and squish
BOOST_AUTO_TEST_CASE( crowd_hops_by_itself )
{
	Crowd crowd(3);
	std::vector<int> members;
	for (int i = 0; i < 20; ++i)
		members.push_back(i);
	crowd.setMembers(members);

	std::set<int> jumped;
	bool squished = false;
	for (int frame = 0; frame < 600; ++frame)
	{
		crowd.step();
		for (const auto& view : crowd.getViews())
		{
			if (inTheAir(view))
				jumped.insert(view.member);
			squished = squished || view.frame > 0;
		}
	}
	BOOST_CHECK_GT(jumped.size(), members.size() / 2);
	BOOST_CHECK(squished);
}

// when the crowd cheers, everybody jumps within a moment
BOOST_AUTO_TEST_CASE( crowd_cheers_together )
{
	Crowd crowd(4);
	std::vector<int> members;
	for (int i = 0; i < 30; ++i)
		members.push_back(i);
	crowd.setMembers(members);
	// let the first hops after arriving calm down
	for (int frame = 0; frame < 120; ++frame)
		crowd.step();

	crowd.cheer();
	std::set<int> jumped;
	for (int frame = 0; frame < 30; ++frame)
	{
		crowd.step();
		for (const auto& view : crowd.getViews())
		{
			if (inTheAir(view))
				jumped.insert(view.member);
		}
	}
	BOOST_CHECK_EQUAL(jumped.size(), members.size());
}

BOOST_AUTO_TEST_SUITE_END()
