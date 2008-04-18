// --------------------------------------------------------------------------
// OpenPanel - The Open Source Control Panel
// Copyright (c) 2006-2007 PanelSix
//
// This software and its source code are subject to version 2 of the
// GNU General Public License. Please be aware that use of the OpenPanel
// and PanelSix trademarks and the IconBase.com iconset may be subject
// to additional restrictions. For more information on these restrictions
// and for a copy of version 2 of the GNU General Public License, please
// visit the Legal and Privacy Information section of the OpenPanel
// website on http://www.openpanel.com/
// --------------------------------------------------------------------------

#include <moduleapp.h>
#include "networkingModule.h"

#include <grace/file.h>
#include <grace/filesystem.h>
#include <grace/process.h>
#include <grace/strutil.h>

APPOBJECT(networkingModule);

//  =========================================================================
/// Main method.
//  =========================================================================
int networkingModule::main (void)
{
	string conferr;

	// Add configuration watcher
	conf.addwatcher ("config", &networkingModule::confSystem);


   // Load will fail if watchers did not valiate.
    if (! conf.load ("openpanel.module.networking", conferr))
    {   
        ferr.printf ("%% Error loading configuration: %s\n", conferr.str());
        return 1;
    } 	
 	
 	
	
// DEBUG	
//	data.loadxml ("test/create.xml");
	
	//
	// Depend the actions which has to 
	// be executed
	//
	caseselector (data["OpenCORE:Command"])
	{
		incaseof ("listobjects") :
			listobjects ();
			break;
			
		incaseof ("getconfig") :
		 	readconfiguration ();
		 	return 0;
		 	
		incaseof ("validate") : ;
			
		defaultcase:
			sendresult (moderr::err_command, 
						"Unsupported command");
			return 0;
	}

	// send quit
	if ( authd.quit () )
	{
		sendresult( moderr::err_authdaemon, 
					"Error authd/quit cmd, possible rollback done");
			
		return false;		
	}

	sendresult (moderr::ok, "");
				
	return 0;
}

value *networkingModule::objectlist (void)
{
	returnclass (value) iflist retain;

	try
	{
		string ln;
		string cmd = "/sbin/ifconfig";
		systemprocess P (cmd);
		P.run ();
		
		while (! P.eof())
		{
			ln = P.gets ();
			value words = strutil::splitspace (ln);
			
			if (words[0].sval().strncmp ("eth", 3) == 0)
			{
				unsigned long long ifkey = checksum64 (words[0].cval());
				
				string uuid;
				// 1fc04f1g
				// 1facecf9
				uuid = "1facecf9-0000-0000-%04x-%04x%08x" %format
					   ((ifkey & 0xffff000000000000LL) >> 48,
					    (ifkey & 0x0000ffff00000000LL) >> 32,
					    (ifkey & 0x00000000ffffffffLL));
				
				string iface = words[0];
				string mac = words[4];
				
				ln = P.gets ();
				words = strutil::splitspace (ln);
				
				string ip = words[1];
				string bcast = words[2];
				string mask = words[3];
				
				ip.cropafterlast (':');
				bcast.cropafterlast (':');
				mask.cropafterlast (':');
				
				iflist[iface] = $("address", ip) ->
								$("id", iface) ->
								$("metaid", iface) ->
								$("uuid", uuid) ->
								$("netmask", mask) ->
								$("mac", mac) ->
								$("type", "Ethernet") ->
								$("onboot", true);
			}
			
			while (ln.strlen() && (! P.eof())) ln = P.gets ();
		}

		P.close ();
		P.serialize ();
	
	}
	catch (...)
	{
	}
	
	return &iflist;
}

ipaddress networkingModule::getgateway (void)
{
	ipaddress res = 0;
	string routes = fs.load ("/proc/net/route");
	value ln = strutil::splitlines (routes);
	
	foreach (line, ln)
	{
		value v = strutil::splitspace (line);
		if (v[1] == "00000000")
		{
			res = ntohl ((unsigned int) v[2].sval().toint(16));
		}
	}
	
	return res;
}

void networkingModule::listobjects (void)
{
	value iflist = objectlist ();

	value out;
	iflist("type") = "class";
	out["objects"]["Network:Interface"] = iflist;
	
	authd.quit ();
	sendresult (moderr::ok, "OK", out);
}

//	=============================================
//	METHOD: networkingModule::readconfiguration
//	=============================================
bool networkingModule::readconfiguration (void)
{
	// In case we are running on an redhat distribution
	// we should read the config files a different way
	
	value		netdata;
	file		f;
	statstring 	general = "general";

#ifdef __FLAVOR_LINUX_REDHAT	
	value		entries;
	
	// inside the file we have to check 
	string generalcfg;
		
	if (! f.openread (conf["config"]["system:network"].sval()))
	{
		sendresult (moderr::err_module, 
					"Error opening systems configuration");
		return false;
	}
	
	value 	tmpdata;
	
	while (! f.eof())
	{
		string line, key;
		
		line = f.gets ();
		line = line.stripchars (" \t");
		
		// Split the string and check if there 
		// has been set an value
		key = line.cutat ('=');
		
		// string will be empty on failure
		if (key.strlen() > 0)
		{
			tmpdata[key.str()] = line;
		}					
	}
	
	f.close ();
	
	
	// Finish the network configuration
	// with the last imported general data
	
	netdata[general]["networking"] = tmpdata["NETWORKING"].sval() == "yes" ?
										  "true" : "false";
	netdata[general]["hostname"] = tmpdata["HOSTNAME"].sval();
	netdata[general]["forward_ipv4"] = tmpdata["FORWARD_IPV4"].sval();
	
	netdata[general]["gateway"] = getgateway ();

#elif __FLAVOR_LINUX_DEBIAN // FIXME: this is *not* an ifdef check

	// actualy two config file need to be parsed
	// /etc/network/interfaces to get all network interface data
	// /etc/hostname to get the configured hostname of this machine
	
	// Now read the hostname from the
	// system config
	if (f.openread (conf["config"]["system:hostname"].sval()))
	{
		if (! f.eof())
			netdata[general]["hostname"] = f.gets ();
		
		f.close ();	
	}
	else
	{
		sendresult (moderr::err_module, 
					"Error opening systems configuration");
		return false;
	}
	

#endif


	// Format the data to post back including the right 
	// parent classes e.a.
	value retval;
	
	retval.type ("opencore.module");
	
	retval["Network"]("type") = "class";
	retval["Network"]["net"] = netdata[general];

	retval["Network"]["net"]["Network:Interface"] = objectlist ();
	retval["Network"]["net"]["Network:Interface"]("type") = "class";

	// DEBUG
	retval["OpenCORE:Result"]["code"] 	= moderr::ok;
	retval["OpenCORE:Result"]["message"] = "OK";
	
	string result;
	result = retval.toxml (false);
	fout.printf ("%u\n", result.strlen());
	fout.puts (result);

	return true;
}


// PROBABLY THIS FUNCTION ONT BE USED!!!!!!
//	==============================================
//	METHOD:
//	==============================================

bool networkingModule::completeconfig (value &v) 
{
	// Add all values which are not impicitly set
	// but those are needed to write a valid 
	// configuration	
	
	
	// Check each interface on all required fields
	// If they don't exists we add them
	
	
	return true;
}



//	==============================================
//	METHOD: networkingModule::writeconfiguration
//	==============================================
#ifdef __FLAVOR_LINUX_REDHAT // Only compile code on `redhat` 
bool networkingModule::writeconfiguration (const value &v)
{
	file 	f;
	
	// TODO: Remove all files from our var directory
	
	// Loop through each interface definition
	// and write the config file

	// TODO: write global network configuration file
	string fname;
	fname.printf ("%s/network", conf["config"]["varpath"].cval());
	
	if (f.openwrite (fname))
	{
		f.printf ("NETWORKING=%s\n",   v["networking"] == "true" ? "yes" : "no");
		f.printf ("HOSTNAME=%s\n",     v["hostname"].cval());
		f.printf ("GATEWAY=%s\n",      v["gateway"].cval());
		f.printf ("FORWARD_IPV4=%s\n", v["forward_ipv4"] == "true" ? "yes" : "no");
	}
	else
	{
		string error;
		error.printf ("Unable to write file: %s", fname.str());
		sendresult (moderr::err_writefile, error.str());
		return false;
	}


	// TODO: Install config and reload
	
	// remove all existing interfcae files
	value dir;
	dir = fs.ls (conf["config"]["system:interfaces"]);
	foreach (file, dir)
	{
		if (file.id().sval().globcmp ("ifcfg-eth*"))
		{
			authd.deletefile (file["path"].sval());
		}
	}

	// Install all new interface files
	foreach (net_if, v["Network:Interface"])
	{
		string filename;
		filename.printf ("ifcfg-%s", net_if.name());
	
		// Installation code here
		if( authd.installfile (filename, conf["config"]["system:interfaces"]))
		{
			// Do rollback
			authd.rollback ();

			sendresult( moderr::err_authdaemon, 
						"Error installing interface file(s)");
			
			return false;
		}
	}
	
	// Install the global network configuration
	if( authd.installfile ("network", conf["config"]["system:networkdir"]))
	{
		// Do rollback
		authd.rollback ();

		sendresult( moderr::err_authdaemon, 
					"Error installing network file");
		
		return false;
	}

	// Reload the service
	if ( authd.reloadservice (conf["config"]["system:servicename"]) )
	{
		// Do rollback
		authd.rollback ();

		sendresult( moderr::err_authdaemon, 
					"Error reloading networking service");
		
		return false;
	}	

	return true;
}
#endif


//	==============================================
//	METHOD: networkingModule::writeconfiguration
//	==============================================
#ifdef __FLAVOR_LINUX_DEBIAN // Only compile on `debian`
bool networkingModule::writeconfiguration (const value &v)
{
	string 	iffile, hfile;
	file	f;
	bool	gw_isset;
	
	// Set file name + path
	hfile.printf  ("%s/hostname", 	conf["config"]["varpath"].cval());
		
	// Write config file with 
	// the given hostname
	if (fs.exists (hfile))
		fs.rm (hfile);
	
	if (f.openwrite (hfile))
	{
		// Write hostname to configuration file
		f.puts (v["hostname"].sval());
		f.close ();
	}
	else
	{
		// error opening file for writing
		sendresult (moderr::err_writefile, "Unable to write hostname file");
		return false;
	}


	string ifdir;
	ifdir = conf["config"]["system:interfaces"];
	ifdir = ifdir.cutatlast ('/');

	// Install the global network configuration
	if( authd.installfile ("interfaces", ifdir))
	{
		// Do rollback
		authd.rollback ();

		sendresult( moderr::err_authdaemon, 
					"Error installing network file");
		
		return false;
	}

	// Install the global network configuration
	if( authd.installfile ("hostname", "/etc"))
	{
		// Do rollback
		authd.rollback ();

		sendresult( moderr::err_authdaemon, 
					"Error installing hostname file");
		
		return false;
	}
	

	// Reload the service
	if ( authd.reloadservice (conf["config"]["system:servicename"]) )
	{
		// Do rollback
		authd.rollback ();

		sendresult( moderr::err_authdaemon, 
					"Error reloading networking service");
		
		return false;
	}	


	return true;
}
#endif
	

//  =========================================================================
/// domainModule::checkconfig
//  =========================================================================
bool networkingModule::checkconfig (value &ibody)
{
	if (! ibody.exists ("Network"))
	{
		sendresult (moderr::err_value, 
						"No Network object found");
			return false;	
	}
	if (! ibody["Network"].exists("Network:Interface"))
	{
		sendresult (moderr::err_value, 
						"No Network Interfaces class found");
			return false;		
	}
	if (ibody["Network"]["Network:Interface"].count() == 0)
	{
		sendresult (moderr::err_value, 
				"No Network interface children found, at least 1 expected");
			return false;		
	}

	// Loopt through all interfaces
	foreach (v, ibody["Network"]["net"])
	{
		if (v["address"].ipval() == 0) {
			sendresult (moderr::err_value, 
						"Invalid ip address");
			return false;
		}
		if (v["netmask"].ipval() == 0) {
			sendresult (moderr::err_value, 
						"Invalid netmask");
			return false;
		}

		if (v.exists("network"))
		{
			if (v["address"].ipval() & v["netmask"].ipval() != 
				v["network"].ipval())
			{
				sendresult (moderr::err_value, 
							"Ip address not in range");
				return false;
			}
		}
		
		// all other requirements are already defined
		// in the module config
	
	}
	
	return true;
}



//  =========================================================================
/// Configuration watcher for the event log.
//  =========================================================================
bool networkingModule::confSystem (config::action act, keypath &kp,
                const value &nval, const value &oval)
{
	switch (act)
	{
		case config::isvalid:
			return true;

		case config::create:
			return true;		
	}

	return false;
}
