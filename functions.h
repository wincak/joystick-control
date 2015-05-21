// Header file for joystick control program

#include <signal.h>


// Signal handler
static void handler(int sig, siginfo_t *si, void *uc);

// Signal timer init
int timer_init(void);
