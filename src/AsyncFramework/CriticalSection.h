/***************************************************************************
 *   Copyright (C) 2009 by Stefan Fuhrmann                                 *
 *   stefanfuhrmann@alice-dsl.de                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#pragma once

namespace async
{

class CCriticalSection
{
private:

    // OS-specific critical section object

    CRITICAL_SECTION section;

public:

    // construction / destruction

    CCriticalSection(void);
    ~CCriticalSection(void);

    // Mutex functionality

    void Acquire();
    void Release();
};

// Mutex functionality

inline void CCriticalSection::Acquire()
{
    EnterCriticalSection (&section);
}

inline void CCriticalSection::Release()
{
    LeaveCriticalSection (&section);
}

class CCriticalSectionLock
{
private:

    CCriticalSection& section;

public:

    // RAII

    CCriticalSectionLock (CCriticalSection& section)
        : section (section)
    {
        section.Acquire();
    }

    ~CCriticalSectionLock()
    {
        section.Release();
    }
};

}
