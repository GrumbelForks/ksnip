/*
 * Copyright (C) 2017 Damir Porobic <https://github.com/damirporobic>
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

/*
 * blendCursorImage() and getNativeCursorPosition() functions have been taken
 * from KDE Spectacle X11ImageGrabber and slightly modified to fit this
 * implementation.
 */

#include "X11Wrapper.h"

#include <cstring>

#include <X11/Xlib.h>

bool X11Wrapper::isCompositorActive() const
{
	auto display = QX11Info::display();
	auto prop_atom = XInternAtom(display, "_NET_WM_CM_S0", false);
	return XGetSelectionOwner(display, prop_atom) != None;
}

QRect X11Wrapper::getFullScreenRect() const
{
	return getWindowRect(QX11Info::appRootWindow());
}

QRect X11Wrapper::getActiveWindowRect() const
{
	auto windowId = getActiveWindowId();
	return getWindowRect(windowId);
}

QRect X11Wrapper::getWindowRect(xcb_window_t windowId) const
{
	// In case of window id 0 return empty rect
	if (windowId == 0) {
		return {};
	}

	auto connection = QX11Info::connection();
	auto geomCookie = xcb_get_geometry_unchecked(connection, windowId);
	ScopedCPointer<xcb_get_geometry_reply_t> geomReply(xcb_get_geometry_reply(connection, geomCookie, nullptr));
	if (geomReply.isNull()) {
		return {};
	}

	// x/y from GetGeometry are relative to the parent; translate to root so the
	// rect is usable as a screen-space capture region for any window in the tree.
	auto translateCookie = xcb_translate_coordinates(connection, windowId, geomReply->root, 0, 0);
	ScopedCPointer<xcb_translate_coordinates_reply_t> translateReply(
		xcb_translate_coordinates_reply(connection, translateCookie, nullptr));
	if (translateReply.isNull()) {
		return { geomReply->x, geomReply->y, geomReply->width, geomReply->height };
	}

	return { translateReply->dst_x, translateReply->dst_y, geomReply->width, geomReply->height };
}

xcb_window_t X11Wrapper::getActiveWindowId() const
{
	xcb_query_tree_cookie_t treeCookie;
	auto connection = QX11Info::connection();
	ScopedCPointer<xcb_get_input_focus_reply_t> focusReply(xcb_get_input_focus_reply(connection, xcb_get_input_focus(connection), nullptr));

	// Get ID of focused window, however, this must not always be the top level
	// window so we loop through parents and search for the top level window.
	auto windowId = focusReply->focus;
	while (true) {
		treeCookie = xcb_query_tree_unchecked(connection, windowId);
		ScopedCPointer<xcb_query_tree_reply_t> treeReply(xcb_query_tree_reply(connection, treeCookie, nullptr));
		if (!treeReply) {
			return 0;
		}
		// If the root window it is equal to the parent or the window ID itself
		// the this must be the top level window.
		if (windowId == treeReply->root || treeReply->parent == treeReply->root) {
			return windowId;
		} else {
			windowId = treeReply->parent;
		}
	}
}

QPoint X11Wrapper::getNativeCursorPosition() const
{
	// QCursor::pos() is not used because it requires additional calculations.
	// Its value is the offset to the origin of the current screen is in
	// device-independent pixels while the origin itself uses native pixels.

	auto xcbConn = QX11Info::connection();
	auto pointerCookie = xcb_query_pointer_unchecked(xcbConn, QX11Info::appRootWindow());
	ScopedCPointer<xcb_query_pointer_reply_t> pointerReply(xcb_query_pointer_reply(xcbConn, pointerCookie, nullptr));

	return { pointerReply->root_x, pointerReply->root_y };
}

CursorDto X11Wrapper::getCursorWithPosition() const
{
	auto cursorPosition = getNativeCursorPosition();

	// now we can get the image and start processing
	auto xcbConn = QX11Info::connection();

	auto cursorCookie = xcb_xfixes_get_cursor_image_unchecked(xcbConn);
	ScopedCPointer<xcb_xfixes_get_cursor_image_reply_t> cursorReply(xcb_xfixes_get_cursor_image_reply(xcbConn, cursorCookie, nullptr));
	if (cursorReply.isNull()) {
		return CursorDto();
	}

	auto pixelData = xcb_xfixes_get_cursor_image_cursor_image(cursorReply.data());
	if (!pixelData) {
		return CursorDto();
	}

	// process the image into a QImage
	auto cursorImage = QImage((quint8 *) pixelData,
	                          cursorReply->width,
	                          cursorReply->height,
	                          QImage::Format_ARGB32_Premultiplied);

	// a small fix for the cursor position for fancier cursor
	cursorPosition -= QPoint(cursorReply->xhot, cursorReply->yhot);

	return { QPixmap::fromImage(cursorImage, Qt::AutoColor), cursorPosition };
}


xcb_window_t X11Wrapper::getInputFocusWindowId() const
{
	auto connection = QX11Info::connection();
	ScopedCPointer<xcb_get_input_focus_reply_t> focusReply(
		xcb_get_input_focus_reply(connection, xcb_get_input_focus(connection), nullptr));
	if (focusReply.isNull()) {
		return 0;
	}
	return focusReply->focus;
}

X11FrameExtents X11Wrapper::getActiveWindowFrameExtents() const
{
	auto connection = QX11Info::connection();
	auto focusWindow = getInputFocusWindowId();
	if (focusWindow == 0) {
		return {};
	}

	// Reparenting WMs (XFCE/xfwm4, MATE/marco, …) attach _NET_FRAME_EXTENTS to
	// the *client* window. getActiveWindowId() returns the WM frame (parent is
	// root), which usually has no such property — so walk from the focused
	// window upward and also probe the frame's children.
	xcb_window_t windowId = focusWindow;
	xcb_window_t topLevel = 0;
	while (windowId != 0) {
		auto extents = getFrameExtents(windowId);
		if (!extents.isNull()) {
			return extents;
		}

		auto treeCookie = xcb_query_tree_unchecked(connection, windowId);
		ScopedCPointer<xcb_query_tree_reply_t> treeReply(xcb_query_tree_reply(connection, treeCookie, nullptr));
		if (treeReply.isNull()) {
			break;
		}

		if (windowId == treeReply->root || treeReply->parent == treeReply->root) {
			topLevel = windowId;
			const auto children = xcb_query_tree_children(treeReply.data());
			const int childCount = xcb_query_tree_children_length(treeReply.data());
			for (int i = 0; i < childCount; ++i) {
				extents = getFrameExtents(children[i]);
				if (!extents.isNull()) {
					return extents;
				}
			}
			break;
		}
		windowId = treeReply->parent;
	}

	// Last resort for WMs that never set the EWMH property: derive extents from
	// the difference between the top-level frame and its first mapped child.
	if (topLevel != 0) {
		return guessFrameExtentsFromChild(topLevel);
	}
	return {};
}

X11FrameExtents X11Wrapper::guessFrameExtentsFromChild(xcb_window_t topLevelWindowId) const
{
	X11FrameExtents values{};
	auto connection = QX11Info::connection();

	auto treeCookie = xcb_query_tree_unchecked(connection, topLevelWindowId);
	ScopedCPointer<xcb_query_tree_reply_t> treeReply(xcb_query_tree_reply(connection, treeCookie, nullptr));
	if (treeReply.isNull()) {
		return values;
	}

	const auto children = xcb_query_tree_children(treeReply.data());
	const int childCount = xcb_query_tree_children_length(treeReply.data());
	if (childCount <= 0) {
		return values;
	}

	const auto frameRect = getWindowRect(topLevelWindowId);
	if (frameRect.isEmpty()) {
		return values;
	}

	// Prefer the largest child — usually the actual client area.
	QRect bestChild;
	for (int i = 0; i < childCount; ++i) {
		const auto childRect = getWindowRect(children[i]);
		if (childRect.isEmpty()) {
			continue;
		}
		if (bestChild.isEmpty() || childRect.width() * childRect.height() > bestChild.width() * bestChild.height()) {
			bestChild = childRect;
		}
	}

	if (bestChild.isEmpty() || !frameRect.contains(bestChild.center())) {
		return values;
	}

	values.left = bestChild.left() - frameRect.left();
	values.top = bestChild.top() - frameRect.top();
	values.right = frameRect.right() - bestChild.right();
	values.bottom = frameRect.bottom() - bestChild.bottom();

	// Sanity: decorations should be modest positive borders, not inverted.
	if (values.left < 0 || values.top < 0 || values.right < 0 || values.bottom < 0) {
		return {};
	}
	return values;
}

X11FrameExtents X11Wrapper::getFrameExtents(xcb_window_t windowId) const
{
	X11FrameExtents values{};
	if (windowId == 0) {
		return values;
	}

	auto connection = QX11Info::connection();

	auto readExtents = [&](const char *atomName) -> bool {
		auto atomCookie = xcb_intern_atom(connection, 0, strlen(atomName), atomName);
		ScopedCPointer<xcb_intern_atom_reply_t> atomReply(xcb_intern_atom_reply(connection, atomCookie, nullptr));
		if (atomReply.isNull()) {
			return false;
		}

		auto cookie = xcb_get_property(connection, 0, windowId, atomReply->atom, XCB_ATOM_CARDINAL, 0, 4);
		ScopedCPointer<xcb_get_property_reply_t> reply(xcb_get_property_reply(connection, cookie, nullptr));
		if (reply.isNull()) {
			return false;
		}

		const int length = xcb_get_property_value_length(reply.data());
		if (length < 16) { // 4 * uint32
			return false;
		}

		auto extents = static_cast<uint32_t*>(xcb_get_property_value(reply.data()));
		values.left = static_cast<int>(extents[0]);
		values.right = static_cast<int>(extents[1]);
		values.top = static_cast<int>(extents[2]);
		values.bottom = static_cast<int>(extents[3]);
		return !values.isNull();
	};

	// Prefer the EWMH standard; fall back to GTK's shadow/border extents.
	if (!readExtents("_NET_FRAME_EXTENTS")) {
		readExtents("_GTK_FRAME_EXTENTS");
	}

	return values;
}
