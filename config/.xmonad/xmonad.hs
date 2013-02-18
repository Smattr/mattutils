import XMonad
import XMonad.Config.Gnome
import XMonad.Layout.NoBorders
import XMonad.Hooks.EwmhDesktops
import XMonad.Hooks.ManageHelpers
import XMonad.StackSet
import XMonad.Util.EZConfig
import XMonad.Actions.CycleWS

myManageHook = composeAll
    [ className =? "Soffice" --> doFloat -- LibreOffice presentation slide show window
    , isFullscreen --> doFullFloat -- YouTube, evince, etc. fullscreen
    ]

main =
    xmonad $ gnomeConfig {
        terminal = "/home/matthew/bin/term"
      , layoutHook = smartBorders $ layoutHook gnomeConfig
      , handleEventHook    = fullscreenEventHook
      , manageHook = myManageHook <+> manageHook gnomeConfig
    }
    `additionalKeysP`
    [ ("M-<Up>", windows swapUp)
    , ("M-<Down>", windows swapDown)
    , ("M-<Right>", shiftToNext)
    , ("M-<Left>", shiftToPrev)
    ]
