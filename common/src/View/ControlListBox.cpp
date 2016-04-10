/*
 Copyright (C) 2010-2016 Kristian Duske
 
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

#include "ControlListBox.h"

#include "View/BorderLine.h"
#include "View/ViewConstants.h"

#include <wx/listbox.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/panel.h>

namespace TrenchBroom {
    namespace View {
        ControlListBox::ControlListBox(wxWindow* parent, const wxString& emptyText) :
        wxScrolledCanvas(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxVSCROLL),
        m_emptyText(emptyText),
        m_selectionIndex(0) {
            SetScrollRate(0, 10);
            ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_ALWAYS);
            SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX));
            SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT));
            Bind(wxEVT_LEFT_DOWN, &ControlListBox::OnClickList, this);
            SetItemCount(0);
        }
        
        int ControlListBox::GetSelection() const {
            if (m_selectionIndex == m_items.size())
                return  wxNOT_FOUND;
            return static_cast<int>(m_selectionIndex);
        }

        void ControlListBox::SetItemCount(const size_t itemCount) {
            if (m_selectionIndex == m_items.size())
                m_selectionIndex = itemCount;
            else
                m_selectionIndex = wxMin(itemCount, m_selectionIndex);
            refresh(itemCount);
            setSelection(m_selectionIndex);
            Refresh();
        }

        void ControlListBox::SetSelection(const int index) {
            if (index < 0 || static_cast<size_t>(index) > m_items.size())
                setSelection(m_items.size());
            else
                setSelection(static_cast<size_t>(index));
        }

        class ControlListBox::Sizer : public wxBoxSizer {
        public:
            Sizer(const int orient) :
            wxBoxSizer(orient) {}
            
            wxSize CalcMin() {
                const wxSize originalSize = wxBoxSizer::CalcMin();
                const wxSize containerSize = GetContainingWindow()->GetClientSize();
                const wxSize result(containerSize.x, originalSize.y);
                return result;
            }
        };

        class ControlListBox::Container : public wxWindow {
        public:
            Container(wxWindow* parent) :
            wxWindow(parent, wxID_ANY) {}
            
            bool AcceptsFocus() const {
                return false;
            }
        };

        void ControlListBox::refresh(const size_t itemCount) {
            SetSizer(NULL);
            DestroyChildren();
            m_items.clear();
            m_items.reserve(itemCount);
            
            if (itemCount > 0) {
                wxSizer* listSizer = new Sizer(wxVERTICAL);
                
                for (size_t i = 0; i < itemCount; ++i) {
                    wxWindow* itemContainer = new Container(this);
                    wxWindow* item = createItem(itemContainer, i);
                    
                    wxSizer* itemSizer = new wxBoxSizer(wxVERTICAL);
                    itemSizer->AddSpacer(LayoutConstants::WideVMargin);
                    itemSizer->Add(item, 0, wxEXPAND | wxLEFT | wxRIGHT, LayoutConstants::WideHMargin);
                    itemSizer->AddSpacer(LayoutConstants::WideVMargin);
                    itemContainer->SetSizer(itemSizer);
                    
                    listSizer->Add(itemContainer, 0, wxEXPAND);
                    listSizer->Add(new BorderLine(this, BorderLine::Direction_Horizontal), 0, wxEXPAND);
                    
                    bindEvents(itemContainer, i);
                    m_items.push_back(itemContainer);
                }
                
                listSizer->AddStretchSpacer();
                SetSizer(listSizer);
                Layout();
            } else if (!m_emptyText.empty()) {
                wxStaticText* emptyText = new wxStaticText(this, wxID_ANY, m_emptyText);
                emptyText->SetFont(emptyText->GetFont().Larger().Bold());
                emptyText->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
                
                wxSizer* justifySizer = new wxBoxSizer(wxHORIZONTAL);
                justifySizer->AddStretchSpacer();
                justifySizer->AddSpacer(LayoutConstants::WideHMargin);
                justifySizer->Add(emptyText, 0, wxEXPAND);
                justifySizer->AddSpacer(LayoutConstants::WideHMargin);
                justifySizer->AddStretchSpacer();
                
                wxSizer* listSizer = new wxBoxSizer(wxVERTICAL);
                listSizer->AddSpacer(LayoutConstants::WideVMargin);
                listSizer->Add(justifySizer, 0, wxEXPAND);
                listSizer->AddSpacer(LayoutConstants::WideVMargin);
                listSizer->AddStretchSpacer();
                
                SetSizer(listSizer);
                Layout();
            }
        }
        
        void ControlListBox::bindEvents(wxWindow* window, const size_t itemIndex) {
            if (window->IsFocusable()) {
                window->Bind(wxEVT_SET_FOCUS, &ControlListBox::OnFocusChild, this, wxID_ANY, wxID_ANY, new wxVariant(long(itemIndex)));
            } else {
                window->Bind(wxEVT_LEFT_DOWN, &ControlListBox::OnClickChild, this, wxID_ANY, wxID_ANY, new wxVariant(long(itemIndex)));
                window->Bind(wxEVT_LEFT_DCLICK, &ControlListBox::OnDoubleClickChild, this);
            }
            
            const wxWindowList& children = window->GetChildren();
            wxWindowList::const_iterator it, end;
            for (it = children.begin(), end = children.end(); it != end; ++it) {
                wxWindow* child = *it;
                bindEvents(child, itemIndex);
            }
        }

        void ControlListBox::OnFocusChild(wxFocusEvent& event) {
            setSelection(event);
            event.Skip();
        }
        
        void ControlListBox::OnClickChild(wxMouseEvent& event) {
            setSelection(event);
        }

        void ControlListBox::OnDoubleClickChild(wxMouseEvent& event) {
            wxCommandEvent* command = new wxCommandEvent(wxEVT_LISTBOX_DCLICK, GetId());
            command->SetInt(GetSelection());
            command->SetEventObject(this);
            QueueEvent(command);
        }

        void ControlListBox::OnClickList(wxMouseEvent& event) {
            setSelection(m_items.size());
        }

        void ControlListBox::setSelection(const wxEvent& event) {
            const wxVariant* variant = static_cast<wxVariant*>(event.GetEventUserData());
            const size_t itemIndex = static_cast<size_t>(variant->GetLong());
            setSelection(itemIndex);
        }

        void ControlListBox::setSelection(const size_t index) {
            assert(index <= m_items.size());
            const bool changed = m_selectionIndex != index;
            m_selectionIndex = index;

            for (size_t i = 0; i < m_items.size(); ++i)
                setColours(m_items[i], GetForegroundColour(), GetBackgroundColour());
            
            if (m_selectionIndex < m_items.size())
                setColours(m_items[m_selectionIndex], wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXHIGHLIGHTTEXT), wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
            
            if (changed) {
                wxCommandEvent* command = new wxCommandEvent(wxEVT_LISTBOX, GetId());
                command->SetInt(GetSelection());
                command->SetEventObject(this);
                QueueEvent(command);
            }
        }
        
        void ControlListBox::setColours(wxWindow* window, const wxColour& foreground, const wxColour& background) {
            if (!window->IsFocusable()) {
                window->SetForegroundColour(foreground);
                window->SetBackgroundColour(background);
            }

            const wxWindowList& children = window->GetChildren();
            wxWindowList::const_iterator it, end;
            for (it = children.begin(), end = children.end(); it != end; ++it) {
                wxWindow* child = *it;
                setColours(child, foreground, background);
            }
        }
    }
}