/*
 Copyright (C) 2010-2013 Kristian Duske
 
 This file is part of TrenchBroom.
 
 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with TrenchBroom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "QuakeGame.h"

#include "StringUtils.h"
#include "IO/FileSystem.h"
#include "IO/Path.h"
#include "IO/QuakeMapParser.h"
#include "IO/WadTextureLoader.h"
#include "Model/Entity.h"
#include "Model/EntityProperties.h"

namespace TrenchBroom {
    namespace Model {
        const BBox3 QuakeGame::WorldBounds = BBox3(Vec3(-16384.0, -16384.0, -16384.0),
                                                   Vec3(+16384.0, +16384.0, +16384.0));
        
        Game::Ptr QuakeGame::newGame() {
            return Ptr(new QuakeGame());
        }

        Map::Ptr QuakeGame::doLoadMap(const IO::Path& path) const {
            IO::FileSystem fs;
            IO::MappedFile::Ptr file = fs.mapFile(path, std::ios::in);
            IO::QuakeMapParser parser(file->begin(), file->end());
            return parser.parseMap(WorldBounds);
        }

        IO::Path::List QuakeGame::doExtractTexturePaths(Model::Map::Ptr map) const {
            IO::Path::List paths;
            
            Entity::Ptr worldspawn = map->worldspawn();
            if (worldspawn == NULL)
                return paths;
            
            const Model::PropertyValue& wadValue = worldspawn->property(Model::PropertyKeys::Wad);
            if (wadValue.empty())
                return paths;
            
            const StringList pathStrs = StringUtils::split(wadValue, ';');
            StringList::const_iterator it, end;
            for (it = pathStrs.begin(), end = pathStrs.end(); it != end; ++it) {
                const String pathStr = StringUtils::trim(*it);
                if (!pathStr.empty()) {
                    const IO::Path path(pathStr);
                    paths.push_back(path);
                }
            }
            
            return paths;
        }

        TextureCollection::Ptr QuakeGame::doLoadTextureCollection(const IO::Path& path) const {
            IO::WadTextureLoader loader;
            return loader.loadTextureCollection(path);
        }

        void QuakeGame::doUploadTextureCollection(TextureCollection::Ptr collection) const {
            IO::WadTextureLoader loader;
            loader.uploadTextureCollection(collection);
        }
    }
}