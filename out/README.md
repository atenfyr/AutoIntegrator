# AutoIntegrator
This is a UE4SS mod that executes [AstroModIntegrator Classic](https://github.com/atenfyr/AstroModLoader-Classic/tree/master/AstroModIntegrator) on game boot. This allows AstroModLoader-compatible mods to be loaded as UE4SS logic mods.

## Features
* Compatible with both UE4SS and AstroModLoader Classic
* Allows installation of AstroModLoader-compatible mods as native UE4SS LogicMods without any other dependencies
* Eliminates the need to execute the integrator manually; the integrator is automatically executed at game start
* Automatically updates to the latest version of AstroModIntegrator Classic from GitHub
* Configurable for different use-cases

## Manual Installation

### r2modman/Thunderstore Mod Manager/Gale
Simply install AutoIntegrator normally as you would any other Thunderstore mod. AutoIntegrator is a required pre-requisite to use most .pak mods with these mod managers.

### Vortex
The built-in Vortex extension for Astroneer automatically downloads AutoIntegrator. If needed, you can also install the extension manually here: https://www.nexusmods.com/site/mods/1547

### UE4SS
First, install UE4SS (experimental v3.0.1-788 or later: https://github.com/UE4SS-RE/RE-UE4SS/releases/download/experimental/UE4SS_v3.0.1-788-g9a20a8f.zip, or this fork https://github.com/atenfyr/RE-UE4SS/releases, which is tested to work for Astroneer). Simply drag the "mod" folder within this .zip file into the folder you are using for UE4SS mods. This is typically the "Astro\Binaries\Win64\Mods" folder within the game installation, meaning you will then have a folder at "Astro\Binaries\Win64\Mods\mod". You can then install your AstroModLoader mods into the "Astro\Content\Paks\LogicMods" directory.

### AstroModLoader Classic
Drag-and-drop the "AutoIntegratorForAMLC.zip" file within the "PAK-FOR-AMLC" directory onto the AstroModLoader Classic window. You must install UE4SS through the Settings -> "Install UE4SS..." button within AstroModLoader Classic to use this mod. This solution is mutually incompatible with pure UE4SS installation, as AstroModLoader Classic takes full control of the UE4SS mod directory.

### astro_modloader (Rust)
This mod is not supported by astro_modloader (the Rust mod loader), which does not have native UE4SS integration support.

## Linux Setup
To set up AutoIntegrator v1.0.7+ for Linux (Proton), follow the above steps for installation with UE4SS or a compatible mod manager of your choice, and then perform the following steps:

1. Install the latest version of winetricks. See this guide: https://github.com/Winetricks/winetricks?tab=readme-ov-file#installing. If you are on Debian/Ubuntu, you should perform the steps under "Manual Install" on the winetricks GitHub page to make sure that winetricks is up-to-date.
2. Install the latest version of protontricks. See this guide: https://github.com/Matoking/protontricks?tab=readme-ov-file#pipx. Using pipx is a good idea to make sure you have the latest version of protontricks.
3. Execute `protontricks --no-bwrap 361420 dotnet8` and go through any prompts that appear.

You can now launch the game as normal. Mods should typically be installed in the directory at `~/.steam/steam/steamapps/common/ASTRONEER/Astro/Content/Paks/LogicMods`.

## Configuration
If this mod is installed through UE4SS directly, you can configure this mod by editing the "config.txt" file within the directory of the mod. The default "config.txt" file is as follows:
```
default
LogicMods
true
true

This is the AutoIntegrator config. Modify the first 4 lines as needed. DON'T MODIFY THIS FILE UNLESS YOU KNOW WHAT YOU'RE DOING!
Line 1: path to folder containing mod paks. "default" = %LOCALAPPDATA\Astro\Saved\Paks; otherwise, an absolute path. LogicMods folder is also automatically integrated
Line 2: folder to output integrated pak to. "default" = Line 1; "LogicMods" = the LogicMods folder; otherwise, an absolute path
Line 3: whether or not to auto-update ModIntegrator.exe. "true" = yes. "false" = no
Line 4: whether or not to override engine version and signatures for Astroneer. "true" = yes. "false" = no
All other lines are treated as comments.
```

If this mod is installed through AstroModLoader Classic, the "config.txt" file can only be edited by modifying the .pak file itself through a tool such as UAssetGUI.

## License
The source code for this mod is available on GitHub at https://github.com/atenfyr/AutoIntegrator. It is licensed under the MIT license.
