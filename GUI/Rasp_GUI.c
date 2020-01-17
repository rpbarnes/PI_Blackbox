// gcc  -Wall -Wextra -o PI_DAQ Rasp_GUI.c -rdynamic $(pkg-config gtk+-3.0 --cflags --libs)
#include <stdlib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

// Display metrics in GUI: AVG, MAX, MIN, COOLDown time
// LUT of MAC Address vs IP

int seconds = 0;
guint timerId = 0;
GtkLabel *Elapsed_Time_Label;
GtkLabel *Current_Time_Label;
GtkLabel *Current_Date_Label;
GtkLabel *IP_Address_Label;

GtkGrid *Cooler_Power_Grid;
GtkGrid *Heat_Load_Grid;
GtkLabel *Cooler_Container_Label;
GtkLabel *Heat_Load_Container_Label;
GtkLabel *Disk_Use_Value_Label;

GString* timer_string = NULL;
GString* current_time_string = NULL;
GString* current_date_string = NULL;
GString* IP_Address_string = NULL;
GString* Disk_Usage_string = NULL;

gpointer tick;

gboolean timerTick(__attribute__((unused)) gpointer userData);
void Get_Time(GString* time_string);
void Get_Date(GString* date_string);
void Get_IP_Addr(GString* IP_Address_string);
void Get_Disk_Usage(GString* Disk_Usage_string);
void startClock();


gboolean timerTick(__attribute__((unused)) gpointer userData)
{
    seconds += 1;
    Get_Time(current_time_string);  // get current time and store in "current_time_string"
    Get_Date(current_date_string);  // get current date and store in "current_date_string"
    Get_IP_Addr(IP_Address_string);
    gtk_label_set_text(Current_Time_Label, current_time_string->str);
    gtk_label_set_text(Current_Date_Label, current_date_string->str);
    gtk_label_set_text(IP_Address_Label, IP_Address_string->str);

    g_string_printf(timer_string,"%02d:%02d:%02d", seconds/3600, seconds/60, seconds%60);
    gtk_label_set_text(Elapsed_Time_Label,timer_string->str);

    return TRUE;
}

void Get_Time(GString* time_string)
{
  time_t raw_time;
  struct tm * timeinfo;

  time ( &raw_time );
  timeinfo = localtime ( &raw_time );
  g_string_printf(time_string,"%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  //printf("%s\n", time_string->str);
}

void Get_Date(GString* date_string)
{
  time_t raw_time;
  struct tm * timeinfo;

  time ( &raw_time );
  timeinfo = localtime ( &raw_time );
  g_string_printf(date_string,"%02d/%02d/%02d", timeinfo->tm_mday, (timeinfo->tm_mon)+1, (timeinfo->tm_year)-100);
  //printf("%s\n", time_string->str);
}


void Get_IP_Addr(GString* IP_Address_string)
{
  int fd;
  struct ifreq ifr;

  fd = socket(AF_INET, SOCK_DGRAM, 0);

  /* I want to get an IPv4 IP address */
  ifr.ifr_addr.sa_family = AF_INET;

  /* I want IP address attached to "wlan0" */
  strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1);
  ioctl(fd, SIOCGIFADDR, &ifr);
  close(fd);

  /* display result */
  g_string_printf(IP_Address_string,"%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
}

void GUI_Init()
{
    if(timer_string == NULL)
    {
        timer_string = g_string_sized_new(15);
    }
    if(current_time_string == NULL)
    {
        current_time_string = g_string_sized_new(15);
    }
    if(current_date_string == NULL)
    {
        current_date_string = g_string_sized_new(15);
    }
    if(IP_Address_string == NULL)
    {
        IP_Address_string = g_string_sized_new(15);
    }
    if(Disk_Usage_string == NULL)
    {
        Disk_Usage_string = g_string_sized_new(15);
    }
}

void startClock()
{
    g_print("Start Clicked\n");
    if(timerId == 0)
	       timerId = g_timeout_add(1000,timerTick,NULL);
    else
	       g_print("Already Running\n");
}


// Handle the user trying to close the window
gboolean windowDelete(__attribute__((unused)) GtkWidget *widget,
		      __attribute__((unused)) GdkEvent  *event,
		      __attribute__((unused)) gpointer   data)
{
    g_print("No Quitting!!\n",__FUNCTION__);
    return TRUE;    // Returning TRUE stops the window being deleted.
                    // Returning FALSE allows deletion.
}

int main ( int argc, char **argv) {

    GtkWidget *window;
    GtkBuilder *builder = NULL;
    GError *error = NULL;

    gtk_init (&argc , &argv);

    builder = gtk_builder_new();

    GUI_Init();
    startClock();
    timerTick(tick);
    if( gtk_builder_add_from_file (builder,"Rasp_GUI2.glade" , &error) == 0)
    {
        g_print("gtk_builder_add_from_file FAILED\n");
	      g_print("%s\n",error->message);
	      return EXIT_FAILURE;
    }



    // ---------------------------------------------------- CSS -----------------------------------------------------------
    GtkCssProvider *provider;
    GdkDisplay *display;
    GdkScreen *screen;
    GtkWidget *close = NULL;
    GtkWindow *main_window = NULL;
    GtkLabel *Temp1_Value_Label = NULL;

    provider = gtk_css_provider_new ();
    display = gdk_display_get_default ();
    screen = gdk_display_get_default_screen (display);
    gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    const gchar *myCssFile = "my_style.css";

    gtk_css_provider_load_from_file(provider, g_file_new_for_path(myCssFile), &error);
    g_object_unref (provider);
   // --------------------------------------------------------------------------------------------------------------------

    window  = GTK_WIDGET (gtk_builder_get_object (builder,"Main_Window"));
    main_window = GTK_WINDOW (gtk_builder_get_object (builder,"Main_Window"));
    Elapsed_Time_Label  = GTK_LABEL(gtk_builder_get_object (builder,"Elapsed_Time_Value_Label"));
    Current_Time_Label  = GTK_LABEL(gtk_builder_get_object (builder,"Clock_Value_Label"));
    Current_Date_Label = GTK_LABEL(gtk_builder_get_object (builder,"Date_Value_Label"));
    IP_Address_Label = GTK_LABEL(gtk_builder_get_object (builder,"IP_Addr_Value_Label"));
    Disk_Use_Value_Label = GTK_LABEL(gtk_builder_get_object (builder,"Disk_Use_Value_Label"));

/*
    GtkWidget *label = Current_Time_Label;
    PangoAttrList *attrlist = pango_attr_list_new();
    PangoAttribute *attr = pango_attr_size_new_absolute(30 * PANGO_SCALE);
    pango_attr_list_insert(attrlist, attr);
    gtk_label_set_attributes(GTK_LABEL(label), attrlist);
    pango_attr_list_unref(attrlist);
*/


    g_signal_connect (main_window, "destroy", gtk_main_quit, NULL);
    gtk_builder_connect_signals(builder,NULL);

    gtk_window_fullscreen(GTK_WINDOW(window));
    gtk_widget_show_all (window);

    gtk_main ();
    return EXIT_SUCCESS;
}
