/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <errno.h>
#include <fcntl.h>

#ifndef __WIN32__
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <sys/socket.h>
#else
	#include <cstdint>
	typedef uint16_t sa_family_t;
	typedef uint16_t in_port_t;
	#include <winsock2.h>
	#include <ws2tcpip.h>
#endif

#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "roc_core/errno_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_netio/socket_ops.h"

namespace roc {
namespace netio {

namespace {

int to_domain(address::AddrFamily family) {
    switch (family) {
    case address::Family_IPv4:
        return AF_INET;

    case address::Family_IPv6:
        return AF_INET6;

    default:
        break;
    }

    roc_panic("socket: unknown address family");
}

int to_type(SocketType type) {
    switch (type) {
    case SocketType_Tcp:
        return SOCK_STREAM;

    case SocketType_Udp:
        return SOCK_DGRAM;

    default:
        break;
    }

    roc_panic("socket: unknown socket type");
}

bool is_ewouldblock(int err) {
    // two separate checks to suppress warning when EWOULDBLOCK == EAGAIN
    if (err == EWOULDBLOCK) {
        return true;
    }
    if (err == EAGAIN) {
        return true;
    }
    return false;
}

bool is_einprogress(int err) {
    return err == EINPROGRESS || is_ewouldblock(err);
}

bool is_malformed(int err) {
    return err == EBADF || err == EFAULT || err == ENOTSOCK;
}

bool get_local_address(SocketHandle sock, address::SocketAddr& address) {
    socklen_t addrlen = address.max_slen();

    if (getsockname(sock, address.saddr(), &addrlen) == -1) {
        roc_panic_if(is_malformed(errno));

        roc_log(LogError, "socket: getsockname(): %s", core::errno_to_str().c_str());
        return false;
    }

    if (addrlen != address.slen()) {
        roc_log(LogError, "socket: getsockname(): unexpected len: got=%lu expected=%lu",
                (unsigned long)addrlen, (unsigned long)address.slen());
        return false;
    }

    return true;
}

bool get_int_option(
    SocketHandle sock, int level, int opt, const char* opt_name, int& opt_val) {
    socklen_t opt_len = sizeof(opt_val);

    if (getsockopt(sock, level, opt, (void*) &opt_val, &opt_len) == -1) {
        roc_panic_if(is_malformed(errno));

        roc_log(LogError, "socket: getsockopt(%s): %s", opt_name,
                core::errno_to_str().c_str());

        return false;
    }

    if (opt_len != sizeof(opt_val)) {
        roc_log(LogError, "socket: getsockopt(): unexpected len: got=%lu expected=%lu",
                (unsigned long)opt_len, (unsigned long)sizeof(opt_val));
        return false;
    }

    return true;
}

bool set_int_option(
    SocketHandle sock, int level, int opt, const char* opt_name, int opt_val) {
    if (setsockopt(sock, level, opt, (void*) &opt_val, sizeof(opt_val)) == -1) {
        roc_panic_if(is_malformed(errno));

        roc_log(LogError, "socket: setsockopt(%s): %s", opt_name,
                core::errno_to_str().c_str());

        return false;
    }

    return true;
}

#if !defined(SOCK_CLOEXEC)

// This function is used if SOCK_CLOEXEC is not available.
//
// Using SOCK_CLOEXEC is preferred because:
//
//  - for security reasons: without SOCK_CLOEXEC there is a time gap between descriptor
//    creation and fcntl() call, during which fork() can be called from another thread
//
//  - for performance reasons: without SOCK_CLOEXEC there are two more system calls


bool set_cloexec(SocketHandle sock) {
#ifndef __WIN32__ // Probably no equivalent on Windows
    int flags;

    while ((flags = fcntl(sock, F_GETFD)) == -1) {
        roc_panic_if(is_malformed(errno));

        if (errno != EINTR) {
            roc_log(LogError, "socket: fcntl(F_GETFD): %s", core::errno_to_str().c_str());
            return false;
        }
    }

    if (flags & FD_CLOEXEC) {
        return true;
    }

    flags |= FD_CLOEXEC;

    while (fcntl(sock, F_SETFD, flags) == -1) {
        roc_panic_if(is_malformed(errno));

        if (errno != EINTR) {
            roc_log(LogError, "socket: fcntl(F_SETFD): %s", core::errno_to_str().c_str());
            return false;
        }
    }

#endif // ! __WIN32__
    return true;
}


#endif // !defined(SOCK_CLOEXEC)

#if !defined(SOCK_NONBLOCK)

// This function is used if SOCK_NONBLOCK is not available.
//
// Using SOCK_NONBLOCK is preferred because of performance reasons.
// Without SOCK_NONBLOCK there are two more system calls.

#ifndef __WIN32__

bool set_nonblock(SocketHandle sock) {
    int flags;

    while ((flags = fcntl(sock, F_GETFL)) == -1) {
        roc_panic_if(is_malformed(errno));

        if (errno != EINTR) {
            roc_log(LogError, "socket: fcntl(F_GETFL): %s", core::errno_to_str().c_str());
            return false;
        }
    }

    if (flags & O_NONBLOCK) {
        return true;
    }

    flags |= O_NONBLOCK;

    while (fcntl(sock, F_SETFL, flags) == -1) {
        roc_panic_if(is_malformed(errno));

        if (errno != EINTR) {
            roc_log(LogError, "socket: fcntl(F_SETFL): %s", core::errno_to_str().c_str());
            return false;
        }
    }

    return true;
}

#else // __WIN32__

bool set_nonblock(SocketHandle sock) {
	int res;
	unsigned long mode = 1;	// 0 for blocking, nonzero for non blocking

	res = ioctlsocket(sock, FIONBIO, &mode);
	//if (iResult != NO_ERROR)
	  //printf("ioctlsocket failed with error: %ld\n", iResult);

    return (res == NO_ERROR);
}

#endif // __WIN32__

#endif // !defined(SOCK_NONBLOCK)

} // namespace

#if defined(SOCK_CLOEXEC) && defined(SOCK_NONBLOCK)

bool socket_create(address::AddrFamily family, SocketType type, SocketHandle& new_sock) {
    new_sock = socket(to_domain(family), to_type(type) | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);

    if (new_sock == -1) {
        roc_panic_if(is_malformed(errno));

        roc_log(LogError, "socket: socket(): %s", core::errno_to_str().c_str());
        return false;
    }

    return true;
}

#else // !defined(SOCK_CLOEXEC) || !defined(SOCK_NONBLOCK)

bool socket_create(address::AddrFamily family, SocketType type, SocketHandle& new_sock) {
    new_sock = socket(to_domain(family), to_type(type), 0);

    if (new_sock == -1) {
        roc_panic_if(is_malformed(errno));

        roc_log(LogError, "socket: socket(): %s", core::errno_to_str().c_str());
        return SockErr_Failure;
    }

    if (!set_cloexec(new_sock)) {
        (void)socket_close(new_sock);
        return false;
    }

    if (!set_nonblock(new_sock)) {
        (void)socket_close(new_sock);
        return false;
    }

    return true;
}

#endif // defined(SOCK_CLOEXEC) && defined(SOCK_NONBLOCK)

#if defined(SOCK_CLOEXEC) && defined(SOCK_NONBLOCK)

bool socket_accept(SocketHandle sock,
                   SocketHandle& new_sock,
                   address::SocketAddr& remote_address) {
    roc_panic_if(sock < 0);

    socklen_t addrlen = remote_address.max_slen();

    // Here we assume that if SOCK_CLOEXEC and SOCK_NONBLOCK are available,
    // then accept4() is available as well.
    new_sock =
        accept4(sock, remote_address.saddr(), &addrlen, SOCK_CLOEXEC | SOCK_NONBLOCK);

    if (new_sock == -1) {
        roc_panic_if(is_malformed(errno));

        roc_log(LogError, "socket: accept4(): %s", core::errno_to_str().c_str());
        return false;
    }

    if (addrlen != remote_address.slen()) {
        roc_log(LogError, "socket: accept4(): unexpected len: got=%lu expected=%lu",
                (unsigned long)addrlen, (unsigned long)remote_address.slen());
        (void)socket_close(new_sock);
        return false;
    }

    return true;
}

#else // !defined(SOCK_CLOEXEC) || !defined(SOCK_NONBLOCK)

bool socket_accept(SocketHandle sock,
                   SocketHandle& new_sock,
                   address::SocketAddr& remote_address) {
    roc_panic_if(sock < 0);

    socklen_t addrlen = remote_address.max_slen();

    new_sock = accept(sock, remote_address.saddr(), &addrlen);

    if (new_sock == -1) {
        roc_panic_if(is_malformed(errno));

        roc_log(LogError, "socket: accept(): %s", core::errno_to_str().c_str());
        return false;
    }

    if (addrlen != remote_address.slen()) {
        roc_log(LogError, "socket: accept(): unexpected len: got=%lu expected=%lu",
                (unsigned long)addrlen, (unsigned long)remote_address.slen());
        (void)socket_close(new_sock);
        return false;
    }

    if (!set_cloexec(new_sock)) {
        (void)socket_close(new_sock);
        return false;
    }

    if (!set_nonblock(new_sock)) {
        (void)socket_close(new_sock);
        return false;
    }

    return true;
}

#endif // defined(SOCK_CLOEXEC) && defined(SOCK_NONBLOCK)

bool socket_setup(SocketHandle sock, const SocketOpts& options) {
    roc_panic_if(sock < 0);

    // If SO_NOSIGPIPE is available, enable it here for socket_try_send().
#if defined(SO_NOSIGPIPE)
    if (!set_int_option(sock, SOL_SOCKET, SO_NOSIGPIPE, "SO_NOSIGPIPE", 1)) {
        return false;
    }
#endif

    if (!set_int_option(sock, IPPROTO_TCP, TCP_NODELAY, "TCP_NODELAY",
                        options.disable_nagle ? 1 : 0)) {
        return false;
    }

    return true;
}

bool socket_bind(SocketHandle sock, address::SocketAddr& local_address) {
    roc_panic_if(sock < 0);
    roc_panic_if(!local_address.has_host_port());

    // If IPV6_V6ONLY is available, use it for IPv6 addresses.
    // We require to bind IPv4 and IPv6 endpoints separately.
#if defined(IPV6_V6ONLY)
    if (local_address.family() == address::Family_IPv6) {
        if (!set_int_option(sock, IPPROTO_IPV6, IPV6_V6ONLY, "IPV6_V6ONLY", 1)) {
            return false;
        }
    }
#endif

    if (bind(sock, local_address.saddr(), local_address.slen()) == -1) {
        roc_panic_if(is_malformed(errno));

        roc_log(LogError, "socket: bind(): %s", core::errno_to_str().c_str());
        return false;
    }

    if (!get_local_address(sock, local_address)) {
        return false;
    }

    return true;
}

bool socket_listen(SocketHandle sock, size_t backlog) {
    roc_panic_if(sock < 0);

    if (listen(sock, (int)backlog) == -1) {
        roc_panic_if(is_malformed(errno));

        roc_log(LogError, "socket: listen(): %s", core::errno_to_str().c_str());
        return false;
    }

    return true;
}

bool socket_begin_connect(SocketHandle sock,
                          const address::SocketAddr& remote_address,
                          bool& completed_immediately) {
    roc_panic_if(sock < 0);
    roc_panic_if(!remote_address.has_host_port());

    int saved_errno = errno;
    int err;

    do {
        errno = 0;
        err = connect(sock, remote_address.saddr(), remote_address.slen());
    } while (err == -1 && errno == EINTR);

    roc_panic_if(is_malformed(errno));

    // On some systems connect() may return -1, but set errno to 0, which
    // indicates successful operation.
    completed_immediately = (err == 0 || errno == 0);

    if (completed_immediately || is_einprogress(errno)) {
        // follow the convention of not changing errno on success
        errno = saved_errno;
        return true;
    }

    roc_log(LogError, "socket: connect(): %s", core::errno_to_str().c_str());
    return false;
}

bool socket_end_connect(SocketHandle sock) {
    roc_panic_if(sock < 0);

    int err = 0;

    // SO_ERROR contains result of asynchronous connect()
    if (!get_int_option(sock, SOL_SOCKET, SO_ERROR, "SO_ERROR", err)) {
        return false;
    }

    roc_panic_if(is_malformed(errno));

    if (err != 0) {
        roc_log(LogError, "socket: SO_ERROR: %s", core::errno_to_str(err).c_str());
        return false;
    }

    return true;
}

#ifdef __WIN32__
#define MSG_DONTWAIT (0) // Eeek! but ok...
#endif

ssize_t socket_try_recv(SocketHandle sock, void* buf, size_t bufsz) {
    roc_panic_if(sock < 0);
    roc_panic_if(!buf);

    if (bufsz == 0) {
        return 0;
    }

    ssize_t ret;
    while ((ret = recv(sock, buf, bufsz, MSG_DONTWAIT)) == -1) {
        roc_panic_if(is_malformed(errno));

        if (errno != EINTR) {
            break;
        }
    }

    if (ret < 0 && is_ewouldblock(errno)) {
        return SockErr_WouldBlock;
    }

    if (ret < 0) {
        roc_log(LogError, "socket: recv(): %s", core::errno_to_str().c_str());
        return SockErr_Failure;
    }

    if (ret == 0) {
        return SockErr_StreamEnd;
    }

    return ret;
}

#if defined(SO_NOSIGPIPE) || defined(MSG_NOSIGNAL)

// This version is used if either SO_NOSIGPIPE or MSG_NOSIGNAL is available
//.
// Both options are needed to disable SIGPIPE on disconnected socket and
// instead get EPIPE error.
//
// If SO_NOSIGPIPE is available (e.g. on macOS and BSD), it was enabled for
// the socket in socket_setup().
//
// If MSG_NOSIGNAL is available (e.g. on Linux), we pass it to send().
ssize_t socket_try_send(SocketHandle sock, const void* buf, size_t bufsz) {
    roc_panic_if(sock < 0);
    roc_panic_if(!buf);

    if (bufsz == 0) {
        return 0;
    }

    int flags = MSG_DONTWAIT;
#if defined(MSG_NOSIGNAL)
    flags |= MSG_NOSIGNAL;
#endif

    ssize_t ret;
    while ((ret = send(sock, buf, bufsz, flags)) == -1) {
        roc_panic_if(is_malformed(errno));

        if (errno != EINTR) {
            break;
        }
    }

    if (ret < 0 && is_ewouldblock(errno)) {
        return SockErr_WouldBlock;
    }

    if (ret < 0) {
        roc_log(LogError, "socket: send(): %s", core::errno_to_str().c_str());
        return SockErr_Failure;
    }

    if (ret == 0) {
        roc_log(LogError, "socket: send(): unexpected zero return code");
        return SockErr_Failure;
    }

    return ret;
}

#else // !defined(SO_NOSIGPIPE) && !defined(MSG_NOSIGNAL)

// This version is used when both SO_NOSIGPIPE and MSG_NOSIGNAL aren't available.
//
// In this case, we modify the signal mask of the current thread to block SIGPIPE,
// then call send(), and the restore the mask back.
//
// If SIGPIPE was generated during send(), we clear the pending signal before
// restoring the mask.
//
// We don't want to mess with signal handlers because we're not controlling them.
// We're inside a library, which may be a part of an app that uses signal
// handlers for its own purposes, and may have SIGPIPE handler as well.
//
// This implementation requires POSIX 2001.
ssize_t socket_try_send(SocketHandle sock, const void* buf, size_t bufsz) {
    roc_panic_if(sock < 0);
    roc_panic_if(!buf);

    if (bufsz == 0) {
        return 0;
    }

#ifndef __WIN32__
    // Block SIGPIPE for this thread.
    // This works since kernel sends SIGPIPE to the thread that called send(),
    // not to the whole process.
    sigset_t sig_block, sig_restore;
    if (sigemptyset(&sig_block) == -1) {
        roc_panic("socket: sigemptyset(): %s", core::errno_to_str().c_str());
    }
    if (sigaddset(&sig_block, SIGPIPE) == -1) {
        roc_panic("socket: sigaddset(): %s", core::errno_to_str().c_str());
    }
    if (int err = pthread_sigmask(SIG_BLOCK, &sig_block, &sig_restore)) {
        roc_panic("socket: pthread_sigmask(): %s", core::errno_to_str(err).c_str());
    }

    // Remember if SIGPIPE was already pending before calling send().
    int sigpipe_pending = -1;
    sigset_t sig_pending;
    if (sigpending(&sig_pending) == -1) {
        roc_panic("socket: sigpending(): %s", core::errno_to_str().c_str());
    }
    if ((sigpipe_pending = sigismember(&sig_pending, SIGPIPE)) == -1) {
        roc_panic("socket: sigismember(): %s", core::errno_to_str().c_str());
    }
#endif // !__WIN32__

    ssize_t ret;
    while ((ret = send(sock, buf, bufsz, MSG_DONTWAIT)) == -1) {
        roc_panic_if(is_malformed(errno));

        if (errno != EINTR) {
            break;
        }
    }

    const int saved_errno = errno;

#ifndef __WIN32__

    // If send() failed with EPIPE, and SIGPIPE was not already pending before calling
    // send(), then fetch SIGPIPE from pending signal mask.
    if (ret == -1 && saved_errno == EPIPE && sigpipe_pending == 0) {
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 0;

        int sig;
        while ((sig = sigtimedwait(&sig_block, 0, &ts)) == -1) {
            if (errno != EINTR) {
                break;
            }
        }
    }

    // Restore blocked signals mask.
    if (int err = pthread_sigmask(SIG_SETMASK, &sig_restore, NULL)) {
        roc_panic("socket: pthread_sigmask(): %s", core::errno_to_str(err).c_str());
    }
#endif // !__WIN32__

    if (ret < 0 && is_ewouldblock(saved_errno)) {
        return SockErr_WouldBlock;
    }

    if (ret < 0) {
        roc_log(LogError, "socket: send(): %s", core::errno_to_str().c_str());
        return SockErr_Failure;
    }

    if (ret == 0) {
        roc_log(LogError, "socket: send(): unexpected zero return code");
        return SockErr_Failure;
    }

    return ret;
}

#endif // defined(SO_NOSIGPIPE) || defined(MSG_NOSIGNAL)

ssize_t socket_try_send_to(SocketHandle sock,
                           const void* buf,
                           size_t bufsz,
                           const address::SocketAddr& remote_address) {
    roc_panic_if(sock < 0);
    roc_panic_if(!buf);
    roc_panic_if(!remote_address.has_host_port());

    ssize_t ret;
    while ((ret = sendto(sock, buf, bufsz, MSG_DONTWAIT, remote_address.saddr(),
                         remote_address.slen()))
           == -1) {
        roc_panic_if(is_malformed(errno));

        if (errno != EINTR) {
            break;
        }
    }

    if (ret < 0 && is_ewouldblock(errno)) {
        return SockErr_WouldBlock;
    }

    if (ret < 0) {
        roc_log(LogError, "socket: sendto(): %s", core::errno_to_str().c_str());
        return SockErr_Failure;
    }

    if ((size_t)ret != bufsz) {
        roc_log(LogError,
                "socket: sendto() processed less bytes than expected: "
                "requested=%lu processed=%lu",
                (unsigned long)bufsz, (unsigned long)ret);
        return SockErr_Failure;
    }

    return ret;
}

#ifdef __WIN32__
#define SHUT_RDWR (SD_BOTH)
#endif

bool socket_shutdown(SocketHandle sock) {
    roc_panic_if(sock < 0);

    if (shutdown(sock, SHUT_RDWR) == -1) {
        roc_panic_if(is_malformed(errno));

        // shutdown() on macOS may return ENOTCONN if the other side gracefully
        // terminated connection, so we don't report a failure.
        if (errno == ENOTCONN) {
            roc_log(LogDebug,
                    "socket: shutdown(): assuming ENOTCONN does not indicate a failure");
        } else {
            roc_log(LogError, "socket: shutdown(): %s", core::errno_to_str().c_str());
            return false;
        }
    }

    return true;
}

bool socket_close(SocketHandle sock) {
    roc_panic_if(sock < 0);

    if (close(sock) == -1) {
        roc_panic_if(is_malformed(errno));

        // EINTR doesn't indiciate an error, it indiciates that retry is needed.
        // However, it's not safe to retry close() since we don't know whether
        // the file descriptor was already closed (and probably reused) or not.
        // On most systems the file descriptor is guaranteed to be closed even
        // if close() returns an error, so there will be no leak.
        if (errno == EINTR) {
            roc_log(LogDebug,
                    "socket: close(): assuming EINTR does not indicate a failure");
        } else {
            roc_log(LogError, "socket: close(): %s", core::errno_to_str().c_str());
            return false;
        }
    }

    return true;
}

bool socket_close_with_reset(SocketHandle sock) {
    roc_panic_if(sock < 0);

    // SO_LINGER with zero timeout instructs close() to send RST instead of FIN.
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = 0;

    bool setsockopt_failed = false;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (void*) &ling, sizeof(ling)) == -1) {
        roc_panic_if(is_malformed(errno));

        roc_log(LogError, "socket: setsockopt(SO_LINGER): %s",
                core::errno_to_str().c_str());
        setsockopt_failed = true;
    }

    return socket_close(sock) && !setsockopt_failed;
}

} // namespace netio
} // namespace roc
