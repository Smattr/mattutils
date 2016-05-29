def meta_c(api):
    '''email'''
    if not api['which']('thunderbird'):
        api['error']('thunderbird not available')
        return -1
    # Restart Thunderbird if it's already running to cope with its shitty
    # inability to roam across networks.
    api['run'](['killall', '--quiet', '--wait', 'thunderbird'])
    return api['run'](['thunderbird'])[0]

def meta_g(api):
    '''news'''
    import feedparser
    RETRIES = 3
    if not api['which']('mplayer'):
        api['error']('mplayer not available')
        return -1
    if not api['which']('wget'):
        api['error']('wget not available')
        return -1
    for _ in range(RETRIES):
        entries = feedparser.parse('http://downloads.bbc.co.uk/podcasts/worldservice/globalnews/rss.xml').entries
        if len(entries) > 0:
            entry = entries[0]
            break
    else:
        api['error']('no entries in feed')
        return -1
    url = entry['media_content'][0]['url']
    api['notify']('streaming BBC Global News')
    return api['run']('wget %s -O - | mplayer -' % url)[0]

def meta_h(api):
    '''Toggle unclutter'''
    import os, signal
    if not api['which']('unclutter'):
        api['error']('unclutter not available')
        return -1
    procs = api['ps']('unclutter')
    if len(procs) > 0:
        api['notify']('disabling unclutter')
        for p in procs:
            os.kill(p.pid, signal.SIGTERM)
        return 0
    else:
        api['notify']('enabling unclutter')
        return api['run']('unclutter')[0]

meta_l = ['gnome-screensaver-command', '--lock']

def meta_o(api):
    '''type password'''
    import os
    return api['run']([os.path.expanduser('~/bin/pw-gui')])[0]

def meta_s(api):
    '''toggle screensaver'''
    import re

    if not api['which']('gsettings'):
        api['error']('gsettings unavailable')
        return -1

    # This looks kind of strange, but apparently there is no standard way of
    # asking Linux "what window manager am I running?"
    if not api['which']('wmctrl'):
        api['error']('wmctrl unavailable; unable to determine window manager')
        return -1
    ret, stdout, _ = api['run'](['wmctrl', '-m'])
    if ret != 0:
        api['error']('wmctrl failed; unable to determine window manager')
        return -1
    name = re.search(r'^Name:\s(.*)$', stdout, flags=re.MULTILINE)
    if name is None:
        api['error']('unidentified window manager')
        return -1
    wm = name.group(1)

    if wm == 'Mutter (Muffin)':
        # Cinnamon. For reasons I fail to comprehend, Cinnamon has an 'idle
        # activation' setting but toggling it appears to do nothing. To
        # workaround this we toggle the timeout to idle instead.
        _, timeout, _ = api['run'](['gsettings', 'get', 'org.cinnamon.desktop.session', 'idle-delay'])
        if timeout.strip() != 'uint32 0':
            ret, _, stderr = api['run'](['gsettings', 'set', 'org.cinnamon.desktop.session', 'idle-delay', '0'])
            # XXX: We need an extra check on stderr because $!^#&% gsettings still
            # returns 0 when the dconf server returns an error.
            if ret == 0 and not stderr.strip():
                api['notify']('screensaver disabled')
            else:
                api['error']('failed to disable screensaver')
        else:
            # Set timeout to 5 mins.
            ret, _, stderr = api['run'](['gsettings', 'set', 'org.cinnamon.desktop.session', 'idle-delay', '300'])
            if ret == 0 and not stderr.strip():
                api['notify']('screensaver enabled')
            else:
                api['error']('failed to enable screensaver')

    elif wm in ['Metacity', 'Compiz']:
        # Gnome 2 perhaps. Hey Gnome devs, WTF happened to the --inhibit option
        # to gnome-screensaver-command? I was using that...
        _, enabled, _ = api['run'](['gsettings', 'get', 'org.gnome.desktop.screensaver', 'idle-activation-enabled'])
        if enabled.strip() == 'true':
            ret, _, stderr = api['run'](['gsettings', 'set', 'org.gnome.desktop.screensaver', 'idle-activation-enabled', 'false'])
            if ret == 0 and not stderr.strip():
                api['notify']('screensaver disabled')
            else:
                api['error']('failed to disable screensaver')
        else:
            ret, _, stderr = api['run'](['gsettings', 'set', 'org.gnome.desktop.screensaver', 'idle-activation-enabled', 'true'])
            if ret == 0 and not stderr.strip():
                api['notify']('screensaver enabled')
            else:
                api['error']('failed to enable screensaver')

    else:
        api['error']('unidentified window manager')
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
        if api['run'](['umount', m])[0] == 0:
            unmounted.append(m)
        else:
            errored.append(m)
    if len(errored) > 0:
        api['error']('failed to unmount %s' % ', '.join(errored))
    if len(unmounted) > 0:
        api['notify']('unmounted %s' % ', '.join(unmounted))
    if len(errored) == 0 and len(unmounted) == 0:
        api['notify']('nothing to unmount')

def meta_v(api):
    '''screenshot'''
    return api['run'](['gnome-screenshot', '-a'])[0]

def meta_x(api):
    '''terminal'''
    import os
    return api['run'](os.path.expanduser('~/bin/term'))[0]

def meta_z(api):
    '''browser'''
    import os, socket
    FIREWALLED_MACHINES = frozenset(['polysemy', 'synecdoche'])
    cmd = ['chromium-browser', '--incognito']
    if socket.gethostname() in FIREWALLED_MACHINES:
        cmd.append('--proxy-pac-url=file://%s' % \
            os.path.expanduser('~/bin/nictabin/nicta-proxy.pac'))
    return api['run'](cmd)[0]
