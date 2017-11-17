def meta_c(api, argv):
    '''email'''
    if not api['which']('thunderbird'):
        api['error']('thunderbird not available')
        return -1
    # Restart Thunderbird if it's already running to cope with its shitty
    # inability to roam across networks.
    api['run'](['killall', '--quiet', '--wait', 'thunderbird'])
    return api['run'](['thunderbird'] + argv[1:])[0]

def meta_h(api, argv):
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

def meta_o(api, argv):
    '''type password'''
    import os
    return api['run']([os.path.expanduser('~/bin/pw-gui')] + argv[1:])[0]

def meta_u(api, argv):
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

def meta_x(api, argv):
    '''terminal'''
    import os
    return api['run'](os.path.expanduser('~/bin/term'))[0]

def meta_z(api, argv):
    '''browser'''
    cmd = ['chromium-browser', '--incognito']
    return api['run'](cmd + argv[1:])[0]
