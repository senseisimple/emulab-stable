////////////////////////////////////////////////////////////////////////////////
//
//  This program is free software; you can redistribute it and/or modify 
//  it under the terms of the GNU General Public License as published by the 
//  Free Software Foundation; either version 3 of the License, or (at your 
//  option) any later version.
//
//  This program is distributed in the hope that it will be useful, but 
//  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
//  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
//  for more details.
//
//  You should have received a copy of the GNU General Public License along 
//  with this program; if not, see <http://www.gnu.org/licenses>.
//
////////////////////////////////////////////////////////////////////////////////

package merapi.examples.helloworld.messages
{

import merapi.messages.Message;

[RemoteClass( alias="merapi.examples.helloworld.messages.HelloWorldMessage" )]
/**
 *  The <code>HelloWorldMessage</code> class extends <code>Message</code> as an example of a 
 *  custom Merapi message.
 *
 *  @see merapi.messages.Message;
 */
public class HelloWorldMessage extends Message
{
    //--------------------------------------------------------------------------
    //
    //  Constants
    //
    //--------------------------------------------------------------------------

    /**
     *  Message type for a Hello World message.
     */
    public static const HELLO : String = "hello";
     
            
    //--------------------------------------------------------------------------
    //
    //  Constructor
    //
    //--------------------------------------------------------------------------

    /**
     *  Constructor.
     */     
    public function HelloWorldMessage() 
    {
        super( HELLO );
    }
    
    
    //--------------------------------------------------------------------------
    //
    //  Properties
    //
    //--------------------------------------------------------------------------

    //----------------------------------
    //  helloText
    //----------------------------------

    /**
     *  A custom data property that contains the Hello World message
     */     
    public var helloText : String = null;
    
    
} //  end class
} //  end package

