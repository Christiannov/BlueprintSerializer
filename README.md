# Blueprint Serializer Plugin

A professional-grade Unreal Engine 5 plugin for serializing Blueprint assets to JSON format, enabling analysis, documentation, and AI-assisted code generation workflows.

## Overview

Blueprint Serializer is an engine-level developer tool that extracts comprehensive data from Unreal Engine Blueprints and exports it as structured JSON. This enables:

- 📊 **Blueprint Analysis** - Understand complex Blueprint structures and dependencies
- 🤖 **AI Code Generation** - Generate C++ code from Blueprints using LLMs like Claude
- 📝 **Documentation** - Auto-generate technical documentation from Blueprint data
- 🔍 **Dependency Mapping** - Trace asset dependencies and references
- 🔄 **Migration Support** - Facilitate Blueprint to C++ conversion workflows

## Features

### Core Functionality
- **Comprehensive Data Extraction** - Captures nodes, pins, graphs, variables, functions, and components
- **Structured Graph Analysis** - Preserves execution flow and data connections
- **Flexible Export Options** - Multiple export locations and organization schemes
- **Project Settings Integration** - Full configuration through Unreal's Project Settings
- **Console Commands** - Powerful command-line interface for automation

### Serialization Capabilities
- Graph structure with node positions
- Pin connections and data flow
- Variable declarations and metadata
- Function signatures and implementations
- Component hierarchies and properties
- Interface implementations
- Macro and function libraries

### AI/LLM Integration
- Generate context files for AI processing
- Customizable prompt templates
- Reference file inclusion
- Implementation notes generation

## Installation

### As Engine Plugin (Recommended)
1. Copy the `BlueprintSerializer` folder to: `[UE5_Install_Dir]/Engine/Plugins/Developer/`
2. Restart Unreal Engine
3. Enable the plugin in your project: Edit → Plugins → Developer → Blueprint Serializer

### As Project Plugin
1. Copy the `BlueprintSerializer` folder to: `[Your_Project]/Plugins/`
2. Regenerate project files
3. Compile your project

## Usage

### Console Commands

| Command | Description | Example |
|---------|-------------|---------|
| `BP_SLZR.Serialize` | Serialize a Blueprint to JSON | `BP_SLZR.Serialize /Game/Blueprints/MyActor` |
| `BS.Serialize` | Alias for `BP_SLZR.Serialize` | `BS.Serialize /Game/Blueprints/MyActor` |
| `BP_SLZR.Count` | Count all Blueprints in project | `BP_SLZR.Count` |
| `BP_SLZR.Settings` | Open plugin settings | `BP_SLZR.Settings` |
| `BP_SLZR.OpenFolder` | Open export directory | `BP_SLZR.OpenFolder` |
| `BP_SLZR.GenerateContext` | Generate LLM context file | `BP_SLZR.GenerateContext /Game/BP_Character` |
| `BP_SLZR.Version` | Show plugin version | `BP_SLZR.Version` |
| `BP_SLZR.Help` | Display command help | `BP_SLZR.Help` |

### Project Settings

Access settings via: **Edit → Project Settings → Plugins → Blueprint Serializer**

#### Export Settings
- **Export Mode** - Choose export location (Project, Engine, Documents, Temp, Custom)
- **Custom Path** - Specify custom export directory
- **Timestamped Folders** - Create time-based subdirectories
- **Organize by Blueprint** - Group exports by Blueprint name

#### Serialization Settings
- **Serialization Depth** - Control data detail level (Minimal, Standard, Extended, Complete)
- **Pretty Print** - Format JSON for readability
- **Include Metadata** - Export additional node information
- **Include Positions** - Save graph layout data

#### AI Integration
- **Generate LLM Context** - Create AI-ready context files
- **Include Reference Files** - Add C++ reference examples
- **Context Template** - Select prompt template
- **Implementation Notes** - Add conversion hints

### Tools Menu Integration

Access via: **Tools → Blueprint Serializer** in the main menu bar

- Serialize Selected Blueprint
- Open Settings
- Open Export Folder

## Workflow Examples

### Basic Blueprint Export
```
1. Open Blueprint in editor
2. Run console command: BP_SLZR.Serialize /Game/Blueprints/BP_MyActor
   - Alias: BS.Serialize /Game/Blueprints/BP_MyActor
3. JSON file created in: [Project]/Saved/BlueprintExports/
```

### AI-Assisted C++ Conversion
```
1. Configure AI settings in Project Settings
2. Run: BP_SLZR.GenerateContext /Game/Blueprints/BP_Character
3. Open generated context file
4. Feed to Claude or other LLM for C++ generation
```

### Batch Processing
```
1. Use BP_SLZR.Count to see total Blueprints
2. Write script using BP_SLZR.Serialize (or BS.Serialize) commands
3. Process JSON files with external tools
```

## Output Format

The plugin generates structured JSON with the following schema:

```json
{
  "BlueprintPath": "/Game/Blueprints/BP_Example",
  "BlueprintName": "BP_Example",
  "ParentClassName": "Actor",
  "Variables": [...],
  "Functions": [...],
  "Components": [...],
  "Graphs": [
    {
      "GraphName": "EventGraph",
      "Nodes": [...],
      "ExecutionFlows": [...],
      "DataFlows": [...]
    }
  ],
  "Metadata": {
    "ExtractionTimestamp": "2024-01-15T10:30:00",
    "EngineVersion": "5.5.0",
    "PluginVersion": "1.0.0"
  }
}
```

## Performance Considerations

- **Memory Limits** - Configure max memory usage in settings (default: 1024MB)
- **Node Limits** - Set maximum nodes per Blueprint (default: 5000)
- **Parallel Processing** - Enable for large Blueprints
- **Single Blueprint Mode** - Extract one Blueprint at a time for stability

## API Reference

### C++ Integration

Include the module in your Build.cs:
```csharp
PublicDependencyModuleNames.AddRange(new string[] { "BlueprintSerializer" });
```

Access the module:
```cpp
#include "BlueprintSerializerModule.h"

FBlueprintSerializerModule& Module = FBlueprintSerializerModule::Get();
```

### Data Structures

Key structures available in `BSDataTypes.h`:
- `FBSBlueprintData` - Complete Blueprint representation
- `FBSGraphData` - Graph structure with nodes
- `FBSNodeData` - Individual node information
- `FBSPinData` - Pin connections and types
- `FBSVariableInfo` - Variable declarations
- `FBSFunctionInfo` - Function signatures

## Troubleshooting

### Common Issues

**Plugin not visible in menu**
- Ensure plugin is enabled in Plugins window
- Restart editor after installation
- Check output log for loading errors

**Export directory not created**
- Verify write permissions
- Check settings for valid export path
- Use BP_SLZR.OpenFolder to create directory

**Large Blueprints fail to export**
- Increase memory limit in settings
- Reduce serialization depth
- Export one Blueprint at a time

**JSON validation errors**
- Enable auto-validation in settings
- Check for special characters in names
- Verify Blueprint compiles without errors

## Development

### Building from Source

1. Clone the repository
2. Copy to Engine/Plugins/Developer/
3. Generate project files
4. Build in Development Editor configuration

### Extending the Plugin

The plugin architecture supports extensions through:
- Custom serialization handlers
- Additional export formats
- New prompt templates
- External tool integration

## Version History

### v1.0.0 (Current)
- Initial release
- Core serialization functionality
- Project Settings integration
- Console commands
- LLM context generation

## License

This plugin is provided as-is for use with Unreal Engine projects. See LICENSE file for details.

## Support

For issues, feature requests, or contributions:
- GitHub: [BlueprintSerializer Repository](https://github.com/your-repo/BlueprintSerializer)
- Documentation: See `/Docs` folder

## Credits

Developed for Unreal Engine 5.5+
Based on insights from NodeToCode and other Blueprint analysis tools
