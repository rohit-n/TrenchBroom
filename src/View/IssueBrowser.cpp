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
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "IssueBrowser.h"

#include "Model/Issue.h"
#include "Model/IssueManager.h"
#include "View/ControllerFacade.h"
#include "View/MapDocument.h"

#include <wx/dataview.h>
#include <wx/menu.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/variant.h>

#include <cassert>

namespace TrenchBroom {
    namespace View {
        class IssueBrowserDataModel : public wxDataViewModel {
        private:
            Model::IssueManager& m_issueManager;
            bool m_showHidden;
        public:
            IssueBrowserDataModel(Model::IssueManager& issueManager) :
            m_issueManager(issueManager),
            m_showHidden(false) {
                bindObservers();
            }
            
            ~IssueBrowserDataModel() {
                unbindObservers();
            }
            
            unsigned int GetColumnCount() const {
                return 2;
            }
            
            wxString GetColumnType(const unsigned int col) const {
                assert(col < GetColumnCount());
                return "string";
            }
            
            bool IsContainer(const wxDataViewItem& item) const {
                if (!item.IsOk())
                    return true;
                return false;
            }
            
            unsigned int GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const {
                if (!item.IsOk()) {
                    Model::Issue* issue = m_issueManager.issues();
                    while (issue != NULL) {
                        if (m_showHidden || !issue->ignore())
                            children.Add(wxDataViewItem(reinterpret_cast<void*>(issue)));
                        issue = issue->next();
                    }
                }
                
                return children.size();
            }
            
            wxDataViewItem GetParent(const wxDataViewItem& item) const {
                return wxDataViewItem(NULL);
            }
            
            void GetValue(wxVariant& result, const wxDataViewItem& item, const unsigned int col) const {
                assert(col < GetColumnCount());
                assert(item.IsOk());

                const void* data = item.GetID();
                assert(data != NULL);
                const Model::Issue* issue = reinterpret_cast<const Model::Issue*>(data);
                
                if (col == 0) {
                    if (issue->filePosition() == 0) {
                        result = wxVariant("");
                    } else {
                        result = wxVariant(wxString() << issue->filePosition());
                    }
                } else {
                    result = wxVariant(wxString(issue->description()));
                }
            }
            
            bool SetValue(const wxVariant& value, const wxDataViewItem& item, const unsigned int col) {
                assert(col < GetColumnCount());
                return false;
            }
            
            void setShowHidden(const bool showHidden) {
                if (showHidden == m_showHidden)
                    return;
                m_showHidden = showHidden;
                Cleared();
                
                Model::Issue* issue = m_issueManager.issues();
                while (issue != NULL) {
                    addIssue(issue);
                    issue = issue->next();
                }
            }
            
            void refreshLineNumbers() {
                Model::Issue* issue = m_issueManager.issues();
                while (issue != NULL) {
                    if (m_showHidden || !issue->ignore())
                        ValueChanged(wxDataViewItem(reinterpret_cast<void*>(issue)), 0);
                    issue = issue->next();
                }
            }
        private:
            void bindObservers() {
                m_issueManager.issueWasAddedNotifier.addObserver(this, &IssueBrowserDataModel::issueWasAdded);
                m_issueManager.issueWillBeRemovedNotifier.addObserver(this, &IssueBrowserDataModel::issueWillBeRemoved);
                m_issueManager.issueIgnoreChangedNotifier.addObserver(this, &IssueBrowserDataModel::issueIgnoreChanged);
                m_issueManager.issuesClearedNotifier.addObserver(this, &IssueBrowserDataModel::issuesCleared);
            }
            
            void unbindObservers() {
                m_issueManager.issueWasAddedNotifier.removeObserver(this, &IssueBrowserDataModel::issueWasAdded);
                m_issueManager.issueWillBeRemovedNotifier.removeObserver(this, &IssueBrowserDataModel::issueWillBeRemoved);
                m_issueManager.issueIgnoreChangedNotifier.removeObserver(this, &IssueBrowserDataModel::issueIgnoreChanged);
                m_issueManager.issuesClearedNotifier.removeObserver(this, &IssueBrowserDataModel::issuesCleared);
            }
            
            void issueWasAdded(Model::Issue* issue) {
                assert(issue != NULL);
                addIssue(issue);
            }
            
            void issueWillBeRemoved(Model::Issue* issue) {
                assert(issue != NULL);
                removeIssue(issue);
            }
            
            void issueIgnoreChanged(Model::Issue* issue) {
                assert(issue != NULL);
                if (issue->ignore())
                    removeIssue(issue);
                else
                    addIssue(issue);
            }
            
            void issuesCleared() {
                Cleared();
            }

            void addIssue(Model::Issue* issue) {
                if (m_showHidden || !issue->ignore()) {
                    const bool success = ItemAdded(wxDataViewItem(NULL), wxDataViewItem(reinterpret_cast<void*>(issue)));
                    assert(success);
                }
            }
            
            void removeIssue(Model::Issue* issue) {
                const bool success = ItemDeleted(wxDataViewItem(NULL), wxDataViewItem(reinterpret_cast<void*>(issue)));
                assert(success);
            }
        };
        
        IssueBrowser::IssueBrowser(wxWindow* parent, MapDocumentWPtr document, ControllerWPtr controller) :
        wxPanel(parent),
        m_document(document),
        m_controller(controller),
        m_model(NULL),
        m_tree(NULL){
            m_model = new IssueBrowserDataModel(lock(document)->issueManager());
            m_tree = new wxDataViewCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES | wxDV_MULTIPLE | wxBORDER_SIMPLE);
            m_tree->AssociateModel(m_model);
            m_tree->AppendTextColumn("Line", 0)->SetWidth(50);
            m_tree->AppendTextColumn("Description", 1)->SetWidth(200);
            m_tree->Expand(wxDataViewItem(NULL));
            
            m_tree->Bind(wxEVT_SIZE, &IssueBrowser::OnTreeViewSize, this);
            m_tree->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &IssueBrowser::OnTreeViewContextMenu, this);

            wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
            sizer->Add(m_tree, 1, wxEXPAND);
            SetSizerAndFit(sizer);
            
            bindObservers();
        }
        
        IssueBrowser::~IssueBrowser() {
            unbindObservers();
        }

        Model::QuickFix::List collectQuickFixes(const wxDataViewItemArray& selections) {
            assert(!selections.empty());
            Model::QuickFix::List result = reinterpret_cast<Model::Issue*>(selections[0].GetID())->quickFixes();
            for (size_t i = 1; i < selections.size(); ++i) {
                const Model::QuickFix::List quickFixes = reinterpret_cast<Model::Issue*>(selections[0].GetID())->quickFixes();
                
                Model::QuickFix::List::iterator it = result.begin();
                while (it != result.end()) {
                    Model::QuickFix& quickFix = *it;
                    if (!VectorUtils::contains(quickFixes, quickFix))
                        it = result.erase(it);
                    else
                        ++it;
                }
            }
            return result;
        }
        
        void IssueBrowser::OnTreeViewContextMenu(wxDataViewEvent& event) {
            const wxDataViewItem& item = event.GetItem();
            if (!item.IsOk())
                return;
            if (!m_tree->IsSelected(item))
                return;
            
            wxDataViewItemArray selections;
            m_tree->GetSelections(selections);
            assert(!selections.empty());

            const Model::QuickFix::List quickFixes = collectQuickFixes(selections);

            wxMenu* quickFixMenu = new wxMenu();
            for (size_t i = 0; i < quickFixes.size(); ++i) {
                const Model::QuickFix& quickFix = quickFixes[i];
                quickFixMenu->Append(FixObjectsBaseId + i, quickFix.description());
            }
            
            wxMenu popupMenu;
            popupMenu.Append(SelectObjectsCommandId, "Select");
            popupMenu.Append(ShowIssuesCommandId, "Show");
            popupMenu.Append(HideIssuesCommandId, "Hide");
            popupMenu.AppendSubMenu(quickFixMenu, "Fix");
            
            popupMenu.Bind(wxEVT_COMMAND_MENU_SELECTED, &IssueBrowser::OnSelectIssues, this, SelectObjectsCommandId);
            popupMenu.Bind(wxEVT_COMMAND_MENU_SELECTED, &IssueBrowser::OnShowIssues, this, ShowIssuesCommandId);
            popupMenu.Bind(wxEVT_COMMAND_MENU_SELECTED, &IssueBrowser::OnHideIssues, this, HideIssuesCommandId);
            quickFixMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, &IssueBrowser::OnApplyQuickFix, this, FixObjectsBaseId, FixObjectsBaseId + quickFixes.size());
            
            PopupMenu(&popupMenu);
        }
        
        void IssueBrowser::OnSelectIssues(wxCommandEvent& event) {
            View::ControllerSPtr controller = lock(m_controller);
            controller->beginUndoableGroup("Select fixable objects");

            wxDataViewItemArray selections;
            m_tree->GetSelections(selections);
            selectIssueObjects(selections, controller);
            
            controller->closeGroup();
        }
        
        void IssueBrowser::OnShowIssues(wxCommandEvent& event) {
            setIssueVisibility(true);
        }
        
        void IssueBrowser::OnHideIssues(wxCommandEvent& event) {
            setIssueVisibility(false);
        }

        void IssueBrowser::OnApplyQuickFix(wxCommandEvent& event) {
            wxDataViewItemArray selections;
            m_tree->GetSelections(selections);
            assert(!selections.empty());
            
            Model::QuickFix::List quickFixes = collectQuickFixes(selections);
            const size_t index = static_cast<size_t>(event.GetId()) - FixObjectsBaseId;
            assert(index < quickFixes.size());
            Model::QuickFix& quickFix = quickFixes[index];
            
            View::ControllerSPtr controller = lock(m_controller);
            controller->beginUndoableGroup("");

            selectIssueObjects(selections, controller);
            
            for (size_t i = 0; i < selections.size(); ++i) {
                const wxDataViewItem& item = selections[i];
                void* data = item.GetID();
                assert(data != NULL);
                Model::Issue* issue = reinterpret_cast<Model::Issue*>(data);
                
                issue->select(controller);
                quickFix.apply(*issue, controller);
            }
            
            controller->closeGroup();
            
            m_tree->UnselectAll();
            m_tree->SetSelections(selections);
        }
        
        void IssueBrowser::OnTreeViewSize(wxSizeEvent& event) {
            const int scrollbarWidth = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X);
            const int newWidth = std::max(0, m_tree->GetClientSize().x - m_tree->GetColumn(0)->GetWidth() - scrollbarWidth);
            m_tree->GetColumn(1)->SetWidth(newWidth);
            event.Skip();
        }

        void IssueBrowser::selectIssueObjects(const wxDataViewItemArray& selections, View::ControllerSPtr controller) {
            controller->deselectAll();
            
            for (size_t i = 0; i < selections.size(); ++i) {
                const wxDataViewItem& item = selections[i];
                void* data = item.GetID();
                assert(data != NULL);
                Model::Issue* issue = reinterpret_cast<Model::Issue*>(data);
                issue->select(controller);
            }
        }

        void IssueBrowser::bindObservers() {
            MapDocumentSPtr document = lock(m_document);
            document->documentWasSavedNotifier.addObserver(this, &IssueBrowser::documentWasSaved);
        }
        
        void IssueBrowser::unbindObservers() {
            if (!expired(m_document)) {
                MapDocumentSPtr document = lock(m_document);
                document->documentWasSavedNotifier.removeObserver(this, &IssueBrowser::documentWasSaved);
            }
        }
        
        void IssueBrowser::documentWasSaved() {
            m_model->refreshLineNumbers();
        }

        void IssueBrowser::setIssueVisibility(const bool show) {
            wxDataViewItemArray selections;
            m_tree->GetSelections(selections);
            
            MapDocumentSPtr document = lock(m_document);
            Model::IssueManager& issueManager = document->issueManager();
            
            for (size_t i = 0; i < selections.size(); ++i) {
                const wxDataViewItem& item = selections[i];
                void* data = item.GetID();
                assert(data != NULL);
                Model::Issue* issue = reinterpret_cast<Model::Issue*>(data);
                issueManager.setIgnoreIssue(issue, !show);
            }
            
            document->incModificationCount();
            m_tree->UnselectAll();
        }
    }
}
