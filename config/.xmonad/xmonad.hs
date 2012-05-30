import XMonad
import XMonad.Config.Gnome
import XMonad.Layout.NoBorders

main = do
    xmonad $ gnomeConfig {
        terminal = "/home/matthew/bin/term",
        layoutHook = smartBorders $ layoutHook gnomeConfig
    }
