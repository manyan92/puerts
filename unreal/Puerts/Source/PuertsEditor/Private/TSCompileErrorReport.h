#pragma once
#include "Subsystems/EngineSubsystem.h"
#include "TSCompileErrorReport.generated.h"

UENUM(BlueprintType)
enum class ETSCompileErrorLevel : uint8
{
	None,
	Warn,
	Error,
};

struct FTSCompileErrorInfo
{
public:
	FTSCompileErrorInfo(): ErrorLevel(ETSCompileErrorLevel::None)
	{
	}

	FTSCompileErrorInfo(ETSCompileErrorLevel InErrorLevel,const FString& InErrorString)
	: ErrorLevel(InErrorLevel),
	ErrorString(InErrorString)
	{
	}
	ETSCompileErrorLevel ErrorLevel;
	FString ErrorString;
};

struct FTSCompileErrorLine
{
public:
	TArray<FTSCompileErrorInfo> ErrorInfos; 
};

struct FTSCompileErrorReport
{
public:
	TMap<FName,FTSCompileErrorLine> Reports;

	bool bErrorModify;
};

struct FCompilerErrorLine
{
	FString FileName;
	FString FilePath;
	FColor TextColor;
	FString ErrorInfo;
};

/**
 *
 */
UCLASS()
class PUERTSEDITOR_API UTSCompilerErrorReportSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	UFUNCTION(BlueprintCallable,Category="ErrorReport")
	static void AddErrorReport(FName ErrorFile,ETSCompileErrorLevel ErrorLevel,FString ErrorString);

	UFUNCTION(BlueprintCallable,Category="ErrorReport")
	static void CleanupErrorReportFile(FName ErrorFile);

	UFUNCTION(BlueprintCallable,Category="ErrorReport")
	static void CleanupErrorReport();

	UFUNCTION(BlueprintCallable,Category="ErrorReport")
	static void ShowErrorReport();

	UFUNCTION(BlueprintCallable,Category="ErrorReport")
	static bool HasCompilerError(FName File);

	bool IsValidError();

protected:

	

	static bool ConsumeErrorReport();

public:
	
};
