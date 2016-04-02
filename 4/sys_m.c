#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <sys/stat.h>
#include <gdk/gdkkeysyms.h>

#define POINT_MAX 100
#define PMAX    200
enum  //进程信息显示列表
{
    NAME_COLUMN,
    PID_COLUMN,
    STATUS_COLUMN,
    CPU_COLUMN,
    MEMORY_COLUMN,
    PRI_COLUMN,
    N_COLUMNS
};

typedef struct
{
    int pid[10];
    char name[128];//进程名称
    char task_state[10];//
    char priority[10];
    char user[10];
    char cpu_usage[10];
    char mem_usage[10];
} proc_info; //保存读取的进程信息

GtkWidget *window;//主窗口

GtkWidget *cpu_draw_area;//CPU信息绘图区
GtkWidget *mem_draw_area;//内存信息绘图区
GtkWidget *mem_label;//内存信息显示标签
GtkWidget *swap_label;
GtkWidget *ptree_view;
GtkWidget *cpu_progress;//显示进度条
GtkWidget *mem_progress;
GtkWidget *memswapprogress;

GtkWidget *status_bar;//状态栏
GtkListStore *process_store;
float cpu_point[POINT_MAX];//CPU绘图区点信息队列					
int cpu_pointp=POINT_MAX;//当前的最后一个有效的点信息
float mem_point[POINT_MAX];//MEM绘图区点信息队列					
float memswap_graph[POINT_MAX];//SWAP MEM绘图区点信息队列
int mem_pointp=POINT_MAX;//当前的最后一个有效的点信息
float cpu_rate = 0;	//CPU利用率						
long int total = 0;					        
long int idle_old = 0;							
long int total_old = 0;	//上一次采样CPU总时间						
long int p_user[PMAX];//每个进程上一次采样的CPU占用时间					   	
GdkPixmap *CPU_graph = NULL;//CPU绘图区后端位图				
GdkPixmap *MEM_graph = NULL;//MEM绘图区后端位图			



//函数声明
void destroy_window (GtkWidget *, gpointer);
int getinfo(char *info,char *version);//获取系统信息函数
int fresh_processinfo ();//刷新进程列表页面
int get_cpu_rate ();//计算当前CPU利用率

int create_status_page (gpointer notebook);//创建资源信息页面，显示CPU内存信息
int create_process_page (gpointer notebook);//创建进程显示信息页面
int create_info_page (gpointer notebook);//创建系统信息页面

int cpu_draw_graph (gpointer data);//绘制cpu利用率曲线
int mem_draw_graph (gpointer data);//绘制MEM利用率曲线

void fresh_clicked ();
void delete_clicked ();

int cpu_configure_event (GtkWidget *, GdkEventConfigure *, gpointer);//cpu绘图区属性改变事件处理函数
int cpu_expose_event (GtkWidget *, GdkEventExpose *, gpointer);//cpu绘图区暴露事件处理函数
int mem_configure_event (GtkWidget *, GdkEventConfigure *, gpointer);
int mem_expose_event (GtkWidget *, GdkEventExpose *, gpointer);

int main (int argc, char *argv[])
{
    GtkWidget *main_vbox;
    GtkWidget *notebook;
    gtk_init (&argc, &argv);
    memset (cpu_point, 0, sizeof (cpu_point));
    memset (mem_point, 0, sizeof (mem_point));
    memset (memswap_graph, 0, sizeof (memswap_graph));
    memset (p_user, 0,sizeof (p_user));

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "系统监视器");//创建窗口
    gtk_widget_set_size_request (window, 550, 500);
    gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(window),5);

    g_signal_connect(G_OBJECT(window), "destroy",
                     G_CALLBACK(gtk_main_quit), NULL);


    main_vbox =gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), main_vbox);

    notebook = gtk_notebook_new ();//创建notebook构件
    gtk_widget_show (notebook);
    gtk_box_pack_start (GTK_BOX (main_vbox), notebook, TRUE, TRUE, 0);


    status_bar = gtk_statusbar_new ();

    gtk_box_pack_start (GTK_BOX (main_vbox), status_bar, 0, FALSE, 0);

    create_process_page (notebook);//添加页面
    create_status_page (notebook);
    create_info_page (notebook);

    cpu_draw_graph(NULL);
    mem_draw_graph(NULL);
    gtk_widget_show_all (window);
    gtk_timeout_add (2000,fresh_processinfo,NULL);//添加定时器并绑定处理函数
    gtk_timeout_add (1000,cpu_draw_graph, NULL); 
    gtk_timeout_add (1000,mem_draw_graph, NULL); 

    gtk_main ();

    return 0;
}

int fresh_processinfo()
{
    gtk_list_store_clear (process_store);//清空列表
    get_process_info (process_store);//重新更新列表
    return 1;
}


int getcpuinfo(long int *idlep,long int *totalp)//获取CPU时间信息
{
    FILE *fp;
    int r_num;
    long int user,nice,sys,idle,iowait,irq,softirq;
    char bufdata[50];
    fp = fopen("/proc/stat","r");
    if(fp == NULL)
        return -1;
    r_num = fread(bufdata,1,sizeof(bufdata)-1, fp);
    if(r_num==0)
        return -1;
    bufdata[r_num-1]='\0';
    sscanf(bufdata,"%*s %d %d %d %d %d %d %d",&user,&nice,&sys,&idle,&iowait,&irq,&softirq);//获取计算CPU利用率所需的时间数据
    if(idle<0)
        idle=0;
    *totalp= user+nice+sys+idle+iowait+irq+softirq;
    *idlep=idle;
    fclose(fp);
    return 0;

}

int get_cpu_rate ()
{
    long int ctotal,cidle;
    getcpuinfo(&cidle,&ctotal);
    cpu_rate = (float)(ctotal-total_old-(cidle-idle_old)) / (ctotal-total_old)*100 ;//计算CPU利用率
    total=ctotal-total_old;
    total_old=ctotal;//更新上一次采样的数据
    idle_old=cidle;
    return 1;
}

int create_status_page (gpointer notebook)
{
    GtkWidget *vbox;
    GtkWidget *frame;
    GtkWidget *label;
    vbox = gtk_vbox_new (FALSE, 10);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (notebook), vbox);

    frame = gtk_frame_new ("CPU历史");
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 10);

    cpu_draw_area = gtk_drawing_area_new ();
    gtk_drawing_area_size (GTK_DRAWING_AREA (cpu_draw_area), 400, 100);
    gtk_widget_show (cpu_draw_area);
    gtk_container_add (GTK_CONTAINER (frame), cpu_draw_area);

    g_signal_connect (cpu_draw_area, "expose_event",
                      G_CALLBACK (cpu_expose_event), NULL);//为绘图区绑定相应的事件处理函数
    g_signal_connect (cpu_draw_area, "configure_event",
                      G_CALLBACK (cpu_configure_event), NULL);

    cpu_progress = gtk_progress_bar_new();//创建进度条
    mem_progress = gtk_progress_bar_new();
    memswapprogress=gtk_progress_bar_new();

    gtk_box_pack_start (GTK_BOX (vbox), cpu_progress, FALSE, FALSE, 5);
    gtk_widget_set_size_request(cpu_progress,5,20);

    frame = gtk_frame_new ("内存历史");
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 10);

    mem_draw_area = gtk_drawing_area_new ();
    gtk_widget_show (mem_draw_area);
    gtk_drawing_area_size (GTK_DRAWING_AREA (mem_draw_area), 400, 100);
    gtk_container_add (GTK_CONTAINER (frame), mem_draw_area);

    g_signal_connect (mem_draw_area, "expose_event",
                      G_CALLBACK (mem_expose_event), NULL);
    g_signal_connect (mem_draw_area, "configure_event",
                      G_CALLBACK (mem_configure_event), NULL);

    gtk_box_pack_start (GTK_BOX (vbox),mem_progress,FALSE,FALSE,5);
    gtk_box_pack_start(GTK_BOX(vbox),memswapprogress,FALSE,FALSE,5);
    gtk_widget_set_size_request(mem_progress,5,20);
    gtk_widget_set_size_request(memswapprogress,5,20);


    label = gtk_label_new ("       资源       ");
    gtk_widget_show (label);
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook),
                                gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 1), label);//向notebook构建添加页面
}

int create_process_page (gpointer notebook)
{
    GtkWidget *scrolled_window;
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkWidget *label;
    GtkWidget *prefresh_button, *pdelete_button; 
    int i;
    char *col_name[6] = { "NAME", "PID", "STATUS", "CPU", "MEMORY","PRI"};

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);

    gtk_container_add (GTK_CONTAINER (notebook), vbox);

    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_size_request (scrolled_window, 500, 300);
    gtk_widget_show (scrolled_window);
    gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 5);

    process_store = gtk_list_store_new ( N_COLUMNS,
                                         G_TYPE_STRING,
                                         G_TYPE_STRING,
                                         G_TYPE_STRING,
                                         G_TYPE_STRING,
                                         G_TYPE_STRING,
                                         G_TYPE_STRING);//创建树状图


    ptree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (process_store));

    g_object_unref (G_OBJECT (process_store));

    gtk_widget_show (ptree_view);
    gtk_container_add (GTK_CONTAINER (scrolled_window), ptree_view);


    for (i = 0; i <=5; i++)//初始化树状图
    {
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (col_name[i],
                 renderer,"text",i,NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (ptree_view), column);
    }

    get_process_info (process_store); 

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, 0, FALSE, 0);

    prefresh_button = gtk_button_new ();//添加按钮
    gtk_widget_show (prefresh_button);
    gtk_button_set_label (GTK_BUTTON (prefresh_button), "refresh");
    g_signal_connect (G_OBJECT (prefresh_button),"clicked",
                      G_CALLBACK(fresh_clicked),
                      NULL);
    gtk_box_pack_start (GTK_BOX (hbox), prefresh_button, FALSE, FALSE, 10);

    pdelete_button = gtk_button_new ();
    gtk_widget_show (pdelete_button);
    gtk_button_set_label (GTK_BUTTON (pdelete_button), "kill");
    g_signal_connect (G_OBJECT (pdelete_button),"clicked",
                      G_CALLBACK(delete_clicked),
                      NULL);//为按钮绑定时间处理函数
    gtk_box_pack_start (GTK_BOX (hbox), pdelete_button, FALSE, FALSE, 10);

    label = gtk_label_new ("       进程       ");
    gtk_widget_show (label);
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook),
                                gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), 0), label);//添加页面
}


int create_info_page (gpointer notebook)
{
    GtkWidget *vbox;
    GtkWidget *frame;
    GtkWidget *label;

    GtkWidget *info_label;
    char info[128];
    char version[128];

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER(notebook), vbox);

    frame = gtk_frame_new ("CPU Information:");
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 10);

    getinfo (info,version);
    info_label = gtk_label_new (info);
    gtk_widget_show (info_label);
    gtk_container_add (GTK_CONTAINER(frame), info_label);

    frame = gtk_frame_new ("OS Information:");
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 10);

    info_label = gtk_label_new (version);
    gtk_widget_show (info_label);
    gtk_container_add (GTK_CONTAINER(frame), info_label);

    label = gtk_label_new ("       系统信息       ");
    gtk_widget_show (label);
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook),
                                gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook),2), label);//添加页面
}

int getinfo(char *info,char *version)//获取系统信息
{
    FILE *fp;
    char buf[256];
    char cat[60];
    char *ps=buf;
    memset(buf,0,sizeof(buf));
    memset(cat,0,sizeof(cat));
    fp=fopen("/proc/cpuinfo","r");
    fread(buf,1,sizeof(buf),fp);
    fclose(fp);
    ps=strstr(buf,"vendor_id");
    sscanf(ps,"%[^\n]",cat);
    sprintf(info,"%s",cat);
    ps=strstr(buf,"model name");
    sscanf(ps,"%[^\n]",cat);
    sprintf(info,"%s\n%s",info,cat);//提取信息
    ps=strstr(buf,"cpu MHz");
    sscanf(ps,"%[^\n]",cat);
    sprintf(info,"%s\n%s",info,cat);
    ps=strstr(buf,"cache size");
    sscanf(ps,"%[^\n]",cat);
    sprintf(info,"%s\n%s",info,cat);

    fp=fopen("/proc/version","r");
    fread(buf,1,sizeof(buf),fp);
    fclose(fp);
    sscanf(buf,"%*s%*s%s",cat);
    sprintf(version,"Linux version:\t%s",cat);
    ps=strstr(buf,"gcc");
    sscanf(ps,"%*s%*s%s",cat);
    sprintf(version,"%s\ngcc version:\t%s",version,cat);
    return 0;
}

void read_proc(proc_info *info,char* c_pid,int num)
{
    FILE* fp;
    char buf[128];
    float mem;
    long int m;
    long int utime;
    float rate;
    memset(buf,0,sizeof(buf));
    sprintf(buf,"/proc/%s/stat",c_pid);//读取stat文件
    if (!(fp = fopen(buf,"r")))
    {
        printf("read %s file fail!\n",buf);
        return;
    }
    fread(buf,1,sizeof(buf)-1,fp);
    fclose(fp);
    sscanf(buf,"%s%s%s%*s%*s%*s%*s%*s%*s%*s%*s%*s%*s%d%*s%*s%*s%s%*s%*s%*s%*s%d",info->pid,info->name,info->task_state,&utime,info->priority,&m);//提取相应信息
    mem = (float) m/(1024 * 1024);
    sscanf(info->name,"(%[^)])",info->name);
    sprintf (info->mem_usage, "%-.2f MB",mem);
    rate =(float) (utime-p_user[num])/total*2;//计算进程CPU利用率
    if(rate<0||rate>1)
    {
        rate=0;
    }
    sprintf (info->cpu_usage, "%.2f%%",rate*100);
    p_user[num] = utime;
}


int get_process_info(GtkListStore *store)
{
    DIR *dir;
    struct dirent *ptr;
    GtkTreeIter iter;
    long int pcuser[PMAX];
    proc_info info;
    int num=0;
    //打开目录

    if (!(dir = opendir("/proc")))
        return 0;
    //读取目录
    while (ptr = readdir(dir))
    {
        //循环读取出所有的进程文件
        if (ptr->d_name[0] > '0' && ptr->d_name[0] <= '9')
        {
            //获取进程信息
            read_proc(&info,ptr->d_name,num);//读取信息
            if(strcmp(info.task_state,"R"))
                sprintf(info.task_state,"%s","running");
            if(strcmp(info.task_state,"S"))
                sprintf(info.task_state,"%s","sleeping");
            gtk_list_store_append (store, &iter);
            gtk_list_store_set (store, &iter,
                                NAME_COLUMN,info.name,
                                PID_COLUMN,info.pid,
                                STATUS_COLUMN,info.task_state,
                                CPU_COLUMN,info.cpu_usage,
                                MEMORY_COLUMN,info.mem_usage,
                                PRI_COLUMN,info.priority,
                                -1);
            num = (num + 1 ) % PMAX;
        }
    }
    closedir(dir);
}

int mem_draw_graph (gpointer data)//绘制mem曲线
{
    FILE *fp;
    char buf[256];
    char string[128];
    char *ps=buf;
    int i,width,height;
    float step,current,current1;
    float mem_rate,memswap_rate;
    long int  mem_total,mem_free,memswap_total,memswap_free;
    int context_id;
    fp=fopen("/proc/meminfo","r");
    fread(buf,1,sizeof(buf),fp);
    ps=strstr(buf,"MemTotal");
    sscanf(ps,"%*s%d",&mem_total);
    ps=strstr(buf,"MemFree");
    sscanf(ps,"%*s%d",&mem_free);

    mem_rate=((float)(mem_total-mem_free)/mem_total);
    sprintf(string,"内存: %d KB / %d KB (%0.2f)",mem_total-mem_free,mem_total,mem_rate*100);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mem_progress), mem_rate);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(mem_progress),string);
    fseek(fp,128L,1);
    fread(buf,1,sizeof(buf),fp);
    fclose(fp);
    ps=strstr(buf,"SwapTotal");
    sscanf(ps,"%*s%d",&memswap_total);
    ps=strstr(buf,"SwapFree");
    sscanf(ps,"%*s%d",&memswap_free);
    memswap_rate=((float)(memswap_total-memswap_free)/memswap_total);
    sprintf(string,"内存2: %d KB / %d KB (%0.2f)",memswap_total-memswap_free,memswap_total,memswap_rate*100);

    context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (status_bar),"status_bar");
    sprintf(buf,"cpu: %0.2f%%  mem: %0.2f%%  memswap: %0.2f%%",cpu_rate,mem_rate*100,memswap_rate*100);
    gtk_statusbar_push (GTK_STATUSBAR (status_bar), context_id, buf);

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(memswapprogress), memswap_rate);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(memswapprogress),string);

    if (MEM_graph == NULL)
        return 1;
//清空绘图区
    gdk_draw_rectangle (MEM_graph, window->style->white_gc, TRUE, 0, 0,
                        mem_draw_area->allocation.width,
                        mem_draw_area->allocation.height);


    width = mem_draw_area->allocation.width;//获取绘图区尺寸
    height = mem_draw_area->allocation.height;


    current=(float)(mem_rate*height);//计算绘图点坐标
    current1=(float)(memswap_rate*height);
    if (mem_pointp)
    {
        mem_pointp--;
    }

    for ( i = 0 ; i < POINT_MAX - 1 ; i ++)//将绘图点添加到绘图坐标数组中
    {
        mem_point[i] = mem_point[i+1];
    }
    mem_point[POINT_MAX-1] = (float)(height-current); 



    for ( i = 0 ; i < POINT_MAX - 1 ; i ++)
    {
        memswap_graph[i] = memswap_graph[i+1];
    }
    memswap_graph[POINT_MAX-1] = (float)(height-current1);


    step = (float)width/POINT_MAX;//计算绘图横坐标
    GdkGC *gc = gdk_gc_new(GDK_DRAWABLE(MEM_graph));
    GdkGC *gc1 = gdk_gc_new(GDK_DRAWABLE(MEM_graph));
    GdkColor color;
    gdk_color_parse("#696969", &color);//设置画笔颜色
    gdk_gc_set_rgb_fg_color(gc, &color);

    gdk_draw_line(MEM_graph, gc, 0,height/2, width,height/2);//绘制参考线
    gdk_draw_line(MEM_graph, gc, step*(POINT_MAX/2),0, step*(POINT_MAX/2),height);
    gdk_color_parse("#00FF00", &color);
    gdk_gc_set_rgb_fg_color(gc, &color);

    gdk_color_parse("#FF00FF", &color);
    gdk_gc_set_rgb_fg_color(gc1, &color);
    for (i = POINT_MAX - 1; i >mem_pointp ; i -- )
    {

        gdk_draw_line (MEM_graph, gc, (i+1) * step,mem_point[i], i* step, mem_point[i-1]);//绘制坐标曲线
        gdk_draw_line (MEM_graph, gc1, (i+1) * step,memswap_graph[i], i* step, memswap_graph[i-1]);
    }
    gtk_widget_queue_draw ( mem_draw_area);//触发暴露事件更新绘图区
    return 1;
}


int cpu_draw_graph (gpointer data)
{
    int width,height,i;
    char buffer[64];
    float step,current;

    get_cpu_rate();
    sprintf (buffer, "CPU rate:%.2f%%",cpu_rate);

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(cpu_progress), cpu_rate/100);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(cpu_progress),buffer);
    if (CPU_graph == NULL)
        return 1;

    gdk_draw_rectangle (CPU_graph, window->style->white_gc, TRUE, 0, 0,
                        mem_draw_area->allocation.width,
                        mem_draw_area->allocation.height);


    width = cpu_draw_area->allocation.width;
    height = cpu_draw_area->allocation.height;

    current = (float)(cpu_rate/100 *height);

    if (cpu_pointp)
    {
        cpu_pointp--;
    }
    for ( i = 0 ; i < POINT_MAX - 1 ; i ++)
    {
        cpu_point[i] = cpu_point[i+1];
    }

    cpu_point[POINT_MAX-1] = (float)(height - current);

    step = (float)width/POINT_MAX;
    GdkGC *gc = gdk_gc_new(GDK_DRAWABLE(CPU_graph));
    GdkColor color;
    gdk_color_parse("#696969", &color);
    gdk_gc_set_rgb_fg_color(gc, &color);

    gdk_draw_line(CPU_graph, gc, 0,height/2, width,height/2);
    gdk_draw_line(CPU_graph, gc, step*(POINT_MAX/2),0, step*(POINT_MAX/2),height);

    gdk_color_parse("#1E90FF", &color);
    gdk_gc_set_rgb_fg_color(gc, &color);

    for (i = POINT_MAX - 1; i >cpu_pointp ; i -- )
    {
        gdk_draw_line (CPU_graph, gc, (i +1)* step, cpu_point[i],
                       i* step, cpu_point[i-1]  );
    }

    gtk_widget_queue_draw (cpu_draw_area);
    return 1;
}


int cpu_configure_event (GtkWidget *widget, GdkEventConfigure *event, gpointer data)//属性改变事件处理函数
{
    if (CPU_graph)//位图已存在则释放重新生成
    {
        g_object_unref (CPU_graph);
    }

    CPU_graph = gdk_pixmap_new (widget->window,
                             widget->allocation.width, widget->allocation.height,-1);//创建位图

    gdk_draw_rectangle (CPU_graph, widget->style->white_gc, TRUE, 0, 0,
                        widget->allocation.width, widget->allocation.height);//清空位图信息
    return 1;
}

int cpu_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data)//暴露事件处理函数
{
    gdk_draw_pixmap (widget->window,
                     widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                     CPU_graph,
                     event->area.x, event->area.y,
                     event->area.x, event->area.y,
                     event->area.width, event->area.height);//从位图向屏幕拷贝图像完成绘图
    return 1;
}


int mem_configure_event (GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
    if (MEM_graph)
    {
        g_object_unref (MEM_graph);
    }

    MEM_graph = gdk_pixmap_new (widget->window,
                             widget->allocation.width, widget->allocation.height,-1);
   
    gdk_draw_rectangle (MEM_graph, widget->style->white_gc, TRUE, 0, 0,
                        widget->allocation.width, widget->allocation.height);
    return 1;
}

int mem_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    gdk_draw_pixmap (widget->window,
                     widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                     MEM_graph,
                     event->area.x, event->area.y,
                     event->area.x, event->area.y,
                     event->area.width, event->area.height);
    return 1;
}


void fresh_clicked ()
{
    gtk_list_store_clear (process_store);//刷新列表
    get_process_info (process_store);
}

void delete_clicked ()
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    char *pid;
    pid_t pid_num;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(ptree_view));//获取选择的列表索引
    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        gtk_tree_model_get (model, &iter, PID_COLUMN, &pid, -1);//根据索引获取相应行进程的pid
        pid_num = atoi (pid);

        if(kill (pid_num, SIGTERM) == -1 )//杀死进程
            return ;
        gtk_list_store_clear (process_store);
        get_process_info (process_store);//更新列表
    }
}

