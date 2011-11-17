#ifndef _CONST_H
#define _CONST_H

  #define dbgprint(...) if(debug) fprintf(stderr,__VA_ARGS__);

  enum resultCode { ERROR=-1, CHILD };

  enum command { ADD=1, FIND, WALK, STOP, INVALID };

  const int debug;
  const int DEAD;
  const int byte;
  const int word;
  
#endif
