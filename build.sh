gcc -fPIC -shared -o keyboard_module.so keyboard_module.c `pkg-config --cflags --libs gtk+-3.0`

