import XMonad
import XMonad.Config.Gnome
import XMonad.Layout.NoBorders
import XMonad.Hooks.EwmhDesktops
import XMonad.Hooks.ManageHelpers

myManageHook = composeAll
    [ className =? "Soffice" --> doFloat -- LibreOffice presentation slide show window
    , isFullscreen --> doFullFloat -- YouTube, evince, etc. fullscreen
    ]

main = do
    xmonad $ gnomeConfig {
        terminal = "/home/matthew/bin/term"
      , layoutHook = smartBorders $ layoutHook gnomeConfig
      , handleEventHook    = fullscreenEventHook
      , manageHook = myManageHook <+> manageHook gnomeConfig
    }
