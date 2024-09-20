#include <gtk/gtk.h>
#include "gtklock-module.h"

#define MODULE_DATA(x) (x->module_data[self_id])
#define VIRTUAL_KEYBOARD(x) ((struct keyboard *)MODULE_DATA(x))

extern void config_load(const char *path, const char *group, GOptionEntry entries[]);

struct keyboard {
    GtkWidget *revealer;
    GtkWidget *keyboard_grid;
};

// Module information
const gchar module_name[] = "virtual_keyboard";
const guint module_major_version = 3;
const guint module_minor_version = 0;

static int self_id;  // Module's unique ID

// Function to insert a character into the input field
static void insert_character(GtkWidget *button, gpointer user_data) {
    const gchar *character = gtk_button_get_label(GTK_BUTTON(button));
    struct Window *win = (struct Window *)user_data;
    
    g_signal_emit_by_name((GTK_ENTRY(win->input_field)), "insert-at-cursor", character);
}

// Function to insert a character into the input field
static void enter_clicked(GtkWidget *button, gpointer user_data) {
    struct Window *win = (struct Window *)user_data;

    g_signal_emit_by_name((GTK_ENTRY(win->input_field)), "activate");
}

// Function to handle backspace
static void backspace_character(GtkWidget *button, gpointer user_data) {
    struct Window *win = (struct Window *)user_data;

    g_signal_emit_by_name((GTK_ENTRY(win->input_field)), "backspace");    
}

// Create the virtual keyboard for the window, placed inside a GtkRevealer
static void setup_virtual_keyboard(struct Window *ctx) {
    if (MODULE_DATA(ctx) != NULL) {
        gtk_widget_destroy(VIRTUAL_KEYBOARD(ctx)->revealer);
        g_free(MODULE_DATA(ctx));
        MODULE_DATA(ctx) = NULL;
    }

    MODULE_DATA(ctx) = g_malloc(sizeof(struct keyboard));

    // Create the revealer for smooth show/hide transitions
    VIRTUAL_KEYBOARD(ctx)->revealer = gtk_revealer_new();
    g_object_set(VIRTUAL_KEYBOARD(ctx)->revealer, "margin", 5, NULL);
    gtk_widget_set_name(VIRTUAL_KEYBOARD(ctx)->revealer, "keyboard-revealer");
    gtk_widget_set_halign(VIRTUAL_KEYBOARD(ctx)->revealer, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(VIRTUAL_KEYBOARD(ctx)->revealer, GTK_ALIGN_END);
    gtk_revealer_set_transition_type(GTK_REVEALER(VIRTUAL_KEYBOARD(ctx)->revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
    gtk_revealer_set_reveal_child(GTK_REVEALER(VIRTUAL_KEYBOARD(ctx)->revealer), TRUE);
    gtk_overlay_add_overlay(GTK_OVERLAY(ctx->overlay), VIRTUAL_KEYBOARD(ctx)->revealer);

    // Create the keyboard grid
    VIRTUAL_KEYBOARD(ctx)->keyboard_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(VIRTUAL_KEYBOARD(ctx)->keyboard_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(VIRTUAL_KEYBOARD(ctx)->keyboard_grid), 5);
    
    const gchar *keys[] = {
        "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
        "q", "w", "e", "r", "t", "y", "u", "i", "o", "p",
        "a", "s", "d", "f", "g", "h", "j", "k", "l", "Enter",
        "z", "x", "c", "v", "b", "n", "m", "Backspace"
    };

    int row = 0, col = 0;
    for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        GtkWidget *button = gtk_button_new_with_label(keys[i]);

        if (g_strcmp0(keys[i], "Backspace") == 0) {
            g_signal_connect(button, "clicked", G_CALLBACK(backspace_character), ctx);
        } else if (g_strcmp0(keys[i], "Enter") == 0) {
			g_signal_connect(button, "clicked", G_CALLBACK(enter_clicked), ctx);
            // Implement enter key if needed (e.g., trigger password submission)
        } else {
            g_signal_connect(button, "clicked", G_CALLBACK(insert_character), ctx);
        }
        gtk_widget_set_can_focus(button, FALSE);

        gtk_grid_attach(GTK_GRID(VIRTUAL_KEYBOARD(ctx)->keyboard_grid), button, col, row, 1, 1);
        col++;

        if (col >= 10) {
            col = 0;
            row++;
        }
    }

    // Add the keyboard grid to the revealer
    gtk_container_add(GTK_CONTAINER(VIRTUAL_KEYBOARD(ctx)->revealer), VIRTUAL_KEYBOARD(ctx)->keyboard_grid);
    gtk_widget_show_all(VIRTUAL_KEYBOARD(ctx)->revealer);
}

// Handle the activation of the module
void on_activation(struct GtkLock *gtklock, int id) {
    self_id = id;
}

// Handle focus change to setup the keyboard for the newly focused window
void on_focus_change(struct GtkLock *gtklock, struct Window *win, struct Window *old) {
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
        gtk_revealer_set_reveal_child(GTK_REVEALER(VIRTUAL_KEYBOARD(gtklock->focused_window)->revealer), FALSE);
    }
}

// Handle idle show event to show the keyboard
void on_idle_show(struct GtkLock *gtklock) {
    if (gtklock->focused_window) {
        gtk_revealer_set_reveal_child(GTK_REVEALER(VIRTUAL_KEYBOARD(gtklock->focused_window)->revealer), TRUE);
    }
}

