/* glkstart.c: Unix-specific startup code -- sample file.
    Designed by Andrew Plotkin <erkyrath@eblong.com>
    http://www.eblong.com/zarf/glk/index.html

    This is Unix startup code for the simplest possible kind of Glk
    program -- no command-line arguments; no startup files; no nothing.

    Remember, this is a sample file. You should copy it into the Glk
    program you are compiling, and modify it to your needs. This should
    *not* be compiled into the Glk library itself.
*/
#include <stdio.h>
#include <unistd.h>
#include "glk.h"
#include "glkstart.h"
#include "zerp.h"

glkunix_argumentlist_t glkunix_arguments[] = {
  { "", glkunix_arg_ValueFollows, "filename: The game file to load." },
  { NULL, glkunix_arg_End, NULL }
};

int glkunix_startup_code(glkunix_startup_t *data)
{
  if (data->argc > 1) {
	zFilename = data->argv[1];
    zGamefileRef = glk_fileref_create_by_name(fileusage_BinaryMode, zFilename, 0);
	
  } else {
	zGamefileRef = glk_fileref_create_by_prompt(fileusage_BinaryMode, filemode_Read, 0);
  }

  return TRUE;
}

