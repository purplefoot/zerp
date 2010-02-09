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

#include "glk.h"
#include "zerp.h"

/* functions */
static void show_banner();

/* Z-code file mmap */
char * zFilename = 0;
int zFilesize = 0;

unsigned char * zGamefile;
unsigned char * zMachine;

/* The story and status windows. */
winid_t mainwin = NULL;
winid_t statuswin = NULL;

void glk_main(void)
{
    int          file;
    struct stat  info;
    char         errbuff[SMALLBUFF];
  
    mainwin = glk_window_open(0, 0, 0, wintype_TextBuffer, 1);
    if (!mainwin) {
        return; 
    }
    statuswin = glk_window_open(mainwin, winmethod_Above | winmethod_Fixed, 1, wintype_TextGrid, 0);

    glk_set_window(mainwin);

    if (!zFilename) {
      glk_put_string("No gamefile. Usage: zerp gamefile\n");
      return;
    }
  
    file = open (zFilename, O_RDONLY);
    if (file < 0)
        goto error;

    if (fstat (file, &info) != 0)
        goto error;

    if (info.st_size < 64)
    {
        glk_put_string("This is too small to be a z-code file.\n");
        return;
    }

    zGamefile = mmap (NULL, info.st_size, PROT_READ, MAP_PRIVATE, file, 0);
    if (zGamefile == MAP_FAILED)
        goto error;
    zFilesize = info.st_size;
    
    /*
        Gamefile is kept pristine for save compression/restarts. Here we make a copy
        that we can write to.
    */
    zMachine = malloc(zFilesize);
    if (!zMachine)
        goto error;
    memcpy(zMachine, zGamefile, zFilesize);    
    
    // show_banner();
    if (get_byte(0) != Z_VERSION_3) {
        glk_printf("Unsupported version: this file needs version %d.", get_word(0));
        return;
    }
    zerp_run();
    
    free(zMachine);
    munmap ((void*) zGamefile, info.st_size);
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
	int len;
	char ampm[3], score[16];

    if (!statuswin)
        return;

    glk_set_window(statuswin);
    glk_window_clear(statuswin);
	glk_set_style(style_Header);

    glk_window_get_size(statuswin, &width, &height);

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