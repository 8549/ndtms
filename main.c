#include <stdio.h>
#include <stdlib.h>

#define true 1
#define false 0

struct transition_s {
  int from;
  int to;
  char read;
  char write;
  char move;
};

struct transition_s *transitions = NULL;

int new_transition(int from, int to, char read_char, char write_char, char move) {
  transitions = realloc(transitions, (size_t) (sizeof(transitions) + sizeof(struct transition_s)));
  if (transitions == NULL) {
    return 0;
  }
  transitions->from = from;
  transitions->to = to;
  transitions->read = read_char;
  transitions->write = write_char;
  transitions->move = move;
  return 1;
}

int main(int argc, char const *argv[]) {
  int max = 0;
  char *line = NULL;
  size_t read = 0;

  // Transitions parsing
  getline(&line, &read, stdin);
  sscanf(line, " tr ", NULL);
  free(line);
  line = NULL;

  int matches = 0;
  do {
    int from = -1, to = -1;
    char read_char = '\0', write_char = '\0', move = '\0';
    getline(&line, &read, stdin);
    matches = sscanf(line, "%d %c %c %c %d ", &from, &read_char, &write_char, &move, &to);
    sprintf(stdout, "found %d matches\n", matches);
    if (matches == 5) {
      int result = new_transition(from, to, read_char, write_char, move);
      if (result != 1) {
        sprintf(stderr, "Error while creating new transition\n");
        exit(EXIT_FAILURE);
      }
    }
    else {
      sprintf(stderr, "Error while parsing transition(s)\n");
      exit(EXIT_FAILURE);
    }
    free(line);
    line = NULL;
  } while (matches == 5);

  ///// DEBUG
  int number_of_transitons = sizeof(transitions) / sizeof(struct transition_s);
  sprintf(stdout, "number of transitions: %d\n", number_of_transitons);
  for (int i = 0; i < number_of_transitons; i++) {
    struct transition_s current_transition = *(transitions + i*sizeof(struct transition_s));
    sprintf(stdout, "from %d to %d, read %c write %c, move %c\n", current_transition.from,
                                                                  current_transition.to,
                                                                  current_transition.read,
                                                                  current_transition.write,
                                                                  current_transition.move);
  }
  

  // Accepting states parsing
  int *acc = NULL;

  // Max tries parsing

  // Run strings parsing

  return 0;
}