#include <linux/input-event-codes.h>
#include "gtklock-module.h"
#include "keyboard.h"
#include "layout.mobintl.h"
#include <gtk/gtk.h>
#include <math.h>
#include <stdbool.h>

#define MODULE_DATA(x) (x->module_data[self_id])
#define VIRTUAL_KEYBOARD(x) ((struct keyboard *)MODULE_DATA(x))

extern char *xdg_get_config_path(const char *basename);
extern void config_load(const char *path, const char *group,
                        GOptionEntry entries[]);

// Module information
const gchar module_name[] = "virtual_keyboard";
const guint module_major_version = 4;
const guint module_minor_version = 0;

static int self_id; // Module's unique ID

struct keyboard {
  GtkWidget *revealer;
  GtkWidget *keyboard_grid;
  GtkWidget *layout_button;
  GtkWidget *mode_button;
  gboolean compose;
  gboolean shift;
  struct layout *current_layout;
};
static void setup_virtual_keyboard(struct Window *ctx);
static void create_keys(struct Window *ctx, struct layout *l);

static void gtk_grid_remove_all(GtkGrid *grid) {
  GList *children = gtk_container_get_children(GTK_CONTAINER(grid));
  while (children != NULL) {
    gtk_container_remove(GTK_CONTAINER(grid), children->data);
    children = g_list_next(children);
  }
}

static void key_pressed(GtkWidget *button, gpointer user_data) {
  struct Window *win = (struct Window *)user_data;
  struct key *key_data =
      (struct key *)g_object_get_data(G_OBJECT(button), "key_data");
  if (key_data->type == Code || key_data->type == Copy) {

    if (VIRTUAL_KEYBOARD(win)->compose) {
      VIRTUAL_KEYBOARD(win)->compose = FALSE;
      if (key_data->layout) {
        create_keys(user_data, key_data->layout);
      } else {
        create_keys(user_data, &layouts[Landscape]);
      }
    } else {
      if (key_data->code == KEY_BACKSPACE) {
        g_signal_emit_by_name((GTK_ENTRY(win->input_field)), "backspace");
      } else if (key_data->code == KEY_ENTER) {
        g_signal_emit_by_name((GTK_ENTRY(win->input_field)), "activate");
      } else if (key_data->code == KEY_TAB) {
        g_signal_emit_by_name((GTK_ENTRY(win->input_field)), "insert-at-cursor",
                              "\t");
      } else if (key_data->code == KEY_ESC) {
      } else if (key_data->code == KEY_UP) {
        g_signal_emit_by_name((GTK_ENTRY(win->input_field)), "move-cursor",
                              GTK_MOVEMENT_DISPLAY_LINES, -1, FALSE);
      } else if (key_data->code == KEY_DOWN) {
        g_signal_emit_by_name((GTK_ENTRY(win->input_field)), "move-cursor",
                              GTK_MOVEMENT_DISPLAY_LINES, 1, FALSE);
      } else if (key_data->code == KEY_LEFT) {
        g_signal_emit_by_name((GTK_ENTRY(win->input_field)), "move-cursor",
                              GTK_MOVEMENT_LOGICAL_POSITIONS, -1, FALSE);
      } else if (key_data->code == KEY_RIGHT) {
        g_signal_emit_by_name((GTK_ENTRY(win->input_field)), "move-cursor",
                              GTK_MOVEMENT_LOGICAL_POSITIONS, 1, FALSE);
      } else if (key_data->code == KEY_HOME) {
        g_signal_emit_by_name((GTK_ENTRY(win->input_field)), "move-cursor",
                              GTK_MOVEMENT_DISPLAY_LINE_ENDS, -1, FALSE);
      } else if (key_data->code == KEY_END) {
        g_signal_emit_by_name((GTK_ENTRY(win->input_field)), "move-cursor",
                              GTK_MOVEMENT_DISPLAY_LINE_ENDS, 1, FALSE);
      } else if (key_data->code == KEY_PAGEUP) {
        g_signal_emit_by_name((GTK_ENTRY(win->input_field)), "move-cursor",
                              GTK_MOVEMENT_PAGES, -1, FALSE);
      } else if (key_data->code == KEY_PAGEDOWN) {
        g_signal_emit_by_name((GTK_ENTRY(win->input_field)), "move-cursor",
                              GTK_MOVEMENT_PAGES, 1, FALSE);
      } else if (VIRTUAL_KEYBOARD(win)->shift) {
        VIRTUAL_KEYBOARD(win)->shift = false;
        if (key_data->code == KEY_SPACE) {
          g_signal_emit_by_name((GTK_ENTRY(win->input_field)),
                                "insert-at-cursor", "\t");
        } else {
          g_signal_emit_by_name((GTK_ENTRY(win->input_field)),
                                "insert-at-cursor", key_data->shift_label);
        }
        create_keys(win, VIRTUAL_KEYBOARD(win)->current_layout);
      } else {
        g_signal_emit_by_name((GTK_ENTRY(win->input_field)), "insert-at-cursor",
                              key_data->label);
      }
    }
  } else if (key_data->type == BackLayer) {
    create_keys(user_data, &layouts[Landscape]);
  } else if (key_data->type == NextLayer) {
    create_keys(user_data, &layouts[LandscapeSpecial]);
  } else if (key_data->type == Layout) {
    create_keys(user_data, key_data->layout);
  } else if (key_data->type == Compose) {
    VIRTUAL_KEYBOARD(win)->compose =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

  } else if (key_data->type == Mod && key_data->code == Shift) {
    VIRTUAL_KEYBOARD(win)->shift =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
    create_keys(win, VIRTUAL_KEYBOARD(win)->current_layout);
  }
}

static void create_keys(struct Window *ctx, struct layout *l) {
  VIRTUAL_KEYBOARD(ctx)->current_layout = l;

  gtk_grid_remove_all(GTK_GRID(VIRTUAL_KEYBOARD(ctx)->keyboard_grid));

  int row = 0, col = 0;

  struct key *k = l->keys;
  while (k->type != Last) {
    if (k->type == EndRow) {
      row++;
      col = 0;
    } else if (k->type == Pad) {
      GtkWidget *pad = gtk_label_new(k->label);
      gtk_grid_attach(GTK_GRID(VIRTUAL_KEYBOARD(ctx)->keyboard_grid), pad, col,
                      row, round(k->width * 4), 1);
      col += (int)(k->width * 4);
    } else {
      GtkWidget *button;
      if (k->type == Compose || k->type == Mod) {
        button = gtk_toggle_button_new_with_label(k->label);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), false);
        if (k->code == Shift) {
          gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                                       VIRTUAL_KEYBOARD(ctx)->shift);
        } else if (k->type == Mod) {
          gtk_widget_set_sensitive(button, false);
        }
      } else {
        const gchar *lbl = k->label;
        if (VIRTUAL_KEYBOARD(ctx)->shift) {
          lbl = k->shift_label;
        }
        button = gtk_button_new_with_label(lbl);
      }
      g_object_set_data(G_OBJECT(button), "key_data", k);
      g_signal_connect(button, "clicked", G_CALLBACK(key_pressed), ctx);
      gtk_widget_set_can_focus(button, false);
      gtk_grid_attach(GTK_GRID(VIRTUAL_KEYBOARD(ctx)->keyboard_grid), button,
                      col, row, (int)(k->width * 4), 1);
      col += round(k->width * 4);
    }
    k++;
  }

  gtk_widget_show_all(VIRTUAL_KEYBOARD(ctx)->keyboard_grid);
}

static void setup_virtual_keyboard(struct Window *ctx) {
  if (MODULE_DATA(ctx) != NULL) {
    gtk_widget_destroy(VIRTUAL_KEYBOARD(ctx)->revealer);
    g_free(MODULE_DATA(ctx));
    MODULE_DATA(ctx) = NULL;
  }

  MODULE_DATA(ctx) = g_malloc(sizeof(struct keyboard));

  VIRTUAL_KEYBOARD(ctx)->revealer = gtk_revealer_new();
  VIRTUAL_KEYBOARD(ctx)->shift = false;
  VIRTUAL_KEYBOARD(ctx)->compose = false;
  g_object_set(VIRTUAL_KEYBOARD(ctx)->revealer, "margin", 5, NULL);
  gtk_widget_set_halign(VIRTUAL_KEYBOARD(ctx)->revealer, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(VIRTUAL_KEYBOARD(ctx)->revealer, GTK_ALIGN_END);
  gtk_revealer_set_transition_type(
      GTK_REVEALER(VIRTUAL_KEYBOARD(ctx)->revealer),
      GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
  gtk_revealer_set_reveal_child(GTK_REVEALER(VIRTUAL_KEYBOARD(ctx)->revealer),
                                TRUE);

  gtk_overlay_add_overlay(GTK_OVERLAY(ctx->overlay),
                          VIRTUAL_KEYBOARD(ctx)->revealer);

  VIRTUAL_KEYBOARD(ctx)->keyboard_grid = gtk_grid_new();
  gtk_widget_set_name(VIRTUAL_KEYBOARD(ctx)->keyboard_grid, "kbgrid");
  gtk_grid_set_row_spacing(GTK_GRID(VIRTUAL_KEYBOARD(ctx)->keyboard_grid), 5);
  gtk_grid_set_column_spacing(GTK_GRID(VIRTUAL_KEYBOARD(ctx)->keyboard_grid),
                              5);

  create_keys(ctx, &layouts[Landscape]);

  gtk_container_add(GTK_CONTAINER(VIRTUAL_KEYBOARD(ctx)->revealer),
                    VIRTUAL_KEYBOARD(ctx)->keyboard_grid);
  gtk_widget_show_all(VIRTUAL_KEYBOARD(ctx)->revealer);
}

// Handle the activation of the module
void on_activation(struct GtkLock *gtklock, int id) {
  self_id = id;

  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(provider,
                                  "#kbgrid button {"
                                  "   font-size: 200%;"
                                  "}"
                                  "#kbgrid button label {"
                                  "   padding: 0.25em;"
                                  "}",
                                  -1, NULL);

  GdkDisplay *display = gdk_display_get_default();
  GdkScreen *screen = gdk_display_get_default_screen(display);
  gtk_style_context_add_provider_for_screen(
      screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
}

// Handle focus change to setup the keyboard for the newly focused window
void on_focus_change(struct GtkLock *gtklock, struct Window *win,
                     struct Window *old) {
  setup_virtual_keyboard(win);
}

// Handle window destruction and cleanup the associated keyboard
void on_window_destroy(struct GtkLock *gtklock, struct Window *ctx) {
  if (MODULE_DATA(ctx) != NULL) {
    gtk_widget_destroy(VIRTUAL_KEYBOARD(ctx)->revealer);
    g_free(MODULE_DATA(ctx));
    MODULE_DATA(ctx) = NULL;
  }
}

// Handle idle hide event to hide the keyboard
void on_idle_hide(struct GtkLock *gtklock) {
  if (gtklock->focused_window) {
    gtk_revealer_set_reveal_child(
        GTK_REVEALER(VIRTUAL_KEYBOARD(gtklock->focused_window)->revealer),
        FALSE);
  }
}

// Handle idle show event to show the keyboard
void on_idle_show(struct GtkLock *gtklock) {
  if (gtklock->focused_window) {
    gtk_revealer_set_reveal_child(
        GTK_REVEALER(VIRTUAL_KEYBOARD(gtklock->focused_window)->revealer),
        TRUE);
  }
}
