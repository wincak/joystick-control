#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>
#include <signal.h>
#include <time.h>

#define JOY_DEV "/dev/input/js0"
#define CLOCKID CLOCK_REALTIME
#define SIG SIGUSR1

timer_t timerid;

struct sigevent sev;
struct itimerspec its;
long long freq_nanosecs;
sigset_t mask;
struct sigaction sa;

int timer1;	// timer overflow notification

// Signal handler for timer interrupt
static void handler(int sig, siginfo_t *si, void *uc)
{
	if(si->si_value.sival_ptr != &timerid){
		printf("Stray signal\n");
	} else {
		timer1 = 1;
		timer_settime(timerid, 0, &its, NULL);
	}
}

int main()
{
	int joy_fd, *axis=NULL, num_of_axis=0, num_of_buttons=0, x;
	char *button=NULL, name_of_joystick[80];
	struct js_event js;

	if( ( joy_fd = open( JOY_DEV , O_RDONLY)) == -1 )
	{
		fprintf(stderr, "Couldn't open joystick: ");
		fprintf(stderr, JOY_DEV"\n");
		return -1;
	}

	ioctl( joy_fd, JSIOCGAXES, &num_of_axis );
	ioctl( joy_fd, JSIOCGBUTTONS, &num_of_buttons );
	ioctl( joy_fd, JSIOCGNAME(80), &name_of_joystick );

	axis = (int *) calloc( num_of_axis, sizeof( int ) );
	button = (char *) calloc( num_of_buttons, sizeof( char ) );

	fprintf(stderr,"Joystick detected: %s\n\t%d axis\n\t%d buttons\n\n"
		, name_of_joystick
		, num_of_axis
		, num_of_buttons );
	fcntl( joy_fd, F_SETFL, O_NONBLOCK );	/* use non-blocking mode */

	// Start up the signal timer
	timer_init();
	timer1 = 0;

	while( 1 ) 	/* infinite loop */
	{

			/* read the joystick state */
		read(joy_fd, &js, sizeof(struct js_event));

			/* see what to do with the event */
		switch (js.type & ~JS_EVENT_INIT)
		{
			case JS_EVENT_AXIS:
				axis   [ js.number ] = js.value;
				break;
			case JS_EVENT_BUTTON:
				button [ js.number ] = js.value;
				break;
		}

		char axis_percent[num_of_axis];
		char count;

		for(count = num_of_axis-1; count >= 0; count--){
			axis_percent[count] = -1*axis[count]/(0x7FFF/100);
		}

		char tx_array[4];

		tx_array[0] = 250;	// Init symbol
		tx_array[1] = (signed char)axis_percent[0];
		tx_array[2] = (signed char)axis_percent[1];
		tx_array[3] = (signed char)axis_percent[2];
		tx_array[4] = button[0];


		if(timer1){
			for( count=0 ; count<=4 ; count++ ){
				putchar(tx_array[count]);
				//fprintf(stderr,"VAL: %i",tx_array[count]);
			}
			putchar('\n');
			timer1 = 0;
		}

		/*
		printf("%4i %4i %4i %i",axis_percent[0], axis_percent[1], 
			axis_percent[2], button[0]);
		*/

		fflush(stdout);

		usleep(1000);
		//sleep(1);

		/*
		printf("X: %4i Y: %4i Z: %4i B:", \
			 axis_percent[0], axis_percent[1], axis_percent[2] );

		for( x=0 ; x<num_of_buttons ; x++ ){
			printf("%i",button[x]);
		}
		fflush(stdout);

		usleep(1000);

		*/

		/*

			// print the results
		printf( "X: %6d  Y: %6d  ", axis[0], axis[1] );

		if( num_of_axis > 2 )
			printf("Z: %6d  ", axis[2] );

		if( num_of_axis > 3 )
			printf("R: %6d  ", axis[3] );

		for( x=0 ; x<num_of_buttons ; ++x )
			printf("B%d: %d  ", x, button[x] );

		printf("  \r");
		fflush(stdout);
		*/
	}

	close( joy_fd );	/* too bad we never get here */
	return 0;
}

int timer_init(void){
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = handler;
	sigemptyset(&sa.sa_mask);
	sigaction(SIG, &sa, NULL);

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIG;
	sev.sigev_value.sival_ptr = &timerid;
	timer_create(CLOCKID, &sev, &timerid);

	// Start the timer
	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = 200000000; // 2200ms
	its.it_interval.tv_sec = its.it_value.tv_sec;
	its.it_interval.tv_nsec = its.it_value.tv_nsec;

	// Activate timer
	timer_settime(timerid, 0, &its, NULL);
		
	return(1);
}

