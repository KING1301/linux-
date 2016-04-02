#include"define.h"


int main()
{
    char buffdata[256];
    char buf[256];
    char cmd[128];
    char path[128];
    char cat[128];
    char *p=NULL;
    if(loadSuper("vmfile.dat")==-1)
    {
        cout<<"第一次使用，初始化磁盘"<<endl;
        format_disk("vmfile.dat");
    }
    loadSuper("vmfile.dat");
    root=iget(0);
    current=root;
    while(1)
    {
        memset(buf,0,sizeof(buf));
        memset(cmd,0,sizeof(cmd));
        memset(path,0,sizeof(path));
        memset(cat,0,sizeof(cat));
        memset(buffdata,0,sizeof(buffdata));
        cout<<"localhost : "<<(current->inodeID==0?"/":curdirect.directName)<<"$  ";
        fgets(buf,sizeof(buf),stdin);
        sscanf(buf,"%[^ |\n] %[^%\n]",cmd,cat);
        if(strcmp(cmd,"ll")==0)
        {
            ll();
        }
        else if(strcmp(cmd,"mkfile")==0)
        {
            if(cat[0]=='/'&&strlen(cat)==1)
                cout<<"当前目录为root目录"<<endl;
            else
            {
                if(strchr(cat,'/')==cat)//绝对路径
                {
                    p=strrchr(cat+sizeof(char),'/');
                    if(p==NULL)
                        mkfile(cat+sizeof(char),root);
                    else
                    {
                        strncpy(path,cat+sizeof(char),p-cat-sizeof(char));
                        inode=search_dir(path,root,0);
                        mkfile(p+sizeof(char),inode);
                    }
                }
                else//相对路径
                {
                    p=strrchr(cat,'/');
                    if(p==NULL)
                        mkfile(cat,current);
                    else
                    {
                        strncpy(path,cat,p-cat);
                        inode=search_dir(path,current,0);
                        mkfile(p+1,inode);
                    }

                }
            }

            cout<<"文件常建成功"<<endl;
        }
        else if(strcmp(cmd,"mkdir")==0)
        {
            if(cat[0]=='/'&&strlen(cat)==1)
                cout<<"当前目录为root目录"<<endl;
            else
            {
                if(strchr(cat,'/')==cat)//绝对路径
                {
                    p=strrchr(cat+sizeof(char),'/');
                    //cout<<cat+1<<p<<endl;
                    if(p==NULL)
                        mkdir(cat+sizeof(char),root);
                    else
                    {
                        strncpy(path,cat+sizeof(char),p-cat-sizeof(char));
                        inode=search_dir(path,root,0);
                        mkdir(p+sizeof(char),inode);
                    }
                }
                else//相对路径
                {
                    p=strrchr(cat,'/');
                    if(p==NULL)
                        mkdir(cat,current);
                    else
                    {
                        strncpy(path,cat,p-cat);
                        inode=search_dir(path,current,0);
                        mkdir(p+1,inode);
                    }

                }
            }

            cout<<"目录常建成功"<<endl;
        }
        else if(strcmp(cmd,"cd")==0)
        {

            if(cat[0]=='/'&&strlen(cat)==1)
                current=root;

            else
            {
                if(strchr(cat,'/')==cat)//绝对路径
                {
                    current=search_dir(cat+1,root,1);
                    // mkdir(p+sizeof(char),inode);
                }
                else//相对路径
                {
                    current=search_dir(cat,current,1);
                }
            }
        }
        else if(strcmp(cmd,"writefile")==0)
        {
            if(cat[0]=='/'&&strlen(cat)==1)
                cout<<"当前目录为root目录"<<endl;
            else
            {
                cout<<"输入带写入字符串"<<endl;
                fgets(buffdata,sizeof(buffdata),stdin);
                buffdata[strlen(buffdata)-1]='\0';
                if(strchr(cat,'/')==cat)//绝对路径
                {
                    p=strrchr(cat+sizeof(char),'/');
                    // cout<<cat+1<<p<<endl;
                    if(p==NULL)
                        //mkfile(cat+sizeof(char),root);
                        writefile(cat+1,root,buffdata,strlen(buffdata));
                    else
                    {
                        strncpy(path,cat+sizeof(char),p-cat-sizeof(char));
                        inode=search_dir(path,root,0);
                        //mkfile(p+sizeof(char),inode);
                        writefile(p+1,inode,buffdata,strlen(buffdata));
                    }
                }
                else//相对路径
                {
                    p=strrchr(cat,'/');
                    if(p==NULL)
                        //mkfile(cat,current);
                        writefile(cat,current,buffdata,strlen(buffdata));
                    else
                    {
                        strncpy(path,cat,p-cat);
                        inode=search_dir(path,current,0);
                        writefile(p+1,inode,buffdata,strlen(buffdata));
                    }
                }
            }
            cout<<"文件写入成功"<<endl;
        }
        else if(strcmp(cmd,"readfile")==0)
        {
            if(cat[0]=='/'&&strlen(cat)==1)
                cout<<"当前目录为root目录"<<endl;
            else
            {
                if(strchr(cat,'/')==cat)//绝对路径
                {
                    p=strrchr(cat+sizeof(char),'/');
                    //  cout<<cat+1<<p<<endl;
                    if(p==NULL)
                        //mkfile(cat+sizeof(char),root);
                        // readfile()
                        readfile(cat+1,root,buffdata);
                    else
                    {
                        strncpy(path,cat+sizeof(char),p-cat-sizeof(char));
                        inode=search_dir(path,root,0);
                        readfile(p+1,inode,buffdata);
                    }
                }
                else//相对路径
                {
                    p=strrchr(cat,'/');
                    if(p==NULL)
                        readfile(cat,current,buffdata);
                    else
                    {
                        strncpy(path,cat,p-cat);
                        inode=search_dir(path,current,0);
                        readfile(p+1,inode,buffdata);
                    }
                }
            }
            cout<<"文件读出成功"<<endl;
        }
        else if(strcmp(cmd,"rmdir")==0)
        {
            if(cat[0]=='/'&&strlen(cat)==1)
                cout<<"当前目录为root目录"<<endl;
            else
            {
                if(strchr(cat,'/')==cat)//绝对路径
                {
                    p=strrchr(cat+sizeof(char),'/');
                    //cout<<cat+1<<p<<endl;
                    if(p==NULL)
                        rmdir(cat+sizeof(char),root);
                    else
                    {
                        strncpy(path,cat+sizeof(char),p-cat-sizeof(char));
                        inode=search_dir(path,root,0);
                        rmdir(p+sizeof(char),inode);
                    }
                }
                else//相对路径
                {
                    p=strrchr(cat,'/');
                    if(p==NULL)
                        rmdir(cat,current);
                    else
                    {
                        strncpy(path,cat,p-cat);
                        inode=search_dir(path,current,0);
                        rmdir(p+1,inode);
                    }

                }
            }

            cout<<"文件删除成功"<<endl;
        }
        else if(strcmp(cmd,"exit")==0)
        {
            fclose(virtualDisk);
            break;
        }

        else
            cout<<"命令错误"<<endl;
    }
    return 0;
}
