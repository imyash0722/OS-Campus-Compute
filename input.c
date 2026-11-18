#include "common.h"

bool read_int(const char *prompt, int lo, int hi, int *out) {
  char buf[64];
  while (1) {
    printf("%s", prompt);
    if (!fgets(buf, sizeof(buf), stdin))
      return false;
    if (!strchr(buf, '\n')) {
      int c;
      while ((c = getchar()) != '\n' && c != EOF) {
      }
    }
    char *end;
    long v = strtol(buf, &end, 10);
    if (end != buf && (*end == '\n' || *end == '\0') && v >= lo && v <= hi) {
      *out = (int)v;
      return true;
    }
    printf("  Invalid input: enter an integer from %d to %d.\n", lo, hi);
  }
}
