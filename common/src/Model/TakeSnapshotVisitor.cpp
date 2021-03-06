/*
 Copyright (C) 2010-2017 Kristian Duske
 
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
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TakeSnapshotVisitor.h"

#include "Model/Brush.h"
#include "Model/Entity.h"
#include "Model/Group.h"
#include "Model/Layer.h"
#include "Model/World.h"

namespace TrenchBroom {
    namespace Model {
        const NodeSnapshotList& TakeSnapshotVisitor::result() const {
            return m_result;
        }

        void TakeSnapshotVisitor::doVisit(World* world)   { handleNode(world); }
        void TakeSnapshotVisitor::doVisit(Layer* layer)   { handleNode(layer); }
        void TakeSnapshotVisitor::doVisit(Group* group)   { handleNode(group); }
        void TakeSnapshotVisitor::doVisit(Entity* entity) { handleNode(entity); }
        void TakeSnapshotVisitor::doVisit(Brush* brush)   { handleNode(brush); }
        
        void TakeSnapshotVisitor::handleNode(Node* node) {
            NodeSnapshot* snapshot = node->takeSnapshot();
            if (snapshot != NULL)
                m_result.push_back(snapshot);
        }
    }
}
