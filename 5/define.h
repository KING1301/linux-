#ifndef   DEFINE_H
#define   DEFINE_H
#include<iostream>
#include<fstream>
#include <string.h>
#include<stdio.h>
#include<stdlib.h>
using namespace std;

#define INODENUM        80
#define BLOCKNUM        80
#define DIRECTNAME  14
#define DIRNUM      63
#define START_POS       1024
#define SUPERSIZE       1024
#define INODE           128
#define BLOCKSTART  912
#define BLOCKSNUM       7280
#define INODESNUM       7280
#define BLOCKSIZE       1024
#define ADDRN       10//索引表项个数

struct supblock
{
    unsigned int flags;                 //虚拟磁盘挂载表示，为1挂载
    unsigned int Blocksize;             //磁盘块大小
    unsigned int disksize;              //磁盘大小
    unsigned int freeBlock[BLOCKNUM];   //空闲块栈
    unsigned int    nextFreeBlock;      //空闲块栈指针
    unsigned int freeBlockNum;          //空闲块数目
    unsigned int freeInode[INODENUM];   //空闲I节点栈
    unsigned int nextFreeInode;     //空闲I节点栈指针
    unsigned int freeInodeNum;          //空闲I节点数目

};

//磁盘I节点
struct f_inode
{
    int         mode;//1为目录 2为文件
    long        int fileSize;
    int         i_free;//为0时表示该I节点空闲
    int         addr[ADDRN];
    int addrnum;//索引表使用数目
};

//内存I节点
struct m_inode
{
    struct                  f_inode finode;
    struct                  m_inode *parent;
    unsigned short int      inodeID;                //I节点号
    int                     userCount;          //内存I节点打开数
};
//目录项结构体
struct direct
{
    char                    directName[DIRECTNAME];
    unsigned short int  inodeID;
};
//目录结构体
struct dir
{
    int     dirNum;
    struct  direct direct[DIRNUM];
};


//全局变量声明
extern struct m_inode *root;
extern struct m_inode   mem_inode[INODESNUM];
//当前节点
extern  struct m_inode *current;
extern struct m_inode *inode;
//超级块
extern struct supblock *super;
//模拟磁盘
extern FILE * virtualDisk;
//当前文件或者目录名字（通过文件，获取名字和文件的inode id）
extern struct direct curdirect;

//函数声明
int write_finode(struct m_inode* inode); //写入磁盘I节点
int write_super();//写入超级块
int format_disk(char * path); //初始化虚拟磁盘
int loadSuper(char *path);
struct m_inode * iget(int ino);//分配主存I节点
void iput(struct m_inode *inode);//释放主存I节点
struct m_inode* ialloc(); //分配空闲磁盘I节点并返回加载到主存的内存I节点
int balloc();//采用成组连接法管理
int bwrite(void * _Buf,unsigned short int bno,long int offset,int size,int count); //写入磁盘块
int bread(void * _Buf,unsigned short int bno,int offset,int size,int count);//读取磁盘块
int bfree(int bno); //释放磁盘块
struct m_inode* search_dir(char* path,struct m_inode* inode,int cd);//查找指定目录下的路径对应目录的I节点
int mkdir(char * dirname,struct m_inode* inode);// inode为待添加字目录的目录的主存索引节点
int ll();
int mkfile(char * filename,struct m_inode* inode);// inode为待添加文件的目录的主存索引节点
int writefile(char * filename,struct m_inode* inode,char *buf,int bufcount);
int readfile(char * filename,struct m_inode* inode,char *buf);
int _rmdir(struct m_inode* inode);
int rmdir(char* filename,struct m_inode* inode);
#endif
