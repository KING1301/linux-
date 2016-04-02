#include <gtk/gtk.h>  
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> 
char buf[10];  
long int totalold,idleold;
gint getinfo(long int *idlep ,long int *totalp)
{
	FILE *fp;
    int r_num;
    long int user,nice,sys,idle,iowait,irq,softirq;   
    char bufdata[50];      
    fp = fopen("/proc/stat","r");  
    if(fp == NULL)  
    {  
                printf("open error\n"); 
                return -1;  
    }   
    r_num = fread(bufdata,1,sizeof(bufdata)-1, fp); 
    if(r_num==0)
    	return -1;
    bufdata[r_num-1]='\0';
//    printf("%s\n",bufdata );
    sscanf(bufdata,"%*s %d %d %d %d %d %d %d",&user,&nice,&sys,&idle,&iowait,&irq,&softirq);
    if(idle<0)
    	idle=0;
    *totalp= user+nice+sys+idle+iowait+irq+softirq;
    *idlep=idle;
    fclose(fp);
    return 0;

}
gint CPUinfo(gpointer cpu_label)
{  
    long int total,idle;     
    float cpu_usage; 
    getinfo(&idle,&total);  
    cpu_usage = (float)(total-totalold-(idle-idleold)) / (total-totalold)*100 ; 
    totalold=total;
    idleold=idle;  
    sprintf(buf,"%0.2f%%",cpu_usage);  
    gtk_label_set_text(cpu_label, buf);   
    return 1; 
}

int main(int argc, char *argv[])   
{  
    GtkWidget *cpu_label;  
    GtkWidget *label;
    GtkWidget *window;  
    GtkWidget *hbox; 
    char title[20];
    gtk_init(&argc, &argv);  
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);   
    gtk_window_set_default_size(GTK_WINDOW(window), 240,120);  
    gtk_window_move(GTK_WINDOW(window),250,0);
    sprintf (title, "CPU PID:%d", getpid ());
    gtk_window_set_title(GTK_WINDOW(window), title); 
    hbox = gtk_hbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(window), hbox);
    cpu_label= gtk_label_new("");  
    getinfo(&idleold,&totalold);
    label = gtk_label_new("cpu"); 
    gtk_box_pack_end(GTK_BOX(hbox),cpu_label,TRUE,TRUE,2); 
    gtk_box_pack_end(GTK_BOX(hbox),label,TRUE,TRUE,2); 
    gtk_widget_show_all(window);  
    g_signal_connect(window, "destroy",G_CALLBACK(gtk_main_quit), NULL); 
    g_timeout_add(2000,CPUinfo,cpu_label);
    gtk_main();  
    return 0;  
}  



