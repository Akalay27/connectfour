// Adam Kalayjian 11/19/20

// This is a connect-four AI that uses the negamax algorithm with alpha-beta pruning and a transposition table.
// For each new position, the program generates all possible outcomes (to a certain depth) or reads previously generated outcomes from the transposition table.
// It then evaluates the value of each outcome based on a basic heuristic function and the ability to win the game.
// It uses the negamax algorithm to choose the best scores from each node (the opponents scores being negative) to decide on the next move to play.
// Pretty difficult to beat, I think there are still bugs with deciding the value of a game with a draw, although I can't confirm it.
// Let me know if the clear command is making new tabs on your terminal because that shouldn't be happening.

// additional improvements based on Jonathan Schaeffe's paper The History Heuristic and Alpha-Beta Search Enhancements in Practice
// http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.56.9124&rep=rep1&type=pdf
// and https://en.wikipedia.org/wiki/Negamax
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <inttypes.h>

#define WIDTH 7
#define HEIGHT 6
#define INTMAX 30000
#define TABLESIZE 256 // megabytes

typedef struct {
  int board[WIDTH][HEIGHT];
  int heights[WIDTH];
  int moves;
  int guessValue;
  int placedColumn;
} position;

enum flagTypes {INVALID, EXACT, LOWERBOUND, UPPERBOUND};

typedef struct {
  uint64_t key: 56;
  int depth: 6;
  enum flagTypes flag: 2;
  int value;
} ttEntry;

typedef struct {
  ttEntry * table;
  unsigned int size;
} transpositionTable;

// displays board with colors and move of players at the top
void showPosition(position *p, int lastMove, int gameFinished);
// returns 1 if the selected column is not full
int canPlay(position *p, int col);
// adds one game piece to the selected column
int play(position *p, int col);
// generates next possible positions for a given position
void generatePositions(position arr[], position *p);
// finds all lines of a certain color, horizontal, vertical, and diagonal
int findLines(position *p, int lineLength, int x, int y, int player, int includeSpace);
// recursive negamax algorithm, using alpha-beta pruning and transposition table. If root node, returns next move
int negamax2(position *p, int depth, int maxdepth, int alpha, int beta, int player, int sort, transpositionTable tt);
// sorts position array based on guessValue member
void sortPositions(position positions[]);
// calculates "value" of a certain position based on four in a rows and 3 in a rows, could be replaced with NN
int heuristicFunction(position *p, int player);
// checks whether (x,y) is in the board bounds
int inBounds(int x, int y);
// creates 56 bit key for transposition table from a given position. concept from http://blog.gamesolver.org/solving-connect-four/06-bitboard/
uint64_t ttKey(position * p);
// prints binary from uint64_t
void bin(uint64_t n);
// creates transposition table of a certain size and allocates memory
transpositionTable createTranspositionTable(size_t size);
// adds entry to transposition table
void putTT(ttEntry entry, transpositionTable tt);
// reads entry from transposition table based on position
ttEntry readTT(position * p, transpositionTable tt);
// writes transposition table to a binary file. Not currently used.
void writeTableToFile(char * filename, transpositionTable tt);
// reads transposition table from a binary file. Not currently used.
transpositionTable readTableFromFile(char * filename);
// returns amount of transposition table utilized from 0-1
double ttStatus(transpositionTable tt);
// clears console (should work on mac, windows, linux)
void clearscr();

int main(void) {
  transpositionTable tt = {0,0};
  //tt = readTableFromFile("testing.bin"); // issues with re-using tables between games so it just resets each time.
  int quit = 0;
  int searchdepth = 12;
  while (!quit) {
    int player = 0;
    int complete = 0;
    clearscr();
    printf("                      Connect Four\n");
    printf("Type 1 or 2 to start a game as the \033[01;33mfirst\033[0m or \033[01;31msecond\033[0m player.\n");
    printf("     D to change difficulty (currently %d). Q to quit.\n", searchdepth);
    char userChoice;
    scanf("\n%c",&userChoice);
    switch (userChoice) {
      case '1':
        player = 1;
        break;
      case '2':
        player = 0;
        break;
      case 'Q':
        complete = 1;
        quit = 1;
        break;
      case 'q':
        complete = 1;
        quit = 1;
        break;
      case 'D':
        printf("Type any number from 1-12 to select the difficulty.\n12 takes less than 5 seconds on my computer and is recommended,\ndifficulties greater than 12 may take awhile!\n");
        scanf("%d",&searchdepth);
        player = -1;
        break;
    }
    position pos = {0,0,0};
    if (!quit) {
      // Erasing transposition table for each round. Not ideal.
      // Still some issues with using the same transposition table when on opposing sides.
      printf("Creating transposition table... ");
      tt = createTranspositionTable(TABLESIZE*1049000);
      printf("done\n");
      showPosition(&pos,-1,complete);
    }
    // game loop
    while (complete == 0 && player != -1) {
      if (pos.moves%2 == 0) {
        printf("\033[01;33mYellow's Turn\033[0m\n");
      } else {
        printf("\033[01;31mRed's Turn\033[0m\n");
      }
      if (player%2 == 0) {
        // computer using negamax algorithm and playing
        printf("Computer is thinking...\n");
        int cpuMove = negamax2(&pos,searchdepth, searchdepth, -INTMAX, INTMAX,-(((pos.moves % 2)*2) - 1), 1, tt);
        play(&pos, cpuMove);
        showPosition(&pos, cpuMove,complete);
        player++;
      } else {
        // user input and playing
        printf("Type 0 through 6 to place a piece in that column.\n");
        int playerMove;
        scanf("%d",&playerMove);
        if (playerMove >= 0 && playerMove <= 6) {
          if (canPlay(&pos,playerMove)) {
            play(&pos,playerMove);
            showPosition(&pos,playerMove,complete);
            player++;
          } else {
            printf("Column %d is full!\n", playerMove);
          }
        } else {
          printf("Invalid column!\n");
        }
      }
      // find lines of four for win condition
      for (int x = 0; x < WIDTH; x++)  {
        for (int y = 0; y < HEIGHT; y++) {
          if (findLines(&pos, 4, x, y, 1, 1) || findLines(&pos, 4, x, y, 2, 1)) {
            complete = 1;
          }
        }
      }
      // or if the game has ended.
      if (pos.moves >= WIDTH * HEIGHT) {
        complete = 1;
      }
    }
    // show highlighted win if game is over
    if (!quit && complete) {
      showPosition(&pos,-1,complete);
      if (pos.moves%2 == 1) {
        printf("\033[01;33mYellow WINS\033[0m\n");
      } else {
        printf("\033[01;31mRed WINS\033[0m\n");
      }
      printf("\033[0mType M to return to menu, Q to quit.\n");
      scanf("\n%c",&userChoice);
      if (userChoice == 'Q' || userChoice == 'q') {
        quit = 1;
      }
    }
    // making sure to free the 100s of megabytes in ram!
    free(tt.table);
  }
  // writeTableToFile("testing.bin",tt); // still testing this
}

void showPosition(position *p, int lastMove, int gameFinished) {
  // show line above board with last played move
  clearscr();
  if (lastMove >= 0) {
    for (int s = 0; s < lastMove; s++)
      printf("    ");
    if (p->moves%2) {
      printf("\033[01;33m__");
    } else {
      printf("\033[01;31m__");
    }
  }
  // show board
  printf("\n");
  for (int y = HEIGHT-1; y >= 0; y--) {
    for (int x = 0; x < WIDTH; x++) {
      int value = p->board[x][y];
      if (!gameFinished || ((findLines(p, 4, x, y, 1, 1) || findLines(p, 4, x, y, 2, 1)) && gameFinished)) {
        switch (value) {
          case 0:
            printf("\033[0m .");
            break;
          case 1:
            printf("\033[01;33m%c%c",219,219);
            break;
          case 2:
            printf("\033[01;31m%c%c",219,219);
            break;
        }
      } else {
        switch (value) {
          case 0:
            printf("\033[0m .");
            break;
          case 1:
            printf("\033[0;33m%c%c",219,219);
            break;
          case 2:
            printf("\033[0;31m%c%c",219,219);
            break;
        }
      }
      printf("  ");

    }
    printf("\n\n");
  }
}

int getPlayerInput() {
  int playerMove = 0;
  scanf("%d",&playerMove);
}

int canPlay(position *p, int col) {
  return (p->heights[col] < HEIGHT);
}

int play(position *p, int col) {
  // put piece at lowest point in column, increase heights array
  if (col >= 0 && col <= WIDTH) {
    int player = p->moves % 2 + 1; // 1 or 2
    p->board[col][p->heights[col]++] = player;
    p->moves++;
  }
}

int findLines(position *p, int lineLength, int x, int y, int player, int includeSpace) {

  int totalLines = 0;
  if (includeSpace && p->board[x][y] != player) {
    return 0;
  }
  // check each direction, diagonals, verticals, horizontals and total consecutive pieces in each direction
  int totals[8] = {0};
  int i = 0;
  for (int xO = -1; xO <= 1; xO+=1) {
    for (int yO = -1; yO <= 1; yO+=1) {
      if (xO == 0 && yO == 0) continue;
      int d = 1;
      while (inBounds(x+xO*d,y+yO*d) && p->board[x+xO*d][y+yO*d] == player && d < lineLength) {
        totals[i]++;
        d++;
      }
      i+=1;
    }
  }
  // if the opposite totals match or exceed the line length then add to final total
  for (int i = 0; i < 4; i++) {
    if (totals[i] + totals[7-i] >= lineLength-1) {
      totalLines++;
    }
  }

  return totalLines;
}

int heuristicFunction(position *p, int player) {
  int checkTile;
  if (p->moves >= WIDTH*HEIGHT) return 0;
  if (player == -1) {
    checkTile = 2;
  } else {
    checkTile = 1;
  }
  int total = 0;
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      // if a four line is found, return max/min
      if (findLines(p, 4, x, y, checkTile, 1)) {
        return INTMAX * player;
      }
      // otherwise return based on number of threes.
      total += findLines(p, 3, x, y, checkTile, 1);
    }
  }
  return total*player;
}

int negamax2(position *p, int depth, int maxdepth, int alpha, int beta, int player, int sort, transpositionTable tt) {
  // keep track of original alpha for tt reads
  int originalAlpha = alpha;
  ttEntry entry = readTT(p, tt);

  // if valid tt entry, then use it for either a/b parameters or value of node
  if (entry.flag != INVALID && entry.depth >= depth && depth != maxdepth) {
    if (entry.flag == EXACT) return entry.value;
    else if (entry.flag == LOWERBOUND) alpha = alpha > entry.value ? alpha : entry.value;
    else if (entry.flag == UPPERBOUND) beta = beta < entry.value ? beta : entry.value;
    if (alpha >= beta) return entry.value;
  }

  // if this node is a "leaf" node (no more branches past this one)
  int heuristicValue = heuristicFunction(p,player);
  if (depth == 0 || heuristicValue >= INTMAX || heuristicValue <= -INTMAX || p->moves >= WIDTH*HEIGHT) { // TODO: add time limit to this return or something
    return player * heuristicValue;
  }

  int value = -INTMAX;
  int bestPlay = -1;

  // generate next possible moves, sort them randomly.
  position positions[WIDTH];
  generatePositions(positions,p);
  sortPositions(positions);

  // for each position, run negamax
  for (int i = 0; i < WIDTH; i++) {
    if (positions[i].moves != -1) {
      if (bestPlay == -1) bestPlay = positions[i].placedColumn;

      int childValue = -negamax2(&positions[i],depth-1,maxdepth, -beta, -alpha, -player, 1, tt);
      if (value < childValue) {
        value = childValue;
        // if this is the root node then keep track of what the final move was
        if (depth == maxdepth) {
          bestPlay = positions[i].placedColumn;
        }
      }
      // alpha beta pruning
      alpha = value > alpha ? value : alpha;
      if (alpha >= beta) {
        break;
      }
    }
  }
  // if root node
  if (depth == maxdepth) {
    return bestPlay;
  } else {
    // put this node in tt
    entry.value = value;
    entry.key = ttKey(p);
    if (value <= originalAlpha) entry.flag = UPPERBOUND;
    else if (value >= beta) entry.flag = LOWERBOUND;
    else entry.flag = EXACT;
    entry.depth = depth;
    putTT(entry,tt);
  }
  return value;
}

void generatePositions(position arr[], position *p) {
  for (int i = 0; i < WIDTH; i++) {
    if (canPlay(p, i)) {
      arr[i] = *p;
      play(&arr[i], i);
      arr[i].placedColumn = i;
      arr[i].guessValue = rand()*20;
    } else {
      arr[i] = *p;
      arr[i].moves = -1;
    }
  }
}

void sortPositions(position positions[]) {
  for (int i = 0; i < WIDTH; i++) {
    for (int x = 0; x < WIDTH-1; x++) {
      position *a = &positions[x];
      position *b = &positions[x+1];
      if (a->moves == -1 || (b->guessValue > a->guessValue && b->moves != -1)) {
        position temp = *a;
        positions[x] = *b;
        positions[x+1] = temp;
      }
    }
  }
}

int inBounds(int x, int y) {
  return (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT);
}

uint64_t ttKey(position * p) {
  uint64_t result = 0;
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      int space = y+x*HEIGHT;
      if (p->board[x][y] == 0) {
        result |= (1ULL << (space+1));
        break;
      } else {
        result |= (p->board[x][y]-1) << space;
      }
    }
  }
  return result;
}

void bin(uint64_t n) {
  uint64_t i;
  for (i = 1ULL << 63; i > 0; i = i / 2)
  (n & i) ? printf("1") : printf("0");
}

transpositionTable createTranspositionTable(size_t size) {
  // using the transpositionTable struct to keep track of size
  // since there could be an imported tt or a generated one with different sizes.
  ttEntry * table = (ttEntry *)malloc(size);
  ttEntry blank = {0,0,0,0};
  for (int i = 0; i < TABLESIZE; i++) {
    table[i] = blank;
  }
  transpositionTable result = {table,(size)/sizeof(ttEntry)};
  return result;
}

transpositionTable readTableFromFile(char * filename) {
  FILE * f;
  if ((f = fopen(filename,"r")) != NULL) {
    // getting size of file
    fseek(f, 0L, SEEK_END);
    size_t size = ftell(f);
    rewind(f);
    printf("Found transposition table with size of %d.\n",size/sizeof(ttEntry));
    transpositionTable result = createTranspositionTable(size);
    fread(result.table,sizeof(ttEntry),result.size, f);
    printf("Loaded transposition table from file!");
    fclose(f);
    return result;
  }
  printf("Could not find transposition table file.\n");
  transpositionTable blank = {0,0};
  return blank;
}

void writeTableToFile(char * filename, transpositionTable tt) {
  FILE * f = fopen(filename,"w");
  int test[5] = {1,2,3,4,5};
  fwrite(tt.table,sizeof(ttEntry),tt.size,f);
  fclose(f);
  printf("Wrote TT to file. %d",(tt.size)/sizeof(ttEntry));
}

double ttStatus(transpositionTable tt) {
  unsigned int total;
  for (int i = 0; i < tt.size; i++) {
    if (tt.table[i].flag != INVALID) total++;
  }
  return (double)total/tt.size;
}

void putTT(ttEntry entry, transpositionTable tt) {
  unsigned int index = entry.key%tt.size;

  tt.table[index] = entry;
}

ttEntry readTT(position * p, transpositionTable tt) {
  uint64_t key = ttKey(p);
  unsigned int index = key%tt.size;

  if (tt.table[index].key == key) {
    return tt.table[index];
  }
  ttEntry doesNotExist = {0,0,0,0};
  return doesNotExist;
}

void createPositionFromString(position * p, char * str) {
  // used for debugging
  int i = 0;
  for (int x = 0; x < WIDTH; x++) {
    for (int y = 0; y < HEIGHT; y++) {
      if (*(str+i) != '\0') {
        p->board[x][y] = *(str+i) - '0';
        if (*(str+i) != '0') {
          p->heights[x]++;
        }
        i++;
        p->moves++;

      } else {
        x = WIDTH;
        y = HEIGHT;
        break;
      }
    }
  }
}

void clearscr(void) {
  #ifdef _WIN32
      system("cls");
  #elif defined(unix) || defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
      system("clear");
  #else
      #error "OS not supported."
  #endif
}
