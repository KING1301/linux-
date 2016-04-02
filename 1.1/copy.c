#include <stdio.h>
int main(int argc,char *argv[])
{
   if(argc!=3)
   {
	printf("参数格式错误\n");
	return 0;
   }
   copy_file(argv[1], argv[2]);
   return 0; 
}

int copy_file(const char* s_file, const char* t_file)
{
   char buf[128];
   int r_num;
   int ret=0;
   FILE *source=NULL,*target=NULL;
   source=fopen(s_file,"rb");	
   target=fopen(t_file,"wb");
   if(source!=NULL&&target!=NULL)
    {
	      while((r_num=fread(buf,1,sizeof(buf),source)))
		   {  		
				 if (fwrite(buf,1,r_num,target)<0)
                  {
                            printf(" write target file error!\n");
		                     ret=-3;
		                     break;
                  }
		   } 
		   if(r_num<0)
		   {
				printf("read source file error!\n");
		        ret=-2;
		   }		
   }
	else
	{
		printf("open file error!\n");
		ret=-1;
	}
   if(source!=NULL)
  	 fclose(source);
   if(target!=NULL)
  	 fclose(target);
   if(ret==0)
		printf("copyfile success!\n");
   return ret; 	
}

