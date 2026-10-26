#include <boost/test/unit_test.hpp>

#include <deque>
#include <random>

#include "KnockoutQueue.h"
#include "GameLogic.h"
#include "DuelMatchState.h"

namespace
{
	void touch(KnockoutQueue& queue, PlayerSide side)
	{
		bool touched[MAX_PLAYERS] = {side == LEFT_PLAYER, side == RIGHT_PLAYER};
		bool error[MAX_PLAYERS] = {false, false};
		queue.update(touched, error);
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

BOOST_AUTO_TEST_CASE( touch_passes_blob_to_next_in_queue )
{
	KnockoutQueue queue(4, 1);

	touch(queue, LEFT_PLAYER);
	BOOST_CHECK_EQUAL(queue.getActive(LEFT_PLAYER), 2);
	BOOST_CHECK(queue.getWaiting() == std::deque<int>({3, 0}));

	// players take over whichever side becomes free
	touch(queue, RIGHT_PLAYER);
	BOOST_CHECK_EQUAL(queue.getActive(RIGHT_PLAYER), 3);
	BOOST_CHECK(queue.getWaiting() == std::deque<int>({0, 1}));

	touch(queue, RIGHT_PLAYER);
	BOOST_CHECK_EQUAL(queue.getActive(RIGHT_PLAYER), 0);	// player 0 now plays on the right
	BOOST_CHECK(queue.getWaiting() == std::deque<int>({1, 3}));
}

BOOST_AUTO_TEST_CASE( two_players_stay_on_court )
{
	KnockoutQueue queue(2, 1);
	touch(queue, LEFT_PLAYER);
	touch(queue, RIGHT_PLAYER);
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
	touch(queue, RIGHT_PLAYER);
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

// the fourth touch of a side is a fault: it counts as the touch and the mistake of
// the same player, who must not leave the court twice
BOOST_AUTO_TEST_CASE( faulty_touch )
{
	KnockoutQueue queue(4, 2);
	bool touched[MAX_PLAYERS] = {true, false};
	bool error[MAX_PLAYERS] = {true, false};
	queue.update(touched, error);

	BOOST_CHECK_EQUAL(queue.getLives(0), 1);
	BOOST_CHECK_EQUAL(queue.getActive(LEFT_PLAYER), 2);
	BOOST_CHECK(queue.getWaiting() == std::deque<int>({3, 0}));
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
				if (random() % 5 == 0)
					mistake(queue, side);
				else
					touch(queue, side);

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
			for (int player = 0; player < players; ++player)
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

BOOST_AUTO_TEST_SUITE_END()
