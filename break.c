#include <stdio.h>

void do_thing(unsigned short *thisval)
{
  unsigned long u;
  unsigned short target = 3461;
  for (u=0; u<100000000000; u++)
    {
      if (u < target)
	{
	  printf("u is shorter: %u than %u\n", u, *thisval);
	  *thisval = u;
	  printf("*thisval now %u\n", *thisval);
	}
    }
}

int main(void)
{
  unsigned short someval = 34;
  printf("someval is %d\n", someval);
  do_thing(&someval);
  printf("someval is %d\n", someval);
  return 0;
}
