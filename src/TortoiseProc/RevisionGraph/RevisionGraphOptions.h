// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#pragma once

class CFullGraphNode;
class CVisibleGraph;
class CVisibleGraphNode;

class IRevisionGraphOption
{
public:

    virtual WORD CommandID() const = 0;
    virtual int Priority() const = 0; 

    virtual bool IsAvailable() const = 0;
    virtual bool IsSelected() const = 0;
    virtual bool IsActive() const = 0; 

    virtual void ToggleSelection() = 0;

};

class IOrderedTraversalOption : public IRevisionGraphOption
{
public:

    virtual bool WantsCopiesFirst() const = 0;
    virtual bool WantsRootFirst() const = 0;

};

class CRevisionGraphOptionList
{
private:

    /// (only) this base class owns all the options

    std::vector<IRevisionGraphOption*> options;

protected:

    /// options owner and filter management

    template<class T, class I> T GetFilteredList() const;

    /// utility method

    IRevisionGraphOption* GetOptionByID (WORD id) const;

    /// construction / destruction (frees all owned options)

    CRevisionGraphOptionList();
    virtual ~CRevisionGraphOptionList();

public:

    /// find a particular option

    template<class T> T* GetOption() const;

    /// called by CRevisionGraphOption constructor

    void Add (IRevisionGraphOption* option);

    /// menu interaction

    bool IsAvailable (WORD id) const;
    bool IsSelected (WORD id) const;

    void ToggleSelection (WORD id);

    /// registry encoding

    DWORD GetRegistryFlags() const;
    void SetRegistryFlags (DWORD flags);
};

// options owner and filter management

template<class I>
bool AscendingPriority (I* lhs, I* rhs)
{
    return lhs->Priority() < rhs->Priority();
}

template<class T, class I> 
T CRevisionGraphOptionList::GetFilteredList() const
{
    // get filtered options

    std::vector<I*> filteredOptions;
    for (size_t i = 0, count = options.size(); i < count; ++i)
    {
        I* option = dynamic_cast<I*>(options[i]);
        if ((option != NULL) && options[i]->IsActive())
            filteredOptions.push_back (option);
    }

    // sort them by priority

    std::stable_sort ( filteredOptions.begin()
                     , filteredOptions.end()
                     , AscendingPriority<I>);

    // create list

    return T (filteredOptions);
}

// find a particular option

template<class T> 
T* CRevisionGraphOptionList::GetOption() const
{
    for (size_t i = 0, count = options.size(); i < count; ++i)
    {
        T* result = dynamic_cast<T*>(options[i]);
        if (result != NULL)
            return result;
    }

    // should not happen 

    assert (0);

    return NULL;
}
