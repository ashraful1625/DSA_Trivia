/* ================================================================
 *  PROJECT   : DSA Trivia Challenge
 *  COURSE    : Data Structures and Algorithms (DSA)
 *  GROUP     : 05
 *  MEMBERS   : [Fill in names & registration numbers]
 *  LANGUAGE  : C (standard C11, no external libraries)
 *
 *  DESCRIPTION
 *  -----------
 *  A console-based multiplayer quiz game. Each player registers
 *  with Name / Student ID / Batch / Department, answers 10-20
 *  multiple-choice DSA questions, and receives a score using
 *  negative marking. Results are written to a per-player file,
 *  every player's score is stored in a shared file, and the
 *  leaderboard is produced using sorting algorithms implemented
 *  from scratch (no library sort functions are used).
 *
 *  DSA CONCEPTS DEMONSTRATED (for viva / explanation)
 *  ----------------------------------------------------
 *   1. Structs & Arrays        -> Player, Question, AnswerRecord
 *   2. Linear Search            -> findPlayerIndexByID()
 *   3. Fisher-Yates Shuffle     -> shuffleIndices()
 *   4. Bubble Sort  O(n^2)      -> bubbleSortByScore()
 *   5. Quick Sort   O(n log n)  -> quickSortByScore() / partitionByScore()
 *   6. File I/O (persistence)   -> loadAllPlayers() / appendPlayerScore()
 *
 *  HOW TO COMPILE & RUN
 *  ---------------------
 *      gcc dsa_trivia_full.c -o dsa_trivia_full
 *      ./dsa_trivia_full          (Linux/Mac)
 *      dsa_trivia_full.exe        (Windows)
 *
 *  FILES CREATED AT RUNTIME
 *  --------------------------
 *      players_scores.txt   -> append-only score log (all players)
 *      result_<StudentID>.txt -> one file per player, full Q&A + score
 *      leaderboard_report.txt -> human-readable sorted snapshot
 * ================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ================================================================
 *  SECTION 0 : CONFIGURATION CONSTANTS
 *  ----------------------------------------------------------------
 *  Keeping every "magic number" here means the whole program can
 *  be tuned (or extended) later by changing ONE line instead of
 *  hunting through the code. This is what makes the program
 *  "future upgradeable".
 * ================================================================ */
#define MAX_NAME_LEN      50     /* Max characters in a player's name        */
#define MAX_ID_LEN        15     /* Max characters in a Student ID           */
#define MAX_DEPT_LEN      10     /* Max characters in a Department code      */
#define MAX_QUESTION_LEN  250    /* Max characters in a question's text      */
#define MAX_OPTION_LEN    100    /* Max characters in one MCQ option         */

#define MAX_BANK_SIZE     200     /* Hard upper limit on the question bank    */
#define MIN_SESSION_Q     5     /* Minimum questions allowed per session    */
#define MAX_SESSION_Q     200     /* Maximum questions allowed per session    */
#define MAX_PLAYERS       200    /* Max records that can be loaded from file */

#define CORRECT_MARK      1.00f  /* Points awarded for a correct answer      */
#define WRONG_PENALTY     0.25f  /* Points deducted for a wrong answer       */

#define SCORES_FILE       "players_scores.txt"    /* shared score log       */
#define LEADERBOARD_FILE  "leaderboard_report.txt" /* pretty snapshot       */
#define FIELD_DELIM       "|"    /* delimiter used inside the scores file    */

#define APP_TITLE         "DSA TRIVIA CHALLENGE"

/* Cross-platform "clear screen" command */
#ifdef _WIN32
    #define CLEAR_SCREEN "cls"
#else
    #define CLEAR_SCREEN "clear"
#endif

/* ================================================================
 *  SECTION 1 : DATA STRUCTURES
 * ================================================================ */

/* Holds everything about one player / quiz-taker.
 * NOTE: 'rank' is NOT saved to file - it depends on the current
 * full leaderboard, so it is recalculated every time it's needed
 * (see assignRanks()). This avoids storing stale/incorrect data. */
typedef struct {
    char  name[MAX_NAME_LEN];
    char  studentID[MAX_ID_LEN];
    int   batch;
    char  deptCode[MAX_DEPT_LEN];
    float score;
    int   correct;
    int   wrong;
    int   skipped;
    int   totalQuestions;
    int   rank;                 /* filled in at runtime by assignRanks()   */
} Player;

/* One multiple-choice question. correctOption is 0-3 (A-D). */
typedef struct {
    char question[MAX_QUESTION_LEN];
    char options[4][MAX_OPTION_LEN];
    int  correctOption;
} Question;

/* Records what a player answered for ONE question during a session.
 * We store only the index + selection (not a copy of the question
 * text) so there is a single source of truth: the question bank.
 * selectedOption == -1 means the player skipped the question. */
typedef struct {
    int questionIndex;
    int selectedOption;
} AnswerRecord;

/* ================================================================
 *  SECTION 2 : FUNCTION PROTOTYPES
 *  (Grouped by module so the structure mirrors the design diagram
 *   submitted earlier: Registration / Quiz Engine / Scoring /
 *   Result Generator / Ranking-Sorting / File Manager / Utility)
 * ================================================================ */

/* ---- Utility Module ---- */
void  clearInputBuffer(void);
void  pauseScreen(void);
void  printDivider(char ch, int len);
void  printBanner(const char *title);
int   getValidatedInt(const char *prompt, int minVal, int maxVal);
int   findPlayerIndexByID(Player arr[], int n, const char *studentID);

/* ---- Question Bank Module ---- */
void  loadQuestionBank(Question bank[], int *count);

/* ---- File Manager Module ---- */
int   loadAllPlayers(Player arr[], int maxSize);
void  appendPlayerScore(const Player *p);

/* ---- Player Registration Module ---- */
void  registerPlayer(Player *p);
void  displayPlayerRecord(const char *studentID);

/* ---- Scoring Module ---- */
void  applyAnswerResult(Player *p, int isCorrect);

/* ---- Quiz Engine Module ---- */
void  shuffleIndices(int arr[], int n);
int   askSingleQuestion(const Question *q, int qNumber, int totalQ);
void  runQuizSession(Player *p, Question bank[], int bankSize,
                      AnswerRecord log[], int numQuestions);

/* ---- Result Generator Module ---- */
void  writeResultFile(const Player *p, Question bank[], AnswerRecord log[],
                       int numQuestions, int rank, int totalPlayers);
void  printResultSummary(const Player *p, int rank, int totalPlayers);

/* ---- Ranking / Sorting Module ---- */
void  bubbleSortByScore(Player arr[], int n);
int   partitionByScore(Player arr[], int low, int high);
void  quickSortByScore(Player arr[], int low, int high);
void  assignRanks(Player arr[], int n);
void  displayLeaderboard(Player arr[], int n, const char *algoUsed, double elapsedMs);
void  saveLeaderboardReport(Player arr[], int n, const char *algoUsed);

/* ---- Menu Handlers ---- */
void  showMainMenu(void);
void  handleTakeQuiz(Question bank[], int bankSize);
void  handleViewLeaderboard(void);
void  handleSearchPlayer(void);


/* ================================================================
 *  SECTION 3 : UTILITY MODULE
 * ================================================================ */

/*
 * Function : clearInputBuffer
 * ------------------------------------------------------------
 * Purpose  : Discards leftover characters (including the newline)
 *            in stdin after a scanf() call. Without this, a
 *            stray '\n' left in the buffer would be picked up
 *            by the NEXT fgets()/scanf() and cause skipped or
 *            corrupted input - a very common bug in C console
 *            programs.
 * Params   : none
 * Returns  : void
 */
void clearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { /* discard */ }
}

/*
 * Function : pauseScreen
 * ------------------------------------------------------------
 * Purpose  : Pauses execution until the user presses ENTER, so
 *            they have time to read the screen before it clears.
 * Params   : none
 * Returns  : void
 */
void pauseScreen(void) {
    printf("\nPress ENTER to continue...");
    clearInputBuffer();
}

/*
 * Function : printDivider
 * ------------------------------------------------------------
 * Purpose  : Prints a horizontal line of a chosen character -
 *            used everywhere to keep console output tidy and
 *            consistent without repeating the same printf calls.
 * Params   : ch  - the character to repeat (e.g. '=' or '-')
 *            len - how many times to repeat it
 * Returns  : void
 */
void printDivider(char ch, int len) {
    int i;
    for (i = 0; i < len; i++) putchar(ch);
    putchar('\n');
}

/*
 * Function : printBanner
 * ------------------------------------------------------------
 * Purpose  : Prints a titled section header, e.g. for switching
 *            between "PLAYER REGISTRATION" and "QUIZ STARTED".
 * Params   : title - text to display inside the banner
 * Returns  : void
 */
void printBanner(const char *title) {
    printDivider('=', 55);
    printf("   %s\n", title);
    printDivider('=', 55);
}

/*
 * Function : getValidatedInt
 * ------------------------------------------------------------
 * Purpose  : Repeatedly prompts the user until they enter an
 *            integer within [minVal, maxVal]. Centralising input
 *            validation here means every menu/answer prompt in
 *            the program is protected from crashes caused by
 *            non-numeric input (e.g. typing a letter).
 * Params   : prompt  - text shown to the user before reading input
 *            minVal  - smallest acceptable value (inclusive)
 *            maxVal  - largest acceptable value (inclusive)
 * Returns  : a valid integer chosen by the user
 */
int getValidatedInt(const char *prompt, int minVal, int maxVal) {
    int value, result;
    while (1) {
        printf("%s", prompt);
        result = scanf("%d", &value);
        clearInputBuffer();  /* always flush the rest of the line */

        if (result == 1 && value >= minVal && value <= maxVal) {
            return value;
        }
        printf("  >> Invalid input. Enter a number between %d and %d.\n",
               minVal, maxVal);
    }
}

/*
 * Function : findPlayerIndexByID
 * ------------------------------------------------------------
 * DSA CONCEPT : LINEAR SEARCH  ->  O(n)
 * ------------------------------------------------------------
 * Purpose  : Scans an array of Players from start to end,
 *            comparing each studentID until a match is found.
 *            This is the classic Linear Search algorithm - the
 *            simplest search technique, correct for both sorted
 *            and unsorted data (our scores file is not sorted
 *            on disk, so this is the appropriate choice here).
 * Params   : arr[]      - array of Player records to search
 *            n          - number of valid entries in arr[]
 *            studentID  - the ID being searched for
 * Returns  : index of the matching player, or -1 if not found
 */
int findPlayerIndexByID(Player arr[], int n, const char *studentID) {
    int i;
    for (i = 0; i < n; i++) {
        if (strcmp(arr[i].studentID, studentID) == 0) {
            return i;   /* found - stop early (best case O(1)) */
        }
    }
    return -1;          /* not found - had to check all n (worst case) */
}


/* ================================================================
 *  SECTION 4 : QUESTION BANK MODULE
 * ================================================================ */

/*
 * Function : loadQuestionBank
 * ------------------------------------------------------------
 * Purpose  : Populates the question bank with 20 hardcoded DSA
 *            multiple-choice questions.
 *
 * FUTURE UPGRADE: Replace this hardcoded array with code that
 * reads from an external "questions.txt" file using fgets/fscanf.
 * That would let non-programmer teammates add or edit questions
 * without touching the C source at all - a very natural next
 * step for this project.
 *
 * Params   : bank[]  - destination array to fill
 *            count   - output parameter, set to how many
 *                      questions were loaded (computed
 *                      automatically so nothing needs to be
 *                      updated by hand if questions are added)
 * Returns  : void (results passed back through the parameters)
 */
void loadQuestionBank(Question bank[], int *count) {
    Question temp[] = {
        {"What is the worst-case time complexity of Bubble Sort?",
            {"O(n)", "O(n log n)", "O(n^2)", "O(log n)"}, 2},

        {"Which data structure uses the LIFO principle?",
            {"Queue", "Stack", "Array", "Graph"}, 1},

        {"What is the average time complexity of Quick Sort?",
            {"O(n^2)", "O(n)", "O(n log n)", "O(log n)"}, 2},

        {"Which algorithm uses the Divide and Conquer strategy?",
            {"Bubble Sort", "Selection Sort", "Merge Sort", "Insertion Sort"}, 2},

        {"What is the time complexity of Binary Search?",
            {"O(n)", "O(n^2)", "O(n log n)", "O(log n)"}, 3},

        {"Which data structure is used in BFS traversal?",
            {"Stack", "Queue", "Tree", "Heap"}, 1},

        {"Which data structure is used in DFS traversal?",
            {"Queue", "Stack", "Graph", "Linked List"}, 1},

        {"What is the space complexity of Merge Sort?",
            {"O(1)", "O(log n)", "O(n)", "O(n^2)"}, 2},

        {"Which sorting algorithm is considered stable?",
            {"Quick Sort", "Heap Sort", "Merge Sort", "Selection Sort"}, 2},

        {"A Stack Overflow occurs when?",
            {"The stack is empty", "The stack exceeds its size limit",
             "The queue is full", "The array is empty"}, 1},

        {"Array indexing in C starts from?",
            {"1", "-1", "0", "2"}, 2},

        {"Which traversal visits the root node first?",
            {"Inorder", "Postorder", "Level Order", "Preorder"}, 3},

        {"What is the best-case time complexity of Bubble Sort?",
            {"O(n^2)", "O(n log n)", "O(n)", "O(1)"}, 2},

        {"Which of these is NOT a linear data structure?",
            {"Array", "Queue", "Tree", "Stack"}, 2},

        {"What is the worst-case time complexity of Quick Sort?",
            {"O(n log n)", "O(n)", "O(n^2)", "O(log n)"}, 2},

        {"A linked list node typically contains?",
            {"Only data", "Only a pointer", "Data and a pointer", "Only an index"}, 2},

        {"Which data structure implements recursion internally?",
            {"Queue", "Stack", "Tree", "Graph"}, 1},

        {"Heap Sort relies on which data structure?",
            {"Queue", "Stack", "Priority Queue (Heap)", "Linked List"}, 2},

        {"In a Min-Heap, the root always contains the?",
            {"Maximum element", "Middle element", "Minimum element", "A random element"}, 2},

        {"What does the acronym DSA stand for?",
            {"Digital System Analysis", "Data Structure and Algorithm",
             "Data Science Application", "Dynamic System Array"}, 1}
    };

    /* Computing the count from sizeof means adding/removing a
       question above never requires updating a count by hand. */
    int n = (int)(sizeof(temp) / sizeof(Question));
    int i;
    for (i = 0; i < n; i++) {
        bank[i] = temp[i];
    }
    *count = n;
}


/* ================================================================
 *  SECTION 5 : FILE MANAGER MODULE
 *  ----------------------------------------------------------------
 *  DESIGN NOTE: players_scores.txt is treated as an APPEND-ONLY
 *  log (like a tiny database table). We never rewrite or reorder
 *  it in place - a new quiz attempt only ever adds one new line.
 *  Sorting happens IN MEMORY every time the leaderboard is needed,
 *  and the sorted result is written to a *separate* file
 *  (leaderboard_report.txt) for human reading. This keeps the raw
 *  data file simple, safe from corruption, and easy to swap for a
 *  real database later without changing the rest of the program.
 * ================================================================ */

/*
 * Function : loadAllPlayers
 * ------------------------------------------------------------
 * Purpose  : Reads every line of players_scores.txt into an array
 *            of Player structs. Each line is stored using
 *            FIELD_DELIM ('|') between fields to safely support
 *            names that may contain spaces.
 * Params   : arr[]     - destination array
 *            maxSize   - capacity of arr[] (protects against overflow)
 * Returns  : number of player records successfully loaded
 */
int loadAllPlayers(Player arr[], int maxSize) {
    FILE *fp = fopen(SCORES_FILE, "r");
    if (fp == NULL) {
        return 0;   /* file doesn't exist yet -> first run, 0 players */
    }

    char line[400];
    int count = 0;

    while (count < maxSize && fgets(line, sizeof(line), fp) != NULL) {
        Player p;
        int matched = sscanf(line,
            "%49[^|]|%14[^|]|%d|%9[^|]|%f|%d|%d|%d|%d",
            p.name, p.studentID, &p.batch, p.deptCode,
            &p.score, &p.correct, &p.wrong, &p.skipped, &p.totalQuestions);

        /* Only accept lines that have all 9 expected fields.
           This protects the program from crashing on a blank
           line or a manually-edited/corrupted row. */
        if (matched == 9) {
            p.rank = 0;   /* recalculated later by assignRanks() */
            arr[count] = p;
            count++;
        }
    }

    fclose(fp);
    return count;
}

/*
 * Function : appendPlayerScore
 * ------------------------------------------------------------
 * Purpose  : Adds ONE new player record as a new line at the end
 *            of players_scores.txt. Uses "a" (append) mode so
 *            existing records are never touched or lost.
 * Params   : p - pointer to the Player record to save
 * Returns  : void
 */
void appendPlayerScore(const Player *p) {
    FILE *fp = fopen(SCORES_FILE, "a");
    if (fp == NULL) {
        printf("  >> ERROR: could not open %s for writing.\n", SCORES_FILE);
        return;
    }
    fprintf(fp, "%s|%s|%d|%s|%.2f|%d|%d|%d|%d\n",
            p->name, p->studentID, p->batch, p->deptCode,
            p->score, p->correct, p->wrong, p->skipped, p->totalQuestions);
    fclose(fp);
}


/* ================================================================
 *  SECTION 6 : PLAYER REGISTRATION MODULE
 * ================================================================ */

/*
 * Function : registerPlayer
 * ------------------------------------------------------------
 * Purpose  : Collects Name, Student ID, Batch, and Department
 *            from the console and initialises every score-related
 *            field to zero for a brand-new attempt.
 * Params   : p - pointer to the Player struct to fill in
 * Returns  : void
 */
void registerPlayer(Player *p) {
    size_t len, i;

    /* NOTE: no clearInputBuffer() call here on purpose. Every call to
       getValidatedInt() already flushes stdin right after its scanf(),
       so by the time we reach this function the input buffer is
       already clean and ready for fgets(). Calling clearInputBuffer()
       again here would incorrectly consume the player's name line
       while waiting for a leftover newline that no longer exists. */

    printf("Enter Full Name        : ");
    fgets(p->name, MAX_NAME_LEN, stdin);

    /* fgets() keeps the trailing newline - strip it off */
    len = strlen(p->name);
    if (len > 0 && p->name[len - 1] == '\n') {
        p->name[len - 1] = '\0';
    }
    /* Replace any '|' the user typed, since '|' is our file
       delimiter and would corrupt the score file otherwise. */
    for (i = 0; i < strlen(p->name); i++) {
        if (p->name[i] == '|') p->name[i] = ' ';
    }

    printf("Enter Student ID (Roll): ");
    scanf("%14s", p->studentID);

    p->batch = getValidatedInt("Enter Batch Number     : ", 200, 1000);

    printf("Enter Department Code  : ");
    scanf("%9s", p->deptCode);
    clearInputBuffer();

    /* Fresh attempt -> everything starts at zero */
    p->score          = 0.0f;
    p->correct         = 0;
    p->wrong           = 0;
    p->skipped         = 0;
    p->totalQuestions  = 0;
    p->rank            = 0;
}

/*
 * Function : displayPlayerRecord
 * ------------------------------------------------------------
 * Purpose  : Looks up ONE player by Student ID (using Linear
 *            Search), works out their current rank against every
 *            other player on file (using Quick Sort), and prints
 *            a summary. Shared by both the "duplicate ID" check
 *            and the "Search Player" menu option, so the lookup
 *            logic only needs to live in one place.
 * Params   : studentID - the ID to look up
 * Returns  : void
 */
void displayPlayerRecord(const char *studentID) {
    Player arr[MAX_PLAYERS];
    int n = loadAllPlayers(arr, MAX_PLAYERS);

    if (n > 0) {
        quickSortByScore(arr, 0, n - 1);
        assignRanks(arr, n);
    }

    int idx = findPlayerIndexByID(arr, n, studentID);
    if (idx == -1) {
        printf("  >> No record found for Student ID: %s\n", studentID);
        return;
    }

    printDivider('-', 45);
    printf("  Name        : %s\n", arr[idx].name);
    printf("  Student ID  : %s\n", arr[idx].studentID);
    printf("  Batch       : %d\n", arr[idx].batch);
    printf("  Department  : %s\n", arr[idx].deptCode);
    printf("  Correct     : %d\n", arr[idx].correct);
    printf("  Wrong       : %d\n", arr[idx].wrong);
    printf("  Skipped     : %d\n", arr[idx].skipped);
    printf("  Final Score : %.2f / %d\n", arr[idx].score, arr[idx].totalQuestions);
    printf("  Rank        : %d out of %d players\n", arr[idx].rank, n);
    printDivider('-', 45);
}


/* ================================================================
 *  SECTION 7 : SCORING MODULE
 * ================================================================ */

/*
 * Function : applyAnswerResult
 * ------------------------------------------------------------
 * Purpose  : Applies the marking scheme after ONE question:
 *              correct  -> +1.00 mark
 *              wrong    -> -0.25 mark (negative marking)
 *              skipped  -> no change
 *            The score is clamped at a minimum of 0.00 so a bad
 *            run of wrong answers never produces a negative
 *            final score.
 * Params   : p         - pointer to the player being scored
 *            isCorrect - 1 = correct, 0 = wrong, -1 = skipped
 * Returns  : void
 */
void applyAnswerResult(Player *p, int isCorrect) {
    if (isCorrect == 1) {
        p->correct++;
        p->score += CORRECT_MARK;
    } else if (isCorrect == 0) {
        p->wrong++;
        p->score -= WRONG_PENALTY;
        if (p->score < 0) p->score = 0.0f;   /* never go negative */
    } else {
        p->skipped++;   /* no mark change for a skipped question */
    }
}


/* ================================================================
 *  SECTION 8 : QUIZ ENGINE MODULE
 * ================================================================ */

/*
 * Function : shuffleIndices
 * ------------------------------------------------------------
 * DSA CONCEPT : FISHER-YATES SHUFFLE  ->  O(n)
 * ------------------------------------------------------------
 * Purpose  : Randomly reorders an array of question indices so
 *            each quiz session presents questions in a different
 *            order (and a subset can be taken from the front of
 *            the shuffled array with no repeats or bias).
 * Params   : arr[] - array to shuffle in place (expected to be
 *                     pre-filled with 0, 1, 2, ... n-1)
 *            n     - number of elements in arr[]
 * Returns  : void
 */
void shuffleIndices(int arr[], int n) {
    int i, j, temp;
    for (i = n - 1; i > 0; i--) {
        j = rand() % (i + 1);
        temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

/*
 * Function : askSingleQuestion
 * ------------------------------------------------------------
 * Purpose  : Displays one question with its four options and
 *            reads the player's validated choice.
 * Params   : q       - pointer to the question to display
 *            qNumber - this question's position in the session (1-based)
 *            totalQ  - total questions in this session (for "X of Y")
 * Returns  : 0-3 for options A-D, or -1 if the player skipped (entered 0)
 */
int askSingleQuestion(const Question *q, int qNumber, int totalQ) {
    int i, choice;

    printf("\nQuestion %d of %d:\n%s\n\n", qNumber, totalQ, q->question);
    for (i = 0; i < 4; i++) {
        printf("   %d. %s\n", i + 1, q->options[i]);
    }

    choice = getValidatedInt("\nYour answer (1-4, or 0 to skip): ", 0, 4);
    return (choice == 0) ? -1 : (choice - 1);
}

/*
 * Function : runQuizSession
 * ------------------------------------------------------------
 * Purpose  : Runs the full question loop for one player:
 *            shuffles the question bank, asks `numQuestions`
 *            unique questions one at a time, scores each answer,
 *            gives immediate feedback, and records everything
 *            into the log[] array for the result file later.
 * Params   : p             - the player currently taking the quiz
 *            bank[]        - the full question bank
 *            bankSize      - number of questions in bank[]
 *            log[]         - output array recording each answer
 *            numQuestions  - how many questions this session uses
 * Returns  : void
 */
void runQuizSession(Player *p, Question bank[], int bankSize,
                     AnswerRecord log[], int numQuestions) {
    int order[MAX_BANK_SIZE];
    int i, qIdx, sel, isCorrect;

    for (i = 0; i < bankSize; i++) order[i] = i;
    shuffleIndices(order, bankSize);

    for (i = 0; i < numQuestions; i++) {
        qIdx = order[i];
        sel  = askSingleQuestion(&bank[qIdx], i + 1, numQuestions);

        log[i].questionIndex  = qIdx;
        log[i].selectedOption = sel;

        if (sel == -1) {
            isCorrect = -1;
        } else if (sel == bank[qIdx].correctOption) {
            isCorrect = 1;
        } else {
            isCorrect = 0;
        }

        applyAnswerResult(p, isCorrect);

        /* Immediate feedback after every question */
        if (isCorrect == 1) {
            printf("\n  >> CORRECT!   Running Score: %.2f\n", p->score);
        } else if (isCorrect == 0) {
            printf("\n  >> WRONG!  Correct answer was %d. %s\n",
                   bank[qIdx].correctOption + 1,
                   bank[qIdx].options[bank[qIdx].correctOption]);
            printf("  Running Score: %.2f\n", p->score);
        } else {
            printf("\n  >> SKIPPED.   Running Score: %.2f\n", p->score);
        }
        printDivider('-', 50);
    }

    /* IMPORTANT: recompute the FINAL score directly from the formula
       here, rather than trusting the value accumulated question-by-
       question in applyAnswerResult(). During the quiz, the running
       score is clamped at 0 after every question purely for friendly
       live feedback (so the player never sees a confusing negative
       number mid-quiz). If we kept that clamped value as the final
       score, an early streak of wrong answers could get silently
       "forgiven" and the final score would stop matching the exact
       formula printed in the result file. Recalculating once here
       guarantees the stored score always equals
       (correct x 1.00) - (wrong x 0.25), floored at 0. */
    p->score = (p->correct * CORRECT_MARK) - (p->wrong * WRONG_PENALTY);
    if (p->score < 0) p->score = 0.0f;

    p->totalQuestions = numQuestions;
}


/* ================================================================
 *  SECTION 9 : RESULT GENERATOR MODULE
 * ================================================================ */

/*
 * Function : writeResultFile
 * ------------------------------------------------------------
 * Purpose  : Writes result_<StudentID>.txt containing every
 *            question asked, the player's answer, whether it was
 *            correct/wrong/skipped, and a summary with the final
 *            score and rank at the bottom (per project requirement
 *            #10).
 * Params   : p            - the player whose result is being written
 *            bank[]       - the full question bank (to look up text)
 *            log[]        - the recorded answers from runQuizSession()
 *            numQuestions - how many questions were in this session
 *            rank         - this player's rank on the leaderboard
 *            totalPlayers - how many players are on the leaderboard
 * Returns  : void
 */
void writeResultFile(const Player *p, Question bank[], AnswerRecord log[],
                      int numQuestions, int rank, int totalPlayers) {
    char filename[64];
    snprintf(filename, sizeof(filename), "result_%s.txt", p->studentID);

    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("  >> ERROR: could not create %s\n", filename);
        return;
    }

    fprintf(fp, "==========================================================================================\n");
    fprintf(fp, "      %s - RESULT REPORT\n", APP_TITLE);
    fprintf(fp, "==========================================================================================\n");
    fprintf(fp, "Name        : %s\n", p->name);
    fprintf(fp, "Student ID  : %s\n", p->studentID);
    fprintf(fp, "Batch       : %d\n", p->batch);
    fprintf(fp, "Department  : %s\n", p->deptCode);
    fprintf(fp, "------------------------------------------------------------------------------------------\n");

    int i;
    for (i = 0; i < numQuestions; i++) {
        int qIdx   = log[i].questionIndex;
        int sel    = log[i].selectedOption;
        Question q = bank[qIdx];

        const char *marker;
        if (sel == -1)                  marker = "[-] SKIPPED";
        else if (sel == q.correctOption) marker = "[+] CORRECT";
        else                              marker = "[x] WRONG  ";

        fprintf(fp, "Q%02d %s\n", i + 1, marker);
        fprintf(fp, "    Question : %s\n", q.question);

        if (sel == -1) {
            fprintf(fp, "    Your Answer   : (skipped)\n");
        } else {
            fprintf(fp, "    Your Answer   : %d. %s\n", sel + 1, q.options[sel]);
        }
        if (sel != q.correctOption) {
            fprintf(fp, "    Correct Answer: %d. %s\n",
                    q.correctOption + 1, q.options[q.correctOption]);
        }
        fprintf(fp, "\n");
    }

    fprintf(fp, "------------------------------------------------------------------------------------------\n");
    fprintf(fp, "Correct : %d   Wrong : %d   Skipped : %d\n",
            p->correct, p->wrong, p->skipped);
    fprintf(fp, "Score   : (%d x 1.00) - (%d x 0.25) = %.2f / %d\n",
            p->correct, p->wrong, p->score, numQuestions);
    fprintf(fp, "Rank    : %d out of %d players\n", rank, totalPlayers);
    fprintf(fp, "==========================================================================================\n");

    fclose(fp);
    printf("  >> Result saved to %s\n", filename);
}

/*
 * Function : printResultSummary
 * ------------------------------------------------------------
 * Purpose  : Prints a short version of the result straight to
 *            the console right after the quiz ends, so the
 *            player doesn't have to open the result file just
 *            to see how they did.
 * Params   : p            - the player who just finished
 *            rank         - this player's rank on the leaderboard
 *            totalPlayers - how many players are on the leaderboard
 * Returns  : void
 */
void printResultSummary(const Player *p, int rank, int totalPlayers) {
    printBanner("QUIZ COMPLETE - YOUR RESULT");
    printf("  Correct : %d   Wrong : %d   Skipped : %d\n",
           p->correct, p->wrong, p->skipped);
    printf("  Final Score : %.2f / %d\n", p->score, p->totalQuestions);
    printf("  Rank        : %d out of %d players\n", rank, totalPlayers);
    printDivider('=', 55);
}


/* ================================================================
 *  SECTION 10 : RANKING / SORTING MODULE
 * ================================================================ */

/*
 * Function : bubbleSortByScore
 * ------------------------------------------------------------
 * DSA CONCEPT : BUBBLE SORT
 * ------------------------------------------------------------
 * Purpose  : Sorts players into DESCENDING order of score by
 *            repeatedly comparing and swapping adjacent elements.
 * Complexity : Best case O(n)   (already sorted, thanks to the
 *              `swapped` flag below causing an early exit)
 *              Worst/Average case O(n^2)
 * Space      : O(1) - sorts in place
 * Params     : arr[] - array of players to sort in place
 *              n     - number of players in arr[]
 * Returns    : void
 */
void bubbleSortByScore(Player arr[], int n) {
    int i, j, swapped;
    Player temp;

    for (i = 0; i < n - 1; i++) {
        swapped = 0;
        for (j = 0; j < n - 1 - i; j++) {
            if (arr[j].score < arr[j + 1].score) {
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
                swapped = 1;
            }
        }
        if (!swapped) break;   /* already sorted - stop early */
    }
}

/*
 * Function : partitionByScore
 * ------------------------------------------------------------
 * Purpose  : Helper for quickSortByScore(). Picks the last
 *            element as the pivot and rearranges the sub-array so
 *            every score >= pivot ends up on its left, and every
 *            score < pivot ends up on its right, then returns the
 *            pivot's final sorted position.
 * Params   : arr[]      - array being sorted
 *            low, high  - the boundaries of the current sub-array
 * Returns  : the index where the pivot element ended up
 */
int partitionByScore(Player arr[], int low, int high) {
    float pivot = arr[high].score;
    int i = low - 1, j;
    Player temp;

    for (j = low; j < high; j++) {
        if (arr[j].score >= pivot) {
            i++;
            temp = arr[i]; arr[i] = arr[j]; arr[j] = temp;
        }
    }
    temp = arr[i + 1]; arr[i + 1] = arr[high]; arr[high] = temp;
    return i + 1;
}

/*
 * Function : quickSortByScore
 * ------------------------------------------------------------
 * DSA CONCEPT : QUICK SORT (Divide and Conquer)
 * ------------------------------------------------------------
 * Purpose  : Sorts players into DESCENDING order of score by
 *            picking a pivot, partitioning the array around it,
 *            then recursively sorting the two resulting halves.
 *            This is the PRIMARY sorting algorithm used for the
 *            main leaderboard because of its speed on larger
 *            datasets.
 * Complexity : Average case O(n log n)
 *              Worst case   O(n^2)   (rare, needs already-sorted input)
 * Space      : O(log n) - recursion stack
 * Params     : arr[]      - array of players to sort in place
 *              low, high  - initial call should use 0 and n-1
 * Returns    : void
 */
void quickSortByScore(Player arr[], int low, int high) {
    if (low < high) {
        int pi = partitionByScore(arr, low, high);
        quickSortByScore(arr, low, pi - 1);
        quickSortByScore(arr, pi + 1, high);
    }
}

/*
 * Function : assignRanks
 * ------------------------------------------------------------
 * Purpose  : After arr[] has been sorted (descending by score),
 *            stamps each player's `rank` field with their 1-based
 *            position on the leaderboard.
 *
 * FUTURE UPGRADE: Two players with the exact same score currently
 * get two different ranks (e.g. 3rd and 4th). Standard
 * "competition ranking" (both would share 3rd, next player becomes
 * 5th) could be added here later by comparing scores between
 * consecutive players before incrementing the rank counter.
 *
 * Params   : arr[] - a SORTED array of players
 *            n     - number of players in arr[]
 * Returns  : void
 */
void assignRanks(Player arr[], int n) {
    int i;
    for (i = 0; i < n; i++) {
        arr[i].rank = i + 1;
    }
}

/*
 * Function : displayLeaderboard
 * ------------------------------------------------------------
 * Purpose  : Prints a formatted leaderboard table to the console.
 * Params   : arr[]       - a SORTED array of players
 *            n           - number of players in arr[]
 *            algoUsed    - name of the sorting algorithm used (for display)
 *            elapsedMs   - how long the sort took, in milliseconds
 * Returns  : void
 */
void displayLeaderboard(Player arr[], int n, const char *algoUsed, double elapsedMs) {
    int i, limit;

    printBanner("LEADERBOARD");
    printf("  Sorted using : %s   (%.4f ms for %d records)\n\n", algoUsed, elapsedMs, n);

    printf("  %-4s %-20s %-12s %-6s %-6s %s\n",
           "Rank", "Name", "Student ID", "Batch", "Dept", "Score");
    printDivider('-', 60);

    limit = (n < 10) ? n : 10;   /* show top 10 only */
    for (i = 0; i < limit; i++) {
        printf("  %-4d %-20s %-12s %-6d %-6s %.2f\n",
               arr[i].rank, arr[i].name, arr[i].studentID,
               arr[i].batch, arr[i].deptCode, arr[i].score);
    }
    printDivider('-', 60);
    if (n > 10) {
        printf("  ...and %d more player(s) not shown.\n", n - 10);
    }
}

/*
 * Function : saveLeaderboardReport
 * ------------------------------------------------------------
 * Purpose  : Writes a human-readable snapshot of the current
 *            sorted leaderboard to leaderboard_report.txt. This
 *            is separate from players_scores.txt (the raw data
 *            file) so the raw data is never overwritten - see the
 *            design note at the top of the File Manager Module.
 * Params   : arr[]     - a SORTED array of players
 *            n         - number of players in arr[]
 *            algoUsed  - name of the sorting algorithm used (for display)
 * Returns  : void
 */
void saveLeaderboardReport(Player arr[], int n, const char *algoUsed) {
    FILE *fp = fopen(LEADERBOARD_FILE, "w");
    if (fp == NULL) {
        printf("  >> ERROR: could not write %s\n", LEADERBOARD_FILE);
        return;
    }

    int i;
    fprintf(fp, "=============================================\n");
    fprintf(fp, "   %s - LEADERBOARD\n", APP_TITLE);
    fprintf(fp, "   Sorted using: %s\n", algoUsed);
    fprintf(fp, "=============================================\n");
    fprintf(fp, "%-4s %-20s %-12s %-6s %-6s %s\n",
            "Rank", "Name", "Student ID", "Batch", "Dept", "Score");
    fprintf(fp, "---------------------------------------------\n");

    for (i = 0; i < n; i++) {
        fprintf(fp, "%-4d %-20s %-12s %-6d %-6s %.2f\n",
                arr[i].rank, arr[i].name, arr[i].studentID,
                arr[i].batch, arr[i].deptCode, arr[i].score);
    }
    fprintf(fp, "=============================================\n");
    fclose(fp);
}


/* ================================================================
 *  SECTION 11 : MENU HANDLERS  (top-level flow for each menu option)
 * ================================================================ */

/*
 * Function : showMainMenu
 * ------------------------------------------------------------
 * Purpose  : Clears the screen and prints the main menu options.
 * Params   : none
 * Returns  : void
 */
void showMainMenu(void) {
    system(CLEAR_SCREEN);
    printBanner(APP_TITLE);
    printf("   1. Take Quiz (New Player)\n");
    printf("   2. View Leaderboard\n");
    printf("   3. Search Player by ID\n");
    printf("   4. Exit\n");
    printDivider('=', 55);
}

/*
 * Function : handleTakeQuiz
 * ------------------------------------------------------------
 * Purpose  : Full flow for a new player: register -> check for a
 *            duplicate Student ID -> ask how many questions ->
 *            run the quiz -> save the score -> compute the rank
 *            -> write the result file -> show the result.
 * Params   : bank[]    - the full question bank
 *            bankSize  - number of questions in bank[]
 * Returns  : void
 */
void handleTakeQuiz(Question bank[], int bankSize) {
    Player existing[MAX_PLAYERS];
    int existingCount = loadAllPlayers(existing, MAX_PLAYERS);

    Player newPlayer;
    printBanner("PLAYER REGISTRATION");
    registerPlayer(&newPlayer);

    /* Requirement: prevent the same Student ID from playing twice */
    if (findPlayerIndexByID(existing, existingCount, newPlayer.studentID) != -1) {
        printf("\n  >> This Student ID has already attempted the quiz!\n");
        displayPlayerRecord(newPlayer.studentID);
        pauseScreen();
        return;
    }

    int numQ = getValidatedInt("\nHow many questions (5-200)? ",
                                MIN_SESSION_Q, MAX_SESSION_Q);
    if (numQ > bankSize) numQ = bankSize;   /* safety clamp */

    AnswerRecord log[MAX_SESSION_Q];
    printBanner("QUIZ STARTED - GOOD LUCK!");
    runQuizSession(&newPlayer, bank, bankSize, log, numQ);

    /* Persist this attempt, then reload the FULL list (including
       this new record) straight from disk before ranking, so the
       leaderboard always reflects exactly what's saved on file. */
    appendPlayerScore(&newPlayer);

    Player allPlayers[MAX_PLAYERS];
    int total = loadAllPlayers(allPlayers, MAX_PLAYERS);
    quickSortByScore(allPlayers, 0, total - 1);
    assignRanks(allPlayers, total);

    int myIdx  = findPlayerIndexByID(allPlayers, total, newPlayer.studentID);
    int myRank = (myIdx != -1) ? allPlayers[myIdx].rank : total;

    writeResultFile(&newPlayer, bank, log, numQ, myRank, total);
    printResultSummary(&newPlayer, myRank, total);
    saveLeaderboardReport(allPlayers, total, "Quick Sort");

    pauseScreen();
}

/*
 * Function : handleViewLeaderboard
 * ------------------------------------------------------------
 * Purpose  : Lets the user pick Bubble Sort or Quick Sort, times
 *            the sort using clock(), then displays and saves the
 *            leaderboard. Letting the player pick either algorithm
 *            is a deliberate teaching feature - it lets you show
 *            your course teacher the real speed difference between
 *            an O(n^2) and an O(n log n) algorithm on the same data.
 * Params   : none
 * Returns  : void
 */
void handleViewLeaderboard(void) {
    Player arr[MAX_PLAYERS];
    int n = loadAllPlayers(arr, MAX_PLAYERS);

    if (n == 0) {
        printf("\n  >> No players have taken the quiz yet.\n");
        pauseScreen();
        return;
    }

    printf("\n  Choose a sorting algorithm:\n");
    printf("   1. Bubble Sort   [O(n^2)]\n");
    printf("   2. Quick Sort    [O(n log n)]\n");
    int choice = getValidatedInt("  Choice: ", 1, 2);

    clock_t start = clock();
    if (choice == 1) {
        bubbleSortByScore(arr, n);
    } else {
        quickSortByScore(arr, 0, n - 1);
    }
    clock_t end = clock();
    double elapsedMs = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;

    assignRanks(arr, n);
    const char *algoName = (choice == 1) ? "Bubble Sort" : "Quick Sort";
    displayLeaderboard(arr, n, algoName, elapsedMs);
    saveLeaderboardReport(arr, n, algoName);

    pauseScreen();
}

/*
 * Function : handleSearchPlayer
 * ------------------------------------------------------------
 * Purpose  : Prompts for a Student ID and displays that player's
 *            saved result using Linear Search.
 * Params   : none
 * Returns  : void
 */
void handleSearchPlayer(void) {
    char id[MAX_ID_LEN];

    printf("\nEnter Student ID to search: ");
    scanf("%14s", id);
    clearInputBuffer();

    displayPlayerRecord(id);
    pauseScreen();
}


/* ================================================================
 *  SECTION 12 : MAIN
 * ================================================================ */
int main(void) {
    Question bank[MAX_BANK_SIZE];
    int bankSize;
    int choice;

    srand((unsigned int)time(NULL));   /* seed random number generator once */
    loadQuestionBank(bank, &bankSize);

    do {
        showMainMenu();
        choice = getValidatedInt("Enter choice (1-4): ", 1, 4);

        switch (choice) {
            case 1: handleTakeQuiz(bank, bankSize); break;
            case 2: handleViewLeaderboard();        break;
            case 3: handleSearchPlayer();           break;
            case 4:
                printf("\nThank you for playing %s. Goodbye!\n", APP_TITLE);
                break;
        }
    } while (choice != 4);

    return 0;
}
