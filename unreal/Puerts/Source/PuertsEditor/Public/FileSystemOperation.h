// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FileSystemOperation.generated.h"

USTRUCT(BlueprintType)
struct FPuretsPluginInfo
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString PluginName;

	UPROPERTY()
	FString Path;
};

/**
 *
 */
UCLASS()
class PUERTSEDITOR_API UFileSystemOperation : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "File")
    static bool ReadFile(FString Path, FString& Data);

    UFUNCTION(BlueprintCallable, Category = "File")
    static void WriteFile(FString Path, FString Data);

    UFUNCTION(BlueprintCallable, Category = "File")
    static FString ResolvePath(FString Path);

    UFUNCTION(BlueprintCallable, Category = "File")
    static bool FileExists(FString Path);

    UFUNCTION(BlueprintCallable, Category = "File")
    static bool DirectoryExists(FString Path);

    UFUNCTION(BlueprintCallable, Category = "File")
    static void CreateDirectory(FString Path);

    UFUNCTION(BlueprintCallable, Category = "File")
    static FString GetCurrentDirectory();

    UFUNCTION(BlueprintCallable, Category = "File")
    static TArray<FString> GetDirectories(FString Path);

    UFUNCTION(BlueprintCallable, Category = "File")
    static TArray<FString> GetFiles(FString Path);

	UFUNCTION(BlueprintCallable, Category = "File")
	static TArray<FPuretsPluginInfo> GetAllPluginInfos();
	
    UFUNCTION(BlueprintCallable, Category = "File")
    static void PuertsNotifyChange(FString Path, FString Source);

    UFUNCTION(BlueprintCallable, Category = "File")
    static FString FileMD5Hash(FString Path);

    // UFUNCTION(BlueprintCallable, Category = "File")
    // static TArray<FString> ReadDirectory(FString Path, TArray<FString> Extensions, TArray<FString> exclude, int32 Depth);
};

#ifdef PUERTS_WITH_SOURCE_CONTROL
namespace PuertsSourceControlUtils
{
PUERTSEDITOR_API bool MakeSourceControlFileWritable(const FString& InFileToMakeWritable);
PUERTSEDITOR_API bool CheckoutSourceControlFile(const FString& InFileToCheckout);
}    // namespace PuertsSourceControlUtils
#endif
