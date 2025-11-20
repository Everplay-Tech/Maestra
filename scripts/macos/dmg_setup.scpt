-- DMG layout automation used by CPack DragNDrop
-- Ensures consistent Finder presentation for OrchestraSynth installers
on run argv
    tell application "Finder"
        set dmgName to item 1 of argv
        tell disk dmgName
            open
            set current view of container window to icon view
            set toolbar visible of container window to false
            set statusbar visible of container window to false
            set bounds of container window to {100, 100, 900, 520}
            set viewOptions to the icon view options of container window
            set arrangement of viewOptions to not arranged
            set icon size of viewOptions to 96
            set text size of viewOptions to 13
            update without registering applications
            delay 0.5
            close
        end tell
    end tell
end run
