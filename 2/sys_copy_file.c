asmlinkage int sys_copy_file(const char* s_file, const char* t_file) 
{        
    int fd_source,fd_target;
    char File_buf[512];             
    int num;
    int ret=0;
    mm_segment_t old_fs=get_fs();//保存旧的地址空间限制  
    set_fs(get_ds());//扩展设置地址空间到内核，避免系统调用时地址空间检查失败
	//系统调用打开源文件
    fd_source = sys_open(s_file,O_RDONLY,S_IRUSR);
    //系统调用创建并打开目标文件 
    fd_target = sys_open(t_file,O_RDWR | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR);
    if(fd_source<0||fd_target<0)
    {
		 printk("file open error!\n");
         ret=-1;
	}
     else
     {
             while( (num=sys_read (fd_source,File_buf,sizeof(File_buf) ) ) )
             {        /* 拷贝文件，若失败，返回-3 */
                      if (sys_write(fd_target,File_buf,num)<0)
                        {
                            printk(" write target file error!\n");
		                     ret=-3;
		                     break;
                         }
             }
	         if(num<0)
	          {
	                printk("read source file error!\n");
		            ret=-2;
	          }
     }
	if(fd_source>=0)
		sys_close(fd_source);
	if(fd_target>=0)
		sys_close(fd_target);
    set_fs(old_fs);                     /* 恢复地址空间 */
    if(ret==0)
        printk("sys_copyfile  syscall success!\n");
    return ret;
}  




