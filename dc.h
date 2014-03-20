#include "os345.h"

d_clock* dc_construct(int size);
void dc_destruct(d_clock** c);
int dc_insert(d_clock *c, Semaphore* sem, int tic);
int dc_remove(d_clock* c);
int dc_dec(d_clock* c);
void dc_print(d_clock* c);
int dc_test(void);
int dc_test_state(Semaphore** sems, int* tics);
int test_dc_dec(d_clock* c);
