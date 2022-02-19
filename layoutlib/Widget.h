#ifndef __LAYOUT__WIDGET_H__
#define __LAYOUT__WIDGET_H__

//==============================================================================
//
//               Widget - the track class in the LAYOUT library
//
//               Copyright (C) 2013  Dick van Oudheusden
//  
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free
// Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//==============================================================================
// 
//  $Date$ $Revision$
//
//==============================================================================

#include "export.h"
#include "Node.h"

#include "String_.h"
#include "Unsigned.h"
#include "List.h"


namespace layout
{
  ///
  /// @class Widget
  ///
  /// @brief The track class.
  ///
  
  class DLL_API Widget : public Node
  {
    public:

    ///
    /// Constructor
    ///
    /// @param  parent     the parent node
    /// @param  name       the name of the attribute or element
    /// @param  type       the node type (ATTRIBUTE or ELEMENT)
    /// @param  mandatory  is the attribute or element mandatory ?
    ///
    Widget(Node *parent, const char *name, Type type, bool mandatory = false);

    ///
    /// Deconstructor
    ///
    virtual ~Widget();
    
    ///
    /// Get name
    ///
    /// @return the name element
    ///
    String  &name() { return _name; }
    
    ///
    /// Get type
    ///
    /// @return the type element
    ///
    String  &type() { return _type; }

    ///
    /// Get align
    ///
    /// @return the align attribute
    ///
    String  &align() { return _align; }

    ///
    /// Get units
    ///
    /// @return the units element
    ///
    String  &units() { return _units; }

    ///
    /// Get format
    ///
    /// @return the format element
    ///
    String  &format() { return _format; }

    ///
    /// Get x
    ///
    /// @return the x attribute
    ///
    Unsigned  &x() { return _x; }

    ///
    /// Get y
    ///
    /// @return the y attribute
    ///
    Unsigned  &y() { return _y; }

    ///
    /// Get width
    ///
    /// @return the width attribute
    ///
    Unsigned  &width() { return _width; }

    ///
    /// Get height
    ///
    /// @return the height attribute
    ///
    Unsigned  &height() { return _height; }

    ///
    /// Get margin
    ///
    /// @return the margin attribute
    ///
    Unsigned  &margin() { return _margin; }

    ///
    /// Get padding
    ///
    /// @return the padding attribute
    ///
    Unsigned  &padding() { return _padding; }

    ///
    /// Get text color
    ///
    /// @return the text color element
    ///
    String  &textColor() { return _txtcolor; }

    ///
    /// Get text shadow
    ///
    /// @return the text shadow attribute
    ///
    Unsigned  &textShadow() { return _txtshadow; }

    ///
    /// Get border
    ///
    /// @return the border attribute
    ///
    Unsigned  &border() { return _border; }

    ///
    /// Get border color
    ///
    /// @return the border color element
    ///
    String  &borderColor() { return _bordercolor; }

    ///
    /// Get background color
    ///
    /// @return the background color element
    ///
    String  &backgroundColor() { return _bgcolor; }

    // Methods

    private:
    
    // Members
    String       _name;
    String       _type;
    String       _align;
    String       _units;
    String       _format;
    Unsigned     _x, _y;
	Unsigned     _width, _height;
	Unsigned     _margin;
	Unsigned     _padding;
	String       _txtcolor;
	Unsigned     _txtshadow;
	Unsigned     _border;
	String       _bordercolor;
	String       _bgcolor;
    
    // Disable copy constructors
    Widget(const Widget &);
    Widget& operator=(const Widget &);  
  };
}

#endif

