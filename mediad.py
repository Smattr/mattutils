# A quick hacked up daemon for remotely starting and controlling mplayer. This
# script makes no attempt to sanitise its input or perform security checks. Do
# not deploy it in a multiuser environment.

import signal, subprocess, sys

DEV_NULL = open('/dev/null', 'w')

def play(filename):
    return subprocess.Popen(
        ['mplayer', filename],
        stdin=subprocess.PIPE,
        stdout=DEV_NULL,
        stderr=DEV_NULL)

COMMANDS = {
    'NEXT':'>',
    'PREV':'<',
    'VOLUP':'0',
    'VOLDOWN':'9',
    'FORWARD':'\x1B[C', # right arrow
    'BACKWARD':'\x1B[D', # left arrow
    'FFORWARD':'\x1B[A', # up arrow
    'FBACKWARD':'\x1B[B', # down arrow
    'PAUSE':' ',
    'MUTE':'m',
    'SUBTITLES':'v',
}

def main():
    def signal_handler(sig, frame):
        sys.exit(0)
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    proc = None

    while True:
        line = sys.stdin.readline()
        if line == '':
            break

        line = line.strip()

        if line.startswith('PLAY '):
            if proc:
                try:
                    proc.terminate()
                except:
                    pass
            proc = play(line[len('PLAY '):])

        elif line == 'QUIT':
            try:
                proc.stdin.write('q')
            except:
                pass
            proc = None

        elif line in COMMANDS:
            try:
                proc.stdin.write(COMMANDS[line])
            except:
                pass

    return 0

if __name__ == '__main__':
    sys.exit(main())
