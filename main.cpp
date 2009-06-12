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
	
	string cmd = data["OpenCORE:Command"];
	string parentid = data["OpenCORE:Session"]["parentid"];
	string classid = data["OpenCORE:Session"]["classid"];
	
	caseselector (data["OpenCORE:Command"])
	{
		incaseof ("listobjects") :
			listobjects (classid, parentid);
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

string *networkingModule::getDefaultGatewayIPv4 (void)
{
	returnclass (string) res retain;
	string cmd = "/sbin/ip route show";
	systemprocess P (cmd);
	P.run ();
	while (! P.eof())
	{
		string ln = P.gets();
		if (ln.strncmp ("default via ", 12) == 0)
		{
			ln.cropafter ("via ");
			ln.cropat (" ");
			res = ln;
		}
	}
	P.close ();
	P.serialize ();
	return &res;
}

string *networkingModule::getDefaultGatewayIPv6 (void)
{
	returnclass (string) res retain;
	string cmd = "/sbin/ip -6 route show";
	systemprocess P (cmd);
	P.run ();
	while (! P.eof())
	{
		string ln = P.gets();
		if (ln.strncmp ("default via ", 12) == 0)
		{
			ln.cropafter ("via ");
			ln.cropat (" ");
			res = ln;
		}
	}
	P.close ();
	P.serialize ();
	return &res;
}

value *networkingModule::getInterfaceList (void)
{
	returnclass (value) res retain;
	statstring curif;
	unsigned int ifkey;
	string ifuuid;
	value panelips;
	
	file fips;
	if (fips.openread ("/etc/openpanel/ips.conf"))
	{
		while (! fips.eof())
		{
			string ln = fips.gets();
			value v = strutil::split (ln, ':');
			if (v.count() != 3)
			{
				statstring ifname = v[0];
				statstring addrtype = v[1];
				string addrmask = v[2];
				panelips[ifname][addrtype].newval() = addrmask;
			}
		}
		fips.close ();
	}
	
	bool isup = false;

	string cmd = "/sbin/ip addr show scope global";
	systemprocess P (cmd);
	P.run ();
	while (! P.eof())
	{
		string ln = P.gets();
		if (! ln) continue;
		if (ln[0] != ' ')
		{
			if (isup)
			{
				res[ifuuid]["metaid"] = curif;
			}
			ln.cropafter (": ");
			curif = ln.cutat (": <");
			ifkey = curif.key();
			ifuuid = "1face000-1337-1337-1337-0000%08x" %format (ifkey);

			ln.cropat ('>');
			value states = strutil::split (ln, ',');
			isup = false;
			foreach (st,states) { if (st == "UP") isup = true; }
		}
		else if (! isup) continue;
		else if (ln.strncmp ("    link/ether"))
		{
			ln.cropafter ("link/ether ");
			ln.cropat (" ");
			res[ifuuid]["mac"] = ln;
		}
		else if (ln.strncmp ("    link/"))
		{
			isup = false;
		}
		else if (ln.strncmp ("    inet "))
		{
			ln.cropafter ("inet ");
			ln.cropat (" ");
			statstring ipmask = ln;
			string mask = "/%s" %format (ln.cutafter ('/'));
			
			if (! res[ifuuid].exists ("address"))
			{
				res[ifuuid]["address"] = ln;
				res[ifuuid]["netmask"] = mask;
			}
			
			bool ispanelip = false;
			foreach (a, panelips[curif]["ipv4"])
			{
				if (a == ipmask)
				{
					ispanelip = true;
					break;
				}
			}
			
			string ipuuid = "1faceadd-1337-1337-1337-add4%08x" %format (ipmask.key());
			
			res[ifuuid]["ipv4"][ipuuid] =
				$("address", ln) ->
				$("netmask", mask) ->
				$("panelip", ispanelip);
		}
		else if (ln.strncmp ("    inet6 "))
		{
			ln.cropafter ("inet6 ");
			ln.cropat (" ");
			statstring ipmask = ln;
			string mask = "/%s" %format (ln.cutafter ('/'));

			bool ispanelip = false;
			foreach (a, panelips[curif]["ipv6"])
			{
				if (a == ipmask)
				{
					ispanelip = true;
					break;
				}
			}

			string ipuuid = "1faceadd-1337-1337-1337-add6%08x" %format (ipmask.key());

			res[ifuuid]["ipv6"][ipuuid] =
				$("address", ln) ->
				$("netmask", mask) ->
				$("panelip", ispanelip);
		}
	}
	P.close ();
	P.serialize ();

	if (isup)
	{
		res[ifuuid]["metaid"] = curif;
	}
	
	return &res;
}

value *networkingModule::objectlist (const string &classname, const statstring &parentid)
{
	returnclass (value) res retain;

	value iflist = getInterfaceList ();

	if (classname == "Network:Interface")
	{
		foreach (i, iflist)
		{
			value &into = res[i["metaid"].sval()];
			
			into = $("address", i["address"])->
			       $("id", i["metaid"])->
			       $("metaid", i["metaid"])->
			       $("uuid", i.id()) ->
			       $("netmask", i["netmask"])->
			       $("mac", i["mac"])->
			       $("type", "ethernet");
		}
		return &res;
	}
	
	if (classname == "Network:IPv4Address")
	{
		if (iflist.exists (parentid))
		{
			foreach (a, iflist[parentid]["ipv4"])
			{
				res[a.id()] = $("address",a["address"]) ->
							  $("netmask",a["netmask"]) ->
							  $("id",a.id()) ->
							  $("uuid",a.id());
			}
		}
		
		return &res;
	}

	if (classname == "Network:IPv6Address")
	{
		if (iflist.exists (parentid))
		{
			foreach (a, iflist[parentid]["ipv4"])
			{
				res[a.id()] = $("address",a["address"]) ->
							  $("v6netmask",a["netmask"]) ->
							  $("id",a.id()) ->
							  $("uuid",a.id());
			}
		}
		
		return &res;
	}
	
	return &res;
}

void networkingModule::listobjects (const string &classname, const statstring &parentid)
{
	value iflist = objectlist (classname, parentid);

	value out;
	iflist("type") = "class";
	out["objects"][classname] = iflist;
	
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
					"Error opening system configuration");
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
	netdata[general]["forward_ipv6"] = tmpdata["FORWARD_IPV6"];
	
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


	netdata[general]["gateway"] = getDefaultGatewayIPv4 ();
	netdata[general]["v6gateway"] = getDefaultGatewayIPv6 ();

	// Format the data to post back including the right 
	// parent classes e.a.
	value retval;
	
	retval.type ("opencore.module");
	
	retval["Network"]("type") = "class";
	retval["Network"]["net"] = netdata[general];

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
