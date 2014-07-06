## Urban Terror 4.2.0xx Client ##

### Added CVARs ###
+ `cl_crosshairhealthcolor [0|2]` - crosshair will change color depending on your health
+ `cl_drawclock12 [0|1]` - use 12 hour clock
+ `cl_drawclockcolor [1|20]` - change the color of the clock
+ `cl_drawclockfontsize [0|--]` - change the size of the clock
+ `cl_drawclockposx [0|--]` - change the x-axis of the clock
+ `cl_drawclockposy [0|--]` - change the y-axis of the clock
+ `cl_drawhealth [0|1]` - show current health on the HUD
+ `cl_drawhealthposx [0|608]` - change the x-axis of the health box
+ `cl_drawhealthposy [0|608]` - change the y-axis of the health box
+ `s_disableEnvSounds [0|1]` - disable environment sounds on maps
+ `s_radio [0|1]` - disable/enable radio calls
+ `cl_chatcolor [1|20]` - change the color of "chat:" and "teamchat:"
+ `cl_randomrgb [0|3]` - generate random cg_rgb upon startup
+ `com_nosplash [0|1]` - disable/enable the frozen sand splash screen
+ `cl_drawKills [0|1]` - show current kills on the HUD
+ `cl_teamchatindicator [0|1]` - show a (TEAM) before client name if chat is teamchat
+ `cl_hpSub [0|1]` - disable/enable ability to chat `$hp` to chat your current health
+ `con_nochat [0|2]` - disable/enable output of chat in the console
+ `cl_autoweapswitch [0|2]` - disable/enable auto weapon switching when out of ammo
+ `cl_autoreload [0|1]` - disable/enable auto weapon reload when out of ammo
+ `clan [string]` - add a clantag to your name
+ `cl_clanpos [0|1]` - change the position of the clantag
+ `con_coloredKills [0|1]` - disable/enable colored player names in the killfeed in the console
+ `con_coloredHits [0|2]` - disable/enable colored % damage in console
+ `con_consolePrompt [string]` - change the `]` that is shown in the console when typing
+ `con_promptColor [1|9]` - change the color of the console prompt (`]`)
+ `con_timePrompt [0|1]` - show the time before the console prompt
+ `con_timePrompt12 [0|1]` - use 12 hour time for the time prompt
+ `con_drawscrollbar [0|1]` - draw scrollbar in console
+ `con_fadeIn [0|1]` - use fadeing animation instead of dropping animation for the console

### Commands ###
+ `randomrgb` - generate a random RGB
+ `sysexec` - execute a command via command line from in-game
+ `maplist` - output list of maps on the server
+ `findcvar [string]` - finds all cvars with with the specified string in their names

### Other ###
+ Updated the recording message to look nicer
+ Changed the ugly console to a prettier one

### Credits ###
+ [clearskies](https://github.com/clearskies)
