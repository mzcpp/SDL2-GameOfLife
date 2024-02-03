#include "Game.hpp"

#include <memory>

int main(int argc, char* argv[])
{
	(void) argc;
	(void) argv;

	const std::unique_ptr<Game> game = std::make_unique<Game>();
	game->Run();

	return 0;
}
