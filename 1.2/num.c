#include <gtk/gtk.h>  
int n=0;
int count=0;
char buf[5];   
gint increase(gpointer num)
{
    if(n==100)
        return 0;
    n++; 
    count+=n; 
    sprintf(buf,"%d=%d+%d",count,count-n,n);  
    gtk_label_set_text(num, buf); 
    return 1;
}
int main(int argc, char *argv[])   
{  
    GtkWidget *num_label;  
    GtkWidget *label;
    GtkWidget *window;  
    GtkWidget *hbox; 
    char title[20];
    gtk_init(&argc, &argv);  
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);   
    gtk_window_set_default_size(GTK_WINDOW(window), 240,120);
    gtk_window_move(GTK_WINDOW(window),500,0);  
    sprintf (title, "NUM PID:%d", getpid ());
    gtk_window_set_title(GTK_WINDOW(window), title);  
    hbox = gtk_hbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(window), hbox);
    num_label = gtk_label_new("");  
    increase(num_label);
    label = gtk_label_new("累加和:"); 
    gtk_box_pack_end(GTK_BOX(hbox),num_label,TRUE,TRUE,2); 
    gtk_box_pack_end(GTK_BOX(hbox),label,TRUE,TRUE,2); 
    gtk_widget_show_all(window);  
    g_signal_connect(window, "destroy",G_CALLBACK(gtk_main_quit), NULL); 
    g_timeout_add(3000,increase,num_label);
    gtk_main();  
    return 0;  
}  



