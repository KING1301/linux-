#include <gtk/gtk.h> 
#include<stdio.h>     
#include<time.h>
char buf[128];
gint sclock(gpointer clock_label)
{
time_t  timet;      
struct  tm  *tm;
time(&timet);
tm =localtime(&timet);
sprintf(buf,"%02d:%02d:%02d",tm->tm_hour,tm->tm_min,tm->tm_sec);  
gtk_label_set_text(clock_label, buf); 
return 1;
}
int main(int argc, char *argv[])   
{  
    GtkWidget *clock_label;  
    GtkWidget *label;
    GtkWidget *window;  
    GtkWidget *hbox; 
    char title[20];
    gtk_init(&argc, &argv);  
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);    
    gtk_window_set_default_size(GTK_WINDOW(window), 240,120);  
    gtk_window_move(GTK_WINDOW(window),0,0);
    sprintf (title, "CLOCK PID:%d", getpid ());
    gtk_window_set_title(GTK_WINDOW(window), title);  
    hbox = gtk_hbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(window), hbox);
    clock_label = gtk_label_new(""); 
    sclock(clock_label); 
    label = gtk_label_new("系统时间"); 
    gtk_box_pack_end(GTK_BOX(hbox),clock_label,TRUE,TRUE,2); 
    gtk_box_pack_end(GTK_BOX(hbox),label,TRUE,TRUE,2); 
    gtk_widget_show_all(window);  
    g_signal_connect(window, "destroy",G_CALLBACK(gtk_main_quit), NULL); 
    g_timeout_add(1000,sclock,clock_label);
    gtk_main();  
    return 0;  
}  



