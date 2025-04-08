#pragma once

#include <Dataset.h>
#include <PluginFactory.h>
#include <TransformationPlugin.h>

// =============================================================================
// Plugin 
// =============================================================================

/**
 * Combine data plugin
 *
 * Creates a new dataset with all data from the selection sets
 *
 * @authors Alex Vieth
 */
class CombineDataPlugin : public mv::plugin::TransformationPlugin
{
    Q_OBJECT

public:

    CombineDataPlugin(const mv::plugin::PluginFactory* factory);
    ~CombineDataPlugin() override = default;

    void init() override {};

    void transform() override;
};

// =============================================================================
// Plugin Factory 
// =============================================================================

class CombineDataPluginFactory : public mv::plugin::TransformationPluginFactory
{
    Q_INTERFACES(mv::plugin::TransformationPluginFactory mv::plugin::PluginFactory)
        Q_OBJECT
        Q_PLUGIN_METADATA(IID   "studio.manivault.CombineDataPlugin"
            FILE  "CombineDataPlugin.json")

public:

    /** Default constructor */
    CombineDataPluginFactory() {}

    /** Destructor */
    ~CombineDataPluginFactory() override {}

    /** Creates an instance of the example analysis plugin */
    mv::plugin::TransformationPlugin* produce() override;

    /** Returns the data types that are supported by the example analysis plugin */
    mv::DataTypes supportedDataTypes() const override;

    /** Enable right-click on data set to open analysis */
    mv::gui::PluginTriggerActions getPluginTriggerActions(const mv::Datasets& datasets) const override;
};