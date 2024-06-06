#ifndef STATES_H
#define STATES_H

typedef enum {
    DISCONNECTED,
    CONNECTING,
    AUTHENTICATING,
    CONNECTED,
    MESSAGING,
    ERROR,
    DISCONNECTING
} State;

extern State current_state;

void handle_transition(State new_state);

#endif // STATES_H
