# DSA Trivia Challenge: Software Design Document

| Item | Detail |
|---|---|
| Project | DSA Trivia Challenge (Merged Edition) |
| Institution | University of Brahmanbaria |
| Language / Standard | C, C11 (`gcc -Wall -Wextra -std=c11`) |
| Source file | `DSA_TRIVIA.c` (single file, about 1,365 lines) |
| Document version | 1.0 |
| Date | 7 October 2026 |

---

## 1. Introduction

### 1.1 Purpose

This document describes the architecture, data structures, algorithms, control flow, and persistence design of the DSA Trivia Challenge, a terminal quiz application written in C. It is meant for instructors reviewing the project and for anyone who needs to understand, test, or extend the code.

### 1.2 Scope

**In scope:** the quiz engine, the question bank, player registration and records, search by ID, leaderboards with three sorting algorithms, and text-file output.

**Out of scope:** networking, a graphical interface, real authentication, and a database.

---

## 2. System Overview

### 2.1 Feature Summary

- A randomized 10-question quiz per attempt, drawn from a bank of 60 multiple-choice questions.
- Student registration. If the 7- or 8-digit ID matches the roll format, the batch is detected automatically.
- Player lookup by ID through a hash table.
- A leaderboard that can be sorted with Bubble Sort, Quick Sort, or Merge Sort. All three use the same comparator, so only the speed differs.
- A result transcript for each completed quiz and a leaderboard report file.
- Access to the question bank (which contains the answers) is limited to students who have completed a quiz.

### 2.2 Marking Scheme

| Outcome | Mark | Effect on Counters |
|---|---|---|
| Correct | +1.00 | `attempted++`, `correct++`, `raw_score += 1.00` |
| Wrong | -0.25 | `attempted++`, `wrong++`, `raw_score -= 0.25`, then clamped at 0 |
| Skipped (option 0) | 0.00 | `skipped` (derived), no change to score |

### 2.3 Runtime Files

| File | Written When | Content |
|---|---|---|
| `players_scores.txt` | After every completed quiz | Full player database, one pipe-delimited record per line (rewritten each time) |
| `result_<StudentID>.txt` | After every completed quiz | Question-by-question transcript and final result |
| `leaderboard_report.txt` | After every leaderboard view or quiz | Full ranked table with the algorithm used and timing |

---

## 3. Architecture

### 3.1 Layered View

```
┌─ Presentation ──── Banners, menus, colour output, input validation     (§4)
├─ Application ───── Quiz engine, leaderboard, search, access control    (§12, §13)
├─ Domain ────────── Scoring, sorting, competition ranking, ID decoder   (§5, §9, §10)
├─ Data Structures ─ Question linked list, player BST, hash table        (§6, §7, §8)
└─ Persistence ───── players_scores.txt, leaderboard_report.txt,
                     result_<ID>.txt                                     (§11)
```

### 3.2 Control Flow Overview

```
main()
  │
  ├── init_hash_table()  ──  init_question_bank()  ──  load_all_players()
  │
  └── Main loop: show_main_menu() → read choice (1-5)
        │
        ├── [1] Take Quiz            handle_take_quiz()
        ├── [2] View Leaderboard     handle_view_leaderboard()
        ├── [3] Search Player by ID  handle_search_player()
        ├── [4] Browse Question Bank handle_browse_questions()
        └── [5] Exit                 free_question_bank(), bst_free()
```

### 3.3 Module Map

| § | Module | Responsibility | Key Functions |
|---|---|---|---|
| 4 | UI utilities | Screens, banners, safe input, progress bar | `print_banner`, `read_integer_input`, `read_string_input`, `draw_progress_bar` |
| 5 | ID decoder | Parse roll number into year, batch, serial | `decode_roll_no` |
| 6 | Question bank | Singly linked list of 60 questions | `add_question`, `init_question_bank`, `count_questions`, `display_question_bank` |
| 7 | Hash table | Fast ID lookup (DJB2 hash, chaining) | `hash_function`, `hash_insert`, `hash_lookup` |
| 8 | Binary search tree | Ordered player storage by ID | `bst_insert`, `bst_count`, `bst_inorder_collect`, `bst_free` |
| 9 | Sorting | Three sort algorithms and ranking | `compare_players`, `bubble_sort_players`, `quick_sort_players`, `merge_sort_players`, `assign_competition_ranks` |
| 10 | Scoring | Marking and composite ranking score | `apply_answer_result`, `finalize_scores` |
| 11 | File I/O | Save, load, reports, transcripts | `save_all_players`, `load_all_players`, `export_leaderboard_report`, `write_result_file` |
| 12 | Quiz engine | Random selection and question flow | `shuffle_indices`, `run_quiz_session`, `ask_single_question` |
| 13 | Menu handlers | One handler per menu option | `handle_take_quiz`, `handle_view_leaderboard`, `handle_search_player`, `handle_browse_questions`, `display_player_card` |
| 14 | Main | Startup, loop, shutdown | `main` |

### 3.4 Key Design Decisions

1. **One node, two indexes.** Each `Player` is allocated once and linked into both the BST (for ordered traversal) and the hash table (for constant-time lookup). This avoids duplicate memory.
2. **Single comparator.** `compare_players()` is the only definition of ranking order. Every sort algorithm uses it, so results are identical regardless of algorithm.
3. **Ranks are derived, not stored.** Ranks are not written to `players_scores.txt`. They are recomputed at startup from the scores.
4. **Retakes keep graph links.** On a retake, the existing node's `left`, `right`, and `hash_next` pointers are saved and restored. The rest of the record is overwritten.

---

## 4. Data Design

### 4.1 Question Node (`QNode`)

| Field | Type | Purpose |
|---|---|---|
| `qno` | `int` | Question ID (e.g., 1 to 20, 101 to 120) |
| `question` | `char[256]` | Question text |
| `options` | `char[4][128]` | Four answer choices |
| `correct_option` | `int` | Correct answer, 1-based (1 to 4) |
| `next` | `QNode*` | Next node in the linked list |

### 4.2 Player Record (`Player`)

| Group | Fields |
|---|---|
| Identity | `name[50]`, `id[32]` (roll number), `batch[20]`, `dept[16]` |
| Counters | `total_questions`, `attempted`, `correct`, `wrong`, `skipped` |
| Scores | `raw_score` (clamped at 0), `accuracy` (%), `ranking_score`, `rank` |
| BST links | `left`, `right` |
| Hash link | `hash_next` |

### 4.3 Hash Table and Answer Log

- **`HashTable`**: an array of `HASH_SIZE = 101` bucket pointers (101 is prime, which helps spread values).
- **`AnswerRecord`**: a per-question log of `questionId` and `selectedOption` (0 means skipped). It is used to write the transcript.

### 4.4 Shared Node Layout

```
 Hash table buckets                    Player node (one malloc per student)
   [42] ──────►  Player "2520853"        name, id, batch, dept, scores, rank
   [77] ──────►  Player "2530112"        left      ──►  BST child with smaller ID
                                         right     ──►  BST child with larger ID
 BST root ────►  (same Player nodes)     hash_next ──►  next node in same bucket
```

---

## 5. Algorithms and Data Structures

| Component | Technique | Why It Was Chosen | Average | Worst |
|---|---|---|---|---|
| Question bank | Singly linked list | Dynamic size, simple traversal | Traversal O(Q) | O(Q) |
| Random selection | Fisher-Yates shuffle | Unbiased-style shuffle, runs in linear time | O(Q) | O(Q) |
| Player lookup | DJB2 hash, separate chaining | Fast ID lookup | O(1) | O(n) |
| Player storage | Binary search tree (by ID) | Ordered traversal for saving and sorting | O(log n) | O(n) |
| Bubble Sort | Adjacent swaps with early exit | Baseline for comparison | O(n²) | O(n²) |
| Quick Sort | Lomuto partition | Fast average sort | O(n log n) | O(n²) |
| Merge Sort | Top-down, stable | Guaranteed O(n log n), stable | O(n log n) | O(n log n) |
| Competition ranking | Linear scan over sorted array | Ties share a rank (1, 2, 2, 4) | O(n) | O(n) |

Here Q is the number of questions (60) and n is the number of players.

---

## 6. Detailed Design

### 6.1 Take Quiz Flow

```
Student enters ID
  │
  ├── hash_lookup(id)
  │     ├── Found ──► "Welcome back" ──► Retake? (y/n)
  │     │               ├── n ──► Cancel, no changes
  │     │               └── y ──► Keep name, batch, dept
  │     └── Not found ──► Registration
  │                        ├── Valid roll ID ──► batch auto-detected
  │                        └── Invalid ID   ──► ask for batch
  │                        └── Ask full name, department
  ▼
run_quiz_session()          10 random questions, with feedback after each
  ▼
finalize_scores()           accuracy and ranking_score computed
  ▼
New player:  malloc ──► bst_insert ──► hash_insert
Retake:      overwrite in place, restore left / right / hash_next
  ▼
save_all_players()          rewrite players_scores.txt
bst_inorder_collect ──► merge_sort_players ──► assign_competition_ranks
write_result_file()         result_<ID>.txt
export_leaderboard_report() leaderboard_report.txt
```

### 6.2 Per-Question Logic

```
For each of 10 questions:
  Show question and four options, plus "0) Skip"
    │
    ▼
  Read input (0-4)
    │
    ├── 0 (skip)      status = -1   skipped; score unchanged           (0.00)
    ├── correct       status = +1   attempted++, correct++, raw += 1.00 (+1.00)
    └── wrong         status = 0    attempted++, wrong++, raw -= 0.25
                                    if raw < 0 then raw = 0            (-0.25)
    │
    ▼
  Show feedback and running score, wait for ENTER
    │
    ▼
After 10 questions: finalize_scores()
```

### 6.3 Ranking Pipeline

```
raw_score, correct, attempted
    │
    ▼
accuracy = correct / attempted × 100
    │
    ├── raw_score > 0 ──► ranking_score = raw × (0.50 + 0.50 × accuracy/100)
    └── raw_score = 0 ──► ranking_score = 0
    │
    ▼
Sort, highest first, using this priority:
  1. ranking_score   2. accuracy   3. raw_score   4. ID (ascending)
    │
    ▼
Competition ranking:  1, 2, 2, 4, 5, 5, 7 ...
```

### 6.4 Leaderboard Flow

```
[2] View Leaderboard
  │
  ├── No players? ──► message and return
  ├── bst_inorder_collect ──► array of players (sorted by ID)
  ├── Choose algorithm:  1 Bubble  |  2 Quick  |  3 Merge
  ├── Time the sort with clock() and convert to milliseconds
  ├── assign_competition_ranks()
  ├── Print top 15 on screen
  └── export_leaderboard_report() writes the full list to leaderboard_report.txt
```

### 6.5 Search and Access Control

```
[3] Search by ID
  read ID ──► decode_roll_no (display only) ──► hash bucket index shown
          ──► hash_lookup ──► found: show Player card   |   not found: message

[4] Browse Question Bank
  read ID ──► hash_lookup
          ├── not found ──► denied ("take the quiz first")
          └── found ──► show all 60 questions with correct answers marked
```

### 6.6 Main Loop State

```
          ┌──────────────┐
  start ─►│  Main menu   │◄────────────── after every handler returns
          └──────┬───────┘
                 │ choice 1 to 4
                 ▼
          ┌──────────────┐
          │   Handler    │
          └──────────────┘
                 │ choice 5
                 ▼
          ┌──────────────┐
          │  Shutdown    │  free question list, free BST, return 0
          └──────────────┘
```

---

## 7. Scoring Model

### 7.1 Formulas

```
accuracy       = correct / attempted × 100
raw_score      = Σ(correct × 1.00) − Σ(wrong × 0.25), never below 0
ranking_score  = raw_score × (0.50 + 0.50 × accuracy / 100)     if raw_score > 0
               = 0                                              otherwise
```

The ranking score rewards players who score well and answer accurately. A player who answers 100 questions with 50% accuracy gets less credit than a player with a higher accuracy and the same raw score.

### 7.2 Worked Examples

| Scenario | Correct | Wrong | Skipped | Attempted | Accuracy | Raw Score | Ranking Score |
|---|---|---|---|---|---|---|---|
| A | 7 | 2 | 1 | 9 | 77.78% | 6.50 | 6.50 × (0.5 + 0.389) = **5.78** |
| B | 0 | 3 | 7 | 3 | 0.00% | 0.00 (clamped) | **0.00** |
| C | 1 | 4 | 5 | 5 | 20.00% | 0.00 | **0.00** |

### 7.3 Order Sensitivity of the Clamp

Because the score is clamped at 0 after each wrong answer, the final score depends on the order of answers.

| Answer sequence | Step by step | Final raw score |
|---|---|---|
| Correct, Wrong, Wrong | 1.00, 0.75, 0.50 | **0.50** |
| Wrong, Wrong, Correct | 0.00, 0.00, 1.00 | **1.00** |

Both sequences contain one correct and two wrong answers, but they produce different scores. See Limitation L1 in Section 12.

---

## 8. Data Persistence

### 8.1 `players_scores.txt` Format

Each line is one player. The fields are separated by `|`:

```
name|id|batch|dept|total_q|attempted|correct|wrong|raw_score|accuracy|ranking_score
```

Example line:

```
Example Student|2520853|252|CSE|10|9|7|2|6.50|77.78|5.78
```

On load, `skipped` is recomputed as `total_questions - attempted`. Ranks are recomputed by the startup code.

### 8.2 Student ID Layout

The decoder accepts 7- or 8-digit IDs where the 4th digit is `0`.

```
Position:   1  2  │  3  │  4  │  5  6  7  (8)
Field:      YY    │  S  │  0  │  SERIAL
Meaning:    year  │ term│ fix │  serial number
Example:    25    │  2  │  0  │  853        →  2025 · Batch 252 · Serial 853
Example:    25    │  2  │  0  │  1000       →  2025 · Batch 252 · Serial 1000
```

`S` is documented as 1 = Spring and 2 = Fall. The code does not validate it.

### 8.3 Result Transcript (`result_<ID>.txt`)

1. Header with university name, application title, name, ID, decoded ID details, batch, department, and date.
2. For each of the 10 questions: a `[CORRECT]`, `[WRONG]`, or `[SKIPPED]` marker, the question, the student's answer, and the correct answer if the student was wrong or skipped.
3. Final summary: total, attempted, correct, wrong, skipped, raw score, accuracy, ranking score, and rank out of the total number of players.

### 8.4 Leaderboard Report (`leaderboard_report.txt`)

A fixed-width table with columns: Rank, ID, Name, Batch, Dept, Raw, Acc %, Score. It includes the algorithm used and the sort time, and lists all players.

---

## 9. Question Bank

The bank contains 60 questions, loaded at startup into a linked list.

| ID Range | Count | Theme |
|---|---|---|
| 1 to 20 | 20 | Bangladesh-focused general knowledge |
| 21 to 40 | 20 | World general knowledge |
| 101 to 110 | 10 | Bangladesh (second set) |
| 111 to 120 | 10 | World (second set) |

Each attempt selects 10 questions. All questions are shuffled, and the first 10 are taken. The correct answer is stored as a 1-based index (1 to 4). Some answers are time-sensitive or approximate (for example, the 2026 World Cup winner and the literacy rate), so they should be checked against current sources before submission.

---

## 10. User Interface Design

| Screen | Key Elements |
|---|---|
| Main menu | Quiz rules, marking scheme, status line (question count and player count), five options |
| Registration | Authentication banner, ID prompt, auto-detected batch or manual entry, name, department |
| Examination instructions | Candidate card and marking scheme, press ENTER to start |
| Question screen | Candidate header with progress bar, question number, four options, skip option |
| Answer feedback | CORRECT, WRONG (with the right answer), or SKIPPED, plus running score |
| Result card | Player profile, performance, raw score, accuracy, ranking score, current rank |
| Leaderboard | Algorithm name and timing, top 15 table, count of players not shown |
| Player search | Hash bucket index, decoded ID details, player card or not-found message |

All screens use ANSI colour codes. Terminals that do not support colour will show the codes as text, but the layout still works.

---

## 11. Complexity Analysis

| Operation | Complexity | Notes |
|---|---|---|
| Question shuffle and selection | O(Q) | Q = 60 |
| Quiz session (10 questions) | O(10 × Q) | Each transcript entry searches the list for its question |
| Hash lookup | O(1) average | Chain length is about n / 101 |
| BST insert or search | O(log n) average | O(n) if IDs were inserted in sorted order |
| Save or load | O(n log n) | Load inserts each record into the BST |
| Bubble Sort | O(n²) | Early exit when no swaps occur |
| Quick Sort | O(n log n) average | Recursion depth can grow to O(n) |
| Merge Sort | O(n log n) | Allocates temporary arrays for each merge |
| Competition ranking | O(n) | Single pass |

---

## 12. Known Limitations and Recommendations

| ID | Area | Observation | Recommendation |
|---|---|---|---|
| L1 | Scoring | The clamp at 0 is applied after each wrong answer, so the total depends on order (Section 7.3). | Track total correct and total wrong, then apply the clamp once at the end. |
| L2 | Security | Anyone who knows a student ID can retake the quiz (overwriting that record) or view the answers in option 4. | Add a password or PIN, and store only a hash of it. |
| L3 | Answer key | Answers are stored in the source code and in the compiled program. | Acceptable for a course project. For real use, load questions from an encrypted or server-side source. |
| L4 | BST balance | IDs inserted in increasing order make the tree a linked list. | Use an AVL or red-black tree, or a sorted array with binary search. |
| L5 | Quick Sort | Lomuto partition with the last element as pivot is O(n²) on sorted input. | Use a random or median-of-three pivot. |
| L6 | Shuffle | `rand() % (i + 1)` introduces slight modulo bias. | Use a better generator, or rejection sampling. |
| L7 | File format | A `\|` character in a name would break the pipe-delimited format. | Reject or escape `\|` in name input. |
| L8 | Hash case | `hash_function` lowercases, but `strcmp` in lookup is case-sensitive. Mixed-case IDs would hash to the same bucket but not match. | Use the same case rule in both, or normalize IDs on input. Not an issue for numeric IDs. |
| L9 | Fixed limits | `QNode* all[1000]` and `int indices[1000]` assume at most 1,000 questions. | Use dynamic arrays sized to the bank. |
| L10 | Timing | Take Quiz writes `0.0 ms` for the sort time in the report. | Pass the measured time, or label the field as not measured. |
| L11 | Input | `read_string_input` truncates IDs longer than the buffer without warning. | Warn the user if input is truncated. |
| L12 | Robustness | The `malloc` result in `handle_take_quiz` is not checked for NULL. | Check the return value and handle the error. |

---

## 13. Build, Run, and Test

### 13.1 Build and Run

```bash
gcc -Wall -Wextra -std=c11 DSA_TRIVIA.c -o dsa_trivia
./dsa_trivia
```

The header comment refers to `dsa_trivia_merged.c`. Use the name of the file you actually compile.

### 13.2 Test Cases

| ID | Scenario | Steps | Expected Result |
|---|---|---|---|
| TC-01 | New student, valid roll | Option 1, enter `2520853`, enter name and department | Batch `252` detected automatically, Admission Year 2025 shown |
| TC-02 | New student, invalid roll | Option 1, enter `12345` | Program asks for batch manually |
| TC-03 | Retake declined | Retake an existing ID, answer `n` | "No changes made", record unchanged |
| TC-04 | Skipped answers | Skip all 10 questions | Raw score 0.00, accuracy 0.00%, skipped count 10 |
| TC-05 | Browse before quiz | Option 4 with an unknown ID | Access denied with a message to take the quiz first |
| TC-06 | Sort consistency | Complete at least 3 quizzes, then view the leaderboard with each algorithm | Same order and ranks from all three algorithms |
| TC-07 | Persistence | Complete a quiz, exit, restart | Player appears on the leaderboard, ranks recomputed |
| TC-08 | Competition ranking | Two players with identical scores | Both share a rank, and the next rank is skipped |

---

## 14. Glossary

| Term | Meaning |
|---|---|
| BST | Binary Search Tree: each node's left child has a smaller key, and its right child has a larger key. |
| Chaining | Hash collision handling where each bucket holds a linked list of entries. |
| Competition ranking | Ranking where tied items share a rank and the next rank skips accordingly (1, 2, 2, 4). |
| DJB2 | A simple, widely used string hash function by Daniel J. Bernstein. |
| Fisher-Yates | An algorithm that shuffles an array in place in linear time. |
| Lomuto partition | A partition scheme for Quick Sort that uses the last element as the pivot. |
| Roll number | The student's university ID, used as the unique key for each player. |
