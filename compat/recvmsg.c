/*****************************************************************************
 * recvmsg.c: POSIX recvmsg() replacement
 *****************************************************************************
 * Copyright © 2016 Rémi Denis-Courmont
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef _WIN32
# include <errno.h>
# include <stdlib.h>
# include <mswsock.h>

ssize_t recvmsg(int fd, struct msghdr *msg, int flags)
{
    if (msg->msg_controllen != 0)
    {
        errno = ENOSYS;
        return -1;
    }

    if (msg->msg_iovlen > IOV_MAX)
    {
        errno = EINVAL;
        return -1;
    }

    WSABUF *buf = malloc(msg->msg_iovlen * sizeof (*buf));
    if (buf == NULL)
        return -1;

    for (unsigned i = 0; i < msg->msg_iovlen; i++)
    {
        buf[i].len = msg->msg_iov[i].iov_len;
        buf[i].buf = msg->msg_iov[i].iov_base;
    }

    DWORD dwFlags = flags;
    INT fromlen = msg->msg_namelen;
    DWORD rcvd;
    int ret;
    if (fromlen)
        ret = WSARecvFrom(fd, buf, msg->msg_iovlen, &rcvd, &dwFlags,
                          msg->msg_name, &fromlen, NULL, NULL);
    else
        ret = WSARecv(fd, buf, msg->msg_iovlen, &rcvd, &dwFlags,
                      NULL, NULL);
    free(buf);

    if (ret == 0)
    {
        msg->msg_namelen = fromlen;
        msg->msg_flags = dwFlags;
        return rcvd;
    }

    switch (WSAGetLastError())
    {
        case WSAEWOULDBLOCK:
            errno = EAGAIN;
            break;
    }
    return -1;
}
#endif