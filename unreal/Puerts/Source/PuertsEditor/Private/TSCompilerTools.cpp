#include "TSCompilerTools.h"

#include "FileSystemOperation.h"
#include "PuertsEditorModule.h"
#include "TSCompileErrorReport.h"

#if PLATFORM_WINDOWS
#include "Internationalization/Regex.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#endif

#define LOCTEXT_NAMESPACE "TSCompilerErrorTools"

namespace STSCompilerErrorColumnId
{
	const FName FileNameColumnId("FileName");
	const FName ErrorStringColumnId("ErrorString");
}

class SCompilerErrorLineRow : public SMultiColumnTableRow<TSharedRef<STableViewBase>>
{
public:
	SLATE_BEGIN_ARGS(SCompilerErrorLineRow)
	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FCompilerErrorLine> InItem, const TSharedRef<STableViewBase>& InOwner);

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;

private:
	TWeakPtr<FCompilerErrorLine> ItemWeakPtr;
};

void SCompilerErrorLineRow::Construct(const FArguments& InArgs, TSharedRef<FCompilerErrorLine> InItem,
	const TSharedRef<STableViewBase>& InOwner)
{
	
	ItemWeakPtr = InItem;

	const FSuperRowType::FArguments Args = FSuperRowType::FArguments();
	FSuperRowType::Construct( Args, InOwner );
}

TSharedRef<SWidget> SCompilerErrorLineRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	if ( TSharedPtr<FCompilerErrorLine> Item = ItemWeakPtr.Pin() )
	{
		if ( ColumnName == STSCompilerErrorColumnId::FileNameColumnId )
		{
			return SNew(STextBlock)
				.Text( FText::FromString( Item->FileName ) )
				.ToolTipText( FText::FromString( Item->FilePath) )
				.ColorAndOpacity(FSlateColor(Item->TextColor));
		}
		else if ( ColumnName == STSCompilerErrorColumnId::ErrorStringColumnId )
		{
			// We don't expect that data to change over the lifetime of the stream
			return SNew(STextBlock)
				.Text( FText::FromString( Item->ErrorInfo ) )
				.ColorAndOpacity(FSlateColor(Item->TextColor));
		}

		checkNoEntry();
	}

	return SNullWidget::NullWidget;
}

TSharedRef<ITableRow> STSCompilerTools::OnGenerateRow(TSharedRef<FCompilerErrorLine> Item, const TSharedRef<STableViewBase>& Owner) const
{
	return SNew(SCompilerErrorLineRow, Item, Owner);
}

void STSCompilerTools::OnListViewMouseButtonDoubleClick(TSharedRef<FCompilerErrorLine> compiler_error_line)
{
	const FString PatternString(TEXT("\\((\\d+),(\\d+)\\)"));
	const FRegexPattern Pattern(PatternString);
	FRegexMatcher Matcher(Pattern, compiler_error_line->ErrorInfo);
	FString LineNumber;
	FString ColumnNumber;
	if (Matcher.FindNext())
	{
		LineNumber = Matcher.GetCaptureGroup(1);
		ColumnNumber = Matcher.GetCaptureGroup(2);
	}

	OpenFileAtLine(compiler_error_line->FilePath,FCString::Atoi(*LineNumber),FCString::Atoi(*ColumnNumber));
}

void STSCompilerTools::InitlizeIDE()
{
	
#if PLATFORM_WINDOWS
	FString IDEPath;

	if (!FWindowsPlatformMisc::QueryRegKey(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Classes\\Applications\\Code.exe\\shell\\open\\command\\"), TEXT(""), IDEPath))
	{
		FWindowsPlatformMisc::QueryRegKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\Applications\\Code.exe\\shell\\open\\command\\"), TEXT(""), IDEPath);
	}

	const FString PatternString(TEXT("\"(.*)\" \".*\""));
	const FRegexPattern Pattern(PatternString);
	FRegexMatcher Matcher(Pattern, IDEPath);
	if (Matcher.FindNext())
	{
		FString URL = Matcher.GetCaptureGroup(1);
		if (FPaths::FileExists(URL))
		{
			Location.URL = URL;
		}
	}
#elif PLATFORM_LINUX
	FString OutURL;
	int32 ReturnCode = -1;

	FPlatformProcess::ExecProcess(TEXT("/bin/bash"), TEXT("-c \"type -p code\""), &ReturnCode, &OutURL, nullptr);
	if (ReturnCode == 0)
	{
		Location.URL = OutURL.TrimStartAndEnd();
	}
	else
	{
		// Fallback to default install location
		FString URL = TEXT("/usr/bin/code");
		if (FPaths::FileExists(URL))
		{
			Location.URL = URL;
		}
	}
#elif PLATFORM_MAC
	NSURL* AppURL = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier:@"com.microsoft.VSCode"];
	if (AppURL != nullptr)
	{
		Location.URL = FString([AppURL path]);
	}
#endif
}

static FString MakePath(const FString& InPath)
{
	return TEXT("\"") + InPath + TEXT("\""); 
}

bool STSCompilerTools::OpenFileAtLine(const FString& FullPath, int32 LineNumber, int32 ColumnNumber)
{
	if (Location.IsValid() && UFileSystemOperation::FileExists(FullPath))
	{
		// Column & line numbers are 1-based, so dont allow zero
		LineNumber = LineNumber > 0 ? LineNumber : 1;
		ColumnNumber = ColumnNumber > 0 ? ColumnNumber : 1;

		TArray<FString> Args;
		Args.Add(TEXT("-g ") + MakePath(FullPath) + FString::Printf(TEXT(":%d:%d"), LineNumber, ColumnNumber));
		return Launch(Args);
	}

	return false;
}

bool STSCompilerTools::Launch(const TArray<FString>& InArgs)
{
	if (Location.IsValid())
	{
		FString ArgsString;
		for (const FString& Arg : InArgs)
		{
			ArgsString.Append(Arg);
			ArgsString.Append(TEXT(" "));
		}

		uint32 ProcessID;
		FProcHandle hProcess = FPlatformProcess::CreateProc(*Location.URL, *ArgsString, true, false, false, &ProcessID, 0, nullptr, nullptr, nullptr);
		return hProcess.IsValid();
	}
	
	return false;
}

void STSCompilerTools::Construct(const FArguments& InArgs)
{
	const IPuertsEditorModule& editor_module = FModuleManager::LoadModuleChecked<IPuertsEditorModule>("PuertsEditor");
	const TSharedPtr<FCompilerErrorData> compiler_error_data = editor_module.GetCompilerErrorData();
	
	InitlizeIDE();
	
	const TSharedRef<SHeaderRow> HeaderRow = SNew( SHeaderRow )
		// Source
		+ SHeaderRow::Column( STSCompilerErrorColumnId::FileNameColumnId )
		.DefaultLabel( LOCTEXT("TSCompilerErrorLine", "FileName") )
		.FillWidth(0.2f)
		// Destination
		+ SHeaderRow::Column( STSCompilerErrorColumnId::ErrorStringColumnId )
		.DefaultLabel( LOCTEXT("TSCompilerErrorLine", "ErrorInfo") )
		.FillWidth(1.0f);
	
	ChildSlot
	[
		SAssignNew(CompilerErrorView, SListView< TSharedRef<FCompilerErrorLine> >)
		.ListItemsSource( &compiler_error_data->CompilerErrorStreams )
		.OnGenerateRow( this, &STSCompilerTools::OnGenerateRow )
		.SelectionMode( ESelectionMode::Single )
		.HeaderRow( HeaderRow )
		.OnMouseButtonDoubleClick( this, &STSCompilerTools::OnListViewMouseButtonDoubleClick )
	];
}

STSCompilerTools::~STSCompilerTools()
{
}

void STSCompilerTools::UpdateTSCompilerError()
{

	CompilerErrorView->RebuildList();
}

#undef LOCTEXT_NAMESPACE
