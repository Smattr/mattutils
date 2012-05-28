import XMonad
import XMonad.Config.Gnome

main = do
    xmonad $ gnomeConfig {
        terminal = "/home/matthew/bin/term"
    }
