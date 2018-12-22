#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int r;
    if ((r = execl("hello", "hello", 0)) < 0)
        panic("exec(hello) failed: %e", r);
}
