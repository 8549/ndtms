#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define true 1
#define false 0
#define TR_SIZE (26 + 26 + 10 + 1)
#define COMP_OK '1'
#define COMP_ERROR '0'
#define COMP_MAX 'U'
#define COMP_UNDEFINED '?'
#define MOVE_LEFT 'L'
#define MOVE_RIGHT 'R'
#define MOVE_STILL 'S'
#define BLANK_CHAR '_'
#define GROW_SIZE 10

struct transition_s {
    int from;
    int to;
    char read;
    char write;
    char move;
    struct transition_s *next;
};

struct state_s {
    unsigned short int valid;
    unsigned short int acceptor;
    struct transition_s *tr[TR_SIZE];
};

struct snapshot_s {
    int current_state;
    char *tape_start;
    char *tape_end;
    int tape_index;
    unsigned short resized;
    unsigned long moves;
    struct snapshot_s *next;
};

struct transition_s *transitions = NULL;
struct state_s *states = NULL;
struct snapshot_s *queue = NULL;
struct snapshot_s *tail = NULL;

long number_of_transitions = 0;
unsigned long max = 0;

void empty_queue() {
    if (queue == NULL) {
        return;
    }
    struct snapshot_s * next = queue->next;
    do {
        free(queue->tape_start);
        free(queue);
        queue = next;
        if (queue != NULL) {
            next = queue->next;
        }
    } while (queue != NULL);
    free(queue);
    queue = NULL;
}

void add_branch(struct snapshot_s *snap) {
    if (queue == NULL) {
        queue = snap;
    }
    else {
        tail->next = snap;
    }
    tail = snap;
}

struct snapshot_s * new_snapshot(int state, unsigned long moves, char *tape_start, char *tape_end, int index) {
    struct snapshot_s *ret = malloc(sizeof(struct snapshot_s));
    ret->resized = false;
    ret->current_state = state;
    ret->tape_start = tape_start;
    ret->tape_end = tape_end;
    ret->tape_index = index;
    ret->moves = moves;
    ret->next = NULL;
    return ret;
}

int get_tr_mapping(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c == BLANK_CHAR) {
        return 10;
    } else if (c >= 'A' && c <= 'Z') {
        return c - 65 + 11;
    } else if (c >= 'a' && c <= 'z') {
        return c - 97 + 11 + 26;
    } else {
        printf("Error while mapping char to index\n");
        exit(EXIT_FAILURE);
    }
}

int add_transition(int from, int to, char read_char, char write_char, char move) {
    number_of_transitions++;
    transitions = realloc(transitions, (size_t) (number_of_transitions * sizeof(struct transition_s)));
    if (transitions == NULL) {
        return 0;
    }
    (transitions + number_of_transitions - 1)->from = from;
    (transitions + number_of_transitions - 1)->to = to;
    (transitions + number_of_transitions - 1)->read = read_char;
    (transitions + number_of_transitions - 1)->write = write_char;
    (transitions + number_of_transitions - 1)->move = move;
    (transitions + number_of_transitions - 1)->next = NULL;
    return 1;
}

char perform_queue() {
    // The global status should start with ? (undefined)
    char comp_status = COMP_UNDEFINED;
    // If the queue is not empty
    if (queue != NULL) {
        // While there are elements in the queue
        while (queue != NULL) {
            // Process the first snapshot
            struct snapshot_s *current_snap = queue;
            // if current state is acceptor, update global status to 1 and break the loop
            if (states[current_snap->current_state].acceptor == true) {
                comp_status = COMP_OK;
                // Remove entire queue
                //empty_queue(); moved before return
                // Free the memory as well
                //free(current_snap->tape_start);
                //free(current_snap); unneeded, current_snap points at queue
                break;
            }
            char next_char;
            if ((current_snap->tape_start + current_snap->tape_index > current_snap->tape_end) ||
                    (current_snap->tape_start + current_snap->tape_index < current_snap->tape_start)) {
                next_char = BLANK_CHAR;
            }
            else {
                next_char = *(current_snap->tape_start + current_snap->tape_index);
            }
            struct transition_s *tra = states[current_snap->current_state].tr[get_tr_mapping(next_char)];
            // If no transition found for this input char, kill off this branch, set status to 0 (only if it's not 1 nor U)
            if (tra == NULL) {
                if (comp_status == COMP_UNDEFINED) {
                    comp_status = COMP_ERROR;
                }
                // Remove this snapshot from the queue
                queue = current_snap->next;
                // Free the memory as well
                free(current_snap->tape_start);
                free(current_snap);
                continue;
            }
            // If one transition is found for this input char, update the current snapshot without creating a new one, dont update the global status
            else if (tra->next == NULL) {
                // If the transition is a cycle set the status to U and kill off the branch
                // or
                // If the transition is a blank eater cycle set the status to U and kill off the branch
                // index = 0 && move=left && read, write = blank
                // or
                // index = fine && move = right && read, write = blank
                if ((current_snap->resized == true && tra->move == MOVE_LEFT && tra->read == BLANK_CHAR && tra->from == tra->to) ||
                    (current_snap->resized == true && tra->move == MOVE_RIGHT && tra->read == BLANK_CHAR && tra->from == tra->to)/* ||
                    (tra->read == tra->write && tra->move == MOVE_STILL && tra->from == tra->to)*/) {
                    if (comp_status == COMP_UNDEFINED) {
                        comp_status = COMP_MAX;
                    }
                    // Remove this snapshot from the queue
                    queue = current_snap->next;
                    // Free the memory as well
                    free(current_snap->tape_start);
                    free(current_snap);
                    current_snap->resized = false;
                    continue;
                }
                // current state = transition.to
                current_snap->current_state = tra->to;
                // overwrite the current char on tape with "write" member of the transition
                current_snap->tape_start[current_snap->tape_index] = tra->write;
                // Calculate tape length
                //size_t tape_size = strlen(current_snap->tape_start); use a more efficient way to calculate tape size
                size_t tape_size = current_snap->tape_end - current_snap->tape_start;
                if (tape_size >= 0) {
                    tape_size++;
                }
                else if (tape_size < 0) {
                    printf("This is bad (tape size < 0)\n");
                    exit(EXIT_FAILURE);
                }
                // if move == L
                if (tra->move == MOVE_LEFT) {
                    //realloc and shift
                    if (current_snap->tape_index - 1 < 0) {
                        size_t new_size = (tape_size + GROW_SIZE) * sizeof(char);
                        char *old_start = realloc(current_snap->tape_start, new_size);
                        if (old_start == NULL) {
                            printf("Bad realloc\n");
                            exit(EXIT_FAILURE);
                        }
                        else {
                            current_snap->tape_start = old_start;
                        }
                        memmove(current_snap->tape_start + GROW_SIZE, current_snap->tape_start, tape_size);
                        memset(current_snap->tape_start, BLANK_CHAR, GROW_SIZE);
                        current_snap->tape_index = GROW_SIZE + current_snap->tape_index;
                        current_snap->tape_end = current_snap->tape_start + new_size -1;
                        current_snap->resized = true;
                    }
                    current_snap->tape_index--;
                }
                // if move == R
                else if (tra->move == MOVE_RIGHT) {
                    if (current_snap->tape_index + 1 >= tape_size) {
                        // realloc
                        //size_t new_size = (current_snap->tape_end - current_snap->tape_start) + GROW_SIZE;
                        size_t new_size = (tape_size + GROW_SIZE) * sizeof(char);
                        char *old_start = realloc(current_snap->tape_start, new_size);
                        if (old_start == NULL) {
                            printf("Bad realloc\n");
                            exit(EXIT_FAILURE);
                        }
                        else {
                            current_snap->tape_start = old_start;
                        }
                        memset(current_snap->tape_start + tape_size, BLANK_CHAR, GROW_SIZE);
                        current_snap->tape_end = current_snap->tape_start + new_size -1;
                        current_snap->resized = true;
                    }
                    current_snap->tape_index++;
                }
                // If the current number of moves +1 exceeds max, set the global status to U and break the loop, else moves++
                if (current_snap->moves + 1 > max) {
                    comp_status = COMP_MAX;
                    //break;
                    // Remove this snapshot from the queue
                    queue = current_snap->next;
                    // Free the memory as well
                    free(current_snap->tape_start);
                    free(current_snap);
                    continue; // NOT SURE ABOUT THIS
                }
                else {
                    current_snap->moves++;
                }
            }
            else if (tra->next != NULL) {
                // If 2+ transitions are found for this input char, add a new branch after executing it (remember to update the number of moves)
                // do the same as for 1 transition but dont do it in place, create a new snapshot and operate on the copy
                do {
                    // If the transition is a cycle set the status to U and kill off the branch
                    // or
                    // If the transition is a blank eater cycle set the status to U and kill off the branch
                    // index = 0 && move=left && read, write = blank
                    // or
                    // index = fine && move = right && read, write = blank
                    if ((current_snap->resized == true && tra->move == MOVE_LEFT && tra->read == BLANK_CHAR && tra->from == tra->to) ||
                        (current_snap->resized == true && tra->move == MOVE_RIGHT && tra->read == BLANK_CHAR && tra->from == tra->to)/* ||
                        (tra->read == tra->write && tra->move == MOVE_STILL && tra->from == tra->to)*/) {
                        if (comp_status == COMP_UNDEFINED) {
                            comp_status = COMP_MAX;
                        }
                        if (tra->next != NULL) {
                            continue;
                        }
                        else {
                            queue = current_snap->next;
                            // Free the memory as well
                            free(current_snap->tape_start);
                            free(current_snap);
                            current_snap->resized = false;
                            continue;
                        }
                    }
                    // Spawn a clone of the current snapshot
                    //size_t tape_size = strlen(current_snap->tape_start); // can substitute with more efficient tape_end - tape_start + 1
                    size_t tape_size = current_snap->tape_end - current_snap->tape_start;
                    if (tape_size >= 0) {
                        tape_size++;
                    }
                    else if (tape_size < 0) {
                        printf("This is bad (tape size < 0)\n");
                        exit(EXIT_FAILURE);
                    }
                    char *new_tape = malloc(tape_size * sizeof(char));
                    memmove(new_tape, current_snap->tape_start, tape_size * sizeof(char));
                    char *new_tape_end = new_tape + tape_size -1;
                    struct snapshot_s *duplicate = new_snapshot(current_snap->current_state, current_snap->moves, new_tape, new_tape_end, current_snap->tape_index);
                    // Perform the transition as if we were in the case before
                    duplicate->current_state = tra->to;
                    duplicate->tape_start[duplicate->tape_index] = tra->write;
                    if (tra->move == MOVE_LEFT) {
                        if (duplicate->tape_index - 1 < 0) {
                            size_t new_size = (tape_size + GROW_SIZE) * sizeof(char);
                            char *old_start = realloc(duplicate->tape_start, new_size);
                            if (old_start == NULL) {
                                printf("Bad realloc\n");
                                exit(EXIT_FAILURE);
                            }
                            else {
                                duplicate->tape_start = old_start;
                            }
                            memmove(duplicate->tape_start + GROW_SIZE, duplicate->tape_start, tape_size);
                            memset(duplicate->tape_start, BLANK_CHAR, GROW_SIZE);
                            duplicate->tape_index = GROW_SIZE + duplicate->tape_index;
                            duplicate->tape_end = duplicate->tape_start + new_size -1;
                            duplicate->resized = true;
                        }
                        duplicate->tape_index--;
                    }
                        // if move == R
                    else if (tra->move == MOVE_RIGHT) {
                        if (duplicate->tape_index + 1 >= tape_size) {
                            size_t new_size = (tape_size + GROW_SIZE) * sizeof(char);
                            char *old_start = realloc(duplicate->tape_start, new_size);
                            if (old_start == NULL) {
                                printf("Bad realloc\n");
                                exit(EXIT_FAILURE);
                            }
                            else {
                                duplicate->tape_start = old_start;
                            }
                            memset(duplicate->tape_start + tape_size, BLANK_CHAR, GROW_SIZE);
                            duplicate->tape_end = duplicate->tape_start + new_size -1;
                            duplicate->resized = true;
                        }
                        duplicate->tape_index++;
                    }
                    if (duplicate->moves + 1 > max) {
                        comp_status = COMP_MAX;
                        break;
                    }
                    else {
                        duplicate->moves++;
                    }
                    // Add this duplicate to the queue
                    add_branch(duplicate);
                    tra = tra->next;
                } while (tra != NULL);
                // Remove this snapshot from the queue
                queue = current_snap->next;
                // Free the memory as well
                free(current_snap->tape_start);
                free(current_snap);
            }
            else {
                printf("This should have never happened.\n");
            }
        }
    }
    empty_queue();
    return comp_status;
}

int main(int argc, char const *argv[]) {
    char *line = NULL;
    size_t read = 0;
    int max_state = 0;

    // Transitions parsing
    getline(&line, &read, stdin);
    // Ignore "tr"
    free(line);
    line = NULL;

    int matches = 0;
    do {
        int from = -1, to = -1;
        char read_char = '\0', write_char = '\0', move = '\0';
        getline(&line, &read, stdin);
        matches = sscanf(line, "%d %c %c %c %d ", &from, &read_char, &write_char, &move, &to);
        if (matches == 5) {
            if (from > max_state) {
                max_state = from;
            }
            if (to > max_state) {
                max_state = to;
            }
            int result = add_transition(from, to, read_char, write_char, move);
            if (result != 1) {
                printf("Error while creating new transition\n");
                exit(EXIT_FAILURE);
            }
        }
        free(line);
        line = NULL;
        read = 0;
    } while (matches == 5);

    // Create states array
    states = calloc(max_state + 1, sizeof(struct state_s));

    for (int i = 0; i < number_of_transitions; i++) {
        int from = transitions[i].from;
        if (states[from].valid == false) {
            states[from].valid = true;
        }
        // Add transition
        struct transition_s *h = states[from].tr[get_tr_mapping(transitions[i].read)];
        if (h != NULL) {
            while (h->next != NULL) {
                h = h->next;
            }
            h->next = &(transitions[i]);
        } else {
            states[from].tr[get_tr_mapping(transitions[i].read)] = &(transitions[i]);
        }


        int to = transitions[i].to;
        if (states[to].valid == false) {
            states[to].valid = true;
        }
    }

    // We already consumed "acc"
    // Accepting states parsing
    int cont = 0;
    do {
        getline(&line, &read, stdin);
        int acc_state;
        cont = sscanf(line, " %d ", &acc_state);
        //printf("parsed acc %d\n", acc_state);
        if (acc_state >= 0 && acc_state <= max_state) {
            states[acc_state].acceptor = true;
        }
        free(line);
        line = NULL;
    } while (cont == 1);

    // We already consumed "max"
    // Max tries parsing
    getline(&line, &read, stdin);
    sscanf(line, " %lu ", &max);

    // PARSING DEBUG
    /*for (int i = 0; i < max_state + 1; i++) {
        printf("state %d, valid: %d, acceptor: %d\n", i, states[i].valid, states[i].acceptor);
        for (int j = 0; j < TR_SIZE; j++) {
            struct transition_s *k = states[i].tr[j];
            if (k != NULL) {
                do {
                    printf("\t%d %c %c %c %d\n", k->from, k->read, k->write, k->move, k->to);
                    k = k->next;
                } while (k != NULL);
            }
        }
    }
    printf("max %lu\n", max);*/
    // END PARSING DEBUG

    // Consume run
    getline(&line, &read, stdin);
    free(line);
    line = NULL;
    // Run strings parsing
    signed long int res;
    do {
        res = getline(&line, &read, stdin);
        if (res == -1) {
            break;
        }
        signed long int char_count = res - 1;
        char result = COMP_UNDEFINED;
        if (char_count >= max) {
            result = COMP_MAX;
        }
        else {
            size_t initial_size = sizeof(char) * char_count;
            char *initial_tape = malloc(initial_size);
            memmove(initial_tape, line, initial_size);
            add_branch(new_snapshot(0, 0, initial_tape, initial_tape+initial_size-1, 0));
            result = perform_queue();
        }
        free(line);
        line = NULL;
        printf("%c\n", result);
    } while (res != -1);
    // free tutto quanto
    free(line);
    free(transitions);
    free(states);
    return 0;
}