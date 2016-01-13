# Notes on configuring VNC

2016, and it's as fiddly as ever...

## Server side

1. Install tightvncserver
2. Gnome Fallback is borked over VNC, so install xfce4-session
3. Edit ~/.vnc/xstartup to read:

```
unset SESSION_MANAGER
unset DBUS_SESSION_MANAGER
startxfce4 &
```

4. `tightvncserver -nolisten tcp -localhost -nevershared :1`

## Client side

1. `ssh -L 6000:localhost:5901 -N server`
2. `xtightvncviewer localhost:6000`
3. The terminal colour scheme makes it initially unusable, so meddle with its preferences.
4. Tab completion doesn't work because of some XFCE/VNC interaction. Edit ~/.xfce4/xfconf/xfce-perchannel-xml/xfce4-keyboard-shortcuts.xml and remove the value attribute of the Super+Tab shortcut.
