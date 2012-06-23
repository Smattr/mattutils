import XMonad
import XMonad.Config.Gnome
import XMonad.Layout.NoBorders

myManageHook = composeAll
    [ className =? "Soffice" --> doFloat -- LibreOffice presentation slide show window
    ]

main = do
    xmonad $ gnomeConfig {
        terminal = "/home/matthew/bin/term",
        layoutHook = smartBorders $ layoutHook gnomeConfig
    } {
        manageHook = myManageHook <+> manageHook gnomeConfig
    }
