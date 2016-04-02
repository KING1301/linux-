#include "define.h"
//全局变量定义
//ROOT的内存i节点
struct m_inode *root;
struct m_inode  mem_inode[INODESNUM];
//当前节点
struct m_inode *current;
struct m_inode *inode;
//超级块
struct supblock *super;
//模拟磁盘
FILE * virtualDisk;
//当前文件或者目录名字（通过文件，获取名字和文件的inode id）
struct direct curdirect;
//当前目录
//struct dir* curdir;
//用户文件节点
//struct inode* userinode;
//int userpos;

int write_finode(struct m_inode* inode) //将内存I节点同步写入磁盘I节点
{
    int ret;
    int ipos;
    ipos=START_POS+SUPERSIZE+inode->inodeID*INODE;
    fseek(virtualDisk,ipos,SEEK_SET);
    ret=fwrite(&inode->finode,sizeof(struct f_inode),1,virtualDisk);
    if(ret!=1)
        return -1;
    return 0;
}

int write_super()//写入超级块
{
    int ret;
    if(virtualDisk==NULL)
        return -1;
    fseek(virtualDisk,START_POS,SEEK_SET);
    ret=fwrite(super,sizeof(struct supblock),1,virtualDisk);
    if(ret!=1)
        return -1;
    return 0;
}


int format_disk(char * path) //初始化虚拟磁盘
{
    unsigned int group[BLOCKNUM];
   // virtualDisk=fopen(path,"w+");//打开虚拟磁盘文件
    virtualDisk=fopen(path,"wb+");//打开虚拟磁盘文件
    if(virtualDisk==NULL)
    {
        return -1;
    }
    super=(struct supblock*)calloc(1,sizeof(struct supblock)); //创建超级块
    if(super==NULL)
        return -1;
    super->flags=1;
    super->disksize=8*BLOCKSIZE*1024;
    super->Blocksize=BLOCKSIZE;//初始化空闲块号栈
    super->freeBlockNum=BLOCKSNUM;
    super->nextFreeBlock=BLOCKNUM;
    for(int i=0; i<BLOCKNUM; i++)
        super->freeBlock[i]=BLOCKSTART+i;

    super->freeInodeNum=INODESNUM-1;//初始化I节点号栈
    super->nextFreeInode=INODENUM;
    for(int i=0; i<INODENUM; i++)
    {
        super->freeInode[i]=i+1;
    }

    write_super(); //更新超级块

    fseek(virtualDisk,BLOCKSTART*BLOCKSIZE,SEEK_SET);//重定向到数据块地址

    for(int i=0; i<BLOCKNUM; i++)
    {
        group[i]=i+BLOCKSTART;
    }
    //for(int i=0; i<363; i++) //初始化空闲块索引，构成索引表管理空闲块
    for(int i=0; i<(BLOCKSNUM-BLOCKNUM)/BLOCKNUM; i++)
    {
        for(int j=0; j<BLOCKNUM; j++)
        {
            group[j]+=BLOCKNUM;
        }
     //   printf("%d\n",BLOCKSTART+i*BLOCKNUM);
        fseek(virtualDisk,(BLOCKSTART+i*BLOCKNUM)*1024,SEEK_SET);
        fwrite(group,sizeof(unsigned int),BLOCKNUM,virtualDisk);
    }

    struct m_inode * iroot= iget(0);//root I节点
    iroot->finode.addr[0]=balloc();//分配root目录磁盘块
    iroot->finode.mode=1;
    iroot->finode.addrnum=1;
    write_finode(iroot);
    fclose( virtualDisk);
    return 0;
}

int loadSuper(char *path)
{
    int ret=0;
   // virtualDisk=fopen(path,"r+");
  virtualDisk=fopen(path,"rb+");
    if(virtualDisk==NULL)
    {
        return -1;
    }
    super=(struct supblock*)calloc(1,sizeof(struct supblock));
    fseek(virtualDisk,START_POS,SEEK_SET);                              //重定向到超级块首址
    //int readSize=fread(super,1,sizeof(struct supblock),virtualDisk);//加载超级块
   ret=fread(super,sizeof(struct supblock),1,virtualDisk);//加载超级块
    if(ret!=1||super->flags==0)
        return -1;
    return 0;
}

struct m_inode * iget(int ino)//分配主存I节点
{
    int ipos    =   0;
    int ret =   0;
    if(mem_inode[ino].userCount!=0)//该主存I节点已存在，则打开数加1
    {
        return &mem_inode[ino];
    }
    if(virtualDisk==NULL)
        return NULL;
    ipos=START_POS+SUPERSIZE+ino*INODE;
    fseek(virtualDisk,ipos,SEEK_SET);
    ret=fread(&mem_inode[ino],sizeof(struct f_inode),1,virtualDisk);//主存I节点不存在则加载磁盘I节点
    if(ret!=1)
        return NULL;
    if(mem_inode[ino].finode.i_free==0)     //如果该磁盘I节点为空闲磁盘I节点
    {
        mem_inode[ino].finode.i_free++;//初始化索引节点
        mem_inode[ino].finode.fileSize=0;
        mem_inode[ino].finode.addrnum=1;
        mem_inode[ino].inodeID=ino;
        write_finode(&mem_inode[ino]);
    }
    mem_inode[ino].userCount++;//该节点为已有文件的I节点
    mem_inode[ino].inodeID=ino;
    return &mem_inode[ino];
}

void iput(struct m_inode *inode)//释放主存I节点
{
    if(inode->userCount>0)
        inode->userCount--;
    if(inode->userCount==0)
        memset(inode,0,sizeof(inode));
}

struct m_inode* ialloc() //分配空闲磁盘I节点并返回加载到主存的内存I节点
{
    int num=0;
    struct f_inode *tmp=NULL;
    if(super->freeInodeNum<=0)//无空闲I节点
        return NULL;
    if(super->nextFreeInode==0)//直接管理的I节点分配完毕
    {
        tmp=(struct f_inode*)calloc(1,sizeof(struct f_inode));
        fseek(virtualDisk,START_POS+SUPERSIZE,SEEK_SET);
        for(int i=1; i<INODESNUM; i++)
        {
            fseek(virtualDisk,START_POS+SUPERSIZE+i*INODE,SEEK_SET);
            fread(tmp,sizeof(struct f_inode),1,virtualDisk);
            if(tmp->i_free==0)//查找空闲磁盘I节点
            {
                super->freeInode[num]=i;
                super->nextFreeInode++;
                num++;
            }
            if(num==INODENUM)
                break;
        }
    }
    super->nextFreeInode--;
    super->freeInodeNum--;
    write_super();//更新超级块信息
    return iget(super->freeInode[super->nextFreeInode]);//返回相应磁盘I节点加载到的主存I节点
}

int balloc()//分配空闲磁盘块，采用成组连接法管理
{
    unsigned int bno;
    int ret;
    if(super->freeBlockNum<=0)//无空闲磁盘块
        return -1;
    if(super->nextFreeBlock==1)//直接管理的空闲块使用完则读取空闲块索引表中的空闲块信息到空闲块号栈
    {
        bno=super->freeBlock[--super->nextFreeBlock];
        fseek(virtualDisk,super->freeBlock[0]*BLOCKSIZE,SEEK_SET);//重定向到索引表
        ret=fread(super->freeBlock,sizeof(unsigned int),BLOCKNUM,virtualDisk);//读取空闲块索引表到空闲块号栈
        super->nextFreeBlock=ret;
        super->freeBlockNum--;
        write_super();
        return bno;
    }
    super->freeBlockNum--;//直接管理的空闲块未分配完则分配空闲磁盘块
    super->nextFreeBlock--;
    write_super();
    return super->freeBlock[super->nextFreeBlock];
}

int bwrite(void * _Buf,unsigned short int bno,long int offset,int size,int count) //写入磁盘块
{
    long int pos;
    int ret;
    pos=bno*BLOCKSIZE+offset;
    fseek(virtualDisk,pos,SEEK_SET);
    ret=fwrite(_Buf,size,count,virtualDisk);
    if(ret!=count)
        return -1;
    return 0;
}
int bread(void * _Buf,unsigned short int bno,int offset,int size,int count)//读取磁盘块
{
    long int pos;
    int ret;
    pos=bno*BLOCKSIZE+offset;
    fseek(virtualDisk,pos,SEEK_SET);
    ret=fread(_Buf,size,count,virtualDisk);
    if(ret!=count)
        return -1;
    return 0;
}
int bfree(int bno) //释放磁盘块
{
    if(super->nextFreeBlock==BLOCKNUM)
    {
        bwrite(&super->freeBlock,bno,0,sizeof(unsigned int),BLOCKNUM);
        super->nextFreeBlock=1;
        super->freeBlock[0]=bno;
    }
    else
    {
        super->freeBlock[super->nextFreeBlock]=bno;
        super->nextFreeBlock++;
    }
    super->freeBlockNum++;
    write_super();
    return 1;
}

/*int ifree(struct m_inode* inode) //释放i节点
{

    inode->finode.i_free=0;
    super->freeInodeNum++;
    write_super();
    write_finode(inode);
    if(super->nextFreeInode==INODENUM)
    {

    }
    else
    {
        super->freeBlock[super->nextFreeBlock]=bno;
        super->nextFreeBlock++;
    }
    super->freeBlockNum++;
    write_super();
    return 1;
}
*/


struct m_inode* search_dir(char* path,struct m_inode* inode,int cd)//查找指定目录下的路径对应目录的I节点
{
    char *ps=NULL;
    char tmp[128]= {0};
    struct m_inode * tmpnode=NULL;
    ps=strchr(path,'/');
    if(ps==NULL)//路径尾
        strcpy(tmp,path);
    else
        sscanf(path,"%[^/]",tmp);
    if(inode->finode.mode==1)  //当前为目录
    {
        dir * dir=(struct dir*)calloc(1,sizeof(struct dir));
        for(int addr=0; addr<inode->finode.addrnum; addr++)//遍历所以目录数据块
        {
            bread(dir,inode->finode.addr[addr],0,sizeof(struct dir),1);
            for(int i=0; i<dir->dirNum; i++)
            {
                if(strcmp(dir->direct[i].directName,tmp)==0)
                {
                    if(cd==1)
                        curdirect=dir->direct[i];
                    tmpnode=iget(dir->direct[i].inodeID);
                    tmpnode->parent=inode;
                    break;
                }
            }
            if(tmpnode)
                break;
        }
    }
    if(ps!=NULL)//非路径尾
    {
        if (tmpnode!=NULL)//查找到相应节点
            return search_dir(ps+1,tmpnode,cd);//递归查找
        else
            return NULL;//未查找到则结束//
    }
    else//路径尾
        return tmpnode;
}

int ll()
{
    char text[128];
    if(current->finode.mode!=1)
    {
        cout<<"目录不存在"<<endl;
        return 0;
    }
    if(current->finode.fileSize==0)
    {
        cout<<"目录为空"<<endl;
        return -1;
    }

    dir * dir=(struct dir*)calloc(1,sizeof(struct dir));
    for(int addr=0; addr<current->finode.addrnum; addr++)
    {
        bread(dir,current->finode.addr[addr],0,sizeof(struct dir),1);
        for(int i=0; i<dir->dirNum; i++)
        {
            struct m_inode * inode=iget(dir->direct[i].inodeID);
            if(inode->finode.mode==1)
                sprintf(text,"d\t size:%d inodeID:%d %s",inode->finode.fileSize,inode->inodeID,dir->direct[i].directName);
            else
                sprintf(text,"-\tsize:%d inodeID:%d %s",inode->finode.fileSize,inode->inodeID,dir->direct[i].directName);
            cout<<text<<endl;

        }
    }
    return 1;

}


int mkdir(char * dirname,struct m_inode* inode)// inode为待添加字目录的目录的主存索引节点
{
    if(inode==NULL)
        return -1;
    if(inode->finode.mode!=1)
    {
        cout<<"非目录文件"<<endl;
        return -1;
    }
    int count=inode->finode.fileSize/sizeof(struct direct);
    if(count>ADDRN*DIRNUM)
    {
        cout<<"目录项已达到最大!"<<endl;
        return -1;
    }
    dir * dir=(struct dir*)calloc(1,sizeof(struct dir));
    for(int addr=0; addr<inode->finode.addrnum; addr++)
    {
        bread(dir,inode->finode.addr[addr],0,sizeof(struct dir),1);
        for(int i=0; i<dir->dirNum; i++)
            if(strcmp(dir->direct[i].directName,dirname)==0)
            {
                cout<<"目录项已存在!"<<endl;
                return -1;
            }
    }
    bread(dir,inode->finode.addr[inode->finode.addrnum-1],0,sizeof(struct dir),1);//读出最后一个数据块的目录
    struct m_inode * tmpinode=ialloc();//分配子目录I节点
    tmpinode->finode.addr[0]=balloc();//分配子目录磁盘块
    tmpinode->finode.mode=1;
    write_finode(tmpinode);

    if(dir->dirNum==DIRNUM)//当前数据块可容纳目录已满,再申请数据块
    {
        inode->finode.addr[inode->finode.addrnum]=balloc();

        memset(dir,0,sizeof(struct dir));
        strcpy(dir->direct[0].directName,dirname);//修改目录
        dir->direct[0].inodeID=tmpinode->inodeID;
        dir->dirNum=1;
        bwrite(dir,inode->finode.addr[inode->finode.addrnum],0,sizeof(struct dir),1);
        inode->finode.addrnum++;
        inode->finode.fileSize+=sizeof(struct direct);//更新inodeI节点
        write_finode(inode);
        return 1;
    }
    else
    {

        strcpy(dir->direct[dir->dirNum].directName,dirname);//修改目录目录项
        dir->direct[dir->dirNum].inodeID=tmpinode->inodeID;
        dir->dirNum+=1;
        bwrite(dir,inode->finode.addr[inode->finode.addrnum-1],0,sizeof(struct dir),1);

        inode->finode.fileSize+=sizeof(struct direct);//更新I节点
        write_finode(inode);
        return 1;
    }

}


int mkfile(char * filename,struct m_inode* inode)// inode为待添加文件的目录的主存索引节点
{
    if(inode==NULL)
        return -1;
    if(inode->finode.mode!=1)
    {
        cout<<"非目录文件"<<endl;
        return -1;
    }
    int count=inode->finode.fileSize/sizeof(struct direct);
    if(count>ADDRN*DIRNUM)
    {
        cout<<"目录项已最大"<<endl;
        return -1;
    }
    dir * dir=(struct dir*)calloc(1,sizeof(struct dir));

    for(int addr=0; addr<inode->finode.addrnum; addr++)
    {
        bread(dir,inode->finode.addr[addr],0,sizeof(struct dir),1);
        for(int i=0; i<dir->dirNum; i++)
            if(strcmp(dir->direct[i].directName,filename)==0)
            {
                cout.write(filename,strlen(filename));
                cout<<"目录项已存在"<<endl;
                return -1;
            }
    }
    bread(dir,inode->finode.addr[inode->finode.addrnum-1],0,sizeof(struct dir),1);//读出最后一个数据块的目录
    struct m_inode * tmpinode=ialloc();//分配子目录I节点
    tmpinode->finode.addr[0]=balloc();//分配子目录磁盘块
    tmpinode->finode.mode=0;
    write_finode(tmpinode);

    if(dir->dirNum==DIRNUM)//当前数据块可容纳目录已满,再申请数据块
    {
        inode->finode.addr[inode->finode.addrnum]=balloc();

        memset(dir,0,sizeof(struct dir));
        strcpy(dir->direct[0].directName,filename);//修改目录
        dir->direct[0].inodeID=tmpinode->inodeID;
        dir->dirNum=1;
        bwrite(dir,inode->finode.addr[inode->finode.addrnum],0,sizeof(struct dir),1);
        inode->finode.addrnum++;
        inode->finode.fileSize+=sizeof(struct direct);//更新inodeI节点
        write_finode(inode);
        return 1;
    }
    else
    {

        strcpy(dir->direct[dir->dirNum].directName,filename);//修改目录目录项
        dir->direct[dir->dirNum].inodeID=tmpinode->inodeID;
        dir->dirNum+=1;
        bwrite(dir,inode->finode.addr[inode->finode.addrnum-1],0,sizeof(struct dir),1);

        inode->finode.fileSize+=sizeof(struct direct);//更新I节点
        write_finode(inode);
        return 1;
    }

}
int writefile(char * filename,struct m_inode* inode,char *buf,int bufcount)
{
    struct m_inode * tmpnode=NULL;
    dir * dir=(struct dir*)calloc(1,sizeof(struct dir));
    for(int addr=0; addr<inode->finode.addrnum; addr++)
    {
        bread(dir,inode->finode.addr[addr],0,sizeof(struct dir),1);
        for(int i=0; i<dir->dirNum; i++)
        {
            if(strcmp(dir->direct[i].directName,filename)==0)
            {
                tmpnode=iget(dir->direct[i].inodeID);
                tmpnode->parent=inode;
                break;
            }
        }
        if( tmpnode)
            break;
    }
    if(tmpnode==NULL)
    {
        cout<<"未发现此文件\n ";
        cout.write(filename,strlen(filename));
        cout<<endl;
        return -1;
    }
    int lastCount=tmpnode->finode.fileSize%BLOCKSIZE;
    int writeCount=(bufcount+lastCount>BLOCKSIZE?BLOCKSIZE-lastCount:bufcount);
    bwrite(buf,tmpnode->finode.addr[tmpnode->finode.addrnum-1],lastCount,sizeof(char),writeCount);
    tmpnode->finode.fileSize+=writeCount;
    write_finode(tmpnode);
    return 1;
}
int readfile(char * filename,struct m_inode* inode,char *buf)
{
    struct m_inode * tmpnode=NULL;
    dir * dir=(struct dir*)calloc(1,sizeof(struct dir));
    for(int addr=0; addr<inode->finode.addrnum; addr++)
    {
        bread(dir,inode->finode.addr[addr],0,sizeof(struct dir),1);
        for(int i=0; i<dir->dirNum; i++)
        {
            if(strcmp(dir->direct[i].directName,filename)==0)
            {
                tmpnode=iget(dir->direct[i].inodeID);
                tmpnode->parent=inode;
                break;
            }
        }
        if(tmpnode)
            break;
    }
    if(tmpnode==NULL)
    {
        cout<<"未发现文件\n ";
        cout.write(filename,strlen(filename));
        cout<<endl;
        return -1;
    }
    if(tmpnode->finode.fileSize==0)
        return -1;
    int lastCount=tmpnode->finode.fileSize%BLOCKSIZE;
    int i;
    char content[1024]= {0};
    for(i=0; i<tmpnode->finode.addrnum-1; i++)
    {
        bread(&content,tmpnode->finode.addr[i],0,sizeof(char),BLOCKSIZE);
        cout<<content;
    }
    memset(content,0,sizeof(content));
    bread(&content,tmpnode->finode.addr[tmpnode->finode.addrnum-1],0,sizeof(char),lastCount);
    cout.write(content,lastCount);
    cout<<endl;
    return 1;
}

int _rmdir(struct m_inode* inode)
{

    struct m_inode * rminode=NULL;
    dir * dir=(struct dir*)calloc(1,sizeof(struct dir));
    for(int addr=0; addr<inode->finode.addrnum; addr++)
    {
        bread(dir,inode->finode.addr[addr],0,sizeof(struct dir),1);
        for(int i=0; i<dir->dirNum; i++)
        {
            rminode=iget(dir->direct[i].inodeID);
            if(rminode->finode.mode==1)
            {
                _rmdir(rminode);
            }
            else
            {
                for(int i=0; i<rminode->finode.addrnum; i++)
                    bfree(rminode->finode.addr[i]);
                rminode->finode.i_free=0;//标志I节点空闲
                super->freeInodeNum++;
                write_super();
                write_finode(rminode);
            }

        }
    }
    //释放目录本省
    for(int i=0; i< inode->finode.addrnum; i++)
        bfree( inode->finode.addr[i]);
    inode->finode.i_free=0;
    super->freeInodeNum++;
    write_super();
    write_finode( inode);

    return 1;
}
int rmdir(char* filename,struct m_inode* inode)
{
    struct m_inode* rminode=NULL;
    dir * dir=(struct dir*)calloc(1,sizeof(struct dir));
    for(int addr=0; addr<inode->finode.addrnum; addr++)
    {
        bread(dir,inode->finode.addr[addr],0,sizeof(struct dir),1);
        for(int i=0; i<dir->dirNum; i++)
        {
            if(strcmp(dir->direct[i].directName,filename)==0)
            {
                rminode=iget(dir->direct[i].inodeID);
                for(int j=i; j<dir->dirNum; j++)
                    dir->direct[j]=dir->direct[j+1];
                dir->dirNum--;
                bwrite(dir,inode->finode.addr[addr],0,sizeof(struct dir),1);
                break;
            }
        }
        if(rminode)
            break;
    }
    if(rminode==NULL)
        return -1;
    else
    {
        if(rminode->finode.mode==1)//删除项为目录
            _rmdir(rminode);//递归删除
        else
        {
            for(int i=0; i<rminode->finode.addrnum; i++)
                bfree(rminode->finode.addr[i]);
            rminode->finode.i_free=0;
            super->freeInodeNum++;
            write_super();
            write_finode(rminode);
        }

    }
    inode->finode.fileSize-=sizeof(struct direct);
    write_finode(inode);
    return 1;
}
