#include <fstream>
#include <iostream>
#include <ncurses.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <thread>

#ifndef NULL
#define NULL nullptr
#endif

using json = nlohmann::json;

std::vector<std::vector<std::vector<int>>> candidateGrid;

int randomInt(int min, int max) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dis(min, max);
  return dis(gen);
}

#define CELL_WIDTH 8
#define CELL_HEIGHT 4

void drawGrid(WINDOW *win, const json &grid, const json &original,
              const json &solution) {
  int startY = 1, startX = 2;

  // draw the grid
  for (uint8_t i = 0; i < grid.size(); i++) {
    mvwaddch(win, startY + i * CELL_HEIGHT, startX, ACS_LTEE);
    for (uint8_t j = 0; j < grid.at(i).size(); j++) {
      for (uint8_t k = 0; k < CELL_HEIGHT; k++) {
        mvwaddch(win, startY + i * CELL_HEIGHT + k + 1, startX + j * CELL_WIDTH,
                 ACS_VLINE);
      }
      mvwhline(win, startY + i * CELL_HEIGHT, startX + j * CELL_WIDTH + 1,
               ACS_HLINE, CELL_WIDTH - 1);
      mvwaddch(win, startY + i * CELL_HEIGHT, startX + (j + 1) * CELL_WIDTH,
               (i == 0) ? ACS_TTEE : ACS_PLUS);
    }
    mvwaddch(win, startY + i * CELL_HEIGHT, startX + grid.size() * CELL_WIDTH,
             ACS_RTEE);
    for (uint8_t k = 0; k < CELL_HEIGHT; k++) {
      mvwaddch(win, startY + i * CELL_HEIGHT + k + 1,
               startX + grid.size() * CELL_WIDTH, ACS_VLINE);
    }
  }
  mvwaddch(win, startY, startX, ACS_ULCORNER);
  for (uint8_t i = 0; i < grid.at(0).size(); i++) {
    mvwhline(win, startY + 9 * CELL_HEIGHT, startX + 1 + i * CELL_WIDTH,
             ACS_HLINE, CELL_WIDTH - 1);
    mvwaddch(win, startY + 9 * CELL_HEIGHT, startX + (i + 1) * CELL_WIDTH,
             ACS_BTEE);
  }
  mvwaddch(win, startY + grid.size() * CELL_HEIGHT, startX, ACS_LLCORNER);
  mvwaddch(win, startY, startX + grid.size() * CELL_WIDTH, ACS_URCORNER);
  mvwaddch(win, startY + grid.size() * CELL_HEIGHT,
           startX + grid.size() * CELL_WIDTH, ACS_LRCORNER);

  // draw the values
  for (uint8_t i = 0; i < grid.size(); i++) {
    for (uint8_t j = 0; j < grid.at(i).size(); j++) {
      if (grid.at(i).at(j) > 0) {
        int colorPair = (grid.at(i).at(j) == original.at(i).at(j))   ? 1
                        : (grid.at(i).at(j) == solution.at(i).at(j)) ? 2
                                                                     : 3;
        wattron(win, COLOR_PAIR(colorPair));
        mvwprintw(win, startY + i * CELL_HEIGHT + 1 + (CELL_HEIGHT - 2) / 2,
                  startX + j * CELL_WIDTH + 1 + (CELL_WIDTH - 2) / 2, "%d",
                  grid.at(i).at(j).get<int>());
        wattroff(win, COLOR_PAIR(colorPair));
      }
    }
  }

  // draw the candidates
  std::map<int, std::pair<int, int>> candidateToOffset = {
      {1, {-3, -1}}, {2, {-2, -1}}, {3, {-1, -1}}, {4, {0, -1}}, {5, {1, -1}},
      {6, {2, -1}},  {7, {3, -1}},  {8, {-3, 0}},  {9, {-2, 0}},
  };

  json tmpGrid = grid;
  for (uint8_t i = 0; i < grid.size(); i++) {
    for (uint8_t j = 0; j < grid.at(i).size(); j++) {
      std::vector<int> candidates;

      for (int num : candidateGrid.at(i).at(j)) {
        candidates.push_back(num);
      }
      for (int num = 1; num <= 9; num++) {
        int xOffset = candidateToOffset.at(num).first;
        int yOffset = candidateToOffset.at(num).second;
        if (std::find(candidates.begin(), candidates.end(), num) !=
            candidates.end()) {
          wattron(win, COLOR_PAIR(4));
          mvwprintw(
              win,
              startY + i * CELL_HEIGHT + 1 + (CELL_HEIGHT - 2) / 2 + yOffset,
              startX + j * CELL_WIDTH + 1 + (CELL_WIDTH - 2) / 2 + xOffset,
              "%d", num);
          wattroff(win, COLOR_PAIR(4));
        } else {
          mvwprintw(
              win,
              startY + i * CELL_HEIGHT + 1 + (CELL_HEIGHT - 2) / 2 + yOffset,
              startX + j * CELL_WIDTH + 1 + (CELL_WIDTH - 2) / 2 + xOffset,
              " ");
        }
      }
    }
  }
}

bool isValidRow(int num, int row, json &grid) {
  std::vector<int> rowValues = grid.at(row);
  if (std::find(rowValues.begin(), rowValues.end(), num) != rowValues.end()) {
    return false;
  }
  return true;
}

bool isValidCol(int num, int col, json &grid) {
  std::vector<int> colValues;
  for (uint8_t i = 0; i < grid.size(); i++) {
    colValues.push_back(grid.at(i).at(col));
  }
  if (std::find(colValues.begin(), colValues.end(), num) != colValues.end()) {
    return false;
  }
  return true;
}

bool isValidSquare(int num, int row, int col, json &grid) {
  std::vector<int> squareValues;
  int squareRow = (row / 3) * 3;
  int squareCol = (col / 3) * 3;
  for (int i = squareRow; i < squareRow + 3; i++) {
    for (int j = squareCol; j < squareCol + 3; j++) {
      squareValues.push_back(grid.at(i).at(j));
    }
  }
  if (std::find(squareValues.begin(), squareValues.end(), num) !=
      squareValues.end()) {
    return false;
  }
  return true;
}

bool isValid(int num, int row, int col, json &grid) {
  return isValidRow(num, row, grid) && isValidCol(num, col, grid) &&
         isValidSquare(num, row, col, grid);
}

std::vector<int> findCandidates(int row, int col, json &grid) {
  std::vector<int> candidates;
  for (int num : candidateGrid.at(row).at(col)) {
    if (isValid(num, row, col, grid)) {
      candidates.push_back(num);
    }
  }
  return candidates;
}

bool removeCandidate(int num, int row, int col) {
  std::vector<int> &candidates = candidateGrid.at(row).at(col);
  for (uint8_t i = 0; i < candidates.size(); i++) {
    if (candidates.at(i) == num) {
      candidateGrid.at(row).at(col).erase(
          candidateGrid.at(row).at(col).begin() + i);
      return true;
    }
  }
  return false;
}

void removeRowCandidates(int num, int row) {
  for (uint8_t i = 0; i < candidateGrid.at(row).size(); i++) {
    removeCandidate(num, row, i);
  }
}

void removeColCandidates(int num, int col) {
  std::vector<int> rowCandidates;
  for (uint8_t i = 0; i < candidateGrid.size(); i++) {
    rowCandidates = candidateGrid.at(i).at(col);
    for (uint8_t j = 0; j < rowCandidates.size(); j++) {
      removeCandidate(num, i, col);
    }
  }
}

void removeSquareCandidates(int num, int row, int col) {
  int squareRow = (row / 3) * 3;
  int squareCol = (col / 3) * 3;
  for (int i = squareRow; i < squareRow + 3; i++) {
    for (int j = squareCol; j < squareCol + 3; j++) {
      removeCandidate(num, i, j);
    }
  }
}

void removeCandidates(int num, int row, int col) {
  removeRowCandidates(num, row);
  removeColCandidates(num, col);
  removeSquareCandidates(num, row, col);

  // remove all candidates for this cell
  candidateGrid.at(row).at(col).clear();
}

std::vector<std::tuple<int, int, int>> findAllNakedSingles(json &grid) {
  std::vector<std::tuple<int, int, int>> nakedSingles;
  for (uint8_t i = 0; i < grid.size(); i++) {
    for (uint8_t j = 0; j < grid.at(i).size(); j++) {
      if (grid.at(i).at(j) == 0) {
        std::vector<int> candidates = findCandidates(i, j, grid);
        if (candidates.size() == 1) {
          nakedSingles.push_back(std::make_tuple(i, j, candidates.at(0)));
        }
      }
    }
  }
  return nakedSingles;
}

std::vector<std::tuple<int, int, int>> findAllHiddenSingles(json &grid) {
  std::vector<std::tuple<int, int, int>> hiddenSingles;

  auto isHiddenSingleInUnit =
      [&](int num, const std::vector<std::pair<int, int>> &unit) {
        std::optional<std::pair<int, int>> pos;
        uint8_t count = 0;
        for (const auto &[row, col] : unit) {
          if (grid.at(row).at(col) == 0 && isValid(num, row, col, grid)) {
            count++;
            if (count > 1)
              break;
            pos = {{row, col}};
          }
        }
        return count == 1 ? pos : std::nullopt;
      };

  // Check rows
  for (uint8_t row = 0; row < 9; row++) {
    std::vector<std::pair<int, int>> rowCells;
    for (uint8_t col = 0; col < 9; col++)
      rowCells.push_back({row, col});

    for (int num = 1; num <= 9; num++) {
      if (auto pos = isHiddenSingleInUnit(num, rowCells)) {
        hiddenSingles.push_back({pos->first, pos->second, num});
      }
    }
  }

  // Check columns
  for (uint8_t col = 0; col < 9; col++) {
    std::vector<std::pair<int, int>> colCells;
    for (uint8_t row = 0; row < 9; row++)
      colCells.push_back({row, col});

    for (int num = 1; num <= 9; num++) {
      if (auto pos = isHiddenSingleInUnit(num, colCells)) {
        hiddenSingles.push_back({pos->first, pos->second, num});
      }
    }
  }

  // Check 3x3 squares
  for (uint8_t squareRow = 0; squareRow < 3; squareRow++) {
    for (uint8_t squareCol = 0; squareCol < 3; squareCol++) {
      std::vector<std::pair<int, int>> squareCells;
      for (uint8_t i = 0; i < 3; i++) {
        for (uint8_t j = 0; j < 3; j++) {
          squareCells.push_back({squareRow * 3 + i, squareCol * 3 + j});
        }
      }

      for (int num = 1; num <= 9; num++) {
        if (auto pos = isHiddenSingleInUnit(num, squareCells)) {
          hiddenSingles.push_back({pos->first, pos->second, num});
        }
      }
    }
  }

  return hiddenSingles;
}

bool applyPointingPairs() {
  bool changed = false;
  for (int boxRow = 0; boxRow < 3; boxRow++) {
    for (int boxCol = 0; boxCol < 3; boxCol++) {
      for (int num = 1; num <= 9; num++) {
        std::vector<std::pair<int, int>> positions;

        for (int i = 0; i < 3; i++) {
          for (int j = 0; j < 3; j++) {
            int row = boxRow * 3 + i;
            int col = boxCol * 3 + j;
            auto &candidates = candidateGrid[row][col];
            if (std::find(candidates.begin(), candidates.end(), num) !=
                candidates.end()) {
              positions.emplace_back(row, col);
            }
          }
        }

        if (positions.size() == 2 || positions.size() == 3) {
          bool sameRow = true;
          bool sameCol = true;
          for (ssize_t i = 1; i < positions.size(); i++) {
            if (positions[i].first != positions[0].first)
              sameRow = false;
            if (positions[i].second != positions[0].second)
              sameCol = false;
          }

          if (sameRow) {
            int row = positions[0].first;
            for (int col = 0; col < 9; col++) {
              if (col / 3 != boxCol) {
                removeCandidate(num, row, col);
                changed = true;
              }
            }
          } else if (sameCol) {
            int col = positions[0].second;
            for (int row = 0; row < 9; row++) {
              if (row / 3 != boxRow) {
                removeCandidate(num, row, col);
                changed = true;
              }
            }
          }
        }
      }
    }
  }

  return changed;
}

bool reduceBoxLine(json &grid) {
  bool changed = false;
  for (int boxRow = 0; boxRow < 3; boxRow++) {
    for (int boxCol = 0; boxCol < 3; boxCol++) {
      for (int num = 1; num <= 9; num++) {
        std::vector<std::pair<int, int>> positions;
        for (int row = 0; row < 9; row++) {
          for (int col = 0; col < 9; col++) {
            if ((row / 3 == boxRow) && (col / 3 == boxCol)) {
              if (grid.at(row).at(col) == 0 && isValid(num, row, col, grid)) {
                positions.emplace_back(row, col);
              }
            }
          }
        }

        if (positions.size() == 2 || positions.size() == 3) {
          bool sameRow = true;
          bool sameCol = true;
          for (ssize_t i = 1; i < positions.size(); i++) {
            if (positions[i].first != positions[0].first)
              sameRow = false;
            if (positions[i].second != positions[0].second)
              sameCol = false;
          }

          if (sameRow) {
            int row = positions[0].first;
            for (int col = 0; col < 9; col++) {
              if (col / 3 != boxCol) {
                removeCandidate(num, row, col);
                changed = true;
              }
            }
          } else if (sameCol) {
            int col = positions[0].second;
            for (int row = 0; row < 9; row++) {
              if (row / 3 != boxRow) {
                removeCandidate(num, row, col);
                changed = true;
              }
            }
          }
        }
      }
    }
  }

  return changed;
}

std::pair<std::vector<int>, std::vector<int>> rowFormsXWing(int row1, int row2,
                                                            int num) {
  std::vector<int> row1Cells;
  std::vector<int> row2Cells;

  for (int col = 0; col < 9; col++) {
    if (std::find(candidateGrid.at(row1).at(col).begin(),
                  candidateGrid.at(row1).at(col).end(),
                  num) != candidateGrid.at(row1).at(col).end()) {
      row1Cells.push_back(col);
    }
    if (std::find(candidateGrid.at(row2).at(col).begin(),
                  candidateGrid.at(row2).at(col).end(),
                  num) != candidateGrid.at(row2).at(col).end()) {
      row2Cells.push_back(col);
    }
  }

  if (row1Cells.size() == 2 && row2Cells.size() == 2) {
    if (row1Cells[0] == row2Cells[0] || row1Cells[0] == row2Cells[1] ||
        row1Cells[1] == row2Cells[0] || row1Cells[1] == row2Cells[1]) {
      return std::make_pair(row1Cells, row2Cells);
    }
  }

  return std::make_pair(std::vector<int>(), std::vector<int>());
}

bool xWing() {
  bool changed = false;
  for (int num = 1; num <= 9; num++) {
    for (int row1 = 0; row1 < 8; row1++) {
      for (int row2 = row1 + 1; row2 < 9; row2++) {
        std::pair<std::vector<int>, std::vector<int>> forms =
            rowFormsXWing(row1, row2, num);
        if (forms.first.size() == 2 && forms.second.size() == 2) {
          changed = true;
          for (uint8_t i = 0; i < candidateGrid.at(row1).size(); i++) {
            if (std::find(forms.first.begin(), forms.first.end(), i) ==
                forms.first.end()) {
              removeCandidate(num, row1, i);
            }
            if (std::find(forms.second.begin(), forms.second.end(), i) ==
                forms.second.end()) {
              removeCandidate(num, row2, i);
            }
          }
        }
      }
    }
  }
  return changed;
}

std::tuple<std::vector<int>, std::vector<int>, std::vector<int>>
rowFormsSwordfish(int row1, int row2, int row3, int num) {
  std::vector<int> row1Cells, row2Cells, row3Cells;

  for (int col = 0; col < 9; col++) {
    if (std::find(candidateGrid[row1][col].begin(),
                  candidateGrid[row1][col].end(),
                  num) != candidateGrid[row1][col].end()) {
      row1Cells.push_back(col);
    }
    if (std::find(candidateGrid[row2][col].begin(),
                  candidateGrid[row2][col].end(),
                  num) != candidateGrid[row2][col].end()) {
      row2Cells.push_back(col);
    }
    if (std::find(candidateGrid[row3][col].begin(),
                  candidateGrid[row3][col].end(),
                  num) != candidateGrid[row3][col].end()) {
      row3Cells.push_back(col);
    }
  }

  if (row1Cells.size() <= 3 && row2Cells.size() <= 3 && row3Cells.size() <= 3) {
    std::set<int> uniqueCols;
    for (int col : row1Cells)
      uniqueCols.insert(col);
    for (int col : row2Cells)
      uniqueCols.insert(col);
    for (int col : row3Cells)
      uniqueCols.insert(col);

    if (uniqueCols.size() == 3) {
      return std::make_tuple(row1Cells, row2Cells, row3Cells);
    }
  }

  return std::make_tuple(std::vector<int>(), std::vector<int>(),
                         std::vector<int>());
}

std::tuple<std::vector<int>, std::vector<int>, std::vector<int>>
columnFormsSwordfish(int row1, int row2, int row3, int num) {
  std::vector<int> col1Cells, col2Cells, col3Cells;

  for (int row = 0; row < 9; row++) {
    if (std::find(candidateGrid[row][row1].begin(),
                  candidateGrid[row][row1].end(),
                  num) != candidateGrid[row][row1].end()) {
      col1Cells.push_back(row);
    }
    if (std::find(candidateGrid[row][row2].begin(),
                  candidateGrid[row][row2].end(),
                  num) != candidateGrid[row][row2].end()) {
      col2Cells.push_back(row);
    }
    if (std::find(candidateGrid[row][row3].begin(),
                  candidateGrid[row][row3].end(),
                  num) != candidateGrid[row][row3].end()) {
      col3Cells.push_back(row);
    }
  }

  if (col1Cells.size() <= 3 && col2Cells.size() <= 3 && col3Cells.size() <= 3) {
    std::set<int> uniqueRows;
    for (int row : col1Cells)
      uniqueRows.insert(row);
    for (int row : col2Cells)
      uniqueRows.insert(row);
    for (int row : col3Cells)
      uniqueRows.insert(row);

    if (uniqueRows.size() == 3) {
      return std::make_tuple(col1Cells, col2Cells, col3Cells);
    }
  }

  return std::make_tuple(std::vector<int>(), std::vector<int>(),
                         std::vector<int>());
}

bool swordfish() {
  bool changed = false;
  for (int num = 1; num <= 9; num++) {
    for (int row1 = 0; row1 < 7; row1++) {
      for (int row2 = row1 + 1; row2 < 8; row2++) {
        for (int row3 = row2 + 1; row3 < 9; row3++) {
          auto [cells1, cells2, cells3] =
              rowFormsSwordfish(row1, row2, row3, num);
          if (!cells1.empty() && !cells2.empty() && !cells3.empty()) {
            changed = true;
            std::set<int> swordfishCols;
            for (int col : cells1)
              swordfishCols.insert(col);
            for (int col : cells2)
              swordfishCols.insert(col);
            for (int col : cells3)
              swordfishCols.insert(col);

            for (int col : swordfishCols) {
              for (int row = 0; row < 9; row++) {
                if (row != row1 && row != row2 && row != row3) {
                  removeCandidate(num, row, col);
                }
              }
            }
          }
        }
      }
    }

    for (int col1 = 0; col1 < 7; col1++) {
      for (int col2 = col1 + 1; col2 < 8; col2++) {
        for (int col3 = col2 + 1; col3 < 9; col3++) {
          auto [cells1, cells2, cells3] =
              columnFormsSwordfish(col1, col2, col3, num);
          if (!cells1.empty() && !cells2.empty() && !cells3.empty()) {
            changed = true;
            std::set<int> swordfishRows;
            for (int row : cells1)
              swordfishRows.insert(row);
            for (int row : cells2)
              swordfishRows.insert(row);
            for (int row : cells3)
              swordfishRows.insert(row);

            for (int row : swordfishRows) {
              for (int col = 0; col < 9; col++) {
                if (col != col1 && col != col2 && col != col3) {
                  removeCandidate(num, row, col);
                }
              }
            }
          }
        }
      }
    }
  }
  return changed;
}

void solveStep(json &grid) {
  std::vector<std::tuple<int, int, int>> nakedSingles =
      findAllNakedSingles(grid);
  if (nakedSingles.size() > 0) {
    for (std::tuple<int, int, int> nakedSingle : nakedSingles) {
      int row = std::get<0>(nakedSingle);
      int col = std::get<1>(nakedSingle);
      int num = std::get<2>(nakedSingle);
      grid.at(row).at(col) = num;
      removeCandidates(num, row, col);
    }
  } else {
    std::vector<std::tuple<int, int, int>> hiddenSingles =
        findAllHiddenSingles(grid);
    if (hiddenSingles.size() > 0) {
      for (std::tuple<int, int, int> hiddenSingle : hiddenSingles) {
        int row = std::get<0>(hiddenSingle);
        int col = std::get<1>(hiddenSingle);
        int num = std::get<2>(hiddenSingle);
        grid.at(row).at(col) = num;
        removeCandidates(num, row, col);
      }
    } else {
      if (!applyPointingPairs()) {
        if (!reduceBoxLine(grid)) {
          if (!swordfish()) {
            if (!xWing()) {
              return;
            }
          }
        }
      }
    }
  }
}

int main() {
  std::ifstream in("boards.json");
  const json boards = json::parse(in);
  in.close();

  const int randInt = randomInt(0, boards.size() - 1);
  const json &board = boards.at(randInt);
  const std::string difficulty = board.at("difficulty");
  const json &solution = board.at("solution");
  const json &original = board.at("value");
  json grid = board.at("value");

  for (int i = 0; i < 9; i++) {
    candidateGrid.push_back({});
    for (int j = 0; j < 9; j++) {
      candidateGrid[i].push_back({});
      for (int num = 1; num <= 9; num++) {
        if (isValid(num, i, j, grid) && grid.at(i).at(j) == 0) {
          candidateGrid[i][j].push_back(num);
        }
      }
    }
  }

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  start_color();
  nodelay(stdscr, TRUE);
  init_pair(1, COLOR_WHITE, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_RED, COLOR_BLACK);
  init_pair(4, COLOR_BLUE, COLOR_BLACK);

  curs_set(0);

  int maxY, maxX;
  getmaxyx(stdscr, maxY, maxX);

  int winHeight = 9 * CELL_HEIGHT + 3;
  int winWidth = 9 * CELL_WIDTH + 3 + 2;
  int startY = (maxY - winHeight) / 2;
  int startX = (maxX - winWidth) / 2;

  WINDOW *sudokuWin = newwin(winHeight, winWidth, startY, startX);
  mvwprintw(sudokuWin, 0, (winWidth - difficulty.length()) / 2, "");

  while (1) {
    drawGrid(sudokuWin, grid, original, solution);
    refresh();
    wrefresh(sudokuWin);
    char c = getch();
    if (c == 'q') {
      break;
    }

    solveStep(grid);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  endwin();

  return 0;
}
