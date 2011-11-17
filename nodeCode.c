#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>

#include"const.h"
#include"err.h"
#include"nodeCode.h"

int genChild(int val, int sonIn[2], int sonOut[2]) {
/* Funkcja uruchamia nowy proces węzła drzewa, przechowujący wartość przekazaną w val     *
 * Tworzy łącza we-wy dla nowego węzła, udostępnia je ojcu poprzez argumenty sonIn i Out  *
 * Zwraca pid syna (dzięki temu ojciec wie, czy utworzył już proces potomny)              *
 * Modyfikowane parametry: sonIn, sonOut (wywołujący otrzymuje nowe deskryptory)          */
  dbgprint("genChild(%d,sonIn, sonOut) invoked\n", val);
  int pid = DEAD, i;
  char cmd;                                                /* Instrukcja odebrana od ojca */
  int added;                                               /* Argument instrukcji  */
  int value = val, count = 1, lTreeSize = 0, rTreeSize = 0;/* Dane drzewa */
  int lSonPid = DEAD, rSonPid = DEAD;                      /* Pidy synów (określają, czy żyją) */
  int lSonIn[2], lSonOut[2], rSonIn[2], rSonOut[2];        /* Deskryptory łącz do synów */
  int targetPid, *targetIn, *targetOut;                    /* Cel find'a */

  if(pipe(sonIn) == ERROR) syserr("Error in pipe(sonIn)\n");
  if(pipe(sonOut) == ERROR) syserr("Error in pipe(sonOut)\n");

  switch(pid = fork()) {
    case ERROR: syserr("Error in fork()"); break;

    case CHILD:
      dbgprint("Child, pid %d, value %d\n", getpid(), value);
      /* Podmiana deskryptorów */
      if(dup2(sonIn[0], 0) == ERROR) syserr("dup2");
      if(dup2(sonOut[1], 1) == ERROR) syserr("dup2");
      if(close(sonIn[0]) == ERROR) syserr("Close on filedesc init");
      if(close(sonIn[1]) == ERROR) syserr("Close on filedesc init");
      if(close(sonOut[0]) == ERROR) syserr("Close on filedesc init");
      if(close(sonOut[1]) == ERROR) syserr("Close on filedesc init");
      /* Deskryptory podmienione */
      while(1) {
        read(0, &cmd, byte); /* Odbieranie polecenia  */
        dbgprint("Wczytalem %d\n", cmd);
        switch(cmd) {
          case ADD:
            dbgprint("ADD command in %d, %d added\n", getpid(), added);
            if(read(0, &added, word) == ERROR) syserr("Add arg read");
            if(added == value) count++;
            else
            if(added < value) {
              lTreeSize++;
              if(lSonPid == DEAD) lSonPid = genChild(added, lSonIn, lSonOut);
              else {
                if(write(lSonIn[1], &cmd, byte) == ERROR) syserr("Add cmd send");
                if(write(lSonIn[1], &added, word) == ERROR) syserr("Add arg send");
              }
            } else {
              rTreeSize++;
              if(rSonPid == DEAD) rSonPid = genChild(added, rSonIn, rSonOut);
              else {
                if(write(rSonIn[1], &cmd, byte) == ERROR) syserr("Add cmd send");
                if(write(rSonIn[1], &added, word) == ERROR) syserr("Add arg send");
              }
            }
            break;

          case FIND:
            if(read(0, &added, word) == ERROR) syserr("Find arg read");
            dbgprint("FIND command in %d, %d added\n", getpid(), added);
            if(added == value) {        /* wysyłamy liczbę własnych wartości */
              if(write(1, &count, word) == ERROR) syserr("Find eq result send");
            } else {                    /* wybieramy syna, do którego przekazać zapytanie */
              if(added < value) {
                targetPid = lSonPid;
                targetOut = lSonOut;
                targetIn = lSonIn;
              } else {
                targetPid = rSonPid;
                targetOut = rSonOut;
                targetIn = rSonIn;
              }
              if(targetPid != DEAD) {
                if(write(targetIn[1], &cmd, byte) == ERROR) syserr("Find cmd send %d", targetIn[1]);
                if(write(targetIn[1], &added, word) == ERROR) syserr("Find arg send");
                if(read(targetOut[0], &added, word) == ERROR) syserr("Find result read");
                if(write(1, &added, word) == ERROR) syserr("Find result send");
              } else {
                added = 0;
                if(write(1, &added, word) == ERROR) syserr("Find result send0");
              }
            }
            break;

          case WALK:
            dbgprint("WALK-in-node\n");
            /* zczytujemy wartości z synów w kolejności infiksowej i wysyłamy je natychmiast     *
             * w górę (wszystko odbywa się prawidłowo, bo procesy znają rozmiary swoich poddrzew *
             * a łącza są kolejkami)                                                             */
            if(lSonPid != DEAD) {
              if(write(lSonIn[1], &cmd, byte) == ERROR) syserr("Write cmd send");
              for(i = 0; i < lTreeSize; i++) {
                if(read(lSonOut[0], &added, word) == ERROR) syserr("Write result read lson");
                if(write(1, &added, word) == ERROR) syserr("Write result send lson");
              }
            }
            for(i=0; i < count; i++) write(1, &value, word);
            if(rSonPid != DEAD) {
              if(write(rSonIn[1], &cmd, byte) == ERROR) syserr("Write cmd send rson");
              for(i = 0; i < rTreeSize; i++) {
                if(read(rSonOut[0], &added, word) == ERROR) syserr("Write result read rson");
                if(write(1, &added, word) == ERROR) syserr("Write result send rson");
              }
            }
            break;

          case STOP:
            dbgprint("Finishing in %d...\n", getpid());
            if(lSonPid != DEAD) {
              if(write(lSonIn[1], &cmd, sizeof(char)) == ERROR) syserr("Stop cmd send");
              if(wait(0) == ERROR) syserr("Wait for lson error");
              dbgprint("lSon of %d dead\n", getpid());
              if(close(lSonIn[1]) == ERROR) syserr("Close lSonIn");
            }
            if(rSonPid != DEAD) {
              if(write(rSonIn[1], &cmd, sizeof(char)) == ERROR) syserr("Stop cmd send");
              if(wait(0) == ERROR) syserr("Wait for rson error");
              dbgprint("rSon of %d dead\n", getpid());
              if(close(rSonIn[1]) == ERROR) syserr("Close rSonIn");
            }
            exit(0);
          
          default:
            fatal("Default in %d reached.\n", getpid());
        }
      }
      break; /* case CHILD */

    default:
      dbgprint("Parent %d, generated child %d\n", getpid(), pid);
      if(close(sonIn[0]) == ERROR) syserr("Close SonIn in parent");
      if(close(sonOut[1]) == ERROR) syserr("Close SonOut in parent");
      break;
  }
  return pid;
}
