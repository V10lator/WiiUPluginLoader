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

Application *Application::applicationInstance = NULL;

Application::Application() {
}

Application::~Application() {
}

int32_t Application::exec() {
        PluginLoader * pluginLoader  = PluginLoader::getInstance();
        std::vector<PluginInformation *> pluginList = pluginLoader->getPluginInformation(WUPS_PLUGIN_PATH);
        std::vector<PluginInformation *> pluginListLoaded = pluginLoader->getPluginsLoadedInMemory();
        
        for (std::vector<PluginInformation *>::iterator it = pluginList.begin() ; it != pluginList.end(); ++it) {
                PluginInformation * curPlugin = *it;
                bool skip = false;
                
                for (std::vector<PluginInformation *>::iterator itOther = pluginListLoaded.begin() ; itOther != pluginListLoaded.end(); ++itOther) {
                        PluginInformation * otherPlugin = *itOther;
                        if(otherPlugin->getPath().compare(curPlugin->getPath()) == 0) {
                                skip = true;
                                break;
                        }
                }
                if(skip)
                        continue;
                
                std::vector<PluginInformation*> willBeLoaded;

                DEBUG_FUNCTION_LINE("We want to link %s\n",curPlugin->getName().c_str());
                willBeLoaded.push_back(curPlugin);
                PluginLoader::getInstance()->loadAndLinkPlugins(willBeLoaded);
        }
        
        pluginLoader->clearPluginInformation(pluginListLoaded);
        
        auto fp = std::bind(&ContentHome::linkPlugins, this);
        Application::instance()->setLinkPluginsCallback(fp);
        
        if(fp != NULL) {
                std::function<bool(void)> f = fp;
                f();
        }
        
        pluginLoader->clearPluginInformation(pluginList);
        
        return EXIT_RELAUNCH_ON_LOAD;
}
