/*
 * Copyright (C) 2021 Damir Porobic <damir.porobic@gmx.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KSNIP_GNOMEX11WRAPPER_H
#define KSNIP_GNOMEX11WRAPPER_H

#include "X11Wrapper.h"

// GNOME-specific X11 behaviour previously lived here (_GTK_FRAME_EXTENTS).
// Frame-extent handling is now centralized in X11Wrapper / BaseX11ImageGrabber
// and controlled by the includeWindowDecoration setting.
class GnomeX11Wrapper : public X11Wrapper
{
public:
	GnomeX11Wrapper() = default;
	~GnomeX11Wrapper() = default;
};

#endif //KSNIP_GNOMEX11WRAPPER_H
