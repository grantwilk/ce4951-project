#ifndef STATE_H
#define STATE_H

enum state {BUSY, IDLE, COLLISION};

state getCurrentState();

void setCurrentState(state nextState);

#endif
