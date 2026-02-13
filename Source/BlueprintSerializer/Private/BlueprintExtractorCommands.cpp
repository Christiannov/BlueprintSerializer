#include "BlueprintExtractorCommands.h"
#include "BlueprintAnalyzer.h"
#include "DataExportManager.h"
#include "AssetReferenceExtractor.h"
#include "Dom/JsonObject.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/AnimBlueprint.h"
#include "Engine/Blueprint.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/IConsoleManager.h"
#endif

#if WITH_EDITOR
// Console command definitions - using static local variables to avoid linker issues
static FAutoConsoleCommand ExportAllBlueprintsCommand(
    TEXT("BP_SLZR.ExportAllBlueprints"),
    TEXT("Export analysis data for all Blueprints in the project"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FBlueprintExtractorCommands::ExportAllBlueprints)
);

static FAutoConsoleCommand ExportCompleteDataCommand(
    TEXT("BP_SLZR.ExportCompleteData"),
    TEXT("Export complete project data (Blueprints + Asset References)"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FBlueprintExtractorCommands::ExportCompleteProjectData)
);

static FAutoConsoleCommand CountBlueprintsCommand(
    TEXT("BP_SLZR.CountBlueprints"),
    TEXT("Count all Blueprints in the project"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FBlueprintExtractorCommands::CountProjectBlueprints)
);

static FAutoConsoleCommand AnalyzeBlueprintCommand(
    TEXT("BP_SLZR.AnalyzeBlueprint"),
    TEXT("Analyze a specific Blueprint. Usage: BP_SLZR.AnalyzeBlueprint /Game/Path/To/Blueprint"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FBlueprintExtractorCommands::AnalyzeSpecificBlueprint)
);

static FAutoConsoleCommand ExportSingleBlueprintCommand(
    TEXT("BP_SLZR.ExportSingleBlueprint"),
    TEXT("Export complete data for a single Blueprint to JSON. Usage: BP_SLZR.ExportSingleBlueprint /Game/Path/To/Blueprint"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FBlueprintExtractorCommands::ExportSingleBlueprint)
);

// Advanced Reference Viewer equivalent commands
static FAutoConsoleCommand ExtractAssetDependenciesCommand(
    TEXT("BP_SLZR.ExtractAssetDependencies"),
    TEXT("Extract asset dependencies with depth/breadth controls. Usage: BP_SLZR.ExtractAssetDependencies <AssetPath> [MaxDepth] [bHardOnly] [bReferencersOnly]"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FBlueprintExtractorCommands::ExtractAssetDependencies)
);

static FAutoConsoleCommand ExtractProjectDependencyMapCommand(
    TEXT("BP_SLZR.ExtractProjectDependencyMap"),
    TEXT("Project-wide dependency mapping with filtering. Usage: BP_SLZR.ExtractProjectDependencyMap [MaxDepth] [AssetClassFilter]"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FBlueprintExtractorCommands::ExtractProjectDependencyMap)
);

static FAutoConsoleCommand MapAssetNetworkCommand(
    TEXT("BP_SLZR.MapAssetNetwork"),
    TEXT("Advanced network mapping with bidirectional depth control. Usage: BP_SLZR.MapAssetNetwork <AssetPath> [MaxDepth] [bBidirectional]"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FBlueprintExtractorCommands::MapAssetNetwork)
);

static FAutoConsoleCommand ExportMultipleBlueprintsCommand(
    TEXT("BP_SLZR.ExportMultipleBlueprints"),
    TEXT("Export multiple Blueprints to JSON. Usage: BP_SLZR.ExportMultipleBlueprints <Path1> <Path2> <Path3> ..."),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FBlueprintExtractorCommands::ExportMultipleBlueprints)
);
#endif

void FBlueprintExtractorCommands::RegisterCommands()
{
#if WITH_EDITOR
    UE_LOG(LogTemp, Log, TEXT("BP_SLZR commands registered"));
    UE_LOG(LogTemp, Log, TEXT("Available commands:"));
    UE_LOG(LogTemp, Warning, TEXT("  ⚠️ BP_SLZR.ExportAllBlueprints - DANGEROUS: May cause memory overload!"));
    UE_LOG(LogTemp, Warning, TEXT("  ⚠️ BP_SLZR.ExportCompleteData - DANGEROUS: May cause memory overload!"));
    UE_LOG(LogTemp, Log, TEXT("  ✅ BP_SLZR.CountBlueprints - SAFE: Count Blueprints in project"));
    UE_LOG(LogTemp, Log, TEXT("  ✅ BP_SLZR.AnalyzeBlueprint <path> - SAFE: Analyze specific Blueprint"));
    UE_LOG(LogTemp, Log, TEXT("  ✅ BP_SLZR.ExportSingleBlueprint <path> - RECOMMENDED: Export single Blueprint to JSON"));
    UE_LOG(LogTemp, Log, TEXT("  ✅ BP_SLZR.ExportMultipleBlueprints <paths...> - RECOMMENDED: Export multiple Blueprints"));
    UE_LOG(LogTemp, Log, TEXT("Advanced Reference Viewer Commands:"));
    UE_LOG(LogTemp, Log, TEXT("  🔍 BP_SLZR.ExtractAssetDependencies <path> [depth] [hardonly] [refs] - Asset dependency analysis"));
    UE_LOG(LogTemp, Log, TEXT("  🌐 BP_SLZR.ExtractProjectDependencyMap [depth] [class] - Project-wide dependency mapping"));
    UE_LOG(LogTemp, Log, TEXT("  🕸️ BP_SLZR.MapAssetNetwork <path> [depth] [bidirectional] - Network dependency mapping"));
#endif
}

void FBlueprintExtractorCommands::UnregisterCommands()
{
#if WITH_EDITOR
    UE_LOG(LogTemp, Log, TEXT("BP_SLZR commands unregistered"));
#endif
}

void FBlueprintExtractorCommands::ExportAllBlueprints(const TArray<FString>& Args)
{
#if WITH_EDITOR
    UE_LOG(LogTemp, Log, TEXT("Starting per-Blueprint export for all project Blueprints..."));

    FString ExportDir;
    if (Args.Num() > 0)
    {
        ExportDir = Args[0];
        int32 SemicolonIndex;
        if (ExportDir.FindChar(';', SemicolonIndex))
        {
            ExportDir = ExportDir.Left(SemicolonIndex);
        }
    }
    if (ExportDir.IsEmpty())
    {
        ExportDir = FPaths::ProjectSavedDir() / TEXT("BlueprintExports") / TEXT("BP_SLZR_All");
    }

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*ExportDir))
    {
        PlatformFile.CreateDirectoryTree(*ExportDir);
    }

    try
    {
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

        TArray<FAssetData> BlueprintAssets;
        FTopLevelAssetPath BlueprintPath(UBlueprint::StaticClass());
        AssetRegistry.GetAssetsByClass(BlueprintPath, BlueprintAssets);

        TArray<FAssetData> AnimBlueprintAssets;
        FTopLevelAssetPath AnimBlueprintPath(UAnimBlueprint::StaticClass());
        AssetRegistry.GetAssetsByClass(AnimBlueprintPath, AnimBlueprintAssets);

        TSet<FName> SeenPackages;
        TArray<FAssetData> AllAssets;
        AllAssets.Reserve(BlueprintAssets.Num() + AnimBlueprintAssets.Num());
        for (const FAssetData& AssetData : BlueprintAssets)
        {
            if (!SeenPackages.Contains(AssetData.PackageName))
            {
                SeenPackages.Add(AssetData.PackageName);
                AllAssets.Add(AssetData);
            }
        }
        for (const FAssetData& AssetData : AnimBlueprintAssets)
        {
            if (!SeenPackages.Contains(AssetData.PackageName))
            {
                SeenPackages.Add(AssetData.PackageName);
                AllAssets.Add(AssetData);
            }
        }

        UE_LOG(LogTemp, Log, TEXT("Exporting %d Blueprints to: %s"), AllAssets.Num(), *ExportDir);

        int32 SuccessCount = 0;
        int32 FailCount = 0;
        TArray<TSharedPtr<FJsonValue>> Results;
        Results.Reserve(AllAssets.Num());

        for (const FAssetData& AssetData : AllAssets)
        {
            const FString BlueprintAssetPath = AssetData.GetObjectPathString();
            const bool bSuccess = UBlueprintAnalyzer::ExportSingleBlueprintToJSON(BlueprintAssetPath, ExportDir);
            SuccessCount += bSuccess ? 1 : 0;
            FailCount += bSuccess ? 0 : 1;

            TSharedPtr<FJsonObject> Entry = MakeShareable(new FJsonObject);
            Entry->SetStringField(TEXT("blueprintPath"), BlueprintAssetPath);
            Entry->SetStringField(TEXT("assetName"), AssetData.AssetName.ToString());
            Entry->SetBoolField(TEXT("success"), bSuccess);
            Results.Add(MakeShareable(new FJsonValueObject(Entry)));
        }

        TSharedPtr<FJsonObject> Manifest = MakeShareable(new FJsonObject);
        Manifest->SetStringField(TEXT("exportTimestamp"), FDateTime::Now().ToString());
        Manifest->SetStringField(TEXT("exportDirectory"), ExportDir);
        Manifest->SetNumberField(TEXT("total"), AllAssets.Num());
        Manifest->SetNumberField(TEXT("successCount"), SuccessCount);
        Manifest->SetNumberField(TEXT("failCount"), FailCount);
        Manifest->SetArrayField(TEXT("blueprints"), Results);

        FString ManifestJson;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ManifestJson);
        FJsonSerializer::Serialize(Manifest.ToSharedRef(), Writer);

        const FString ManifestName = FString::Printf(TEXT("BP_SLZR_Manifest_%s.json"), *UDataExportManager::GetTimestamp());
        const FString ManifestPath = FPaths::Combine(ExportDir, ManifestName);
        UBlueprintAnalyzer::SaveBlueprintDataToFile(ManifestJson, ManifestPath);

        UE_LOG(LogTemp, Log, TEXT("Export complete. Success: %d, Failed: %d"), SuccessCount, FailCount);
        UE_LOG(LogTemp, Log, TEXT("Manifest saved: %s"), *ManifestPath);
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("Exception during export"));
    }
#else
    UE_LOG(LogTemp, Warning, TEXT("Blueprint extraction only available in editor builds"));
#endif
}

void FBlueprintExtractorCommands::ExportCompleteProjectData(const TArray<FString>& Args)
{
#if WITH_EDITOR
    // ⚠️ CRITICAL WARNING: This command can cause memory overload and system crashes!
    UE_LOG(LogTemp, Error, TEXT("⚠️ WARNING: BP_SLZR.ExportCompleteData is DANGEROUS and may cause memory overload!"));
    UE_LOG(LogTemp, Error, TEXT("⚠️ RECOMMENDATION: Use BP_SLZR.ExportSingleBlueprint for individual blueprints instead!"));
    UE_LOG(LogTemp, Error, TEXT("⚠️ This command processes ALL project data and may crash the system!"));
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("⚠️ WARNING: Complete data extraction may cause crashes!"));
    }
    
    // Add a safety prompt - uncomment the return to disable this command entirely
    // return;
    
    UE_LOG(LogTemp, Log, TEXT("🚀 Starting complete project data extraction..."));
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, TEXT("Starting complete data extraction..."));
    }
    
    bool bSuccess = UDataExportManager::ExportCompleteProjectData();
    
    if (bSuccess)
    {
        FString ExportPath = UDataExportManager::GetDefaultExportPath();
        UE_LOG(LogTemp, Log, TEXT("🎯 Complete extraction successful! Check: %s"), *ExportPath);
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, 
                FString::Printf(TEXT("✅ Complete project data exported to: %s"), *ExportPath));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Complete extraction failed!"));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("❌ Complete extraction failed!"));
        }
    }
#endif
}

void FBlueprintExtractorCommands::CountProjectBlueprints(const TArray<FString>& Args)
{
#if WITH_EDITOR
    const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    const IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
    
    TArray<FAssetData> BlueprintAssets;
    AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BlueprintAssets);

    TArray<FAssetData> AnimBlueprintAssets;
    AssetRegistry.GetAssetsByClass(UAnimBlueprint::StaticClass()->GetClassPathName(), AnimBlueprintAssets);

    TSet<FName> SeenPackages;
    TArray<FAssetData> AllAssets;
    AllAssets.Reserve(BlueprintAssets.Num() + AnimBlueprintAssets.Num());
    for (const FAssetData& Asset : BlueprintAssets)
    {
        if (!SeenPackages.Contains(Asset.PackageName))
        {
            SeenPackages.Add(Asset.PackageName);
            AllAssets.Add(Asset);
        }
    }
    for (const FAssetData& Asset : AnimBlueprintAssets)
    {
        if (!SeenPackages.Contains(Asset.PackageName))
        {
            SeenPackages.Add(Asset.PackageName);
            AllAssets.Add(Asset);
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("📊 Found %d Blueprints in project"), AllAssets.Num());
    
    // Show breakdown by location
    TMap<FString, int32> LocationCounts;
    for (const FAssetData& Asset : AllAssets)
    {
        FString Path = Asset.PackageName.ToString();
        FString Location = Path.Contains(TEXT("/Game/")) ? Path.Mid(0, Path.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, 6)) : TEXT("Other");
        LocationCounts.FindOrAdd(Location)++;
    }
    
    UE_LOG(LogTemp, Log, TEXT("📋 Blueprint distribution:"));
    for (const auto& Pair : LocationCounts)
    {
        UE_LOG(LogTemp, Log, TEXT("  %s: %d Blueprints"), *Pair.Key, Pair.Value);
    }
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Cyan, 
            FString::Printf(TEXT("📊 Project contains %d Blueprints"), AllAssets.Num()));
    }
#endif
}

void FBlueprintExtractorCommands::AnalyzeSpecificBlueprint(const TArray<FString>& Args)
{
#if WITH_EDITOR
    if (Args.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ Usage: BP_SLZR.AnalyzeBlueprint /Game/Path/To/Blueprint"));
        return;
    }
    
    FString BlueprintPath = Args[0];
    
    // Handle semicolon-separated commands (e.g., "path;quit")
    int32 SemicolonIndex;
    if (BlueprintPath.FindChar(';', SemicolonIndex))
    {
        BlueprintPath = BlueprintPath.Left(SemicolonIndex);
    }
    
    UE_LOG(LogTemp, Log, TEXT("🔍 Analyzing Blueprint: %s"), *BlueprintPath);
    
    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
    
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Could not load Blueprint: %s"), *BlueprintPath);
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, 
                FString::Printf(TEXT("❌ Blueprint not found: %s"), *BlueprintPath));
        }
        return;
    }
    
    FBS_BlueprintData Data = UBlueprintAnalyzer::AnalyzeBlueprint(Blueprint);
    
    UE_LOG(LogTemp, Log, TEXT("✅ Blueprint Analysis Complete:"));
    UE_LOG(LogTemp, Log, TEXT("  Name: %s"), *Data.BlueprintName);
    UE_LOG(LogTemp, Log, TEXT("  Parent: %s"), *Data.ParentClassName);
    UE_LOG(LogTemp, Log, TEXT("  Variables: %d"), Data.Variables.Num());
    UE_LOG(LogTemp, Log, TEXT("  Functions: %d"), Data.Functions.Num());
    UE_LOG(LogTemp, Log, TEXT("  Components: %d"), Data.Components.Num());
    UE_LOG(LogTemp, Log, TEXT("  Total Nodes: %d"), Data.TotalNodeCount);
    UE_LOG(LogTemp, Log, TEXT("  Interfaces: %d"), Data.ImplementedInterfaces.Num());
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, 
            FString::Printf(TEXT("✅ %s: %d vars, %d funcs, %d nodes"), *Data.BlueprintName, Data.Variables.Num(), Data.Functions.Num(), Data.TotalNodeCount));
    }
#endif
}

void FBlueprintExtractorCommands::ExportSingleBlueprint(const TArray<FString>& Args)
{
#if WITH_EDITOR
    if (Args.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ Usage: BP_SLZR.ExportSingleBlueprint /Game/Path/To/Blueprint"));
        return;
    }
    
    FString BlueprintPath = Args[0];
    
    UE_LOG(LogTemp, Log, TEXT("🔍 Original argument: %s"), *BlueprintPath);
    
    // Handle semicolon-separated commands (e.g., "path;quit")
    int32 SemicolonIndex;
    if (BlueprintPath.FindChar(';', SemicolonIndex))
    {
        FString OriginalPath = BlueprintPath;
        BlueprintPath = BlueprintPath.Left(SemicolonIndex);
        UE_LOG(LogTemp, Log, TEXT("🔧 Cleaned path from '%s' to '%s'"), *OriginalPath, *BlueprintPath);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("ℹ️ No semicolon found in path"));
    }
    
    UE_LOG(LogTemp, Log, TEXT("🚀 Starting single Blueprint export: %s"), *BlueprintPath);
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, 
            FString::Printf(TEXT("Exporting Blueprint: %s"), *BlueprintPath));
    }
    
    try
    {
        UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
        
        if (!Blueprint)
        {
            UE_LOG(LogTemp, Error, TEXT("❌ Could not load Blueprint: %s"), *BlueprintPath);
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, 
                    FString::Printf(TEXT("❌ Blueprint not found: %s"), *BlueprintPath));
            }
            return;
        }
        
        // Use the new complete export method that handles everything
        bool bSuccess = UBlueprintAnalyzer::ExportSingleBlueprintToJSON(BlueprintPath);
        
        // The ExportSingleBlueprintToJSON method handles all logging internally
        if (!bSuccess)
        {
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("❌ Blueprint export failed!"));
            }
        }
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Exception during single Blueprint export"));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("❌ Exception during export"));
        }
    }
#else
    UE_LOG(LogTemp, Warning, TEXT("Blueprint extraction only available in editor builds"));
#endif
}

void FBlueprintExtractorCommands::ExportMultipleBlueprints(const TArray<FString>& Args)
{
#if WITH_EDITOR
    if (Args.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ Usage: BP_SLZR.ExportMultipleBlueprints <Path1> <Path2> <Path3> ..."));
        UE_LOG(LogTemp, Warning, TEXT("Example: BP_SLZR.ExportMultipleBlueprints /Game/BP_Character /Game/BP_Enemy /Game/BP_Weapon"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("🚀 Starting export of %d Blueprints..."), Args.Num());
    
    int32 SuccessCount = 0;
    int32 FailCount = 0;
    
    for (const FString& BlueprintPath : Args)
    {
        FString CleanPath = BlueprintPath;
        
        // Handle semicolon-separated commands
        int32 SemicolonIndex;
        if (CleanPath.FindChar(';', SemicolonIndex))
        {
            CleanPath = CleanPath.Left(SemicolonIndex);
        }
        
        // Skip empty paths
        if (CleanPath.IsEmpty())
        {
            continue;
        }
        
        UE_LOG(LogTemp, Log, TEXT("📘 Exporting: %s"), *CleanPath);
        
        // Try to load and export the blueprint
        UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *CleanPath);
        
        if (Blueprint)
        {
            bool bSuccess = UBlueprintAnalyzer::ExportSingleBlueprintToJSON(CleanPath);
            if (bSuccess)
            {
                SuccessCount++;
                UE_LOG(LogTemp, Log, TEXT("✅ Successfully exported: %s"), *CleanPath);
            }
            else
            {
                FailCount++;
                UE_LOG(LogTemp, Warning, TEXT("⚠️ Failed to export: %s"), *CleanPath);
            }
        }
        else
        {
            FailCount++;
            UE_LOG(LogTemp, Warning, TEXT("❌ Could not load Blueprint: %s"), *CleanPath);
        }
    }
    
    // Summary
    UE_LOG(LogTemp, Log, TEXT("🎯 Export complete: %d successful, %d failed out of %d total"), 
        SuccessCount, FailCount, Args.Num());
    
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, 
            SuccessCount > 0 ? FColor::Green : FColor::Red,
            FString::Printf(TEXT("Exported %d/%d Blueprints"), SuccessCount, Args.Num()));
    }
#else
    UE_LOG(LogTemp, Warning, TEXT("Blueprint extraction only available in editor builds"));
#endif
}

// ExtractProjectDependencyMap, ExtractAssetDependencies, and MapAssetNetwork implementations
// are in BlueprintExtractorCommands_Extensions.cpp
