#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

int signal_received = 0;

void sigterm_handler(int sig) {
    signal_received = 1;
}

int main(int argc, char **argv) {
    Display *d;
    Window root_win;
    int xrr_event_base = 0;
    int xrr_error_base = 0;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <script>\n", argv[0]);
        exit(1);
    }

    signal(SIGTERM, sigterm_handler);
    signal(SIGCHLD, SIG_IGN);

    d = XOpenDisplay(NULL);
    if (!d) {
        fprintf(stderr, "Failed to open display\n");
        exit(1);
    }
    root_win = DefaultRootWindow(d);

    if (!XRRQueryExtension(d, &xrr_event_base, &xrr_error_base)) {
        fprintf(stderr, "RandR extension missing\n");
        exit(1);
    }
    XRRSelectInput(d, root_win, RRScreenChangeNotifyMask);

    while (!signal_received) {
        XEvent ev;

        XNextEvent(d, &ev);
        XRRUpdateConfiguration(&ev);

        if (ev.type != xrr_event_base + RRScreenChangeNotify) {
            /* skip other events (this shouldn't happen) */
            continue;
        }

        fprintf(stderr, "Screen layout change event received\n");
        switch (fork()) {
            case 0:
                close(ConnectionNumber(d));
                execvp(argv[1], &argv[1]);
                perror("Failed to execute script\n");
                exit(1);
            case -1:
                perror("fork");
                exit(1);
            default:
                break;
        }
    }
    XCloseDisplay(d);

    return 0;
}
