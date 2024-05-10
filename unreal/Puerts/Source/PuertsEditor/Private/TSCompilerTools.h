#pragma once
#include "TSCompileErrorReport.h"
#include "Widgets/Views/SListView.h"


class FCompilerErrorData : public TSharedFromThis<FCompilerErrorData>
{
public:
	FCompilerErrorData()
		: GlobalErrorReport()
	{
	}
	
	FTSCompileErrorReport GlobalErrorReport;
	// The list of streams connection
	TArray<TSharedRef<FCompilerErrorLine>> CompilerErrorStreams;
};


class STSCompilerTools : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STSCompilerTools)
	{
	}
	SLATE_END_ARGS()


	
	void Construct(const FArguments& InArgs);
	virtual ~STSCompilerTools() override;

	void UpdateTSCompilerError();

protected:
	struct FLocation
	{
		bool IsValid() const
		{
			return URL.Len() > 0;
		}

		FString URL;
	};
	/** Location instance */
	FLocation Location;

	/** String storing the solution path obtained from the module manager to avoid having to use it on a thread */
	mutable FString CachedSolutionPath;

	mutable FCriticalSection CachedSolutionPathCriticalSection;
	
	TSharedPtr<SListView<TSharedRef<FCompilerErrorLine>>> CompilerErrorView;

	TSharedRef<ITableRow> OnGenerateRow(TSharedRef<FCompilerErrorLine> Item, const TSharedRef<STableViewBase>& Owner) const;
	void OnListViewMouseButtonDoubleClick(TSharedRef<FCompilerErrorLine> compiler_error_line);

	void InitlizeIDE();

	bool OpenFileAtLine(const FString& FullPath, int32 LineNumber, int32 ColumnNumber = 0);
	/** Helper function for launching the VSCode instance with the given list of arguments */
	bool Launch(const TArray<FString>& InArgs);
};
