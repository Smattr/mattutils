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
