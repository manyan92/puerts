#pragma once
#include "PuertsEditorStyle.h"
#include "Framework/Commands/Commands.h"

class FCompilerToolsCommands : public TCommands<FCompilerToolsCommands>
{
public:
	FCompilerToolsCommands()
		: TCommands<FCompilerToolsCommands>(
			  TEXT("TSCompileScripts"), NSLOCTEXT("Contexts", "TSCompileScripts", "TS Compile Scripts"), NAME_None, FPuertsEditorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> PluginAction;
};
