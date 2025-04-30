#include "raylib.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>

struct Cell {
  bool is_mine = false;
  bool is_revealed = false;
  bool is_flagged = false;
  int adjacent_mines = 0;
};

class MinesweeperGame {
private:
  const int grid_cols = 16;
  const int grid_rows = 16;
  const int cell_size = 30;
  const int num_mines = 40;
  const int ui_height = 50;

  std::vector<std::vector<Cell>> grid{};

  double start_time{};
  double final_time{};
  double paused_duration{};
  double confirm_start_time{};

  bool is_game_over{};
  bool is_game_won{};
  bool is_confirm_reset{};
  bool is_first_click{};

  Texture2D mine_texture{};
  Texture2D flag_texture{};

  // UI rectangles
  Rectangle new_game_btn{};
  Rectangle modal_rect{};
  Rectangle yes_btn{};
  Rectangle no_btn{};

  // Colours for numbers 1 to 8
  Color num_colours[9] = {
    WHITE,                 // index 0 (placeholder)
    { 0, 70, 241, 255 },   // index 1
    DARKGREEN,             // index 2
    RED,                   // index 3
    DARKBLUE,              // index 4
    ORANGE,                // index 5
    { 64, 224, 208, 255 }, // index 6 (turquoise)
    BLACK,                 // index 7
    GRAY                   // index 8
  };

public:
  MinesweeperGame() {
    grid.resize(grid_rows, std::vector<Cell>(grid_cols));
    start_time = GetTime();
    final_time = 0.0;
    paused_duration = 0.0;
    confirm_start_time = 0.0;
    is_game_over = false;
    is_game_won = false;
    is_confirm_reset = false;
    is_first_click = true;

    // Set up UI rectangles
    new_game_btn = { 10, 10, 105, 30 };
    int modal_width = 300;
    int modal_height = 150;
    int screen_width = grid_cols * cell_size;
    int screen_height = grid_rows * cell_size + ui_height;
    modal_rect = { (float)(screen_width / 2 - modal_width / 2),
                   (float)(screen_height / 2 - modal_height / 2),
                   (float)modal_width, (float)modal_height };
    yes_btn = { modal_rect.x + 30, modal_rect.y + modal_height - 50, 100, 30 };
    no_btn  = { modal_rect.x + modal_width - 130, modal_rect.y + modal_height - 50, 100, 30 };

    mine_texture = LoadTexture("assets/mine.png");
    flag_texture = LoadTexture("assets/flag.png");

    Image window_icon = LoadImage("assets/mine.png");
    SetWindowIcon(window_icon);
    UnloadImage(window_icon);

    reset_game();
  }

  ~MinesweeperGame() {
    if (mine_texture.width > 0) UnloadTexture(mine_texture);
    if (flag_texture.width > 0) UnloadTexture(flag_texture);
  }

  void place_mines() {
    int mines_placed = 0;
    while (mines_placed < num_mines) {
      int row = rand() % grid_rows;
      int col = rand() % grid_cols;
      if (!grid[row][col].is_mine) {
        grid[row][col].is_mine = true;
        mines_placed++;
      }
    }
  }

  void calc_adj_mines() {
    for (int row = 0; row < grid_rows; row++) {
      for (int col = 0; col < grid_cols; col++) {
        if (grid[row][col].is_mine)
          continue;
        int count = 0;
        for (int i = -1; i <= 1; i++) {
          for (int j = -1; j <= 1; j++) {
            int new_row = row + i;
            int new_col = col + j;
            if (new_row >= 0 && new_row < grid_rows &&
                new_col >= 0 && new_col < grid_cols) {
              if (grid[new_row][new_col].is_mine)
                count++;
            }
          }
        }
        grid[row][col].adjacent_mines = count;
      }
    }
  }

  void first_click_safe_zone(int safe_row, int safe_col) {
    int min_row = std::max(0, safe_row - 1);
    int max_row = std::min(grid_rows - 1, safe_row + 1);
    int min_col = std::max(0, safe_col - 1);
    int max_col = std::min(grid_cols - 1, safe_col + 1);
    for (int row = min_row; row <= max_row; row++) {
      for (int col = min_col; col <= max_col; col++) {
        if (grid[row][col].is_mine) {
          grid[row][col].is_mine = false;
          bool placed = false;
          for (int r = 0; r < grid_rows && !placed; r++) {
            for (int c = 0; c < grid_cols && !placed; c++) {
              if (r < min_row || r > max_row || c < min_col || c > max_col) {
                if (!grid[r][c].is_mine) {
                  grid[r][c].is_mine = true;
                  placed = true;
                }
              }
            }
          }
        }
      }
    }
    calc_adj_mines();
  }

  void reveal_cell(int row, int col, bool &game_over) {
    if (row < 0 || row >= grid_rows || col < 0 || col >= grid_cols)
      return;
    if (grid[row][col].is_revealed || grid[row][col].is_flagged)
      return;
    grid[row][col].is_revealed = true;
    if (grid[row][col].is_mine) {
      game_over = true;
      return;
    }
    if (grid[row][col].adjacent_mines > 0)
      return;
    for (int i = -1; i <= 1; i++) {
      for (int j = -1; j <= 1; j++) {
        if (i != 0 || j != 0)
          reveal_cell(row + i, col + j, game_over);
      }
    }
  }

  void auto_reveal(int row, int col, bool &game_over) {
    if (!grid[row][col].is_revealed || grid[row][col].is_mine)
      return;
    int num_flagged = 0;
    for (int i = -1; i <= 1; i++) {
      for (int j = -1; j <= 1; j++) {
        int new_row = row + i;
        int new_col = col + j;
        if (new_row >= 0 && new_row < grid_rows &&
            new_col >= 0 && new_col < grid_cols) {
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
          if (new_row >= 0 && new_row < grid_rows &&
              new_col >= 0 && new_col < grid_cols) {
            if (!grid[new_row][new_col].is_flagged &&
                !grid[new_row][new_col].is_revealed)
              reveal_cell(new_row, new_col, game_over);
          }
        }
      }
    }
  }

  void reset_game() {
    grid.assign(grid_rows, std::vector<Cell>(grid_cols));
    place_mines();
    calc_adj_mines();
    is_game_over = false;
    is_game_won = false;
    start_time = GetTime();
    final_time = 0.0;
    paused_duration = 0.0;
    is_first_click = true;
  }

  void update() {
    Vector2 mouse_pos = GetMousePosition();
    if (is_confirm_reset) {
      if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (CheckCollisionPointRec(mouse_pos, yes_btn)) {
          reset_game();
          final_time = 0.0;
          is_confirm_reset = false;
        } else if (CheckCollisionPointRec(mouse_pos, no_btn)) {
          paused_duration += GetTime() - confirm_start_time;
          is_confirm_reset = false;
        }
      }
    } else {
      if (!is_game_over && !is_game_won) {
        // LMB (clear cell)
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
          if (CheckCollisionPointRec(mouse_pos, new_game_btn)) {
            is_confirm_reset = true;
            confirm_start_time = GetTime();
          } else {
            if (mouse_pos.y > ui_height) {
              int grid_x = mouse_pos.x;
              int grid_y = mouse_pos.y - ui_height;
              int cell_col = grid_x / cell_size;
              int cell_row = grid_y / cell_size;
              if (cell_row >= 0 && cell_row < grid_rows &&
                  cell_col >= 0 && cell_col < grid_cols) {
                if (is_first_click) {
                  first_click_safe_zone(cell_row, cell_col);
                  is_first_click = false;
                }
                if (!grid[cell_row][cell_col].is_flagged)
                  reveal_cell(cell_row, cell_col, is_game_over);
              }
            }
          }
        }

        // RMB (flag)
        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
          if (mouse_pos.y > ui_height) {
            int grid_x = mouse_pos.x;
            int grid_y = mouse_pos.y - ui_height;
            int cell_col = grid_x / cell_size;
            int cell_row = grid_y / cell_size;
            if (cell_row >= 0 && cell_row < grid_rows &&
                cell_col >= 0 && cell_col < grid_cols) {
              if (!grid[cell_row][cell_col].is_revealed)
                grid[cell_row][cell_col].is_flagged = !grid[cell_row][cell_col].is_flagged;
            }
          }
        }

        // MMB (auto reveal)
        if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
          if (mouse_pos.y > ui_height) {
            int grid_x = mouse_pos.x;
            int grid_y = mouse_pos.y - ui_height;
            int cell_col = grid_x / cell_size;
            int cell_row = grid_y / cell_size;
            if (cell_row >= 0 && cell_row < grid_rows &&
                cell_col >= 0 && cell_col < grid_cols) {
              if (grid[cell_row][cell_col].is_revealed &&
                  grid[cell_row][cell_col].adjacent_mines > 0)
                auto_reveal(cell_row, cell_col, is_game_over);
            }
          }
        }
      } else {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
          if (CheckCollisionPointRec(mouse_pos, new_game_btn)) {
            reset_game();
            final_time = 0.0;
          }
        }
      }
    }

    int revealed_count = 0;
    for (int row = 0; row < grid_rows; row++) {
      for (int col = 0; col < grid_cols; col++) {
        if (grid[row][col].is_revealed)
          revealed_count++;
      }
    }

    if (revealed_count == grid_rows * grid_cols - num_mines)
      is_game_won = true;

    if ((is_game_over || is_game_won) && final_time == 0.0)
      final_time = GetTime() - start_time - paused_duration;
  }

  void draw() {
    int screen_width = grid_cols * cell_size;
    int screen_height = grid_rows * cell_size + ui_height;

    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Draw UI elements
    DrawRectangleRec(new_game_btn, LIGHTGRAY);
    DrawRectangleLines(new_game_btn.x, new_game_btn.y, new_game_btn.width, new_game_btn.height, BLACK);
    DrawText("New Game", new_game_btn.x + 5, new_game_btn.y + 5, 20, BLACK);

    int elapsed = (int)((is_game_over || is_game_won) ? final_time : (GetTime() - start_time - paused_duration));
    std::string timer_text = "Time: " + std::to_string(elapsed) + "s";
    DrawText(timer_text.c_str(), new_game_btn.x + new_game_btn.width + 20, new_game_btn.y + 5, 20, BLACK);

    // Draw grid cells
    for (int row = 0; row < grid_rows; row++) {
      for (int col = 0; col < grid_cols; col++) {
        int x = col * cell_size;
        int y = row * cell_size + ui_height;
        Rectangle cell_rect = { (float)x, (float)y, (float)cell_size, (float)cell_size };

        if (grid[row][col].is_revealed) {
          if (grid[row][col].is_mine) {
            if (mine_texture.width > 0) {
              float scale = (float)cell_size / mine_texture.width;
              DrawTextureEx(mine_texture, { (float)x, (float)y }, 0.0f, scale, WHITE);
            } else {
              DrawText("M", x + cell_size / 4, y + cell_size / 4, 20, BLACK);
            }
          } else {
            DrawRectangleRec(cell_rect, LIGHTGRAY);
            if (grid[row][col].adjacent_mines > 0) {
              int number = grid[row][col].adjacent_mines;
              int font_size = 20;
              Color num_colour = num_colours[number];
              std::string num_str = TextFormat("%d", number);

              // Bold effect: we draw an offset text for outline then normal text
              DrawText(num_str.c_str(), x + cell_size / 3 + 1, y + cell_size / 4 + 1, font_size, num_colour);
              DrawText(num_str.c_str(), x + cell_size / 3, y + cell_size / 4, font_size, num_colour);
            }
          }
        } else {
          DrawRectangleRec(cell_rect, GRAY);
          if (grid[row][col].is_flagged) {
            if (flag_texture.width > 0) {
              float scale = (float)cell_size / flag_texture.width;
              DrawTextureEx(flag_texture, { (float)x, (float)y }, 0.0f, scale, WHITE);
            } else {
              DrawText("F", x + cell_size / 3, y + cell_size / 4, 20, MAROON);
            }
          }
        }
        DrawRectangleLines(x, y, cell_size, cell_size, BLACK);
      }
    }

    // Draw overlay on grid area if game ended (without covering UI)
    if (is_game_over || is_game_won) {
      int grid_area_y = ui_height;
      int grid_area_height = screen_height - ui_height;
      Color overlay_color = Fade(LIGHTGRAY, 0.8f);
      DrawRectangle(0, grid_area_y, screen_width, grid_area_height, overlay_color);
      const char* end_text = is_game_over ? "Game Over!" : "You Win!";
      int font_size = 40;
      int text_width = MeasureText(end_text, font_size);
      int text_x = screen_width / 2 - text_width / 2;
      int text_y = grid_area_y + grid_area_height / 2 - font_size / 2;
      DrawText(end_text, text_x, text_y, font_size, is_game_over ? RED : DARKGREEN);
    }

    if (is_confirm_reset) {
      DrawRectangle(0, 0, screen_width, screen_height, Fade(BLACK, 0.5f));
      DrawRectangleRec(modal_rect, LIGHTGRAY);
      DrawRectangleLines(modal_rect.x, modal_rect.y, modal_rect.width, modal_rect.height, BLACK);
      DrawText("Are you sure?", modal_rect.x + 60, modal_rect.y + 30, 20, BLACK);

      DrawRectangleRec(yes_btn, GREEN);
      DrawRectangleLines(yes_btn.x, yes_btn.y, yes_btn.width, yes_btn.height, BLACK);
      DrawText("Yes", yes_btn.x + 30, yes_btn.y + 5, 20, BLACK);

      DrawRectangleRec(no_btn, RED);
      DrawRectangleLines(no_btn.x, no_btn.y, no_btn.width, no_btn.height, BLACK);
      DrawText("No", no_btn.x + 35, no_btn.y + 5, 20, BLACK);
    }

    EndDrawing();
  }

  void run() {
    while (!WindowShouldClose()) {
      update();
      draw();
    }
  }
};

int main() {
  const int screen_width = 16 * 30;
  const int screen_height = 16 * 30 + 50;

  InitWindow(screen_width, screen_height, "Minesweeper");
  SetTargetFPS(60);

  srand(time(NULL));
  MinesweeperGame game;
  game.run();

  CloseWindow();
  return 0;
}
