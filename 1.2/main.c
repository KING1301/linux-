#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
int main () 
{
	pid_t p1,p2,p3;
	 if ((p1=fork()) == 0) 
	{
		execv("./clock",NULL);
	}
	 else
	{
		if ((p2=fork())==0) 
		{
			execv("./cpu",NULL);	
		}
		else
		{
			if ((p3=fork()==0))
			{
				execv("./num",NULL);
			}
		}
	}
	 waitpid(p1,NULL,0);
	 waitpid(p2,NULL,0);
	 waitpid(p3,NULL,0);
	 return 0;
}


