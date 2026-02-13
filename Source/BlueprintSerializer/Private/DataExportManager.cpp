#include "DataExportManager.h"
#include "BlueprintAnalyzer.h"
#include "AssetReferenceExtractor.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "HAL/PlatformFilemanager.h"

FBS_CompleteProjectData UDataExportManager::ExtractCompleteProjectData()
{
	FBS_CompleteProjectData Data;
	
	UE_LOG(LogTemp, Warning, TEXT("DataExportManager: Starting complete project data extraction"));
	
	// Basic project information
	Data.ProjectName = FApp::GetProjectName();
	Data.ProjectPath = FPaths::GetProjectFilePath();
	Data.ExtractionTimestamp = FDateTime::Now().ToString();
	Data.VersionInfo = TEXT("Blueprint Serializer v1.0");
	
	// Extract Blueprint data
	UE_LOG(LogTemp, Warning, TEXT("Extracting Blueprint data..."));
	Data.BlueprintAnalyses = UBlueprintAnalyzer::AnalyzeAllProjectBlueprints();
	Data.TotalBlueprintCount = Data.BlueprintAnalyses.Num();
	
	// Extract reference graph
	UE_LOG(LogTemp, Warning, TEXT("Extracting reference graph..."));
	Data.ReferenceGraph = UAssetReferenceExtractor::ExtractProjectReferenceGraph();
	Data.TotalAssetCount = Data.ReferenceGraph.TotalAssetCount;
	
	UE_LOG(LogTemp, Warning, TEXT("Complete project data extraction finished: %d Blueprints, %d Total Assets"), 
		Data.TotalBlueprintCount, Data.TotalAssetCount);
	
	return Data;
}

FString UDataExportManager::ExportCompleteProjectDataToJSON(const FBS_CompleteProjectData& Data)
{
	TSharedPtr<FJsonObject> JsonObject = CompleteProjectDataToJsonObject(Data);
	
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
	
	return OutputString;
}

bool UDataExportManager::SaveCompleteProjectData(const FBS_CompleteProjectData& Data)
{
	if (!EnsureExportDirectoryExists())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create export directory"));
		return false;
	}
	
	// Generate file name
	FString FileName = GenerateExportFileName(TEXT("CompleteData"));
	FString FilePath = FPaths::Combine(GetDefaultExportPath(), FileName);
	
	// Export to JSON
	FString JsonData = ExportCompleteProjectDataToJSON(Data);
	
	// Save to file
	bool bSuccess = FFileHelper::SaveStringToFile(JsonData, *FilePath, FFileHelper::EEncodingOptions::AutoDetect);
	
	if (bSuccess)
	{
		LogExtractionResults(Data);
		UE_LOG(LogTemp, Warning, TEXT("Complete project data saved to: %s"), *FilePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save complete project data to: %s"), *FilePath);
	}
	
	return bSuccess;
}

bool UDataExportManager::ExportProjectBlueprintData()
{
	UE_LOG(LogTemp, Warning, TEXT("Exporting project Blueprint data..."));

	if (!EnsureExportDirectoryExists())
	{
		return false;
	}

	TArray<FBS_BlueprintData> BlueprintData = UBlueprintAnalyzer::AnalyzeAllProjectBlueprints();

	FString FileName = GenerateExportFileName(TEXT("BlueprintData"));
	FString FilePath = FPaths::Combine(GetDefaultExportPath(), FileName);

	FString JsonData = UBlueprintAnalyzer::ExportMultipleBlueprintsToJSON(BlueprintData);

	bool bSuccess = FFileHelper::SaveStringToFile(JsonData, *FilePath, FFileHelper::EEncodingOptions::AutoDetect);
	if (bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("Blueprint data exported: %d blueprints to %s"), BlueprintData.Num(), *FilePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to export Blueprint data"));
	}

	return bSuccess;
}

bool UDataExportManager::ExportProjectReferenceGraph()
{
	UE_LOG(LogTemp, Warning, TEXT("Exporting project reference graph..."));

	if (!EnsureExportDirectoryExists())
	{
		return false;
	}

	FBS_ProjectReferenceGraph ReferenceGraph = UAssetReferenceExtractor::ExtractProjectReferenceGraph();

	FString FileName = GenerateExportFileName(TEXT("ReferenceGraph"));
	FString FilePath = FPaths::Combine(GetDefaultExportPath(), FileName);

	FString JsonData = UAssetReferenceExtractor::ExportProjectGraphToJSON(ReferenceGraph);

	bool bSuccess = FFileHelper::SaveStringToFile(JsonData, *FilePath, FFileHelper::EEncodingOptions::AutoDetect);
	if (bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("Reference graph exported: %d assets, %d refs to %s"),
			ReferenceGraph.TotalAssetCount, ReferenceGraph.TotalReferenceCount, *FilePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to export reference graph"));
	}

	return bSuccess;
}

bool UDataExportManager::ExportCompleteProjectData()
{
	UE_LOG(LogTemp, Warning, TEXT("Exporting complete project data (Blueprints + References)..."));

	FBS_CompleteProjectData CompleteData = ExtractCompleteProjectData();
	if (!ValidateExtractedData(CompleteData))
	{
		UE_LOG(LogTemp, Error, TEXT("Data validation failed"));
		return false;
	}

	return SaveCompleteProjectData(CompleteData);
}

FString UDataExportManager::GetDefaultExportPath()
{
	// Create export path in project's Saved directory
	FString ExportPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("BlueprintSerializer"));
	return FPaths::ConvertRelativePathToFull(ExportPath);
}

FString UDataExportManager::GetTimestamp()
{
	FDateTime Now = FDateTime::Now();
	return Now.ToString(TEXT("%Y%m%d_%H%M%S"));
}

bool UDataExportManager::EnsureExportDirectoryExists()
{
	FString ExportPath = GetDefaultExportPath();
	
	if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*ExportPath))
	{
		bool bCreated = FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*ExportPath);
		if (!bCreated)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create export directory: %s"), *ExportPath);
			return false;
		}
		UE_LOG(LogTemp, Warning, TEXT("Created export directory: %s"), *ExportPath);
	}
	
	return true;
}

FString UDataExportManager::GenerateExportFileName(const FString& DataType)
{
	FString ProjectName = FApp::GetProjectName();
	FString Timestamp = GetTimestamp();
	return FString::Printf(TEXT("%s_%s_%s.json"), *ProjectName, *DataType, *Timestamp);
}

TSharedPtr<FJsonObject> UDataExportManager::CompleteProjectDataToJsonObject(const FBS_CompleteProjectData& Data)
{
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	
	// Project metadata
	RootObject->SetStringField(TEXT("projectName"), Data.ProjectName);
	RootObject->SetStringField(TEXT("projectPath"), Data.ProjectPath);
	RootObject->SetNumberField(TEXT("totalBlueprintCount"), Data.TotalBlueprintCount);
	RootObject->SetNumberField(TEXT("totalAssetCount"), Data.TotalAssetCount);
	RootObject->SetStringField(TEXT("extractionTimestamp"), Data.ExtractionTimestamp);
	RootObject->SetStringField(TEXT("versionInfo"), Data.VersionInfo);
	
	// Blueprint analyses
	TArray<TSharedPtr<FJsonValue>> BlueprintArray;
	for (const FBS_BlueprintData& BlueprintData : Data.BlueprintAnalyses)
	{
		TSharedPtr<FJsonObject> BlueprintObj = UBlueprintAnalyzer::BlueprintDataToJsonObject(BlueprintData);
		BlueprintArray.Add(MakeShareable(new FJsonValueObject(BlueprintObj)));
	}
	RootObject->SetArrayField(TEXT("blueprintAnalyses"), BlueprintArray);
	
	// Reference graph
	TSharedPtr<FJsonObject> ReferenceGraphObj = UAssetReferenceExtractor::ProjectGraphToJsonObject(Data.ReferenceGraph);
	RootObject->SetObjectField(TEXT("referenceGraph"), ReferenceGraphObj);
	
	return RootObject;
}

void UDataExportManager::LogExtractionResults(const FBS_CompleteProjectData& Data)
{
	UE_LOG(LogTemp, Warning, TEXT("=== Blueprint Serializer Data Extraction Results ==="));
	UE_LOG(LogTemp, Warning, TEXT("Project: %s"), *Data.ProjectName);
	UE_LOG(LogTemp, Warning, TEXT("Blueprints Analyzed: %d"), Data.TotalBlueprintCount);
	UE_LOG(LogTemp, Warning, TEXT("Total Assets: %d"), Data.TotalAssetCount);
	UE_LOG(LogTemp, Warning, TEXT("Total References: %d"), Data.ReferenceGraph.TotalReferenceCount);
	UE_LOG(LogTemp, Warning, TEXT("Export Time: %s"), *Data.ExtractionTimestamp);
	UE_LOG(LogTemp, Warning, TEXT("Export Location: %s"), *GetDefaultExportPath());
	UE_LOG(LogTemp, Warning, TEXT("====================================="));
	
	// Log top-level Blueprint statistics
	int32 TotalVariables = 0;
	int32 TotalFunctions = 0;
	int32 TotalComponents = 0;
	int32 TotalNodes = 0;
	
	for (const FBS_BlueprintData& BlueprintData : Data.BlueprintAnalyses)
	{
		TotalVariables += BlueprintData.Variables.Num();
		TotalFunctions += BlueprintData.Functions.Num();
		TotalComponents += BlueprintData.Components.Num();
		TotalNodes += BlueprintData.TotalNodeCount;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Blueprint Totals - Variables: %d, Functions: %d, Components: %d, Nodes: %d"), 
		TotalVariables, TotalFunctions, TotalComponents, TotalNodes);
}

bool UDataExportManager::ValidateExtractedData(const FBS_CompleteProjectData& Data)
{
	// Basic validation checks
	if (Data.ProjectName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("Data validation failed: Empty project name"));
		return false;
	}
	
	if (Data.TotalAssetCount <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Data validation failed: No assets found"));
		return false;
	}
	
	if (Data.BlueprintAnalyses.Num() != Data.TotalBlueprintCount)
	{
		UE_LOG(LogTemp, Error, TEXT("Data validation failed: Blueprint count mismatch"));
		return false;
	}
	
	if (Data.ReferenceGraph.AssetReferences.Num() != Data.TotalAssetCount)
	{
		UE_LOG(LogTemp, Error, TEXT("Data validation failed: Asset reference count mismatch"));
		return false;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Data validation passed"));
	return true;
}
