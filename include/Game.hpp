#ifndef GAME_HPP
#define GAME_HPP

#include <SDL2/SDL.h>

#include <vector>
#include <cmath>

struct Cell
{
	SDL_Rect rect_;
	bool alive_;
};

template <typename T>
class Vector2d
{
public:
	T x;
	T y;

	void Normalize()
	{
		T length = GetLength();
		x /= length;
		y /= length;
	}

	void SetLength(T length)
	{
		Normalize();
		x *= length;
		y *= length;
	}

	T GetLength()
	{
		return std::sqrt((x * x) + (y * y));
	}
};

class Game
{
private:
	bool initialized_;
	bool running_;
	int ticks_;
	int evolve_speed_;

	int cell_size_;
	int cells_width_;
	int cells_height_;

	bool mouse_left_pressed_;
	bool setting_walls_;
	bool evolving_;

	std::vector<Cell> board_;
	std::vector<Cell> buffer_board_;
	std::vector<Cell> init_board_;
	SDL_Point mouse_position_;

	SDL_Window* window_;
	SDL_Renderer* renderer_;

public:
	Game();

	~Game();

	bool Initialize();

	void InitializeBoard(std::vector<Cell>* board);

	void Finalize();

	void EvolveBoard();

	void Run();

	void HandleEvents();
	
	void Tick();

	void Render();

	void RenderGrid();
	
	void RenderCells();

	std::vector<std::size_t> GetNeighboursIndices(std::size_t cell_index);
};

#endif