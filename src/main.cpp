#include "raylib.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>

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

const int UI_HEIGHT = 50; // space at the top for button and timer

std::vector<std::vector<Cell>> grid(GRID_ROWS, std::vector<Cell>(GRID_COLS));

bool confirm_reset = false;
double confirm_start_time = 0.0;
double paused_duration = 0.0;

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
  if (row < 0 || row >= GRID_ROWS || col < 0 || col >= GRID_COLS)
    return;
  if (grid[row][col].is_revealed || grid[row][col].is_flagged)
    return;

  grid[row][col].is_revealed = true;
  if (grid[row][col].is_mine) {
    is_game_over = true;
    return;
  }
  if (grid[row][col].adjacent_mines > 0)
    return;

  for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
      if (i != 0 || j != 0)
        reveal_cell(row + i, col + j, is_game_over);
    }
  }
}

void auto_reveal(int row, int col, bool &gameOver) {
  if (!grid[row][col].is_revealed || grid[row][col].is_mine)
    return;

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

void reset_game(bool &is_game_over, bool &is_game_won, double &startTime) {
  grid.assign(GRID_ROWS, std::vector<Cell>(GRID_COLS));
  place_mines();
  calc_adj_mines();
  is_game_over = false;
  is_game_won = false;
  startTime = GetTime();
  paused_duration = 0.0;
}

int main() {
  const int screen_width = GRID_COLS * CELL_SIZE;
  const int screen_height = GRID_ROWS * CELL_SIZE + UI_HEIGHT;

  InitWindow(screen_width, screen_height, "Minesweeper");
  SetTargetFPS(60);

  srand(time(NULL));
  bool is_game_over = false, is_game_won = false;
  double startTime = GetTime();

  reset_game(is_game_over, is_game_won, startTime);

  Rectangle new_game_btn = { 10, 10, 105, 30 };

  const int modal_width = 300;
  const int modal_height = 150;
  Rectangle modal_rect =
   { screen_width / 2 - modal_width / 2,
     screen_height / 2 - modal_height / 2,
      modal_width, modal_height };

  Rectangle yes_btn = { modal_rect.x + 30, modal_rect.y + modal_height - 50, 100, 30 };
  Rectangle no_btn  = { modal_rect.x + modal_width - 130, modal_rect.y + modal_height - 50, 100, 30 };

  while (!WindowShouldClose()) {
    Vector2 mouse_pos = GetMousePosition();

    if (confirm_reset) {
      if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        // Reset game if 'Yes'
        if (CheckCollisionPointRec(mouse_pos, yes_btn)) {
          reset_game(is_game_over, is_game_won, startTime);
          confirm_reset = false;
        }
        // Reset if 'No'
        else if (CheckCollisionPointRec(mouse_pos, no_btn)) {
          paused_duration += GetTime() - confirm_start_time;
          confirm_reset = false;
        }
      }
    } else {
      // LMB (either on cell or on new_game_btn)
      if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (CheckCollisionPointRec(mouse_pos, new_game_btn)) {
          if (!is_game_over && !is_game_won) {
            confirm_reset = true;
            confirm_start_time = GetTime();
          } else {
            reset_game(is_game_over, is_game_won, startTime);
          }
        } else {
          if (mouse_pos.y > UI_HEIGHT) {
            int gridX = mouse_pos.x;
            int gridY = mouse_pos.y - UI_HEIGHT;
            int cell_col = gridX / CELL_SIZE;
            int cell_row = gridY / CELL_SIZE;

            if (cell_row >= 0 && cell_row < GRID_ROWS &&
                cell_col >= 0 && cell_col < GRID_COLS) {
              if (!grid[cell_row][cell_col].is_flagged) {
                reveal_cell(cell_row, cell_col, is_game_over);
              }
            }
          }
        }
      }

      // RMB (flag)
      if (!is_game_over && !is_game_won && IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        if (mouse_pos.y > UI_HEIGHT) {
          int gridX = mouse_pos.x;
          int gridY = mouse_pos.y - UI_HEIGHT;
          int cell_col = gridX / CELL_SIZE;
          int cell_row = gridY / CELL_SIZE;

          if (cell_row >= 0 && cell_row < GRID_ROWS &&
            cell_col >= 0 && cell_col < GRID_COLS) {
            if (!grid[cell_row][cell_col].is_revealed) {
              grid[cell_row][cell_col].is_flagged = !grid[cell_row][cell_col].is_flagged;
            }
          }
        }
      }

      // MMB (auto clear/reveal)
      if (!is_game_over && !is_game_won && IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
        if (mouse_pos.y > UI_HEIGHT) {
          int gridX = mouse_pos.x;
          int gridY = mouse_pos.y - UI_HEIGHT;
          int cell_col = gridX / CELL_SIZE;
          int cell_row = gridY / CELL_SIZE;

          if (cell_row >= 0 && cell_row < GRID_ROWS &&
            cell_col >= 0 && cell_col < GRID_COLS) {
            if (grid[cell_row][cell_col].is_revealed && grid[cell_row][cell_col].adjacent_mines > 0) {
              auto_reveal(cell_row, cell_col, is_game_over);
            }
          }
        }
      }
    } // end confirmation check

    // Win condition
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

    // UI elements
    DrawRectangleRec(new_game_btn, LIGHTGRAY);
    DrawRectangleLines(new_game_btn.x, new_game_btn.y, new_game_btn.width, new_game_btn.height, BLACK);
    DrawText("New Game", new_game_btn.x + 5, new_game_btn.y + 5, 20, BLACK);

    // Calculating elpased time
    int elapsed = (int)(GetTime() - startTime - paused_duration);
    std::string timerText = "Time: " + std::to_string(elapsed) + "s";
    DrawText(timerText.c_str(), new_game_btn.x + new_game_btn.width + 20, new_game_btn.y + 5, 20, BLACK);

    // Grid
    for (int row = 0; row < GRID_ROWS; row++) {
      for (int col = 0; col < GRID_COLS; col++) {
        int x = col * CELL_SIZE;
        int y = row * CELL_SIZE + UI_HEIGHT;  // add UI offset
        Rectangle cell_rect = { (float)x, (float)y, (float)CELL_SIZE, (float)CELL_SIZE };

        if (grid[row][col].is_revealed) {
          if (grid[row][col].is_mine) {
            DrawRectangleRec(cell_rect, RED);
            DrawText("M", x + CELL_SIZE / 4, y + CELL_SIZE / 4, 20, BLACK);
          } else {
            DrawRectangleRec(cell_rect, LIGHTGRAY);
            if (grid[row][col].adjacent_mines > 0) {
              DrawText(TextFormat("%d", grid[row][col].adjacent_mines),
                   x + CELL_SIZE / 3, y + CELL_SIZE / 4, 20, DARKGRAY);
            }
          }
        } else {
          DrawRectangleRec(cell_rect, GRAY);
          if (grid[row][col].is_flagged) {
            DrawText("F", x + CELL_SIZE / 3, y + CELL_SIZE / 4, 20, MAROON);
          }
        }
        DrawRectangleLines(x, y, CELL_SIZE, CELL_SIZE, BLACK);
      }
    }

    // End of game screen
    if (is_game_over) {
      DrawText("Game Over!", screen_width / 2 - 70, screen_height / 2, 40, RED);
    } else if (is_game_won) {
      DrawText("You Win!", screen_width / 2 - 70, screen_height / 2, 40, DARKGREEN);
    }

    // When confirming reset, we draw the modal
    if (confirm_reset) {
      // Semi transparent background over the whole window
      DrawRectangle(0, 0, screen_width, screen_height, Fade(BLACK, 0.5f));
      // Modal box
      DrawRectangleRec(modal_rect, LIGHTGRAY);
      DrawRectangleLines(modal_rect.x, modal_rect.y, modal_rect.width, modal_rect.height, BLACK);
      DrawText("Are you sure?", modal_rect.x + 60, modal_rect.y + 30, 20, BLACK);

      // Yes button
      DrawRectangleRec(yes_btn, GREEN);
      DrawRectangleLines(yes_btn.x, yes_btn.y, yes_btn.width, yes_btn.height, BLACK);
      DrawText("Yes", yes_btn.x + 30, yes_btn.y + 5, 20, BLACK);

      // No button
      DrawRectangleRec(no_btn, RED);
      DrawRectangleLines(no_btn.x, no_btn.y, no_btn.width, no_btn.height, BLACK);
      DrawText("No", no_btn.x + 35, no_btn.y + 5, 20, BLACK);
    }

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
