/**
 * xrdp: A Remote Desktop Protocol server.
 *
 * Copyright (C) Jay Sorg 2004-2013
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * librdp main file
 */

#include "rdp.h"

/******************************************************************************/
/* return error */
int DEFAULT_CC
lib_mod_start(struct mod *mod, int w, int h, int bpp)
{
    DEBUG(("in lib_mod_start"));
    mod->width = w;
    mod->height = h;
    mod->rdp_bpp = bpp;
    mod->xrdp_bpp = bpp;
    mod->keylayout = 0x409;
    g_strncpy(mod->port, "3389", 255); /* default */
    DEBUG(("out lib_mod_start"));
    return 0;
}

/******************************************************************************/
/* return error */
int DEFAULT_CC
lib_mod_connect(struct mod *mod)
{
    DEBUG(("in lib_mod_connect"));
    /* clear screen */
    mod->server_begin_update(mod);
    mod->server_set_fgcolor(mod, 0);
    mod->server_fill_rect(mod, 0, 0, mod->width, mod->height);
    mod->server_end_update(mod);

    /* connect */
    if (rdp_rdp_connect(mod->rdp_layer, mod->ip, mod->port) == 0)
    {
        mod->sck = mod->rdp_layer->sec_layer->mcs_layer->iso_layer->tcp_layer->sck;
        g_tcp_set_non_blocking(mod->sck);
        g_tcp_set_no_delay(mod->sck);
        mod->sck_obj = g_create_wait_obj_from_socket(mod->sck, 0);
        DEBUG(("out lib_mod_connect"));
        return 0;
    }

    DEBUG(("out lib_mod_connect error"));
    return 1;
}

/******************************************************************************/
/* return error */
int DEFAULT_CC
lib_mod_event(struct mod *mod, int msg, long param1, long param2,
              long param3, long param4)
{
    struct stream *s;

    if (!mod->up_and_running)
    {
        return 0;
    }

    DEBUG(("in lib_mod_event"));
    make_stream(s);
    init_stream(s, 8192 * 2);

    switch (msg)
    {
        case 15:
            rdp_rdp_send_input(mod->rdp_layer, s, 0, RDP_INPUT_SCANCODE,
                               param4, param3, 0);
            break;
        case 16:
            rdp_rdp_send_input(mod->rdp_layer, s, 0, RDP_INPUT_SCANCODE,
                               param4, param3, 0);
            break;
        case 17:
            rdp_rdp_send_input(mod->rdp_layer, s, 0, RDP_INPUT_SYNCHRONIZE,
                               param4, param3, 0);
            break;
        case 100:
            rdp_rdp_send_input(mod->rdp_layer, s, 0, RDP_INPUT_MOUSE,
                               MOUSE_FLAG_MOVE, param1, param2);
            break;
        case 101:
            rdp_rdp_send_input(mod->rdp_layer, s, 0, RDP_INPUT_MOUSE,
                               MOUSE_FLAG_BUTTON1, param1, param2);
            break;
        case 102:
            rdp_rdp_send_input(mod->rdp_layer, s, 0, RDP_INPUT_MOUSE,
                               MOUSE_FLAG_BUTTON1 | MOUSE_FLAG_DOWN,
                               param1, param2);
            break;
        case 103:
            rdp_rdp_send_input(mod->rdp_layer, s, 0, RDP_INPUT_MOUSE,
                               MOUSE_FLAG_BUTTON2, param1, param2);
            break;
        case 104:
            rdp_rdp_send_input(mod->rdp_layer, s, 0, RDP_INPUT_MOUSE,
                               MOUSE_FLAG_BUTTON2 | MOUSE_FLAG_DOWN,
                               param1, param2);
            break;
        case 105:
            rdp_rdp_send_input(mod->rdp_layer, s, 0, RDP_INPUT_MOUSE,
                               MOUSE_FLAG_BUTTON3, param1, param2);
            break;
        case 106:
            rdp_rdp_send_input(mod->rdp_layer, s, 0, RDP_INPUT_MOUSE,
                               MOUSE_FLAG_BUTTON3 | MOUSE_FLAG_DOWN,
                               param1, param2);
            break;
        case 107:
            rdp_rdp_send_input(mod->rdp_layer, s, 0, RDP_INPUT_MOUSE,
                               MOUSE_FLAG_BUTTON4, param1, param2);
            break;
        case 108:
            rdp_rdp_send_input(mod->rdp_layer, s, 0, RDP_INPUT_MOUSE,
                               MOUSE_FLAG_BUTTON4 | MOUSE_FLAG_DOWN,
                               param1, param2);
            break;
        case 109:
            rdp_rdp_send_input(mod->rdp_layer, s, 0, RDP_INPUT_MOUSE,
                               MOUSE_FLAG_BUTTON5, param1, param2);
            break;
        case 110:
            rdp_rdp_send_input(mod->rdp_layer, s, 0, RDP_INPUT_MOUSE,
                               MOUSE_FLAG_BUTTON5 | MOUSE_FLAG_DOWN,
                               param1, param2);
            break;
        case 200:
            rdp_rdp_send_invalidate(mod->rdp_layer, s,
                                    (param1 >> 16) & 0xffff, param1 & 0xffff,
                                    (param2 >> 16) & 0xffff, param2 & 0xffff);
            break;
    }

    free_stream(s);
    DEBUG(("out lib_mod_event"));
    return 0;
}

/******************************************************************************/
/* return error */
int DEFAULT_CC
lib_mod_signal(struct mod *mod)
{
    int type;
    int cont;
    struct stream *s;

    DEBUG(("in lib_mod_signal"));

    if (mod->in_s == 0)
    {
        make_stream(mod->in_s);
    }

    s = mod->in_s;
    init_stream(s, 8192 * 2);
    cont = 1;

    while (cont)
    {
        type = 0;

        if (rdp_rdp_recv(mod->rdp_layer, s, &type) != 0)
        {
            DEBUG(("out lib_mod_signal error rdp_rdp_recv failed"));
            return 1;
        }

        DEBUG(("lib_mod_signal type %d", type));

        switch (type)
        {
            case RDP_PDU_DATA:
                rdp_rdp_process_data_pdu(mod->rdp_layer, s);
                break;
            case RDP_PDU_DEMAND_ACTIVE:
                rdp_rdp_process_demand_active(mod->rdp_layer, s);
                mod->up_and_running = 1;
                break;
            case RDP_PDU_DEACTIVATE:
                mod->up_and_running = 0;
                break;
            case RDP_PDU_REDIRECT:
                break;
            case 0:
                break;
            default:
                break;
        }

        cont = s->next_packet < s->end;
    }

    DEBUG(("out lib_mod_signal"));
    return 0;
}

/******************************************************************************/
/* return error */
int DEFAULT_CC
lib_mod_end(struct mod *mod)
{
    rdp_rdp_delete(mod->rdp_layer);
    mod->rdp_layer = 0;
    free_stream(mod->in_s);
    mod->in_s = 0;

    if (mod->sck_obj != 0)
    {
        g_delete_wait_obj_from_socket(mod->sck_obj);
        mod->sck_obj = 0;
    }

    if (mod->sck != 0)
    {
        g_tcp_close(mod->sck);
        mod->sck = 0;
    }

    return 0;
}

/******************************************************************************/
/* return error */
int DEFAULT_CC
lib_mod_set_param(struct mod *mod, char *name, char *value)
{
    if (g_strncasecmp(name, "ip", 255) == 0)
    {
        g_strncpy(mod->ip, value, 255);
    }
    else if (g_strncasecmp(name, "port", 255) == 0)
    {
        g_strncpy(mod->port, value, 255);
    }
    else if (g_strncasecmp(name, "username", 255) == 0)
    {
        g_strncpy(mod->username, value, 255);
    }
    else if (g_strncasecmp(name, "domain", 255) == 0)
    {
        g_strncpy(mod->domain, value, 255);
    }
    else if (g_strncasecmp(name, "password", 255) == 0)
    {
        g_strncpy(mod->password, value, 255);
    }
    else if (g_strncasecmp(name, "hostname", 255) == 0)
    {
        g_strncpy(mod->hostname, value, 255);
    }
    else if (g_strncasecmp(name, "keylayout", 255) == 0)
    {
        mod->keylayout = g_atoi(value);
    }

    return 0;
}

/******************************************************************************/
/* return error */
int DEFAULT_CC
lib_mod_get_wait_objs(struct mod *mod, tbus *read_objs, int *rcount,
                      tbus *write_objs, int *wcount, int *timeout)
{
    int i;

    i = *rcount;

    if (mod != 0)
    {
        if (mod->sck_obj != 0)
        {
            read_objs[i++] = mod->sck_obj;
        }
    }

    *rcount = i;
    return 0;
}

/******************************************************************************/
/* return error */
int DEFAULT_CC
lib_mod_check_wait_objs(struct mod *mod)
{
    int rv;

    rv = 0;

    if (mod != 0)
    {
        if (mod->sck_obj != 0)
        {
            if (g_is_wait_obj_set(mod->sck_obj))
            {
                rv = lib_mod_signal(mod);
            }
        }
    }

    return rv;
}

/******************************************************************************/
struct mod *EXPORT_CC
mod_init(void)
{
    struct mod *mod;

    DEBUG(("in mod_init"));
    mod = (struct mod *)g_malloc(sizeof(struct mod), 1);
    mod->size = sizeof(struct mod);
    mod->version = CURRENT_MOD_VER;
    mod->handle = (long)mod;
    mod->mod_connect = lib_mod_connect;
    mod->mod_start = lib_mod_start;
    mod->mod_event = lib_mod_event;
    mod->mod_signal = lib_mod_signal;
    mod->mod_end = lib_mod_end;
    mod->mod_set_param = lib_mod_set_param;
    mod->mod_get_wait_objs = lib_mod_get_wait_objs;
    mod->mod_check_wait_objs = lib_mod_check_wait_objs;
    mod->rdp_layer = rdp_rdp_create(mod);
    DEBUG(("out mod_init"));
    return mod;
}

/******************************************************************************/
int EXPORT_CC
mod_exit(struct mod *mod)
{
    DEBUG(("in mod_exit"));
    g_free(mod);
    DEBUG(("out mod_exit"));
    return 0;
}
