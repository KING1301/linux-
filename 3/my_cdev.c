#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#define SIZE 1024  //缓冲区大小
/*设备描述结构体*/
typedef struct my_cdev
{
    struct cdev cdev;//cdev结构体
    char *bufdata;    //数据缓冲区
} my_cdev;

/*设备描述结构体指针*/
my_cdev *my_cdevp;
//int MAJOR;//主设备号
dev_t devt;//设备号
/*打开函数*/
int my_copen(struct inode *inode, struct file *filp)
{
    /*将设备描述结构指针赋值给文件私有数据指针*/
    filp->private_data = my_cdevp;
    return 0;
}

/*释放函数*/
int release(struct inode *inode, struct file *filp)
{
    return 0;
}

/*读函数*/
ssize_t read(struct file *filp, char __user *buf, size_t size, loff_t *p) //p为文件指针偏移位置，size为读取的字节数
{
    unsigned int num = size;    /*记录需要读取的字节数*/
    int re = 0;
     my_cdev * cdevp = filp->private_data; /*获得设备结构体指针*/
    /*判断读位置是否越界*/
    if (*p >= SIZE)    /*要读取的偏移大于设备的数据缓冲区*/
        return 0;
    if (size > SIZE - *p)     /*要读取的字节数大于设备的数据缓冲区*/
        num = SIZE - *p;
    /*内核空间->用户空间*/
    if (copy_to_user(buf, (void*)(cdevp->bufdata + *p), num))//拷贝成功返回0
    {
        re =  - EFAULT;
    }
    else
    {
        *p += num;/*修改文件指针偏移位置*/
        re = num;    /*写入字节数*/
    }
    return re;
}

/*写函数*/
ssize_t write(struct file *filp, const char __user *buf, size_t size, loff_t *p)
{
    unsigned int num = size;
    int re = 0;
    my_cdev * cdevp = filp->private_data; /*获得设备结构体指针*/
    if (*p >= SIZE) /*要写的指针偏移大于设备的数据缓冲区大小*/
        return 0;
    if (num > SIZE - *p)    /*要写入的字节大于设备的内存空间*/
        num = SIZE - *p;
    if (copy_from_user((void*)cdevp->bufdata + *p, buf, num))
        re =  - EFAULT;
    else
    {
        *p += num;      /*修改文件指针偏移位置*/
        re = num;      /*写入字节数*/
    }
    return re;
}

/*定位函数 */
loff_t llseek(struct file *filp, loff_t p, int orig)//参数 p 为文件定位的目标偏移量；参数orig为对文件定位
//的起始地址
{
    loff_t pos;
    switch(orig)
    {
    case 0: /* SEEK_SET */     /*设置为p*/
        pos = p;           
        break;
    case 1: /* SEEK_CUR */  //设置为当前位置加上p
        pos = filp->f_pos + p;
        break;
    case 2: /* SEEK_END */ //设置为文件大小加上p
        pos = SIZE-1+ p;
        break;
    default: 
        return -EINVAL;
    }
    if (pos<0|| pos>SIZE)
        return -EINVAL;
    filp->f_pos = pos;
    return pos;
}

/*文件操作结构体*/
struct file_operations fops =
{
    .owner = THIS_MODULE,
    .llseek = llseek,
    //.ioctl=ioctl,
    .read = read,
    .write = write,
    .open = my_copen,
    .release = release,
};

/*设备驱动模块加载函数*/
int my_cdev_init(void)
{
    int re;
//动态分配设备号
    re = alloc_chrdev_region(&devt, 0, 1, "my_cdev");//次设备号为0
    if (re < 0)
        return re;
    /* 为设备描述结构分配内存*/
    my_cdevp =(my_cdev*)kmalloc(sizeof(my_cdev), GFP_KERNEL);
   if (!my_cdevp)    /*申请失败*/
   {
        re=  - ENOMEM;
        return re;
   }
    my_cdevp->bufdata = kmalloc(SIZE, GFP_KERNEL);
    if (!my_cdevp->bufdata)    /*申请失败*/
   {
        re=  - ENOMEM;
        return re;
   }
memset(my_cdevp->bufdata,0,SIZE); /*初始化*/  
/*初始化设备描述结构体成员cdev结构*/
cdev_init(&my_cdevp->cdev, &fops);
my_cdevp->cdev.owner = THIS_MODULE;    /*指定所属模块*/
my_cdevp->cdev.ops = &fops;
/* 注册字符设备 */
cdev_add(&my_cdevp->cdev, devt,1);
printk("my_cdev init sussess\n");
return re;
}

 /*模块卸载函数*/
 void my_cdev_exit(void)
{
cdev_del(&my_cdevp->cdev);   /*注销设备*/
unregister_chrdev_region(devt, 1); /*释放设备号*/
kfree(my_cdevp->bufdata);     /*释放设备结构体内存*/
kfree(my_cdevp);     /*释放设备结构体内存*/
printk("my_cdev exit sussess \n");
}
MODULE_AUTHOR("ML");
MODULE_LICENSE("GPL");
module_init(my_cdev_init);
module_exit(my_cdev_exit);
