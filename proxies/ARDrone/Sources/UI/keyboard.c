/**
 *  \brief    gamepad handling implementation
 *  \author   Sylvain Gaeremynck <sylvain.gaeremynck@parrot.fr>
 *  \version  1.0
 *  \date     04/06/2007
 *  \warning  Subject to completion
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include <linux/joystick.h>

#include <ardrone_api.h>
#include <VP_Os/vp_os_print.h>
#include "keyboard.h"

input_device_t keyboard = {
  "Keyboard",
  open_keyboard,
  update_keyboard,
  close_keyboard
};

//Linux Libraries
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

///////////////////////////////
//  Keyboard input functions  //
///////////////////////////////

C_RESULT open_keyboard(void)
{
	return C_OK;
}

C_RESULT update_keyboard(void)
{
	return C_OK;
}

C_RESULT close_keyboard(void)
{
	return C_OK;
}
