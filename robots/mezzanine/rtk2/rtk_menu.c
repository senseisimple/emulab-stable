
#include <stdlib.h>
#define DEBUG
#include "rtk.h"
#include "rtkprivate.h"


// Some local functions
static void rtk_on_activate(GtkWidget *widget, rtk_menuitem_t *item);


// Create a new menu
rtk_menu_t *rtk_menu_create(rtk_canvas_t *canvas, const char *label)
{
  rtk_menu_t *menu;

  menu = malloc(sizeof(rtk_menu_t));
  menu->item = gtk_menu_item_new_with_label(label);
  menu->menu = gtk_menu_new();

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu->item), menu->menu);
  gtk_menu_bar_append(GTK_MENU_BAR(canvas->menu_bar), menu->item);

  return menu;
}


// Create a new menu item
rtk_menuitem_t *rtk_menuitem_create(rtk_menu_t *menu,
                                    const char *label, int check)
{
  rtk_menuitem_t *item;

  item = malloc(sizeof(rtk_menuitem_t));
  item->menu = menu;
  item->activated = FALSE;
  item->checked = FALSE;

  if (check)
  {
    item->checkitem = 1;
    item->item = gtk_check_menu_item_new_with_label(label);
    gtk_check_menu_item_set_show_toggle(GTK_CHECK_MENU_ITEM(item->item), 1);
  }
  else
  {
    item->checkitem = 0;
    item->item = gtk_menu_item_new_with_label(label);
  }
  
  gtk_menu_append(GTK_MENU(menu->menu), item->item);

  gtk_signal_connect(GTK_OBJECT(item->item), "activate",
                     GTK_SIGNAL_FUNC(rtk_on_activate), item);

  return item;
}


// Delete a menu item
void rtk_menuitem_destroy(rtk_menuitem_t *item)
{
  // Hmmm, there doesnt seem to be any way to remove
  // an item from a menu.
  gtk_container_remove(GTK_CONTAINER(item->menu->menu), item->item);
  free(item);
}


// Test to see if the menu item has been activated
// Calling this function will reset the flag
int rtk_menuitem_isactivated(rtk_menuitem_t *item)
{
  if (item->activated)
  {
    item->activated = FALSE;
    return TRUE;
  }
  return FALSE;
}


// Set the check state of a menu item
void rtk_menuitem_check(rtk_menuitem_t *item, int check)
{
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item->item), check);
}


// Test to see if the menu item is checked
int rtk_menuitem_ischecked(rtk_menuitem_t *item)
{
  return item->checked;
}


// Handle activation
void rtk_on_activate(GtkWidget *widget, rtk_menuitem_t *item)
{
  item->activated = TRUE;
  if (item->checkitem)
    item->checked = GTK_CHECK_MENU_ITEM(item->item)->active;
}
