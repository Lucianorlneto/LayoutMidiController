.SUFFIXES:
.SUFFIXES: .c .cpp

CC = gcc
GCC = g++

DBUS_PATH = -I/usr/lib/x86_64-linux-gnu/glib-2.0/include/ -Wall -Wextra

TESSERACT_PATH = 

INCLUDES = $(TESSERACT_PATH)

.c:
	$(CC) -I../include -I$(INCDIR) $(CFLAGS) $< $(GL_LIBS) -o $@ -lwkhtmltox

.cpp:
	$(GCC) -I../include -Wall -Wunused -std=c++11 $(INCLUDES) -O2 `pkg-config --cflags opencv` $< -o $@ -lwkhtmltox `pkg-config --libs opencv tesseract zbar rtmidi`


