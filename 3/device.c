#include <stdio.h>
#include <stdlib.h>
#include <linux/fcntl.h>

int main()
{
 int cdevfd;
 char w_buf[2024];
 char r_buf[1024]; 

 
 if((cdevfd=open("/dev/my_cdev",O_RDWR))==-1) //打开字符设备
  printf("open ERROR！\n");
 else
       {
	printf("open SUCCESS!\n");
 	printf("输入要写入到设备的内容:\n");
 	fgets(w_buf,sizeof(w_buf),stdin);
 	write(cdevfd,w_buf,sizeof(w_buf)); //写入设备
 	lseek(cdevfd,0,SEEK_SET); //把文件指针重新定位到文件开始的位置
 	read(cdevfd,r_buf,sizeof(r_buf)); //读取设备内容
 	printf("读取内容为:\n%s\n",r_buf);
	}
   return 0;
}
