meta_a = ['hipchat']

def meta_c(api):
    '''email'''
    if not api[1]['which']('thunderbird'):
        api[1]['error']('thunderbird not available')
        return -1
    # Restart Thunderbird if it's already running to cope with its shitty
    # inability to roam across networks.
    api[1]['run'](['killall', '--quiet', '--wait', 'thunderbird'])
    return api[1]['run'](['thunderbird'])

def meta_g(api):
    '''news'''
    import feedparser
    if not api[1]['which']('mplayer'):
        api[1]['error']('mplayer not available')
        return -1
    if not api[1]['which']('wget'):
        api[1]['error']('wget not available')
        return -1
    entry = feedparser.parse('http://downloads.bbc.co.uk/podcasts/worldservice/globalnews/rss.xml').entries[0]
    url = entry['media_content'][0]['url']
    api[1]['notify']('streaming BBC Global News')
    return api[2]['run']('wget %s -O - | mplayer -' % url)[0]

def meta_h(api):
    '''Toggle unclutter'''
    import os, signal
    if not api[1]['which']('unclutter'):
        api[1]['error']('unclutter not available')
        return -1
    procs = api[1]['ps']('unclutter')
    if len(procs) > 0:
        api[1]['notify']('disabling unclutter')
        for p in procs:
            os.kill(p.pid, signal.SIGTERM)
        return 0
    else:
        api[1]['notify']('enabling unclutter')
        return api[1]['run']('unclutter')

meta_l = ['gnome-screensaver-command', '--lock']

def meta_p(api):
    '''type password'''
    import os
    return api[1]['run']([os.path.expanduser('~/bin/pw'), 'gui-type'])

def meta_q(api):
    '''quit hipchat'''
    import os, signal
    ret, stdout, _ = api[2]['run'](['pgrep', '^hipchat\.bin$'])
    if ret != 0:
        api[1]['notify']('hipchat doesn\'t seem to be running')
        return 0
    for hipchat in map(int, stdout.split()):
        os.kill(hipchat, signal.SIGTERM)
    return 0

def meta_s(api):
    '''toggle screensaver'''
    import re

    if not api[1]['which']('gsettings'):
        api[1]['error']('gsettings unavailable')
        return -1

    # This looks kind of strange, but apparently there is no standard way of
    # asking Linux "what window manager am I running?"
    if not api[1]['which']('wmctrl'):
        api[1]['error']('wmctrl unavailable; unable to determine window manager')
        return -1
    ret, stdout, _ = api[2]['run'](['wmctrl', '-m'])
    if ret != 0:
        api[1]['error']('wmctrl failed; unable to determine window manager')
        return -1
    name = re.search(r'^Name:\s(.*)$', stdout, flags=re.MULTILINE)
    if name is None:
        api[1]['error']('unidentified window manager')
        return -1
    wm = name.group(1)

    if wm == 'Mutter (Muffin)':
        # Cinnamon. For reasons I fail to comprehend, Cinnamon has an 'idle
        # activation' setting but toggling it appears to do nothing. To
        # workaround this we toggle the timeout to idle instead.
        _, timeout, _ = api[2]['run'](['gsettings', 'get', 'org.cinnamon.desktop.session', 'idle-delay'])
        if timeout.strip() != 'uint32 0':
            ret, _, stderr = api[2]['run'](['gsettings', 'set', 'org.cinnamon.desktop.session', 'idle-delay', '0'])
            # XXX: We need an extra check on stderr because $!^#&% gsettings still
            # returns 0 when the dconf server returns an error.
            if ret == 0 and not stderr.strip():
                api[1]['notify']('screensaver disabled')
            else:
                api[1]['error']('failed to disable screensaver')
        else:
            # Set timeout to 5 mins.
            ret, _, stderr = api[2]['run'](['gsettings', 'set', 'org.cinnamon.desktop.session', 'idle-delay', '300'])
            if ret == 0 and not stderr.strip():
                api[1]['notify']('screensaver enabled')
            else:
                api[1]['error']('failed to enable screensaver')

    elif wm in ['Metacity', 'Compiz']:
        # Gnome 2 perhaps. Hey Gnome devs, WTF happened to the --inhibit option
        # to gnome-screensaver-command? I was using that...
        _, enabled, _ = api[2]['run'](['gsettings', 'get', 'org.gnome.desktop.screensaver', 'idle-activation-enabled'])
        if enabled.strip() == 'true':
            ret, _, stderr = api[2]['run'](['gsettings', 'set', 'org.gnome.desktop.screensaver', 'idle-activation-enabled', 'false'])
            if ret == 0 and not stderr.strip():
                api[1]['notify']('screensaver disabled')
            else:
                api[1]['error']('failed to disable screensaver')
        else:
            ret, _, stderr = api[2]['run'](['gsettings', 'set', 'org.gnome.desktop.screensaver', 'idle-activation-enabled', 'true'])
            if ret == 0 and not stderr.strip():
                api[1]['notify']('screensaver enabled')
            else:
                api[1]['error']('failed to enable screensaver')

    else:
        api[1]['error']('unidentified window manager')
        return -1

    return ret

def meta_u(api):
    '''unmount all user drives we currently have mounted'''
    import os
    USER_MOUNT_ROOT = '/media/%s/' % os.environ['USER']
    unmounted = []
    errored = []
    for d in os.listdir(USER_MOUNT_ROOT):
        m = os.path.join(USER_MOUNT_ROOT, d)
        if not os.path.ismount(m):
            continue
        if api[1]['run'](['umount', m]) == 0:
            unmounted.append(m)
        else:
            errored.append(m)
    if len(errored) > 0:
        api[1]['error']('failed to unmount %s' % ', '.join(errored))
    if len(unmounted) > 0:
        api[1]['notify']('unmounted %s' % ', '.join(unmounted))
    if len(errored) == 0 and len(unmounted) == 0:
        api[1]['notify']('nothing to unmount')

def meta_v(api):
    '''screenshot'''
    return api[1]['run'](['gnome-screenshot', '-a'])

def meta_x(api):
    '''terminal'''
    import os
    return api[1]['run'](os.path.expanduser('~/bin/term'))

def meta_z(api):
    '''browser'''
    import os, socket
    FIREWALLED_MACHINES = frozenset(['polysemy', 'synecdoche'])
    cmd = ['chromium-browser', '--incognito']
    if socket.gethostname() in FIREWALLED_MACHINES:
        cmd.append('--proxy-pac-url=file://%s' % \
            os.path.expanduser('~/bin/nictabin/nicta-proxy.pac'))
    return api[1]['run'](cmd)
