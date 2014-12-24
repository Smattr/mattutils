def meta_c(api):
    '''email'''
    if not api[1]['which']('thunderbird'):
        api[1]['error']('thunderbird not available')
        return -1
    # Restart Thunderbird if it's already running to cope with its shitty
    # inability to roam across networks.
    api[1]['run'](['killall', '--quiet', '--wait', 'thunderbird'])
    return api[1]['run'](['thunderbird'])

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

def meta_p(api):
    '''type password'''
    import os
    return api[1]['run']([os.path.expanduser('~/bin/pw'), 'gui-type'])

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

def meta_x(api):
    import os
    return api[1]['run'](os.path.expanduser('~/bin/term'))

meta_z = ['chromium-browser', '--incognito']
