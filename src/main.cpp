#include "raylib.h"

#include <cstdlib>
#include <ctime>
#include <vector>

struct Cell {
  bool is_mine = false;
  bool revealed = false;
  int adj_mines = 0; // adjacent mines
};

const int GRID_COLS = 16;
const int GRID_ROWS = 16;
const int CELL_SIZE = 30;
const int NUM_MINES = 40;

std::vector<std::vector<Cell>> grid(GRID_ROWS, std::vector<Cell>(GRID_COLS));

void place_mines() {
  int mines_placed = 0;

  while (mines_placed < NUM_MINES) {
    int row = rand() % GRID_ROWS;
    int col = rand() % GRID_COLS;
    if (!grid[row][col].is_mine) {
      grid[row][col].is_mine = true;
      mines_placed++;
    }
  }
}

void calc_adj_mines() {
  for (int row = 0; row < GRID_ROWS; row++) {
    for (int col = 0; col < GRID_COLS; col++) {
      if (grid[row][col].is_mine) continue;

      int count = 0;
      for (int i = -1; i <= 1; i++){
        for (int j = -1; j <= 1; j++) {
          int newRow = row + i;
          int newCol = col + j;
          if (newRow >= 0 && newRow < GRID_ROWS && newCol >= 0 && newCol < GRID_COLS)
            if (grid[newRow][newCol].is_mine)
              count++;
        }
      }

      grid[row][col].adj_mines = count;
    }
  }
}

void reveal_cell(int row, int col) {
  if (row < 0 || row >= GRID_ROWS || col < 0 || col >= GRID_COLS) return;
  if (grid[row][col].revealed) return;

  grid[row][col].revealed = true;

  if (grid[row][col].adj_mines > 0) return;

  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      if (i || j)
        reveal_cell(row + i, col + j);
    }
  }
}

int main() {
  const int screen_width = GRID_COLS * CELL_SIZE;
  const int screen_height = GRID_ROWS * CELL_SIZE;

  InitWindow(screen_width, screen_height, "Minesweeper");
  SetTargetFPS(60);

  srand(time(NULL));
  place_mines();
  calc_adj_mines();

  bool game_over = false, game_won = false;

  while (!WindowShouldClose()) {
    // Mouse input
    if (!game_over && !game_won && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      Vector2 mouse_pos = GetMousePosition();
      int clicked_col = mouse_pos.x / CELL_SIZE;
      int clicked_row = mouse_pos.y / CELL_SIZE;

      if (clicked_row >= 0 && clicked_row < GRID_ROWS &&
          clicked_col >= 0 && clicked_col < GRID_COLS) {
        if (grid[clicked_row][clicked_col].is_mine) {
          game_over = true;
          grid[clicked_row][clicked_col].revealed = true;
        } else {
          reveal_cell(clicked_row, clicked_col);
        }
      }
    }

    // Winner check
    int revealed_count = 0;
    for (int row = 0; row < GRID_ROWS; row++) {
      for (int col = 0; col < GRID_COLS; col++) {
        if (grid[row][col].revealed)
          revealed_count++;
      }
    }
    if (revealed_count == GRID_ROWS * GRID_COLS - NUM_MINES) {
      game_won = true;
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Drawing grid
    for (int row = 0; row < GRID_ROWS; row++) {
      for (int col = 0; col < GRID_COLS; col++) {
        int x = col * CELL_SIZE;
        int y = row * CELL_SIZE;
        Rectangle cell_rect = { (float)x, (float)y, (float)CELL_SIZE, (float)CELL_SIZE };

        if (grid[row][col].revealed) {
          if (grid[row][col].is_mine) {
            DrawRectangleRec(cell_rect, RED);
            DrawText("M", x + CELL_SIZE / 4, y + CELL_SIZE / 4, 20, BLACK);
          } else {
            DrawRectangleRec(cell_rect, LIGHTGRAY);
            if (grid[row][col].adj_mines > 0) {
              DrawText(TextFormat("%d", grid[row][col].adj_mines), x + CELL_SIZE / 3, y + CELL_SIZE / 4, 20, DARKGRAY);
            }
          }
        } else {
          DrawRectangleRec(cell_rect, GRAY);
        }
        DrawRectangleLines(x, y, CELL_SIZE, CELL_SIZE, BLACK);
      }
    }

    if (game_over) {
      DrawText("Game Over!", screen_width / 2 - 70, screen_height / 2, 40, RED);
    } else if (game_won) {
      DrawText("You Win!", screen_width / 2 - 70, screen_height / 2, 40, GREEN);
    }

    EndDrawing();
  }

  CloseWindow();
  return 0;
}