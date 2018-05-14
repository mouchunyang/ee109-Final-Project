volatile unsigned int time_us;
volatile int distance;
volatile int pulse_count;
unsigned char pulse_flag;
unsigned char mode_state;

volatile int min_value;
volatile unsigned char rotary_newstate, rotary_oldstate;

volatile int RX_startflag;
volatile int RX_endflag;
volatile char bufff[10];
volatile int bufff_count;

volatile char acquire;