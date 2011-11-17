#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<sys/wait.h>

#include"const.h"
#include"err.h"
#include"nodeCode.h" /* treść funkcji genChild => kod węzłów */

const int BUFSIZE = 200;

int parseCmd(char* line, int *arg) {
/* Funkcja parseCmd odczytuje polecenie przekazane w stringu line, ewentualny argument zapisuje *
 * w zmiennej arg. Zwraca kod polecenia albo INVALID <=> line nie zawiera poprawnego polecenia  *
 * Modyfikowane parametry: arg - zawiera liczbe podawana z ADD lub FIND                         */
  enum command result = INVALID;
  char *word1, *word2, *endptr = NULL;
  if(strcmp(line, "walk") == 0) result = WALK;
  else
    if(strcmp(line, "stop") == 0) result = STOP;
    else {
      word1 = strtok(line, " ");
      if(word1 != NULL) {
        if(strcmp(word1, "add") == 0) result = ADD;
        else
          if(strcmp(word1, "find") == 0) result = FIND;
      }
      if(result == ADD || result == FIND) {
        word2 = strtok(NULL, " ");
        if(word2 == NULL) result = INVALID;
        else {
          errno = 0;
          *arg = strtol(word2, &endptr, 10);
          if(errno != 0 || (endptr != NULL && *endptr != '\0') || strtok(NULL, " ") != NULL)
            result = INVALID;
        }
      }
    }
  return result;
}

int main() {
  /* cmdString - linia wczytana z wejścia, cmd - kod odczytanej komendy,   *
   * rootPid - pid procesu korzenia, jeśli != DEAD, oznacza, że korzeń już *
   * utworzono, treeSize - liczba wczytanych wartości (potrzebna do walk)  */
  size_t len = BUFSIZE;
  char *cmdString = malloc(len), cmd;
  int rootPid = DEAD, argument, treeSize = 0, i;
  int rootIn[2], rootOut[2];           /* deskryptory łącz do i z korzenia */
  dbgprint("Supervisor init...\n");
  dbgprint("%d %d %d %d %d\n", ADD, FIND, WALK, STOP, INVALID);
  if(debug) setvbuf(stdout, NULL, _IONBF, 0);
  /* podstawowy schemat wykonywania komendy - odczytujemy zmienną rootPid, *
     aby dowiedzieć się, czy korzeń żyje (= drzewo jest niepuste), jeśli   *
     tak to wysyłamy komendę do korzenia i ewentualnie czekamy na odpowiedź*/                                  
  while(1) {
    cmdString[getline(&cmdString, &len, stdin)-1] = '\0';
    cmd = parseCmd(cmdString, &argument);
    dbgprint("cmd = %d\n", cmd);
    switch(cmd) {
      case ADD:
        treeSize++;
        if(rootPid == DEAD) rootPid = genChild(argument, rootIn, rootOut);
        else {
          if(write(rootIn[1], &cmd, byte) == ERROR) syserr("SUP add cmd send");
          if(write(rootIn[1], &argument, word) == ERROR) syserr("SUP add arg send");
        }
        break;

      case FIND:
        if(rootPid == DEAD) printf("0");
        else {
          dbgprint("FIND\n");
          if(write(rootIn[1], &cmd, byte) == ERROR) syserr("SUP find cmd send");
          if(write(rootIn[1], &argument, word) == ERROR) syserr("SUP find arg send");
          if(read(rootOut[0], &argument, word) == ERROR) syserr("SUP find result read");
          printf("%d\n", argument);
        }
        break;

      case WALK:
        if(rootPid == DEAD) printf("<empty>");
        else {
          dbgprint("WALK\n");
          if(write(rootIn[1], &cmd, byte) == ERROR) syserr("SUP wlk cmd send");
          if(treeSize > 0) {
            for(i = 0; i < treeSize-1; i++) {
              if(read(rootOut[0], &argument, word) == ERROR) syserr("SUP walk result read");
              printf("%d ", argument);
            }
            if(read(rootOut[0], &argument, word) == ERROR) syserr("SUP last walk result read");
            printf("%d\n", argument);
          }
        }
        break;
      
      case INVALID:
        printf("Invalid command.\n");
        break;

      case STOP:
        if(rootPid != DEAD) {
          if(write(rootIn[1], &cmd, byte) == ERROR) syserr("SUP stop cmd send");
          if(wait(0) == ERROR) syserr("SUP wait for root error");
          dbgprint("rootDead\n");
          if(close(rootIn[1]) == ERROR) syserr("SUP close rootfd error");
          if(close(rootOut[0]) == ERROR) syserr("SUP close rootfd error");
        }
        dbgprint("Finished.\n");
        return 0;
      
      default:
        fatal("Default in supervisor reached\n");
    }
  }
} /* main() */
