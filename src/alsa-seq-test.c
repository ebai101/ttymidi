#include <alsa/asoundlib.h>
#include <unistd.h> 

void errormessage(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    putc('\n', stderr);
}

int main()
{
    int status;
    int mode = SND_RAWMIDI_SYNC;
    snd_rawmidi_t* midiout = NULL;
    const char* portname = "hw:1,0,0";  // see alsarawportlist.c example program

    if ((status = snd_rawmidi_open(NULL, &midiout, portname, mode)) < 0) 
    {
        errormessage("Problem opening MIDI output: %s", snd_strerror(status));
        exit(1);
    }

    char noteon[3]  = {0x90, 60, 100};
    char noteoff[3] = {0x90, 60, 0};

    if ((status = snd_rawmidi_write(midiout, noteon, 3)) < 0)
    {
        errormessage("Problem writing to MIDI output: %s", snd_strerror(status));
        exit(1);
    }
    
    printf ("sent oon mensage\n");
    sleep(1);  // pause the program for one second to allow note to sound.

    if ((status = snd_rawmidi_write(midiout, noteoff, 3)) < 0)
    {
        errormessage("Problem writing to MIDI output: %s", snd_strerror(status));
        exit(1);
    }

    printf ("sent oof mensage\n");
    snd_rawmidi_close(midiout);
    midiout = NULL;    // snd_rawmidi_close() does not clear invalid pointer,
    return 0;          // so might be a good idea to erase it after closing.
}