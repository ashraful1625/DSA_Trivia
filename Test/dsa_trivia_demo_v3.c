/*
 * DSA QUIZ GAME - C Implementation
 * Data Structures Used:
 *   1. Linked List     : Question bank
 *   2. Binary Search Tree: Player storage by Roll Number
 *   3. Hash Table      : O(1) Player authentication (Separate Chaining)
 *   4. Merge Sort      : Leaderboard ranking (from scratch)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_SIZE     101
#define MAX_NAME      50
#define MAX_DEPT      10
#define MAX_Q_TEXT    256
#define MAX_OPT       100
#define PLAYERS_FILE  "players.dat"
#define LB_FILE       "leaderboard.txt"

/* ======================== DATA STRUCTURES ======================== */

typedef struct QNode {
    int qno;
    char question[MAX_Q_TEXT];
    char opt1[MAX_OPT], opt2[MAX_OPT], opt3[MAX_OPT], opt4[MAX_OPT];
    int correct;                /* 1-4 */
    struct QNode *next;
} QNode;

typedef struct Player {
    long roll;
    char name[MAX_NAME];
    int batch;
    char dept[MAX_DEPT];
    float score;
    struct Player *left;        /* BST links */
    struct Player *right;
    struct Player *hash_next;   /* Hash table chain link */
} Player;

typedef struct {
    Player *buckets[HASH_SIZE];
} HashTable;

/* ======================== GLOBAL ROOTS ======================== */
QNode *question_head = NULL;
Player *bst_root = NULL;
HashTable ht;

/* ======================== FUNCTION PROTOTYPES ======================== */

/* Question Linked List */
void initQuestions();
void addQuestion(int qno, const char *q, const char *o1, const char *o2,
                 const char *o3, const char *o4, int correct);
void freeQuestions();
int countQuestions();

/* Hash Table (Authentication) */
void initHashTable();
int hashFunc(long roll);
void insertHash(Player *p);
Player* searchHash(long roll);

/* Binary Search Tree (Player Storage) */
Player* insertBST(Player *root, Player *p);
Player* searchBST(Player *root, long roll);
int countBST(Player *root);
void inorderCollect(Player *root, Player **arr, int *idx);
void freeBST(Player *root);

/* Merge Sort (Leaderboard) */
void mergeSort(Player **arr, int left, int right);
void merge(Player **arr, int left, int mid, int right);

/* File I/O */
void savePlayers();       /* Persist BST to file */
void loadPlayers();       /* Load from file into BST + Hash */
void saveSession(long roll, int *chosen, int q_count, float score);
void saveLeaderboardFile(Player **arr, int n);

/* Quiz Engine */
float conductQuiz(long roll, int **chosen_answers, int *q_count);
void showLeaderboard();

/* ======================== MAIN ======================== */
int main() {
    int *chosen = NULL;
    int q_count = 0;
    float final_score;
    long roll;
    Player *p;

    /* Initialize core systems */
    initHashTable();
    initQuestions();
    loadPlayers();   /* Load existing players into BST + Hash */

    printf("\n");
    printf("=====================================================\n");
    printf("          DSA QUIZ GAME (C Edition)\n");
    printf("  Linked List | BST | Hash Table | Merge Sort\n");
    printf("=====================================================\n\n");

    /* ---------- AUTHENTICATION (Hash Table O(1)) ---------- */
    printf("Enter University Roll Number: ");
    scanf("%ld", &roll);

    p = searchHash(roll);   /* O(1) average lookup */

    if (p == NULL) {
        /* New player registration */
        p = (Player *)malloc(sizeof(Player));
        if (!p) { printf("Memory error!\n"); return 1; }

        p->roll = roll;
        p->score = 0.0f;
        p->left = p->right = p->hash_next = NULL;

        printf("New Player Detected!\n");
        printf("Enter Name          : ");
        scanf(" %[^\n]", p->name);
        printf("Enter Batch Number  : ");
        scanf("%d", &p->batch);
        printf("Enter Department Code: ");
        scanf("%s", p->dept);

        /* Insert into both DSA structures */
        bst_root = insertBST(bst_root, p);   /* BST for ordered storage */
        insertHash(p);                        /* Hash for O(1) auth */

        printf("\n[+] Registration successful! Welcome, %s.\n\n", p->name);
    } else {
        printf("\n[+] Welcome back, %s! (Batch: %d, Dept: %s)\n\n",
               p->name, p->batch, p->dept);
    }

    /* ---------- QUIZ (Linked List Traversal) ---------- */
    printf("Press Enter to start the quiz...");
    getchar(); getchar();

    final_score = conductQuiz(roll, &chosen, &q_count);

    /* Update score in shared Player object (BST & Hash both see it) */
    p->score = final_score;

    printf("\n=====================================================\n");
    printf("  FINAL SCORE: %.2f / %d\n", final_score, q_count);
    printf("=====================================================\n");

    /* ---------- PERSISTENCE ---------- */
    saveSession(roll, chosen, q_count, final_score);
    savePlayers();

    /* ---------- LEADERBOARD (Merge Sort) ---------- */
    printf("\n");
    showLeaderboard();

    /* ---------- CLEANUP ---------- */
    free(chosen);
    freeQuestions();
    /* Players are freed via BST; hash table only held references */
    freeBST(bst_root);

    printf("\n[*] Session saved. Players data persisted. Goodbye!\n");
    return 0;
}

/* ======================== QUESTION LINKED LIST ======================== */

void addQuestion(int qno, const char *q, const char *o1, const char *o2,
                 const char *o3, const char *o4, int correct) {
    QNode *newq = (QNode *)malloc(sizeof(QNode));
    newq->qno = qno;
    strncpy(newq->question, q, MAX_Q_TEXT - 1);
    newq->question[MAX_Q_TEXT - 1] = '\0';
    strncpy(newq->opt1, o1, MAX_OPT - 1); newq->opt1[MAX_OPT - 1] = '\0';
    strncpy(newq->opt2, o2, MAX_OPT - 1); newq->opt2[MAX_OPT - 1] = '\0';
    strncpy(newq->opt3, o3, MAX_OPT - 1); newq->opt3[MAX_OPT - 1] = '\0';
    strncpy(newq->opt4, o4, MAX_OPT - 1); newq->opt4[MAX_OPT - 1] = '\0';
    newq->correct = correct;
    newq->next = NULL;

    if (question_head == NULL) {
        question_head = newq;
    } else {
        QNode *temp = question_head;
        while (temp->next != NULL) temp = temp->next;
        temp->next = newq;
    }
}

void initQuestions() {
    /* 10 DSA/Programming questions */
    addQuestion(1,
        "Which data structure uses LIFO principle?",
        "Queue", "Stack", "Array", "Linked List", 2);
    addQuestion(2,
        "Time complexity of binary search is?",
        "O(n)", "O(log n)", "O(n^2)", "O(1)", 2);
    addQuestion(3,
        "Which sorting algorithm uses divide and conquer?",
        "Bubble Sort", "Insertion Sort", "Merge Sort", "Selection Sort", 3);
    addQuestion(4,
        "A BST with n nodes has maximum height of?",
        "n", "log n", "n/2", "n-1", 1);
    addQuestion(5,
        "Hash tables handle collisions using?",
        "Pointers", "Recursion", "Chaining/Open Addressing", "Trees", 3);
    addQuestion(6,
        "Which traversal gives sorted order in BST?",
        "Preorder", "Postorder", "Inorder", "Level order", 3);
    addQuestion(7,
        "Worst case time complexity of QuickSort is?",
        "O(n log n)", "O(n^2)", "O(n)", "O(log n)", 2);
    addQuestion(8,
        "A full binary tree with n leaves has how many internal nodes?",
        "n", "n-1", "2n", "n+1", 2);
    addQuestion(9,
        "Pointer to the node before the first node is called?",
        "Tail", "Head", "Root", "Sentinel", 2);
    addQuestion(10,
        "Which is NOT a linear data structure?",
        "Array", "Queue", "Stack", "Binary Tree", 4);
}

void freeQuestions() {
    QNode *temp;
    while (question_head != NULL) {
        temp = question_head;
        question_head = question_head->next;
        free(temp);
    }
}

int countQuestions() {
    int c = 0;
    QNode *t = question_head;
    while (t) { c++; t = t->next; }
    return c;
}

/* ======================== HASH TABLE (AUTH) ======================== */

void initHashTable() {
    int i;
    for (i = 0; i < HASH_SIZE; i++) ht.buckets[i] = NULL;
}

int hashFunc(long roll) {
    return (int)(roll % HASH_SIZE);
}

void insertHash(Player *p) {
    int idx = hashFunc(p->roll);
    p->hash_next = ht.buckets[idx];
    ht.buckets[idx] = p;
}

Player* searchHash(long roll) {
    int idx = hashFunc(roll);
    Player *curr = ht.buckets[idx];
    while (curr != NULL) {
        if (curr->roll == roll) return curr;
        curr = curr->hash_next;
    }
    return NULL;
}

/* ======================== BINARY SEARCH TREE ======================== */

Player* insertBST(Player *root, Player *p) {
    if (root == NULL) return p;
    if (p->roll < root->roll)
        root->left = insertBST(root->left, p);
    else if (p->roll > root->roll)
        root->right = insertBST(root->right, p);
    /* Duplicate roll numbers: update existing (not expected in normal flow) */
    return root;
}

Player* searchBST(Player *root, long roll) {
    if (root == NULL || root->roll == roll) return root;
    if (roll < root->roll) return searchBST(root->left, roll);
    return searchBST(root->right, roll);
}

int countBST(Player *root) {
    if (root == NULL) return 0;
    return 1 + countBST(root->left) + countBST(root->right);
}

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

/* ======================== MERGE SORT (LEADERBOARD) ======================== */

void merge(Player **arr, int left, int mid, int right) {
    int i, j, k;
    int n1 = mid - left + 1;
    int n2 = right - mid;

    Player **L = (Player **)malloc(n1 * sizeof(Player *));
    Player **R = (Player **)malloc(n2 * sizeof(Player *));

    for (i = 0; i < n1; i++) L[i] = arr[left + i];
    for (j = 0; j < n2; j++) R[j] = arr[mid + 1 + j];

    i = 0; j = 0; k = left;

    /* Sort by score DESCENDING. If tie, sort by roll ASCENDING */
    while (i < n1 && j < n2) {
        if (L[i]->score > R[j]->score) {
            arr[k++] = L[i++];
        } else if (L[i]->score < R[j]->score) {
            arr[k++] = R[j++];
        } else {
            if (L[i]->roll < R[j]->roll)
                arr[k++] = L[i++];
            else
                arr[k++] = R[j++];
        }
    }

    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];

    free(L);
    free(R);
}

void mergeSort(Player **arr, int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);
        merge(arr, left, mid, right);
    }
}

/* ======================== FILE I/O ======================== */

void savePlayers() {
    FILE *fp = fopen(PLAYERS_FILE, "w");
    if (!fp) { printf("[!] Warning: Could not save players.\n"); return; }

    int n = countBST(bst_root);
    Player **arr = (Player **)malloc(n * sizeof(Player *));
    int idx = 0;
    inorderCollect(bst_root, arr, &idx);

    for (idx = 0; idx < n; idx++) {
        fprintf(fp, "%ld|%s|%d|%s|%.2f\n",
                arr[idx]->roll, arr[idx]->name, arr[idx]->batch,
                arr[idx]->dept, arr[idx]->score);
    }
    free(arr);
    fclose(fp);
}

void loadPlayers() {
    FILE *fp = fopen(PLAYERS_FILE, "r");
    if (!fp) return; /* No existing data */

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        Player *p = (Player *)malloc(sizeof(Player));
        p->left = p->right = p->hash_next = NULL;

        sscanf(line, "%ld|%[^|]|%d|%[^|]|%f\n",
               &p->roll, p->name, &p->batch, p->dept, &p->score);

        bst_root = insertBST(bst_root, p);
        insertHash(p);
    }
    fclose(fp);
}

void saveSession(long roll, int *chosen, int q_count, float score) {
    char fname[64];
    sprintf(fname, "session_%ld.txt", roll);
    FILE *fp = fopen(fname, "w");
    if (!fp) return;

    Player *p = searchHash(roll);

    fprintf(fp, "========================================\n");
    fprintf(fp, "         QUIZ SESSION REPORT\n");
    fprintf(fp, "========================================\n");
    fprintf(fp, "Player : %s\n", p->name);
    fprintf(fp, "Roll   : %ld\n", p->roll);
    fprintf(fp, "Batch  : %d\n", p->batch);
    fprintf(fp, "Dept   : %s\n", p->dept);
    fprintf(fp, "========================================\n\n");

    QNode *q = question_head;
    int i = 0;
    int correct_count = 0;

    while (q != NULL && i < q_count) {
        int is_correct = (chosen[i] == q->correct);
        if (is_correct) correct_count++;

        fprintf(fp, "Question %d: %s\n", q->qno, q->question);
        fprintf(fp, "  [1] %s\n", q->opt1);
        fprintf(fp, "  [2] %s\n", q->opt2);
        fprintf(fp, "  [3] %s\n", q->opt3);
        fprintf(fp, "  [4] %s\n", q->opt4);
        fprintf(fp, "Correct Answer : %d\n", q->correct);
        fprintf(fp, "Your Answer    : %d\n", chosen[i]);
        fprintf(fp, "Status         : %s\n\n", is_correct ? "CORRECT" : "WRONG");

        q = q->next;
        i++;
    }

    int wrong_count = q_count - correct_count;
    fprintf(fp, "========================================\n");
    fprintf(fp, "              RESULT\n");
    fprintf(fp, "========================================\n");
    fprintf(fp, "Total Questions : %d\n", q_count);
    fprintf(fp, "Correct         : %d\n", correct_count);
    fprintf(fp, "Wrong           : %d\n", wrong_count);
    fprintf(fp, "Marks Obtained  : %.2f\n", score);
    fprintf(fp, "Max Possible    : %d\n", q_count);
    fprintf(fp, "========================================\n");

    fclose(fp);
    printf("[*] Detailed session saved to: %s\n", fname);
}

void saveLeaderboardFile(Player **arr, int n) {
    FILE *fp = fopen(LB_FILE, "w");
    if (!fp) return;

    fprintf(fp, "========================================\n");
    fprintf(fp, "         LEADERBOARD (Ranked)\n");
    fprintf(fp, "========================================\n");
    fprintf(fp, "Rank  Roll\tName\t\tScore\n");
    fprintf(fp, "----------------------------------------\n");

    int i;
    for (i = 0; i < n; i++) {
        fprintf(fp, "#%-4d %-10ld %-15s %.2f\n",
                i + 1, arr[i]->roll, arr[i]->name, arr[i]->score);
    }
    fprintf(fp, "========================================\n");
    fclose(fp);
}

/* ======================== QUIZ ENGINE ======================== */

float conductQuiz(long roll, int **chosen_answers, int *q_count) {
    QNode *q = question_head;
    int n = countQuestions();
    int i = 0;
    int correct = 0;
    int ans;

    *chosen_answers = (int *)malloc(n * sizeof(int));
    *q_count = n;

    printf("\n========== QUIZ STARTED ==========\n\n");

    while (q != NULL) {
        printf("Question %d/%d:\n", i + 1, n);
        printf("%s\n", q->question);
        printf("  [1] %s\n", q->opt1);
        printf("  [2] %s\n", q->opt2);
        printf("  [3] %s\n", q->opt3);
        printf("  [4] %s\n", q->opt4);
        printf("Your answer (1-4): ");

        /* Input validation */
        while (scanf("%d", &ans) != 1 || ans < 1 || ans > 4) {
            printf("Invalid! Enter 1-4: ");
            while (getchar() != '\n'); /* clear buffer */
        }

        (*chosen_answers)[i] = ans;
        if (ans == q->correct) {
            correct++;
            printf("  -> Correct!\n\n");
        } else {
            printf("  -> Wrong! Correct was: %d\n\n", q->correct);
        }

        q = q->next;
        i++;
    }

    int wrong = n - correct;
    float score = (float)correct * 1.0f - (float)wrong * 0.25f;
    if (score < 0) score = 0;
    return score;
}

/* ======================== LEADERBOARD DISPLAY ======================== */

void showLeaderboard() {
    int n = countBST(bst_root);
    if (n == 0) {
        printf("No players yet!\n");
        return;
    }

    Player **arr = (Player **)malloc(n * sizeof(Player *));
    int idx = 0;
    inorderCollect(bst_root, arr, &idx);  /* BST inorder -> array */

    /* Sort using Merge Sort by score (descending) */
    mergeSort(arr, 0, n - 1);

    printf("========================================\n");
    printf("         LEADERBOARD (Top %d)\n", n);
    printf("========================================\n");
    printf("Rank  Roll\tName\t\tScore\n");
    printf("----------------------------------------\n");

    int i;
    for (i = 0; i < n; i++) {
        printf("#%-4d %-10ld %-15s %.2f\n",
               i + 1, arr[i]->roll, arr[i]->name, arr[i]->score);
    }
    printf("========================================\n");

    saveLeaderboardFile(arr, n);
    printf("[*] Leaderboard saved to: %s\n", LB_FILE);

    free(arr);
}
