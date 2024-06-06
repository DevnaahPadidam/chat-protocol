#include "states.h"
#include <stdio.h>

State current_state;

void handle_transition(State new_state) {
    printf("Transitioning from %d to %d\n", current_state, new_state);
    current_state = new_state;
    // Additional logic for entering new state
    switch (new_state) {
        case CONNECTING:
            // Initialize connection
            break;
        case AUTHENTICATING:
            // Handle authentication
            break;
        case CONNECTED:
            // Connection established
            break;
        case MESSAGING:
            // Ready for messaging
            break;
        case ERROR:
            // Handle error
            break;
        case DISCONNECTING:
            // Handle disconnection
            break;
        case DISCONNECTED:
            // Clean up resources
            break;
    }
}
