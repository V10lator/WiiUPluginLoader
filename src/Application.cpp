/****************************************************************************
 * Copyright (C) 2015 Dimok
 * Modified by Maschell, 2018 for Wii U Plugin System loader
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#include "Application.h"
#include "common/common.h"
#include <dynamic_libs/os_functions.h>
#include <utils/logger.h>
#include "mymemory/memory_mapping.h"
#include "plugin/PluginLoader.h"
#include <vector>
#include "utils.h"
#include <iterator>
#include <fstream>

Application *Application::applicationInstance = NULL;

Application::Application() {
}

Application::~Application() {
}

int32_t Application::exec() {
        PluginLoader * pluginLoader  = PluginLoader::getInstance();
        std::vector<PluginInformation *> pluginList;
        
        if(isInMiiMakerHBL())
        {
                DEBUG_FUNCTION_LINE("HBL! Unloading...\n");
                std::vector<PluginInformation *> pluginList = pluginLoader->getPluginsLoadedInMemory();
                if(pluginList.size() != 0)
                {
                        pluginLoader->clearPluginInformation(pluginList);
                        pluginLoader->resetPluginLoader();
                }
                DEBUG_FUNCTION_LINE("Done!...\n");
                return APPLICATION_CLOSE_MIIMAKER;
        }
        
        pluginList = pluginLoader->getPluginInformation(WUPS_PLUGIN_PATH);
        DEBUG_FUNCTION_LINE("Found %i plugins at "WUPS_PLUGIN_PATH"\n", pluginList.size());
        
        std::ifstream cfgStream(WUPS_AUTO_CONF);
        if(cfgStream.fail())
                pluginLoader->loadAndLinkPlugins(pluginList);
        else
        {
                std::vector<PluginInformation *> toLoad;
                PluginInformation *plugin;
                std::string cfgEntry;
                while(getline(cfgStream, cfgEntry))
                {
                        for(std::vector<PluginInformation *>::iterator it = pluginList.begin() ; it != pluginList.end(); ++it)
                        {
                                plugin = *it;
                                if(plugin->getName().compare(cfgEntry) == 0)
                                        toLoad.push_back(plugin);
                        }
                }
                DEBUG_FUNCTION_LINE("Loading %i plugins\n", toLoad.size());
                pluginLoader->loadAndLinkPlugins(toLoad);
        }
        
        return APPLICATION_CLOSE_APPLY;
}
