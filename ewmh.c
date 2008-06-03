/*
 * ewmh.c - EWMH support functions
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_aux.h>

#include "ewmh.h"
#include "tag.h"
#include "focus.h"
#include "screen.h"
#include "client.h"
#include "widget.h"
#include "titlebar.h"

extern awesome_t globalconf;

static xcb_atom_t net_supported;
static xcb_atom_t net_client_list;
static xcb_atom_t net_number_of_desktops;
static xcb_atom_t net_current_desktop;
static xcb_atom_t net_desktop_names;
static xcb_atom_t net_active_window;
static xcb_atom_t net_close_window;
static xcb_atom_t net_wm_name;
static xcb_atom_t net_wm_icon_name;
static xcb_atom_t net_wm_window_type;
static xcb_atom_t net_wm_window_type_normal;
static xcb_atom_t net_wm_window_type_dock;
static xcb_atom_t net_wm_window_type_splash;
static xcb_atom_t net_wm_window_type_dialog;
static xcb_atom_t net_wm_icon;
static xcb_atom_t net_wm_state;
static xcb_atom_t net_wm_state_sticky;
static xcb_atom_t net_wm_state_skip_taskbar;
static xcb_atom_t net_wm_state_fullscreen;
static xcb_atom_t net_wm_state_above;
static xcb_atom_t net_wm_state_below;

static xcb_atom_t utf8_string;

typedef struct
{
    const char *name;
    xcb_atom_t *atom;
} AtomItem;

static AtomItem AtomNames[] =
{
    { "_NET_SUPPORTED", &net_supported },
    { "_NET_CLIENT_LIST", &net_client_list },
    { "_NET_NUMBER_OF_DESKTOPS", &net_number_of_desktops },
    { "_NET_CURRENT_DESKTOP", &net_current_desktop },
    { "_NET_DESKTOP_NAMES", &net_desktop_names },
    { "_NET_ACTIVE_WINDOW", &net_active_window },

    { "_NET_CLOSE_WINDOW", &net_close_window },

    { "_NET_WM_NAME", &net_wm_name },
    { "_NET_WM_ICON_NAME", &net_wm_icon_name },
    { "_NET_WM_WINDOW_TYPE", &net_wm_window_type },
    { "_NET_WM_WINDOW_TYPE_NORMAL", &net_wm_window_type_normal },
    { "_NET_WM_WINDOW_TYPE_DOCK", &net_wm_window_type_dock },
    { "_NET_WM_WINDOW_TYPE_SPLASH", &net_wm_window_type_splash },
    { "_NET_WM_WINDOW_TYPE_DIALOG", &net_wm_window_type_dialog },
    { "_NET_WM_ICON", &net_wm_icon },
    { "_NET_WM_STATE", &net_wm_state },
    { "_NET_WM_STATE_STICKY", &net_wm_state_sticky },
    { "_NET_WM_STATE_SKIP_TASKBAR", &net_wm_state_skip_taskbar },
    { "_NET_WM_STATE_FULLSCREEN", &net_wm_state_fullscreen },
    { "_NET_WM_STATE_ABOVE", &net_wm_state_above },
    { "_NET_WM_STATE_BELOW", &net_wm_state_below },

    { "UTF8_STRING", &utf8_string },
};

#define ATOM_NUMBER (sizeof(AtomNames)/sizeof(AtomItem))

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

void
ewmh_init_atoms(void)
{
    unsigned int i;
    xcb_intern_atom_cookie_t cs[ATOM_NUMBER];
    xcb_intern_atom_reply_t *r;

    /*
     * Create the atom and get the reply in a XCB way (e.g. send all
     * the requests at the same time and then get the replies)
     */
    for(i = 0; i < ATOM_NUMBER; i++)
	cs[i] = xcb_intern_atom_unchecked(globalconf.connection,
					  false,
					  strlen(AtomNames[i].name),
					  AtomNames[i].name);

    for(i = 0; i < ATOM_NUMBER; i++)
    {
	if(!(r = xcb_intern_atom_reply(globalconf.connection, cs[i], NULL)))
	    /* An error occured, get reply for next atom */
	    continue;

	*AtomNames[i].atom = r->atom;
        p_delete(&r);
    }
}

void
ewmh_set_supported_hints(int phys_screen)
{
    xcb_atom_t atom[ATOM_NUMBER];
    int i = 0;

    atom[i++] = net_supported;
    atom[i++] = net_client_list;
    atom[i++] = net_number_of_desktops;
    atom[i++] = net_current_desktop;
    atom[i++] = net_desktop_names;
    atom[i++] = net_active_window;

    atom[i++] = net_close_window;

    atom[i++] = net_wm_name;
    atom[i++] = net_wm_icon_name;
    atom[i++] = net_wm_window_type;
    atom[i++] = net_wm_window_type_normal;
    atom[i++] = net_wm_window_type_dock;
    atom[i++] = net_wm_window_type_splash;
    atom[i++] = net_wm_window_type_dialog;
    atom[i++] = net_wm_icon;
    atom[i++] = net_wm_state;
    atom[i++] = net_wm_state_sticky;
    atom[i++] = net_wm_state_skip_taskbar;
    atom[i++] = net_wm_state_fullscreen;
    atom[i++] = net_wm_state_above;
    atom[i++] = net_wm_state_below;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			xcb_aux_get_screen(globalconf.connection, phys_screen)->root,
			net_supported, ATOM, 32, i, atom);
}

void
ewmh_update_net_client_list(int phys_screen)
{
    xcb_window_t *wins;
    client_t *c;
    int n = 0;

    for(c = globalconf.clients; c; c = c->next)
        n++;

    wins = p_new(xcb_window_t, n);

    for(n = 0, c = globalconf.clients; c; c = c->next, n++)
        if(c->phys_screen == phys_screen)
            wins[n] = c->win;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			xcb_aux_get_screen(globalconf.connection, phys_screen)->root,
			net_client_list, WINDOW, 32, n, wins);

    p_delete(&wins);
}

void
ewmh_update_net_numbers_of_desktop(int phys_screen)
{
    uint32_t count = 0;
    tag_t *tag;

    for(tag = globalconf.screens[phys_screen].tags; tag; tag = tag->next)
        count++;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			xcb_aux_get_screen(globalconf.connection, phys_screen)->root,
			net_number_of_desktops, CARDINAL, 32, 1, &count);
}

void
ewmh_update_net_current_desktop(int phys_screen)
{
    uint32_t count = 0;
    tag_t *tag, **curtags = tags_get_current(phys_screen);

    for(tag = globalconf.screens[phys_screen].tags; tag != curtags[0]; tag = tag->next)
        count++;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
                        xcb_aux_get_screen(globalconf.connection, phys_screen)->root,
                        net_current_desktop, CARDINAL, 32, 1, &count);

    p_delete(&curtags);
}

void
ewmh_update_net_desktop_names(int phys_screen)
{
    char buf[1024], *pos;
    ssize_t len, curr_size;
    tag_t *tag;

    pos = buf;
    len = 0;
    for(tag = globalconf.screens[phys_screen].tags; tag; tag = tag->next)
    {
        curr_size = a_strlen(tag->name);
        a_strcpy(pos, sizeof(buf), tag->name);
        pos += curr_size + 1;
        len += curr_size + 1;
    }

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			xcb_aux_get_screen(globalconf.connection, phys_screen)->root,
			net_desktop_names, utf8_string, 8, len, buf);
}

void
ewmh_update_net_active_window(int phys_screen)
{
    xcb_window_t win;
    client_t *sel = focus_get_current_client(phys_screen);

    win = sel ? sel->win : XCB_NONE;

    xcb_change_property(globalconf.connection, XCB_PROP_MODE_REPLACE,
			xcb_aux_get_screen(globalconf.connection, phys_screen)->root,
			net_active_window, WINDOW, 32, 1, &win);
}

static void
ewmh_process_state_atom(client_t *c, xcb_atom_t state, int set)
{
    const uint32_t raise_window_val = XCB_STACK_MODE_ABOVE;
    titlebar_t *titlebar;

    if(state == net_wm_state_sticky)
    {
        tag_t *tag;
        for(tag = globalconf.screens[c->screen].tags; tag; tag = tag->next)
            tag_client(c, tag);
    }
    else if(state == net_wm_state_skip_taskbar)
    {
        if(set == _NET_WM_STATE_REMOVE)
        {
            c->skiptb = false;
            c->skip = false;
        }
        else if(set == _NET_WM_STATE_ADD)
        {
            c->skiptb = true;
            c->skip = true;
        }
    }
    else if(state == net_wm_state_fullscreen)
    {
        area_t geometry = c->geometry;
        if(set == _NET_WM_STATE_REMOVE)
        {
            /* restore geometry */
            geometry = c->m_geometry;
            /* restore borders and titlebar */
            if((titlebar = titlebar_getbyclient(c))
               && (titlebar->position = titlebar->oldposition))
            {
                titlebar->position = titlebar->oldposition;
                xcb_map_window(globalconf.connection, titlebar->sw->window);
            }
            c->noborder = false;
            c->ismax = false;
            client_setborder(c, c->oldborder);
            client_setfloating(c, c->wasfloating, c->oldlayer);
        }
        else if(set == _NET_WM_STATE_ADD)
        {
            geometry = screen_get_area(c->screen, NULL, &globalconf.screens[c->screen].padding);
            /* save geometry */
            c->m_geometry = c->geometry;
            c->wasfloating = c->isfloating;
            /* disable titlebar and borders */
            if((titlebar = titlebar_getbyclient(c))
               && (titlebar->oldposition = titlebar->position))
            {
                xcb_unmap_window(globalconf.connection, titlebar->sw->window);
                titlebar->position = Off;
            }
            c->ismax = true;
            c->oldborder = c->border;
            client_setborder(c, 0);
            c->noborder = true;
            client_setfloating(c, true, LAYER_FULLSCREEN);
        }
        widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
        client_resize(c, geometry, false);
	xcb_configure_window(globalconf.connection, c->win, XCB_CONFIG_WINDOW_STACK_MODE, &raise_window_val);
    }
    else if(state == net_wm_state_above)
    {
        if(set == _NET_WM_STATE_REMOVE)
        {
            c->layer = c->oldlayer;
        }
        else if(set == _NET_WM_STATE_ADD)
        {
            c->oldlayer = c->layer;
            c->layer = LAYER_ABOVE;
        }
    }
    else if(state == net_wm_state_below)
    {
        if(set == _NET_WM_STATE_REMOVE)
        {
            c->layer = c->oldlayer;
        }
        else if(set == _NET_WM_STATE_ADD)
        {
            c->oldlayer = c->layer;
            c->layer = LAYER_BELOW;
        }
    }

}

static void
ewmh_process_window_type_atom(client_t *c, xcb_atom_t state)
{
    titlebar_t *titlebar;

    if(state == net_wm_window_type_normal)
    {
        /* do nothing. this is REALLY IMPORTANT */
    }
    else if(state == net_wm_window_type_dock
            || state == net_wm_window_type_splash)
    {
        c->skip = true;
        c->isfixed = true;
        if((titlebar = titlebar_getbyclient(c))
            && titlebar->position && titlebar->sw)
        {
            xcb_unmap_window(globalconf.connection, titlebar->sw->window);
            titlebar->position = Off;
        }
        client_setborder(c, 0);
        c->noborder = true;
        client_setfloating(c, true, LAYER_ABOVE);
    }
    else if (state == net_wm_window_type_dialog)
        client_setfloating(c, true, LAYER_ABOVE);
}

void
ewmh_process_client_message(xcb_client_message_event_t *ev)
{
    client_t *c;
    int screen;

    if(ev->type == net_current_desktop)
        for(screen = 0;
            screen < xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
            screen++)
        {
            if(ev->window == xcb_aux_get_screen(globalconf.connection, screen)->root)
                tag_view_only_byindex(screen, ev->data.data32[0]);
        }

    if(ev->type == net_close_window)
    {
        if((c = client_get_bywin(globalconf.clients, ev->window)))
           client_kill(c);
    }
    else if(ev->type == net_wm_state)
    {
        if((c = client_get_bywin(globalconf.clients, ev->window)))
        {
            ewmh_process_state_atom(c, (xcb_atom_t) ev->data.data32[1], ev->data.data32[0]);
            if(ev->data.data32[2])
                ewmh_process_state_atom(c, (xcb_atom_t) ev->data.data32[2],
                                        ev->data.data32[0]);
        }
    }
}

void
ewmh_check_client_hints(client_t *c)
{
    xcb_atom_t *state;
    void *data = NULL;
    int i;

    xcb_get_property_cookie_t c1, c2;
    xcb_get_property_reply_t *reply;

    /* Send the GetProperty requests which will be processed later */
    c1 = xcb_get_property_unchecked(globalconf.connection, false, c->win,
                                    net_wm_state, ATOM, 0, UINT32_MAX);

    c2 = xcb_get_property_unchecked(globalconf.connection, false, c->win,
                                    net_wm_window_type, ATOM, 0, UINT32_MAX);

    reply = xcb_get_property_reply(globalconf.connection, c1, NULL);
    if(reply && (data = xcb_get_property_value(reply)))
    {
        state = (xcb_atom_t *) data;
        for(i = 0; i < xcb_get_property_value_length(reply); i++)
            ewmh_process_state_atom(c, state[i], _NET_WM_STATE_ADD);
    }

    p_delete(&reply);

    reply = xcb_get_property_reply(globalconf.connection, c2, NULL);
    if(reply && (data = xcb_get_property_value(reply)))
    {
        state = (xcb_atom_t *) data;
        for(i = 0; i < xcb_get_property_value_length(reply); i++)
            ewmh_process_window_type_atom(c, state[i]);
    }

    p_delete(&reply);
}

NetWMIcon *
ewmh_get_window_icon(xcb_window_t w)
{
    double alpha;
    NetWMIcon *icon;
    int size, i;
    uint32_t *data;
    unsigned char *imgdata;
    xcb_get_property_reply_t *r;

    r = xcb_get_property_reply(globalconf.connection,
                               xcb_get_property_unchecked(globalconf.connection, false, w,
                                                          net_wm_icon, CARDINAL, 0, UINT32_MAX),
                               NULL);
    if(!r || r->type != CARDINAL || r->format != 32 || r->length < 2 ||
       !(data = (uint32_t *) xcb_get_property_value(r)))
    {
        p_delete(&r);
        return NULL;
    }

    icon = p_new(NetWMIcon, 1);

    icon->width = data[0];
    icon->height = data[1];
    size = icon->width * icon->height;

    if(!size)
    {
        p_delete(&icon);
        p_delete(&r);
        return NULL;
    }

    icon->image = p_new(unsigned char, size * 4);
    for(imgdata = icon->image, i = 2; i < size + 2; i++, imgdata += 4)
    {
        imgdata[3] = (data[i] >> 24) & 0xff;           /* A */
        alpha = imgdata[3] / 255.0;
        imgdata[2] = ((data[i] >> 16) & 0xff) * alpha; /* R */
        imgdata[1] = ((data[i] >>  8) & 0xff) * alpha; /* G */
        imgdata[0] = (data[i]         & 0xff) * alpha; /* B */
    }

    p_delete(&r);

    return icon;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
