// This file is part of OpenPanel - The Open Source Control Panel
// OpenPanel is free software: you can redistribute it and/or modify it 
// under the terms of the GNU General Public License as published by the Free 
// Software Foundation, using version 3 of the License.
//
// Please note that use of the OpenPanel trademark may be subject to additional 
// restrictions. For more information, please visit the Legal Information 
// section of the OpenPanel website on http://www.openpanel.com/


#ifndef _networkingModule_H
#define _networkingModule_H 1

#include <opencore/moduleapp.h>
#include <grace/system.h>
#include <grace/configdb.h>


typedef configdb<class networkingModule> appconfig;

//  -------------------------------------------------------------------------
/// Main application class.
//  -------------------------------------------------------------------------
class networkingModule : public moduleapp
{
public:
		 	 networkingModule (void) :
				moduleapp ("openpanel.module.networking"),
				conf (this)
			 {
			 }
			~networkingModule (void)
			 {
			 }

	int		 main (void);


	
protected:

	appconfig		conf;			///< Modules configuration data
	value			networkconf;	///< Zone configuration

	
			 //	 =============================================
			 /// Configuration handler 
			 //	 =============================================
	bool     confSystem (config::action act, keypath &path, 
					  	 const value &nval, const value &oval);	
	
			 //	=============================================
			 /// validate the given configuration
			 /// \return true on ok / false on failure
			 //	=============================================
	bool	 checkconfig (value &ibody);	
	
			 //	=============================================
			 /// Read the current loaded network module
			 /// and sends the network configuration to the 
			 /// standard output or gives an error.
			 /// \return true on ok / false on failure
			 //	=============================================
	bool	 readconfiguration (void);
	
			 //	=============================================
			 /// Writes a network configuration
			 /// \param v given post data
			 /// \return true on ok / false on failure
			 //	=============================================
	bool 	 writeconfiguration (const value &v);
	
	
			 //	=============================================
			 /// Complete the configuration before writing
			 /// it to disk
			 /// \param v The post data
			 /// \return true on ok / false on failure
			 //	=============================================
	bool 	 completeconfig (value &v);
	
	
	void	 listobjects (const string &, const statstring &);
	value	*objectlist (const string &, const statstring &);
	value	*getInterfaceList (void);
	string	*getDefaultGatewayIPv4 (void);
	string	*getDefaultGatewayIPv6 (void);
};

#endif
