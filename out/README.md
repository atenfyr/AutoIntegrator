# AutoIntegrator
This is a UE4SS mod that executes [AstroModIntegrator Classic](https://github.com/atenfyr/AstroModLoader-Classic/tree/master/AstroModIntegrator) on game boot. This allows AstroModLoader-compatible mods to be loaded as UE4SS logic mods.

## Features
* Compatible with both UE4SS and AstroModLoader Classic
* Allows installation of AstroModLoader-compatible mods as native UE4SS LogicMods without any other dependencies
* Eliminates the need to execute the integrator manually; the integrator is automatically executed at game start
* Automatically updates to the latest version of AstroModIntegrator Classic from GitHub
* Configurable for different use-cases

## Manual Installation
### UE4SS
Simply drag the "mod" folder within this .zip file into the folder you are using for UE4SS mods. This is typically the "Astro\Binaries\Win64\Mods" folder within the game installation, meaning you will then have a folder at "Astro\Binaries\Win64\Mods\mod". You can then install your AstroModLoader mods into the "Astro\Content\Paks\LogicMods" directory.

### AstroModLoader Classic
Install the "000-AutoIntegratorForAML-1.0.0_P.pak" as you would any other mod in AstroModLoader Classic. You must install UE4SS through the Settings -> "Install UE4SS..." button within AstroModLoader Classic. This solution is mutually incompatible with pure UE4SS installation, as AstroModLoader Classic takes full control of the UE4SS mod directory.

### astro_modloader (Rust)
This mod is not supported by astro_modloader (the Rust mod loader), which does not have native UE4SS integration support.

## Configuration
If this mod is installed through UE4SS directly, you can configure this mod by editing the "config.txt" file within the directory of the mod. The default "config.txt" file is as follows:
```
default
default
true

This is the AutoIntegrator config. Modify the first 3 lines as needed. DON'T MODIFY THIS FILE UNLESS YOU KNOW WHAT YOU'RE DOING!
Line 1: path to folder containing mod paks. "default" = %LOCALAPPDATA\Astro\Saved\Paks; otherwise, an absolute path. LogicMods folder is also automatically integrated
Line 2: folder to output integrated pak to. "default" = Line 1; "LogicMods" = the LogicMods folder; otherwise, an absolute path
Line 3: whether or not to auto-update ModIntegrator.exe. "true" = yes. "false" = no
All other lines are treated as comments.
```

If this mod is installed through AstroModLoader Classic, the "config.txt" file can only be edited by modifying the .pak file itself through a tool such as UAssetGUI.
