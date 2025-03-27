#include "raylib.h"

#include <vector>
#include <cstdlib>
#include <ctime>

struct Cell {
  bool is_mine = false;
  bool is_revealed = false;
  bool is_flagged = false;

  int adjacent_mines = 0;
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
      for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
          int new_row = row + i;
          int new_col = col + j;
          
          if (new_row >= 0 && new_row < GRID_ROWS &&
              new_col >= 0 && new_col < GRID_COLS) {
            if (grid[new_row][new_col].is_mine)
              count++;
          }
        }
      }

      grid[row][col].adjacent_mines = count;
    }
  }
}

void reveal_cell(int row, int col, bool &is_game_over) {
  if (row < 0 || row >= GRID_ROWS || col < 0 || col >= GRID_COLS) return;
  if (grid[row][col].is_revealed || grid[row][col].is_flagged) return;

  grid[row][col].is_revealed = true;

  if (grid[row][col].is_mine) {
    is_game_over = true;
    return;
  }

  // When we have adjacent mines, we stop recursion
  if (grid[row][col].adjacent_mines > 0)
    return;

  // Auto reveal
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      if (i != 0 || j != 0)
        reveal_cell(row + i, col + j, is_game_over);
    }
  }
}

void auto_reveal(int row, int col, bool &gameOver) {
  if (!grid[row][col].is_revealed || grid[row][col].is_mine) return;

  int num_flagged = 0;
  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      int new_row = row + i;
      int new_col = col + j;

      if (new_row >= 0 && new_row < GRID_ROWS &&
          new_col >= 0 && new_col < GRID_COLS) {
        if (grid[new_row][new_col].is_flagged)
          num_flagged++;
      }
    }
  }

  // When flagged count is equal to the cell's number,
  // we reveal neighbours that aren't flagged
  if (num_flagged == grid[row][col].adjacent_mines) {
    for (int i = -1; i <= 1; i++) {
      for (int j = -1; j <= 1; j++) {
        int new_row = row + i;
        int new_col = col + j;

        if (new_row >= 0 && new_row < GRID_ROWS &&
            new_col >= 0 && new_col < GRID_COLS) {
          if (!grid[new_row][new_col].is_flagged && !grid[new_row][new_col].is_revealed) {
            reveal_cell(new_row, new_col, gameOver);
          }  
        }
      }
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

  bool is_game_over = false, is_game_won = false;

  while (!WindowShouldClose()) {
    // LMB press (reveal)
    if (!is_game_over && !is_game_won && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      Vector2 mouse_pos = GetMousePosition();

      int lmb_col = mouse_pos.x / CELL_SIZE;
      int lmb_row = mouse_pos.y / CELL_SIZE;

      if (lmb_row >= 0 && lmb_row < GRID_ROWS &&
          lmb_col >= 0 && lmb_col < GRID_COLS) {
        if (!grid[lmb_row][lmb_col].is_flagged) {
          reveal_cell(lmb_row, lmb_col, is_game_over);
        }
      }
    }

    // RMB press (flag)
    if (!is_game_over && !is_game_won && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
      Vector2 mouse_pos = GetMousePosition();

      int rmb_col = mouse_pos.x / CELL_SIZE;
      int rmb_row = mouse_pos.y / CELL_SIZE;

      if (rmb_row >= 0 && rmb_row < GRID_ROWS &&
          rmb_col >= 0 && rmb_col < GRID_COLS) {
        if (!grid[rmb_row][rmb_col].is_revealed) {
          grid[rmb_row][rmb_col].is_flagged = !grid[rmb_row][rmb_col].is_flagged;
        }
      }
    }

    // MMB press (auto clear? auto reveal? idk wht it's called)
    if (!is_game_over && !is_game_won && IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
      Vector2 mouse_pos = GetMousePosition();

      int mmb_col = mouse_pos.x / CELL_SIZE;
      int mmb_row = mouse_pos.y / CELL_SIZE;

      if (mmb_row >= 0 && mmb_row < GRID_ROWS &&
          mmb_col >= 0 && mmb_col < GRID_COLS) {
        if (grid[mmb_row][mmb_col].is_revealed && grid[mmb_row][mmb_col].adjacent_mines > 0) {
          auto_reveal(mmb_row, mmb_col, is_game_over);
        }
      }
    }

    // Check for wins
    int revealed_count = 0;
    for (int row = 0; row < GRID_ROWS; row++) {
      for (int col = 0; col < GRID_COLS; col++) {
        if (grid[row][col].is_revealed)
          revealed_count++;
      }
    }

    if (revealed_count == GRID_ROWS * GRID_COLS - NUM_MINES) {
      is_game_won = true;
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Grid
    for (int row = 0; row < GRID_ROWS; row++) {
      for (int col = 0; col < GRID_COLS; col++) {
        int x = col * CELL_SIZE;
        int y = row * CELL_SIZE;
        Rectangle cell_rect = { (float)x, (float)y, (float)CELL_SIZE, (float)CELL_SIZE };

        if (grid[row][col].is_revealed) {
          if (grid[row][col].is_mine) {
            DrawRectangleRec(cell_rect, RED);
            DrawText("M", x + CELL_SIZE / 4, y + CELL_SIZE / 4, 20, BLACK);
          } else {
            DrawRectangleRec(cell_rect, LIGHTGRAY);
            if (grid[row][col].adjacent_mines > 0) {
              DrawText(TextFormat("%d", grid[row][col].adjacent_mines), x + CELL_SIZE / 3, y + CELL_SIZE / 4, 20, DARKGRAY);
            }
          }
        } else {
          DrawRectangleRec(cell_rect, GRAY);
          if (grid[row][col].is_flagged) {
            // maroon looks better than red lol
            DrawText("F", x + CELL_SIZE / 3, y + CELL_SIZE / 4, 20, MAROON);
          }
        }
        DrawRectangleLines(x, y, CELL_SIZE, CELL_SIZE, BLACK);
      }
    }

    if (is_game_over) {
      DrawText("Game Over!", screen_width / 2 - 70, screen_height / 2, 40, RED);
    } else if (is_game_won) {
      DrawText("You Win!", screen_width / 2 - 70, screen_height / 2, 40, GREEN);
    }

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
