#ifndef __GTK_H__
#define __GTK_H__

#include <windows.h>
#include <glib.h>
#include <stdarg.h>

typedef enum {
   GTK_WINDOW_TOPLEVEL, GTK_WINDOW_DIALOG, GTK_WINDOW_POPUP
} GtkWindowType;

typedef enum {
   GTK_EXPAND = 1 << 0,
   GTK_SHRINK = 1 << 1,
   GTK_FILL   = 1 << 2
} GtkAttachOptions;

typedef enum {
   GTK_SELECTION_SINGLE,
   GTK_SELECTION_BROWSE,
   GTK_SELECTION_MULTIPLE,
   GTK_SELECTION_EXTENDED
} GtkSelectionMode;

typedef enum {
   GDK_INPUT_READ      = 1 << 0,
   GDK_INPUT_WRITE     = 1 << 1,
   GDK_INPUT_EXCEPTION = 1 << 2
} GdkInputCondition;

typedef void (*GdkInputFunction)(gpointer data,gint source,
                                 GdkInputCondition condition);
typedef gchar* (*GtkTranslateFunc)(const gchar *path,gpointer func_data);
typedef void (*GtkDestroyNotify)(gpointer data);

typedef enum {
   GTK_REALIZED    = 1 << 6,
   GTK_VISIBLE     = 1 << 8,
   GTK_SENSITIVE   = 1 << 10,
   GTK_CAN_DEFAULT = 1 << 13
} GtkWidgetFlags;

#define GTK_VISIBLE 1

typedef struct _GtkClass GtkClass;
typedef struct _GtkObject GtkObject;

typedef struct _GtkRequisition GtkRequisition;
typedef struct _GtkAllocation GtkAllocation;
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkSignalType GtkSignalType;

typedef void (*GtkSignalFunc) ();
typedef void (*GtkItemFactoryCallback) ();
typedef void (*GtkSignalMarshaller)(GtkObject *object,GSList *actions,
                                    GtkSignalFunc default_action,
                                    va_list args);
typedef struct _GtkMenuShell GtkMenuShell;
typedef struct _GtkMenuBar GtkMenuBar;
typedef struct _GtkMenuItem GtkMenuItem;
typedef struct _GtkMenu GtkMenu;
typedef struct _GtkAdjustment GtkAdjustment;
typedef struct _GtkSeparator GtkSeparator;
typedef struct _GtkHSeparator GtkHSeparator;
typedef struct _GtkVSeparator GtkVSeparator;
typedef struct _GtkAccelGroup GtkAccelGroup;
typedef struct _GtkPanedChild GtkPanedChild;
typedef struct _GtkPaned GtkPaned;
typedef struct _GtkVPaned GtkVPaned;
typedef struct _GtkHPaned GtkHPaned;

struct _GtkAccelGroup {
   GSList *accel;
};

struct _GtkSignalType {
   gchar *name;
   GtkSignalMarshaller marshaller;
   GtkSignalFunc default_action;
};

struct _GtkClass {
   gchar *Name;
   GtkClass *parent;
   gint Size;
   GtkSignalType *signals;
};

typedef GtkClass *GtkType;

struct _GtkObject {
   GtkClass *klass;
   GData *object_data;
   GData *signals;
   guint32 flags;
};

struct _GtkAdjustment {
   GtkObject object;
   gfloat value,lower,upper;
   gfloat step_increment,page_increment,page_size;
};

struct _GtkRequisition {
   gint16 width, height;
};

struct _GtkAllocation {
   gint16 x,y,width,height;
};

struct _GtkWidget {
   GtkObject object;
   HWND hWnd;
   GtkRequisition requisition;
   GtkAllocation allocation;
   GtkWidget *parent;
};

struct _GtkSeparator {
   GtkWidget widget;
};

struct _GtkHSeparator {
   GtkSeparator separator;
};

struct _GtkVSeparator {
   GtkSeparator separator;
};

struct _GtkMenuItem {
   GtkWidget widget;
   GtkMenu *submenu;
   gint ID;
   gchar *text;
};

struct _GtkMenuShell {
   GtkWidget widget;
   HMENU menu;
   GSList *children;
};

struct _GtkMenu {
   GtkMenuShell menushell;
};

struct _GtkMenuBar {
   GtkMenuShell menushell;
   gint LastID;
};

typedef struct _GtkEditable GtkEditable;
typedef struct _GtkEntry GtkEntry;
typedef struct _GtkText GtkText;
typedef struct _GtkSpinButton GtkSpinButton;

struct _GtkEditable {
   GtkWidget widget;
};

struct _GtkEntry {
   GtkEditable editable;
};

struct _GtkSpinButton {
   GtkEntry entry;
   GtkAdjustment *adj;
   HWND updown;
};

struct _GtkText {
   GtkEditable editable;
};

typedef struct _GtkLabel GtkLabel;

struct _GtkLabel {
   GtkWidget widget;
   gchar *text;
};

typedef struct _GtkContainer GtkContainer;

struct _GtkContainer {
   GtkWidget widget;
   GtkWidget *child;
   guint border_width : 16;
};

struct _GtkPanedChild {
   GtkWidget *widget;
   gint resize : 1;
   gint shrink : 1;
};

struct _GtkPaned {
   GtkContainer container;
   GtkPanedChild children[2];
   gint handle_size,gutter_size;
   gint handle_pos;
};

struct _GtkVPaned {
   GtkPaned paned;
};

struct _GtkHPaned {
   GtkPaned paned;
};

typedef struct _GtkBox GtkBox;
typedef struct _GtkBoxChild GtkBoxChild;
typedef struct _GtkHBox GtkHBox;
typedef struct _GtkVBox GtkVBox;
typedef struct _GtkNotebookChild GtkNotebookChild;
typedef struct _GtkNotebook GtkNotebook;
typedef struct _GtkCList GtkCList;
typedef struct _GtkCListRow GtkCListRow;
typedef struct _GtkCListColumn GtkCListColumn;
typedef struct _GtkItemFactoryEntry GtkItemFactoryEntry;
typedef struct _GtkItemFactory GtkItemFactory;

struct _GtkItemFactoryEntry {
   gchar *path;
   gchar *accelerator;
   GtkItemFactoryCallback callback;
   guint callback_action;
   gchar *item_type;
};

struct _GtkItemFactory {
   GtkObject object;
   GSList *children;
   gchar *path;
   GtkWidget *top_widget;
};

struct _GtkBoxChild {
   GtkWidget *widget;
   guint expand : 1;
   guint fill : 1;
};

struct _GtkBox {
   GtkContainer container;
   GList *children;
   guint16 spacing;
   guint homogeneous : 1;
};

struct _GtkHBox {
   GtkBox box;
};

struct _GtkVBox {
   GtkBox box;
};

struct _GtkNotebookChild {
   GtkWidget *child,*tab_label;
};

struct _GtkNotebook {
   GtkContainer container;
   GSList *children;
   gint selection;
};

struct _GtkCListColumn {
   gchar *title;
   gint width;
   guint visible        : 1;
   guint resizeable     : 1;
   guint auto_resize    : 1;
   guint button_passive : 1;
};

struct _GtkCListRow {
   gpointer data;
   gchar **text;
};

struct _GtkCList {
   GtkContainer container;
   gint ncols;
   HWND header;
   gint16 header_size;
   GSList *rows;
   GtkCListColumn *cols;
   GtkSelectionMode mode;
};

typedef struct _GtkBin GtkBin;

struct _GtkBin {
   GtkContainer container;
   GtkWidget *child;
};

typedef struct _GtkFrame GtkFrame;
typedef struct _GtkButton GtkButton;
typedef struct _GtkToggleButton GtkToggleButton;
typedef struct _GtkCheckButton GtkCheckButton;
typedef struct _GtkRadioButton GtkRadioButton;

struct _GtkFrame {
   GtkBin bin;
   gchar *text;
   GtkRequisition label_req;
};

struct _GtkButton {
   GtkWidget widget;
   gchar *text;
};

struct _GtkToggleButton {
   GtkButton button;
   gboolean toggled;
};

struct _GtkCheckButton {
   GtkToggleButton toggle;
};

struct _GtkRadioButton {
   GtkCheckButton check;
   GSList *group;
};

typedef struct _GtkWindow GtkWindow;

struct _GtkWindow {
   GtkBin bin;
   GtkWindowType type;
   gchar *title;
   gint default_width,default_height;
   GtkMenuBar *menu_bar;
   GtkAccelGroup *accel_group;
   HACCEL hAccel;
   guint modal : 1;
};

typedef struct _GtkTable GtkTable;
typedef struct _GtkTableChild GtkTableChild;
typedef struct _GtkTableRowCol GtkTableRowCol;

struct _GtkTable {
   GtkContainer container;
   GList *children;
   GtkTableRowCol *rows,*cols;
   guint16 nrows,ncols;
   guint16 column_spacing,row_spacing;
   guint homogeneous : 1;
};

struct _GtkTableChild {
   GtkWidget *widget;
   guint16 left_attach,right_attach,top_attach,bottom_attach;
};

struct _GtkTableRowCol {
   guint16 requisition;
   guint16 allocation;
   gint16 spacing;
};

#define GTK_OBJECT(obj) ((GtkObject *)(obj))
#define GTK_CONTAINER(obj) ((GtkContainer *)(obj))
#define GTK_PANED(obj) ((GtkPaned *)(obj))
#define GTK_VPANED(obj) ((GtkVPaned *)(obj))
#define GTK_HPANED(obj) ((GtkHPaned *)(obj))
#define GTK_BIN(obj) ((GtkBin *)(obj))
#define GTK_FRAME(obj) ((GtkFrame *)(obj))
#define GTK_BOX(obj) ((GtkBox *)(obj))
#define GTK_CLIST(obj) ((GtkCList *)(obj))
#define GTK_HBOX(obj) ((GtkHBox *)(obj))
#define GTK_VBOX(obj) ((GtkVBox *)(obj))
#define GTK_NOTEBOOK(obj) ((GtkNotebook *)(obj))
#define GTK_WIDGET(obj) ((GtkWidget *)(obj))
#define GTK_EDITABLE(obj) ((GtkEditable *)(obj))
#define GTK_ENTRY(obj) ((GtkEntry *)(obj))
#define GTK_SPIN_BUTTON(obj) ((GtkSpinButton *)(obj))
#define GTK_TEXT(obj) ((GtkText *)(obj))
#define GTK_WINDOW(obj) ((GtkWindow *)(obj))
#define GTK_BUTTON(obj) ((GtkButton *)(obj))
#define GTK_TOGGLE_BUTTON(obj) ((GtkToggleButton *)(obj))
#define GTK_RADIO_BUTTON(obj) ((GtkRadioButton *)(obj))
#define GTK_CHECK_BUTTON(obj) ((GtkCheckButton *)(obj))
#define GTK_LABEL(obj) ((GtkLabel *)(obj))
#define GTK_TABLE(obj) ((GtkTable *)(obj))
#define GTK_MENU_SHELL(obj) ((GtkMenuShell *)(obj))
#define GTK_MENU_BAR(obj) ((GtkMenuBar *)(obj))
#define GTK_MENU_ITEM(obj) ((GtkMenuItem *)(obj))
#define GTK_MENU(obj) ((GtkMenu *)(obj))
#define GTK_SIGNAL_FUNC(f) ((GtkSignalFunc) f)

#define GTK_OBJECT_FLAGS(obj) (GTK_OBJECT(obj)->flags)
#define GTK_WIDGET_FLAGS(wid) (GTK_OBJECT_FLAGS(wid))
#define GTK_WIDGET_REALIZED(wid) ((GTK_WIDGET_FLAGS(wid)&GTK_REALIZED) != 0)
#define GTK_WIDGET_VISIBLE(wid) ((GTK_WIDGET_FLAGS(wid)&GTK_VISIBLE) != 0)
#define GTK_WIDGET_SENSITIVE(wid) ((GTK_WIDGET_FLAGS(wid)&GTK_SENSITIVE) != 0)
#define GTK_WIDGET_SET_FLAGS(wid,flag) (GTK_WIDGET_FLAGS(wid) |= (flag))
#define GTK_WIDGET_UNSET_FLAGS(wid,flag) (GTK_WIDGET_FLAGS(wid) &= ~(flag))

typedef int GdkEvent;

void gtk_widget_show(GtkWidget *widget);
void gtk_widget_show_all(GtkWidget *widget);
void gtk_widget_hide(GtkWidget *widget);
void gtk_widget_destroy(GtkWidget *widget);
void gtk_widget_realize(GtkWidget *widget);
void gtk_widget_set_sensitive(GtkWidget *widget,gboolean sensitive);
void gtk_widget_size_request(GtkWidget *widget,GtkRequisition *requisition);
void gtk_widget_set_size(GtkWidget *widget,GtkAllocation *allocation);
GtkWidget *gtk_widget_get_ancestor(GtkWidget *widget,GtkType type);
GtkWidget *gtk_window_new(GtkWindowType type);
void gtk_window_set_title(GtkWindow *window,const gchar *title);
void gtk_window_set_default_size(GtkWindow *window,gint width,gint height);
void gtk_window_set_transient_for(GtkWindow *window,GtkWindow *parent);
void gtk_container_add(GtkContainer *container,GtkWidget *widget);
void gtk_container_set_border_width(GtkContainer *container,guint border_width);
GtkWidget *gtk_button_new_with_label(const gchar *label);
GtkWidget *gtk_label_new(const gchar *text);
GtkWidget *gtk_hbox_new(gboolean homogeneous,gint spacing);
GtkWidget *gtk_vbox_new(gboolean homogeneous,gint spacing);
GtkWidget *gtk_check_button_new_with_label(const gchar *label);
GtkWidget *gtk_radio_button_new_with_label(GSList *group,const gchar *label);
GtkWidget *gtk_radio_button_new_with_label_from_widget(GtkRadioButton *group,
                                                       const gchar *label);
GtkWidget *gtk_frame_new(const gchar *text);
GtkWidget *gtk_text_new(GtkAdjustment *hadj,GtkAdjustment *vadj);
GtkWidget *gtk_scrolled_text_new(GtkAdjustment *hadj,GtkAdjustment *vadj,
                                 GtkWidget **pack_widg);
GtkWidget *gtk_entry_new();
GtkWidget *gtk_table_new(guint rows,guint cols,gboolean homogeneous);
GtkItemFactory *gtk_item_factory_new(GtkType container_type,
                                     const gchar *path,
                                     GtkAccelGroup *accel_group);
void gtk_item_factory_create_item(GtkItemFactory *ifactory,
                                  GtkItemFactoryEntry *entry,
                                  gpointer callback_data,guint callback_type);
void gtk_item_factory_create_items(GtkItemFactory *ifactory,guint n_entries,
                                   GtkItemFactoryEntry *entries,
                                   gpointer callback_data);
GtkWidget *gtk_item_factory_get_widget(GtkItemFactory *ifactory,
                                       const gchar *path);
GtkWidget *gtk_clist_new(gint columns);
GtkWidget *gtk_clist_new_with_titles(gint columns,gchar *titles[]);
gint gtk_clist_append(GtkCList *clist,gchar *text[]);
void gtk_clist_set_column_title(GtkCList *clist,gint column,const gchar *title);
gint gtk_clist_insert(GtkCList *clist,gint row,gchar *text[]);
void gtk_clist_set_column_width(GtkCList *clist,gint column,gint width);
void gtk_clist_column_title_passive(GtkCList *clist,gint column);
void gtk_clist_column_titles_passive(GtkCList *clist);
void gtk_clist_set_selection_mode(GtkCList *clist,GtkSelectionMode mode);
void gtk_clist_sort(GtkCList *clist);
void gtk_clist_freeze(GtkCList *clist);
void gtk_clist_thaw(GtkCList *clist);
void gtk_clist_clear(GtkCList *clist);
GSList *gtk_radio_button_group(GtkRadioButton *radio_button);
void gtk_editable_insert_text(GtkEditable *editable,const gchar *new_text,
                              gint new_text_length,gint *position);
void gtk_editable_delete_text(GtkEditable *editable,
                              gint start_pos,gint end_pos);
gchar *gtk_editable_get_chars(GtkEditable *editable,
                              gint start_pos,gint end_pos);
void gtk_editable_set_editable(GtkEditable *editable,gboolean is_editable);
void gtk_editable_set_position(GtkEditable *editable,gint position);
gint gtk_editable_get_position(GtkEditable *editable);
guint gtk_text_get_length(GtkText *text);
void gtk_text_set_editable(GtkText *text,gboolean is_editable);
void gtk_text_set_word_wrap(GtkText *text,gboolean word_wrap);
void gtk_text_freeze(GtkText *text);
void gtk_text_thaw(GtkText *text);
void gtk_table_attach(GtkTable *table,GtkWidget *widget,
                      guint left_attach,guint right_attach,
                      guint top_attach,guint bottom_attach,
                      GtkAttachOptions xoptions,GtkAttachOptions yoptions,
                      guint xpadding,guint ypadding);
void gtk_table_attach_defaults(GtkTable *table,GtkWidget *widget,
                               guint left_attach,guint right_attach,
                               guint top_attach,guint bottom_attach);
void gtk_table_set_row_spacing(GtkTable *table,guint row,guint spacing);
void gtk_table_set_col_spacing(GtkTable *table,guint column,guint spacing);
void gtk_table_set_row_spacings(GtkTable *table,guint spacing);
void gtk_table_set_col_spacings(GtkTable *table,guint spacing);
void gtk_box_pack_start(GtkBox *box,GtkWidget *child,gboolean Expand,
                        gboolean Fill,gint Padding);
void gtk_toggle_button_toggled(GtkToggleButton *toggle_button);
gboolean gtk_toggle_button_get_active(GtkToggleButton *toggle_button);
void gtk_toggle_button_set_active(GtkToggleButton *toggle_button,
                                  gboolean is_active);
void gtk_main_quit();
void gtk_main();
guint gtk_signal_connect(GtkObject *object,const gchar *name,
                         GtkSignalFunc func,gpointer func_data);
guint gtk_signal_connect_object(GtkObject *object,const gchar *name,
                                GtkSignalFunc func,GtkObject *slot_object);
void gtk_signal_emit(GtkObject *object,const gchar *name, ...);
void win32_init(HINSTANCE hInstance,HINSTANCE hPrevInstance);
void gtk_menu_shell_insert(GtkMenuShell *menu_shell,GtkWidget *child,
                           gint position);
void gtk_menu_shell_append(GtkMenuShell *menu_shell,GtkWidget *child);
void gtk_menu_shell_prepend(GtkMenuShell *menu_shell,GtkWidget *child);
GtkWidget *gtk_menu_bar_new();
void gtk_menu_bar_insert(GtkMenuBar *menu_bar,GtkWidget *child,gint position);
void gtk_menu_bar_append(GtkMenuBar *menu_bar,GtkWidget *child);
void gtk_menu_bar_prepend(GtkMenuBar *menu_bar,GtkWidget *child);
GtkWidget *gtk_menu_new();
void gtk_menu_insert(GtkMenu *menu,GtkWidget *child,gint position);
void gtk_menu_append(GtkMenu *menu,GtkWidget *child);
void gtk_menu_prepend(GtkMenu *menu,GtkWidget *child);
GtkWidget *gtk_menu_item_new_with_label(const gchar *label);
void gtk_menu_item_set_submenu(GtkMenuItem *menu_item,GtkWidget *submenu);
GtkWidget *gtk_notebook_new();
void gtk_notebook_append_page(GtkNotebook *notebook,GtkWidget *child,
                              GtkWidget *tab_label);
void gtk_notebook_insert_page(GtkNotebook *notebook,GtkWidget *child,
                              GtkWidget *tab_label,gint position);
void gtk_notebook_set_page(GtkNotebook *notebook,gint page_num);
GtkObject *gtk_adjustment_new(gfloat value,gfloat lower,gfloat upper,
                              gfloat step_increment,gfloat page_increment,
                              gfloat page_size);
GtkWidget *gtk_spin_button_new(GtkAdjustment *adjustment,gfloat climb_rate,
                               guint digits);
void gdk_input_remove(gint tag);
gint gdk_input_add(gint source,GdkInputCondition condition,
                   GdkInputFunction function,gpointer data);
GtkWidget *gtk_hseparator_new();
GtkWidget *gtk_vseparator_new();
void gtk_object_set_data(GtkObject *object,const gchar *key,gpointer data);
gpointer gtk_object_get_data(GtkObject *object,const gchar *key);
GtkAccelGroup *gtk_accel_group_new();
void gtk_item_factory_set_translate_func(GtkItemFactory *ifactory,
                                         GtkTranslateFunc func,
                                         gpointer data,
                                         GtkDestroyNotify notify);
void gtk_widget_grab_default(GtkWidget *widget);
void gtk_widget_grab_focus(GtkWidget *widget);
void gtk_window_set_modal(GtkWindow *window,gboolean modal);
void gtk_window_add_accel_group(GtkWindow *window,GtkAccelGroup *accel_group);
void gtk_entry_set_text(GtkEntry *entry,const gchar *text);
void gtk_widget_add_accelerator(GtkWidget *widget,
                                const gchar *accel_signal,
                                GtkAccelGroup *accel_group,
                                guint accel_key,guint accel_mods);
void gtk_widget_remove_accelerator(GtkWidget *widget,
                                   GtkAccelGroup *accel_group,
                                   guint accel_key,guint accel_mods);
guint SetAccelerator(GtkWidget *labelparent,gchar *Text,
                     GtkWidget *sendto,gchar *signal,
                     GtkAccelGroup *accel_group);

extern const GtkType GTK_TYPE_WINDOW,GTK_TYPE_MENU_BAR;
GtkWidget *gtk_vpaned_new();
GtkWidget *gtk_hpaned_new();
void gtk_paned_add1(GtkPaned *paned,GtkWidget *child);
void gtk_paned_add2(GtkPaned *paned,GtkWidget *child);
void gtk_paned_pack1(GtkPaned *paned,GtkWidget *child,gboolean resize,
                     gboolean shrink);
void gtk_paned_pack2(GtkPaned *paned,GtkWidget *child,gboolean resize,
                     gboolean shrink);

#define gtk_container_border_width gtk_container_set_border_width
#define gtk_hbutton_box_new() gtk_hbox_new(TRUE,5)
#define gtk_vbutton_box_new() gtk_vbox_new(TRUE,5)

#endif
