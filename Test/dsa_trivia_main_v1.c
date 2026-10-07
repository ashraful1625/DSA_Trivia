/* ================================================================
 *  PROJECT   : DSA Trivia Challenge - UPGRADED v2.0
 *  COURSE    : Data Structures and Algorithms (DSA)
 *  GROUP     : 05
 *  LANGUAGE  : C (standard C11, no external libraries)
 *
 *  DESCRIPTION
 *  -----------
 *  A console-based multiplayer quiz game demonstrating ADVANCED DSA:
 *
 *   1. SINGLY LINKED LIST   -> Dynamic Question Bank
 *   2. BINARY SEARCH TREE   -> Player storage ordered by Roll Number
 *   3. HASH TABLE (Chaining)-> O(1) player authentication by Roll
 *   4. MERGE SORT           -> Leaderboard ranking (from scratch)
 *   5. BUBBLE SORT          -> Timing comparison baseline
 *   6. QUICK SORT           -> Timing comparison (existing)
 *   7. FISHER-YATES SHUFFLE -> Randomized question order
 *
 *  HOW TO COMPILE & RUN
 *  ---------------------
 *      gcc dsa_trivia_upgraded.c -o dsa_trivia_upgraded
 *      ./dsa_trivia_upgraded
 *
 *  FILES CREATED AT RUNTIME
 *  -------------------------
 *      players_scores.txt     -> append-only score log (all players)
 *      result_<StudentID>.txt -> one file per player, full Q&A + score
 *      leaderboard_report.txt -> human-readable sorted snapshot
 * ================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ================================================================
 *  SECTION 0 : CONFIGURATION CONSTANTS
 * ================================================================ */
#define MAX_NAME_LEN      50
#define MAX_ID_LEN        10
#define MAX_DEPT_LEN      4
#define MAX_Q_TEXT        250
#define MAX_OPT_LEN       100
#define HASH_SIZE         101
#define MIN_SESSION_Q     5
#define MAX_SESSION_Q     200
#define MAX_PLAYERS       200
#define CORRECT_MARK      1.00f
#define WRONG_PENALTY     0.25f
#define SCORES_FILE       "players_scores.txt"
#define LEADERBOARD_FILE  "leaderboard_report.txt"
#define FIELD_DELIM       "|"
#define APP_TITLE         "DSA TRIVIA CHALLENGE"

#ifdef _WIN32
    #define CLEAR_SCREEN "cls"
#else
    #define CLEAR_SCREEN "clear"
#endif

/* ================================================================
 *  SECTION 1 : DATA STRUCTURES
 * ================================================================ */

/* ---- 1.1 Linked List Node for Question Bank ---- */
typedef struct QNode {
    int  qno;
    char question[MAX_Q_TEXT];
    char options[4][MAX_OPT_LEN];
    int  correctOption;          /* 0-3 */
    struct QNode *next;
} QNode;

/* ---- 1.2 Player Node (shared by BST and Hash Table) ---- */
typedef struct Player {
    char  name[MAX_NAME_LEN];
    char  studentID[MAX_ID_LEN];
    int   batch;
    char  deptCode[MAX_DEPT_LEN];
    float score;
    int   correct;
    int   wrong;
    int   skipped;
    int   totalQuestions;
    int   rank;

    /* BST links (ordered by studentID string comparison) */
    struct Player *left;
    struct Player *right;

    /* Hash Table chain link */
    struct Player *hash_next;
} Player;

/* ---- 1.3 Hash Table ---- */
typedef struct {
    Player *buckets[HASH_SIZE];
} HashTable;

/* ---- 1.4 Answer Record (unchanged) ---- */
typedef struct {
    int questionIndex;   /* 1-based question number in linked list */
    int selectedOption;  /* 0-3, or -1 for skipped */
} AnswerRecord;

/* ================================================================
 *  SECTION 2 : GLOBAL STATE
 * ================================================================ */
QNode     *g_question_head = NULL;
Player    *g_bst_root    = NULL;
HashTable  g_ht;

/* ================================================================
 *  SECTION 3 : FUNCTION PROTOTYPES
 * ================================================================ */

/* Utility */
void  clearInputBuffer(void);
void  pauseScreen(void);
void  printDivider(char ch, int len);
void  printBanner(const char *title);
int   getValidatedInt(const char *prompt, int minVal, int maxVal);

/* Question Bank - Linked List */
void  initQuestionBank(void);
void  addQuestion(int qno, const char *qtext,
                  const char *o1, const char *o2,
                  const char *o3, const char *o4, int correct);
void  freeQuestionBank(void);
int   countQuestions(void);
QNode* getQuestionByIndex(int idx);  /* 1-based traverse */

/* Hash Table - O(1) Authentication */
void  initHashTable(void);
int   hashFunc(const char *studentID);
void  insertHash(Player *p);
Player* searchHash(const char *studentID);

/* Binary Search Tree - Ordered Player Storage */
Player* insertBST(Player *root, Player *p);
Player* searchBST(Player *root, const char *studentID);
int   countBST(Player *root);
void  inorderCollect(Player *root, Player **arr, int *idx);
void  freeBST(Player *root);

/* Sorting Algorithms */
void  bubbleSortByScore(Player *arr[], int n);
int   partitionByScore(Player *arr[], int low, int high);
void  quickSortByScore(Player *arr[], int low, int high);
void  mergeSortByScore(Player *arr[], int left, int right);
void  merge(Player *arr[], int left, int mid, int right);

/* File I/O */
void  saveAllPlayers(void);
void  loadAllPlayers(void);
void  appendPlayerScore(const Player *p);
void  saveLeaderboardReport(Player *arr[], int n, const char *algoUsed);

/* Quiz Engine */
void  shuffleIndices(int arr[], int n);
int   askSingleQuestion(const QNode *q, int qNumber, int totalQ);
void  runQuizSession(Player *p, AnswerRecord log[], int numQuestions);

/* Result Generator */
void  writeResultFile(const Player *p, AnswerRecord log[],
                      int numQuestions, int rank, int totalPlayers);
void  printResultSummary(const Player *p, int rank, int totalPlayers);

/* Ranking */
void  assignRanks(Player *arr[], int n);
void  displayLeaderboard(Player *arr[], int n,
                         const char *algoUsed, double elapsedMs);

/* Menu Handlers */
void  showMainMenu(void);
void  handleTakeQuiz(void);
void  handleViewLeaderboard(void);
void  handleSearchPlayer(void);

/* ================================================================
 *  SECTION 4 : UTILITY MODULE
 * ================================================================ */
void clearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

void pauseScreen(void) {
    printf("\nPress ENTER to continue...");
    clearInputBuffer();
}

void printDivider(char ch, int len) {
    int i;
    for (i = 0; i < len; i++) putchar(ch);
    putchar('\n');
}

void printBanner(const char *title) {
    printDivider('=', 60);
    printf("   %s\n", title);
    printDivider('=', 60);
}

int getValidatedInt(const char *prompt, int minVal, int maxVal) {
    int value, result;
    while (1) {
        printf("%s", prompt);
        result = scanf("%d", &value);
        clearInputBuffer();
        if (result == 1 && value >= minVal && value <= maxVal)
            return value;
        printf("  >> Invalid. Enter a number between %d and %d.\n", minVal, maxVal);
    }
}

/* ================================================================
 *  SECTION 5 : QUESTION BANK - LINKED LIST MODULE
 * ================================================================ */

/*
 * DSA CONCEPT: SINGLY LINKED LIST
 * Each question is a node. Nodes are linked via 'next' pointer.
 * This allows dynamic growth without a fixed array size.
 * Traversal is sequential: O(n) to reach the i-th question.
 */
void addQuestion(int qno, const char *qtext,
                 const char *o1, const char *o2,
                 const char *o3, const char *o4, int correct) {
    QNode *newq = (QNode *)malloc(sizeof(QNode));
    if (!newq) {
        printf("[!] Memory allocation failed for question.\n");
        exit(1);
    }
    newq->qno = qno;
    strncpy(newq->question, qtext, MAX_Q_TEXT - 1);
    newq->question[MAX_Q_TEXT - 1] = '\0';
    strncpy(newq->options[0], o1, MAX_OPT_LEN - 1);
    strncpy(newq->options[1], o2, MAX_OPT_LEN - 1);
    strncpy(newq->options[2], o3, MAX_OPT_LEN - 1);
    strncpy(newq->options[3], o4, MAX_OPT_LEN - 1);
    newq->correctOption = correct;
    newq->next = NULL;

    if (g_question_head == NULL) {
        g_question_head = newq;
    } else {
        QNode *temp = g_question_head;
        while (temp->next != NULL) temp = temp->next;
        temp->next = newq;
    }
}

void initQuestionBank(void) {
    addQuestion(1,
        "What is the worst-case time complexity of Bubble Sort?",
        "O(n)", "O(n log n)", "O(n^2)", "O(log n)", 2);
    addQuestion(2,
        "Which data structure uses the LIFO principle?",
        "Queue", "Stack", "Array", "Graph", 1);
    addQuestion(3,
        "What is the average time complexity of Quick Sort?",
        "O(n^2)", "O(n)", "O(n log n)", "O(log n)", 2);
    addQuestion(4,
        "Which algorithm uses the Divide and Conquer strategy?",
        "Bubble Sort", "Selection Sort", "Merge Sort", "Insertion Sort", 2);
    addQuestion(5,
        "What is the time complexity of Binary Search?",
        "O(n)", "O(n^2)", "O(n log n)", "O(log n)", 3);
    addQuestion(6,
        "Which data structure is used in BFS traversal?",
        "Stack", "Queue", "Tree", "Heap", 1);
    addQuestion(7,
        "Which data structure is used in DFS traversal?",
        "Queue", "Stack", "Graph", "Linked List", 1);
    addQuestion(8,
        "What is the space complexity of Merge Sort?",
        "O(1)", "O(log n)", "O(n)", "O(n^2)", 2);
    addQuestion(9,
        "Which sorting algorithm is considered stable?",
        "Quick Sort", "Heap Sort", "Merge Sort", "Selection Sort", 2);
    addQuestion(10,
        "A Stack Overflow occurs when?",
        "The stack is empty", "The stack exceeds its size limit",
        "The queue is full", "The array is empty", 1);
    addQuestion(11,
        "Array indexing in C starts from?",
        "1", "-1", "0", "2", 2);
    addQuestion(12,
        "Which traversal visits the root node first?",
        "Inorder", "Postorder", "Level Order", "Preorder", 3);
    addQuestion(13,
        "What is the best-case time complexity of Bubble Sort?",
        "O(n^2)", "O(n log n)", "O(n)", "O(1)", 2);
    addQuestion(14,
        "Which of these is NOT a linear data structure?",
        "Array", "Queue", "Tree", "Stack", 2);
    addQuestion(15,
        "What is the worst-case time complexity of Quick Sort?",
        "O(n log n)", "O(n)", "O(n^2)", "O(log n)", 2);
    addQuestion(16,
        "A linked list node typically contains?",
        "Only data", "Only a pointer", "Data and a pointer", "Only an index", 2);
    addQuestion(17,
        "Which data structure implements recursion internally?",
        "Queue", "Stack", "Tree", "Graph", 1);
    addQuestion(18,
        "Heap Sort relies on which data structure?",
        "Queue", "Stack", "Priority Queue (Heap)", "Linked List", 2);
    addQuestion(19,
        "In a Min-Heap, the root always contains the?",
        "Maximum element", "Middle element", "Minimum element", "A random element", 2);
    addQuestion(20,
        "What does the acronym DSA stand for?",
        "Digital System Analysis", "Data Structure and Algorithm",
        "Data Science Application", "Dynamic System Array", 1);
}

void freeQuestionBank(void) {
    QNode *temp;
    while (g_question_head != NULL) {
        temp = g_question_head;
        g_question_head = g_question_head->next;
        free(temp);
    }
}

int countQuestions(void) {
    int c = 0;
    QNode *t = g_question_head;
    while (t) { c++; t = t->next; }
    return c;
}

/* Traverse linked list to 1-based index. Returns NULL if out of bounds. */
QNode* getQuestionByIndex(int idx) {
    QNode *t = g_question_head;
    int i;
    for (i = 1; i < idx && t != NULL; i++)
        t = t->next;
    return t;
}

/* ================================================================
 *  SECTION 6 : HASH TABLE MODULE (Authentication)
 * ================================================================ */

/*
 * DSA CONCEPT: HASH TABLE with SEPARATE CHAINING
 * We hash the studentID string to an index. Collisions are
 * resolved by chaining (linked list at each bucket).
 * Average case lookup: O(1). Worst case: O(n) if all collide.
 */
void initHashTable(void) {
    int i;
    for (i = 0; i < HASH_SIZE; i++) g_ht.buckets[i] = NULL;
}

/* Simple string hash: djb2 algorithm */
int hashFunc(const char *studentID) {
    unsigned long hash = 5381;
    int c;
    while ((c = *studentID++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return (int)(hash % HASH_SIZE);
}

void insertHash(Player *p) {
    int idx = hashFunc(p->studentID);
    p->hash_next = g_ht.buckets[idx];
    g_ht.buckets[idx] = p;
}

Player* searchHash(const char *studentID) {
    int idx = hashFunc(studentID);
    Player *curr = g_ht.buckets[idx];
    while (curr != NULL) {
        if (strcmp(curr->studentID, studentID) == 0)
            return curr;
        curr = curr->hash_next;
    }
    return NULL;
}

/* ================================================================
 *  SECTION 7 : BINARY SEARCH TREE MODULE (Player Storage)
 * ================================================================ */

/*
 * DSA CONCEPT: BINARY SEARCH TREE
 * Players are ordered by studentID (string comparison).
 * Left subtree: all IDs lexicographically smaller.
 * Right subtree: all IDs lexicographically larger.
 * Search: O(h) where h = height. Balanced: O(log n). Skewed: O(n).
 */
Player* insertBST(Player *root, Player *p) {
    if (root == NULL) return p;
    if (strcmp(p->studentID, root->studentID) < 0)
        root->left = insertBST(root->left, p);
    else if (strcmp(p->studentID, root->studentID) > 0)
        root->right = insertBST(root->right, p);
    /* Duplicate IDs: update score fields if re-taking (not expected) */
    return root;
}

Player* searchBST(Player *root, const char *studentID) {
    if (root == NULL || strcmp(root->studentID, studentID) == 0)
        return root;
    if (strcmp(studentID, root->studentID) < 0)
        return searchBST(root->left, studentID);
    return searchBST(root->right, studentID);
}

int countBST(Player *root) {
    if (root == NULL) return 0;
    return 1 + countBST(root->left) + countBST(root->right);
}

/* Inorder traversal collects BST nodes into an array */
void inorderCollect(Player *root, Player **arr, int *idx) {
    if (root == NULL) return;
    inorderCollect(root->left, arr, idx);
    arr[(*idx)++] = root;
    inorderCollect(root->right, arr, idx);
}

void freeBST(Player *root) {
    if (root == NULL) return;
    freeBST(root->left);
    freeBST(root->right);
    free(root);
}

/* ================================================================
 *  SECTION 8 : SORTING MODULE (Bubble, Quick, Merge)
 * ================================================================ */

/* ---- Bubble Sort ---- */
void bubbleSortByScore(Player *arr[], int n) {
    int i, j, swapped;
    Player *temp;
    for (i = 0; i < n - 1; i++) {
        swapped = 0;
        for (j = 0; j < n - 1 - i; j++) {
            if (arr[j]->score < arr[j + 1]->score) {
                temp = arr[j]; arr[j] = arr[j + 1]; arr[j + 1] = temp;
                swapped = 1;
            }
        }
        if (!swapped) break;
    }
}

/* ---- Quick Sort ---- */
int partitionByScore(Player *arr[], int low, int high) {
    float pivot = arr[high]->score;
    int i = low - 1, j;
    Player *temp;
    for (j = low; j < high; j++) {
        if (arr[j]->score >= pivot) {
            i++;
            temp = arr[i]; arr[i] = arr[j]; arr[j] = temp;
        }
    }
    temp = arr[i + 1]; arr[i + 1] = arr[high]; arr[high] = temp;
    return i + 1;
}

void quickSortByScore(Player *arr[], int low, int high) {
    if (low < high) {
        int pi = partitionByScore(arr, low, high);
        quickSortByScore(arr, low, pi - 1);
        quickSortByScore(arr, pi + 1, high);
    }
}

/* ---- Merge Sort (from scratch, no qsort) ---- */
/*
 * DSA CONCEPT: MERGE SORT (Divide and Conquer)
 * Splits array into halves, recursively sorts each half,
 * then merges them back in sorted order.
 * Time: O(n log n) in all cases. Space: O(n) auxiliary.
 */
void merge(Player *arr[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    Player **L = (Player **)malloc(n1 * sizeof(Player *));
    Player **R = (Player **)malloc(n2 * sizeof(Player *));
    int i, j, k;

    for (i = 0; i < n1; i++) L[i] = arr[left + i];
    for (j = 0; j < n2; j++) R[j] = arr[mid + 1 + j];

    i = 0; j = 0; k = left;
    while (i < n1 && j < n2) {
        if (L[i]->score > R[j]->score)
            arr[k++] = L[i++];
        else if (L[i]->score < R[j]->score)
            arr[k++] = R[j++];
        else { /* tie-break by studentID ascending */
            if (strcmp(L[i]->studentID, R[j]->studentID) < 0)
                arr[k++] = L[i++];
            else
                arr[k++] = R[j++];
        }
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];

    free(L); free(R);
}

void mergeSortByScore(Player *arr[], int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSortByScore(arr, left, mid);
        mergeSortByScore(arr, mid + 1, right);
        merge(arr, left, mid, right);
    }
}

/* ================================================================
 *  SECTION 9 : FILE I/O MODULE
 * ================================================================ */

/*
 * Save all players from BST to file (inorder traversal).
 * This preserves a sorted-by-ID snapshot of the database.
 */
void saveAllPlayers(void) {
    FILE *fp = fopen(SCORES_FILE, "w");
    if (!fp) { printf("[!] Could not save players.\n"); return; }

    int n = countBST(g_bst_root);
    Player **arr = (Player **)malloc(n * sizeof(Player *));
    int idx = 0;
    inorderCollect(g_bst_root, arr, &idx);

    int i;
    for (i = 0; i < n; i++) {
        fprintf(fp, "%s|%s|%d|%s|%.2f|%d|%d|%d|%d\n",
                arr[i]->name, arr[i]->studentID, arr[i]->batch,
                arr[i]->deptCode, arr[i]->score, arr[i]->correct,
                arr[i]->wrong, arr[i]->skipped, arr[i]->totalQuestions);
    }
    free(arr);
    fclose(fp);
}

/*
 * Load players from file into BST + Hash Table.
 * Each line becomes ONE malloc'd Player node shared by both structures.
 */
void loadAllPlayers(void) {
    FILE *fp = fopen(SCORES_FILE, "r");
    if (!fp) return;

    char line[400];
    while (fgets(line, sizeof(line), fp)) {
        Player *p = (Player *)malloc(sizeof(Player));
        if (!p) continue;
        p->left = p->right = p->hash_next = NULL;

        int matched = sscanf(line,
            "%49[^|]|%14[^|]|%d|%9[^|]|%f|%d|%d|%d|%d",
            p->name, p->studentID, &p->batch, p->deptCode,
            &p->score, &p->correct, &p->wrong, &p->skipped, &p->totalQuestions);

        if (matched == 9) {
            p->rank = 0;
            g_bst_root = insertBST(g_bst_root, p);
            insertHash(p);
        } else {
            free(p);
        }
    }
    fclose(fp);
}

void appendPlayerScore(const Player *p) {
    FILE *fp = fopen(SCORES_FILE, "a");
    if (!fp) {
        printf("  >> ERROR: could not open %s\n", SCORES_FILE);
        return;
    }
    fprintf(fp, "%s|%s|%d|%s|%.2f|%d|%d|%d|%d\n",
            p->name, p->studentID, p->batch, p->deptCode,
            p->score, p->correct, p->wrong, p->skipped, p->totalQuestions);
    fclose(fp);
}

void saveLeaderboardReport(Player *arr[], int n, const char *algoUsed) {
    FILE *fp = fopen(LEADERBOARD_FILE, "w");
    if (!fp) return;

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
                arr[i]->rank, arr[i]->name, arr[i]->studentID,
                arr[i]->batch, arr[i]->deptCode, arr[i]->score);
    }
    fprintf(fp, "=============================================\n");
    fclose(fp);
}

/* ================================================================
 *  SECTION 10 : PLAYER REGISTRATION MODULE
 * ================================================================ */
void registerPlayer(Player *p) {
    size_t len, i;

    printf("Enter Full Name        : ");
    fgets(p->name, MAX_NAME_LEN, stdin);
    len = strlen(p->name);
    if (len > 0 && p->name[len - 1] == '\n') p->name[len - 1] = '\0';
    for (i = 0; i < strlen(p->name); i++)
        if (p->name[i] == '|') p->name[i] = ' ';

    printf("Enter Student ID (Roll): ");
    scanf("%14s", p->studentID);

    p->batch = getValidatedInt("Enter Batch Number     : ", 200, 1000);

    printf("Enter Department Code  : ");
    scanf("%9s", p->deptCode);
    clearInputBuffer();

    p->score = 0.0f; p->correct = 0; p->wrong = 0;
    p->skipped = 0; p->totalQuestions = 0; p->rank = 0;
    p->left = p->right = p->hash_next = NULL;
}

/*
 * Display a single player's record by ID.
 * Uses Hash Table O(1) lookup instead of linear search.
 */
void displayPlayerRecord(const char *studentID) {
    Player *p = searchHash(studentID);
    if (p == NULL) {
        printf("  >> No record found for Student ID: %s\n", studentID);
        return;
    }

    printDivider('-', 45);
    printf("  Name        : %s\n", p->name);
    printf("  Student ID  : %s\n", p->studentID);
    printf("  Batch       : %d\n", p->batch);
    printf("  Department  : %s\n", p->deptCode);
    printf("  Correct     : %d\n", p->correct);
    printf("  Wrong       : %d\n", p->wrong);
    printf("  Skipped     : %d\n", p->skipped);
    printf("  Final Score : %.2f / %d\n", p->score, p->totalQuestions);
    printDivider('-', 45);
}

/* ================================================================
 *  SECTION 11 : SCORING MODULE
 * ================================================================ */
void applyAnswerResult(Player *p, int isCorrect) {
    if (isCorrect == 1) {
        p->correct++;
        p->score += CORRECT_MARK;
    } else if (isCorrect == 0) {
        p->wrong++;
        p->score -= WRONG_PENALTY;
        if (p->score < 0) p->score = 0.0f;
    } else {
        p->skipped++;
    }
}

/* ================================================================
 *  SECTION 12 : QUIZ ENGINE MODULE
 * ================================================================ */

/* Fisher-Yates Shuffle on an index array */
void shuffleIndices(int arr[], int n) {
    int i, j, temp;
    for (i = n - 1; i > 0; i--) {
        j = rand() % (i + 1);
        temp = arr[i]; arr[i] = arr[j]; arr[j] = temp;
    }
}

int askSingleQuestion(const QNode *q, int qNumber, int totalQ) {
    int i, choice;
    printf("\nQuestion %d of %d:\n%s\n\n", qNumber, totalQ, q->question);
    for (i = 0; i < 4; i++)
        printf("   %d. %s\n", i + 1, q->options[i]);
    choice = getValidatedInt("\nYour answer (1-4, or 0 to skip): ", 0, 4);
    return (choice == 0) ? -1 : (choice - 1);
}

/*
 * Quiz session using the LINKED LIST question bank.
 * We build a shuffled array of 1-based indices, then use
 * getQuestionByIndex() to traverse the linked list for each question.
 */
void runQuizSession(Player *p, AnswerRecord log[], int numQuestions) {
    int bankSize = countQuestions();
    int *order = (int *)malloc(bankSize * sizeof(int));
    int i, qIdx, sel, isCorrect;
    QNode *q;

    for (i = 0; i < bankSize; i++) order[i] = i + 1; /* 1-based indices */
    shuffleIndices(order, bankSize);

    for (i = 0; i < numQuestions; i++) {
        qIdx = order[i];
        q = getQuestionByIndex(qIdx);  /* Linked list traversal O(n) each time */
        if (q == NULL) continue;       /* safety */

        sel = askSingleQuestion(q, i + 1, numQuestions);

        log[i].questionIndex = qIdx;
        log[i].selectedOption = sel;

        if (sel == -1) isCorrect = -1;
        else if (sel == q->correctOption) isCorrect = 1;
        else isCorrect = 0;

        applyAnswerResult(p, isCorrect);

        if (isCorrect == 1)
            printf("\n  >> CORRECT!   Running Score: %.2f\n", p->score);
        else if (isCorrect == 0)
            printf("\n  >> WRONG!  Correct answer was %d. %s\nRunning Score: %.2f\n",
                   q->correctOption + 1, q->options[q->correctOption], p->score);
        else
            printf("\n  >> SKIPPED.   Running Score: %.2f\n", p->score);
        printDivider('-', 50);
    }

    p->score = (p->correct * CORRECT_MARK) - (p->wrong * WRONG_PENALTY);
    if (p->score < 0) p->score = 0.0f;
    p->totalQuestions = numQuestions;
    free(order);
}

/* ================================================================
 *  SECTION 13 : RESULT GENERATOR MODULE
 * ================================================================ */
void writeResultFile(const Player *p, AnswerRecord log[],
                     int numQuestions, int rank, int totalPlayers) {
    char filename[64];
    snprintf(filename, sizeof(filename), "result_%s.txt", p->studentID);

    FILE *fp = fopen(filename, "w");
    if (!fp) { printf("  >> ERROR: could not create %s\n", filename); return; }

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
        QNode *q = getQuestionByIndex(log[i].questionIndex);
        if (!q) continue;
        int sel = log[i].selectedOption;
        const char *marker;
        if (sel == -1) marker = "[-] SKIPPED";
        else if (sel == q->correctOption) marker = "[+] CORRECT";
        else marker = "[x] WRONG  ";

        fprintf(fp, "Q%02d %s\n", i + 1, marker);
        fprintf(fp, "    Question : %s\n", q->question);
        if (sel == -1)
            fprintf(fp, "    Your Answer   : (skipped)\n");
        else
            fprintf(fp, "    Your Answer   : %d. %s\n", sel + 1, q->options[sel]);
        if (sel != q->correctOption)
            fprintf(fp, "    Correct Answer: %d. %s\n",
                    q->correctOption + 1, q->options[q->correctOption]);
        fprintf(fp, "\n");
    }

    fprintf(fp, "------------------------------------------------------------------------------------------\n");
    fprintf(fp, "Correct : %d   Wrong : %d   Skipped : %d\n", p->correct, p->wrong, p->skipped);
    fprintf(fp, "Score   : (%d x 1.00) - (%d x 0.25) = %.2f / %d\n",
            p->correct, p->wrong, p->score, numQuestions);
    fprintf(fp, "Rank    : %d out of %d players\n", rank, totalPlayers);
    fprintf(fp, "==========================================================================================\n");
    fclose(fp);
    printf("  >> Result saved to %s\n", filename);
}

void printResultSummary(const Player *p, int rank, int totalPlayers) {
    printBanner("QUIZ COMPLETE - YOUR RESULT");
    printf("  Correct : %d   Wrong : %d   Skipped : %d\n",
           p->correct, p->wrong, p->skipped);
    printf("  Final Score : %.2f / %d\n", p->score, p->totalQuestions);
    printf("  Rank        : %d out of %d players\n", rank, totalPlayers);
    printDivider('=', 55);
}

/* ================================================================
 *  SECTION 14 : RANKING / LEADERBOARD MODULE
 * ================================================================ */
void assignRanks(Player *arr[], int n) {
    int i;
    for (i = 0; i < n; i++) arr[i]->rank = i + 1;
}

void displayLeaderboard(Player *arr[], int n, const char *algoUsed, double elapsedMs) {
    int i, limit;
    printBanner("LEADERBOARD");
    printf("  Sorted using : %s   (%.4f ms for %d records)\n\n", algoUsed, elapsedMs, n);
    printf("  %-4s %-20s %-12s %-6s %-6s %s\n",
           "Rank", "Name", "Student ID", "Batch", "Dept", "Score");
    printDivider('-', 60);
    limit = (n < 10) ? n : 10;
    for (i = 0; i < limit; i++) {
        printf("  %-4d %-20s %-12s %-6d %-6s %.2f\n",
               arr[i]->rank, arr[i]->name, arr[i]->studentID,
               arr[i]->batch, arr[i]->deptCode, arr[i]->score);
    }
    printDivider('-', 60);
    if (n > 10) printf("  ...and %d more player(s) not shown.\n", n - 10);
}

/* ================================================================
 *  SECTION 15 : MENU HANDLERS
 * ================================================================ */
void showMainMenu(void) {
    system(CLEAR_SCREEN);
    printBanner(APP_TITLE);
    printf("   1. Take Quiz (New Player)\n");
    printf("   2. View Leaderboard\n");
    printf("   3. Search Player by ID\n");
    printf("   4. Exit\n");
    printDivider('=', 60);
}

void handleTakeQuiz(void) {
    char id[MAX_ID_LEN];

    printBanner("PLAYER REGISTRATION");
    printf("Enter Student ID (Roll): ");
    scanf("%14s", id);
    clearInputBuffer();

    /* O(1) Hash Table lookup for existing player */
    Player *existing = searchHash(id);
    if (existing != NULL) {
        printf("\n  >> This Student ID has already attempted the quiz!\n");
        displayPlayerRecord(id);
        pauseScreen();
        return;
    }

    Player *newPlayer = (Player *)malloc(sizeof(Player));
    if (!newPlayer) { printf("[!] Memory error.\n"); return; }

    strncpy(newPlayer->studentID, id, MAX_ID_LEN - 1);
    newPlayer->studentID[MAX_ID_LEN - 1] = '\0';

    printf("Enter Full Name        : ");
    fgets(newPlayer->name, MAX_NAME_LEN, stdin);
    size_t len = strlen(newPlayer->name);
    if (len > 0 && newPlayer->name[len - 1] == '\n')
        newPlayer->name[len - 1] = '\0';
    size_t i;
    for (i = 0; i < strlen(newPlayer->name); i++)
        if (newPlayer->name[i] == '|') newPlayer->name[i] = ' ';

    newPlayer->batch = getValidatedInt("Enter Batch Number     : ", 200, 1000);
    printf("Enter Department Code  : ");
    scanf("%9s", newPlayer->deptCode);
    clearInputBuffer();

    newPlayer->score = 0.0f; newPlayer->correct = 0; newPlayer->wrong = 0;
    newPlayer->skipped = 0; newPlayer->totalQuestions = 0; newPlayer->rank = 0;
    newPlayer->left = newPlayer->right = newPlayer->hash_next = NULL;

    int bankSize = countQuestions();
    int numQ = getValidatedInt("\nHow many questions (5-200)? ", MIN_SESSION_Q, MAX_SESSION_Q);
    if (numQ > bankSize) numQ = bankSize;

    AnswerRecord log[MAX_SESSION_Q];
    printBanner("QUIZ STARTED - GOOD LUCK!");
    runQuizSession(newPlayer, log, numQ);

    /* Insert into BST + Hash Table */
    g_bst_root = insertBST(g_bst_root, newPlayer);
    insertHash(newPlayer);

    /* Persist */
    appendPlayerScore(newPlayer);
    saveAllPlayers();  /* rewrite entire file from BST inorder */

    /* Build leaderboard array from BST */
    int total = countBST(g_bst_root);
    Player **arr = (Player **)malloc(total * sizeof(Player *));
    int idx = 0;
    inorderCollect(g_bst_root, arr, &idx);
    quickSortByScore(arr, 0, total - 1);
    assignRanks(arr, total);

    int myIdx = 0;
    for (i = 0; i < total; i++)
        if (strcmp(arr[i]->studentID, newPlayer->studentID) == 0) { myIdx = i; break; }

    writeResultFile(newPlayer, log, numQ, arr[myIdx]->rank, total);
    printResultSummary(newPlayer, arr[myIdx]->rank, total);
    saveLeaderboardReport(arr, total, "Quick Sort");

    free(arr);
    pauseScreen();
}

void handleViewLeaderboard(void) {
    int total = countBST(g_bst_root);
    if (total == 0) {
        printf("\n  >> No players have taken the quiz yet.\n");
        pauseScreen();
        return;
    }

    Player **arr = (Player **)malloc(total * sizeof(Player *));
    int idx = 0;
    inorderCollect(g_bst_root, arr, &idx);

    printf("\n  Choose a sorting algorithm:\n");
    printf("   1. Bubble Sort   [O(n^2)]\n");
    printf("   2. Quick Sort    [O(n log n)]\n");
    printf("   3. Merge Sort    [O(n log n) - stable]\n");
    int choice = getValidatedInt("  Choice: ", 1, 3);

    clock_t start = clock();
    if (choice == 1) bubbleSortByScore(arr, total);
    else if (choice == 2) quickSortByScore(arr, 0, total - 1);
    else mergeSortByScore(arr, 0, total - 1);
    clock_t end = clock();
    double elapsedMs = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;

    assignRanks(arr, total);
    const char *algoName = (choice == 1) ? "Bubble Sort" :
                           (choice == 2) ? "Quick Sort" : "Merge Sort";
    displayLeaderboard(arr, total, algoName, elapsedMs);
    saveLeaderboardReport(arr, total, algoName);

    free(arr);
    pauseScreen();
}

void handleSearchPlayer(void) {
    char id[MAX_ID_LEN];
    printf("\nEnter Student ID to search: ");
    scanf("%14s", id);
    clearInputBuffer();
    displayPlayerRecord(id);
    pauseScreen();
}

/* ================================================================
 *  SECTION 16 : MAIN
 * ================================================================ */
int main(void) {
    int choice;
    srand((unsigned int)time(NULL));
    initHashTable();
    initQuestionBank();
    loadAllPlayers();   /* Populate BST + Hash from file */

    do {
        showMainMenu();
        choice = getValidatedInt("Enter choice (1-4): ", 1, 4);
        switch (choice) {
            case 1: handleTakeQuiz();       break;
            case 2: handleViewLeaderboard(); break;
            case 3: handleSearchPlayer();    break;
            case 4:
                printf("\nThank you for playing %s. Goodbye!\n", APP_TITLE);
                break;
        }
    } while (choice != 4);

    freeQuestionBank();
    freeBST(g_bst_root);
    return 0;
}
