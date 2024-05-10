#include "CompilerToolsCommands.h"


#define LOCTEXT_NAMESPACE "CompilerToolsModule"


void FCompilerToolsCommands::RegisterCommands()
{
	UI_COMMAND(
		PluginAction, "TSCompileScripts", "ReGenerate all scripts", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE

