/* ================================================================
 *  PROJECT   : DSA Trivia Challenge - UPGRADED v3 (Category-Based)
 *  COURSE    : Data Structures and Algorithms (DSA)
 *  GROUP     : 05
 *  LANGUAGE  : C (standard C11, no external libraries)
 *
 *  DESCRIPTION
 *  -----------
 *  A console-based multiplayer quiz game demonstrating ADVANCED DSA:
 *
 *   1. SINGLY LINKED LIST   -> Dynamic Question Bank (per category)
 *   2. BINARY SEARCH TREE   -> Player storage ordered by Roll Number
 *   3. HASH TABLE (Chaining)-> O(1) player authentication by Roll
 *   4. MERGE SORT           -> Leaderboard ranking (from scratch)
 *   5. BUBBLE SORT          -> Timing comparison baseline
 *   6. QUICK SORT           -> Timing comparison
 *   7. FISHER-YATES SHUFFLE -> Randomized question order per category
 *   8. COMPETITION RANKING  -> Same score = same rank (1224 style)
 *   9. CATEGORY SYSTEM      -> 5 categories, 2 questions each per player
 *
 *  HOW TO COMPILE & RUN
 *  ---------------------
 *      gcc dsa_trivia_upgraded_v3.c -o dsa_trivia_upgraded_v3
 *      ./dsa_trivia_upgraded_v3
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
#define MAX_ID_LEN        15
#define MAX_DEPT_LEN      10
#define MAX_Q_TEXT        250
#define MAX_OPT_LEN       100
#define HASH_SIZE         101
#define FIXED_QUESTIONS   10       /* 2 from each of 5 categories = 10 */
#define QUESTIONS_PER_CAT 2        /* Questions per category per player */
#define NUM_CATEGORIES    5        /* Total categories */
#define MAX_PLAYERS       200
#define CORRECT_MARK      1.00f
#define WRONG_PENALTY     0.25f
#define SCORES_FILE       "players_scores.txt"
#define LEADERBOARD_FILE  "leaderboard_report.txt"
#define FIELD_DELIM       "|"
#define APP_TITLE         "DSA TRIVIA CHALLENGE - UPGRADED v3"

#ifdef _WIN32
    #define CLEAR_SCREEN "cls"
#else
    #define CLEAR_SCREEN "clear"
#endif

/* ================================================================
 *  SECTION 1 : ENUMS & DATA STRUCTURES
 * ================================================================ */

/* ---- 1.0 Question Categories ---- */
typedef enum {
    CAT_BANGLADESH    = 0,
    CAT_INTERNATIONAL = 1,
    CAT_ENGLISH       = 2,
    CAT_IQ            = 3,
    CAT_CSE           = 4
} QuestionCategory;

/* Category names for display */
const char* CATEGORY_NAMES[] = {
    "Bangladesh",
    "International",
    "English",
    "IQ",
    "CSE"
};

/* ---- 1.1 Linked List Node for Question Bank ---- */
typedef struct QNode {
    int  qno;
    QuestionCategory category;
    char question[MAX_Q_TEXT];
    char options[4][MAX_OPT_LEN];
    int  correctOption;
    struct QNode *next;
} QNode;

/* ---- 1.2 Player Node ---- */
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

    /* Category-wise stats */
    int   cat_correct[NUM_CATEGORIES];
    int   cat_wrong[NUM_CATEGORIES];
    int   cat_skipped[NUM_CATEGORIES];
    float cat_score[NUM_CATEGORIES];

    struct Player *left;
    struct Player *right;
    struct Player *hash_next;
} Player;

/* ---- 1.3 Hash Table ---- */
typedef struct {
    Player *buckets[HASH_SIZE];
} HashTable;

/* ---- 1.4 Answer Record ---- */
typedef struct {
    int questionIndex;
    int selectedOption;
    QuestionCategory category;
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
void  clearInputBuffer(void);
void  pauseScreen(void);
void  printDivider(char ch, int len);
void  printBanner(const char *title);
int   getValidatedInt(const char *prompt, int minVal, int maxVal);
const char* categoryName(QuestionCategory cat);

void  initQuestionBank(void);
void  addQuestion(int qno, QuestionCategory cat, const char *qtext,
                  const char *o1, const char *o2,
                  const char *o3, const char *o4, int correct);
void  freeQuestionBank(void);
int   countQuestions(void);
int   countQuestionsByCategory(QuestionCategory cat);
void  collectQuestionsByCategory(QuestionCategory cat, QNode **arr, int *count);

void  initHashTable(void);
int   hashFunc(const char *studentID);
void  insertHash(Player *p);
Player* searchHash(const char *studentID);

Player* insertBST(Player *root, Player *p);
Player* searchBST(Player *root, const char *studentID);
int   countBST(Player *root);
void  inorderCollect(Player *root, Player **arr, int *idx);
void  freeBST(Player *root);

void  bubbleSortByScore(Player *arr[], int n);
int   partitionByScore(Player *arr[], int low, int high);
void  quickSortByScore(Player *arr[], int low, int high);
void  mergeSortByScore(Player *arr[], int left, int right);
void  merge(Player *arr[], int left, int mid, int right);

void  saveAllPlayers(void);
void  loadAllPlayers(void);
void  appendPlayerScore(const Player *p);
void  saveLeaderboardReport(Player *arr[], int n, const char *algoUsed);

void  shuffleIndices(int arr[], int n);
int   askSingleQuestion(const QNode *q, int qNumber, int totalQ);
void  runQuizSession(Player *p, AnswerRecord log[]);

void  writeResultFile(const Player *p, AnswerRecord log[],
                      int rank, int totalPlayers);
void  printResultSummary(const Player *p, int rank, int totalPlayers);

void  assignCompetitionRanks(Player *arr[], int n);
void  displayLeaderboard(Player *arr[], int n,
                         const char *algoUsed, double elapsedMs);

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

const char* categoryName(QuestionCategory cat) {
    if (cat >= 0 && cat < NUM_CATEGORIES)
        return CATEGORY_NAMES[cat];
    return "Unknown";
}

/* ================================================================
 *  SECTION 5 : QUESTION BANK - LINKED LIST MODULE
 * ================================================================ */
void addQuestion(int qno, QuestionCategory cat, const char *qtext,
                 const char *o1, const char *o2,
                 const char *o3, const char *o4, int correct) {
    QNode *newq = (QNode *)malloc(sizeof(QNode));
    if (!newq) {
        printf("[!] Memory allocation failed for question.\n");
        exit(1);
    }
    newq->qno = qno;
    newq->category = cat;
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

/*
 * ================================================================
 *  SAMPLE QUESTION BANK - 100 QUESTIONS
 *  5 Categories x 20 Questions each
 *
 *  NOTE: These are SAMPLE questions. Your group member should
 *  replace these with actual questions. The module structure
 *  supports exactly 20 questions per category.
 * ================================================================
 */

void initQuestionBank(void) {
    /* ==================== BANGLADESH (20 questions) ==================== */
    addQuestion(1, CAT_BANGLADESH,
        "What is the national flower of Bangladesh?",
        "Water Lily (Shapla)", "Rose", "Marigold", "Sunflower", 0);
    addQuestion(2, CAT_BANGLADESH,
        "In which year did Bangladesh gain independence?",
        "1969", "1970", "1971", "1972", 2);
    addQuestion(3, CAT_BANGLADESH,
        "What is the longest river in Bangladesh?",
        "Meghna", "Jamuna", "Padma", "Surma", 2);
    addQuestion(4, CAT_BANGLADESH,
        "Which is the largest mangrove forest in the world located in Bangladesh?",
        "Lawachara", "Sundarbans", "Bhawal", "Modhupur", 1);
    addQuestion(5, CAT_BANGLADESH,
        "What is the national animal of Bangladesh?",
        "Royal Bengal Tiger", "Asian Elephant", "Clouded Leopard", "Fishing Cat", 0);
    addQuestion(6, CAT_BANGLADESH,
        "Which district is known as the 'Tea Capital' of Bangladesh?",
        "Sylhet", "Moulvibazar", "Habiganj", "Chittagong", 1);
    addQuestion(7, CAT_BANGLADESH,
        "What is the currency of Bangladesh?",
        "Rupee", "Taka", "Rial", "Dinar", 1);
    addQuestion(8, CAT_BANGLADESH,
        "Which is the largest sea beach in the world located in Bangladesh?",
        "Patenga Beach", "St. Martin's Beach", "Cox's Bazar Beach", "Kuakata Beach", 2);
    addQuestion(9, CAT_BANGLADESH,
        "Who is known as the 'Father of the Nation' of Bangladesh?",
        "Huseyn Shaheed Suhrawardy", "Tajuddin Ahmad", "Sheikh Mujibur Rahman", "Ziaur Rahman", 2);
    addQuestion(10, CAT_BANGLADESH,
        "What is the national anthem of Bangladesh written by?",
        "Kazi Nazrul Islam", "Rabindranath Tagore", "Jasimuddin", "Sukanta Bhattacharya", 1);
    addQuestion(11, CAT_BANGLADESH,
        "Which is the largest division of Bangladesh by area?",
        "Dhaka", "Chittagong", "Rajshahi", "Khulna", 1);
    addQuestion(12, CAT_BANGLADESH,
        "What is the main export product of Bangladesh?",
        "Tea", "Jute", "Ready-Made Garments (RMG)", "Leather", 2);
    addQuestion(13, CAT_BANGLADESH,
        "Which river is known as the 'Sorrow of Bengal'?",
        "Padma", "Jamuna", "Meghna", "Damodar", 3);
    addQuestion(14, CAT_BANGLADESH,
        "In which year was the Language Movement in Bangladesh?",
        "1948", "1950", "1952", "1954", 2);
    addQuestion(15, CAT_BANGLADESH,
        "What is the literacy rate of Bangladesh (approximate)?",
        "55%", "65%", "75%", "85%", 2);
    addQuestion(16, CAT_BANGLADESH,
        "Which is the smallest district of Bangladesh by area?",
        "Narayanganj", "Munshiganj", "Meherpur", "Madaripur", 2);
    addQuestion(17, CAT_BANGLADESH,
        "What is the national parliament building of Bangladesh called?",
        "Sangsad Bhaban", "Gono Bhaban", "Bangabhaban", "Secretariat", 0);
    addQuestion(18, CAT_BANGLADESH,
        "Which sport is most popular in Bangladesh?",
        "Football", "Hockey", "Cricket", "Badminton", 2);
    addQuestion(19, CAT_BANGLADESH,
        "What is the total number of districts in Bangladesh?",
        "64", "68", "72", "76", 0);
    addQuestion(20, CAT_BANGLADESH,
        "Which UNESCO World Heritage Site is in Bangladesh?",
        "Lalbagh Fort", "Ahsan Manzil", "Sundarbans", "Sixty Dome Mosque", 2);

    /* ==================== INTERNATIONAL (20 questions) ==================== */
    addQuestion(21, CAT_INTERNATIONAL,
        "Which country has the largest population in the world?",
        "India", "China", "USA", "Indonesia", 1);
    addQuestion(22, CAT_INTERNATIONAL,
        "What is the capital city of Japan?",
        "Osaka", "Kyoto", "Tokyo", "Yokohama", 2);
    addQuestion(23, CAT_INTERNATIONAL,
        "Which planet is known as the 'Red Planet'?",
        "Venus", "Mars", "Jupiter", "Saturn", 1);
    addQuestion(24, CAT_INTERNATIONAL,
        "Who painted the Mona Lisa?",
        "Vincent van Gogh", "Pablo Picasso", "Leonardo da Vinci", "Michelangelo", 2);
    addQuestion(25, CAT_INTERNATIONAL,
        "What is the chemical symbol for gold?",
        "Go", "Gd", "Au", "Ag", 2);
    addQuestion(26, CAT_INTERNATIONAL,
        "Which is the longest river in the world?",
        "Amazon", "Nile", "Yangtze", "Mississippi", 1);
    addQuestion(27, CAT_INTERNATIONAL,
        "In which year did World War II end?",
        "1943", "1944", "1945", "1946", 2);
    addQuestion(28, CAT_INTERNATIONAL,
        "What is the largest ocean on Earth?",
        "Atlantic Ocean", "Indian Ocean", "Arctic Ocean", "Pacific Ocean", 3);
    addQuestion(29, CAT_INTERNATIONAL,
        "Who wrote 'Romeo and Juliet'?",
        "Charles Dickens", "William Shakespeare", "Jane Austen", "Mark Twain", 1);
    addQuestion(30, CAT_INTERNATIONAL,
        "What is the speed of light in vacuum (approx)?",
        "150,000 km/s", "200,000 km/s", "300,000 km/s", "400,000 km/s", 2);
    addQuestion(31, CAT_INTERNATIONAL,
        "Which country is known as the 'Land of the Rising Sun'?",
        "China", "South Korea", "Japan", "Thailand", 2);
    addQuestion(32, CAT_INTERNATIONAL,
        "What is the smallest country in the world by area?",
        "Monaco", "Vatican City", "San Marino", "Liechtenstein", 1);
    addQuestion(33, CAT_INTERNATIONAL,
        "Who invented the telephone?",
        "Thomas Edison", "Nikola Tesla", "Alexander Graham Bell", "Guglielmo Marconi", 2);
    addQuestion(34, CAT_INTERNATIONAL,
        "What is the hardest natural substance on Earth?",
        "Gold", "Iron", "Diamond", "Platinum", 2);
    addQuestion(35, CAT_INTERNATIONAL,
        "Which element has the atomic number 1?",
        "Helium", "Oxygen", "Hydrogen", "Carbon", 2);
    addQuestion(36, CAT_INTERNATIONAL,
        "What is the tallest mountain in the world?",
        "K2", "Mount Kilimanjaro", "Mount Everest", "Mount Fuji", 2);
    addQuestion(37, CAT_INTERNATIONAL,
        "Which country won the FIFA World Cup 2022?",
        "France", "Brazil", "Argentina", "Germany", 2);
    addQuestion(38, CAT_INTERNATIONAL,
        "What is the currency of the United Kingdom?",
        "Euro", "Dollar", "Pound Sterling", "Yen", 2);
    addQuestion(39, CAT_INTERNATIONAL,
        "Who discovered penicillin?",
        "Louis Pasteur", "Alexander Fleming", "Robert Koch", "Joseph Lister", 1);
    addQuestion(40, CAT_INTERNATIONAL,
        "What is the largest desert in the world?",
        "Sahara", "Arabian", "Gobi", "Antarctica", 3);

    /* ==================== ENGLISH (20 questions) ==================== */
    addQuestion(41, CAT_ENGLISH,
        "What is the synonym of 'Abundant'?",
        "Scarce", "Plentiful", "Rare", "Meager", 1);
    addQuestion(42, CAT_ENGLISH,
        "Which of the following is a noun?",
        "Quickly", "Beautiful", "Happiness", "Run", 2);
    addQuestion(43, CAT_ENGLISH,
        "What is the past tense of 'go'?",
        "Gone", "Going", "Went", "Goes", 2);
    addQuestion(44, CAT_ENGLISH,
        "Which sentence is grammatically correct?",
        "She don't like apples.", "She doesn't likes apples.", "She doesn't like apples.", "She not like apples.", 2);
    addQuestion(45, CAT_ENGLISH,
        "What is the antonym of 'Generous'?",
        "Kind", "Stingy", "Helpful", "Charitable", 1);
    addQuestion(46, CAT_ENGLISH,
        "Which word is an adverb?",
        "Quick", "Quickly", "Quickness", "Quicken", 1);
    addQuestion(47, CAT_ENGLISH,
        "What is the plural of 'child'?",
        "Childs", "Children", "Childes", "Childern", 1);
    addQuestion(48, CAT_ENGLISH,
        "Which punctuation mark indicates a question?",
        "Period (.)", "Comma (,)", "Question Mark (?)", "Exclamation Mark (!)", 2);
    addQuestion(49, CAT_ENGLISH,
        "What does the idiom 'break the ice' mean?",
        "To shatter frozen water", "To start a conversation", "To make someone angry", "To win a game", 1);
    addQuestion(50, CAT_ENGLISH,
        "Which is a compound sentence?",
        "I ran.", "I ran and she walked.", "Running fast.", "The running man.", 1);
    addQuestion(51, CAT_ENGLISH,
        "What is the meaning of 'Eloquent'?",
        "Silent", "Fluent in speech", "Angry", "Confused", 1);
    addQuestion(52, CAT_ENGLISH,
        "Which is the correct spelling?",
        "Accomodate", "Accommodate", "Acommodate", "Accomadate", 1);
    addQuestion(53, CAT_ENGLISH,
        "What part of speech is 'however'?",
        "Noun", "Verb", "Conjunction/Adverb", "Preposition", 2);
    addQuestion(54, CAT_ENGLISH,
        "What is the passive voice of 'The cat chased the mouse'?",
        "The mouse chased the cat.", "The mouse was chased by the cat.", "The cat was chasing the mouse.", "The mouse is chasing the cat.", 1);
    addQuestion(55, CAT_ENGLISH,
        "Which word is a homophone of 'right'?",
        "Write", "Rite", "Both write and rite", "None", 2);
    addQuestion(56, CAT_ENGLISH,
        "What is the superlative form of 'good'?",
        "Gooder", "More good", "Best", "Most good", 2);
    addQuestion(57, CAT_ENGLISH,
        "Which sentence uses a metaphor?",
        "He runs like the wind.", "He is a shining star.", "The wind was howling.", "She is as brave as a lion.", 1);
    addQuestion(58, CAT_ENGLISH,
        "What does 'ambiguous' mean?",
        "Clear and definite", "Open to more than one interpretation", "Very loud", "Extremely small", 1);
    addQuestion(59, CAT_ENGLISH,
        "Which is a preposition?",
        "Run", "Under", "Happy", "Quickly", 1);
    addQuestion(60, CAT_ENGLISH,
        "What is the correct article: '___ honest man'?",
        "A", "An", "The", "No article", 1);

    /* ==================== IQ (20 questions) ==================== */
    addQuestion(61, CAT_IQ,
        "If 2+3=10, 7+2=63, 6+5=66, then 8+4=?",
        "96", "32", "48", "72", 0);
    addQuestion(62, CAT_IQ,
        "What comes next in the sequence: 2, 6, 12, 20, 30, ?",
        "36", "40", "42", "44", 2);
    addQuestion(63, CAT_IQ,
        "A farmer has 17 sheep and all but 9 die. How many are left?",
        "8", "9", "17", "0", 1);
    addQuestion(64, CAT_IQ,
        "If you rearrange the letters 'CIFAIPC' you get the name of a:",
        "City", "Animal", "Ocean", "Country", 2);
    addQuestion(65, CAT_IQ,
        "What is the missing number: 1, 1, 2, 3, 5, 8, 13, ?",
        "18", "19", "21", "24", 2);
    addQuestion(66, CAT_IQ,
        "Which number does not belong: 2, 3, 5, 9, 11, 13?",
        "2", "3", "9", "11", 2);
    addQuestion(67, CAT_IQ,
        "If a clock shows 3:15, what is the angle between the hour and minute hands?",
        "0 degrees", "7.5 degrees", "15 degrees", "90 degrees", 1);
    addQuestion(68, CAT_IQ,
        "What is the next letter in the series: O, T, T, F, F, S, S, ?",
        "E", "N", "T", "O", 0);
    addQuestion(69, CAT_IQ,
        "A bat and a ball cost $1.10 in total. The bat costs $1.00 more than the ball. How much does the ball cost?",
        "$0.10", "$0.05", "$0.15", "$0.20", 1);
    addQuestion(70, CAT_IQ,
        "If all Bloops are Razzies and all Razzies are Lazzies, then all Bloops are definitely Lazzies?",
        "True", "False", "Cannot determine", "Maybe", 0);
    addQuestion(71, CAT_IQ,
        "What is the odd one out: Circle, Triangle, Square, Pentagon, Sphere?",
        "Circle", "Triangle", "Square", "Sphere", 3);
    addQuestion(72, CAT_IQ,
        "Complete the analogy: Book is to Reading as Fork is to:",
        "Cooking", "Eating", "Writing", "Cutting", 1);
    addQuestion(73, CAT_IQ,
        "If 5 machines take 5 minutes to make 5 widgets, how long would 100 machines take to make 100 widgets?",
        "100 minutes", "5 minutes", "20 minutes", "1 minute", 1);
    addQuestion(74, CAT_IQ,
        "What is the next number: 1, 4, 9, 16, 25, ?",
        "30", "36", "42", "49", 1);
    addQuestion(75, CAT_IQ,
        "A man pushes his car to a hotel and tells the owner he's bankrupt. Why?",
        "He lost money", "He's playing Monopoly", "His car broke down", "He has no cash", 1);
    addQuestion(76, CAT_IQ,
        "Which shape has the most sides: Hexagon, Octagon, Pentagon, Heptagon?",
        "Hexagon", "Octagon", "Pentagon", "Heptagon", 1);
    addQuestion(77, CAT_IQ,
        "If Mary's mother has 4 daughters: April, May, June, and _____. What is the 4th daughter's name?",
        "July", "Mary", "March", "August", 1);
    addQuestion(78, CAT_IQ,
        "What is half of two plus two?",
        "2", "3", "4", "1", 1);
    addQuestion(79, CAT_IQ,
        "If you have a bowl with six apples and you take away four, how many do you have?",
        "2", "4", "6", "10", 1);
    addQuestion(80, CAT_IQ,
        "What number should replace the question mark: 8, 27, 64, 125, ?",
        "196", "216", "256", "343", 1);

    /* ==================== CSE INFOGRAPHIC (20 questions) ==================== */
    addQuestion(81, CAT_CSE,
        "What is the time complexity of accessing an element in an array by index?",
        "O(n)", "O(log n)", "O(1)", "O(n^2)", 2);
    addQuestion(82, CAT_CSE,
        "Which data structure follows the FIFO principle?",
        "Stack", "Queue", "Tree", "Graph", 1);
    addQuestion(83, CAT_CSE,
        "What does 'HTTP' stand for?",
        "HyperText Transfer Protocol", "HighText Transfer Protocol", "HyperText Transmission Process", "HostText Transfer Protocol", 0);
    addQuestion(84, CAT_CSE,
        "In C programming, what does 'malloc' do?",
        "Free memory", "Allocate memory dynamically", "Copy memory", "Compare memory", 1);
    addQuestion(85, CAT_CSE,
        "Which sorting algorithm has the best average-case time complexity?",
        "Bubble Sort", "Insertion Sort", "Merge Sort", "Selection Sort", 2);
    addQuestion(86, CAT_CSE,
        "What is the default port number for HTTP?",
        "21", "80", "443", "8080", 1);
    addQuestion(87, CAT_CSE,
        "Which of these is NOT a programming paradigm?",
        "Object-Oriented", "Functional", "Procedural", "Alphabetical", 3);
    addQuestion(88, CAT_CSE,
        "What does 'SQL' stand for?",
        "Structured Query Language", "Simple Query Language", "Standard Query Language", "System Query Language", 0);
    addQuestion(89, CAT_CSE,
        "In a binary tree, what is the maximum number of nodes at level 'k'?",
        "k", "2^k", "2^k - 1", "k^2", 1);
    addQuestion(90, CAT_CSE,
        "Which layer of the OSI model handles routing?",
        "Transport Layer", "Network Layer", "Data Link Layer", "Session Layer", 1);
    addQuestion(91, CAT_CSE,
        "What is the output of: printf(\"%d\", 5/2) in C?",
        "2.5", "2", "3", "Error", 1);
    addQuestion(92, CAT_CSE,
        "Which data structure is used for implementing recursion?",
        "Queue", "Stack", "Heap", "Array", 1);
    addQuestion(93, CAT_CSE,
        "What does 'DNS' stand for?",
        "Domain Name System", "Data Network System", "Digital Name Service", "Domain Network Service", 0);
    addQuestion(94, CAT_CSE,
        "Which of these is a lossless compression algorithm?",
        "JPEG", "MP3", "ZIP", "MPEG", 2);
    addQuestion(95, CAT_CSE,
        "What is the space complexity of Merge Sort?",
        "O(1)", "O(log n)", "O(n)", "O(n log n)", 2);
    addQuestion(96, CAT_CSE,
        "In OOP, what is 'polymorphism'?",
        "Hiding data", "Multiple forms of a method", "Inheriting properties", "Creating objects", 1);
    addQuestion(97, CAT_CSE,
        "Which protocol is used for secure web browsing?",
        "HTTP", "FTP", "HTTPS", "SMTP", 2);
    addQuestion(98, CAT_CSE,
        "What is the maximum value of a signed 8-bit integer?",
        "128", "255", "127", "256", 2);
    addQuestion(99, CAT_CSE,
        "Which data structure is best for implementing a priority queue?",
        "Array", "Linked List", "Heap", "Stack", 2);
    addQuestion(100, CAT_CSE,
        "What does 'CPU' stand for?",
        "Central Processing Unit", "Computer Processing Unit", "Central Program Unit", "Core Processing Unit", 0);
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

/* Count questions in a specific category */
int countQuestionsByCategory(QuestionCategory cat) {
    int c = 0;
    QNode *t = g_question_head;
    while (t) {
        if (t->category == cat) c++;
        t = t->next;
    }
    return c;
}

/* Collect all questions of a specific category into an array */
void collectQuestionsByCategory(QuestionCategory cat, QNode **arr, int *count) {
    QNode *t = g_question_head;
    *count = 0;
    while (t != NULL) {
        if (t->category == cat) {
            arr[*count] = t;
            (*count)++;
        }
        t = t->next;
    }
}

/* ================================================================
 *  SECTION 6 : HASH TABLE MODULE (Authentication)
 * ================================================================ */
void initHashTable(void) {
    int i;
    for (i = 0; i < HASH_SIZE; i++) g_ht.buckets[i] = NULL;
}

/* Simple string hash: djb2 algorithm */
int hashFunc(const char *studentID) {
    unsigned long hash = 5381;
    int c;
    while ((c = *studentID++))
        hash = ((hash << 5) + hash) + c;
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
Player* insertBST(Player *root, Player *p) {
    if (root == NULL) return p;
    if (strcmp(p->studentID, root->studentID) < 0)
        root->left = insertBST(root->left, p);
    else if (strcmp(p->studentID, root->studentID) > 0)
        root->right = insertBST(root->right, p);
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
        else {
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
void saveAllPlayers(void) {
    FILE *fp = fopen(SCORES_FILE, "w");
    if (!fp) { printf("[!] Could not save players.\n"); return; }

    int n = countBST(g_bst_root);
    Player **arr = (Player **)malloc(n * sizeof(Player *));
    int idx = 0;
    inorderCollect(g_bst_root, arr, &idx);

    int i, c;
    for (i = 0; i < n; i++) {
        fprintf(fp, "%s|%s|%d|%s|%.2f|%d|%d|%d|%d",
                arr[i]->name, arr[i]->studentID, arr[i]->batch,
                arr[i]->deptCode, arr[i]->score, arr[i]->correct,
                arr[i]->wrong, arr[i]->skipped, arr[i]->totalQuestions);
        for (c = 0; c < NUM_CATEGORIES; c++) {
            fprintf(fp, "|%.2f,%d,%d,%d",
                    arr[i]->cat_score[c], arr[i]->cat_correct[c],
                    arr[i]->cat_wrong[c], arr[i]->cat_skipped[c]);
        }
        fprintf(fp, "\n");
    }
    free(arr);
    fclose(fp);
}

void loadAllPlayers(void) {
    FILE *fp = fopen(SCORES_FILE, "r");
    if (!fp) return;

    char line[800];
    while (fgets(line, sizeof(line), fp)) {
        Player *p = (Player *)malloc(sizeof(Player));
        if (!p) continue;
        p->left = p->right = p->hash_next = NULL;

        char *ptr = line;
        int matched = sscanf(ptr,
            "%49[^|]|%14[^|]|%d|%9[^|]|%f|%d|%d|%d|%d",
            p->name, p->studentID, &p->batch, p->deptCode,
            &p->score, &p->correct, &p->wrong, &p->skipped, &p->totalQuestions);

        if (matched == 9) {
            p->rank = 0;
            int c;
            for (c = 0; c < NUM_CATEGORIES; c++) {
                p->cat_score[c] = 0.0f;
                p->cat_correct[c] = 0;
                p->cat_wrong[c] = 0;
                p->cat_skipped[c] = 0;
            }

            char *cat_ptr = strchr(ptr, '|');
            int field_count = 0;
            while (cat_ptr && field_count < 8) {
                cat_ptr = strchr(cat_ptr + 1, '|');
                field_count++;
            }

            if (cat_ptr) {
                cat_ptr++;
                for (c = 0; c < NUM_CATEGORIES && cat_ptr; c++) {
                    sscanf(cat_ptr, "%f,%d,%d,%d",
                           &p->cat_score[c], &p->cat_correct[c],
                           &p->cat_wrong[c], &p->cat_skipped[c]);
                    cat_ptr = strchr(cat_ptr, '|');
                    if (cat_ptr) cat_ptr++;
                }
            }

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
    fprintf(fp, "%s|%s|%d|%s|%.2f|%d|%d|%d|%d",
            p->name, p->studentID, p->batch, p->deptCode,
            p->score, p->correct, p->wrong, p->skipped, p->totalQuestions);
    int c;
    for (c = 0; c < NUM_CATEGORIES; c++) {
        fprintf(fp, "|%.2f,%d,%d,%d",
                p->cat_score[c], p->cat_correct[c],
                p->cat_wrong[c], p->cat_skipped[c]);
    }
    fprintf(fp, "\n");
    fclose(fp);
}

void saveLeaderboardReport(Player *arr[], int n, const char *algoUsed) {
    FILE *fp = fopen(LEADERBOARD_FILE, "w");
    if (!fp) return;

    int i;
    fprintf(fp, "=============================================\n");
    fprintf(fp, "   %s - LEADERBOARD\n", APP_TITLE);
    fprintf(fp, "   Sorted using: %s\n", algoUsed);
    fprintf(fp, "   Ranking: Competition Ranking (1,2,2,4...)\n");
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

    int c;
    for (c = 0; c < NUM_CATEGORIES; c++) {
        p->cat_score[c] = 0.0f;
        p->cat_correct[c] = 0;
        p->cat_wrong[c] = 0;
        p->cat_skipped[c] = 0;
    }
}

void displayPlayerRecord(const char *studentID) {
    Player *p = searchHash(studentID);
    if (p == NULL) {
        printf("  >> No record found for Student ID: %s\n", studentID);
        return;
    }

    printDivider('-', 50);
    printf("  Name        : %s\n", p->name);
    printf("  Student ID  : %s\n", p->studentID);
    printf("  Batch       : %d\n", p->batch);
    printf("  Department  : %s\n", p->deptCode);
    printf("  Correct     : %d\n", p->correct);
    printf("  Wrong       : %d\n", p->wrong);
    printf("  Skipped     : %d\n", p->skipped);
    printf("  Final Score : %.2f / %d\n", p->score, p->totalQuestions);

    printf("\n  --- Category-wise Performance ---\n");
    int c;
    for (c = 0; c < NUM_CATEGORIES; c++) {
        printf("  %-15s: Correct=%d  Wrong=%d  Skipped=%d  Score=%.2f\n",
               categoryName((QuestionCategory)c),
               p->cat_correct[c], p->cat_wrong[c], p->cat_skipped[c], p->cat_score[c]);
    }

    printDivider('-', 50);
}

/* ================================================================
 *  SECTION 11 : SCORING MODULE
 * ================================================================ */
void applyAnswerResult(Player *p, int isCorrect, QuestionCategory cat) {
    if (isCorrect == 1) {
        p->correct++;
        p->cat_correct[cat]++;
        p->score += CORRECT_MARK;
        p->cat_score[cat] += CORRECT_MARK;
    } else if (isCorrect == 0) {
        p->wrong++;
        p->cat_wrong[cat]++;
        p->score -= WRONG_PENALTY;
        p->cat_score[cat] -= WRONG_PENALTY;
        if (p->score < 0) p->score = 0.0f;
        if (p->cat_score[cat] < 0) p->cat_score[cat] = 0.0f;
    } else {
        p->skipped++;
        p->cat_skipped[cat]++;
    }
}

/* ================================================================
 *  SECTION 12 : QUIZ ENGINE MODULE
 * ================================================================ */
void shuffleIndices(int arr[], int n) {
    int i, j, temp;
    for (i = n - 1; i > 0; i--) {
        j = rand() % (i + 1);
        temp = arr[i]; arr[i] = arr[j]; arr[j] = temp;
    }
}

int askSingleQuestion(const QNode *q, int qNumber, int totalQ) {
    int i, choice;
    printf("\n[Category: %s] Question %d of %d:\n%s\n\n",
           categoryName(q->category), qNumber, totalQ, q->question);
    for (i = 0; i < 4; i++)
        printf("   %d. %s\n", i + 1, q->options[i]);
    choice = getValidatedInt("\nYour answer (1-4, or 0 to skip): ", 0, 4);
    return (choice == 0) ? -1 : (choice - 1);
}

/*
 * UPGRADED: Category-based question selection.
 * Each player gets exactly QUESTIONS_PER_CAT (2) questions from each category.
 * Total = 5 categories x 2 = 10 questions (FIXED_QUESTIONS).
 */
void runQuizSession(Player *p, AnswerRecord log[]) {
    QNode *sessionQuestions[FIXED_QUESTIONS];
    int sessionIdx = 0;
    int cat, i;

    /* For each category, select QUESTIONS_PER_CAT random questions */
    for (cat = 0; cat < NUM_CATEGORIES; cat++) {
        QNode *catQuestions[100];
        int catCount = 0;
        collectQuestionsByCategory((QuestionCategory)cat, catQuestions, &catCount);

        if (catCount < QUESTIONS_PER_CAT) {
            printf("[!] Not enough questions in category '%s' (%d available, %d required).\n",
                   categoryName((QuestionCategory)cat), catCount, QUESTIONS_PER_CAT);
            for (i = 0; i < QUESTIONS_PER_CAT; i++) {
                if (sessionIdx < FIXED_QUESTIONS) {
                    sessionQuestions[sessionIdx++] = NULL;
                }
            }
            continue;
        }

        int *indices = (int *)malloc(catCount * sizeof(int));
        for (i = 0; i < catCount; i++) indices[i] = i;
        shuffleIndices(indices, catCount);

        for (i = 0; i < QUESTIONS_PER_CAT; i++) {
            sessionQuestions[sessionIdx++] = catQuestions[indices[i]];
        }

        free(indices);
    }

    /* Shuffle the session questions to mix categories */
    for (i = FIXED_QUESTIONS - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        QNode *temp = sessionQuestions[i];
        sessionQuestions[i] = sessionQuestions[j];
        sessionQuestions[j] = temp;
    }

    /* Now ask the questions */
    for (i = 0; i < FIXED_QUESTIONS; i++) {
        QNode *q = sessionQuestions[i];
        if (q == NULL) continue;

        int sel = askSingleQuestion(q, i + 1, FIXED_QUESTIONS);

        log[i].questionIndex = q->qno;
        log[i].selectedOption = sel;
        log[i].category = q->category;

        int isCorrect;
        if (sel == -1) isCorrect = -1;
        else if (sel == q->correctOption) isCorrect = 1;
        else isCorrect = 0;

        applyAnswerResult(p, isCorrect, q->category);

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

    for (cat = 0; cat < NUM_CATEGORIES; cat++) {
        p->cat_score[cat] = (p->cat_correct[cat] * CORRECT_MARK) - (p->cat_wrong[cat] * WRONG_PENALTY);
        if (p->cat_score[cat] < 0) p->cat_score[cat] = 0.0f;
    }

    p->totalQuestions = FIXED_QUESTIONS;
}

/* ================================================================
 *  SECTION 13 : RESULT GENERATOR MODULE
 * ================================================================ */
void writeResultFile(const Player *p, AnswerRecord log[],
                     int rank, int totalPlayers) {
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
    for (i = 0; i < FIXED_QUESTIONS; i++) {
        QNode *q = g_question_head;
        while (q != NULL && q->qno != log[i].questionIndex) {
            q = q->next;
        }
        if (!q) continue;

        int sel = log[i].selectedOption;
        const char *marker;
        if (sel == -1) marker = "[-] SKIPPED";
        else if (sel == q->correctOption) marker = "[+] CORRECT";
        else marker = "[x] WRONG  ";

        fprintf(fp, "Q%02d [%s] %s\n", i + 1, categoryName(log[i].category), marker);
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
            p->correct, p->wrong, p->score, FIXED_QUESTIONS);
    fprintf(fp, "Rank    : %d out of %d players\n", rank, totalPlayers);

    fprintf(fp, "\n--- CATEGORY-WISE PERFORMANCE ---\n");
    int c;
    for (c = 0; c < NUM_CATEGORIES; c++) {
        fprintf(fp, "%-15s: Correct=%d  Wrong=%d  Skipped=%d  Score=%.2f\n",
               categoryName((QuestionCategory)c),
               p->cat_correct[c], p->cat_wrong[c], p->cat_skipped[c], p->cat_score[c]);
    }

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

    printf("\n  --- Category-wise Score ---\n");
    int c;
    for (c = 0; c < NUM_CATEGORIES; c++) {
        printf("  %-15s: %.2f\n", categoryName((QuestionCategory)c), p->cat_score[c]);
    }

    printDivider('=', 55);
}

/* ================================================================
 *  SECTION 14 : RANKING MODULE - COMPETITION RANKING (1224)
 * ================================================================ */
void assignCompetitionRanks(Player *arr[], int n) {
    if (n == 0) return;

    int i;
    int currentRank = 1;
    arr[0]->rank = currentRank;

    for (i = 1; i < n; i++) {
        if (arr[i]->score == arr[i - 1]->score) {
            arr[i]->rank = arr[i - 1]->rank;
        } else {
            currentRank = i + 1;
            arr[i]->rank = currentRank;
        }
    }
}

void displayLeaderboard(Player *arr[], int n, const char *algoUsed, double elapsedMs) {
    int i, limit;
    printBanner("LEADERBOARD");
    printf("  Ranking Method : Competition Ranking (1,2,2,4...)\n");
    printf("  Sorted using   : %s   (%.4f ms for %d records)\n\n", algoUsed, elapsedMs, n);
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

    int c;
    for (c = 0; c < NUM_CATEGORIES; c++) {
        newPlayer->cat_score[c] = 0.0f;
        newPlayer->cat_correct[c] = 0;
        newPlayer->cat_wrong[c] = 0;
        newPlayer->cat_skipped[c] = 0;
    }

    int cat;
    for (cat = 0; cat < NUM_CATEGORIES; cat++) {
        int catCount = countQuestionsByCategory((QuestionCategory)cat);
        if (catCount < QUESTIONS_PER_CAT) {
            printf("[!] Not enough questions in category '%s' (%d available, %d required).\n",
                   categoryName((QuestionCategory)cat), catCount, QUESTIONS_PER_CAT);
            free(newPlayer);
            pauseScreen();
            return;
        }
    }

    printf("\n  >> This quiz contains %d questions (2 from each of 5 categories).\n", FIXED_QUESTIONS);
    printf("  >> +1 for correct, -0.25 for wrong.\n");
    printf("  >> Press ENTER to start...");
    clearInputBuffer();

    AnswerRecord log[FIXED_QUESTIONS];
    printBanner("QUIZ STARTED - GOOD LUCK!");
    runQuizSession(newPlayer, log);

    g_bst_root = insertBST(g_bst_root, newPlayer);
    insertHash(newPlayer);

    appendPlayerScore(newPlayer);
    saveAllPlayers();

    int total = countBST(g_bst_root);
    Player **arr = (Player **)malloc(total * sizeof(Player *));
    int idx = 0;
    inorderCollect(g_bst_root, arr, &idx);
    quickSortByScore(arr, 0, total - 1);
    assignCompetitionRanks(arr, total);

    int myIdx = 0;
    for (i = 0; i < total; i++)
        if (strcmp(arr[i]->studentID, newPlayer->studentID) == 0) { myIdx = i; break; }

    writeResultFile(newPlayer, log, arr[myIdx]->rank, total);
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

    assignCompetitionRanks(arr, total);
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
    loadAllPlayers();

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
