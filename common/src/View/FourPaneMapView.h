/*
 Copyright (C) 2010-2014 Kristian Duske
 
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

#ifndef __TrenchBroom__FourPaneMapView__
#define __TrenchBroom__FourPaneMapView__

#include "View/MapViewContainer.h"
#include "View/ViewTypes.h"

#include <wx/panel.h>

namespace TrenchBroom {
    class Logger;
    
    namespace Renderer {
        class MapRenderer;
    }
    
    namespace View {
        class GLContextManager;
        class MapViewBase;
        class MapView2D;
        class MapView3D;
        class MapViewToolBox;
        
        class FourPaneMapView : public MapViewContainer {
        private:
            Logger* m_logger;
            MapDocumentWPtr m_document;
            
            MapView3D* m_mapView3D;
            MapView2D* m_mapViewXY;
            MapView2D* m_mapViewXZ;
            MapView2D* m_mapViewYZ;
        public:
            FourPaneMapView(wxWindow* parent, Logger* logger, MapDocumentWPtr document, MapViewToolBox& toolBox, Renderer::MapRenderer& mapRenderer, GLContextManager& contextManager);
        private:
            void createGui(MapViewToolBox& toolBox, Renderer::MapRenderer& mapRenderer, GLContextManager& contextManager);
        private:
            MapViewBase* currentMapView() const;
        private: // implement MapViewContainer interface
            void doSetToolBoxDropTarget();
            void doClearDropTarget();
        private: // implement MapView interface
            Vec3 doGetPasteObjectsDelta(const BBox3& bounds) const;
            
            void doCenterCameraOnSelection();
            void doMoveCameraToPosition(const Vec3& position);
            
            void doMoveCameraToCurrentTracePoint();
        };
    }
}

#endif /* defined(__TrenchBroom__FourPaneMapView__) */
