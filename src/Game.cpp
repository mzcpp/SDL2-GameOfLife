#include "Game.hpp"
#include "Constants.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <cstdint>
#include <iostream>
#include <cmath>
#include <cassert>

Game::Game() : 
	initialized_(false), 
	running_(false), 
	ticks_(0), 
	evolve_speed_(10), 
	cell_size_(16), 
	cells_width_(constants::screen_width / cell_size_), 
	cells_height_(constants::screen_height / cell_size_), 
	mouse_left_pressed_(false), 
	setting_walls_(true), 
	evolving_(false)
{
	initialized_ = Initialize();

	InitializeBoard(&board_);
	InitializeBoard(&buffer_board_);
	InitializeBoard(&init_board_);

	mouse_position_ = { 0, 0 };
}

Game::~Game()
{
	Finalize();
}

bool Game::Initialize()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not be initialized! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"))
	{
		printf("%s\n", "Warning: Texture filtering is not enabled!");
	}

	window_ = SDL_CreateWindow(constants::game_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, constants::screen_width, constants::screen_height, SDL_WINDOW_SHOWN);

	if (window_ == nullptr)
	{
		printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);

	if (renderer_ == nullptr)
	{
		printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	constexpr int img_flags = IMG_INIT_PNG;

	if (!(IMG_Init(img_flags) & img_flags))
	{
		printf("SDL_image could not be initialized! SDL_image Error: %s\n", IMG_GetError());
		return false;
	}

	return true;
}

void Game::InitializeBoard(std::vector<Cell>* board)
{
	assert(board != nullptr);

	board->resize(cells_width_ * cells_height_);

	for (int y = 0; y < cells_height_; ++y)
	{
		for (int x = 0; x < cells_width_; ++x)
		{
			const int index = y * cells_width_ + x;

			(*board)[index].rect_.x = x * cell_size_;
			(*board)[index].rect_.y = y * cell_size_;
			(*board)[index].rect_.w = cell_size_;
			(*board)[index].rect_.h = (*board)[index].rect_.w;
			(*board)[index].alive_ = false;
		}
	}
}

void Game::Finalize()
{
	SDL_DestroyWindow(window_);
	window_ = nullptr;
	
	SDL_DestroyRenderer(renderer_);
	renderer_ = nullptr;

	SDL_Quit();
	IMG_Quit();
}

void Game::EvolveBoard()
{
	const bool all_cells_dead = std::all_of(board_.begin(), board_.end(), [](const Cell& cell)
		{
			return !cell.alive_;
		});

	if (all_cells_dead)
	{
		evolving_ = false;
		return;
	}

	buffer_board_ = board_;

	for (int y = 0; y < cells_height_; ++y)
	{
		for (int x = 0; x < cells_width_; ++x)
		{
			const int index = y * cells_width_ + x;
			const std::vector<std::size_t> neighbour_indices = GetNeighboursIndices(index);

			const int alive_neighbours = std::count_if(neighbour_indices.begin(), neighbour_indices.end(), [this](std::size_t neighbour_index)
				{
					return board_[neighbour_index].alive_;
				});

			if (board_[index].alive_)
			{
				if (alive_neighbours < 2 || alive_neighbours > 3)
				{
					buffer_board_[index].alive_ = false;
				}
			}
			else
			{
				if (alive_neighbours == 3)
				{
					buffer_board_[index].alive_ = true;
				}
			}
		}
	}

	std::swap(buffer_board_, board_);
}

void Game::Run()
{
	if (!initialized_)
	{
		return;
	}

	running_ = true;

	constexpr double ms = 1.0 / 60.0;
	std::uint64_t last_time = SDL_GetPerformanceCounter();
	long double delta = 0.0;

	double timer = SDL_GetTicks();

	int frames = 0;
	int ticks = 0;

	while (running_)
	{
		const std::uint64_t now = SDL_GetPerformanceCounter();
		const long double elapsed = static_cast<long double>(now - last_time) / static_cast<long double>(SDL_GetPerformanceFrequency());

		last_time = now;
		delta += elapsed;

		HandleEvents();

		while (delta >= ms)
		{
			Tick();
			delta -= ms;
			++ticks;
		}

		//printf("%Lf\n", delta / ms);
		Render();
		++frames;

		if (SDL_GetTicks() - timer > 1000.0)
		{
			timer += 1000.0;
			//printf("Frames: %d, Ticks: %d\n", frames, ticks);
			frames = 0;
			ticks = 0;
		}
	}
}

void Game::HandleEvents()
{
	SDL_Event e;

	while (SDL_PollEvent(&e) != 0)
	{
		SDL_GetMouseState(&mouse_position_.x, &mouse_position_.y);

		if (e.type == SDL_QUIT)
		{
			running_ = false;
			return;
		}
		
		if (!evolving_)
		{
			if (e.type == SDL_MOUSEBUTTONDOWN)
			{
				if (e.button.button == SDL_BUTTON_LEFT)
				{
					init_board_ = board_;
					mouse_left_pressed_ = true;
					const int index = (mouse_position_.y / cell_size_) * cells_width_ + (mouse_position_.x / cell_size_);
					setting_walls_ = !board_[index].alive_;
					board_[index].alive_ = !board_[index].alive_;
					init_board_[index].alive_ = !init_board_[index].alive_;
				}
			}
			else if (e.type == SDL_MOUSEBUTTONUP)
			{
				if (e.button.button == SDL_BUTTON_LEFT)
				{
					init_board_ = board_;
					mouse_left_pressed_ = false;
				}
			}
			
			if (e.type == SDL_MOUSEMOTION)
			{
				if (mouse_left_pressed_)
				{
					init_board_ = board_;
					const int index = (mouse_position_.y / cell_size_) * cells_width_ + (mouse_position_.x / cell_size_);
					board_[index].alive_ = setting_walls_;
					init_board_[index].alive_ = setting_walls_;
				}
			}
		}

		if (e.type == SDL_KEYDOWN)
		{
			constexpr int speed_adjustment = 5;

			if (e.key.keysym.sym == SDLK_s)
			{
				const bool all_cells_dead = std::all_of(board_.begin(), board_.end(), [](const Cell& cell)
					{
						return !cell.alive_;
					});

				if (!all_cells_dead)
				{
					evolving_ = !evolving_;
				}
			}
			else if (e.key.keysym.sym == SDLK_r)
			{
				evolving_ = false;
				board_ = init_board_;
			}
			else if (e.key.keysym.sym == SDLK_c)
			{
				if (!evolving_)
				{
					std::for_each(board_.begin(), board_.end(), [](Cell& cell)
						{
							cell.alive_ = false;
						});
					
					init_board_ = board_;
				}
			}
			else if (e.key.keysym.sym == SDLK_DOWN)
			{
				if (evolve_speed_ < 60)
				{
					evolve_speed_ += speed_adjustment;
				}
			}
			else if (e.key.keysym.sym == SDLK_UP)
			{
				if (evolve_speed_ - speed_adjustment >= 0)
				{
					evolve_speed_ -= speed_adjustment;
				}
			}
		}
	}
}

void Game::Tick()
{
	++ticks_;

	if (evolving_ && (evolve_speed_ == 0 || ticks_ % evolve_speed_ == 0))
	{
		EvolveBoard();
	}
}

void Game::Render()
{
	SDL_RenderSetViewport(renderer_, NULL);
	SDL_SetRenderDrawColor(renderer_, 0x00, 0x00, 0x00, 0xff);
	SDL_RenderClear(renderer_);

	RenderCells();
	RenderGrid();

	SDL_RenderPresent(renderer_);
}

void Game::RenderGrid()
{
	SDL_SetRenderDrawColor(renderer_, 0x14, 0x14, 0x14, 0xff);

	for (int y = 1; y < cells_height_; ++y)
	{
		SDL_RenderDrawLine(renderer_, 0, y * cell_size_, constants::screen_width, y * cell_size_);
	}

	for (int x = 1; x < cells_width_; ++x)
	{
		SDL_RenderDrawLine(renderer_, x * cell_size_, 0, x * cell_size_, constants::screen_height);
	}
}

void Game::RenderCells()
{
	for (int i = 0; i < cells_width_ * cells_height_; ++i)
	{
		if (board_[i].alive_)
		{
			SDL_SetRenderDrawColor(renderer_, 0xff, 0xff, 0x00, 0xff);
			SDL_RenderFillRect(renderer_, &board_[i].rect_);
		}
	}
}

std::vector<std::size_t> Game::GetNeighboursIndices(std::size_t cell_index)
{
	std::vector<std::size_t> result_indices;
	
	const SDL_Rect cell_rect = board_[cell_index].rect_;

	bool left_cell_available = (cell_rect.x - cell_size_) >= 0;
	bool right_cell_available = (cell_rect.x + cell_size_) < constants::screen_width;
	bool upper_cell_available = (cell_rect.y - cell_size_) >= 0;
	bool lower_cell_available = (cell_rect.y + cell_size_) < constants::screen_height;

	if (left_cell_available)
	{
		result_indices.emplace_back(cell_index - 1);
	}

	if (right_cell_available)
	{
		result_indices.emplace_back(cell_index + 1);
	}

	if (upper_cell_available)
	{
		result_indices.emplace_back(cell_index - (constants::screen_width / cell_size_));
	}

	if (lower_cell_available)
	{
		result_indices.emplace_back(cell_index + (constants::screen_width / cell_size_));
	}

	if (left_cell_available && upper_cell_available)
	{
		result_indices.emplace_back(cell_index - (constants::screen_width / cell_size_) - 1);
	}

	if (left_cell_available && lower_cell_available)
	{
		result_indices.emplace_back(cell_index + (constants::screen_width / cell_size_) - 1);
	}

	if (right_cell_available && upper_cell_available)
	{
		result_indices.emplace_back(cell_index - (constants::screen_width / cell_size_) + 1);
	}

	if (right_cell_available && lower_cell_available)
	{
		result_indices.emplace_back(cell_index + (constants::screen_width / cell_size_) + 1);
	}

	return result_indices;
}