#include <stdbool.h>
#define MAX_LAYERS 25

enum key_type;
enum key_modifier_type;
struct key;
struct layout;
struct kbd;

enum key_type {
	Pad = 0, // Padding, not a pressable key
	Code,    // A normal key emitting a keycode
	Mod,     // A modifier key
	Copy,    // Copy key, copies the unicode value specified in code (creates and
	         // activates temporary keymap)
	         // used for keys that are not part of the keymap
	Layout,  // Layout switch to a specific layout
	BackLayer, // Layout switch to the layout that was previously active
	NextLayer, // Layout switch to the next layout in the layers sequence
	Compose,   // Compose modifier key, switches to a specific associated layout
	           // upon next keypress
	EndRow,    // Incidates the end of a key row
	Last,      // Indicated the end of a layout
};

/* Modifiers passed to the virtual_keyboard protocol. They are based on
 * wayland's wl_keyboard, which doesn't document them.
 */
enum key_modifier_type {
	NoMod = 0,
	Shift = 1,
	CapsLock = 2,
	Ctrl = 4,
	Alt = 8,
	Super = 64,
	AltGr = 128,
};

struct key {
	const char *label;       // primary label
	const char *shift_label; // secondary label
	const double width;      // relative width (1.0)
	const enum key_type type;

	const uint32_t
	  code;                  /* code: key scancode or modifier name (see
	                          *   `/usr/include/linux/input-event-codes.h` for scancode names, and
	                          *   `keyboard.h` for modifiers)
	                          *   XKB keycodes are +8 */
	struct layout *layout;   // pointer back to the parent layout that holds this
	                         // key
	const uint32_t code_mod; /* modifier to force when this key is pressed */
	uint8_t scheme;          // index of the scheme to use
	bool reset_mod;          /* reset modifiers when clicked */

	// actual coordinates on the surface (pixels), will be computed automatically
	// for all keys
	uint32_t x, y, w, h;
};

struct layout {
	struct key *keys;
	const char *keymap_name;
	const char *name;
	bool abc; //is this an alphabetical/abjad layout or not? (i.e. something that is a primary input layout)
	uint32_t keyheight; // absolute height (pixels)
};
