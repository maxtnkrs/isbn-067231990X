/* poll.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <poll.h>

static void
quit(int rc,const char *fmt,...) {
    va_list ap;

    if ( errno != 0 )   /* Report errno */
        fprintf(stderr,"%s: ",strerror(errno));

    va_start(ap,fmt);   /* Format error message */
    vfprintf(stderr,fmt,ap);
    va_end(ap);
    fputc('\n',stderr);

    exit(rc);           /* Exit with return code */
}

int
main(int argc,char **argv) {
    int z;              /* General status code */
    int f1;             /* Open fifo 1 */
    int f2;             /* Open fifo 2 */
    struct pollfd fds[2]; /* Poll events */
    int nfds;           /* Number of file descriptors */
    char buf[200+1];    /* I/O Buffer */
    FILE *p1, *p2;      /* Pipes from popen(3) */

    /*
     * Pipes :
     */
    if ( !(p1 = popen("ls -l|tr '[a-z]' '[A-Z]'","r")) )
        quit(1,"popen(3) failed for p1");

    if ( !(p2 = popen("ls -l|tr '[A-Z]' '[a-z]' && sleep 8","r")) )
        quit(1,"popen(3) failed for p2");

    /*
     * Obtain the underlying file descriptors :
     */
    f1 = fileno(p1);
    fds[0].fd = f1;             /* File descriptor to poll.. */
    fds[0].events = POLLIN;     /* for input events */

    f2 = fileno(p2);
    fds[1].fd = f2;             /* File descriptor to poll.. */
    fds[1].events = POLLIN;     /* for input events */

    nfds = 2;                   /* nfds is fds[2] array size */

    printf("BEGUN: f1=%d, f2=%d\n",f1,f2);

    /*
     * Enter a poll loop :
     */
    do  {
        do  {
            z = poll(fds,nfds,3500); /* Timeout is 3.5 seconds */
        } while ( z == -1 && errno == EINTR );

        if ( z == -1 )          /* Error? */
            quit(13,"poll(2)");

        if ( z == 0 ) {
            printf("TIMEOUT: f1=%d, f2=%d\n",f1,f2);
            continue;
        }

        /*
         * Control is here if f1 or f2 has data
         * available to be read.
         */
        if ( fds[0].revents & POLLIN ) {
            z = read(f1,buf,sizeof buf-1);
            if ( z == -1 )
                quit(6,"read(2) of f1.");
            if ( z > 0 ) {
                buf[z] = 0;
                printf("*** read %d bytes <<<%s>>> from f1;\n",z,buf);
            } else {
                puts("read EOF from f1;");
                pclose(p1);
                fds[0].fd = f1 = -1;
            }
        }

        if ( fds[1].revents & POLLIN ) {
            z = read(f2,buf,sizeof buf-1);
            if ( z == -1 )
                quit(6,"read(2) of f2.");
            if ( z > 0 ) {
                buf[z] = 0;
                printf("*** read %d bytes <<<%s>>> from f2;\n",z,buf);
            } else {
                puts("read EOF from f2;");
                pclose(p2);
                fds[1].fd = f2 = -1;
            }
        }

    } while ( f1 >= 0 || f2 >= 0 );

    puts("End poll.");

    return 0;
}    
