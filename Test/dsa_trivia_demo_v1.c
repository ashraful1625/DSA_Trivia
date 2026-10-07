/*
 * ============================================================
 *        DSA TRIVIA CHALLENGE - Group Project Prototype
 *        Language  : C
 *        Concepts  : Bubble Sort, Quick Sort, Linear Search,
 *                    Structs, Arrays, Game Loop
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
    #define CLEAR "cls"
#else
    #define CLEAR "clear"
#endif

/* ==================== CONSTANTS ==================== */
#define MAX_PLAYERS   10
#define MAX_QUESTIONS 20
#define MAX_NAME      50

/* ==================== STRUCTURES ==================== */
typedef struct {
    char name[MAX_NAME];
    int  score;
} Player;

typedef struct {
    char question[250];
    char options[4][100];
    int  correct;   /* 0 = A, 1 = B, 2 = C, 3 = D */
    int  points;
} Question;

/* ==================== GLOBAL DATA ==================== */
Player  players[MAX_PLAYERS];
int     playerCount = 0;

Question questions[MAX_QUESTIONS] = {
    {"What is the worst-case time complexity of Bubble Sort?",
        {"O(n)", "O(n log n)", "O(n^2)", "O(log n)"}, 2, 10},

    {"Which data structure uses LIFO principle?",
        {"Queue", "Stack", "Array", "Graph"}, 1, 10},

    {"What is the average time complexity of Quick Sort?",
        {"O(n^2)", "O(n)", "O(n log n)", "O(log n)"}, 2, 10},

    {"Which algorithm uses Divide and Conquer strategy?",
        {"Bubble Sort", "Selection Sort", "Merge Sort", "Insertion Sort"}, 2, 10},

    {"What is the time complexity of Binary Search?",
        {"O(n)", "O(n^2)", "O(n log n)", "O(log n)"}, 3, 10},

    {"Which data structure is used in BFS traversal?",
        {"Stack", "Queue", "Tree", "Heap"}, 1, 10},

    {"Which data structure is used in DFS traversal?",
        {"Queue", "Stack", "Graph", "Linked List"}, 1, 10},

    {"What is the space complexity of Merge Sort?",
        {"O(1)", "O(log n)", "O(n)", "O(n^2)"}, 2, 10},

    {"Which sorting algorithm is considered stable?",
        {"Quick Sort", "Heap Sort", "Merge Sort", "Selection Sort"}, 2, 10},

    {"Stack overflow occurs when?",
        {"Stack is empty", "Stack exceeds its size limit", "Queue is full", "Array is empty"}, 1, 10},

    {"Array indexing in C starts from?",
        {"1", "-1", "0", "2"}, 2, 10},

    {"Which traversal visits the root node first?",
        {"Inorder", "Postorder", "Level Order", "Preorder"}, 3, 10},

    {"Best-case time complexity of Bubble Sort is?",
        {"O(n^2)", "O(n log n)", "O(n)", "O(1)"}, 2, 10},

    {"Which is NOT a linear data structure?",
        {"Array", "Queue", "Tree", "Stack"}, 2, 10},

    {"Worst-case time complexity of Quick Sort?",
        {"O(n log n)", "O(n)", "O(n^2)", "O(log n)"}, 2, 10},

    {"A linked list node contains?",
        {"Only data", "Only pointer", "Data and pointer", "Only index"}, 2, 10},

    {"Which data structure implements recursion internally?",
        {"Queue", "Stack", "Tree", "Graph"}, 1, 10},

    {"Heap Sort uses which data structure?",
        {"Queue", "Stack", "Priority Queue (Heap)", "Linked List"}, 2, 10},

    {"In a Min-Heap, the root contains?",
        {"Maximum element", "Middle element", "Minimum element", "Random element"}, 2, 10},

    {"What does DSA stand for?",
        {"Digital System Analysis", "Data Structure and Algorithm",
         "Data Science Application", "Dynamic System Array"}, 1, 10}
};

/* ==================== UTILITY FUNCTIONS ==================== */
void flushInput() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void pressEnter() {
    printf("\n  Press ENTER to continue...");
    flushInput();
}

void printLine() {
    printf("  ==================================================\n");
}

void printHeader(const char *title) {
    system(CLEAR);
    printLine();
    printf("  ||  %-44s||\n", title);
    printLine();
}

/* ==================== SORTING ALGORITHMS ==================== */

/* --- Bubble Sort (descending by score) --- */
void bubbleSort(Player arr[], int n) {
    Player temp;
    int i, j, swapped;
    for (i = 0; i < n - 1; i++) {
        swapped = 0;
        for (j = 0; j < n - i - 1; j++) {
            if (arr[j].score < arr[j + 1].score) {
                temp       = arr[j];
                arr[j]     = arr[j + 1];
                arr[j + 1] = temp;
                swapped    = 1;
            }
        }
        if (!swapped) break; /* optimisation: already sorted */
    }
}

/* --- Quick Sort (descending by score) --- */
int partitionArr(Player arr[], int low, int high) {
    int pivot = arr[high].score;
    int i     = low - 1;
    Player temp;

    for (int j = low; j < high; j++) {
        if (arr[j].score >= pivot) {
            i++;
            temp    = arr[i];
            arr[i]  = arr[j];
            arr[j]  = temp;
        }
    }
    temp        = arr[i + 1];
    arr[i + 1]  = arr[high];
    arr[high]   = temp;
    return i + 1;
}

void quickSort(Player arr[], int low, int high) {
    if (low < high) {
        int pi = partitionArr(arr, low, high);
        quickSort(arr, low,  pi - 1);
        quickSort(arr, pi + 1, high);
    }
}

/* --- Linear Search by name (case-insensitive) --- */
int findPlayer(Player arr[], int n, char *name) {
    char a[MAX_NAME], b[MAX_NAME];
    int k;
    for (int i = 0; i < n; i++) {
        strcpy(a, arr[i].name);
        strcpy(b, name);
        for (k = 0; a[k]; k++) a[k] = (char)tolower((unsigned char)a[k]);
        for (k = 0; b[k]; k++) b[k] = (char)tolower((unsigned char)b[k]);
        if (strcmp(a, b) == 0) return i;
    }
    return -1;
}

/* ==================== LEADERBOARD ==================== */
void showLeaderboard() {
    if (playerCount == 0) {
        printf("\n  No players registered yet!\n");
        pressEnter();
        return;
    }

    printHeader("LEADERBOARD");
    printf("\n  Choose sorting algorithm:\n\n");
    printf("    1. Bubble Sort  [O(n^2)]\n");
    printf("    2. Quick Sort   [O(n log n)]\n");
    printf("    3. Back to Menu\n");
    printf("\n  Choice: ");

    int choice;
    scanf("%d", &choice);
    flushInput();

    if (choice == 3) return;

    /* Work on a copy so original order is preserved */
    Player temp[MAX_PLAYERS];
    memcpy(temp, players, playerCount * sizeof(Player));

    if (choice == 1) {
        bubbleSort(temp, playerCount);
        printHeader("LEADERBOARD  [Bubble Sort]");
        printf("  Time Complexity: O(n^2)  |  Space: O(1)\n");
    } else {
        quickSort(temp, 0, playerCount - 1);
        printHeader("LEADERBOARD  [Quick Sort]");
        printf("  Time Complexity: O(n log n)  |  Space: O(log n)\n");
    }

    printf("\n");
    printLine();
    printf("  %-6s %-22s %-10s %s\n", "Rank", "Player Name", "Score", "Award");
    printLine();

    for (int i = 0; i < playerCount; i++) {
        char award[12] = "  -";
        if      (i == 0) strcpy(award, "  *** GOLD");
        else if (i == 1) strcpy(award, "  ** SILVER");
        else if (i == 2) strcpy(award, "  *  BRONZE");

        printf("  %-6d %-22s %-10d%s\n",
               i + 1, temp[i].name, temp[i].score, award);
    }
    printLine();
    pressEnter();
}

/* ==================== PLAYER SETUP ==================== */
void setupPlayers() {
    printHeader("PLAYER SETUP");

    printf("\n  How many players? (2 - %d): ", MAX_PLAYERS);
    scanf("%d", &playerCount);
    flushInput();

    if (playerCount < 2)          playerCount = 2;
    if (playerCount > MAX_PLAYERS) playerCount = MAX_PLAYERS;

    printf("\n");
    for (int i = 0; i < playerCount; i++) {
        printf("  Enter name for Player %d: ", i + 1);
        scanf("%49s", players[i].name);
        flushInput();
        players[i].score = 0;
    }

    printf("\n  %d players registered successfully!\n", playerCount);
    pressEnter();
}

/* ==================== GAME LOOP ==================== */
void startGame() {
    if (playerCount == 0) {
        printf("\n  Please setup players first! (Option 1)\n");
        pressEnter();
        return;
    }

    int numQ;
    printf("\n  Questions per player (1 - %d): ", MAX_QUESTIONS);
    scanf("%d", &numQ);
    flushInput();
    if (numQ < 1)           numQ = 1;
    if (numQ > MAX_QUESTIONS) numQ = MAX_QUESTIONS;

    srand((unsigned int)time(NULL));

    for (int p = 0; p < playerCount; p++) {
        printHeader("DSA TRIVIA CHALLENGE");
        printf("\n");
        printLine();
        printf("  Player : %-22s Score: %d pts\n",
               players[p].name, players[p].score);
        printLine();
        pressEnter();

        int used[MAX_QUESTIONS] = {0};
        int asked = 0, correct = 0;

        while (asked < numQ) {
            int qIdx = rand() % MAX_QUESTIONS;
            if (used[qIdx]) continue;
            used[qIdx] = 1;
            asked++;

            Question q = questions[qIdx];

            printf("\n  Q%d/%d:  %s\n\n", asked, numQ, q.question);
            printf("    A)  %s\n", q.options[0]);
            printf("    B)  %s\n", q.options[1]);
            printf("    C)  %s\n", q.options[2]);
            printf("    D)  %s\n\n", q.options[3]);
            printf("  Your answer (A/B/C/D): ");

            char ans;
            scanf(" %c", &ans);
            flushInput();
            ans = (char)toupper((unsigned char)ans);

            int ansIdx = ans - 'A';

            if (ansIdx >= 0 && ansIdx <= 3 && ansIdx == q.correct) {
                players[p].score += q.points;
                correct++;
                printf("\n  >> CORRECT!  +%d pts  |  Total: %d pts\n",
                       q.points, players[p].score);
            } else {
                printf("\n  >> WRONG!  Correct answer: %c)  %s\n",
                       'A' + q.correct, q.options[q.correct]);
            }
            printLine();
        }

        printf("\n  Round Summary for %s\n", players[p].name);
        printf("  Correct : %d / %d\n", correct, numQ);
        printf("  Score   : %d pts\n",  players[p].score);

        if (p < playerCount - 1) {
            printf("\n  Next up: %s\n", players[p + 1].name);
        }
        pressEnter();
    }

    printf("\n  All players finished! View the Leaderboard (Option 3).\n");
    pressEnter();
}

/* ==================== SEARCH PLAYER ==================== */
void searchPlayerMenu() {
    printHeader("SEARCH PLAYER");

    char name[MAX_NAME];
    printf("\n  Enter player name to search: ");
    scanf("%49s", name);
    flushInput();

    int idx = findPlayer(players, playerCount, name);

    printf("\n");
    printLine();
    if (idx != -1) {
        printf("  Player Found!\n");
        printf("  Name  :  %s\n",   players[idx].name);
        printf("  Score :  %d pts\n", players[idx].score);
    } else {
        printf("  Player \"%s\" not found.\n", name);
    }
    printLine();
    pressEnter();
}

/* ==================== RESET SCORES ==================== */
void resetScores() {
    for (int i = 0; i < playerCount; i++) players[i].score = 0;
    printf("\n  All scores have been reset to 0.\n");
    pressEnter();
}

/* ==================== MAIN MENU ==================== */
int main() {
    int choice;

    while (1) {
        printHeader("DSA TRIVIA CHALLENGE");
        printf("\n");
        printf("  1.  Setup Players\n");
        printf("  2.  Start Game\n");
        printf("  3.  View Leaderboard\n");
        printf("  4.  Search Player\n");
        printf("  5.  Reset Scores\n");
        printf("  6.  Exit\n");
        printf("\n");
        printLine();
        printf("  Choice: ");

        if (scanf("%d", &choice) != 1) {
            flushInput();
            continue;
        }
        flushInput();

        switch (choice) {
            case 1: setupPlayers();    break;
            case 2: startGame();       break;
            case 3: showLeaderboard(); break;
            case 4:
                if (playerCount == 0) {
                    printf("\n  No players yet! Use Option 1 first.\n");
                    pressEnter();
                } else {
                    searchPlayerMenu();
                }
                break;
            case 5:
                if (playerCount == 0) {
                    printf("\n  No players to reset.\n");
                    pressEnter();
                } else {
                    resetScores();
                }
                break;
            case 6:
                printHeader("GOODBYE!");
                printf("\n  Thank you for playing DSA Trivia Challenge!\n\n");
                return 0;
            default:
                printf("\n  Invalid choice! Please enter 1-6.\n");
                pressEnter();
        }
    }
    return 0;
}
