/*
 * awesome-client.c - awesome client, communicate with socket
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
 * Copyright © 2007 daniel@brinkers.de
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

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "awesome-client.h"
#include "util.h"

/* GNU/Hurd workaround */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

int
main(void)
{
    struct sockaddr_un *addr;
    char buf[1024], *msg;
    int csfd, ret_value = EXIT_SUCCESS;
    ssize_t len, msg_len = 1;

    csfd = get_client_socket();
    addr = get_client_addr(getenv("DISPLAY"));

    if(!addr || csfd < 0)
        return EXIT_FAILURE;

    msg = p_new(char, 1);
    while(fgets(buf, sizeof(buf), stdin))
    {
        len = a_strlen(buf);
        msg_len += len;
        p_realloc(&msg, msg_len);
        a_strncat(msg, msg_len, buf, len);
    }

    if(sendto(csfd, buf, a_strlen(buf), MSG_NOSIGNAL,
              (const struct sockaddr *) addr, sizeof(struct sockaddr_un)) == -1)
    {
        switch (errno)
        {
          case ENOENT:
              warn("Can't write to %s\n", addr->sun_path);
              break;
          default:
              perror("error sending datagram");
         }
         ret_value = errno;
    }

    close(csfd);

    p_delete(&msg);
    p_delete(&addr);

    return ret_value;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
