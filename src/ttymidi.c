/*
 This file is part of ttymidi.

 ttymidi is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 ttymidi is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with ttymidi. If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <asm/ioctls.h>
#include <asm/termbits.h>
#include <alsa/asoundlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define SERIAL_PATH "/dev/ttyS4"
#define MIDI_PORT "hw:1,1,0"
#define PRINTONLY 0
#define VERBOSE 1
#define SILENT 0

int run = 1;
int serial;
int port_out_id;

void errormessage (const char *format, ...)
{
   	va_list ap;
   	va_start(ap, format);
   	vfprintf(stderr, format, ap);
   	va_end(ap);
   	putc('\n', stderr);
}

void exit_cli (int sig)
{
    printf ("\nStopping read thread...\n");
    close (serial);
    run = 0;
}

int setup_serial_port (const char path[], int speed)
{
    //Below is some new code for setting up serial comms which allows me
    //to use non-standard baud rates (such as 31250 for MIDI interface comms).
    //This code was provided in this thread:
    //https://groups.google.com/forum/#!searchin/beagleboard/Peter$20Hurdley%7Csort:date/beagleboard/GC0rKe6rM0g/lrHWS_e2_poJ
    //This is a direct link to the example code:
    //https://gist.githubusercontent.com/peterhurley/fbace59b55d87306a5b8/raw/220cfc2cb1f2bf03ce662fe387362c3cc21b65d7/anybaud.c

    int fd;
    struct termios2 tio;

    // open device for read
    fd = open(path, O_RDWR | O_NOCTTY | O_ASYNC);

    //if can't open file
    if (fd < 0) {
        //show error and exit
        perror(path);
        return (-1);
    }

    if (ioctl(fd, TCGETS2, & tio) < 0)
        perror("TCGETS2 ioctl");

    tio.c_cflag &= ~CBAUD;
    tio.c_cflag |= BOTHER | CS8 | CLOCAL | CREAD; // Baud rate, 8N1, local modem, receive chars
    tio.c_iflag = IGNPAR; // ignore parity errors
    tio.c_oflag = 0; //raw output
    tio.c_lflag = 0; // non-canonical
    tio.c_cc[VTIME] = 0; // don't use inter-char timer
    tio.c_cc[VMIN] = 1; // block read until 1 char arrives
    tio.c_ispeed = speed;
    tio.c_ospeed = speed;

    if (ioctl(fd, TCSETS2, & tio) < 0)
        perror("TCSETS2 ioctl");

    printf("%s speed set to %d baud\r\n", path, speed);

    return fd;
}

void read_midi_from_serial_port (snd_rawmidi_t *midiout) 
{
	char buf[3];
	int i, status;
	
	/* Lets first fast forward to first status byte... */
	if (PRINTONLY) {
		do read(serial, buf, 1);
		while (buf[0] >> 7 == 0);
	}

	while (run)
	{
		/* 
		 * super-debug mode: only print to screen whatever
		 * comes through the serial port.
		 */

		if (PRINTONLY) 
		{
			read(serial, buf, 1);
			printf("%x\t", (int) buf[0]&0xFF);
			fflush(stdout);
			continue;
		}

        int i = 1;
        while (i < 3) {
            read(serial, buf + i, 1);

            if (buf[i] >> 7 != 0) {
                // status byte
                buf[0] = buf[i];
                i = 1;
            } else {
                // data byte
                if (i == 2) {
                    i = 3;
                } else {
                    // if the message type is program change or mono key pressure, it only uses 2 bytes
                    if ((buf[0] & 0xF0) == 0xC0 || (buf[0] & 0xF0) == 0xD0)
                        i = 3;
                    else
                        i = 2;
                }
            }
        }

		// write to hardware port
        // avoid writing extra bytes on SIGINT
        if (run)
        {
            if ((status = snd_rawmidi_write (midiout, buf, 3)) < 0)
            {
                errormessage("Problem writing to MIDI output: %s", snd_strerror(status));
            }
            else
            {
                // if successful, print the MIDI message
                printf ("0x%x %d %d\n", buf[0], buf[1], buf[2]);
            }
        }
    }
}

int main(void) {
    int status;
    int mode = SND_RAWMIDI_SYNC;
    const char * portname = MIDI_PORT;

    // setup sequencer 
    printf("Setting up MIDI...\n");
    snd_rawmidi_t * midiout;
    if ((status = snd_rawmidi_open(NULL, & midiout, portname, mode)) < 0) {
        errormessage("Problem opening MIDI output: %s", snd_strerror(status));
        exit(1);
    }

    // open UART device file for read/write
    printf("Opening %s...\n", SERIAL_PATH);
    serial = setup_serial_port(SERIAL_PATH, 31250);

	if (PRINTONLY) printf ("Only printing serial messages.\n");
    signal (SIGINT, exit_cli);
    signal (SIGTERM, exit_cli);

    // start main thread
    printf ("Starting read thread.\n");
    read_midi_from_serial_port (midiout);

    // on main thread exit
    snd_rawmidi_close (midiout);
    printf ("done.\n");
  
    return 0;  
}