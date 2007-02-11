/* $Id: nunchuk.h 15 2007-01-09 01:19:31Z bja $ 
 *
 * Copyright (C) 2007, Joel Andersson <bja@kth.se>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
 
#ifndef _NUNCHUK_H_
#define _NUNCHUK_H_

#include "wiimote.h"

#define nunchuk_decode_byte(x)	(((x) ^ 0x17) + 0x17)

int nunchuk_init(wiimote_t *wiimote);
int nunchuk_update(wiimote_t *wiimote);
int nunchuk_free(wiimote_t *wiimote);
void nunchuk_decode(uint8_t *data, uint32_t size);
int nunchuk_enable(wiimote_t *wiimote, uint8_t enable);

#endif /* _NUNCHUK_H_ */
