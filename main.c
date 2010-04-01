/* 
    Zerp: a Z-machine interpreter
    main.c : glk_main entrypoint - load gamefile and print banner
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <libgen.h>

#ifdef TARGET_OS_MAC
#include <GlkClient/glk.h>
#else
#include "glk.h"
#endif /* TARGET_OS_MAC */
#include "zerp.h"

/* functions */
static void show_banner();

/* Z-code file mmap */
frefid_t zGamefileRef = 0;
int zFilesize = 0;
char * zFilename = 0;
int zGameVersion = 0;
unsigned char * zGamefile;
unsigned char * zMachine;

/* The story, upper and status windows. */
winid_t mainwin = NULL;
winid_t statuswin = NULL;
winid_t upperwin = NULL;

void glk_main(void)
{
    strid_t      file;
    struct stat  info;
    char         errbuff[SMALLBUFF];
  

	/* TODO: Put the window init code somewhere else */
	glk_stylehint_set(wintype_TextGrid, style_Alert, stylehint_ReverseColor, 1);
    mainwin = glk_window_open(0, 0, 0, wintype_TextBuffer, 1);
    if (!mainwin) {
        return; 
    }

    glk_set_window(mainwin);

    if (!zGamefileRef) {
      glk_put_string("No gamefile. Usage: zerp gamefile\n");
      return;
    }
  
    file = glk_stream_open_file(zGamefileRef, filemode_Read, 0);
    if (file == NULL)
        goto error;

		glk_stream_set_position(file, 0, seekmode_End);
		zFilesize = glk_stream_get_position(file);
	    if (zFilesize < 64) {
	        glk_put_string("This is too small to be a z-code file.\n");
	        return;
	    }

		glk_stream_set_position(file, 0, seekmode_Start);
		zGamefile = malloc(zFilesize);
	    zMachine = malloc(zFilesize);
	    if (!zMachine || !zGamefile)
	        goto error;
		glk_get_buffer_stream(file, (char*)zGamefile, zFilesize);
    
    /*
        Gamefile is kept pristine for save compression/restarts. Here we make a copy
        that we can write to.
    */
    memcpy(zMachine, zGamefile, zFilesize);    

    // zGamefile = mmap (NULL, info.st_size, PROT_READ, MAP_PRIVATE, file, 0);
    // if (zGamefile == MAP_FAILED)
    //     goto error;

    
    // show_banner();
	zGameVersion = get_byte(0);
    switch (zGameVersion) {
			case Z_VERSION_1:
			case Z_VERSION_2:
			case Z_VERSION_6:
			case Z_VERSION_7:
		        glk_printf("Unsupported version: this file needs version %d.", zGameVersion);
		        return;
				break;
			default:
				break;
    }

	/* Provide status line for V3 */
	if (zGameVersion < Z_VERSION_4) {
		statuswin = glk_window_open(mainwin, winmethod_Above | winmethod_Fixed, 1, wintype_TextGrid, 0);
		set_screen_width(statuswin);
	}

    zerp_run();
    
    free(zMachine);
    // munmap ((void*) zGamefile, info.st_size);
    return;

    error:
        strerror_r(errno, errbuff, SMALLBUFF);
        glk_printf("zerp: %s", errbuff);
        return;
}

static void show_banner() {
    glk_put_string("zerp\nA Z-machine interpreter using GLK\n");
    glk_put_string("By Ian Webb\n");
  
    glk_put_string("Gamefile loaded: ");
    
    glk_printf("%s, %d bytes.\n\n\n", basename(zFilename), zFilesize);
}

void show_status_line() {
	glui32 width, height;
	zbyte_t room_object;
	int len, i;
	char ampm[3], score[16];

    if (!statuswin)
        return;

    glk_set_window(statuswin);
    glk_window_clear(statuswin);
	glk_set_style(style_Alert);

    glk_window_get_size(statuswin, &width, &height);
	for (i = 0; i < width; i++)
		glk_put_string(" ");
	glk_window_move_cursor(statuswin, 0, 0);

	if (room_object = variable_get(0x10));
		print_object_name(room_object);

	if (get_byte(FLAGS_1) & 0x2) {
		if (variable_get(0x11) < 12) {
			sprintf(ampm, "AM");
		} else {
			sprintf(ampm, "PM");
		}
		sprintf(score, "%d:%02d%s", variable_get(0x11) % 12, variable_get(0x12), ampm);
	} else {
		sprintf(score, "%d/%d", variable_get(0x11), variable_get(0x12));
	}

	for (len = 0; score[len] != '\0'; len++);
	glk_window_move_cursor(statuswin, width - len, 0);
	glk_put_string(score);

	glk_set_window(mainwin);

	return;
}


/* Note the screen size (this is usually the size of a textgrid window) */
void set_screen_width(winid_t win) {
	int columns, lines;

	if (!win)
		return;

	glk_window_get_size(win, &columns, &lines);
	store_byte(SCREEN_WIDTH, columns);
}

int glk_printf(char *format, ...) {
    va_list ap;
    char    buf[SMALLBUFF];
    int     res;
    
    va_start(ap, format);
    res = vsnprintf(buf, SMALLBUFF, format, ap);
    if (res >= 0)
        glk_put_string(buf);
    va_end(ap);
    return res;
}

void fatal_error(char *message) {
    glk_printf("\nFATAL ERROR: %#04x : %s\n", zPC, message);
    
    /* free any space we might have alloc'd */
    if (zStack)
        free(zStack);
    if (zCallStack)
        free(zCallStack);
    if (zMachine)
        free(zMachine);
        
    /* And.... done. */    
    glk_exit();
}