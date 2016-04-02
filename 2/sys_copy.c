#include <linux/unistd.h>
#include <stdio.h>
#include <asm/unistd.h>

int main(int argc,char*argv[])
{
    if(argc!=3)
    {
    	printf("参数错误 \n");
    	return -1;
    }
    if(syscall(357,argv[1],argv[2])==0)
	printf("系统调用成功! \n");
    else
	printf("调用失败!\n");
    return 0;
}

