/* ================================================================================
 *                         UNIVERSITY OF BRAHMANBARIA
 *                   DSA TRIVIA CHALLENGE - MERGED EDITION
 *                       (Data Structures & Algorithms Project)
 * ================================================================================
 *
 *  This build merges two earlier prototypes into a single, cleaner program:
 *
 *  DATA STRUCTURES & ALGORITHMS DEMONSTRATED
 *  -------------------------------------------------------------------------
 *   1. Singly Linked List   -> Dynamic question bank (100 MCQs, 5 categories)
 *   2. Hash Table (Chaining)-> DJB2 string hashing, O(1) avg. ID lookup
 *   3. Binary Search Tree   -> Player records indexed by Student/Roll ID
 *   4. Bubble Sort          -> O(n^2) leaderboard sort (baseline)
 *   5. Quick Sort           -> O(n log n) leaderboard sort
 *   6. Merge Sort           -> O(n log n) stable leaderboard sort
 *   7. Fisher-Yates Shuffle -> Fair random question selection per category
 *   8. Roll/ID Decoder      -> Parses 8-digit IDs into Year / Semester / Serial
 *   9. Fair Ranking Score   -> Accuracy-weighted composite performance metric
 *  10. Competition Ranking  -> Tied scores share a rank (1, 2, 2, 4 style)
 *
 *  FILES CREATED AT RUNTIME
 *  -------------------------------------------------------------------------
 *      players_scores.txt      -> full player database (rewritten each save)
 *      result_<StudentID>.txt  -> one detailed transcript per completed quiz
 *      leaderboard_report.txt  -> latest leaderboard snapshot (text table)
 *
 *  COMPILE
 *  -------------------------------------------------------------------------
 *      gcc -Wall -Wextra -std=c11 dsa_trivia_merged.c -o dsa_trivia_merged
 *      ./dsa_trivia_merged
 *
 * ================================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* ================================================================================
 *  SECTION 0 : CONFIGURATION CONSTANTS
 * ================================================================================ */
#define MAX_NAME_LEN      50
#define MAX_ID_LEN        32
#define MAX_BATCH_LEN     20
#define MAX_DEPT_LEN      16
#define MAX_Q_TEXT        256
#define MAX_OPT_LEN       128
#define HASH_SIZE         101
#define FIXED_QUESTIONS   10       /* 2 questions x 5 categories per attempt   */
#define QUESTIONS_PER_CAT 2
#define NUM_CATEGORIES    5
#define CORRECT_MARK      1.00
#define WRONG_PENALTY     0.25
#define SCORES_FILE       "players_scores.txt"
#define LEADERBOARD_FILE  "leaderboard_report.txt"
#define APP_TITLE         "DSA TRIVIA CHALLENGE"
#define UNI_LINE1         "UNIVERSITY OF BRAHMANBARIA"
#define BOX_WIDTH         82

#ifdef _WIN32
    #define CLEAR_SCREEN "cls"
#else
    #define CLEAR_SCREEN "clear"
#endif

/* ---- ANSI colour codes (safely ignored by terminals that don't support them) --- */
#define CLR_RESET   "\x1b[0m"
#define CLR_BOLD    "\x1b[1m"
#define CLR_DIM     "\x1b[2m"
#define CLR_CYAN    "\x1b[36m"
#define CLR_GREEN   "\x1b[32m"
#define CLR_RED     "\x1b[31m"
#define CLR_YELLOW  "\x1b[33m"
#define CLR_BLUE    "\x1b[34m"
#define CLR_MAGENTA "\x1b[35m"
#define CLR_WHITE   "\x1b[97m"

/* ================================================================================
 *  SECTION 1 : ENUMS & DATA STRUCTURES
 * ================================================================================ */

typedef enum {
    CAT_BANGLADESH    = 0,
    CAT_INTERNATIONAL = 1,
    CAT_ENGLISH       = 2,
    CAT_IQ            = 3,
    CAT_CSE           = 4
} QuestionCategory;

static const char* CATEGORY_NAMES[NUM_CATEGORIES] = {
    "Bangladesh", "International", "English", "IQ", "CSE"
};

/* ---- 1.1 Question Bank Node (Singly Linked List) ---- */
typedef struct QNode {
    int  qno;
    QuestionCategory category;
    char question[MAX_Q_TEXT];
    char options[4][MAX_OPT_LEN];
    int  correct_option;        /* 1, 2, 3, or 4 (human friendly 1-based) */
    struct QNode* next;
} QNode;

/* ---- 1.2 Player Record : doubles as BST node AND Hash node (single alloc) ---- */
typedef struct Player {
    char name[MAX_NAME_LEN];
    char id[MAX_ID_LEN];            /* Student / University Roll Number   */
    char batch[MAX_BATCH_LEN];      /* e.g. "2025 Fall" (decoded or manual) */
    char dept[MAX_DEPT_LEN];

    int total_questions;
    int attempted;
    int correct;
    int wrong;
    int skipped;

    double raw_score;               /* clamped >= 0 : (+1.00/-0.25 marking) */
    double accuracy;                /* correct / attempted * 100            */
    double ranking_score;           /* accuracy-weighted fair composite     */
    int rank;                       /* competition rank (1,2,2,4 style)     */

    int   cat_correct[NUM_CATEGORIES];
    int   cat_wrong[NUM_CATEGORIES];
    int   cat_skipped[NUM_CATEGORIES];
    double cat_score[NUM_CATEGORIES];

    struct Player* left;
    struct Player* right;
    struct Player* hash_next;
} Player;

/* ---- 1.3 Hash Table (buckets point directly into the BST's Player nodes) ---- */
typedef struct {
    Player* buckets[HASH_SIZE];
} HashTable;

/* ---- 1.4 Per-question answer log for transcript generation ---- */
typedef struct {
    int questionId;
    int selectedOption;   /* 0 = skipped, else 1-4 */
    QuestionCategory category;
} AnswerRecord;

/* ================================================================================
 *  SECTION 2 : GLOBAL STATE
 * ================================================================================ */
static QNode*    g_question_head = NULL;
static Player*   g_bst_root      = NULL;
static HashTable g_ht;

/* ================================================================================
 *  SECTION 3 : FUNCTION PROTOTYPES
 * ================================================================================ */

/* -- UI utilities -- */
void   clear_screen(void);
void   pause_prompt(void);
void   print_divider(char ch, int len);
void   print_banner(const char* line1, const char* line2);
void   print_box_top(void);
void   print_box_bottom(void);
void   print_box_title(const char* text);
void   print_box_row(const char* label, const char* value);
void   print_fixed_row(const char* content, const char* color);
void   draw_progress_bar(int current, int total, char* out, int out_len);
void   read_string_input(const char* prompt, char* buffer, int max_len);
int    read_integer_input(const char* prompt, int min_val, int max_val);
const char* category_name(QuestionCategory cat);

/* -- Roll / ID decoder -- */
void   decode_roll_no(const char* id, char* batch_out, int batch_len,
                       char* info_out, int info_len);

/* -- Question bank (Linked List) -- */
void   add_question(int id, QuestionCategory cat, const char* qtext,
                     const char* o1, const char* o2, const char* o3,
                     const char* o4, int correct_option);
void   init_question_bank(void);
void   free_question_bank(void);
int    count_questions(void);
int    count_questions_by_category(QuestionCategory cat);
void   collect_questions_by_category(QuestionCategory cat, QNode** arr, int* count);
void   display_question_bank(void);

/* -- Hash table -- */
void   init_hash_table(void);
unsigned long hash_function(const char* str);
void   hash_insert(Player* p);
Player* hash_lookup(const char* id);

/* -- Binary Search Tree -- */
Player* bst_insert(Player* root, Player* p);
Player* bst_search(Player* root, const char* id);
int     bst_count(const Player* root);
void    bst_inorder_collect(Player* root, Player** arr, int* idx);
void    bst_free(Player* root);

/* -- Sorting algorithms (leaderboard ranking) -- */
int    compare_players(const Player* a, const Player* b);
void   bubble_sort_players(Player* arr[], int n);
int    partition_players(Player* arr[], int low, int high);
void   quick_sort_players(Player* arr[], int low, int high);
void   merge_players(Player* arr[], int left, int mid, int right);
void   merge_sort_players(Player* arr[], int left, int right);
void   assign_competition_ranks(Player* arr[], int n);

/* -- Scoring -- */
void   apply_answer_result(Player* p, int status, QuestionCategory cat);
void   finalize_scores(Player* p);

/* -- File I/O -- */
void   save_all_players(void);
void   load_all_players(void);
void   export_leaderboard_report(Player* arr[], int n, const char* algo_used, double elapsed_ms);
void   write_result_file(const Player* p, AnswerRecord log[], int total_players);

/* -- Quiz engine -- */
void   shuffle_indices(int arr[], int n);
int    ask_single_question(const QNode* q, int q_number, int total_q,
                            const Player* p);
void   run_quiz_session(Player* p, AnswerRecord log[]);

/* -- Menu handlers -- */
void   show_main_menu(int q_count, int player_count);
void   handle_take_quiz(void);
void   handle_view_leaderboard(void);
void   handle_search_player(void);
void   handle_browse_questions(void);
void   display_player_card(const Player* p);

/* ================================================================================
 *  SECTION 4 : UI UTILITIES MODULE
 * ================================================================================ */

void clear_screen(void) {
    system(CLEAR_SCREEN);
}

void pause_prompt(void) {
    printf("\n" CLR_DIM "  >> Press ENTER to continue..." CLR_RESET);
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF && c != '\r') {}
}

void print_divider(char ch, int len) {
    printf(CLR_CYAN);
    for (int i = 0; i < len; i++) putchar(ch);
    printf(CLR_RESET "\n");
}

/* Draws the standard two-line boxed banner used on every major screen */
void print_banner(const char* line1, const char* line2) {
    print_box_top();
    print_box_title(line1);
    if (line2 && line2[0] != '\0') print_box_title(line2);
    print_box_bottom();
}

void print_box_top(void) {
    printf(CLR_CYAN "+");
    for (int i = 0; i < BOX_WIDTH - 2; i++) putchar('=');
    printf("+" CLR_RESET "\n");
}

void print_box_bottom(void) {
    printf(CLR_CYAN "+");
    for (int i = 0; i < BOX_WIDTH - 2; i++) putchar('=');
    printf("+" CLR_RESET "\n");
}

/* Centers a line of text inside the box width */
void print_box_title(const char* text) {
    int len = (int)strlen(text);
    int inner = BOX_WIDTH - 2;
    int left_pad = (inner - len) / 2;
    if (left_pad < 0) left_pad = 0;
    int right_pad = inner - len - left_pad;
    if (right_pad < 0) right_pad = 0;
    printf(CLR_CYAN "|" CLR_RESET CLR_BOLD CLR_WHITE);
    for (int i = 0; i < left_pad; i++) putchar(' ');
    printf("%s", text);
    for (int i = 0; i < right_pad; i++) putchar(' ');
    printf(CLR_RESET CLR_CYAN "|" CLR_RESET "\n");
}

/* A single "label : value" row inside a light card border. Width is
   computed from plain (uncoloured) text so borders always line up
   regardless of embedded ANSI colour codes.                         */
void print_box_row(const char* label, const char* value) {
    char plain[BOX_WIDTH + 8];
    snprintf(plain, sizeof(plain), " %-22s : %-50s", label, value);
    int inner = BOX_WIDTH - 2;
    int len = (int)strlen(plain);
    printf(CLR_CYAN "|" CLR_RESET CLR_YELLOW " %-22s" CLR_RESET " : " CLR_WHITE "%-50s" CLR_RESET, label, value);
    for (int i = len; i < inner; i++) putchar(' ');
    printf(CLR_CYAN "|" CLR_RESET "\n");
}

/* Prints one plain (uncoloured-length) row padded to the box's inner width,
   then wraps it in a single colour for the whole line. Used for menu items
   and other rows where per-substring colouring would break alignment.     */
void print_fixed_row(const char* content, const char* color) {
    int inner = BOX_WIDTH - 2;
    int len = (int)strlen(content);
    printf(CLR_CYAN "|" CLR_RESET "%s%s" CLR_RESET, color ? color : "", content);
    for (int i = len; i < inner; i++) putchar(' ');
    printf(CLR_CYAN "|" CLR_RESET "\n");
}

/* Builds a [====>-----] NN%% style progress bar into out */
void draw_progress_bar(int current, int total, char* out, int out_len) {
    const int bar_len = 20;
    int filled = (int)(((double)current / (double)total) * bar_len);
    if (filled > bar_len) filled = bar_len;
    int pos = 0;
    for (int i = 0; i < bar_len && pos < out_len - 1; i++) {
        if (i < filled - 1)      out[pos++] = '=';
        else if (i == filled - 1) out[pos++] = '>';
        else                       out[pos++] = '-';
    }
    out[pos] = '\0';
}

void read_string_input(const char* prompt, char* buffer, int max_len) {
    while (1) {
        printf(CLR_GREEN "%s" CLR_RESET, prompt);
        fflush(stdout);
        if (fgets(buffer, max_len, stdin) != NULL) {
            size_t len = strlen(buffer);
            while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) buffer[--len] = '\0';
            if (len > 0) return;
            printf(CLR_RED "  [!] Input cannot be blank. Please try again.\n" CLR_RESET);
        } else {
            clearerr(stdin);
        }
    }
}

/* Robust integer reader: rejects blank input, non-numeric input, AND
   trailing-garbage input like "5x" (unlike a bare scanf("%d")).      */
int read_integer_input(const char* prompt, int min_val, int max_val) {
    char buffer[64];
    int value;
    while (1) {
        printf(CLR_GREEN "%s" CLR_RESET, prompt);
        fflush(stdout);
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            char extra;
            if (sscanf(buffer, "%d %c", &value, &extra) == 1) {
                if (value >= min_val && value <= max_val) return value;
                printf(CLR_RED "  [!] Enter a number between %d and %d.\n" CLR_RESET, min_val, max_val);
            } else {
                printf(CLR_RED "  [!] Invalid input! Please enter a valid whole number.\n" CLR_RESET);
            }
        } else {
            clearerr(stdin);
        }
    }
}

const char* category_name(QuestionCategory cat) {
    if (cat >= 0 && cat < NUM_CATEGORIES) return CATEGORY_NAMES[cat];
    return "Unknown";
}

/* ================================================================================
 *  SECTION 5 : ROLL / STUDENT ID DECODER
 *  Parses an 8-digit roll number (e.g. 25201000) into:
 *      Admission Year | Semester (Spring/Fall) | Serial Number
 *  Non-8-digit IDs are left as free-form (no decoding attempted).
 * ================================================================================ */
void decode_roll_no(const char* id, char* batch_out, int batch_len,
                     char* info_out, int info_len) {
    if (id && strlen(id) == 8 &&
        isdigit((unsigned char)id[0]) && isdigit((unsigned char)id[1]) && isdigit((unsigned char)id[2])) {

        int year = (id[0] - '0') * 10 + (id[1] - '0') + 2000;
        char sem_code = id[2];
        const char* sem_name = (sem_code == '1') ? "Spring" :
                                (sem_code == '2') ? "Fall" : "Semester";

        if (batch_out && batch_len > 0) {
            snprintf(batch_out, batch_len, "%d %s", year, sem_name);
        }
        if (info_out && info_len > 0) {
            snprintf(info_out, info_len, "Admission Year: %d | %s Semester | Serial: %s",
                      year, sem_name, id + 3);
        }
    } else {
        if (info_out && info_len > 0) info_out[0] = '\0';
    }
}

/* ================================================================================
 *  SECTION 6 : QUESTION BANK MODULE (Singly Linked List)
 * ================================================================================ */
void add_question(int id, QuestionCategory cat, const char* qtext,
                   const char* o1, const char* o2, const char* o3,
                   const char* o4, int correct_option) {
    QNode* newq = (QNode*)malloc(sizeof(QNode));
    if (!newq) { printf("[!] Memory allocation failed for question.\n"); exit(1); }

    newq->qno = id;
    newq->category = cat;
    strncpy(newq->question, qtext, MAX_Q_TEXT - 1);
    newq->question[MAX_Q_TEXT - 1] = '\0';
    strncpy(newq->options[0], o1, MAX_OPT_LEN - 1); newq->options[0][MAX_OPT_LEN - 1] = '\0';
    strncpy(newq->options[1], o2, MAX_OPT_LEN - 1); newq->options[1][MAX_OPT_LEN - 1] = '\0';
    strncpy(newq->options[2], o3, MAX_OPT_LEN - 1); newq->options[2][MAX_OPT_LEN - 1] = '\0';
    strncpy(newq->options[3], o4, MAX_OPT_LEN - 1); newq->options[3][MAX_OPT_LEN - 1] = '\0';
    newq->correct_option = correct_option;
    newq->next = NULL;

    if (g_question_head == NULL) {
        g_question_head = newq;
    } else {
        QNode* temp = g_question_head;
        while (temp->next != NULL) temp = temp->next;
        temp->next = newq;
    }
}

/* 100 questions, 5 categories x 20 each. correct_option is 1-based (1-4). */
void init_question_bank(void) {
    add_question(1, CAT_BANGLADESH,
        "What is the national flower of Bangladesh?",
        "Water Lily (Shapla)", "Rose", "Marigold", "Sunflower", 1);
    add_question(2, CAT_BANGLADESH,
        "In which year did Bangladesh gain independence?",
        "1969", "1970", "1971", "1972", 3);
    add_question(3, CAT_BANGLADESH,
        "What is the longest river in Bangladesh?",
        "Meghna", "Jamuna", "Padma", "Surma", 3);
    add_question(4, CAT_BANGLADESH,
        "Which is the largest mangrove forest in the world located in Bangladesh?",
        "Lawachara", "Sundarbans", "Bhawal", "Modhupur", 2);
    add_question(5, CAT_BANGLADESH,
        "What is the national animal of Bangladesh?",
        "Royal Bengal Tiger", "Asian Elephant", "Clouded Leopard", "Fishing Cat", 1);
    add_question(6, CAT_BANGLADESH,
        "Which district is known as the 'Tea Capital' of Bangladesh?",
        "Sylhet", "Moulvibazar", "Habiganj", "Chittagong", 2);
    add_question(7, CAT_BANGLADESH,
        "What is the currency of Bangladesh?",
        "Rupee", "Taka", "Rial", "Dinar", 2);
    add_question(8, CAT_BANGLADESH,
        "Which is the largest sea beach in the world located in Bangladesh?",
        "Patenga Beach", "St. Martin's Beach", "Cox's Bazar Beach", "Kuakata Beach", 3);
    add_question(9, CAT_BANGLADESH,
        "Who is known as the 'Father of the Nation' of Bangladesh?",
        "Huseyn Shaheed Suhrawardy", "Tajuddin Ahmad", "Sheikh Mujibur Rahman", "Ziaur Rahman", 3);
    add_question(10, CAT_BANGLADESH,
        "What is the national anthem of Bangladesh written by?",
        "Kazi Nazrul Islam", "Rabindranath Tagore", "Jasimuddin", "Sukanta Bhattacharya", 2);
    add_question(11, CAT_BANGLADESH,
        "Which is the largest division of Bangladesh by area?",
        "Dhaka", "Chittagong", "Rajshahi", "Khulna", 2);
    add_question(12, CAT_BANGLADESH,
        "What is the main export product of Bangladesh?",
        "Tea", "Jute", "Ready-Made Garments (RMG)", "Leather", 3);
    add_question(13, CAT_BANGLADESH,
        "Which river is known as the 'Sorrow of Bengal'?",
        "Padma", "Jamuna", "Meghna", "Damodar", 4);
    add_question(14, CAT_BANGLADESH,
        "In which year was the Language Movement in Bangladesh?",
        "1948", "1950", "1952", "1954", 3);
    add_question(15, CAT_BANGLADESH,
        "What is the literacy rate of Bangladesh (approximate)?",
        "55%", "65%", "75%", "85%", 3);
    add_question(16, CAT_BANGLADESH,
        "Which is the smallest district of Bangladesh by area?",
        "Narayanganj", "Munshiganj", "Meherpur", "Madaripur", 3);
    add_question(17, CAT_BANGLADESH,
        "What is the national parliament building of Bangladesh called?",
        "Sangsad Bhaban", "Gono Bhaban", "Bangabhaban", "Secretariat", 1);
    add_question(18, CAT_BANGLADESH,
        "Which sport is most popular in Bangladesh?",
        "Football", "Hockey", "Cricket", "Badminton", 3);
    add_question(19, CAT_BANGLADESH,
        "What is the total number of districts in Bangladesh?",
        "64", "68", "72", "76", 1);
    add_question(20, CAT_BANGLADESH,
        "Which UNESCO World Heritage Site is in Bangladesh?",
        "Lalbagh Fort", "Ahsan Manzil", "Sundarbans", "Sixty Dome Mosque", 3);
    add_question(21, CAT_INTERNATIONAL,
        "Which country has the largest population in the world?",
        "India", "China", "USA", "Indonesia", 2);
    add_question(22, CAT_INTERNATIONAL,
        "What is the capital city of Japan?",
        "Osaka", "Kyoto", "Tokyo", "Yokohama", 3);
    add_question(23, CAT_INTERNATIONAL,
        "Which planet is known as the 'Red Planet'?",
        "Venus", "Mars", "Jupiter", "Saturn", 2);
    add_question(24, CAT_INTERNATIONAL,
        "Who painted the Mona Lisa?",
        "Vincent van Gogh", "Pablo Picasso", "Leonardo da Vinci", "Michelangelo", 3);
    add_question(25, CAT_INTERNATIONAL,
        "What is the chemical symbol for gold?",
        "Go", "Gd", "Au", "Ag", 3);
    add_question(26, CAT_INTERNATIONAL,
        "Which is the longest river in the world?",
        "Amazon", "Nile", "Yangtze", "Mississippi", 2);
    add_question(27, CAT_INTERNATIONAL,
        "In which year did World War II end?",
        "1943", "1944", "1945", "1946", 3);
    add_question(28, CAT_INTERNATIONAL,
        "What is the largest ocean on Earth?",
        "Atlantic Ocean", "Indian Ocean", "Arctic Ocean", "Pacific Ocean", 4);
    add_question(29, CAT_INTERNATIONAL,
        "Who wrote 'Romeo and Juliet'?",
        "Charles Dickens", "William Shakespeare", "Jane Austen", "Mark Twain", 2);
    add_question(30, CAT_INTERNATIONAL,
        "What is the speed of light in vacuum (approx)?",
        "150,000 km/s", "200,000 km/s", "300,000 km/s", "400,000 km/s", 3);
    add_question(31, CAT_INTERNATIONAL,
        "Which country is known as the 'Land of the Rising Sun'?",
        "China", "South Korea", "Japan", "Thailand", 3);
    add_question(32, CAT_INTERNATIONAL,
        "What is the smallest country in the world by area?",
        "Monaco", "Vatican City", "San Marino", "Liechtenstein", 2);
    add_question(33, CAT_INTERNATIONAL,
        "Who invented the telephone?",
        "Thomas Edison", "Nikola Tesla", "Alexander Graham Bell", "Guglielmo Marconi", 3);
    add_question(34, CAT_INTERNATIONAL,
        "What is the hardest natural substance on Earth?",
        "Gold", "Iron", "Diamond", "Platinum", 3);
    add_question(35, CAT_INTERNATIONAL,
        "Which element has the atomic number 1?",
        "Helium", "Oxygen", "Hydrogen", "Carbon", 3);
    add_question(36, CAT_INTERNATIONAL,
        "What is the tallest mountain in the world?",
        "K2", "Mount Kilimanjaro", "Mount Everest", "Mount Fuji", 3);
    add_question(37, CAT_INTERNATIONAL,
        "Which country won the FIFA World Cup 2022?",
        "France", "Brazil", "Argentina", "Germany", 3);
    add_question(38, CAT_INTERNATIONAL,
        "What is the currency of the United Kingdom?",
        "Euro", "Dollar", "Pound Sterling", "Yen", 3);
    add_question(39, CAT_INTERNATIONAL,
        "Who discovered penicillin?",
        "Louis Pasteur", "Alexander Fleming", "Robert Koch", "Joseph Lister", 2);
    add_question(40, CAT_INTERNATIONAL,
        "What is the largest desert in the world?",
        "Sahara", "Arabian", "Gobi", "Antarctica", 4);
    add_question(41, CAT_ENGLISH,
        "What is the synonym of 'Abundant'?",
        "Scarce", "Plentiful", "Rare", "Meager", 2);
    add_question(42, CAT_ENGLISH,
        "Which of the following is a noun?",
        "Quickly", "Beautiful", "Happiness", "Run", 3);
    add_question(43, CAT_ENGLISH,
        "What is the past tense of 'go'?",
        "Gone", "Going", "Went", "Goes", 3);
    add_question(44, CAT_ENGLISH,
        "Which sentence is grammatically correct?",
        "She don't like apples.", "She doesn't likes apples.", "She doesn't like apples.", "She not like apples.", 3);
    add_question(45, CAT_ENGLISH,
        "What is the antonym of 'Generous'?",
        "Kind", "Stingy", "Helpful", "Charitable", 2);
    add_question(46, CAT_ENGLISH,
        "Which word is an adverb?",
        "Quick", "Quickly", "Quickness", "Quicken", 2);
    add_question(47, CAT_ENGLISH,
        "What is the plural of 'child'?",
        "Childs", "Children", "Childes", "Childern", 2);
    add_question(48, CAT_ENGLISH,
        "Which punctuation mark indicates a question?",
        "Period (.)", "Comma (,)", "Question Mark (?)", "Exclamation Mark (!)", 3);
    add_question(49, CAT_ENGLISH,
        "What does the idiom 'break the ice' mean?",
        "To shatter frozen water", "To start a conversation", "To make someone angry", "To win a game", 2);
    add_question(50, CAT_ENGLISH,
        "Which is a compound sentence?",
        "I ran.", "I ran and she walked.", "Running fast.", "The running man.", 2);
    add_question(51, CAT_ENGLISH,
        "What is the meaning of 'Eloquent'?",
        "Silent", "Fluent in speech", "Angry", "Confused", 2);
    add_question(52, CAT_ENGLISH,
        "Which is the correct spelling?",
        "Accomodate", "Accommodate", "Acommodate", "Accomadate", 2);
    add_question(53, CAT_ENGLISH,
        "What part of speech is 'however'?",
        "Noun", "Verb", "Conjunction/Adverb", "Preposition", 3);
    add_question(54, CAT_ENGLISH,
        "What is the passive voice of 'The cat chased the mouse'?",
        "The mouse chased the cat.", "The mouse was chased by the cat.", "The cat was chasing the mouse.", "The mouse is chasing the cat.", 2);
    add_question(55, CAT_ENGLISH,
        "Which word is a homophone of 'right'?",
        "Write", "Rite", "Both write and rite", "None", 3);
    add_question(56, CAT_ENGLISH,
        "What is the superlative form of 'good'?",
        "Gooder", "More good", "Best", "Most good", 3);
    add_question(57, CAT_ENGLISH,
        "Which sentence uses a metaphor?",
        "He runs like the wind.", "He is a shining star.", "The wind was howling.", "She is as brave as a lion.", 2);
    add_question(58, CAT_ENGLISH,
        "What does 'ambiguous' mean?",
        "Clear and definite", "Open to more than one interpretation", "Very loud", "Extremely small", 2);
    add_question(59, CAT_ENGLISH,
        "Which is a preposition?",
        "Run", "Under", "Happy", "Quickly", 2);
    add_question(60, CAT_ENGLISH,
        "What is the correct article: '___ honest man'?",
        "A", "An", "The", "No article", 2);
    add_question(61, CAT_IQ,
        "If 2+3=10, 7+2=63, 6+5=66, then 8+4=?",
        "96", "32", "48", "72", 1);
    add_question(62, CAT_IQ,
        "What comes next in the sequence: 2, 6, 12, 20, 30, ?",
        "36", "40", "42", "44", 3);
    add_question(63, CAT_IQ,
        "A farmer has 17 sheep and all but 9 die. How many are left?",
        "8", "9", "17", "0", 2);
    add_question(64, CAT_IQ,
        "If you rearrange the letters 'CIFAIPC' you get the name of a:",
        "City", "Animal", "Ocean", "Country", 3);
    add_question(65, CAT_IQ,
        "What is the missing number: 1, 1, 2, 3, 5, 8, 13, ?",
        "18", "19", "21", "24", 3);
    add_question(66, CAT_IQ,
        "Which number does not belong: 2, 3, 5, 9, 11, 13?",
        "2", "3", "9", "11", 3);
    add_question(67, CAT_IQ,
        "If a clock shows 3:15, what is the angle between the hour and minute hands?",
        "0 degrees", "7.5 degrees", "15 degrees", "90 degrees", 2);
    add_question(68, CAT_IQ,
        "What is the next letter in the series: O, T, T, F, F, S, S, ?",
        "E", "N", "T", "O", 1);
    add_question(69, CAT_IQ,
        "A bat and a ball cost $1.10 in total. The bat costs $1.00 more than the ball. How much does the ball cost?",
        "$0.10", "$0.05", "$0.15", "$0.20", 2);
    add_question(70, CAT_IQ,
        "If all Bloops are Razzies and all Razzies are Lazzies, then all Bloops are definitely Lazzies?",
        "True", "False", "Cannot determine", "Maybe", 1);
    add_question(71, CAT_IQ,
        "What is the odd one out: Circle, Triangle, Square, Pentagon, Sphere?",
        "Circle", "Triangle", "Square", "Sphere", 4);
    add_question(72, CAT_IQ,
        "Complete the analogy: Book is to Reading as Fork is to:",
        "Cooking", "Eating", "Writing", "Cutting", 2);
    add_question(73, CAT_IQ,
        "If 5 machines take 5 minutes to make 5 widgets, how long would 100 machines take to make 100 widgets?",
        "100 minutes", "5 minutes", "20 minutes", "1 minute", 2);
    add_question(74, CAT_IQ,
        "What is the next number: 1, 4, 9, 16, 25, ?",
        "30", "36", "42", "49", 2);
    add_question(75, CAT_IQ,
        "A man pushes his car to a hotel and tells the owner he's bankrupt. Why?",
        "He lost money", "He's playing Monopoly", "His car broke down", "He has no cash", 2);
    add_question(76, CAT_IQ,
        "Which shape has the most sides: Hexagon, Octagon, Pentagon, Heptagon?",
        "Hexagon", "Octagon", "Pentagon", "Heptagon", 2);
    add_question(77, CAT_IQ,
        "If Mary's mother has 4 daughters: April, May, June, and _____. What is the 4th daughter's name?",
        "July", "Mary", "March", "August", 2);
    add_question(78, CAT_IQ,
        "What is half of two plus two?",
        "2", "3", "4", "1", 2);
    add_question(79, CAT_IQ,
        "If you have a bowl with six apples and you take away four, how many do you have?",
        "2", "4", "6", "10", 2);
    add_question(80, CAT_IQ,
        "What number should replace the question mark: 8, 27, 64, 125, ?",
        "196", "216", "256", "343", 2);
    add_question(81, CAT_CSE,
        "What is the time complexity of accessing an element in an array by index?",
        "O(n)", "O(log n)", "O(1)", "O(n^2)", 3);
    add_question(82, CAT_CSE,
        "Which data structure follows the FIFO principle?",
        "Stack", "Queue", "Tree", "Graph", 2);
    add_question(83, CAT_CSE,
        "What does 'HTTP' stand for?",
        "HyperText Transfer Protocol", "HighText Transfer Protocol", "HyperText Transmission Process", "HostText Transfer Protocol", 1);
    add_question(84, CAT_CSE,
        "In C programming, what does 'malloc' do?",
        "Free memory", "Allocate memory dynamically", "Copy memory", "Compare memory", 2);
    add_question(85, CAT_CSE,
        "Which sorting algorithm has the best average-case time complexity?",
        "Bubble Sort", "Insertion Sort", "Merge Sort", "Selection Sort", 3);
    add_question(86, CAT_CSE,
        "What is the default port number for HTTP?",
        "21", "80", "443", "8080", 2);
    add_question(87, CAT_CSE,
        "Which of these is NOT a programming paradigm?",
        "Object-Oriented", "Functional", "Procedural", "Alphabetical", 4);
    add_question(88, CAT_CSE,
        "What does 'SQL' stand for?",
        "Structured Query Language", "Simple Query Language", "Standard Query Language", "System Query Language", 1);
    add_question(89, CAT_CSE,
        "In a binary tree, what is the maximum number of nodes at level 'k'?",
        "k", "2^k", "2^k - 1", "k^2", 2);
    add_question(90, CAT_CSE,
        "Which layer of the OSI model handles routing?",
        "Transport Layer", "Network Layer", "Data Link Layer", "Session Layer", 2);
    add_question(91, CAT_CSE,
        "What is the output of: printf(\"%d\", 5/2) in C?",
        "2.5", "2", "3", "Error", 2);
    add_question(92, CAT_CSE,
        "Which data structure is used for implementing recursion?",
        "Queue", "Stack", "Heap", "Array", 2);
    add_question(93, CAT_CSE,
        "What does 'DNS' stand for?",
        "Domain Name System", "Data Network System", "Digital Name Service", "Domain Network Service", 1);
    add_question(94, CAT_CSE,
        "Which of these is a lossless compression algorithm?",
        "JPEG", "MP3", "ZIP", "MPEG", 3);
    add_question(95, CAT_CSE,
        "What is the space complexity of Merge Sort?",
        "O(1)", "O(log n)", "O(n)", "O(n log n)", 3);
    add_question(96, CAT_CSE,
        "In OOP, what is 'polymorphism'?",
        "Hiding data", "Multiple forms of a method", "Inheriting properties", "Creating objects", 2);
    add_question(97, CAT_CSE,
        "Which protocol is used for secure web browsing?",
        "HTTP", "FTP", "HTTPS", "SMTP", 3);
    add_question(98, CAT_CSE,
        "What is the maximum value of a signed 8-bit integer?",
        "128", "255", "127", "256", 3);
    add_question(99, CAT_CSE,
        "Which data structure is best for implementing a priority queue?",
        "Array", "Linked List", "Heap", "Stack", 3);
    add_question(100, CAT_CSE,
        "What does 'CPU' stand for?",
        "Central Processing Unit", "Computer Processing Unit", "Central Program Unit", "Core Processing Unit", 1);
}

void free_question_bank(void) {
    QNode* temp;
    while (g_question_head != NULL) {
        temp = g_question_head;
        g_question_head = g_question_head->next;
        free(temp);
    }
}

int count_questions(void) {
    int c = 0;
    for (QNode* t = g_question_head; t != NULL; t = t->next) c++;
    return c;
}

int count_questions_by_category(QuestionCategory cat) {
    int c = 0;
    for (QNode* t = g_question_head; t != NULL; t = t->next)
        if (t->category == cat) c++;
    return c;
}

void collect_questions_by_category(QuestionCategory cat, QNode** arr, int* count) {
    *count = 0;
    for (QNode* t = g_question_head; t != NULL; t = t->next)
        if (t->category == cat) arr[(*count)++] = t;
}

/* Browse-all-questions feature (menu option) */
void display_question_bank(void) {
    clear_screen();
    print_banner(UNI_LINE1, "QUESTION BANK OVERVIEW (LINKED LIST)");
    for (int cat = 0; cat < NUM_CATEGORIES; cat++) {
        printf("\n" CLR_MAGENTA CLR_BOLD "  === Category: %s ===" CLR_RESET "\n\n",
               category_name((QuestionCategory)cat));
        int idx = 1;
        for (QNode* t = g_question_head; t != NULL; t = t->next) {
            if (t->category != (QuestionCategory)cat) continue;
            printf(CLR_WHITE "  [Q%02d] %s" CLR_RESET "\n", idx, t->question);
            for (int i = 0; i < 4; i++) {
                if (i + 1 == t->correct_option)
                    printf("        " CLR_GREEN "%d) %s  <- Correct" CLR_RESET "\n", i + 1, t->options[i]);
                else
                    printf("        %d) %s\n", i + 1, t->options[i]);
            }
            printf("\n");
            idx++;
        }
    }
    printf(CLR_CYAN "  Total Questions in Bank: %d\n" CLR_RESET, count_questions());
    print_divider('-', BOX_WIDTH);
}

/* ================================================================================
 *  SECTION 7 : HASH TABLE MODULE (Player Authentication by ID)
 *  DJB2 polynomial string hashing + separate chaining.
 *  Buckets point directly into Player nodes that also live in the BST --
 *  no duplicate allocation, one node serves both structures.
 * ================================================================================ */
void init_hash_table(void) {
    for (int i = 0; i < HASH_SIZE; i++) g_ht.buckets[i] = NULL;
}

unsigned long hash_function(const char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + (unsigned long)tolower(c);
    }
    return hash % HASH_SIZE;
}

void hash_insert(Player* p) {
    unsigned long idx = hash_function(p->id);
    p->hash_next = g_ht.buckets[idx];
    g_ht.buckets[idx] = p;
}

Player* hash_lookup(const char* id) {
    unsigned long idx = hash_function(id);
    Player* curr = g_ht.buckets[idx];
    while (curr != NULL) {
        if (strcmp(curr->id, id) == 0) return curr;
        curr = curr->hash_next;
    }
    return NULL;
}

/* ================================================================================
 *  SECTION 8 : BINARY SEARCH TREE MODULE (Player Storage, ordered by ID)
 * ================================================================================ */
Player* bst_insert(Player* root, Player* p) {
    if (root == NULL) return p;
    int cmp = strcmp(p->id, root->id);
    if (cmp < 0) root->left = bst_insert(root->left, p);
    else if (cmp > 0) root->right = bst_insert(root->right, p);
    return root;
}

Player* bst_search(Player* root, const char* id) {
    if (root == NULL) return NULL;
    int cmp = strcmp(id, root->id);
    if (cmp == 0) return root;
    return (cmp < 0) ? bst_search(root->left, id) : bst_search(root->right, id);
}

int bst_count(const Player* root) {
    if (root == NULL) return 0;
    return 1 + bst_count(root->left) + bst_count(root->right);
}

void bst_inorder_collect(Player* root, Player** arr, int* idx) {
    if (root == NULL) return;
    bst_inorder_collect(root->left, arr, idx);
    arr[(*idx)++] = root;
    bst_inorder_collect(root->right, arr, idx);
}

void bst_free(Player* root) {
    if (root == NULL) return;
    bst_free(root->left);
    bst_free(root->right);
    free(root);
}

/* ================================================================================
 *  SECTION 9 : SORTING MODULE (Bubble / Quick / Merge -- from scratch)
 *  All three use the SAME comparator so the leaderboard order is identical
 *  no matter which algorithm the user picks -- only speed differs.
 *
 *  Ranking priority : ranking_score -> accuracy -> raw_score -> id (asc)
 * ================================================================================ */
int compare_players(const Player* a, const Player* b) {
    const double EPS = 1e-6;
    if (a->ranking_score - b->ranking_score >  EPS) return 1;
    if (b->ranking_score - a->ranking_score >  EPS) return -1;
    if (a->accuracy - b->accuracy >  EPS) return 1;
    if (b->accuracy - a->accuracy >  EPS) return -1;
    if (a->raw_score - b->raw_score >  EPS) return 1;
    if (b->raw_score - a->raw_score >  EPS) return -1;
    int idcmp = strcmp(a->id, b->id);
    if (idcmp < 0) return 1;
    if (idcmp > 0) return -1;
    return 0;
}

/* ---- Bubble Sort : O(n^2), simple baseline, early-exit optimized ---- */
void bubble_sort_players(Player* arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        int swapped = 0;
        for (int j = 0; j < n - 1 - i; j++) {
            if (compare_players(arr[j], arr[j + 1]) < 0) {
                Player* tmp = arr[j]; arr[j] = arr[j + 1]; arr[j + 1] = tmp;
                swapped = 1;
            }
        }
        if (!swapped) break;
    }
}

/* ---- Quick Sort : O(n log n) average, Lomuto partition on comparator ---- */
int partition_players(Player* arr[], int low, int high) {
    Player* pivot = arr[high];
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (compare_players(arr[j], pivot) >= 0) {
            i++;
            Player* tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
        }
    }
    Player* tmp = arr[i + 1]; arr[i + 1] = arr[high]; arr[high] = tmp;
    return i + 1;
}

void quick_sort_players(Player* arr[], int low, int high) {
    if (low < high) {
        int pi = partition_players(arr, low, high);
        quick_sort_players(arr, low, pi - 1);
        quick_sort_players(arr, pi + 1, high);
    }
}

/* ---- Merge Sort : O(n log n), stable ---- */
void merge_players(Player* arr[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    Player** L = (Player**)malloc(n1 * sizeof(Player*));
    Player** R = (Player**)malloc(n2 * sizeof(Player*));
    if (!L || !R) { free(L); free(R); return; }

    for (int i = 0; i < n1; i++) L[i] = arr[left + i];
    for (int j = 0; j < n2; j++) R[j] = arr[mid + 1 + j];

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (compare_players(L[i], R[j]) >= 0) arr[k++] = L[i++];
        else arr[k++] = R[j++];
    }
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
    free(L); free(R);
}

void merge_sort_players(Player* arr[], int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        merge_sort_players(arr, left, mid);
        merge_sort_players(arr, mid + 1, right);
        merge_players(arr, left, mid, right);
    }
}

/* Competition ranking: equal ranking_score => same rank; next distinct
   score jumps to (position + 1), e.g. 1, 2, 2, 4, 5, 5, 7 ...          */
void assign_competition_ranks(Player* arr[], int n) {
    if (n == 0) return;
    const double EPS = 1e-6;
    arr[0]->rank = 1;
    for (int i = 1; i < n; i++) {
        double diff = arr[i]->ranking_score - arr[i - 1]->ranking_score;
        if (diff < 0) diff = -diff;
        if (diff < EPS)
            arr[i]->rank = arr[i - 1]->rank;
        else
            arr[i]->rank = i + 1;
    }
}

/* ================================================================================
 *  SECTION 10 : SCORING MODULE
 *  status: 1 = correct, 0 = wrong, -1 = skipped
 * ================================================================================ */
void apply_answer_result(Player* p, int status, QuestionCategory cat) {
    if (status == 1) {
        p->correct++;               p->cat_correct[cat]++;
        p->raw_score      += CORRECT_MARK;
        p->cat_score[cat] += CORRECT_MARK;
    } else if (status == 0) {
        p->wrong++;                 p->cat_wrong[cat]++;
        p->raw_score      -= WRONG_PENALTY;
        p->cat_score[cat] -= WRONG_PENALTY;
        if (p->raw_score < 0)      p->raw_score = 0.0;
        if (p->cat_score[cat] < 0) p->cat_score[cat] = 0.0;
    } else {
        p->skipped++;
        p->cat_skipped[cat]++;
    }
}

/* Computes accuracy + the fair, accuracy-weighted "ranking_score" composite:
     ranking_score = raw_score * (0.50 + 0.50 * accuracy/100)   [if raw_score > 0]
     ranking_score = raw_score                                  [otherwise]
   This rewards players who are both scoring well AND answering accurately,
   rather than rewarding attempt volume alone.                                */
void finalize_scores(Player* p) {
    p->accuracy = (p->attempted > 0)
                  ? ((double)p->correct / (double)p->attempted) * 100.0
                  : 0.0;

    if (p->raw_score > 0.0) {
        double accuracy_factor = 0.50 + 0.50 * (p->accuracy / 100.0);
        p->ranking_score = p->raw_score * accuracy_factor;
    } else {
        p->ranking_score = p->raw_score;
    }
}

/* ================================================================================
 *  SECTION 11 : FILE I/O MODULE
 *  Record format (pipe-delimited):
 *    name|id|batch|dept|total_q|attempted|correct|wrong|raw_score|accuracy|
 *    ranking_score|<cat0 correct,wrong,skipped,score>|...(x5 categories)
 * ================================================================================ */
void save_all_players(void) {
    FILE* fp = fopen(SCORES_FILE, "w");
    if (!fp) { printf(CLR_RED "[!] Could not save player database.\n" CLR_RESET); return; }

    int n = bst_count(g_bst_root);
    Player** arr = (Player**)malloc((n > 0 ? n : 1) * sizeof(Player*));
    int idx = 0;
    bst_inorder_collect(g_bst_root, arr, &idx);

    for (int i = 0; i < n; i++) {
        Player* p = arr[i];
        fprintf(fp, "%s|%s|%s|%s|%d|%d|%d|%d|%.2f|%.2f|%.2f",
                p->name, p->id, p->batch, p->dept,
                p->total_questions, p->attempted, p->correct, p->wrong,
                p->raw_score, p->accuracy, p->ranking_score);
        for (int c = 0; c < NUM_CATEGORIES; c++) {
            fprintf(fp, "|%d,%d,%d,%.2f",
                    p->cat_correct[c], p->cat_wrong[c], p->cat_skipped[c], p->cat_score[c]);
        }
        fprintf(fp, "\n");
    }
    free(arr);
    fclose(fp);
}

void load_all_players(void) {
    FILE* fp = fopen(SCORES_FILE, "r");
    if (!fp) return;

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        Player* p = (Player*)malloc(sizeof(Player));
        if (!p) continue;
        p->left = p->right = p->hash_next = NULL;
        p->rank = 0;

        int matched = sscanf(line, "%49[^|]|%31[^|]|%19[^|]|%15[^|]|%d|%d|%d|%d|%lf|%lf|%lf",
                              p->name, p->id, p->batch, p->dept,
                              &p->total_questions, &p->attempted, &p->correct, &p->wrong,
                              &p->raw_score, &p->accuracy, &p->ranking_score);

        if (matched != 11) { free(p); continue; }

        p->skipped = p->total_questions - p->attempted;
        if (p->skipped < 0) p->skipped = 0;

        for (int c = 0; c < NUM_CATEGORIES; c++) {
            p->cat_correct[c] = 0; p->cat_wrong[c] = 0;
            p->cat_skipped[c] = 0; p->cat_score[c] = 0.0;
        }

        /* Skip past the 11 base fields to reach the category sub-fields */
        char* ptr = line;
        int field_count = 0;
        while (ptr && field_count < 11) {
            ptr = strchr(ptr + 1, '|');
            field_count++;
        }
        if (ptr) {
            ptr++;
            for (int c = 0; c < NUM_CATEGORIES && ptr; c++) {
                sscanf(ptr, "%d,%d,%d,%lf",
                       &p->cat_correct[c], &p->cat_wrong[c], &p->cat_skipped[c], &p->cat_score[c]);
                ptr = strchr(ptr, '|');
                if (ptr) ptr++;
            }
        }

        g_bst_root = bst_insert(g_bst_root, p);
        hash_insert(p);
    }
    fclose(fp);
}

void export_leaderboard_report(Player* arr[], int n, const char* algo_used, double elapsed_ms) {
    FILE* fp = fopen(LEADERBOARD_FILE, "w");
    if (!fp) return;

    fprintf(fp, "================================================================================\n");
    fprintf(fp, "   %s\n", UNI_LINE1);
    fprintf(fp, "   %s - OFFICIAL LEADERBOARD\n", APP_TITLE);
    fprintf(fp, "   Sorted using : %s   (%.4f ms for %d records)\n", algo_used, elapsed_ms, n);
    fprintf(fp, "   Ranking Rule : Competition Ranking (1, 2, 2, 4 ...)\n");
    fprintf(fp, "================================================================================\n");
    fprintf(fp, "%-5s | %-16s | %-20s | %-12s | %-8s | %9s | %8s | %10s\n",
            "Rank", "ID", "Name", "Batch", "Dept", "Raw", "Acc %", "Score");
    fprintf(fp, "--------------------------------------------------------------------------------\n");
    for (int i = 0; i < n; i++) {
        fprintf(fp, "#%-4d | %-16s | %-20s | %-12s | %-8s | %9.2f | %7.2f%% | %10.2f\n",
                arr[i]->rank, arr[i]->id, arr[i]->name, arr[i]->batch, arr[i]->dept,
                arr[i]->raw_score, arr[i]->accuracy, arr[i]->ranking_score);
    }
    fprintf(fp, "================================================================================\n");
    fclose(fp);
}

void write_result_file(const Player* p, AnswerRecord log[], int total_players) {
    char filename[80];
    char clean_id[MAX_ID_LEN];
    int idx = 0;
    for (int i = 0; p->id[i] != '\0' && idx < MAX_ID_LEN - 1; i++) {
        char c = p->id[i];
        clean_id[idx++] = (isalnum((unsigned char)c) || c == '-' || c == '_') ? c : '_';
    }
    clean_id[idx] = '\0';
    snprintf(filename, sizeof(filename), "result_%s.txt", clean_id);

    FILE* fp = fopen(filename, "w");
    if (!fp) { printf(CLR_RED "  >> ERROR: could not create %s\n" CLR_RESET, filename); return; }

    char roll_info[128] = "";
    decode_roll_no(p->id, NULL, 0, roll_info, sizeof(roll_info));

    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    fprintf(fp, "================================================================================\n");
    fprintf(fp, "                         %s\n", UNI_LINE1);
    fprintf(fp, "                    %s - RESULT TRANSCRIPT\n", APP_TITLE);
    fprintf(fp, "================================================================================\n");
    fprintf(fp, "Name        : %s\n", p->name);
    fprintf(fp, "Student ID  : %s\n", p->id);
    if (roll_info[0] != '\0') fprintf(fp, "ID Details  : %s\n", roll_info);
    fprintf(fp, "Batch       : %s\n", p->batch);
    fprintf(fp, "Department  : %s\n", p->dept);
    fprintf(fp, "Date        : %s\n", time_str);
    fprintf(fp, "--------------------------------------------------------------------------------\n");
    fprintf(fp, "                       QUESTION-BY-QUESTION BREAKDOWN\n");
    fprintf(fp, "--------------------------------------------------------------------------------\n");

    for (int i = 0; i < FIXED_QUESTIONS; i++) {
        QNode* q = g_question_head;
        while (q != NULL && q->qno != log[i].questionId) q = q->next;
        if (!q) continue;

        int sel = log[i].selectedOption;
        const char* marker = (sel == 0) ? "[SKIPPED]" :
                              (sel == q->correct_option) ? "[CORRECT]" : "[WRONG]  ";

        fprintf(fp, "\nQ%02d [%s] %s\n", i + 1, category_name(log[i].category), marker);
        fprintf(fp, "    %s\n", q->question);
        if (sel == 0)
            fprintf(fp, "    Your Answer    : (skipped)\n");
        else
            fprintf(fp, "    Your Answer    : %d) %s\n", sel, q->options[sel - 1]);
        if (sel != q->correct_option)
            fprintf(fp, "    Correct Answer : %d) %s\n", q->correct_option, q->options[q->correct_option - 1]);
    }

    fprintf(fp, "\n================================================================================\n");
    fprintf(fp, "                            FINAL EXAMINATION RESULT\n");
    fprintf(fp, "================================================================================\n");
    fprintf(fp, "Total Questions : %d   Attempted : %d   Correct : %d   Wrong : %d   Skipped : %d\n",
            p->total_questions, p->attempted, p->correct, p->wrong,
            p->total_questions - p->attempted);
    fprintf(fp, "--------------------------------------------------------------------------------\n");
    fprintf(fp, "Raw Score          : %.2f marks\n", p->raw_score);
    fprintf(fp, "Accuracy           : %.2f%%\n", p->accuracy);
    fprintf(fp, "Fair Ranking Score : %.2f points\n", p->ranking_score);
    fprintf(fp, "Rank               : %d out of %d players\n", p->rank, total_players);

    fprintf(fp, "\n--- CATEGORY-WISE PERFORMANCE ---\n");
    for (int c = 0; c < NUM_CATEGORIES; c++) {
        fprintf(fp, "%-15s: Correct=%d  Wrong=%d  Skipped=%d  Score=%.2f\n",
                category_name((QuestionCategory)c),
                p->cat_correct[c], p->cat_wrong[c], p->cat_skipped[c], p->cat_score[c]);
    }
    fprintf(fp, "================================================================================\n");
    fprintf(fp, "Status: Transcript generated successfully.\n");
    fclose(fp);

    printf(CLR_GREEN "  [+] Transcript exported to: '%s'\n" CLR_RESET, filename);
}

/* ================================================================================
 *  SECTION 12 : QUIZ ENGINE MODULE
 * ================================================================================ */
void shuffle_indices(int arr[], int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = arr[i]; arr[i] = arr[j]; arr[j] = tmp;
    }
}

/* Presents one question with a live progress bar + candidate header card. */
int ask_single_question(const QNode* q, int q_number, int total_q, const Player* p) {
    clear_screen();
    char bar[32];
    draw_progress_bar(q_number, total_q, bar, sizeof(bar));
    int percent = (int)(((double)q_number / (double)total_q) * 100);

    char header_line[256];
    snprintf(header_line, sizeof(header_line), " %-18s | Roll: %-12s | [%-20s] %3d%%",
             p->name, p->id, bar, percent);
    print_box_top();
    print_fixed_row(header_line, CLR_WHITE);
    print_box_bottom();

    printf("\n" CLR_YELLOW CLR_BOLD "  [Category: %s]  Question %d of %d" CLR_RESET "\n\n",
           category_name(q->category), q_number, total_q);
    printf("  " CLR_WHITE "%s" CLR_RESET "\n\n", q->question);
    for (int i = 0; i < 4; i++) printf("    " CLR_CYAN "%d)" CLR_RESET " %s\n", i + 1, q->options[i]);
    printf("    " CLR_DIM "0) Skip Question" CLR_RESET "\n");
    print_divider('-', BOX_WIDTH);

    return read_integer_input("  >> Your answer (1-4, or 0 to skip): ", 0, 4);
}

/* Category-based selection: exactly QUESTIONS_PER_CAT random questions from
   EACH category, then the combined set is reshuffled so categories mix.   */
void run_quiz_session(Player* p, AnswerRecord log[]) {
    QNode* session[FIXED_QUESTIONS];
    int session_idx = 0;

    for (int cat = 0; cat < NUM_CATEGORIES; cat++) {
        QNode* cat_questions[100];
        int cat_count = 0;
        collect_questions_by_category((QuestionCategory)cat, cat_questions, &cat_count);

        if (cat_count < QUESTIONS_PER_CAT) {
            printf(CLR_RED "[!] Not enough questions in category '%s'.\n" CLR_RESET,
                   category_name((QuestionCategory)cat));
            continue;
        }

        int indices[100];
        for (int i = 0; i < cat_count; i++) indices[i] = i;
        shuffle_indices(indices, cat_count);

        for (int i = 0; i < QUESTIONS_PER_CAT; i++)
            session[session_idx++] = cat_questions[indices[i]];
    }

    /* Reshuffle the combined 10-question set so categories are interleaved
       rather than presented in five back-to-back category blocks.        */
    for (int i = session_idx - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        QNode* tmp = session[i]; session[i] = session[j]; session[j] = tmp;
    }

    p->total_questions = session_idx;

    printf("\n");
    print_banner(UNI_LINE1, "EXAMINATION INSTRUCTIONS");
    print_box_row("Candidate Name", p->name);
    print_box_row("Student ID", p->id);
    print_box_row("Batch / Dept", p->batch);
    print_box_row("Marking Scheme", "+1.00 Correct | -0.25 Wrong | 0.00 Skip");
    print_box_row("Question Set", "10 Questions (2 per category x 5 categories)");
    print_box_bottom();
    printf(CLR_DIM "\n  >> Press ENTER to start Question 1..." CLR_RESET);
    fflush(stdout);
    int ch; while ((ch = getchar()) != '\n' && ch != EOF && ch != '\r') {}

    for (int i = 0; i < session_idx; i++) {
        QNode* q = session[i];
        int choice = ask_single_question(q, i + 1, session_idx, p);

        log[i].questionId      = q->qno;
        log[i].selectedOption  = choice;
        log[i].category        = q->category;

        int status;
        if (choice == 0) status = -1;
        else if (choice == q->correct_option) status = 1;
        else status = 0;

        if (status >= 0) p->attempted++;
        apply_answer_result(p, status, q->category);

        if (status == 1)
            printf("\n  " CLR_GREEN CLR_BOLD ">> CORRECT!  (+1.00)" CLR_RESET "  Running Score: %.2f\n", p->raw_score);
        else if (status == 0)
            printf("\n  " CLR_RED CLR_BOLD ">> WRONG!  Correct was %d) %s  (-0.25)" CLR_RESET "  Running Score: %.2f\n",
                   q->correct_option, q->options[q->correct_option - 1], p->raw_score);
        else
            printf("\n  " CLR_YELLOW CLR_BOLD ">> SKIPPED  (0.00)" CLR_RESET "  Running Score: %.2f\n", p->raw_score);
        pause_prompt();
    }

    finalize_scores(p);
}

/* ================================================================================
 *  SECTION 13 : MENU HANDLERS
 * ================================================================================ */
void show_main_menu(int q_count, int player_count) {
    clear_screen();
    printf("\n");
    print_banner(UNI_LINE1, APP_TITLE);
    printf(CLR_DIM "  Quiz Rules: 10 randomized MCQs (2 per category, 5 categories)\n");
    printf("  Marking: Correct +1.00 | Wrong -0.25 | Skip 0.00\n" CLR_RESET);
    print_divider('-', BOX_WIDTH);
    printf("  " CLR_CYAN "[Status]" CLR_RESET " %d Questions in Bank  |  %d Player Records\n",
           q_count, player_count);
    printf("\n  " CLR_BOLD CLR_MAGENTA ">>> MAIN MENU <<<" CLR_RESET "\n");
    print_box_top();
    print_fixed_row("  [1] Take Quiz",            CLR_YELLOW);
    print_fixed_row("  [2] View Leaderboard",     CLR_YELLOW);
    print_fixed_row("  [3] Search Player by ID",  CLR_YELLOW);
    print_fixed_row("  [4] Browse Question Bank", CLR_YELLOW);
    print_fixed_row("  [5] Exit",                 CLR_YELLOW);
    print_box_bottom();
}

void display_player_card(const Player* p) {
    char buf[64], dept_batch[80];
    snprintf(dept_batch, sizeof(dept_batch), "%s (%s)", p->dept, p->batch);

    print_box_top();
    print_box_title("PLAYER PROFILE");
    print_box_bottom();
    print_box_row("Name", p->name);
    print_box_row("Student ID", p->id);
    print_box_row("Department & Batch", dept_batch);
    snprintf(buf, sizeof(buf), "%d / %d", p->attempted, p->total_questions);
    print_box_row("Attempted", buf);
    snprintf(buf, sizeof(buf), "%d Correct / %d Wrong / %d Skipped", p->correct, p->wrong, p->skipped);
    print_box_row("Performance", buf);
    snprintf(buf, sizeof(buf), "%.2f Marks", p->raw_score);
    print_box_row("Raw Score", buf);
    snprintf(buf, sizeof(buf), "%.2f %%", p->accuracy);
    print_box_row("Accuracy", buf);
    snprintf(buf, sizeof(buf), "%.2f Points", p->ranking_score);
    print_box_row("Fair Ranking Score", buf);
    if (p->rank > 0) {
        snprintf(buf, sizeof(buf), "#%d", p->rank);
        print_box_row("Current Rank", buf);
    }
    print_box_bottom();

    printf("\n" CLR_MAGENTA "  --- Category-wise Performance ---" CLR_RESET "\n");
    for (int c = 0; c < NUM_CATEGORIES; c++) {
        printf("  %-15s: " CLR_GREEN "C=%-2d" CLR_RESET " " CLR_RED "W=%-2d" CLR_RESET " "
               CLR_YELLOW "S=%-2d" CLR_RESET "  Score=%.2f\n",
               category_name((QuestionCategory)c),
               p->cat_correct[c], p->cat_wrong[c], p->cat_skipped[c], p->cat_score[c]);
    }
}

void handle_take_quiz(void) {
    clear_screen();
    print_banner(UNI_LINE1, "STUDENT REGISTRATION & AUTHENTICATION");

    Player p;
    memset(&p, 0, sizeof(Player));

    read_string_input("  >> Enter Student ID / Roll Number (8-digit e.g. 25201000): ", p.id, MAX_ID_LEN);

    char decoded_batch[MAX_BATCH_LEN] = "";
    char roll_info[128] = "";
    decode_roll_no(p.id, decoded_batch, sizeof(decoded_batch), roll_info, sizeof(roll_info));

    Player* existing = hash_lookup(p.id);
    if (existing != NULL) {
        printf("\n" CLR_YELLOW "  [+] Welcome back, %s! (Verified via Hash Table)\n" CLR_RESET, existing->name);
        printf("      Previous Score: %.2f | Accuracy: %.2f%% | Rank: %d\n",
               existing->ranking_score, existing->accuracy, existing->rank);
        char answer[16];
        read_string_input("      Retake the quiz and overwrite this record? (y/n): ", answer, sizeof(answer));
        if (answer[0] != 'y' && answer[0] != 'Y') {
            printf(CLR_DIM "\n  Quiz session cancelled. No changes made.\n" CLR_RESET);
            pause_prompt();
            return;
        }
        strncpy(p.name, existing->name, MAX_NAME_LEN - 1);
        strncpy(p.batch, existing->batch, MAX_BATCH_LEN - 1);
        strncpy(p.dept, existing->dept, MAX_DEPT_LEN - 1);
    } else {
        printf(CLR_GREEN "\n  [+] New student registration...\n" CLR_RESET);
        if (roll_info[0] != '\0') {
            printf("  [+] Detected ID Details: %s\n", roll_info);
            strncpy(p.batch, decoded_batch, MAX_BATCH_LEN - 1);
        } else {
            read_string_input("  >> Enter Batch (e.g. 2025 Fall): ", p.batch, MAX_BATCH_LEN);
        }
        read_string_input("  >> Enter Full Name: ", p.name, MAX_NAME_LEN);
        read_string_input("  >> Enter Department Code (e.g. CSE / BBA / EEE): ", p.dept, MAX_DEPT_LEN);
    }

    for (int c = 0; c < NUM_CATEGORIES; c++) {
        p.cat_correct[c] = 0; p.cat_wrong[c] = 0;
        p.cat_skipped[c] = 0; p.cat_score[c] = 0.0;
    }

    /* Fisher-Yates reseed: mix time, clock, and an incrementing counter so
       repeated attempts in the same run don't reproduce the same order.  */
    static unsigned int attempt_counter = 0;
    attempt_counter += 101;
    srand((unsigned int)time(NULL) ^ (unsigned int)clock() ^ attempt_counter);

    AnswerRecord log[FIXED_QUESTIONS];
    run_quiz_session(&p, log);

    /* Insert or update in BST + Hash (new player only -- retakes reuse
       the existing heap node found via hash_lookup, avoiding duplicates).
       The node's tree/hash links must survive the overwrite, since p's
       own left/right/hash_next are still NULL (never inserted anywhere). */
    Player* stored;
    if (existing != NULL) {
        Player* saved_left      = existing->left;
        Player* saved_right     = existing->right;
        Player* saved_hash_next = existing->hash_next;
        *existing = p;
        existing->left      = saved_left;
        existing->right     = saved_right;
        existing->hash_next = saved_hash_next;
        stored = existing;
    } else {
        stored = (Player*)malloc(sizeof(Player));
        *stored = p;
        stored->left = stored->right = stored->hash_next = NULL;
        g_bst_root = bst_insert(g_bst_root, stored);
        hash_insert(stored);
    }

    save_all_players();

    int total = bst_count(g_bst_root);
    Player** arr = (Player**)malloc(total * sizeof(Player*));
    int idx = 0;
    bst_inorder_collect(g_bst_root, arr, &idx);
    merge_sort_players(arr, 0, total - 1);
    assign_competition_ranks(arr, total);

    for (int i = 0; i < total; i++)
        if (arr[i] == stored) { stored->rank = arr[i]->rank; break; }

    clear_screen();
    print_banner(UNI_LINE1, "EXAMINATION COMPLETED");
    display_player_card(stored);

    write_result_file(stored, log, total);
    export_leaderboard_report(arr, total, "Merge Sort", 0.0);
    free(arr);

    printf(CLR_GREEN "\n  [+] Recorded! Check Leaderboard (Option 2) for your rank.\n" CLR_RESET);
    pause_prompt();
}

void handle_view_leaderboard(void) {
    int total = bst_count(g_bst_root);
    if (total == 0) {
        clear_screen();
        printf("\n" CLR_YELLOW "  [!] No player records found. Take a quiz first!\n" CLR_RESET);
        pause_prompt();
        return;
    }

    Player** arr = (Player**)malloc(total * sizeof(Player*));
    int idx = 0;
    bst_inorder_collect(g_bst_root, arr, &idx);

    clear_screen();
    print_banner(UNI_LINE1, "CHOOSE A SORTING ALGORITHM");
    printf("   " CLR_YELLOW "1." CLR_RESET " Bubble Sort   [O(n^2)]\n");
    printf("   " CLR_YELLOW "2." CLR_RESET " Quick Sort    [O(n log n) average]\n");
    printf("   " CLR_YELLOW "3." CLR_RESET " Merge Sort    [O(n log n), stable]\n");
    int choice = read_integer_input("\n  Choice: ", 1, 3);

    clock_t start = clock();
    if (choice == 1) bubble_sort_players(arr, total);
    else if (choice == 2) quick_sort_players(arr, 0, total - 1);
    else merge_sort_players(arr, 0, total - 1);
    clock_t end = clock();
    double elapsed_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;

    assign_competition_ranks(arr, total);
    const char* algo_name = (choice == 1) ? "Bubble Sort" : (choice == 2) ? "Quick Sort" : "Merge Sort";

    clear_screen();
    print_banner(UNI_LINE1, "OFFICIAL LEADERBOARD");
    printf("  Sorted using: " CLR_CYAN "%s" CLR_RESET "  (%.4f ms for %d records)\n", algo_name, elapsed_ms, total);
    printf("  Ranking Rule: Competition Ranking (1, 2, 2, 4 ...)\n\n");
    printf("  " CLR_BOLD "%-5s %-16s %-20s %-12s %-8s %9s %8s %10s" CLR_RESET "\n",
           "Rank", "ID", "Name", "Batch", "Dept", "Raw", "Acc %", "Score");
    print_divider('-', BOX_WIDTH);
    int limit = (total < 15) ? total : 15;
    for (int i = 0; i < limit; i++) {
        const char* rowclr = (arr[i]->rank == 1) ? CLR_GREEN : (arr[i]->rank <= 3) ? CLR_YELLOW : CLR_WHITE;
        printf("  %s#%-4d %-16s %-20s %-12s %-8s %9.2f %7.2f%% %10.2f" CLR_RESET "\n",
               rowclr, arr[i]->rank, arr[i]->id, arr[i]->name, arr[i]->batch, arr[i]->dept,
               arr[i]->raw_score, arr[i]->accuracy, arr[i]->ranking_score);
    }
    print_divider('-', BOX_WIDTH);
    if (total > limit) printf(CLR_DIM "  ...and %d more player(s) not shown.\n" CLR_RESET, total - limit);

    export_leaderboard_report(arr, total, algo_name, elapsed_ms);
    printf(CLR_GREEN "\n  [+] Leaderboard saved to '%s'\n" CLR_RESET, LEADERBOARD_FILE);

    free(arr);
    pause_prompt();
}

void handle_search_player(void) {
    clear_screen();
    print_banner(UNI_LINE1, "PLAYER SEARCH & AUTH");

    char id[MAX_ID_LEN];
    read_string_input("  >> Enter Student ID (8-digit e.g. 25201000): ", id, MAX_ID_LEN);

    char roll_info[128] = "";
    decode_roll_no(id, NULL, 0, roll_info, sizeof(roll_info));
    unsigned long bucket = hash_function(id);
    printf("\n  [*] Hash Bucket Index: %lu  (Table Capacity: %d)\n", bucket, HASH_SIZE);
    if (roll_info[0] != '\0') printf("  [*] ID Decoder: %s\n", roll_info);

    Player* found = hash_lookup(id);
    if (found) {
        printf("\n");
        display_player_card(found);
    } else {
        printf(CLR_RED "\n  [-] Student ID '%s' not found in system.\n" CLR_RESET, id);
    }
    pause_prompt();
}

void handle_browse_questions(void) {
    display_question_bank();
    pause_prompt();
}

/* ================================================================================
 *  SECTION 14 : MAIN
 * ================================================================================ */
int main(void) {
    srand((unsigned int)time(NULL));
    init_hash_table();
    init_question_bank();
    load_all_players();

    /* Ranks aren't persisted to disk (they're derived), so recompute them
       once at startup for any players loaded from a previous session.  */
    int loaded_total = bst_count(g_bst_root);
    if (loaded_total > 0) {
        Player** boot_arr = (Player**)malloc(loaded_total * sizeof(Player*));
        int boot_idx = 0;
        bst_inorder_collect(g_bst_root, boot_arr, &boot_idx);
        merge_sort_players(boot_arr, 0, loaded_total - 1);
        assign_competition_ranks(boot_arr, loaded_total);
        free(boot_arr);
    }

    int running = 1;
    while (running) {
        int q_count = count_questions();
        int player_count = bst_count(g_bst_root);
        show_main_menu(q_count, player_count);

        int choice = read_integer_input("\n  >> Select Option (1-5): ", 1, 5);
        switch (choice) {
            case 1: handle_take_quiz();        break;
            case 2: handle_view_leaderboard();  break;
            case 3: handle_search_player();     break;
            case 4: handle_browse_questions();  break;
            case 5:
                clear_screen();
                print_banner(UNI_LINE1, "Thank you for playing! Goodbye.");
                running = 0;
                break;
        }
    }

    free_question_bank();
    bst_free(g_bst_root);
    return 0;
}
