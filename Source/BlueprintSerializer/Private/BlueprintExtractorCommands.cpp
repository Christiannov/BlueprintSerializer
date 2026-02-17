#include "BlueprintExtractorCommands.h"
#include "BlueprintAnalyzer.h"
#include "DataExportManager.h"
#include "AssetReferenceExtractor.h"
#include "Dom/JsonObject.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
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

static FAutoConsoleCommand ValidateConverterReadyCommand(
    TEXT("BP_SLZR.ValidateConverterReady"),
    TEXT("Validate converter-ready export gates. Usage: BP_SLZR.ValidateConverterReady [ExportDir] [ReportPath]"),
    FConsoleCommandWithArgsDelegate::CreateStatic(&FBlueprintExtractorCommands::ValidateConverterReady)
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
    UE_LOG(LogTemp, Log, TEXT("  ✅ BP_SLZR.ValidateConverterReady [dir] [report] - Run converter gate checks"));
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

    if (FPaths::IsRelative(ExportDir))
    {
        ExportDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), ExportDir);
    }
    else
    {
        ExportDir = FPaths::ConvertRelativePathToFull(ExportDir);
    }

    FPaths::NormalizeDirectoryName(ExportDir);
    FPaths::CollapseRelativeDirectories(ExportDir);

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

void FBlueprintExtractorCommands::ValidateConverterReady(const TArray<FString>& Args)
{
#if WITH_EDITOR
    auto CleanConsoleArg = [](const FString& InArg) -> FString
    {
        FString Cleaned = InArg;
        int32 SemicolonIndex;
        if (Cleaned.FindChar(';', SemicolonIndex))
        {
            Cleaned = Cleaned.Left(SemicolonIndex);
        }
        Cleaned.TrimStartAndEndInline();
        return Cleaned;
    };

    auto ResolveLatestBatchDir = [](const FString& BaseDir) -> FString
    {
        TArray<FString> CandidateDirs;
        IFileManager::Get().FindFiles(CandidateDirs, *FPaths::Combine(BaseDir, TEXT("BP_SLZR_All_*")), false, true);
        CandidateDirs.Sort(TGreater<FString>());

        for (const FString& Candidate : CandidateDirs)
        {
            const FString FullPath = FPaths::Combine(BaseDir, Candidate);
            if (IFileManager::Get().DirectoryExists(*FullPath))
            {
                return FullPath;
            }
        }

        return FString();
    };

    const FString AbsoluteProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

    auto ResolveAbsolutePath = [&AbsoluteProjectDir](const FString& MaybeRelative) -> FString
    {
        FString OutPath = MaybeRelative;
        if (OutPath.IsEmpty())
        {
            return OutPath;
        }

        if (FPaths::IsRelative(OutPath))
        {
            OutPath = FPaths::ConvertRelativePathToFull(AbsoluteProjectDir, OutPath);
        }
        else
        {
            OutPath = FPaths::ConvertRelativePathToFull(OutPath);
        }

        FPaths::NormalizeDirectoryName(OutPath);
        FPaths::CollapseRelativeDirectories(OutPath);
        return OutPath;
    };

    FString ExportDir;
    if (Args.Num() > 0)
    {
        ExportDir = CleanConsoleArg(Args[0]);
    }

    if (ExportDir.IsEmpty())
    {
        const FString AbsoluteSavedDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
        const FString ExportRoot = ResolveAbsolutePath(FPaths::Combine(AbsoluteSavedDir, TEXT("BlueprintExports")));
        ExportDir = ResolveLatestBatchDir(ExportRoot);
        if (ExportDir.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("❌ ValidateConverterReady: no BP_SLZR_All_* export directory found under %s"), *ExportRoot);
            return;
        }
    }
    else
    {
        ExportDir = ResolveAbsolutePath(ExportDir);

        if (!IFileManager::Get().DirectoryExists(*ExportDir))
        {
            UE_LOG(LogTemp, Error, TEXT("❌ ValidateConverterReady: export directory does not exist: %s"), *ExportDir);
            return;
        }

        TArray<FString> TopLevelJsonFiles;
        IFileManager::Get().FindFiles(TopLevelJsonFiles, *FPaths::Combine(ExportDir, TEXT("*.json")), true, false);

        if (TopLevelJsonFiles.Num() == 0)
        {
            const FString MaybeBatchDir = ResolveLatestBatchDir(ExportDir);
            if (!MaybeBatchDir.IsEmpty())
            {
                ExportDir = MaybeBatchDir;
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("🔎 Validating converter-ready batch: %s"), *ExportDir);

    TArray<FString> JsonFileNames;
    IFileManager::Get().FindFiles(JsonFileNames, *FPaths::Combine(ExportDir, TEXT("*.json")), true, false);
    JsonFileNames.Sort();

    TArray<FString> ManifestFileNames;
    TArray<FString> BlueprintFileNames;
    for (const FString& FileName : JsonFileNames)
    {
        if (FileName.Contains(TEXT("Manifest"), ESearchCase::IgnoreCase))
        {
            ManifestFileNames.Add(FileName);
        }
        else if (FileName.Contains(TEXT("ValidationReport"), ESearchCase::IgnoreCase))
        {
            continue;
        }
        else
        {
            BlueprintFileNames.Add(FileName);
        }
    }

    ManifestFileNames.Sort(TGreater<FString>());
    const FString ManifestPath = ManifestFileNames.Num() > 0 ? FPaths::Combine(ExportDir, ManifestFileNames[0]) : FString();

    int32 ManifestTotal = -1;
    int32 ManifestSuccessCount = -1;
    int32 ManifestFailCount = -1;
    bool bManifestParsed = false;
    if (!ManifestPath.IsEmpty())
    {
        FString ManifestContents;
        if (FFileHelper::LoadFileToString(ManifestContents, *ManifestPath))
        {
            TSharedPtr<FJsonObject> ManifestJson;
            const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ManifestContents);
            if (FJsonSerializer::Deserialize(Reader, ManifestJson) && ManifestJson.IsValid())
            {
                double NumericField = 0.0;
                if (ManifestJson->TryGetNumberField(TEXT("total"), NumericField)) ManifestTotal = static_cast<int32>(NumericField);
                if (ManifestJson->TryGetNumberField(TEXT("successCount"), NumericField)) ManifestSuccessCount = static_cast<int32>(NumericField);
                if (ManifestJson->TryGetNumberField(TEXT("failCount"), NumericField)) ManifestFailCount = static_cast<int32>(NumericField);
                bManifestParsed = true;
            }
        }
    }

    const TArray<FString> RequiredTopLevelKeys = {
        TEXT("classSpecifiers"),
        TEXT("classConfigName"),
        TEXT("classConfigFlags"),
        TEXT("classDefaultValues"),
        TEXT("classDefaultValueDelta"),
        TEXT("dependencyClosure"),
        TEXT("userDefinedStructSchemas"),
        TEXT("userDefinedEnumSchemas")
    };

    const TArray<FString> RequiredClassConfigFlagKeys = {
        TEXT("isConfigClass"),
        TEXT("isDefaultConfig"),
        TEXT("isPerObjectConfig"),
        TEXT("isGlobalUserConfig"),
        TEXT("isProjectUserConfig"),
        TEXT("isPerPlatformConfig"),
        TEXT("configDoesNotCheckDefaults"),
        TEXT("hasConfigName")
    };

    const TArray<FString> RequiredComponentOverrideKeys = {
        TEXT("hasInheritableOverride"),
        TEXT("inheritableOwnerClassPath"),
        TEXT("inheritableOwnerClassName"),
        TEXT("inheritableComponentGuid"),
        TEXT("inheritableSourceTemplatePath"),
        TEXT("inheritableOverrideTemplatePath"),
        TEXT("inheritableOverrideProperties"),
        TEXT("inheritableOverrideValues"),
        TEXT("inheritableParentValues")
    };

    auto HasNonEmptyArrayField = [](const TSharedPtr<FJsonObject>& Obj, const FString& FieldName) -> bool
    {
        const TArray<TSharedPtr<FJsonValue>>* ArrayField = nullptr;
        return Obj.IsValid() && Obj->TryGetArrayField(FieldName, ArrayField) && ArrayField && ArrayField->Num() > 0;
    };

    int32 ParseErrors = 0;
    int32 MissingRequiredExports = 0;
    int32 DependencyClosureNonEmpty = 0;
    int32 IncludeHintsNonEmpty = 0;
    int32 UserStructSchemaNonEmpty = 0;
    int32 UserEnumSchemaNonEmpty = 0;
    int32 UserStructFieldCount = 0;
    int32 UserEnumValueCount = 0;

    int32 VariableTotal = 0;
    int32 VariableWithDeclarationSpecifiers = 0;
    int32 FunctionTotal = 0;
    int32 FunctionWithDeclarationSpecifiers = 0;

    int32 ComponentTotal = 0;
    int32 ComponentInheritedCount = 0;
    int32 ComponentHasOverrideCount = 0;
    int32 ComponentWithOverrideDeltaCount = 0;
    int32 ExportsWithOverrideCount = 0;

    int32 ClassConfigFlagsPresentCount = 0;
    int32 ExportsMissingClassConfigFlagKeys = 0;
    int32 ComponentsMissingOverrideShape = 0;

    TSet<FString> MissingTopLevelKeyNames;
    TSet<FString> MissingClassConfigFlagKeyNames;
    TSet<FString> MissingComponentOverrideKeyNames;

    for (const FString& FileName : BlueprintFileNames)
    {
        const FString FullPath = FPaths::Combine(ExportDir, FileName);
        FString Contents;
        if (!FFileHelper::LoadFileToString(Contents, *FullPath))
        {
            ParseErrors++;
            continue;
        }

        TSharedPtr<FJsonObject> Json;
        const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Contents);
        if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
        {
            ParseErrors++;
            continue;
        }

        bool bMissingTopLevelInThisExport = false;
        for (const FString& Key : RequiredTopLevelKeys)
        {
            if (!Json->HasField(Key))
            {
                bMissingTopLevelInThisExport = true;
                MissingTopLevelKeyNames.Add(Key);
            }
        }
        if (bMissingTopLevelInThisExport)
        {
            MissingRequiredExports++;
        }

        const TSharedPtr<FJsonObject>* DependencyClosureObj = nullptr;
        if (Json->TryGetObjectField(TEXT("dependencyClosure"), DependencyClosureObj) && DependencyClosureObj && DependencyClosureObj->IsValid())
        {
            const TSharedPtr<FJsonObject> Closure = *DependencyClosureObj;
            const bool bHasClosure =
                HasNonEmptyArrayField(Closure, TEXT("classPaths")) ||
                HasNonEmptyArrayField(Closure, TEXT("structPaths")) ||
                HasNonEmptyArrayField(Closure, TEXT("enumPaths")) ||
                HasNonEmptyArrayField(Closure, TEXT("interfacePaths")) ||
                HasNonEmptyArrayField(Closure, TEXT("assetPaths")) ||
                HasNonEmptyArrayField(Closure, TEXT("controlRigPaths")) ||
                HasNonEmptyArrayField(Closure, TEXT("moduleNames"));

            if (bHasClosure)
            {
                DependencyClosureNonEmpty++;
            }
            if (HasNonEmptyArrayField(Closure, TEXT("includeHints")))
            {
                IncludeHintsNonEmpty++;
            }
        }

        const TArray<TSharedPtr<FJsonValue>>* StructSchemas = nullptr;
        if (Json->TryGetArrayField(TEXT("userDefinedStructSchemas"), StructSchemas) && StructSchemas)
        {
            if (StructSchemas->Num() > 0)
            {
                UserStructSchemaNonEmpty++;
            }

            for (const TSharedPtr<FJsonValue>& StructValue : *StructSchemas)
            {
                const TSharedPtr<FJsonObject> StructObj = StructValue.IsValid() ? StructValue->AsObject() : nullptr;
                if (!StructObj.IsValid())
                {
                    continue;
                }

                const TArray<TSharedPtr<FJsonValue>>* Fields = nullptr;
                if (StructObj->TryGetArrayField(TEXT("fields"), Fields) && Fields)
                {
                    UserStructFieldCount += Fields->Num();
                }
            }
        }

        const TArray<TSharedPtr<FJsonValue>>* EnumSchemas = nullptr;
        if (Json->TryGetArrayField(TEXT("userDefinedEnumSchemas"), EnumSchemas) && EnumSchemas)
        {
            if (EnumSchemas->Num() > 0)
            {
                UserEnumSchemaNonEmpty++;
            }

            for (const TSharedPtr<FJsonValue>& EnumValue : *EnumSchemas)
            {
                const TSharedPtr<FJsonObject> EnumObj = EnumValue.IsValid() ? EnumValue->AsObject() : nullptr;
                if (!EnumObj.IsValid())
                {
                    continue;
                }

                const TArray<TSharedPtr<FJsonValue>>* Enumerators = nullptr;
                if (EnumObj->TryGetArrayField(TEXT("enumerators"), Enumerators) && Enumerators)
                {
                    UserEnumValueCount += Enumerators->Num();
                }
            }
        }

        const TArray<TSharedPtr<FJsonValue>>* VariableArray = nullptr;
        if (Json->TryGetArrayField(TEXT("detailedVariables"), VariableArray) && VariableArray)
        {
            for (const TSharedPtr<FJsonValue>& VariableValue : *VariableArray)
            {
                const TSharedPtr<FJsonObject> VariableObj = VariableValue.IsValid() ? VariableValue->AsObject() : nullptr;
                if (!VariableObj.IsValid())
                {
                    continue;
                }

                VariableTotal++;
                if (HasNonEmptyArrayField(VariableObj, TEXT("declarationSpecifiers")))
                {
                    VariableWithDeclarationSpecifiers++;
                }
            }
        }

        const TArray<TSharedPtr<FJsonValue>>* FunctionArray = nullptr;
        if (Json->TryGetArrayField(TEXT("detailedFunctions"), FunctionArray) && FunctionArray)
        {
            for (const TSharedPtr<FJsonValue>& FunctionValue : *FunctionArray)
            {
                const TSharedPtr<FJsonObject> FunctionObj = FunctionValue.IsValid() ? FunctionValue->AsObject() : nullptr;
                if (!FunctionObj.IsValid())
                {
                    continue;
                }

                FunctionTotal++;
                if (HasNonEmptyArrayField(FunctionObj, TEXT("declarationSpecifiers")))
                {
                    FunctionWithDeclarationSpecifiers++;
                }
            }
        }

        const TSharedPtr<FJsonObject>* ClassConfigFlagsObj = nullptr;
        if (Json->TryGetObjectField(TEXT("classConfigFlags"), ClassConfigFlagsObj) && ClassConfigFlagsObj && ClassConfigFlagsObj->IsValid())
        {
            ClassConfigFlagsPresentCount++;

            bool bMissingClassConfigShape = false;
            for (const FString& FlagKey : RequiredClassConfigFlagKeys)
            {
                if (!(*ClassConfigFlagsObj)->HasField(FlagKey))
                {
                    bMissingClassConfigShape = true;
                    MissingClassConfigFlagKeyNames.Add(FlagKey);
                }
            }

            if (bMissingClassConfigShape)
            {
                ExportsMissingClassConfigFlagKeys++;
            }
        }
        else
        {
            ExportsMissingClassConfigFlagKeys++;
            for (const FString& FlagKey : RequiredClassConfigFlagKeys)
            {
                MissingClassConfigFlagKeyNames.Add(FlagKey);
            }
        }

        const TArray<TSharedPtr<FJsonValue>>* ComponentArray = nullptr;
        if (Json->TryGetArrayField(TEXT("detailedComponents"), ComponentArray) && ComponentArray)
        {
            bool bExportHasOverride = false;

            for (const TSharedPtr<FJsonValue>& ComponentValue : *ComponentArray)
            {
                const TSharedPtr<FJsonObject> ComponentObj = ComponentValue.IsValid() ? ComponentValue->AsObject() : nullptr;
                if (!ComponentObj.IsValid())
                {
                    continue;
                }

                ComponentTotal++;

                bool bIsInherited = false;
                if (ComponentObj->TryGetBoolField(TEXT("isInherited"), bIsInherited) && bIsInherited)
                {
                    ComponentInheritedCount++;
                }

                bool bHasOverride = false;
                if (ComponentObj->TryGetBoolField(TEXT("hasInheritableOverride"), bHasOverride) && bHasOverride)
                {
                    ComponentHasOverrideCount++;
                    bExportHasOverride = true;
                }

                if (HasNonEmptyArrayField(ComponentObj, TEXT("inheritableOverrideProperties")))
                {
                    ComponentWithOverrideDeltaCount++;
                }

                bool bMissingComponentShape = false;
                for (const FString& ComponentKey : RequiredComponentOverrideKeys)
                {
                    if (!ComponentObj->HasField(ComponentKey))
                    {
                        bMissingComponentShape = true;
                        MissingComponentOverrideKeyNames.Add(ComponentKey);
                    }
                }

                if (bMissingComponentShape)
                {
                    ComponentsMissingOverrideShape++;
                }
            }

            if (bExportHasOverride)
            {
                ExportsWithOverrideCount++;
            }
        }
    }

    const int32 BlueprintFileCount = BlueprintFileNames.Num();
    const bool bGateManifestPresent = bManifestParsed;
    const bool bGateManifestCounts = bManifestParsed && ManifestTotal == BlueprintFileCount && ManifestSuccessCount == BlueprintFileCount && ManifestFailCount == 0;
    const bool bGateParse = ParseErrors == 0;
    const bool bGateRequiredKeys = MissingRequiredExports == 0;
    const bool bGateVariableDeclarations = VariableTotal > 0 && VariableTotal == VariableWithDeclarationSpecifiers;
    const bool bGateFunctionDeclarations = FunctionTotal > 0 && FunctionTotal == FunctionWithDeclarationSpecifiers;
    const bool bGateClassConfigShape = ExportsMissingClassConfigFlagKeys == 0;
    const bool bGateComponentShape = ComponentsMissingOverrideShape == 0;
    const bool bGateDependencyClosureCoverage = DependencyClosureNonEmpty == BlueprintFileCount;
    const bool bGateIncludeHintsCoverage = IncludeHintsNonEmpty == BlueprintFileCount;

    const bool bOverallPass =
        bGateManifestPresent &&
        bGateManifestCounts &&
        bGateParse &&
        bGateRequiredKeys &&
        bGateVariableDeclarations &&
        bGateFunctionDeclarations &&
        bGateClassConfigShape &&
        bGateComponentShape &&
        bGateDependencyClosureCoverage &&
        bGateIncludeHintsCoverage;

    TArray<FString> MissingTopLevelKeys = MissingTopLevelKeyNames.Array();
    MissingTopLevelKeys.Sort();
    TArray<FString> MissingClassConfigKeys = MissingClassConfigFlagKeyNames.Array();
    MissingClassConfigKeys.Sort();
    TArray<FString> MissingComponentShapeKeys = MissingComponentOverrideKeyNames.Array();
    MissingComponentShapeKeys.Sort();

    TSharedPtr<FJsonObject> Report = MakeShareable(new FJsonObject);
    Report->SetStringField(TEXT("analysisType"), TEXT("ConverterReadyValidation"));
    Report->SetStringField(TEXT("validationTimestamp"), FDateTime::Now().ToString());
    Report->SetStringField(TEXT("exportDirectory"), ExportDir);
    Report->SetStringField(TEXT("manifestPath"), ManifestPath);
    Report->SetBoolField(TEXT("overallPass"), bOverallPass);
    Report->SetNumberField(TEXT("blueprintFileCount"), BlueprintFileCount);
    Report->SetNumberField(TEXT("parseErrors"), ParseErrors);
    Report->SetNumberField(TEXT("missingRequiredExports"), MissingRequiredExports);

    TSharedPtr<FJsonObject> Gates = MakeShareable(new FJsonObject);
    Gates->SetBoolField(TEXT("manifestPresent"), bGateManifestPresent);
    Gates->SetBoolField(TEXT("manifestCountsMatch"), bGateManifestCounts);
    Gates->SetBoolField(TEXT("parseErrorsZero"), bGateParse);
    Gates->SetBoolField(TEXT("requiredKeysPresent"), bGateRequiredKeys);
    Gates->SetBoolField(TEXT("variableDeclarationCoverage"), bGateVariableDeclarations);
    Gates->SetBoolField(TEXT("functionDeclarationCoverage"), bGateFunctionDeclarations);
    Gates->SetBoolField(TEXT("classConfigFlagShape"), bGateClassConfigShape);
    Gates->SetBoolField(TEXT("componentOverrideShape"), bGateComponentShape);
    Gates->SetBoolField(TEXT("dependencyClosureCoverage"), bGateDependencyClosureCoverage);
    Gates->SetBoolField(TEXT("includeHintsCoverage"), bGateIncludeHintsCoverage);
    Report->SetObjectField(TEXT("gates"), Gates);

    TSharedPtr<FJsonObject> ManifestSummary = MakeShareable(new FJsonObject);
    ManifestSummary->SetNumberField(TEXT("total"), ManifestTotal);
    ManifestSummary->SetNumberField(TEXT("successCount"), ManifestSuccessCount);
    ManifestSummary->SetNumberField(TEXT("failCount"), ManifestFailCount);
    Report->SetObjectField(TEXT("manifestSummary"), ManifestSummary);

    TSharedPtr<FJsonObject> Metrics = MakeShareable(new FJsonObject);
    Metrics->SetNumberField(TEXT("dependencyClosureNonEmpty"), DependencyClosureNonEmpty);
    Metrics->SetNumberField(TEXT("includeHintsNonEmpty"), IncludeHintsNonEmpty);
    Metrics->SetNumberField(TEXT("userStructSchemaNonEmpty"), UserStructSchemaNonEmpty);
    Metrics->SetNumberField(TEXT("userEnumSchemaNonEmpty"), UserEnumSchemaNonEmpty);
    Metrics->SetNumberField(TEXT("userStructFieldCount"), UserStructFieldCount);
    Metrics->SetNumberField(TEXT("userEnumValueCount"), UserEnumValueCount);
    Metrics->SetNumberField(TEXT("variablesWithDeclarationSpecifiers"), VariableWithDeclarationSpecifiers);
    Metrics->SetNumberField(TEXT("variablesTotal"), VariableTotal);
    Metrics->SetNumberField(TEXT("functionsWithDeclarationSpecifiers"), FunctionWithDeclarationSpecifiers);
    Metrics->SetNumberField(TEXT("functionsTotal"), FunctionTotal);
    Metrics->SetNumberField(TEXT("componentsTotal"), ComponentTotal);
    Metrics->SetNumberField(TEXT("componentsInherited"), ComponentInheritedCount);
    Metrics->SetNumberField(TEXT("componentsHasInheritableOverride"), ComponentHasOverrideCount);
    Metrics->SetNumberField(TEXT("componentsWithOverrideDelta"), ComponentWithOverrideDeltaCount);
    Metrics->SetNumberField(TEXT("exportsWithInheritableOverride"), ExportsWithOverrideCount);
    Metrics->SetNumberField(TEXT("classConfigFlagsPresent"), ClassConfigFlagsPresentCount);
    Metrics->SetNumberField(TEXT("exportsMissingClassConfigFlagKeys"), ExportsMissingClassConfigFlagKeys);
    Metrics->SetNumberField(TEXT("componentsMissingOverrideShape"), ComponentsMissingOverrideShape);
    Report->SetObjectField(TEXT("metrics"), Metrics);

    Report->SetArrayField(TEXT("missingTopLevelKeys"), [&MissingTopLevelKeys]()
    {
        TArray<TSharedPtr<FJsonValue>> Out;
        for (const FString& Key : MissingTopLevelKeys)
        {
            Out.Add(MakeShareable(new FJsonValueString(Key)));
        }
        return Out;
    }());

    Report->SetArrayField(TEXT("missingClassConfigFlagKeys"), [&MissingClassConfigKeys]()
    {
        TArray<TSharedPtr<FJsonValue>> Out;
        for (const FString& Key : MissingClassConfigKeys)
        {
            Out.Add(MakeShareable(new FJsonValueString(Key)));
        }
        return Out;
    }());

    Report->SetArrayField(TEXT("missingComponentOverrideShapeKeys"), [&MissingComponentShapeKeys]()
    {
        TArray<TSharedPtr<FJsonValue>> Out;
        for (const FString& Key : MissingComponentShapeKeys)
        {
            Out.Add(MakeShareable(new FJsonValueString(Key)));
        }
        return Out;
    }());

    FString ReportPath;
    if (Args.Num() > 1)
    {
        ReportPath = CleanConsoleArg(Args[1]);
        if (FPaths::IsRelative(ReportPath))
        {
            ReportPath = FPaths::ConvertRelativePathToFull(AbsoluteProjectDir, ReportPath);
        }
        else
        {
            ReportPath = FPaths::ConvertRelativePathToFull(ReportPath);
        }
    }
    else
    {
        ReportPath = FPaths::Combine(ExportDir, FString::Printf(TEXT("BP_SLZR_ValidationReport_%s.json"), *UDataExportManager::GetTimestamp()));
    }

    FPaths::NormalizeFilename(ReportPath);
    FPaths::CollapseRelativeDirectories(ReportPath);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(ReportPath), true);

    FString ReportJson;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ReportJson);
    FJsonSerializer::Serialize(Report.ToSharedRef(), Writer);

    const bool bSaved = FFileHelper::SaveStringToFile(ReportJson, *ReportPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    UE_LOG(LogTemp, Log, TEXT("✅ Converter-ready validation completed. OverallPass=%s"), bOverallPass ? TEXT("true") : TEXT("false"));
    UE_LOG(LogTemp, Log, TEXT("   Blueprints=%d ParseErrors=%d MissingRequired=%d"), BlueprintFileCount, ParseErrors, MissingRequiredExports);
    UE_LOG(LogTemp, Log, TEXT("   Components total=%d inherited=%d overrides=%d deltas=%d"), ComponentTotal, ComponentInheritedCount, ComponentHasOverrideCount, ComponentWithOverrideDeltaCount);
    UE_LOG(LogTemp, Log, TEXT("   Dependency closure non-empty=%d includeHints non-empty=%d"), DependencyClosureNonEmpty, IncludeHintsNonEmpty);
    UE_LOG(LogTemp, Log, TEXT("   Validation report: %s%s"), *ReportPath, bSaved ? TEXT("") : TEXT(" (save failed)"));

    if (GEngine)
    {
        const FColor StatusColor = bOverallPass ? FColor::Green : FColor::Yellow;
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, StatusColor,
            FString::Printf(TEXT("Converter validation: %s (%d blueprints)"), bOverallPass ? TEXT("PASS") : TEXT("WARN"), BlueprintFileCount));
    }
#else
    UE_LOG(LogTemp, Warning, TEXT("Blueprint extraction only available in editor builds"));
#endif
}

// ExtractProjectDependencyMap, ExtractAssetDependencies, and MapAssetNetwork implementations
// are in BlueprintExtractorCommands_Extensions.cpp
