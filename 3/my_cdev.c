#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#define SIZE 1024  //��������С
/*�豸�����ṹ��*/
typedef struct my_cdev
{
    struct cdev cdev;//cdev�ṹ��
    char *bufdata;    //���ݻ�����
} my_cdev;

/*�豸�����ṹ��ָ��*/
my_cdev *my_cdevp;
//int MAJOR;//���豸��
dev_t devt;//�豸��
/*�򿪺���*/
int my_copen(struct inode *inode, struct file *filp)
{
    /*���豸�����ṹָ�븳ֵ���ļ�˽������ָ��*/
    filp->private_data = my_cdevp;
    return 0;
}

/*�ͷź���*/
int release(struct inode *inode, struct file *filp)
{
    return 0;
}

/*������*/
ssize_t read(struct file *filp, char __user *buf, size_t size, loff_t *p) //pΪ�ļ�ָ��ƫ��λ�ã�sizeΪ��ȡ���ֽ���
{
    unsigned int num = size;    /*��¼��Ҫ��ȡ���ֽ���*/
    int re = 0;
     my_cdev * cdevp = filp->private_data; /*����豸�ṹ��ָ��*/
    /*�ж϶�λ���Ƿ�Խ��*/
    if (*p >= SIZE)    /*Ҫ��ȡ��ƫ�ƴ����豸�����ݻ�����*/
        return 0;
    if (size > SIZE - *p)     /*Ҫ��ȡ���ֽ��������豸�����ݻ�����*/
        num = SIZE - *p;
    /*�ں˿ռ�->�û��ռ�*/
    if (copy_to_user(buf, (void*)(cdevp->bufdata + *p), num))//�����ɹ�����0
    {
        re =  - EFAULT;
    }
    else
    {
        *p += num;/*�޸��ļ�ָ��ƫ��λ��*/
        re = num;    /*д���ֽ���*/
    }
    return re;
}

/*д����*/
ssize_t write(struct file *filp, const char __user *buf, size_t size, loff_t *p)
{
    unsigned int num = size;
    int re = 0;
    my_cdev * cdevp = filp->private_data; /*����豸�ṹ��ָ��*/
    if (*p >= SIZE) /*Ҫд��ָ��ƫ�ƴ����豸�����ݻ�������С*/
        return 0;
    if (num > SIZE - *p)    /*Ҫд����ֽڴ����豸���ڴ�ռ�*/
        num = SIZE - *p;
    if (copy_from_user((void*)cdevp->bufdata + *p, buf, num))
        re =  - EFAULT;
    else
    {
        *p += num;      /*�޸��ļ�ָ��ƫ��λ��*/
        re = num;      /*д���ֽ���*/
    }
    return re;
}

/*��λ���� */
loff_t llseek(struct file *filp, loff_t p, int orig)//���� p Ϊ�ļ���λ��Ŀ��ƫ����������origΪ���ļ���λ
//����ʼ��ַ
{
    loff_t pos;
    switch(orig)
    {
    case 0: /* SEEK_SET */     /*����Ϊp*/
        pos = p;           
        break;
    case 1: /* SEEK_CUR */  //����Ϊ��ǰλ�ü���p
        pos = filp->f_pos + p;
        break;
    case 2: /* SEEK_END */ //����Ϊ�ļ���С����p
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

/*�ļ������ṹ��*/
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

/*�豸����ģ����غ���*/
int my_cdev_init(void)
{
    int re;
//��̬�����豸��
    re = alloc_chrdev_region(&devt, 0, 1, "my_cdev");//���豸��Ϊ0
    if (re < 0)
        return re;
    /* Ϊ�豸�����ṹ�����ڴ�*/
    my_cdevp =(my_cdev*)kmalloc(sizeof(my_cdev), GFP_KERNEL);
   if (!my_cdevp)    /*����ʧ��*/
   {
        re=  - ENOMEM;
        return re;
   }
    my_cdevp->bufdata = kmalloc(SIZE, GFP_KERNEL);
    if (!my_cdevp->bufdata)    /*����ʧ��*/
   {
        re=  - ENOMEM;
        return re;
   }
memset(my_cdevp->bufdata,0,SIZE); /*��ʼ��*/  
/*��ʼ���豸�����ṹ���Աcdev�ṹ*/
cdev_init(&my_cdevp->cdev, &fops);
my_cdevp->cdev.owner = THIS_MODULE;    /*ָ������ģ��*/
my_cdevp->cdev.ops = &fops;
/* ע���ַ��豸 */
cdev_add(&my_cdevp->cdev, devt,1);
printk("my_cdev init sussess\n");
return re;
}

 /*ģ��ж�غ���*/
 void my_cdev_exit(void)
{
cdev_del(&my_cdevp->cdev);   /*ע���豸*/
unregister_chrdev_region(devt, 1); /*�ͷ��豸��*/
kfree(my_cdevp->bufdata);     /*�ͷ��豸�ṹ���ڴ�*/
kfree(my_cdevp);     /*�ͷ��豸�ṹ���ڴ�*/
printk("my_cdev exit sussess \n");
}
MODULE_AUTHOR("ML");
MODULE_LICENSE("GPL");
module_init(my_cdev_init);
module_exit(my_cdev_exit);
