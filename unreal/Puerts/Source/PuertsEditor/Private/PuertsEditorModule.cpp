/*
 * Tencent is pleased to support the open source community by making Puerts available.
 * Copyright (C) 2020 THL A29 Limited, a Tencent company.  All rights reserved.
 * Puerts is licensed under the BSD 3-Clause License, except for the third-party components listed in the file 'LICENSE' which may
 * be subject to their corresponding license terms. This file is subject to the terms and conditions defined in file 'LICENSE',
 * which is part of this source code package.
 */

#include "PuertsEditorModule.h"
#include "JsEnv.h"
#include "Editor.h"
#include "Misc/FileHelper.h"
#include "PuertsModule.h"
#include "TypeScriptCompilerContext.h"
#include "TypeScriptBlueprint.h"
#include "SourceFileWatcher.h"
#include "JSLogger.h"
#include "JSModuleLoader.h"
#include "Binding.hpp"
#include "CompilerToolsCommands.h"
#include "UEDataBinding.hpp"
#include "Object.hpp"
#include "ToolMenus.h"
#include "TSCompilerTools.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Docking/SDockTab.h"

class FPuertsEditorModule : public IPuertsEditorModule
{
    
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    virtual void SetCmdImpl(std::function<void(const FString&, const FString&)> Func) override
    {
        CmdImpl = Func;
    }

public:
    static void SetCmdCallback(const std::function<void(const FString&, const FString&)>& Func)
    {
        Get().SetCmdImpl(Func);
    }

    virtual void OpenTSCompilerTools() override;
    virtual void UpdateTSCompilerError() override;
    virtual TSharedPtr<FCompilerErrorData> GetCompilerErrorData() const override;
private:
    void TSCompileScripts();
    //
    void PreBeginPIE(bool bIsSimulating);

    void EndPIE(bool bIsSimulating);
    virtual void CloseTSCompilerTools() override;

    void OnPostEngineInit();

    void OnEnginePreExit();

    void ShutdownJSEnv();

    TSharedRef<SDockTab> CreateTab(const FSpawnTabArgs& Args);

    TSharedPtr<puerts::FJsEnv> JsEnv;

    TSharedPtr<puerts::FSourceFileWatcher> SourceFileWatcher;

    bool Enabled = false;

    std::function<void(const FString&, const FString&)> CmdImpl;

    TUniquePtr<FAutoConsoleCommand> ConsoleCommand;

    TWeakPtr<STSCompilerTools> TSCompilerTools;
    TWeakPtr<SWindow> TSCompilerToolsWindow;

    TSharedPtr<FCompilerErrorData> CompilerErrorData;

    TSharedPtr<class FUICommandList> PluginCommands;
};

UsingCppType(FPuertsEditorModule);

struct AutoRegisterForPEM
{
    AutoRegisterForPEM()
    {
        puerts::DefineClass<FPuertsEditorModule>()
            .Function("SetCmdCallback", MakeFunction(&FPuertsEditorModule::SetCmdCallback))
            .Register();
    }
};

AutoRegisterForPEM _AutoRegisterForPEM__;

IMPLEMENT_MODULE(FPuertsEditorModule, PuertsEditor)

TSharedRef<SDockTab> FPuertsEditorModule::CreateTab(const FSpawnTabArgs& Args)
{
    
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SAssignNew(TSCompilerTools,STSCompilerTools)
        ];
}

void FPuertsEditorModule::StartupModule()
{
    FCoreDelegates::OnEnginePreExit.AddRaw(this, &FPuertsEditorModule::OnEnginePreExit);
    CompilerErrorData = MakeShareable(new FCompilerErrorData());
    
    Enabled = IPuertsModule::Get().IsWatchEnabled() && !IsRunningCommandlet();

    FEditorDelegates::PreBeginPIE.AddRaw(this, &FPuertsEditorModule::PreBeginPIE);
    FEditorDelegates::EndPIE.AddRaw(this, &FPuertsEditorModule::EndPIE);

    ConsoleCommand = MakeUnique<FAutoConsoleCommand>(TEXT("Puerts"), TEXT("Puerts action"),
        FConsoleCommandWithArgsDelegate::CreateLambda(
            [this](const TArray<FString>& Args)
            {
                if (CmdImpl)
                {
                    FString CmdForJs = TEXT("");
                    FString ArgsForJs = TEXT("");

                    if (Args.Num() > 0)
                    {
                        CmdForJs = Args[0];
                    }
                    if (Args.Num() > 1)
                    {
                        ArgsForJs = Args[1];
                    }
                    CmdImpl(CmdForJs, ArgsForJs);
                }
                else
                {
                    UE_LOG(Puerts, Error, TEXT("Puerts command not initialized"));
                }
            }));


    FPuertsEditorStyle::Initialize();
    FCompilerToolsCommands::Register();
    
    PluginCommands = MakeShareable(new FUICommandList);

    PluginCommands->MapAction(FCompilerToolsCommands::Get().PluginAction,
        FExecuteAction::CreateRaw(this, &FPuertsEditorModule::TSCompileScripts), FCanExecuteAction());

    UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.User");
    if (ToolbarMenu == nullptr)
    {
        ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
    }
    {
        FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("TSCompileScripts");
        FToolMenuEntry& Entry =
            Section.AddEntry(FToolMenuEntry::InitToolBarButton(FCompilerToolsCommands::Get().PluginAction));
        Entry.SetCommandList(PluginCommands);
    }

    OnPostEngineInit();
}

TSharedPtr<FKismetCompilerContext> MakeCompiler(
    UBlueprint* InBlueprint, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompileOptions)
{
    return MakeShared<FTypeScriptCompilerContext>(CastChecked<UTypeScriptBlueprint>(InBlueprint), InMessageLog, InCompileOptions);
}

void FPuertsEditorModule::OnPostEngineInit()
{
    if (Enabled)
    {
        FKismetCompilerContext::RegisterCompilerForBP(UTypeScriptBlueprint::StaticClass(), &MakeCompiler);

        SourceFileWatcher = MakeShared<puerts::FSourceFileWatcher>(
            [this](const FString& InPath)
            {
                if (JsEnv.IsValid())
                {
                    TArray<uint8> Source;
                    if (FFileHelper::LoadFileToArray(Source, *InPath))
                    {
                        JsEnv->ReloadSource(InPath, std::string((const char*) Source.GetData(), Source.Num()));
                    }
                    else
                    {
                        UE_LOG(Puerts, Error, TEXT("read file fail for %s"), *InPath);
                    }
                }
            });
        JsEnv = MakeShared<puerts::FJsEnv>(
            std::make_shared<puerts::DefaultJSModuleLoader>(TEXT("JavaScript")), std::make_shared<puerts::FDefaultLogger>(), -1,
            [this](const FString& InPath)
            {
                if (SourceFileWatcher.IsValid())
                {
                    SourceFileWatcher->OnSourceLoaded(InPath);
                }
            },
            TEXT("--max-old-space-size=2048"));

        JsEnv->Start("PuertsEditor/CodeAnalyze");
    }
}

void FPuertsEditorModule::OnEnginePreExit()
{

    if (TSCompilerToolsWindow.IsValid())
    {
        TSCompilerToolsWindow.Pin()->GetOnWindowClosedEvent().Clear();
        TSCompilerToolsWindow.Pin()->RequestDestroyWindow();
    }

    ShutdownJSEnv();
}

void FPuertsEditorModule::ShutdownJSEnv()
{
    if (JsEnv.IsValid())
    {
        JsEnv.Reset();
    }
    if (SourceFileWatcher.IsValid())
    {
        SourceFileWatcher.Reset();
    }
}

void FPuertsEditorModule::ShutdownModule()
{
    CmdImpl = nullptr;
    ShutdownJSEnv();
}

void FPuertsEditorModule::OpenTSCompilerTools()
{
    if (!TSCompilerToolsWindow.IsValid())
    {
        const TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(NSLOCTEXT("TSCompilerTools", "TabTitle", "TSCompilerTools"))
        .SizingRule(ESizingRule::UserSized)
        .ClientSize(FVector2D(800.0f, 600.0f))
        .AutoCenter(EAutoCenter::PreferredWorkArea)
        .SupportsMinimize(true)
        .IsTopmostWindow(true)
        .HasCloseButton(false)
        [
            SAssignNew(TSCompilerTools,STSCompilerTools)
        ];

        TSCompilerToolsWindow = Window;

        FSlateApplication::Get().AddWindow(Window);
    }
    else
    {
        TSCompilerToolsWindow.Pin()->FlashWindow();
    }
}

void FPuertsEditorModule::UpdateTSCompilerError()
{
    if (TSCompilerTools.IsValid())
    {
        TSCompilerTools.Pin()->UpdateTSCompilerError();
    }
}

TSharedPtr<FCompilerErrorData> FPuertsEditorModule::GetCompilerErrorData() const
{
    return CompilerErrorData;
}

void FPuertsEditorModule::TSCompileScripts()
{
    const FString Filename = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) / TEXT("ts_file_versions_info.json");
    IFileManager::Get().Delete(*Filename, true, true);
}

void FPuertsEditorModule::PreBeginPIE(bool bIsSimulating)
{
    if (Enabled)
    {
    }
}

void FPuertsEditorModule::EndPIE(bool bIsSimulating)
{
}

void FPuertsEditorModule::CloseTSCompilerTools()
{
    if (TSCompilerToolsWindow.IsValid())
    {
        TSCompilerToolsWindow.Pin()->RequestDestroyWindow();
    }
}
