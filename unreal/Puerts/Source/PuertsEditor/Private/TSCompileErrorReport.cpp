#include "TSCompileErrorReport.h"
#include "PuertsEditorModule.h"
#include "TSCompilerTools.h"

void UTSCompilerErrorReportSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UTSCompilerErrorReportSubsystem::AddErrorReport(FName ErrorFile,
                                                     ETSCompileErrorLevel ErrorLevel, FString ErrorString)
{
	const IPuertsEditorModule& editor_module = FModuleManager::LoadModuleChecked<IPuertsEditorModule>("PuertsEditor");
	const TSharedPtr<FCompilerErrorData> compiler_error_data = editor_module.GetCompilerErrorData();
	FTSCompileErrorLine& reportLines = compiler_error_data->GlobalErrorReport.Reports.FindOrAdd(ErrorFile);
	reportLines.ErrorInfos.Add({ErrorLevel,ErrorString});

	compiler_error_data->GlobalErrorReport.bErrorModify = true;
}

void UTSCompilerErrorReportSubsystem::CleanupErrorReportFile( FName ErrorFile)
{
	const IPuertsEditorModule& editor_module = FModuleManager::LoadModuleChecked<IPuertsEditorModule>("PuertsEditor");
	const TSharedPtr<FCompilerErrorData> compiler_error_data = editor_module.GetCompilerErrorData();
	compiler_error_data->GlobalErrorReport.Reports.Remove(ErrorFile);
	compiler_error_data->GlobalErrorReport.bErrorModify = true;
}

void UTSCompilerErrorReportSubsystem::CleanupErrorReport()
{
	const IPuertsEditorModule& editor_module = FModuleManager::LoadModuleChecked<IPuertsEditorModule>("PuertsEditor");
	const TSharedPtr<FCompilerErrorData> compiler_error_data = editor_module.GetCompilerErrorData();
	compiler_error_data->GlobalErrorReport.Reports.Empty();
	compiler_error_data->GlobalErrorReport.bErrorModify = true;
}

void UTSCompilerErrorReportSubsystem::ShowErrorReport()
{
	IPuertsEditorModule& editor_module = FModuleManager::LoadModuleChecked<IPuertsEditorModule>("PuertsEditor");
	const TSharedPtr<FCompilerErrorData> compiler_error_data = editor_module.GetCompilerErrorData();
	
	const bool bValidConsume = ConsumeErrorReport();
	
	if (!bValidConsume)
	{
		return;
	}

	if(compiler_error_data->CompilerErrorStreams.Num() > 0)
	{
		editor_module.OpenTSCompilerTools();
	}
	else
	{
		editor_module.CloseTSCompilerTools();
	}

	editor_module.UpdateTSCompilerError();
}

bool UTSCompilerErrorReportSubsystem::HasCompilerError(FName File)
{
	const IPuertsEditorModule& editor_module = FModuleManager::LoadModuleChecked<IPuertsEditorModule>("PuertsEditor");
	const TSharedPtr<FCompilerErrorData> compiler_error_data = editor_module.GetCompilerErrorData();
	const FTSCompileErrorLine* Reports = compiler_error_data->GlobalErrorReport.Reports.Find(File);
	return Reports && !Reports->ErrorInfos.IsEmpty();
}

bool UTSCompilerErrorReportSubsystem::IsValidError()
{
	const IPuertsEditorModule& editor_module = FModuleManager::LoadModuleChecked<IPuertsEditorModule>("PuertsEditor");
	const TSharedPtr<FCompilerErrorData> compiler_error_data = editor_module.GetCompilerErrorData();
	return compiler_error_data->CompilerErrorStreams.Num() > 0;
}

bool UTSCompilerErrorReportSubsystem::ConsumeErrorReport()
{
	const IPuertsEditorModule& editor_module = FModuleManager::LoadModuleChecked<IPuertsEditorModule>("PuertsEditor");
	const TSharedPtr<FCompilerErrorData> compiler_error_data = editor_module.GetCompilerErrorData();
	const bool bErrorReport = compiler_error_data->GlobalErrorReport.bErrorModify;
	if(bErrorReport)
	{
		compiler_error_data->CompilerErrorStreams.Empty();

		TArray ErrorColorLUT = { FColor::White, FColor::Yellow, FColor::Red };
		for (auto&& report : compiler_error_data->GlobalErrorReport.Reports)
		{
			for (auto&& error_info : report.Value.ErrorInfos)
			{
				const TSharedRef<FCompilerErrorLine> error_line = MakeShareable(new FCompilerErrorLine);
				error_line->FileName = FPaths::GetBaseFilename(report.Key.ToString());
				error_line->FilePath = report.Key.ToString();
				error_line->ErrorInfo = error_info.ErrorString;
				error_line->TextColor = ErrorColorLUT[(int32)error_info.ErrorLevel];
		
				compiler_error_data->CompilerErrorStreams.Add(error_line);
			}
		}
	}
	
	compiler_error_data->GlobalErrorReport.bErrorModify = false;
	return bErrorReport;
}
