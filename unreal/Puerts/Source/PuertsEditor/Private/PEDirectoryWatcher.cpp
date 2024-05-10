// Fill out your copyright notice in the Description page of Project Settings.

#include "PEDirectoryWatcher.h"
#include "DirectoryWatcherModule.h"
#include "Modules/ModuleManager.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/SecureHash.h"

UPEDirectoryWatcher::UPEDirectoryWatcher()
{
    this->AddToRoot();
}

bool UPEDirectoryWatcher::Watch(const FString& InDirectory)
{
    Directory = FPaths::IsRelative(InDirectory) ? FPaths::ConvertRelativePathToFull(InDirectory) : InDirectory;
    // UE_LOG(LogTemp, Warning, TEXT("PEDirectoryWatcher::Watch: %s"), *InDirectory);
    if (IFileManager::Get().DirectoryExists(*Directory))
    {
        const auto Changed = IDirectoryWatcher::FDirectoryChanged::CreateLambda(
            [&](const TArray<FFileChangeData>& FileChanges)
            {
                TArray<FString> Added;
                TArray<FString> Modified;
                TArray<FString> Removed;

                for (auto Change : FileChanges)
                {
                    FString ProjectSave = FPaths::ProjectSavedDir();
                	FString ProjectIntermediate = FPaths::ProjectIntermediateDir();
                	FString ProjectDDCache = FPaths::ProjectDir() / TEXT("DerivedDataCache");
                	
                    if (FPaths::IsUnderDirectory(Change.Filename, ProjectSave) ||
                        FPaths::IsUnderDirectory(Change.Filename, ProjectIntermediate) ||
                        FPaths::IsUnderDirectory(Change.Filename, ProjectDDCache))
                    {
                        continue;
                    }

                	bool bSkip = false;
                	auto Plugins = IPluginManager::Get().GetEnabledPlugins();
                	for (const auto plugin : Plugins)
                	{
                		FString PluginSaveDir = plugin->GetBaseDir() / TEXT("Saved");
                		FString PluginIntermediateDir = plugin->GetBaseDir() / TEXT("Intermediate");
                		FString PluginDDCacheDir = plugin->GetBaseDir() / TEXT("DerivedDataCache");
                		if (FPaths::IsUnderDirectory(Change.Filename, PluginSaveDir) ||
                            FPaths::IsUnderDirectory(Change.Filename, PluginIntermediateDir) ||
                            FPaths::IsUnderDirectory(Change.Filename, PluginDDCacheDir))
                		{
                			bSkip = true;
                			break;
                		}
                	}

                	if (bSkip)
                    {
                        continue;
                    }
                	
                    //因为要算md5，过滤掉不关心的
                    if (!Change.Filename.EndsWith(TEXT(".ts")) && !Change.Filename.EndsWith(TEXT(".mts")) &&
                        !Change.Filename.EndsWith(TEXT(".tsx")) && !Change.Filename.EndsWith(TEXT(".json")) &&
                        !Change.Filename.EndsWith(TEXT(".js")))
                    {
                        continue;
                    }
                	
                    FPaths::NormalizeFilename(Change.Filename);
                    Change.Filename = FPaths::ConvertRelativePathToFull(Change.Filename);
                    switch (Change.Action)
                    {
                        case FFileChangeData::FCA_Added:
                            if (Added.Contains(Change.Filename))
                                continue;
                            Added.Add(Change.Filename);
                            break;
                        case FFileChangeData::FCA_Modified:
                            if (Modified.Contains(Change.Filename))
                                continue;
                            Modified.Add(Change.Filename);
                            break;
                        case FFileChangeData::FCA_Removed:
                            if (Removed.Contains(Change.Filename))
                                continue;
                            Removed.Add(Change.Filename);
                            break;
                        default:
                            continue;
                    }
                }

                if(Added.Num() > 0 || Modified.Num() > 0 || Removed.Num() > 0)
                    OnChanged.Broadcast(Added, Modified, Removed);
            });
        FDirectoryWatcherModule& DirectoryWatcherModule =
            FModuleManager::Get().LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
        IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();
        DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(
            Directory, Changed, DelegateHandle, IDirectoryWatcher::IncludeDirectoryChanges);
        return true;
    }
    return false;
}

void UPEDirectoryWatcher::UnWatch()
{
    if (DelegateHandle.IsValid())
    {
        FDirectoryWatcherModule& DirectoryWatcherModule =
            FModuleManager::Get().LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
        IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();

        DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle(Directory, DelegateHandle);
        Directory = TEXT("");
    }
}

UPEDirectoryWatcher::~UPEDirectoryWatcher()
{
    UnWatch();
}
