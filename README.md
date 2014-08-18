## Urban Terror 4.2.0xx Client ##

### Added CVARs ###

_<b>Automation</b>_
+ `cl_autoweapswitch [0|2]` - disable/enable auto weapon switching when out of ammo
+ `cl_autoreload [0|1]` - disable/enable auto weapon reload when out of ammo
+ `cl_autoweapdrop [0|1]` - auto drop your weapon when out of ammo
+ `cl_autokevdrop [1|99]` - drop your kevlar when you reach a certain health percent
+ `cl_autokevdroponflag [0|1]` - auto drop your kevlar when you pick up a flag

_<b>HUD</b>_
+ `cl_crosshairhealthcolor [0|2]` - crosshair will change color depending on your health
+ `cl_drawclock12 [0|1]` - use 12 hour clock
+ `cl_drawclockcolor [1|20]` - change the color of the clock
+ `cl_drawclockfontsize [0|--]` - change the size of the clock
+ `cl_drawclockposx [0|--]` - change the x-axis of the clock
+ `cl_drawclockposy [0|--]` - change the y-axis of the clock
+ `cl_drawhealth [0|1]` - show current health on the HUD
+ `cl_drawkills [0|1]` - show current kills on the HUD
+ `cl_chatcolor [1|20]` - change the color of "chat:" and "teamchat:"
+ `cl_teamchatindicator [string]` - show a string before client name if chat is teamchat
+ `cl_deadText [string]` - change the `(DEAD)` string before a playername
+ `cl_chatArrow [0|1]` - remove the `>` in chat
+ `cl_persistentcrosshair [0|1]` - always show the dot crosshair

_<b>Sound</b>_
+ `s_disableEnvSounds [0|1]` - disable environment sounds on maps
+ `s_radio [0|1]` - disable/enable radio calls

_<b>Userinfo</b>_
+ `cl_randomrgb [0|4]` - generate random cg_rgb upon startup
+ `clan [string]` - add a clantag to your name
+ `cl_clanpos [0|1]` - change the position of the clantag

_<b>Console</b>_
+ `con_coloredKills [0|1]` - disable/enable colored player names in the killfeed in the console
+ `con_coloredHits [0|2]` - disable/enable colored % damage in console
+ `con_prompt [string]` - change the `]` that is shown in the console when typing
+ `con_height [0|100]` - change the height of the console
+ `con_promptColor [1|9]` - change the color of the console prompt (`]`)
+ `con_timePrompt [0|1]` - show the time before the console prompt
+ `con_timePrompt12 [0|1]` - use 12 hour time for the time prompt
+ `con_drawscrollbar [0|1]` - draw scrollbar in console
+ `con_fadeIn [0|1]` - use fadeing animation instead of dropping animation for the console
+ `con_margin [0|50]` - add margins to the console to make it a floating box
+ `con_tabs [0|1]` - disable/enable console tabs
+ `con_borderRGB [r,g,b]` - change the rgb of the console border

_<b>Other</b>_
+ `com_nosplash [0|1]` - disable/enable the frozen sand splash screen
+ `r_drawLegs [0|1]` - shows your legs when you look down
+ `r_noBorder [0|1]` - remove the border in windowed mode

### Commands ###
+ `randomrgb` - generate a random RGB
+ `maplist` - output list of maps on the server
+ `findcvar [string]` - finds all cvars with with the specified string in their names
+ `stealrgb [playername]` - steal another player's rgb
+ `dropitems` - bind a key to `dropitems` to drop flag in ctf, and medkit in ts 

### Chat Variables ###
+ `$hp` - chat your current health
+ `$team` - chat your current teamname
+ `$oteam` - chat the opposing teamname
+ `$p` - chat your name or the name of the player you are following

### Other ###
+ Updated the recording message to look nicer
+ Changed the ugly console to a prettier one
+ Console tabs

### Credits ###
+ [clearskies](https://github.com/clearskies)
